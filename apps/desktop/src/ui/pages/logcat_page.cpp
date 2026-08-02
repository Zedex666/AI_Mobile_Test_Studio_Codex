#include "ui/pages/logcat_page.h"

#include "ui/common/widget_helpers.h"

#include <QAbstractTableModel>
#include <QBoxLayout>
#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QSaveFile>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QStringConverter>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTextStream>
#include <QTimer>
#include <QToolButton>

#include <algorithm>
#include <utility>

namespace {

constexpr int kMaxEntries = 10000;

int priorityValue(QChar priority)
{
    switch (priority.toLatin1()) {
    case 'V':
        return 2;
    case 'D':
        return 3;
    case 'I':
        return 4;
    case 'W':
        return 5;
    case 'E':
    case 'F':
    case 'A':
        return 6;
    default:
        return 2;
    }
}

QColor priorityColor(QChar priority)
{
    switch (priority.toLatin1()) {
    case 'V':
        return QColor(QStringLiteral("#9aa3af"));
    case 'D':
        return QColor(QStringLiteral("#55b938"));
    case 'I':
        return QColor(QStringLiteral("#2584f6"));
    case 'W':
        return QColor(QStringLiteral("#e7a116"));
    case 'E':
    case 'F':
    case 'A':
        return QColor(QStringLiteral("#ef4d4d"));
    default:
        return QColor(QStringLiteral("#9aa3af"));
    }
}

QToolButton *makeToolButton(const QString &icon, const QString &tooltip)
{
    auto *button = new QToolButton;
    button->setObjectName("LogcatToolButton");
    button->setText(icon);
    button->setToolTip(tooltip);
    button->setFixedSize(34, 34);
    button->setCursor(Qt::PointingHandCursor);
    button->setFont(ui::appFont(13, QFont::DemiBold));
    return button;
}

class LogcatPriorityDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();
        painter->fillRect(option.rect, index.data(Qt::BackgroundRole).value<QColor>());
        painter->setPen(index.data(Qt::ForegroundRole).value<QColor>());
        painter->setFont(option.font);
        painter->drawText(option.rect, Qt::AlignCenter, index.data().toString());
        painter->restore();
    }
};

} // namespace

class LogcatModel : public QAbstractTableModel
{
public:
    enum Column {
        Time = 0,
        Thread,
        Tag,
        Process,
        Priority,
        Message,
        ColumnCount
    };

    explicit LogcatModel(QObject *parent = nullptr)
        : QAbstractTableModel(parent)
    {
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : m_entries.size();
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : ColumnCount;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
            return QVariant();
        }
        const LogcatEntry &entry = m_entries[index.row()];
        if (role == Qt::DisplayRole) {
            switch (index.column()) {
            case Time:
                return entry.time;
            case Thread:
                return QStringLiteral("%1-%2").arg(entry.pid).arg(entry.tid);
            case Tag:
                return entry.tag;
            case Process:
                return entry.processName;
            case Priority:
                return QString(entry.priority);
            case Message:
                return entry.message;
            default:
                return QVariant();
            }
        }
        if (role == Qt::TextAlignmentRole && index.column() == Priority) {
            return Qt::AlignCenter;
        }
        if (role == Qt::BackgroundRole && index.column() == Priority) {
            return priorityColor(entry.priority);
        }
        if (role == Qt::ForegroundRole) {
            if (index.column() == Priority) {
                return QColor(QStringLiteral("#ffffff"));
            }
            if (entry.priority == QLatin1Char('E') || entry.priority == QLatin1Char('F')
                || entry.priority == QLatin1Char('A')) {
                return QColor(QStringLiteral("#df3f47"));
            }
            if (entry.priority == QLatin1Char('W')) {
                return QColor(QStringLiteral("#9a6200"));
            }
        }
        if (role == Qt::ToolTipRole) {
            return QStringLiteral("%1 %2-%3 %4/%5 %6: %7")
                .arg(entry.time)
                .arg(entry.pid)
                .arg(entry.tid)
                .arg(entry.processName,
                     QString(entry.priority),
                     entry.tag,
                     entry.message);
        }
        return QVariant();
    }

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
            return QVariant();
        }
        static const QStringList headers = {ui::text("时间"),
                                            QStringLiteral("PID-TID"),
                                            ui::text("标签"),
                                            ui::text("包名"),
                                            ui::text("级别"),
                                            ui::text("消息")};
        return section >= 0 && section < headers.size() ? headers[section] : QVariant();
    }

    const LogcatEntry &entryAt(int row) const
    {
        return m_entries[row];
    }

    const QVector<LogcatEntry> &entries() const
    {
        return m_entries;
    }

    void appendBatch(QVector<LogcatEntry> entries)
    {
        if (entries.isEmpty()) {
            return;
        }
        const qsizetype overflowSize = std::max<qsizetype>(
            0, m_entries.size() + entries.size() - kMaxEntries);
        const int overflow = static_cast<int>(overflowSize);
        if (overflow > 0) {
            beginRemoveRows(QModelIndex(), 0, overflow - 1);
            m_entries.remove(0, overflow);
            endRemoveRows();
        }
        const int first = m_entries.size();
        beginInsertRows(QModelIndex(), first, first + entries.size() - 1);
        m_entries += std::move(entries);
        endInsertRows();
    }

    void clear()
    {
        if (m_entries.isEmpty()) {
            return;
        }
        beginResetModel();
        m_entries.clear();
        endResetModel();
    }

