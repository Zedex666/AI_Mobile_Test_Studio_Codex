#include "ui/pages/layout_page.h"

#include "ui/common/app_preferences.h"
#include "ui/common/widget_helpers.h"

#ifdef AI_MOBILE_TEST_STUDIO_HAS_WEBENGINE
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QWebEngineDownloadRequest>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineView>
#endif

#include <QBoxLayout>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTabWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QUrl>

#include <utility>

namespace {

#ifdef AI_MOBILE_TEST_STUDIO_HAS_WEBENGINE
class ExternalLinkPage final : public QWebEnginePage
{
public:
    explicit ExternalLinkPage(QObject *parent = nullptr)
        : QWebEnginePage(parent)
    {
    }

protected:
    bool acceptNavigationRequest(const QUrl &url,
                                 NavigationType type,
                                 bool isMainFrame) override
    {
        Q_UNUSED(type)
        if (isMainFrame && url.scheme() != QStringLiteral("about")) {
            QDesktopServices::openUrl(url);
            return false;
        }
        return true;
    }
};

class AppiumInspectorPage final : public QWebEnginePage
{
public:
    explicit AppiumInspectorPage(const QString &contentRoot, QObject *parent = nullptr)
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
        if (!url.isLocalFile()) {
            QDesktopServices::openUrl(url);
            return false;
        }
        const QString target = QFileInfo(url.toLocalFile()).canonicalFilePath();
        if (target.isEmpty() || m_contentRoot.isEmpty()) {
            return false;
        }
        const QString relativePath = QDir::fromNativeSeparators(
            QDir(m_contentRoot).relativeFilePath(target));
        return !QDir::isAbsolutePath(relativePath)
            && relativePath != QStringLiteral("..")
            && !relativePath.startsWith(QStringLiteral("../"));
    }

    QWebEnginePage *createWindow(WebWindowType type) override
    {
        Q_UNUSED(type)
        return new ExternalLinkPage(this);
    }

private:
    QString m_contentRoot;
};

QString javaScriptString(const QString &value)
{
    const QByteArray json = QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact);
    return QString::fromUtf8(json.mid(1, json.size() - 2));
}

void applyInspectorFont(QWebEnginePage *page, ui::AppLanguage language)
{
    if (page == nullptr) {
        return;
    }

    const QDir runtime(QCoreApplication::applicationDirPath());
    const bool english = language == ui::AppLanguage::English;
    const QString family = english ? QStringLiteral("AI JetBrains Mono")
                                   : QStringLiteral("AI LXGW WenKai");
    const QString regularPath = runtime.filePath(
        english
            ? QStringLiteral(
                  "runtime/fonts/us/JetBrainsMono-2.304/fonts/ttf/JetBrainsMono-Regular.ttf")
            : QStringLiteral("runtime/fonts/cn/LXGWWenKai-Regular.ttf"));
    const QString mediumPath = runtime.filePath(
        english
            ? QStringLiteral(
                  "runtime/fonts/us/JetBrainsMono-2.304/fonts/ttf/JetBrainsMono-Medium.ttf")
            : QStringLiteral("runtime/fonts/cn/LXGWWenKai-Medium.ttf"));
    const QString boldPath = runtime.filePath(
        english
            ? QStringLiteral(
                  "runtime/fonts/us/JetBrainsMono-2.304/fonts/ttf/JetBrainsMono-Bold.ttf")
            : QStringLiteral("runtime/fonts/cn/LXGWWenKai-Medium.ttf"));
    const auto fontUrl = [](const QString &path) {
        return QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded);
    };
    const QString css = QStringLiteral(
                            "@font-face{font-family:'%1';src:url('%2') format('truetype');"
                            "font-style:normal;font-weight:400;font-display:block;}"
                            "@font-face{font-family:'%1';src:url('%3') format('truetype');"
                            "font-style:normal;font-weight:500 600;font-display:block;}"
                            "@font-face{font-family:'%1';src:url('%4') format('truetype');"
                            "font-style:normal;font-weight:700 900;font-display:block;}"
                            "html,body,#root,#root *{font-family:'%1' !important;"
                            "letter-spacing:0 !important;}")
                            .arg(family,
                                 fontUrl(regularPath),
                                 fontUrl(mediumPath),
                                 fontUrl(boldPath));
    const QString script = QStringLiteral(
                               "(()=>{let style=document.getElementById("
                               "'ai-mobile-test-studio-font');"
                               "if(!style){style=document.createElement('style');"
                               "style.id='ai-mobile-test-studio-font';"
                               "document.head.appendChild(style);}style.textContent=%1;})()")
                               .arg(javaScriptString(css));
    page->runJavaScript(script);
}
#endif

QLabel *label(const QString &value,
              int size = 10,
              QFont::Weight weight = QFont::Normal,
              const QString &color = QStringLiteral("#172033"))
{
    auto *result = new QLabel(value);
    result->setFont(ui::appFont(size, weight));
    result->setStyleSheet(QStringLiteral("color:%1;").arg(color));
    return result;
}

QFrame *card(const QString &name = QStringLiteral("LayoutCard"))
{
    return ui::makePanel(name);
}

QPushButton *toolButton(const QString &text,
                        const QString &tooltip = QString(),
                        bool primary = false)
{
    auto *button = new QPushButton(text);
    button->setObjectName(primary ? QStringLiteral("LayoutPrimaryButton")
                                  : QStringLiteral("LayoutToolButton"));
    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip(tooltip);
    button->setMinimumHeight(34);
    button->setFont(ui::appFont(9, primary ? QFont::DemiBold : QFont::Normal));
    return button;
}

QFrame *inputGroup(const QString &caption, QLineEdit **editor)
{
    auto *frame = card(QStringLiteral("LayoutInputGroup"));
    auto *layout = new QHBoxLayout(frame);
    layout->setContentsMargins(12, 0, 8, 0);
    layout->setSpacing(8);
    layout->addWidget(label(caption, 9, QFont::DemiBold, QStringLiteral("#293243")));
    *editor = new QLineEdit;
    (*editor)->setMinimumHeight(34);
    (*editor)->setFrame(false);
    (*editor)->setFont(ui::appFont(10));
    layout->addWidget(*editor, 1);
    return frame;
}

class PhonePreview final : public QWidget
{
public:
    explicit PhonePreview(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumSize(300, 640);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor(QStringLiteral("#fafafa")));

        const int phoneWidth = qMin(290, width() - 32);
        const int phoneHeight = qMin(600, height() - 24);
        const QRect phone((width() - phoneWidth) / 2, (height() - phoneHeight) / 2,
                          phoneWidth, phoneHeight);
        painter.setPen(QPen(QColor(QStringLiteral("#7d8794")), 4));
        painter.setBrush(QColor(QStringLiteral("#20232a")));
        painter.drawRoundedRect(phone, 22, 22);

        const QRect screen = phone.adjusted(10, 10, -10, -10);
        QLinearGradient wallpaper(screen.topLeft(), screen.bottomRight());
        wallpaper.setColorAt(0.0, QColor(QStringLiteral("#21125e")));
        wallpaper.setColorAt(0.45, QColor(QStringLiteral("#4936b7")));
        wallpaper.setColorAt(1.0, QColor(QStringLiteral("#d588c7")));
        painter.setPen(Qt::NoPen);
        painter.setBrush(wallpaper);
        painter.drawRoundedRect(screen, 14, 14);

