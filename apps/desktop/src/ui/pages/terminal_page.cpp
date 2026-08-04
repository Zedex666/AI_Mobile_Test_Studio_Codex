#include "ui/pages/terminal_page.h"

#include "ui/common/widget_helpers.h"

#ifdef AI_MOBILE_TEST_STUDIO_HAS_WEB_TERMINAL
#include "ui/pages/terminal_bridge.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QWebEngineView>
#endif

#include <QApplication>
#include <QBoxLayout>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QFontDatabase>
#include <QHideEvent>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QPlainTextEdit>
#include <QResizeEvent>
#include <QScrollBar>
#include <QShowEvent>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStringConverter>
#include <QTabBar>
#include <QTextCursor>
#include <QTimer>
#include <QToolButton>
#include <QUuid>

#include <algorithm>
#include <functional>

#ifndef AI_MOBILE_TEST_STUDIO_HAS_WEB_TERMINAL
class TerminalView : public QPlainTextEdit
{
public:
    explicit TerminalView(QWidget *parent = nullptr)
        : QPlainTextEdit(parent)
        , m_decoder(QStringDecoder::Utf8)
    {
        setObjectName("TerminalViewport");
        setReadOnly(true);
        setFrameShape(QFrame::NoFrame);
        setLineWrapMode(QPlainTextEdit::NoWrap);
        setUndoRedoEnabled(false);
        setFocusPolicy(Qt::StrongFocus);
        setAttribute(Qt::WA_InputMethodEnabled, true);
        setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
        setFont(ui::appFont(11));
        m_lines.append(QString());
        m_renderTimer.setSingleShot(true);
        connect(&m_renderTimer, &QTimer::timeout, this, [this] {
            render();
        });
    }

    std::function<void(const QByteArray &)> writeRequested;
    std::function<void()> resetRequested;
    std::function<void(int, int)> resizeRequested;

    void setDeviceAvailable(bool available)
    {
        m_deviceAvailable = available;
    }

    void setSessionReady(bool ready)
    {
        m_sessionReady = ready;
        if (ready) {
            reportSize();
        }
    }

    bool sessionReady() const
    {
        return m_sessionReady;
    }

    void appendOutput(const QByteArray &data)
    {
        if (data.isEmpty()) {
            return;
        }
        processText(m_decoder(data));
        if (!m_renderTimer.isActive()) {
        m_renderTimer.start(33);
        }
    }

    void setOutputDeliveryEnabled(bool)
    {
    }

    void setBackgroundRenderingEnabled(bool)
    {
    }

    void appendStatus(const QString &message)
    {
        processText(QStringLiteral("\r\n[%1]\r\n").arg(message));
        render();
    }

    void clearTerminal()
    {
        m_renderTimer.stop();
        m_lines = {QString()};
        m_row = 0;
        m_column = 0;
        m_savedRow = 0;
        m_savedColumn = 0;
        m_parserState = ParserState::Normal;
        m_csi.clear();
        m_decoder.resetState();
        render();
    }