private:
    QVector<LogcatEntry> m_entries;
};

class LogcatFilterModel : public QSortFilterProxyModel
{
public:
    explicit LogcatFilterModel(LogcatModel *source, QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
        , m_source(source)
    {
        setSourceModel(source);
        setDynamicSortFilter(true);
    }

    void setFilters(int minimumPriority, const QString &process, const QString &tag)
    {
        m_minimumPriority = minimumPriority;
        m_process = process.trimmed();
        m_tag = tag.trimmed();
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &) const override
    {
        const LogcatEntry &entry = m_source->entryAt(sourceRow);
        return priorityValue(entry.priority) >= m_minimumPriority
            && (m_process.isEmpty()
                || entry.processName.contains(m_process, Qt::CaseInsensitive))
            && (m_tag.isEmpty() || entry.tag.contains(m_tag, Qt::CaseInsensitive));
    }

private:
    LogcatModel *m_source = nullptr;
    int m_minimumPriority = 2;
    QString m_process;
    QString m_tag;
};

LogcatPage::LogcatPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("LogcatPage");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *toolbar = new QWidget;
    toolbar->setObjectName("LogcatToolbar");
    toolbar->setFixedHeight(50);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(10, 7, 12, 7);
    toolbarLayout->setSpacing(8);

    m_viewCombo = new QComboBox;
    m_viewCombo->setObjectName("LogcatSelect");
    m_viewCombo->addItem(ui::text("标准视图"), QStringLiteral("standard"));
    m_viewCombo->addItem(ui::text("紧凑视图"), QStringLiteral("compact"));
    m_viewCombo->setFixedWidth(118);
    connect(m_viewCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &LogcatPage::updateViewMode);

    m_priorityCombo = new QComboBox;
    m_priorityCombo->setObjectName("LogcatSelect");
    m_priorityCombo->addItem(QStringLiteral("VERBOSE"), 2);
    m_priorityCombo->addItem(QStringLiteral("DEBUG"), 3);
    m_priorityCombo->addItem(QStringLiteral("INFO"), 4);
    m_priorityCombo->addItem(QStringLiteral("WARNING"), 5);
    m_priorityCombo->addItem(QStringLiteral("ERROR"), 6);
    m_priorityCombo->setFixedWidth(112);
    connect(m_priorityCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &LogcatPage::updateFilters);

    m_packageInput = new QLineEdit;
    m_packageInput->setObjectName("LogcatFilterInput");
    m_packageInput->setPlaceholderText(ui::text("包名"));
    m_packageInput->setClearButtonEnabled(true);
    m_packageInput->setFixedWidth(190);
    connect(m_packageInput, &QLineEdit::textChanged, this, &LogcatPage::updateFilters);
    m_tagInput = new QLineEdit;
    m_tagInput->setObjectName("LogcatFilterInput");
    m_tagInput->setPlaceholderText(ui::text("标签"));
    m_tagInput->setClearButtonEnabled(true);
    m_tagInput->setFixedWidth(180);
    connect(m_tagInput, &QLineEdit::textChanged, this, &LogcatPage::updateFilters);

    toolbarLayout->addWidget(m_viewCombo);
    toolbarLayout->addWidget(m_priorityCombo);
    toolbarLayout->addWidget(m_packageInput);
    toolbarLayout->addWidget(m_tagInput);
    toolbarLayout->addStretch();

    m_statusLabel = new QLabel(ui::text("等待设备连接"));
    m_statusLabel->setObjectName("LogcatStatus");
    m_statusLabel->setFont(ui::appFont(8));
    toolbarLayout->addWidget(m_statusLabel);