        painter.setPen(Qt::white);
        painter.setFont(ui::appFont(9, QFont::DemiBold));
        painter.drawText(screen.adjusted(12, 8, -12, 0), Qt::AlignLeft, QStringLiteral("6:24"));
        painter.drawText(screen.adjusted(12, 8, -12, 0), Qt::AlignRight,
                         QStringLiteral("5G 96%"));

        const QStringList apps = {QStringLiteral("手机管家"), QStringLiteral("音乐"), QStringLiteral("QQ"),
                                  QStringLiteral("浏览器"), QStringLiteral("工具"), QStringLiteral("Play"),
                                  QStringLiteral("Xesim"), QStringLiteral("Edge"), QStringLiteral("相机")};
        const int columns = 3;
        const int cellWidth = (screen.width() - 32) / columns;
        for (int index = 0; index < apps.size(); ++index) {
            const int row = index / columns;
            const int column = index % columns;
            const QRect iconRect(screen.left() + 12 + column * cellWidth,
                                 screen.top() + 74 + row * 76, 42, 42);
            painter.setBrush(index % 2 == 0 ? QColor(QStringLiteral("#21d46f"))
                                           : QColor(QStringLiteral("#f5f6fb")));
            painter.drawRoundedRect(iconRect, 12, 12);
            painter.setPen(index % 2 == 0 ? Qt::white : QColor(QStringLiteral("#23314c")));
            painter.setFont(ui::appFont(8, QFont::DemiBold));
            painter.drawText(iconRect, Qt::AlignCenter, index == 2 ? QStringLiteral("QQ") : QStringLiteral("●"));
            painter.setPen(Qt::white);
            painter.setFont(ui::appFont(7));
            painter.drawText(QRect(iconRect.left() - 10, iconRect.bottom() + 4,
                                   iconRect.width() + 20, 18), Qt::AlignCenter, apps[index]);
        }

        painter.setBrush(QColor(255, 255, 255, 185));
        painter.drawRoundedRect(QRect(screen.left() + 74, screen.bottom() - 52, 110, 25), 12, 12);
        painter.setPen(QColor(QStringLiteral("#443d73")));
        painter.setFont(ui::appFont(8));
        painter.drawText(QRect(screen.left() + 74, screen.bottom() - 52, 110, 25),
                         Qt::AlignCenter, QStringLiteral("⌕ 搜索"));
        painter.setPen(Qt::white);
        painter.drawRoundedRect(QRect(screen.left() + 96, screen.bottom() - 14, 66, 3), 2, 2);
    }
};

QString capabilityTypeLabel(const QString &type)
{
    if (type == QStringLiteral("bool")) {
        return QStringLiteral("boolean");
    }
    if (type == QStringLiteral("number")) {
        return QStringLiteral("number");
    }
    if (type == QStringLiteral("object")) {
        return QStringLiteral("JSON object");
    }
    return QStringLiteral("text");
}

} // namespace