    void focusTerminal()
    {
        setFocus(Qt::OtherFocusReason);
    }

protected:
    void keyPressEvent(QKeyEvent *event) override
    {
        const Qt::KeyboardModifiers modifiers = event->modifiers();
        const bool control = modifiers.testFlag(Qt::ControlModifier);
        const bool shift = modifiers.testFlag(Qt::ShiftModifier);

        if (control && shift && event->key() == Qt::Key_C) {
            copy();
            event->accept();
            return;
        }
        if ((control && shift && event->key() == Qt::Key_V)
            || (shift && event->key() == Qt::Key_Insert)) {
            pasteClipboard();
            event->accept();
            return;
        }
        if (!m_sessionReady || !writeRequested) {
            if (event->matches(QKeySequence::SelectAll)) {
                selectAll();
            }
            event->accept();
            return;
        }

        QByteArray data;
        switch (event->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            data = QByteArrayLiteral("\r");
            break;
        case Qt::Key_Backspace:
            data = QByteArray(1, '\x7f');
            break;
        case Qt::Key_Tab:
            data = shift ? QByteArrayLiteral("\x1b[Z") : QByteArrayLiteral("\t");
            break;
        case Qt::Key_Escape:
            data = QByteArrayLiteral("\x1b");
            break;
        case Qt::Key_Up:
            data = QByteArrayLiteral("\x1b[A");
            break;
        case Qt::Key_Down:
            data = QByteArrayLiteral("\x1b[B");
            break;
        case Qt::Key_Right:
            data = QByteArrayLiteral("\x1b[C");
            break;
        case Qt::Key_Left:
            data = QByteArrayLiteral("\x1b[D");
            break;
        case Qt::Key_Home:
            data = QByteArrayLiteral("\x1b[H");
            break;
        case Qt::Key_End:
            data = QByteArrayLiteral("\x1b[F");
            break;
        case Qt::Key_Insert:
            data = QByteArrayLiteral("\x1b[2~");
            break;
        case Qt::Key_Delete:
            data = QByteArrayLiteral("\x1b[3~");
            break;
        case Qt::Key_PageUp:
            data = QByteArrayLiteral("\x1b[5~");
            break;
        case Qt::Key_PageDown:
            data = QByteArrayLiteral("\x1b[6~");
            break;
        default:
            break;
        }

        if (data.isEmpty() && control) {
            if (event->key() >= Qt::Key_A && event->key() <= Qt::Key_Z) {
                data.append(static_cast<char>(event->key() - Qt::Key_A + 1));
            } else if (event->key() == Qt::Key_Space || event->key() == Qt::Key_At) {
                data.append('\0');
            } else if (event->key() == Qt::Key_BracketLeft) {
                data.append('\x1b');
            } else if (event->key() == Qt::Key_Backslash) {
                data.append('\x1c');
            } else if (event->key() == Qt::Key_BracketRight) {
                data.append('\x1d');
            } else if (event->key() == Qt::Key_Underscore) {
                data.append('\x1f');
            }
        }
        if (data.isEmpty() && !control && !event->text().isEmpty()) {
            data = event->text().toUtf8();
            if (modifiers.testFlag(Qt::AltModifier)) {
                data.prepend('\x1b');
            }
        }

        if (!data.isEmpty()) {
            writeRequested(data);
        }
        event->accept();
    }

    void inputMethodEvent(QInputMethodEvent *event) override
    {
        if (m_sessionReady && writeRequested && !event->commitString().isEmpty()) {
            writeRequested(event->commitString().toUtf8());
        }
        event->accept();
    }

    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override
    {
        if (query == Qt::ImEnabled) {
            return m_sessionReady;
        }
        return QPlainTextEdit::inputMethodQuery(query);
    }

    void contextMenuEvent(QContextMenuEvent *event) override
    {
        QMenu menu(this);
        QAction *copyAction = menu.addAction(ui::text("复制"));
        copyAction->setEnabled(textCursor().hasSelection());
        connect(copyAction, &QAction::triggered, this, &QPlainTextEdit::copy);

        QAction *pasteAction = menu.addAction(ui::text("粘贴"));
        pasteAction->setEnabled(m_sessionReady && !QApplication::clipboard()->text().isEmpty());
        connect(pasteAction, &QAction::triggered, this, [this] {
            pasteClipboard();
        });

        QAction *selectAllAction = menu.addAction(ui::text("全选"));
        connect(selectAllAction, &QAction::triggered, this, &QPlainTextEdit::selectAll);
        menu.addSeparator();

        QAction *resetAction = menu.addAction(ui::text("重置终端"));
        resetAction->setEnabled(m_deviceAvailable && static_cast<bool>(resetRequested));
        connect(resetAction, &QAction::triggered, this, [this] {
            if (resetRequested) {
                resetRequested();
            }
        });

        QAction *clearAction = menu.addAction(ui::text("清空显示"));
        connect(clearAction, &QAction::triggered, this, [this] {
            clearTerminal();
            focusTerminal();
        });
        menu.exec(event->globalPos());
    }

    void resizeEvent(QResizeEvent *event) override
    {
        QPlainTextEdit::resizeEvent(event);
        reportSize();
    }

private:
    enum class ParserState {
        Normal,
        Escape,
        Csi,
        Osc,
        OscEscape
    };

    void pasteClipboard()
    {
        if (!m_sessionReady || !writeRequested) {
            return;
        }
        const QString text = QApplication::clipboard()->text();
        if (!text.isEmpty()) {
            writeRequested(text.toUtf8());
        }
    }