    auto *saveButton = makeToolButton(QStringLiteral("▣"), ui::text("保存日志"));
    connect(saveButton, &QToolButton::clicked, this, &LogcatPage::saveEntries);
    m_wrapButton = makeToolButton(QStringLiteral("↵"), ui::text("软换行"));
    connect(m_wrapButton, &QToolButton::clicked, this, &LogcatPage::toggleSoftWrap);
    auto *bottomButton = makeToolButton(QStringLiteral("⇣"), ui::text("滚动到底部"));
    connect(bottomButton, &QToolButton::clicked, this, [this] {
        if (m_table != nullptr) {
            m_table->scrollToBottom();
        }
    });
    auto *restartButton = makeToolButton(QStringLiteral("↻"), ui::text("重新启动日志流"));
    connect(restartButton, &QToolButton::clicked, this, [this] {
        clearEntries();
        emit restartRequested();
    });
    m_pauseButton = makeToolButton(QStringLiteral("Ⅱ"), ui::text("暂停"));
    connect(m_pauseButton, &QToolButton::clicked, this, &LogcatPage::togglePaused);
    auto *clearButton = makeToolButton(QStringLiteral("×"), ui::text("清空日志"));
    connect(clearButton, &QToolButton::clicked, this, &LogcatPage::clearEntries);

    toolbarLayout->addWidget(saveButton);
    toolbarLayout->addWidget(m_wrapButton);
    toolbarLayout->addWidget(bottomButton);
    toolbarLayout->addWidget(restartButton);
    toolbarLayout->addWidget(m_pauseButton);
    toolbarLayout->addWidget(clearButton);
    layout->addWidget(toolbar);

    m_model = new LogcatModel(this);
    m_filterModel = new LogcatFilterModel(m_model, this);
    m_table = new QTableView;
    m_table->setObjectName("LogcatTable");
    m_table->setModel(m_filterModel);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setWordWrap(false);
    m_table->setTextElideMode(Qt::ElideRight);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_table->setFont(ui::appFont(9));
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(27);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setItemDelegateForColumn(LogcatModel::Priority,
                                      new LogcatPriorityDelegate(m_table));
    m_table->setColumnWidth(LogcatModel::Time, 190);
    m_table->setColumnWidth(LogcatModel::Thread, 92);
    m_table->setColumnWidth(LogcatModel::Tag, 176);
    m_table->setColumnWidth(LogcatModel::Process, 190);
    m_table->setColumnWidth(LogcatModel::Priority, 52);

    auto *empty = new QWidget;
    empty->setObjectName("LogcatEmpty");
    auto *emptyLayout = new QVBoxLayout(empty);
    m_emptyLabel = new QLabel(ui::text("连接 Android 设备后开始读取日志"));
    m_emptyLabel->setObjectName("LogcatEmptyLabel");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setFont(ui::appFont(10));
    emptyLayout->addWidget(m_emptyLabel);

    m_contentStack = new QStackedWidget;
    m_contentStack->addWidget(empty);
    m_contentStack->addWidget(m_table);
    layout->addWidget(m_contentStack, 1);

    m_flushTimer = new QTimer(this);
    m_flushTimer->setInterval(33);
    connect(m_flushTimer, &QTimer::timeout, this, &LogcatPage::flushEntries);
    m_flushTimer->start();
    connect(m_filterModel, &QAbstractItemModel::rowsInserted, this, [this] { updateStatus(); });
    connect(m_filterModel, &QAbstractItemModel::rowsRemoved, this, [this] { updateStatus(); });
    connect(m_filterModel, &QAbstractItemModel::modelReset, this, [this] { updateStatus(); });
}

void LogcatPage::setDeviceConnected(bool connected, const QString &serial)
{
    const bool changed = m_serial != serial;
    m_connected = connected;
    m_serial = connected ? serial : QString();
    m_contentStack->setCurrentIndex(connected ? 1 : 0);
    for (QWidget *widget : {static_cast<QWidget *>(m_viewCombo),
                            static_cast<QWidget *>(m_priorityCombo),
                            static_cast<QWidget *>(m_packageInput),
                            static_cast<QWidget *>(m_tagInput),
                            static_cast<QWidget *>(m_wrapButton),
                            static_cast<QWidget *>(m_pauseButton)}) {
        widget->setEnabled(connected);
    }
    if (!connected || changed) {
        clearEntries();
        m_paused = false;
        m_pauseButton->setText(QStringLiteral("Ⅱ"));
        m_pauseButton->setToolTip(ui::text("暂停"));
    }
    updateStatus();
}

void LogcatPage::appendEntry(const LogcatEntry &entry)
{
    if (!m_connected || m_paused) {
        return;
    }
    m_pendingEntries.append(entry);
    if (m_pendingEntries.size() > 2000) {
        flushEntries();
    }
}