LayoutPage::LayoutPage(QWidget *parent)
    : QWidget(parent)
    , m_network(new QNetworkAccessManager(this))
{
    setObjectName(QStringLiteral("LayoutPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

#ifdef AI_MOBILE_TEST_STUDIO_HAS_WEBENGINE
    const QString contentRoot = QDir(QCoreApplication::applicationDirPath())
                                    .filePath(QStringLiteral("runtime/appium-inspector"));
    const QString entryPath = QDir(contentRoot).filePath(QStringLiteral("index.html"));
    if (QFileInfo::exists(entryPath)) {
        setProperty("skipWorkspaceTransition", true);
        auto *webView = new QWebEngineView;
        webView->setObjectName(QStringLiteral("AppiumInspectorWebView"));
        webView->setContextMenuPolicy(Qt::DefaultContextMenu);
        auto *page = new AppiumInspectorPage(contentRoot, webView);
        page->setBackgroundColor(QColor(QStringLiteral("#f7faff")));
        webView->setPage(page);
        webView->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
        webView->settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
        webView->settings()->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
        webView->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
        webView->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
        webView->settings()->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);
        connect(page->profile(),
                &QWebEngineProfile::downloadRequested,
                webView,
                [webView](QWebEngineDownloadRequest *download) {
                    const QString path = QFileDialog::getSaveFileName(
                        webView,
                        QStringLiteral("Save Appium Inspector file"),
                        download->downloadFileName());
                    if (path.isEmpty()) {
                        download->cancel();
                        return;
                    }
                    const QFileInfo target(path);
                    download->setDownloadDirectory(target.absolutePath());
                    download->setDownloadFileName(target.fileName());
                    download->accept();
                });
        connect(webView,
                &QWebEngineView::loadFinished,
                webView,
                [page](bool loaded) {
                    if (loaded) {
                        applyInspectorFont(page,
                                           ui::AppPreferences::instance().language());
                    }
                });
        connect(&ui::AppPreferences::instance(),
                &ui::AppPreferences::languageChanged,
                webView,
                [page](ui::AppLanguage language) {
                    applyInspectorFont(page, language);
                });
        webView->load(QUrl::fromLocalFile(entryPath));
        layout->addWidget(webView);
        return;
    }
#endif

    m_modeStack = new QStackedWidget;
    buildSessionBuilder();
    buildInspector();
    m_modeStack->addWidget(m_sessionBuilder);
    m_modeStack->addWidget(m_sessionInspector);
    layout->addWidget(m_modeStack);
}

void LayoutPage::setDeviceConnected(bool connected, const QString &serial)
{
    m_deviceConnected = connected;
    m_deviceSerial = serial;
    if (m_deviceLabel) {
        m_deviceLabel->setText(connected
                                   ? ui::text("Device: %1").arg(serial)
                                   : ui::text("Device: not connected"));
        m_deviceLabel->setStyleSheet(QStringLiteral("color:%1;").arg(connected ? "#3b9f62" : "#8d98a8"));
    }
}

void LayoutPage::buildSessionBuilder()
{
    m_sessionBuilder = new QWidget;
    m_sessionBuilder->setObjectName(QStringLiteral("LayoutSessionBuilder"));
    auto *pageLayout = new QVBoxLayout(m_sessionBuilder);
    pageLayout->setContentsMargins(16, 14, 16, 12);
    pageLayout->setSpacing(10);

    auto *titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->addWidget(label(QStringLiteral("Appium Inspector"), 17, QFont::DemiBold));
    titleRow->addSpacing(12);
    titleRow->addWidget(label(QStringLiteral("Session Builder"), 10, QFont::Normal, QStringLiteral("#7b8798")));
    titleRow->addStretch();
    m_builderStatus = label(QStringLiteral("Ready to create a session"), 9, QFont::Normal,
                            QStringLiteral("#596579"));
    titleRow->addWidget(m_builderStatus);
    pageLayout->addLayout(titleRow);

    m_serverTabs = new QTabWidget;
    m_serverTabs->setObjectName(QStringLiteral("LayoutServerTabs"));
    m_serverTabs->addTab(buildServerTab(), QStringLiteral("Appium Server"));
    m_serverTabs->addTab(buildCloudProvidersTab(), QStringLiteral("Select Cloud Providers"));
    pageLayout->addWidget(m_serverTabs);

    auto *advancedButton = new QToolButton;
    advancedButton->setText(QStringLiteral("›   Advanced Settings"));
    advancedButton->setCheckable(true);
    advancedButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    advancedButton->setObjectName(QStringLiteral("LayoutAdvancedButton"));
    auto *advancedPanel = card(QStringLiteral("LayoutAdvancedPanel"));
    advancedPanel->setVisible(false);
    auto *advancedLayout = new QHBoxLayout(advancedPanel);
    advancedLayout->setContentsMargins(12, 8, 12, 8);
    m_allowUnauthorized = new QCheckBox(QStringLiteral("Allow Unauthorized Certificates"));
    m_useProxy = new QCheckBox(QStringLiteral("Use Proxy"));
    m_proxy = new QLineEdit;
    m_proxy->setPlaceholderText(QStringLiteral("Proxy URL"));
    m_proxy->setEnabled(false);
    connect(m_useProxy, &QCheckBox::toggled, m_proxy, &QLineEdit::setEnabled);
    advancedLayout->addWidget(m_allowUnauthorized);
    advancedLayout->addStretch();
    advancedLayout->addWidget(m_useProxy);
    advancedLayout->addWidget(m_proxy);
    connect(advancedButton, &QToolButton::toggled, this, [advancedButton, advancedPanel](bool checked) {
        advancedPanel->setVisible(checked);
        advancedButton->setText(checked ? QStringLiteral("⌄   Advanced Settings")
                                        : QStringLiteral("›   Advanced Settings"));
    });
    pageLayout->addWidget(advancedButton);
    pageLayout->addWidget(advancedPanel);

    m_builderTabs = new QTabWidget;
    m_builderTabs->setObjectName(QStringLiteral("LayoutBuilderTabs"));
    m_builderTabs->addTab(buildCapabilityBuilderTab(), QStringLiteral("Capability Builder"));
    m_builderTabs->addTab(buildSavedCapabilitiesTab(), QStringLiteral("Saved Capability Sets  0"));
    m_builderTabs->addTab(buildAttachSessionTab(), QStringLiteral("Attach to Session"));
    pageLayout->addWidget(m_builderTabs, 1);

    auto *footer = new QHBoxLayout;
    footer->setContentsMargins(0, 2, 0, 0);
    auto *docsButton = toolButton(QStringLiteral("↗  Capabilities Documentation"));
    docsButton->setFlat(true);
    footer->addWidget(docsButton);
    footer->addStretch();
    auto *saveButton = toolButton(QStringLiteral("Save"));
    auto *saveAsButton = toolButton(QStringLiteral("Save As…"));
    auto *startButton = toolButton(QStringLiteral("Start Session"), QStringLiteral("Create an Appium session"), true);
    connect(saveButton, &QPushButton::clicked, this, [this] {
        if (m_savedCapabilitySets.isEmpty()) {
            saveCapabilitiesAs();
        } else {
            m_savedCapabilitySets[0] = capabilityObject();
            m_builderStatus->setText(QStringLiteral("Capability set saved"));
        }
    });
    connect(saveAsButton, &QPushButton::clicked, this, &LayoutPage::saveCapabilitiesAs);
    connect(startButton, &QPushButton::clicked, this, &LayoutPage::startSession);
    footer->addWidget(saveButton);
    footer->addWidget(saveAsButton);
    footer->addWidget(startButton);
    pageLayout->addLayout(footer);
}

QWidget *LayoutPage::buildServerTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 10, 0, 4);
    layout->setSpacing(8);
    auto *row = new QHBoxLayout;
    row->setSpacing(10);
    row->addWidget(inputGroup(QStringLiteral("Remote Host"), &m_remoteHost), 3);
    row->addWidget(inputGroup(QStringLiteral("Remote Port"), &m_remotePort), 1);
    row->addWidget(inputGroup(QStringLiteral("Remote Path"), &m_remotePath), 3);
    m_ssl = new QCheckBox(QStringLiteral("SSL"));
    row->addWidget(m_ssl, 0, Qt::AlignCenter);
    layout->addLayout(row);
    m_remoteHost->setText(QStringLiteral("127.0.0.1"));
    m_remotePort->setText(QStringLiteral("4723"));
    m_remotePath->setText(QStringLiteral("/"));
    layout->addWidget(label(QStringLiteral("The local Appium server is used by default. You can also connect to a remote server or a cloud provider."),
                            9, QFont::Normal, QStringLiteral("#7b8798")));
    return page;
}

QWidget *LayoutPage::buildCloudProvidersTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 10, 0, 4);
    layout->setSpacing(8);
    auto *row = new QHBoxLayout;
    row->addWidget(label(QStringLiteral("Cloud Provider"), 10, QFont::DemiBold));
    auto *provider = new QComboBox;
    provider->addItems({QStringLiteral("BrowserStack"), QStringLiteral("Sauce Labs"),
                        QStringLiteral("HeadSpin"), QStringLiteral("Kobiton"),
                        QStringLiteral("Perfecto"), QStringLiteral("TestingBot"),
                        QStringLiteral("BitBar"), QStringLiteral("Custom Provider")});
    provider->setMinimumWidth(220);
    row->addWidget(provider);
    row->addStretch();
    layout->addLayout(row);
    auto *form = card(QStringLiteral("LayoutCloudProviderForm"));
    auto *formLayout = new QFormLayout(form);
    formLayout->setContentsMargins(14, 10, 14, 10);
    auto *username = new QLineEdit;
    auto *accessKey = new QLineEdit;
    accessKey->setEchoMode(QLineEdit::Password);
    auto *hub = new QLineEdit;
    hub->setPlaceholderText(QStringLiteral("https://hub-cloud.browserstack.com/wd/hub"));
    formLayout->addRow(QStringLiteral("Username / Email"), username);
    formLayout->addRow(QStringLiteral("Access Key"), accessKey);
    formLayout->addRow(QStringLiteral("Hub URL"), hub);
    layout->addWidget(form);
    layout->addStretch();
    return page;
}