    void processText(const QString &text)
    {
        for (const QChar character : text) {
            switch (m_parserState) {
            case ParserState::Normal:
                processNormalCharacter(character);
                break;
            case ParserState::Escape:
                if (character == u'[') {
                    m_csi.clear();
                    m_parserState = ParserState::Csi;
                } else if (character == u']') {
                    m_parserState = ParserState::Osc;
                } else if (character == u'7') {
                    saveCursor();
                    m_parserState = ParserState::Normal;
                } else if (character == u'8') {
                    restoreCursor();
                    m_parserState = ParserState::Normal;
                } else {
                    m_parserState = ParserState::Normal;
                }
                break;
            case ParserState::Csi:
                if (character.unicode() >= 0x40 && character.unicode() <= 0x7e) {
                    processCsi(character);
                    m_parserState = ParserState::Normal;
                } else if (m_csi.size() < 64) {
                    m_csi.append(character);
                }
                break;
            case ParserState::Osc:
                if (character == u'\a') {
                    m_parserState = ParserState::Normal;
                } else if (character == u'\x1b') {
                    m_parserState = ParserState::OscEscape;
                }
                break;
            case ParserState::OscEscape:
                m_parserState = character == u'\\' ? ParserState::Normal : ParserState::Osc;
                break;
            }
        }
        trimScrollback();
    }

    void processNormalCharacter(QChar character)
    {
        if (character == u'\x1b') {
            m_parserState = ParserState::Escape;
        } else if (character == u'\r') {
            m_column = 0;
        } else if (character == u'\n') {
            ++m_row;
            m_column = 0;
            ensureRow(m_row);
        } else if (character == u'\b') {
            m_column = std::max(0, m_column - 1);
        } else if (character == u'\t') {
            const int spaces = 8 - (m_column % 8);
            for (int index = 0; index < spaces; ++index) {
                writeCharacter(u' ');
            }
        } else if (character.unicode() >= 0x20 && character != u'\x7f') {
            writeCharacter(character);
        }
    }

    void writeCharacter(QChar character)
    {
        ensureRow(m_row);
        QString &line = m_lines[m_row];
        if (line.size() < m_column) {
            line.append(QString(m_column - line.size(), u' '));
        }
        if (m_column < line.size()) {
            line[m_column] = character;
        } else {
            line.append(character);
        }
        ++m_column;
    }

    void processCsi(QChar command)
    {
        QString parameters = m_csi;
        while (!parameters.isEmpty()
               && (parameters.front() == u'?' || parameters.front() == u'>'
                   || parameters.front() == u'!')) {
            parameters.removeFirst();
        }
        const QStringList parts = parameters.split(u';', Qt::KeepEmptyParts);
        const auto value = [&parts](int index, int fallback, bool zeroMeansFallback = true) {
            if (index >= parts.size() || parts.at(index).isEmpty()) {
                return fallback;
            }
            bool ok = false;
            const int parsed = parts.at(index).toInt(&ok);
            if (!ok || (zeroMeansFallback && parsed == 0)) {
                return fallback;
            }
            return parsed;
        };

        switch (command.unicode()) {
        case 'A':
            m_row = std::max(0, m_row - value(0, 1));
            break;
        case 'B':
            m_row += value(0, 1);
            ensureRow(m_row);
            break;
        case 'C':
            m_column += value(0, 1);
            break;
        case 'D':
            m_column = std::max(0, m_column - value(0, 1));
            break;
        case 'E':
            m_row += value(0, 1);
            m_column = 0;
            ensureRow(m_row);
            break;
        case 'F':
            m_row = std::max(0, m_row - value(0, 1));
            m_column = 0;
            break;
        case 'G':
            m_column = std::max(0, value(0, 1) - 1);
            break;
        case 'H':
        case 'f':
            m_row = std::max(0, value(0, 1) - 1);
            m_column = std::max(0, value(1, 1) - 1);
            ensureRow(m_row);
            break;
        case 'J': {
            const int mode = value(0, 0, false);
            if (mode == 2 || mode == 3) {
                m_lines = {QString()};
                m_row = 0;
                m_column = 0;
            } else if (mode == 0) {
                eraseLineFromCursor();
                while (m_lines.size() > m_row + 1) {
                    m_lines.removeLast();
                }
            }
            break;
        }
        case 'K': {
            ensureRow(m_row);
            QString &line = m_lines[m_row];
            const int mode = value(0, 0, false);
            if (mode == 0) {
                eraseLineFromCursor();
            } else if (mode == 1) {
                const int count = std::min(m_column + 1, static_cast<int>(line.size()));
                line.replace(0, count, QString(count, u' '));
            } else if (mode == 2) {
                line.clear();
            }
            break;
        }
        case 'P': {
            ensureRow(m_row);
            QString &line = m_lines[m_row];
            if (m_column < line.size()) {
                line.remove(m_column, value(0, 1));
            }
            break;
        }
        case 'X': {
            ensureRow(m_row);
            QString &line = m_lines[m_row];
            const int remaining = std::max(0, static_cast<int>(line.size()) - m_column);
            const int count = std::min(value(0, 1), remaining);
            if (count > 0) {
                line.replace(m_column, count, QString(count, u' '));
            }
            break;
        }
        case 's':
            saveCursor();
            break;
        case 'u':
            restoreCursor();
            break;
        default:
            break;
        }
    }