void LogcatPage::setStreamRunning(bool running)
{
    m_streamRunning = running;
    updateStatus();
}

void LogcatPage::showStreamError(const QString &message)
{
    if (!m_connected) {
        return;
    }
    m_statusLabel->setText(ui::text("读取失败：%1").arg(message.left(60)));
    m_statusLabel->setToolTip(message);
    m_statusLabel->setStyleSheet(QStringLiteral("color:#d45b5b;"));
}

void LogcatPage::flushEntries()
{
    if (m_pendingEntries.isEmpty()) {
        return;
    }
    QVector<LogcatEntry> batch;
    batch.swap(m_pendingEntries);
    m_model->appendBatch(std::move(batch));
    m_table->scrollToBottom();
    updateStatus();
}

void LogcatPage::updateFilters()
{
    m_filterModel->setFilters(m_priorityCombo->currentData().toInt(),
                              m_packageInput->text(),
                              m_tagInput->text());
    updateStatus();
}

void LogcatPage::updateViewMode()
{
    const bool compact = m_viewCombo->currentData().toString() == QStringLiteral("compact");
    m_table->setColumnHidden(LogcatModel::Thread, compact);
    m_table->setColumnHidden(LogcatModel::Process, compact);
    m_table->setColumnWidth(LogcatModel::Time, compact ? 168 : 190);
    m_table->setColumnWidth(LogcatModel::Tag, compact ? 150 : 176);
}

void LogcatPage::updateStatus()
{
    if (!m_connected) {
        m_statusLabel->setText(ui::text("等待设备连接"));
        m_statusLabel->setStyleSheet(QStringLiteral("color:#7b8798;"));
        return;
    }
    const QString state = m_paused ? ui::text("已暂停")
                                   : (m_streamRunning ? ui::text("实时") : ui::text("连接中"));
    m_statusLabel->setText(ui::text("%1 · %2 / %3 行")
                               .arg(state)
                               .arg(m_filterModel->rowCount())
                               .arg(m_model->rowCount()));
    m_statusLabel->setStyleSheet(m_paused ? QStringLiteral("color:#e2a43a;")
                                          : QStringLiteral("color:#4f8f3d;"));
}

void LogcatPage::saveEntries()
{
    if (!m_connected) {
        return;
    }
    const QString suggested = QStringLiteral("%1.%2.log.txt")
                                  .arg(m_serial.isEmpty() ? QStringLiteral("logcat") : m_serial,
                                       QDateTime::currentDateTime().toString(
                                           QStringLiteral("yyyyMMddHHmmss")));
    const QString path = QFileDialog::getSaveFileName(this,
                                                      ui::text("保存日志"),
                                                      suggested,
                                                      ui::text("文本文件 (*.txt);;所有文件 (*.*)"));
    if (path.isEmpty()) {
        return;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, ui::text("保存失败"), file.errorString());
        return;
    }
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    for (const LogcatEntry &entry : m_model->entries()) {
        stream << entry.time << ' ' << entry.pid << '-' << entry.tid << ' '
               << entry.priority << ' ' << entry.tag << '/' << entry.processName << ": "
               << entry.message << '\n';
    }
    if (!file.commit()) {
        QMessageBox::warning(this, ui::text("保存失败"), file.errorString());
    }
}

void LogcatPage::clearEntries()
{
    m_pendingEntries.clear();
    m_model->clear();
    updateStatus();
}

void LogcatPage::toggleSoftWrap()
{
    m_softWrap = !m_softWrap;
    m_wrapButton->setProperty("active", m_softWrap);
    m_wrapButton->style()->unpolish(m_wrapButton);
    m_wrapButton->style()->polish(m_wrapButton);
    m_table->setWordWrap(m_softWrap);
    m_table->setTextElideMode(m_softWrap ? Qt::ElideNone : Qt::ElideRight);
    m_table->verticalHeader()->setSectionResizeMode(
        m_softWrap ? QHeaderView::ResizeToContents : QHeaderView::Fixed);
    if (!m_softWrap) {
        m_table->verticalHeader()->setDefaultSectionSize(27);
    }
}

void LogcatPage::togglePaused()
{
    m_paused = !m_paused;
    m_pauseButton->setText(m_paused ? QStringLiteral("▶") : QStringLiteral("Ⅱ"));
    m_pauseButton->setToolTip(m_paused ? ui::text("继续") : ui::text("暂停"));
    m_pauseButton->setProperty("active", m_paused);
    m_pauseButton->style()->unpolish(m_pauseButton);
    m_pauseButton->style()->polish(m_pauseButton);
    updateStatus();
}