QWidget *LayoutPage::buildCapabilityBuilderTab()
{
    auto *page = new QWidget;
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(10);

    auto *leftPanel = card(QStringLiteral("LayoutCapabilityPanel"));
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(10, 10, 10, 8);
    leftLayout->setSpacing(6);
    auto *headers = new QHBoxLayout;
    headers->setContentsMargins(4, 0, 4, 0);
    headers->addWidget(label(QStringLiteral("Name"), 9, QFont::DemiBold, QStringLiteral("#7b8798")), 3);
    headers->addWidget(label(QStringLiteral("Type"), 9, QFont::DemiBold, QStringLiteral("#7b8798")), 2);
    headers->addWidget(label(QStringLiteral("Value"), 9, QFont::DemiBold, QStringLiteral("#7b8798")), 3);
    headers->addSpacing(78);
    leftLayout->addLayout(headers);
    auto *scroll = new QScrollArea;
    scroll->setObjectName(QStringLiteral("LayoutCapabilityScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *rows = new QWidget;
    auto *rowsLayout = new QVBoxLayout(rows);
    rowsLayout->setContentsMargins(0, 0, 0, 0);
    rowsLayout->setSpacing(5);
    rows->setLayout(rowsLayout);
    scroll->setWidget(rows);
    leftLayout->addWidget(scroll, 1);
    m_autoPrefixes = new QCheckBox(QStringLiteral("Automatically add necessary Appium vendor prefixes on start"));
    m_autoPrefixes->setChecked(true);
    leftLayout->addWidget(m_autoPrefixes);
    auto *addButton = toolButton(QStringLiteral("＋  Add Capability"));
    connect(addButton, &QPushButton::clicked, this, [this, rowsLayout] {
        const int index = m_capabilityRows.size();
        auto row = addCapabilityRow(QString(), QStringLiteral("text"), QString());
        m_capabilityRows.append(row);
        rowsLayout->insertWidget(rowsLayout->count(), row.container);
        connect(row.name, &QLineEdit::textChanged, this, [this] { updateCapabilityJson(); });
        connect(row.type, &QComboBox::currentTextChanged, this, [this, index](const QString &value) {
            if (index < m_capabilityRows.size()) {
                rebuildCapabilityValue(m_capabilityRows[index], QString());
                m_capabilityRows[index].type->setCurrentText(value);
                updateCapabilityJson();
            }
        });
        connect(row.enabled, &QCheckBox::toggled, this, [this] { updateCapabilityJson(); });
        connect(row.remove, &QPushButton::clicked, this, [this, index] {
            if (index < m_capabilityRows.size()) {
                removeCapability(&m_capabilityRows[index]);
            }
        });
        row.name->setFocus();
        updateCapabilityJson();
    });
    leftLayout->addWidget(addButton, 0, Qt::AlignRight);

    auto *jsonPanel = card(QStringLiteral("LayoutJsonPanel"));
    auto *jsonLayout = new QVBoxLayout(jsonPanel);
    jsonLayout->setContentsMargins(14, 14, 14, 12);
    jsonLayout->setSpacing(8);
    auto *jsonHeader = new QHBoxLayout;
    jsonHeader->addWidget(label(QStringLiteral("JSON Representation"), 13, QFont::DemiBold));
    jsonHeader->addStretch();
    auto *copyButton = toolButton(QStringLiteral("▣"), QStringLiteral("Copy JSON"));
    auto *editButton = toolButton(QStringLiteral("✎"), QStringLiteral("Edit JSON"));
    copyButton->setFixedWidth(40);
    editButton->setFixedWidth(40);
    connect(copyButton, &QPushButton::clicked, this, [this] {
        QApplication::clipboard()->setText(m_capabilityJson->toPlainText());
        m_builderStatus->setText(QStringLiteral("JSON copied to clipboard"));
    });
    connect(editButton, &QPushButton::clicked, this, [this, editButton] {
        m_capabilityJson->setReadOnly(!m_capabilityJson->isReadOnly());
        editButton->setText(m_capabilityJson->isReadOnly() ? QStringLiteral("✎") : QStringLiteral("✓"));
    });
    jsonHeader->addWidget(copyButton);
    jsonHeader->addWidget(editButton);
    jsonLayout->addLayout(jsonHeader);
    m_capabilityJson = new QPlainTextEdit;
    m_capabilityJson->setObjectName(QStringLiteral("LayoutJsonEditor"));
    m_capabilityJson->setReadOnly(true);
    m_capabilityJson->setFont(ui::appFont(10));
    jsonLayout->addWidget(m_capabilityJson, 1);

    layout->addWidget(leftPanel, 1);
    layout->addWidget(jsonPanel, 1);

    const QStringList names = {QStringLiteral("platformName"), QStringLiteral("appium:automationName"),
                               QStringLiteral("appium:platformVersion"), QStringLiteral("appium:deviceName"),
                               QStringLiteral("appium:udid"), QStringLiteral("appium:appPackage"),
                               QStringLiteral("appium:appActivity"), QStringLiteral("appium:noReset")};
    const QStringList types = {QStringLiteral("text"), QStringLiteral("text"), QStringLiteral("text"),
                               QStringLiteral("text"), QStringLiteral("text"), QStringLiteral("text"),
                               QStringLiteral("text"), QStringLiteral("bool")};
    const QVariantList values = {QStringLiteral("Android"), QStringLiteral("UiAutomator2"), QStringLiteral("15"),
                                 QStringLiteral("459c8df8"), QStringLiteral("459c8df8"),
                                 QStringLiteral("com.microsoft.emmx"), QStringLiteral("com.microsoft.ruby.Main"), true};
    for (int index = 0; index < names.size(); ++index) {
        auto row = addCapabilityRow(names[index], types[index], values[index]);
        m_capabilityRows.append(row);
        rowsLayout->addWidget(row.container);
        const int rowIndex = index;
        connect(row.name, &QLineEdit::textChanged, this, [this] { updateCapabilityJson(); });
        connect(row.type, &QComboBox::currentTextChanged, this, [this, rowIndex](const QString &value) {
            if (rowIndex < m_capabilityRows.size()) {
                rebuildCapabilityValue(m_capabilityRows[rowIndex], QString());
                m_capabilityRows[rowIndex].type->setCurrentText(value);
                updateCapabilityJson();
            }
        });
        connect(row.enabled, &QCheckBox::toggled, this, [this] { updateCapabilityJson(); });
        connect(row.remove, &QPushButton::clicked, this, [this, rowIndex] {
            if (rowIndex < m_capabilityRows.size()) {
                removeCapability(&m_capabilityRows[rowIndex]);
            }
        });
    }
    updateCapabilityJson();
    return page;
}

LayoutPage::CapabilityRow LayoutPage::addCapabilityRow(const QString &name,
                                                        const QString &type,
                                                        const QVariant &value,
                                                        bool enabled)
{
    CapabilityRow row;
    row.container = new QWidget;
    row.container->setObjectName(QStringLiteral("LayoutCapabilityRow"));
    auto *layout = new QHBoxLayout(row.container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(7);
    row.name = new QLineEdit(name);
    row.name->setPlaceholderText(QStringLiteral("Name"));
    row.name->setFont(ui::appFont(10));
    row.name->setMinimumHeight(38);
    row.type = new QComboBox;
    row.type->addItem(QStringLiteral("text"), QStringLiteral("text"));
    row.type->addItem(QStringLiteral("boolean"), QStringLiteral("bool"));
    row.type->addItem(QStringLiteral("number"), QStringLiteral("number"));
    row.type->addItem(QStringLiteral("JSON object"), QStringLiteral("object"));
    row.type->setMinimumHeight(38);
    row.type->setCurrentText(capabilityTypeLabel(type));
    row.enabled = new QCheckBox;
    row.enabled->setChecked(enabled);
    row.enabled->setToolTip(QStringLiteral("Enable capability"));
    row.remove = toolButton(QStringLiteral("▢"), QStringLiteral("Delete capability"));
    row.remove->setFixedWidth(38);
    layout->addWidget(row.name, 3);
    layout->addWidget(row.type, 2);
    row.value = nullptr;
    rebuildCapabilityValue(row, value);
    layout->addWidget(row.enabled, 0, Qt::AlignCenter);
    layout->addWidget(row.remove);
    return row;
}

void LayoutPage::rebuildCapabilityValue(CapabilityRow &row, const QVariant &value)
{
    auto *layout = qobject_cast<QHBoxLayout *>(row.container->layout());
    if (!layout) {
        return;
    }
    if (row.value) {
        layout->removeWidget(row.value);
        row.value->deleteLater();
        row.value = nullptr;
    }
    const QString type = row.type ? row.type->currentData().toString() : QStringLiteral("text");
    if (type == QStringLiteral("bool")) {
        auto *check = new QCheckBox;
        check->setChecked(value.isValid() ? value.toBool() : false);
        check->setText(check->isChecked() ? QStringLiteral("true") : QStringLiteral("false"));
        check->setMinimumHeight(38);
        connect(check, &QCheckBox::toggled, this, [this, check](bool checked) {
            check->setText(checked ? QStringLiteral("true") : QStringLiteral("false"));
            updateCapabilityJson();
        });
        row.value = check;
    } else {
        auto *edit = new QLineEdit;
        edit->setMinimumHeight(38);
        edit->setFont(ui::appFont(10));
        edit->setPlaceholderText(QStringLiteral("Value"));
        edit->setText(value.isValid() ? value.toString() : QString());
        connect(edit, &QLineEdit::textChanged, this, [this] { updateCapabilityJson(); });
        row.value = edit;
    }
    layout->insertWidget(3, row.value, 3);
}

void LayoutPage::removeCapability(CapabilityRow *row)
{
    if (!row || row->name->text().isEmpty()) {
        return;
    }
    int visibleCount = 0;
    for (const CapabilityRow &candidate : std::as_const(m_capabilityRows)) {
        if (candidate.container->isVisible() && !candidate.name->text().isEmpty()) {
            ++visibleCount;
        }
    }
    if (visibleCount <= 1) {
        m_builderStatus->setText(QStringLiteral("At least one capability row is required"));
        return;
    }
    row->name->clear();
    row->enabled->setChecked(false);
    row->container->setVisible(false);
    updateCapabilityJson();
}

QJsonObject LayoutPage::capabilityObject() const
{
    QJsonObject object;
    for (const CapabilityRow &row : m_capabilityRows) {
        if (!row.container->isVisible() || !row.enabled->isChecked()) {
            continue;
        }
        const QString name = row.name->text().trimmed();
        if (name.isEmpty()) {
            continue;
        }
        const QString type = row.type->currentData().toString();
        if (type == QStringLiteral("bool")) {
            object.insert(name, qobject_cast<QCheckBox *>(row.value)->isChecked());
        } else if (type == QStringLiteral("number")) {
            bool ok = false;
            const double number = qobject_cast<QLineEdit *>(row.value)->text().toDouble(&ok);
            object.insert(name, ok ? QJsonValue(number) : QJsonValue());
        } else if (type == QStringLiteral("object")) {
            QJsonParseError error;
            const QJsonDocument parsed = QJsonDocument::fromJson(
                qobject_cast<QLineEdit *>(row.value)->text().toUtf8(), &error);
            object.insert(name, error.error == QJsonParseError::NoError && parsed.isObject()
                                     ? QJsonValue(parsed.object())
                                     : QJsonValue(qobject_cast<QLineEdit *>(row.value)->text()));
        } else {
            object.insert(name, qobject_cast<QLineEdit *>(row.value)->text());
        }
    }
    return object;
}

void LayoutPage::updateCapabilityJson()
{
    if (!m_capabilityJson) {
        return;
    }
    const QJsonDocument document(capabilityObject());
    m_capabilityJson->setPlainText(QString::fromUtf8(document.toJson(QJsonDocument::Indented)));
}

QWidget *LayoutPage::buildSavedCapabilitiesTab()
{
    auto *page = new QWidget;
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(10);
    auto *left = card(QStringLiteral("LayoutSavedPanel"));
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(10, 10, 10, 10);
    auto *hint = label(QStringLiteral("Saved capability sets can be reused, exported, or imported from an Appium Inspector session file."),
                       9, QFont::Normal, QStringLiteral("#7b8798"));
    hint->setWordWrap(true);
    leftLayout->addWidget(hint);
    m_savedTable = new QTableWidget(0, 3);
    m_savedTable->setObjectName(QStringLiteral("LayoutSavedTable"));
    m_savedTable->setHorizontalHeaderLabels({QStringLiteral("Name"), QStringLiteral("Created"), QStringLiteral("Actions")});
    m_savedTable->horizontalHeader()->setStretchLastSection(true);
    m_savedTable->verticalHeader()->setVisible(false);
    m_savedTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_savedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    leftLayout->addWidget(m_savedTable, 1);
    auto *actions = new QHBoxLayout;
    auto *importButton = toolButton(QStringLiteral("Import from File"));
    auto *exportButton = toolButton(QStringLiteral("Export Selected"));
    connect(importButton, &QPushButton::clicked, this, &LayoutPage::importCapabilities);
    connect(exportButton, &QPushButton::clicked, this, [this] {
        const int row = m_savedTable->currentRow();
        if (row < 0 || row >= m_savedCapabilitySets.size()) {
            return;
        }
        const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Export capability set"),
                                                          QString(), QStringLiteral("JSON (*.json)"));
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(QJsonDocument(m_savedCapabilitySets[row]).toJson(QJsonDocument::Indented));
        }
    });
    actions->addWidget(importButton);
    actions->addWidget(exportButton);
    actions->addStretch();
    leftLayout->addLayout(actions);
    auto *preview = card(QStringLiteral("LayoutJsonPanel"));
    auto *previewLayout = new QVBoxLayout(preview);
    previewLayout->setContentsMargins(14, 14, 14, 14);
    previewLayout->addWidget(label(QStringLiteral("JSON Representation"), 13, QFont::DemiBold));
    auto *previewJson = new QPlainTextEdit;
    previewJson->setReadOnly(true);
    previewJson->setFont(ui::appFont(10));
    previewLayout->addWidget(previewJson, 1);
    connect(m_savedTable, &QTableWidget::currentCellChanged, this,
            [this, previewJson](int currentRow, int, int, int) {
                if (currentRow >= 0 && currentRow < m_savedCapabilitySets.size()) {
                    previewJson->setPlainText(QString::fromUtf8(
                        QJsonDocument(m_savedCapabilitySets[currentRow]).toJson(QJsonDocument::Indented)));
                }
            });
    layout->addWidget(left, 1);
    layout->addWidget(preview, 1);
    return page;
}

QWidget *LayoutPage::buildAttachSessionTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 14, 0, 0);
    layout->setSpacing(12);
    auto *instructions = card(QStringLiteral("LayoutInstruction"));
    auto *instructionLayout = new QVBoxLayout(instructions);
    instructionLayout->setContentsMargins(16, 14, 16, 14);
    instructionLayout->addWidget(label(QStringLiteral("Attach to an existing Appium session"), 13, QFont::DemiBold));
    instructionLayout->addWidget(label(QStringLiteral("Enter a session id or discover running sessions on the configured server."),
                                       9, QFont::Normal, QStringLiteral("#7b8798")));
    layout->addWidget(instructions);
    auto *manual = new QHBoxLayout;
    m_attachSessionId = new QLineEdit;
    m_attachSessionId->setPlaceholderText(QStringLiteral("Session ID"));
    auto *refresh = toolButton(QStringLiteral("↻  Refresh"));
    auto *attach = toolButton(QStringLiteral("Attach"), QStringLiteral("Attach to this session"), true);
    manual->addWidget(m_attachSessionId, 1);
    manual->addWidget(refresh);
    manual->addWidget(attach);
    layout->addLayout(manual);
    auto *sessions = new QListWidget;
    sessions->setObjectName(QStringLiteral("LayoutSessionList"));
    sessions->addItem(QStringLiteral("No running sessions found. Click Refresh to discover sessions."));
    sessions->addItem(QStringLiteral("Tip: Appium Inspector can attach to sessions created by another client."));
    layout->addWidget(sessions, 1);
    connect(refresh, &QPushButton::clicked, this, [this, sessions] {
        sessions->clear();
        sessions->addItem(QStringLiteral("Searching %1:%2 …").arg(m_remoteHost->text(), m_remotePort->text()));
        m_builderStatus->setText(QStringLiteral("Session discovery requested"));
    });
    connect(attach, &QPushButton::clicked, this, [this] {
        m_sessionId = m_attachSessionId->text().trimmed();
        if (m_sessionId.isEmpty()) {
            m_sessionId = QStringLiteral("attached-session");
        }
        m_modeStack->setCurrentWidget(m_sessionInspector);
        if (m_sessionIdLabel) {
            m_sessionIdLabel->setText(QStringLiteral("Session %1").arg(m_sessionId));
        }
        refreshInspectorSource();
    });
    return page;
}