    void eraseLineFromCursor()
    {
        ensureRow(m_row);
        QString &line = m_lines[m_row];
        if (m_column < line.size()) {
            line.truncate(m_column);
        }
    }

    void ensureRow(int row)
    {
        while (m_lines.size() <= row) {
            m_lines.append(QString());
        }
    }

    void saveCursor()
    {
        m_savedRow = m_row;
        m_savedColumn = m_column;
    }

    void restoreCursor()
    {
        m_row = std::max(0, m_savedRow);
        m_column = std::max(0, m_savedColumn);
        ensureRow(m_row);
    }

    void trimScrollback()
    {
        constexpr int maximumLines = 5000;
        constexpr int trimCount = 500;
        if (m_lines.size() <= maximumLines) {
            return;
        }
        m_lines.erase(m_lines.begin(), m_lines.begin() + trimCount);
        m_row = std::max(0, m_row - trimCount);
        m_savedRow = std::max(0, m_savedRow - trimCount);
    }

    void render()
    {
        QScrollBar *scroll = verticalScrollBar();
        const bool followOutput = scroll->value() >= scroll->maximum() - 2;
        const int previousValue = scroll->value();
        setPlainText(m_lines.join(u'\n'));
        int cursorPosition = 0;
        const int cursorRow = std::min(m_row, static_cast<int>(m_lines.size()) - 1);
        for (int row = 0; row < cursorRow; ++row) {
            cursorPosition += m_lines.at(row).size() + 1;
        }
        cursorPosition += std::min(m_column, static_cast<int>(m_lines.at(cursorRow).size()));
        QTextCursor cursor(document());
        cursor.setPosition(cursorPosition);
        setTextCursor(cursor);
        if (followOutput) {
            scroll->setValue(scroll->maximum());
        } else {
            scroll->setValue(std::min(previousValue, scroll->maximum()));
        }
    }

    void reportSize()
    {
        if (!m_sessionReady || !resizeRequested) {
            return;
        }
        const QFontMetrics metrics(font());
        const int characterWidth = std::max(1, metrics.horizontalAdvance(QLatin1Char('M')));
        const int lineHeight = std::max(1, metrics.height());
        const int columns = std::max(2, viewport()->width() / characterWidth);
        const int rows = std::max(2, viewport()->height() / lineHeight);
        if (columns == m_lastColumns && rows == m_lastRows) {
            return;
        }
        m_lastColumns = columns;
        m_lastRows = rows;
        resizeRequested(columns, rows);
    }

    QStringDecoder m_decoder;
    QTimer m_renderTimer;
    QStringList m_lines;
    QString m_csi;
    ParserState m_parserState = ParserState::Normal;
    int m_row = 0;
    int m_column = 0;
    int m_savedRow = 0;
    int m_savedColumn = 0;
    int m_lastColumns = 0;
    int m_lastRows = 0;
    bool m_deviceAvailable = false;
    bool m_sessionReady = false;
};
#else
class TerminalWebPage : public QWebEnginePage
{
public:
    explicit TerminalWebPage(const QString &contentRoot, QObject *parent = nullptr)
        : QWebEnginePage(parent)
        , m_contentRoot(QFileInfo(contentRoot).canonicalFilePath())
    {
    }

protected:
    bool acceptNavigationRequest(const QUrl &url,
                                 NavigationType type,
                                 bool isMainFrame) override
    {
        Q_UNUSED(type)
        if (!isMainFrame) {
            return true;
        }
        if (!url.isLocalFile() || m_contentRoot.isEmpty()) {
            return false;
        }
        const QString target = QFileInfo(url.toLocalFile()).canonicalFilePath();
        if (target.isEmpty()) {
            return false;
        }
        const QString relativePath = QDir::fromNativeSeparators(
            QDir(m_contentRoot).relativeFilePath(target));
        return !QDir::isAbsolutePath(relativePath)
            && relativePath != QStringLiteral("..")
            && !relativePath.startsWith(QStringLiteral("../"));
    }

private:
    QString m_contentRoot;
};

class TerminalView : public QWidget
{
public:
    explicit TerminalView(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_bridge(new TerminalBridge(this))
    {
        setObjectName("TerminalViewport");
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        m_webView = new QWebEngineView;
        m_webView->setContextMenuPolicy(Qt::CustomContextMenu);
        m_webView->settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows,
                                            false);
        m_webView->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls,
                                            false);
        m_webView->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls,
                                            true);
        m_webView->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, false);
        m_webView->settings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, false);

        const QString contentRoot = QDir(QCoreApplication::applicationDirPath())
                                        .filePath(QStringLiteral("runtime/terminal-web"));
        auto *page = new TerminalWebPage(contentRoot, m_webView);
        auto *channel = new QWebChannel(page);
        channel->registerObject(QStringLiteral("terminalBridge"), m_bridge);
        page->setWebChannel(channel);
        m_webView->setPage(page);
        m_webView->load(QUrl::fromLocalFile(
            QDir(contentRoot).filePath(QStringLiteral("index.html"))));
        layout->addWidget(m_webView);

        m_bridge->writeRequested = [this](const QByteArray &data) {
            if (writeRequested) {
                writeRequested(data);
            }
        };
        m_bridge->resizeRequested = [this](int columns, int rows) {
            if (resizeRequested) {
                resizeRequested(columns, rows);
            }
        };
        connect(m_webView,
                &QWidget::customContextMenuRequested,
                this,
                [this](const QPoint &position) {
                    showContextMenu(position);
                });
    }

    std::function<void(const QByteArray &)> writeRequested;
    std::function<void()> resetRequested;
    std::function<void(int, int)> resizeRequested;

    void setDeviceAvailable(bool available)
    {
        m_backendAvailable = available;
    }

    void setSessionReady(bool ready)
    {
        m_sessionReady = ready;
        m_bridge->setSessionReady(ready);
    }

    bool sessionReady() const
    {
        return m_sessionReady;
    }

    void appendOutput(const QByteArray &data)
    {
        m_bridge->sendOutput(data);
    }

    void setOutputDeliveryEnabled(bool enabled)
    {
        QWebEnginePage *page = m_webView->page();
        page->setLifecycleState(QWebEnginePage::LifecycleState::Active);
        page->setVisible(true);
        m_bridge->setDeliveryEnabled(enabled);
        if (!enabled) {
            return;
        }
        page->runJavaScript(QStringLiteral(
            "requestAnimationFrame(()=>{void document.body.offsetWidth;"
            "window.dispatchEvent(new Event('resize'));})"));
    }

    void setBackgroundRenderingEnabled(bool enabled)
    {
        setOutputDeliveryEnabled(enabled);
    }

    void appendStatus(const QString &message)
    {
        m_bridge->sendStatus(message);
    }

    void clearTerminal()
    {
        m_bridge->clear();
    }

    void focusTerminal()
    {
        m_webView->setFocus(Qt::OtherFocusReason);
        m_bridge->focus();
    }

private:
    void showContextMenu(const QPoint &position)
    {
        QMenu menu(this);
        QAction *copyAction = menu.addAction(ui::text("复制"));
        connect(copyAction, &QAction::triggered, this, [this] {
            m_webView->page()->runJavaScript(
                QStringLiteral("window.terminalHost.copySelection()"));
        });
        QAction *pasteAction = menu.addAction(ui::text("粘贴"));
        pasteAction->setEnabled(m_sessionReady
                                && !QApplication::clipboard()->text().isEmpty());
        connect(pasteAction, &QAction::triggered, m_bridge, &TerminalBridge::pasteClipboard);
        menu.addSeparator();
        QAction *resetAction = menu.addAction(ui::text("重置终端"));
        resetAction->setEnabled(m_backendAvailable && static_cast<bool>(resetRequested));
        connect(resetAction, &QAction::triggered, this, [this] {
            if (resetRequested) {
                resetRequested();
            }
        });
        QAction *clearAction = menu.addAction(ui::text("清空显示"));
        connect(clearAction, &QAction::triggered, this, [this] {
            clearTerminal();
            focusTerminal();
        });
        menu.exec(m_webView->mapToGlobal(position));
    }

    TerminalBridge *m_bridge = nullptr;
    QWebEngineView *m_webView = nullptr;
    bool m_backendAvailable = false;
    bool m_sessionReady = false;
};
#endif