void LayoutPage::saveCapabilitiesAs()
{
    const QString name = QStringLiteral("Capability set %1").arg(m_savedCapabilitySets.size() + 1);
    const QJsonObject object = capabilityObject();
    m_savedCapabilitySets.append(object);
    const int row = m_savedTable ? m_savedTable->rowCount() : 0;
    if (m_savedTable) {
        m_savedTable->insertRow(row);
        m_savedTable->setItem(row, 0, new QTableWidgetItem(name));
        m_savedTable->setItem(row, 1, new QTableWidgetItem(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd"))));
        auto *deleteButton = toolButton(QStringLiteral("Delete"));
        connect(deleteButton, &QPushButton::clicked, this, [this, deleteButton] {
            int row = -1;
            for (int index = 0; index < m_savedTable->rowCount(); ++index) {
                if (m_savedTable->cellWidget(index, 2) == deleteButton) {
                    row = index;
                    break;
                }
            }
            if (row >= 0 && row < m_savedCapabilitySets.size()) {
                m_savedCapabilitySets.removeAt(row);
                m_savedTable->removeRow(row);
                m_builderTabs->setTabText(1, QStringLiteral("Saved Capability Sets  %1").arg(m_savedCapabilitySets.size()));
            }
        });
        m_savedTable->setCellWidget(row, 2, deleteButton);
        m_builderTabs->setTabText(1, QStringLiteral("Saved Capability Sets  %1").arg(m_savedCapabilitySets.size()));
    }
    m_builderStatus->setText(QStringLiteral("Saved %1").arg(name));
}

void LayoutPage::importCapabilities()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Import Appium Inspector session"),
                                                      QString(), QStringLiteral("JSON (*.json);;All files (*.*)"));
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        QMessageBox::warning(this, QStringLiteral("Import failed"), error.errorString());
        return;
    }
    m_savedCapabilitySets.append(document.object());
    if (m_savedTable) {
        const int row = m_savedTable->rowCount();
        m_savedTable->insertRow(row);
        m_savedTable->setItem(row, 0, new QTableWidgetItem(QFileInfo(path).baseName()));
        m_savedTable->setItem(row, 1, new QTableWidgetItem(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd"))));
        m_savedTable->setItem(row, 2, new QTableWidgetItem(QStringLiteral("Imported")));
        m_builderTabs->setTabText(1, QStringLiteral("Saved Capability Sets  %1").arg(m_savedCapabilitySets.size()));
    }
}

void LayoutPage::startSession()
{
    m_sessionId = QStringLiteral("preview-session");
    if (m_sessionIdLabel) {
        m_sessionIdLabel->setText(QStringLiteral("Session preview-session"));
    }
    m_modeStack->setCurrentWidget(m_sessionInspector);
    setInspectorStatus(QStringLiteral("Preview session ready; connecting to Appium server…"), QStringLiteral("#2f6df6"));
    postSessionToAppium();
}

void LayoutPage::returnToBuilder()
{
    m_modeStack->setCurrentWidget(m_sessionBuilder);
    m_builderStatus->setText(QStringLiteral("Session is still available; edit capabilities and restart when ready"));
}

void LayoutPage::setInspectorStatus(const QString &message, const QString &color)
{
    if (m_inspectorStatus) {
        m_inspectorStatus->setText(message);
        m_inspectorStatus->setStyleSheet(QStringLiteral("color:%1;").arg(color));
    }
}

void LayoutPage::postSessionToAppium()
{
    QString path = m_remotePath ? m_remotePath->text().trimmed() : QStringLiteral("/");
    if (path.isEmpty()) {
        path = QStringLiteral("/");
    }
    if (!path.startsWith(QLatin1Char('/'))) {
        path.prepend(QLatin1Char('/'));
    }
    while (path.endsWith(QLatin1Char('/')) && path.size() > 1) {
        path.chop(1);
    }
    QUrl url;
    url.setScheme(m_ssl && m_ssl->isChecked() ? QStringLiteral("https") : QStringLiteral("http"));
    url.setHost(m_remoteHost ? m_remoteHost->text().trimmed() : QStringLiteral("127.0.0.1"));
    url.setPort(m_remotePort ? m_remotePort->text().toInt() : 4723);
    url.setPath(path == QStringLiteral("/") ? QStringLiteral("/session")
                                             : path + QStringLiteral("/session"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QJsonObject caps;
    caps.insert(QStringLiteral("alwaysMatch"), capabilityObject());
    caps.insert(QStringLiteral("firstMatch"), QJsonArray());
    QJsonObject body;
    body.insert(QStringLiteral("capabilities"), caps);
    auto *reply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            setInspectorStatus(QStringLiteral("Appium server unavailable; showing preview data"), QStringLiteral("#b27b2a"));
            reply->deleteLater();
            return;
        }
        QJsonParseError error;
        const QJsonDocument response = QJsonDocument::fromJson(data, &error);
        if (error.error == QJsonParseError::NoError && response.isObject()) {
            const QJsonObject value = response.object().value(QStringLiteral("value")).toObject();
            const QString sessionId = response.object().value(QStringLiteral("sessionId")).toString(
                value.value(QStringLiteral("sessionId")).toString());
            if (!sessionId.isEmpty()) {
                m_sessionId = sessionId;
                m_sessionIdLabel->setText(QStringLiteral("Session %1").arg(sessionId));
                setInspectorStatus(QStringLiteral("Connected to Appium server"), QStringLiteral("#3b9f62"));
                refreshInspectorSource();
            }
        }
        reply->deleteLater();
    });
}

void LayoutPage::refreshInspectorSource()
{
    if (m_sourceTree) {
        m_sourceTree->clear();
        auto *root = new QTreeWidgetItem(m_sourceTree, {QStringLiteral("<android.widget.FrameLayout>")});
        auto *linear = new QTreeWidgetItem(root, {QStringLiteral("<android.widget.LinearLayout>")});
        new QTreeWidgetItem(linear, {QStringLiteral("<android.widget.TextView text=\"Appium Inspector\">")});
        new QTreeWidgetItem(linear, {QStringLiteral("<android.widget.Button resource-id=\"btnStart\">")});
        root->setExpanded(true);
        linear->setExpanded(true);
    }
    setInspectorStatus(QStringLiteral("Source refreshed at %1").arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"))),
                       QStringLiteral("#596579"));
}

void LayoutPage::buildInspector()
{
    m_sessionInspector = new QWidget;
    m_sessionInspector->setObjectName(QStringLiteral("LayoutSessionInspector"));
    auto *pageLayout = new QVBoxLayout(m_sessionInspector);
    pageLayout->setContentsMargins(12, 10, 12, 10);
    pageLayout->setSpacing(8);

    auto *header = card(QStringLiteral("LayoutInspectorHeader"));
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);
    auto addHeaderButton = [headerLayout](const QString &text, bool selected = false) {
        auto *button = toolButton(text);
        button->setFixedSize(42, 34);
        if (selected) {
            button->setObjectName(QStringLiteral("LayoutInspectorSelectedButton"));
        }
        headerLayout->addWidget(button);
        return button;
    };
    addHeaderButton(QStringLiteral("‹"));
    addHeaderButton(QStringLiteral("○"));
    addHeaderButton(QStringLiteral("□"));
    headerLayout->addSpacing(10);
    addHeaderButton(QStringLiteral("⌘"), true);
    addHeaderButton(QStringLiteral("◎"));
    headerLayout->addSpacing(10);
    addHeaderButton(QStringLiteral("↻"));
    auto *refreshButton = addHeaderButton(QStringLiteral("⌕"));
    addHeaderButton(QStringLiteral("▣"));
    headerLayout->addSpacing(10);
    addHeaderButton(QStringLiteral("◌"));
    addHeaderButton(QStringLiteral("⌁"));
    auto *quitButton = addHeaderButton(QStringLiteral("×"));
    headerLayout->addStretch();
    m_deviceLabel = label(QStringLiteral("Device: not connected"), 9, QFont::Normal, QStringLiteral("#8d98a8"));
    headerLayout->addWidget(m_deviceLabel);
    m_sessionIdLabel = label(QStringLiteral("Session preview-session"), 9, QFont::Normal, QStringLiteral("#596579"));
    headerLayout->addWidget(m_sessionIdLabel);
    auto *builderButton = toolButton(QStringLiteral("Session Builder"));
    headerLayout->addWidget(builderButton);
    connect(builderButton, &QPushButton::clicked, this, &LayoutPage::returnToBuilder);
    connect(quitButton, &QPushButton::clicked, this, &LayoutPage::returnToBuilder);
    connect(refreshButton, &QPushButton::clicked, this, &LayoutPage::refreshInspectorSource);
    pageLayout->addWidget(header);

    auto *main = new QSplitter(Qt::Horizontal);
    main->setObjectName(QStringLiteral("LayoutInspectorSplitter"));
    auto *left = card(QStringLiteral("LayoutScreenshotCard"));
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(8, 8, 8, 8);
    auto *screenshotControls = new QHBoxLayout;
    auto *handleButton = toolButton(QStringLiteral("◉"), QStringLiteral("Show Element Handles"));
    auto *selectButton = toolButton(QStringLiteral("⌗"), QStringLiteral("Select Elements"), true);
    auto *coordinateButton = toolButton(QStringLiteral("⌖"), QStringLiteral("Tap/Swipe By Coordinates"));
    auto *downloadButton = toolButton(QStringLiteral("⇩"), QStringLiteral("Download Screenshot"));
    for (QPushButton *button : {handleButton, selectButton, coordinateButton, downloadButton}) {
        button->setFixedWidth(40);
        screenshotControls->addWidget(button);
    }
    screenshotControls->addStretch();
    leftLayout->addLayout(screenshotControls);
    auto *phone = new PhonePreview;
    leftLayout->addWidget(phone, 1);
    main->addWidget(left);

    auto *right = new QWidget;
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(6);
    m_inspectorTabs = new QTabWidget;
    m_inspectorTabs->setObjectName(QStringLiteral("LayoutInspectorTabs"));
    m_inspectorTabs->addTab(buildSourceTab(), QStringLiteral("Source"));
    m_inspectorTabs->addTab(buildCommandsTab(), QStringLiteral("Commands"));
    m_inspectorTabs->addTab(buildGesturesTab(), QStringLiteral("Gestures"));
    m_inspectorTabs->addTab(buildRecorderTab(), QStringLiteral("Recorder"));
    m_inspectorTabs->addTab(buildSessionInfoTab(), QStringLiteral("Session Information"));
    rightLayout->addWidget(m_inspectorTabs, 1);
    m_inspectorStatus = label(QStringLiteral("Source is ready"), 9, QFont::Normal, QStringLiteral("#596579"));
    rightLayout->addWidget(m_inspectorStatus);
    main->addWidget(right);
    main->setStretchFactor(0, 0);
    main->setStretchFactor(1, 1);
    pageLayout->addWidget(main, 1);
}