namespace {

QToolButton *makeTerminalButton(const QString &text, const QString &tooltip)
{
    auto *button = new QToolButton;
    button->setObjectName("TerminalToolButton");
    button->setText(text);
    button->setToolTip(tooltip);
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedSize(38, 38);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setFont(ui::appFont(15, QFont::DemiBold));
    return button;
}

} // namespace

TerminalPage::TerminalPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("TerminalPage");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *toolbar = new QWidget;
    toolbar->setObjectName("TerminalToolbar");
    toolbar->setFixedHeight(44);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(8, 0, 8, 0);
    toolbarLayout->setSpacing(4);

    m_tabBar = new QTabBar;
    m_tabBar->setObjectName("TerminalTabs");
    m_tabBar->setTabsClosable(false);
    m_tabBar->setMovable(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setUsesScrollButtons(true);
    m_tabBar->setElideMode(Qt::ElideRight);
    m_tabBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_tabBar->setMaximumWidth(720);
    toolbarLayout->addWidget(m_tabBar, 1);

    m_addButton = makeTerminalButton(ui::text("+"), ui::text("新建终端"));
    toolbarLayout->addWidget(m_addButton);
    toolbarLayout->addStretch();

    m_shortcutsButton = makeTerminalButton(ui::text("≡"), ui::text("快捷命令"));
    auto *shortcutMenu = new QMenu(m_shortcutsButton);
    const QList<QPair<QString, QByteArray>> shortcuts = {
        {ui::text("重启设备"), QByteArrayLiteral("reboot\n")},
        {ui::text("重启到 Recovery"), QByteArrayLiteral("reboot recovery\n")},
        {ui::text("重启到 Bootloader"), QByteArrayLiteral("reboot bootloader\n")},
        {ui::text("内存信息"), QByteArrayLiteral("dumpsys meminfo\n")},
        {ui::text("电池信息"), QByteArrayLiteral("dumpsys battery\n")},
        {ui::text("启动 Shizuku"),
         QByteArrayLiteral("sh /sdcard/Android/data/moe.shizuku.privileged.api/start.sh\n")},
        {ui::text("授权 GKD"),
         QByteArrayLiteral("pm grant li.songe.gkd android.permission.WRITE_SECURE_SETTINGS; "
                           "appops set li.songe.gkd ACCESS_RESTRICTED_SETTINGS allow\n")},
    };
    for (const auto &shortcut : shortcuts) {
        QAction *action = shortcutMenu->addAction(shortcut.first);
        connect(action, &QAction::triggered, this, [this, command = shortcut.second] {
            runShortcut(command);
        });
    }
    connect(m_shortcutsButton, &QToolButton::clicked, this, [this, shortcutMenu] {
        shortcutMenu->popup(
            m_shortcutsButton->mapToGlobal(QPoint(0, m_shortcutsButton->height())));
    });
    toolbarLayout->addWidget(m_shortcutsButton);
    layout->addWidget(toolbar);

    m_terminalStack = new QStackedWidget;
    m_terminalStack->setObjectName("TerminalStack");
    layout->addWidget(m_terminalStack, 1);

    connect(m_addButton, &QToolButton::clicked, this, [this] {
        showNewTerminalMenu(m_addButton);
    });
    connect(m_tabBar, &QTabBar::currentChanged, this, &TerminalPage::selectTerminal);
    addTerminal(QStringLiteral("adb-shell"));
    updateControls();
}

void TerminalPage::setDeviceConnected(bool connected, const QString &serial)
{
    const bool deviceChanged = connected != m_connected || (connected && serial != m_serial);
    m_connected = connected;
    m_serial = connected ? serial : QString();
    if (!deviceChanged) {
        updateControls();
        return;
    }

    for (auto iterator = m_views.cbegin(); iterator != m_views.cend(); ++iterator) {
        if (kindForSession(iterator.key()) != QStringLiteral("adb-shell")) {
            continue;
        }
        TerminalView *view = iterator.value();
        view->setDeviceAvailable(connected);
        view->setSessionReady(false);
        if (connected) {
            view->clearTerminal();
            emit sessionCreateRequested(iterator.key(), QStringLiteral("adb-shell"));
        } else {
            view->appendStatus(ui::text("设备连接已断开"));
        }
    }
    updateControls();
}