QWidget *LayoutPage::buildSourceTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(8);
    auto *sourceCard = card(QStringLiteral("LayoutSourceCard"));
    auto *sourceLayout = new QVBoxLayout(sourceCard);
    sourceLayout->setContentsMargins(14, 12, 14, 12);
    auto *header = new QHBoxLayout;
    header->addWidget(label(QStringLiteral("▤  App Source"), 13, QFont::DemiBold));
    header->addStretch();
    auto *copy = toolButton(QStringLiteral("▣"), QStringLiteral("Copy source"));
    auto *download = toolButton(QStringLiteral("⇩"), QStringLiteral("Download source"));
    copy->setFixedWidth(36);
    download->setFixedWidth(36);
    header->addWidget(copy);
    header->addWidget(download);
    sourceLayout->addLayout(header);
    auto *searchRow = new QHBoxLayout;
    auto *expand = toolButton(QStringLiteral("✣"), QStringLiteral("Expand all"));
    auto *showHandles = toolButton(QStringLiteral("◉"), QStringLiteral("Show handles"));
    expand->setFixedWidth(38);
    showHandles->setFixedWidth(38);
    searchRow->addWidget(expand);
    searchRow->addWidget(showHandles);
    m_sourceSearch = new QLineEdit;
    m_sourceSearch->setPlaceholderText(QStringLiteral("⌕  Search Source"));
    searchRow->addWidget(m_sourceSearch, 1);
    searchRow->addWidget(label(QStringLiteral("0"), 10, QFont::DemiBold, QStringLiteral("#596579")));
    sourceLayout->addLayout(searchRow);
    auto *split = new QSplitter(Qt::Horizontal);
    m_sourceTree = new QTreeWidget;
    m_sourceTree->setObjectName(QStringLiteral("LayoutSourceTree"));
    m_sourceTree->setHeaderHidden(true);
    m_sourceTree->setIndentation(22);
    split->addWidget(m_sourceTree);
    auto *selectedCard = card(QStringLiteral("LayoutSelectedElement"));
    auto *selectedLayout = new QVBoxLayout(selectedCard);
    selectedLayout->setContentsMargins(14, 12, 14, 12);
    selectedLayout->addWidget(label(QStringLiteral("Selected Element"), 12, QFont::DemiBold));
    auto *selected = new QPlainTextEdit;
    selected->setReadOnly(true);
    selected->setPlainText(QStringLiteral("Select an element in the source tree to inspect its attributes, locators, and box model."));
    selectedLayout->addWidget(selected, 1);
    split->addWidget(selectedCard);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 1);
    sourceLayout->addWidget(split, 1);
    layout->addWidget(sourceCard, 1);
    refreshInspectorSource();
    connect(m_sourceTree, &QTreeWidget::itemClicked, this, [selected](QTreeWidgetItem *item, int) {
        selected->setPlainText(QStringLiteral("Element: %1\n\nAttributes\n  className: %2\n  enabled: true\n  visible: true\n  bounds: [0,0][1080,2400]\n\nLocators\n  xpath: //%2\n  accessibility id: %2")
                                   .arg(item->text(0), item->text(0).section(QLatin1Char('.'), -1)));
    });
    return page;
}