void TerminalPage::activate()
{
    selectTerminal(m_tabBar->currentIndex());
    if (TerminalView *view = viewForSession(currentSessionId())) {
        QTimer::singleShot(0, view, [view] {
            view->focusTerminal();
        });
    }
}

void TerminalPage::preloadOpenCode()
{
    if (!m_backgroundRenderSessionId.isEmpty()) {
        return;
    }
    m_backgroundPreloading = true;
    addTerminal(QStringLiteral("opencode"));
    m_backgroundRenderSessionId = currentSessionId();
    if (TerminalView *view = viewForSession(m_backgroundRenderSessionId)) {
        view->setBackgroundRenderingEnabled(true);
    }
}

void TerminalPage::finishBackgroundPreload()
{
    m_backgroundPreloading = false;
    const QString sessionId = m_backgroundRenderSessionId;
    m_backgroundRenderSessionId.clear();
    if (TerminalView *view = viewForSession(sessionId)) {
        view->setOutputDeliveryEnabled(isVisible() && sessionId == currentSessionId());
    }
}

void TerminalPage::handleSessionStarted(const QString &sessionId)
{
    if (TerminalView *view = viewForSession(sessionId)) {
        view->setSessionReady(true);
        if (sessionId == currentSessionId() && isVisible() && !m_backgroundPreloading) {
            view->focusTerminal();
        }
    }
    updateControls();
}

void TerminalPage::handleSessionOutput(const QString &sessionId, const QByteArray &data)
{
    if (TerminalView *view = viewForSession(sessionId)) {
        view->appendOutput(data);
    }
}

void TerminalPage::handleSessionFailed(const QString &sessionId, const QString &message)
{
    if (TerminalView *view = viewForSession(sessionId)) {
        view->setSessionReady(false);
        view->appendStatus(ui::text("终端连接失败：%1").arg(message));
    }
    updateControls();
}

void TerminalPage::handleSessionClosed(const QString &sessionId,
                                       bool expected,
                                       const QString &message)
{
    if (TerminalView *view = viewForSession(sessionId)) {
        view->setSessionReady(false);
        if (!expected) {
            view->appendStatus(message.isEmpty() ? ui::text("远程 shell 已退出") : message);
        }
    }
    updateControls();
}

void TerminalPage::showNewTerminalMenu(QWidget *anchor)
{
    QMenu menu(this);
    QAction *openCodeAction = menu.addAction(ui::text("OpenCode"));
    openCodeAction->setToolTip(ui::text("在当前工作区启动 OpenCode TUI"));
    connect(openCodeAction, &QAction::triggered, this, [this] {
        addTerminal(QStringLiteral("opencode"));
    });

    QAction *adbAction = menu.addAction(ui::text("ADB Shell"));
    adbAction->setEnabled(m_connected);
    adbAction->setToolTip(m_connected ? ui::text("连接到当前 Android 设备")
                                     : ui::text("连接 Android 设备后可用"));
    connect(adbAction, &QAction::triggered, this, [this] {
        addTerminal(QStringLiteral("adb-shell"));
    });

    menu.exec(anchor->mapToGlobal(QPoint(0, anchor->height())));
}