QWidget *LayoutPage::buildCommandsTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(8);
    auto *cardPanel = card(QStringLiteral("LayoutCommandsCard"));
    auto *cardLayout = new QVBoxLayout(cardPanel);
    cardLayout->setContentsMargins(14, 12, 14, 12);
    cardLayout->addWidget(label(QStringLiteral("WebDriver Commands"), 13, QFont::DemiBold));
    const QStringList commands = {QStringLiteral("getPageSource"), QStringLiteral("getScreenshot"),
                                  QStringLiteral("getWindowSize"), QStringLiteral("getContexts"),
                                  QStringLiteral("getOrientation"), QStringLiteral("getSession"),
                                  QStringLiteral("findElement"), QStringLiteral("executeScript")};
    for (const QString &command : commands) {
        auto *button = toolButton(command + QStringLiteral("   ▶"));
        button->setObjectName(QStringLiteral("LayoutCommandButton"));
        connect(button, &QPushButton::clicked, this, [this, command] {
            setInspectorStatus(QStringLiteral("Command %1 executed (preview)").arg(command), QStringLiteral("#2f6df6"));
        });
        cardLayout->addWidget(button);
    }
    auto *result = new QPlainTextEdit;
    result->setReadOnly(true);
    result->setPlainText(QStringLiteral("Command result\n\nSelect a command to send it to the current Appium session."));
    cardLayout->addWidget(result, 1);
    layout->addWidget(cardPanel, 1);
    return page;
}

QWidget *LayoutPage::buildGesturesTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(8);
    auto *toolbar = new QHBoxLayout;
    toolbar->addWidget(label(QStringLiteral("Gesture Editor"), 13, QFont::DemiBold));
    toolbar->addStretch();
    auto *newGesture = toolButton(QStringLiteral("＋  New Gesture"));
    auto *runGesture = toolButton(QStringLiteral("▶  Run Gesture"), QString(), true);
    toolbar->addWidget(newGesture);
    toolbar->addWidget(runGesture);
    layout->addLayout(toolbar);
    auto *panel = card(QStringLiteral("LayoutGestureCard"));
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(12, 12, 12, 12);
    m_gestureTable = new QTableWidget(0, 4);
    m_gestureTable->setHorizontalHeaderLabels({QStringLiteral("Pointer"), QStringLiteral("Action"),
                                               QStringLiteral("X"), QStringLiteral("Y")});
    m_gestureTable->horizontalHeader()->setStretchLastSection(true);
    m_gestureTable->verticalHeader()->setVisible(false);
    panelLayout->addWidget(m_gestureTable, 1);
    connect(newGesture, &QPushButton::clicked, this, [this] {
        const int row = m_gestureTable->rowCount();
        m_gestureTable->insertRow(row);
        m_gestureTable->setItem(row, 0, new QTableWidgetItem(QStringLiteral("finger1")));
        m_gestureTable->setItem(row, 1, new QTableWidgetItem(QStringLiteral("pointerMove")));
        m_gestureTable->setItem(row, 2, new QTableWidgetItem(QStringLiteral("540")));
        m_gestureTable->setItem(row, 3, new QTableWidgetItem(QStringLiteral("1200")));
    });
    connect(runGesture, &QPushButton::clicked, this, [this] {
        setInspectorStatus(QStringLiteral("Gesture dispatched to the current session (preview)"), QStringLiteral("#2f6df6"));
    });
    layout->addWidget(panel, 1);
    return page;
}

QWidget *LayoutPage::buildRecorderTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(8);
    auto *toolbar = new QHBoxLayout;
    toolbar->addWidget(label(QStringLiteral("Interaction Recorder"), 13, QFont::DemiBold));
    toolbar->addStretch();
    auto *record = toolButton(QStringLiteral("●  Start Recording"), QString(), true);
    toolbar->addWidget(record);
    layout->addLayout(toolbar);
    auto *panel = card(QStringLiteral("LayoutRecorderCard"));
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(12, 12, 12, 12);
    m_recorderTable = new QTableWidget(0, 3);
    m_recorderTable->setHorizontalHeaderLabels({QStringLiteral("Time"), QStringLiteral("Action"), QStringLiteral("Generated Code")});
    m_recorderTable->horizontalHeader()->setStretchLastSection(true);
    m_recorderTable->verticalHeader()->setVisible(false);
    panelLayout->addWidget(m_recorderTable, 1);
    connect(record, &QPushButton::clicked, this, [this, record] {
        m_recording = !m_recording;
        record->setText(m_recording ? QStringLiteral("■  Stop Recording") : QStringLiteral("●  Start Recording"));
        record->setObjectName(m_recording ? QStringLiteral("LayoutDangerButton") : QStringLiteral("LayoutPrimaryButton"));
        record->style()->unpolish(record);
        record->style()->polish(record);
        if (m_recording) {
            const int row = m_recorderTable->rowCount();
            m_recorderTable->insertRow(row);
            m_recorderTable->setItem(row, 0, new QTableWidgetItem(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"))));
            m_recorderTable->setItem(row, 1, new QTableWidgetItem(QStringLiteral("Recording started")));
            m_recorderTable->setItem(row, 2, new QTableWidgetItem(QStringLiteral("driver = new RemoteWebDriver(...);")));
        }
    });
    layout->addWidget(panel, 1);
    return page;
}

QWidget *LayoutPage::buildSessionInfoTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(8);
    auto *panel = card(QStringLiteral("LayoutSessionInfoCard"));
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(14, 12, 14, 12);
    panelLayout->addWidget(label(QStringLiteral("Session Information"), 13, QFont::DemiBold));
    auto *info = new QPlainTextEdit;
    info->setReadOnly(true);
    info->setFont(ui::appFont(10));
    info->setPlainText(QStringLiteral("{\n  \"platformName\": \"Android\",\n  \"appium:automationName\": \"UiAutomator2\",\n  \"appium:deviceName\": \"459c8df8\",\n  \"driver\": \"UiAutomator2\",\n  \"context\": \"NATIVE_APP\"\n}"));
    panelLayout->addWidget(info, 1);
    layout->addWidget(panel, 1);
    return page;
}