void TerminalPage::addTerminal(const QString &kindId)
{
    const bool openCode = kindId == QStringLiteral("opencode");
    const QString sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto *view = new TerminalView;
    view->setDeviceAvailable(openCode || m_connected);
    view->writeRequested = [this, sessionId](const QByteArray &data) {
        emit sessionWriteRequested(sessionId, data);
    };
    view->resizeRequested = [this, sessionId](int columns, int rows) {
        emit sessionResizeRequested(sessionId, columns, rows);
    };
    view->resetRequested = [this, sessionId, kindId, view] {
        if (kindId == QStringLiteral("adb-shell") && !m_connected) {
            return;
        }
        view->clearTerminal();
        view->setSessionReady(false);
        emit sessionRestartRequested(sessionId);
    };
    if (!openCode && !m_connected) {
        view->appendStatus(ui::text("连接 Android 设备后可使用终端"));
    }

    m_views.insert(sessionId, view);
    m_sessionKinds.insert(sessionId, kindId);
    m_terminalStack->addWidget(view);
    const QString title = openCode ? ui::text("OpenCode %1").arg(m_nextOpenCodeNumber++)
                                   : ui::text("ADB Shell %1").arg(m_nextTerminalNumber++);
    const int index = m_tabBar->addTab(title);
    m_tabBar->setTabData(index, sessionId);
    auto *closeButton = new QToolButton(m_tabBar);
    closeButton->setObjectName("TerminalTabCloseButton");
    closeButton->setText(ui::text("×"));
    closeButton->setToolTip(ui::text("关闭终端"));
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setFixedSize(18, 18);
    closeButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_tabBar->setTabButton(index, QTabBar::RightSide, closeButton);
    connect(closeButton, &QToolButton::clicked, this, [this, closeButton] {
        for (int tabIndex = 0; tabIndex < m_tabBar->count(); ++tabIndex) {
            if (m_tabBar->tabButton(tabIndex, QTabBar::RightSide) == closeButton) {
                closeTerminal(tabIndex);
                return;
            }
        }
    });
    m_tabBar->setCurrentIndex(index);
    m_terminalStack->setCurrentWidget(view);

    if (openCode || m_connected) {
        emit sessionCreateRequested(sessionId, kindId);
    }
    updateControls();
}

void TerminalPage::closeTerminal(int index)
{
    if (index < 0 || index >= m_tabBar->count()) {
        return;
    }
    const QString sessionId = m_tabBar->tabData(index).toString();
    TerminalView *view = m_views.take(sessionId);
    if (sessionId == m_backgroundRenderSessionId) {
        m_backgroundRenderSessionId.clear();
    }
    m_sessionKinds.remove(sessionId);
    emit sessionCloseRequested(sessionId);
    m_tabBar->removeTab(index);
    if (view != nullptr) {
        m_terminalStack->removeWidget(view);
        view->deleteLater();
    }
    selectTerminal(m_tabBar->currentIndex());
    updateControls();
}

void TerminalPage::selectTerminal(int index)
{
    const QString selectedSessionId = index >= 0 && index < m_tabBar->count()
        ? m_tabBar->tabData(index).toString()
        : QString();
    for (auto iterator = m_views.cbegin(); iterator != m_views.cend(); ++iterator) {
        iterator.value()->setOutputDeliveryEnabled(
            iterator.key() == m_backgroundRenderSessionId
            || (isVisible() && iterator.key() == selectedSessionId));
    }
    if (index < 0 || index >= m_tabBar->count()) {
        updateControls();
        return;
    }
    if (TerminalView *view = viewForSession(selectedSessionId)) {
        m_terminalStack->setCurrentWidget(view);
        if (isVisible() && !m_backgroundPreloading) {
            view->focusTerminal();
        }
    }
    updateControls();
}

void TerminalPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    selectTerminal(m_tabBar->currentIndex());
}

void TerminalPage::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    for (auto iterator = m_views.cbegin(); iterator != m_views.cend(); ++iterator) {
        iterator.value()->setOutputDeliveryEnabled(
            iterator.key() == m_backgroundRenderSessionId);
    }
}

void TerminalPage::runShortcut(const QByteArray &command)
{
    const QString sessionId = currentSessionId();
    TerminalView *view = viewForSession(sessionId);
    if (!m_connected || kindForSession(sessionId) != QStringLiteral("adb-shell")
        || view == nullptr || !view->sessionReady()) {
        return;
    }
    emit sessionWriteRequested(sessionId, command);
    view->focusTerminal();
}

QString TerminalPage::currentSessionId() const
{
    const int index = m_tabBar->currentIndex();
    return index >= 0 ? m_tabBar->tabData(index).toString() : QString();
}

QString TerminalPage::kindForSession(const QString &sessionId) const
{
    return m_sessionKinds.value(sessionId);
}

TerminalView *TerminalPage::viewForSession(const QString &sessionId) const
{
    return m_views.value(sessionId, nullptr);
}

void TerminalPage::updateControls()
{
    m_addButton->setEnabled(true);
    TerminalView *view = viewForSession(currentSessionId());
    m_shortcutsButton->setEnabled(
        m_connected && kindForSession(currentSessionId()) == QStringLiteral("adb-shell")
        && view != nullptr && view->sessionReady());
}
