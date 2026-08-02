#include "ui/pages/process_page.h"

#include "ui/common/widget_helpers.h"

#include <QAbstractTableModel>
#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QFrame>
#include <QHeaderView>
#include <QHash>
#include <QIcon>
#include <QImage>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QStyle>
#include <QTableView>
#include <QTimer>
#include <QToolButton>

#include <algorithm>
#include <utility>

namespace {

enum ProcessRole {
    SortRole = Qt::UserRole,
    SearchRole,
    ApplicationRole
};

QLabel *makeLabel(const QString &text,
                  int size,
                  QFont::Weight weight,
                  const QString &color)
{
    auto *label = new QLabel(text);
    label->setFont(ui::appFont(size, weight));
    label->setStyleSheet(QStringLiteral("color:%1;").arg(color));
    return label;
}

} // namespace

class ProcessModel final : public QAbstractTableModel
{
public:
    enum Column {
        Name = 0,
        Cpu,
        CpuTime,
        Memory,
        Pid,
        User,
        ColumnCount
    };

    explicit ProcessModel(QObject *parent = nullptr)
        : QAbstractTableModel(parent)
        , m_applicationFallbackIcon(
              QApplication::style()->standardIcon(QStyle::SP_ComputerIcon))
        , m_processFallbackIcon(
              QApplication::style()->standardIcon(QStyle::SP_FileIcon))
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
        const DeviceProcessEntry &entry = m_entries[index.row()];
        if (role == Qt::DisplayRole) {
            switch (index.column()) {
            case Name:
                return entry.name;
            case Cpu:
                return QString::number(entry.cpuPercent, 'f', 1);
            case CpuTime:
                return entry.cpuTime;
            case Memory:
                return entry.memory;
            case Pid:
                return entry.pid;
            case User:
                return entry.user;
            default:
                return QVariant();
            }
        }
        if (role == Qt::DecorationRole && index.column() == Name) {
            const QIcon applicationIcon = m_applicationIcons.value(entry.packageName);
            if (entry.isApplication && !applicationIcon.isNull()) {
                return applicationIcon;
            }
            return entry.isApplication ? m_applicationFallbackIcon : m_processFallbackIcon;
        }
        if (role == Qt::ToolTipRole) {
            return entry.arguments;
        }
        if (role == SortRole) {
            switch (index.column()) {
            case Name:
                return entry.name.toLower();
            case Cpu:
                return entry.cpuPercent;
            case CpuTime:
                return entry.cpuTime;
            case Memory:
                return entry.memoryBytes;
            case Pid:
                return entry.pid;
            case User:
                return entry.user.toLower();
            default:
                return QVariant();
            }
        }
        if (role == SearchRole) {
            return QStringLiteral("%1 %2 %3 %4")
                .arg(entry.name, entry.arguments, entry.user)
                .arg(entry.pid);
        }
        if (role == ApplicationRole) {
            return entry.isApplication;
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
        static const QStringList headers = {ui::text("进程名称"),
                                            QStringLiteral("% CPU"),
                                            ui::text("CPU 时间"),
                                            ui::text("内存"),
                                            QStringLiteral("PID"),
                                            ui::text("用户")};
        return section >= 0 && section < headers.size() ? headers[section] : QVariant();
    }

    void setEntries(QVector<DeviceProcessEntry> entries)
    {
        if (m_entries.isEmpty()) {
            if (entries.isEmpty()) {
                return;
            }
            beginInsertRows(QModelIndex(), 0, entries.size() - 1);
            m_entries = std::move(entries);
            endInsertRows();
            return;
        }
        if (entries.isEmpty()) {
            beginRemoveRows(QModelIndex(), 0, m_entries.size() - 1);
            m_entries.clear();
            endRemoveRows();
            return;
        }

        QSet<int> incomingPids;
        incomingPids.reserve(entries.size());
        for (const DeviceProcessEntry &entry : std::as_const(entries)) {
            incomingPids.insert(entry.pid);
        }

        for (int row = m_entries.size() - 1; row >= 0;) {
            if (incomingPids.contains(m_entries[row].pid)) {
                --row;
                continue;
            }
            const int last = row;
            while (row >= 0 && !incomingPids.contains(m_entries[row].pid)) {
                --row;
            }
            const int first = row + 1;
            beginRemoveRows(QModelIndex(), first, last);
            m_entries.erase(m_entries.begin() + first, m_entries.begin() + last + 1);
            endRemoveRows();
        }

        QHash<int, int> rowsByPid;
        rowsByPid.reserve(m_entries.size());
        for (int row = 0; row < m_entries.size(); ++row) {
            rowsByPid.insert(m_entries[row].pid, row);
        }

        int firstChanged = m_entries.size();
        int lastChanged = -1;
        QVector<DeviceProcessEntry> additions;
        for (DeviceProcessEntry &entry : entries) {
            const auto iterator = rowsByPid.constFind(entry.pid);
            if (iterator == rowsByPid.cend()) {
                additions.append(std::move(entry));
                continue;
            }
            const int row = iterator.value();
            if (!sameEntry(m_entries[row], entry)) {
                m_entries[row] = std::move(entry);
                firstChanged = std::min(firstChanged, row);
                lastChanged = std::max(lastChanged, row);
            }
        }

        if (firstChanged <= lastChanged) {
            emit dataChanged(index(firstChanged, Name),
                             index(lastChanged, User),
                             {Qt::DisplayRole,
                              Qt::DecorationRole,
                              Qt::ToolTipRole,
                              SortRole,
                              SearchRole,
                              ApplicationRole});
        }
        if (!additions.isEmpty()) {
            const int first = m_entries.size();
            const int last = first + additions.size() - 1;
            beginInsertRows(QModelIndex(), first, last);
            m_entries += std::move(additions);
            endInsertRows();
        }
    }

    void setApplicationIcons(QHash<QString, QIcon> icons)
    {
        QSet<QString> affectedPackages(m_applicationIcons.keyBegin(),
                                       m_applicationIcons.keyEnd());
        for (auto iterator = icons.cbegin(); iterator != icons.cend(); ++iterator) {
            affectedPackages.insert(iterator.key());
        }
        m_applicationIcons = std::move(icons);
        notifyIconChanges(affectedPackages);
    }

    void mergeApplicationIcons(const QHash<QString, QIcon> &icons)
    {
        if (icons.isEmpty()) {
            return;
        }
        QSet<QString> affectedPackages;
        for (auto iterator = icons.cbegin(); iterator != icons.cend(); ++iterator) {
            const QIcon previous = m_applicationIcons.value(iterator.key());
            if (previous.isNull() || previous.cacheKey() != iterator.value().cacheKey()) {
                m_applicationIcons.insert(iterator.key(), iterator.value());
                affectedPackages.insert(iterator.key());
            }
        }
        notifyIconChanges(affectedPackages);
    }

    const DeviceProcessEntry *entryAt(int row) const
    {
        return row >= 0 && row < m_entries.size() ? &m_entries[row] : nullptr;
    }

    int rowForPid(int pid) const
    {
        for (int row = 0; row < m_entries.size(); ++row) {
            if (m_entries[row].pid == pid) {
                return row;
            }
        }
        return -1;
    }

private:
    static bool sameEntry(const DeviceProcessEntry &left,
                          const DeviceProcessEntry &right)
    {
        return left.name == right.name && left.cpuPercent == right.cpuPercent
            && left.cpuTime == right.cpuTime && left.memory == right.memory
            && left.memoryBytes == right.memoryBytes && left.pid == right.pid
            && left.user == right.user && left.arguments == right.arguments
            && left.packageName == right.packageName
            && left.isApplication == right.isApplication;
    }

    void notifyIconChanges(const QSet<QString> &packages)
    {
        if (packages.isEmpty() || m_entries.isEmpty()) {
            return;
        }
        int first = -1;
        for (int row = 0; row <= m_entries.size(); ++row) {
            const bool affected = row < m_entries.size()
                && packages.contains(m_entries[row].packageName);
            if (affected && first < 0) {
                first = row;
            } else if (!affected && first >= 0) {
                emit dataChanged(index(first, Name),
                                 index(row - 1, Name),
                                 {Qt::DecorationRole});
                first = -1;
            }
        }
    }

    QVector<DeviceProcessEntry> m_entries;
    QHash<QString, QIcon> m_applicationIcons;
    QIcon m_applicationFallbackIcon;
    QIcon m_processFallbackIcon;
};

class ProcessFilterProxyModel final : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    void setSearchText(const QString &text)
    {
        m_search = text.trimmed();
        invalidateFilter();
    }

    void setOnlyApps(bool onlyApps)
    {
        m_onlyApps = onlyApps;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        if (m_onlyApps && !index.data(ApplicationRole).toBool()) {
            return false;
        }
        return m_search.isEmpty()
            || index.data(SearchRole).toString().contains(m_search, Qt::CaseInsensitive);
    }

private:
    QString m_search;
    bool m_onlyApps = false;
};

ProcessPage::ProcessPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("ProcessPage");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *toolbar = new QWidget;
    toolbar->setObjectName("ProcessToolbar");
    toolbar->setFixedHeight(48);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(8, 5, 10, 5);
    toolbarLayout->setSpacing(10);

    m_filterInput = new QLineEdit;
    m_filterInput->setObjectName("ProcessFilterInput");
    m_filterInput->setPlaceholderText(ui::text("过滤"));
    m_filterInput->setClearButtonEnabled(true);
    m_filterInput->setFixedWidth(200);
    toolbarLayout->addWidget(m_filterInput);
    m_onlyApps = new QCheckBox(ui::text("仅显示应用"));
    m_onlyApps->setObjectName("ProcessOnlyApps");
    m_onlyApps->setFont(ui::appFont(9));
    toolbarLayout->addWidget(m_onlyApps);
    auto *separator = new QFrame;
    separator->setObjectName("ProcessSeparator");
    separator->setFrameShape(QFrame::VLine);
    separator->setFixedHeight(26);
    toolbarLayout->addWidget(separator);
    m_countLabel = makeLabel(ui::text("共 0 个进程"),
                             9,
                             QFont::Normal,
                             QStringLiteral("#3f4857"));
    toolbarLayout->addWidget(m_countLabel);
    toolbarLayout->addStretch();
    m_statusLabel = makeLabel(ui::text("等待设备连接"),
                              8,
                              QFont::Normal,
                              QStringLiteral("#7b8798"));
    toolbarLayout->addWidget(m_statusLabel);
    m_refreshButton = new QToolButton;
    m_refreshButton->setObjectName("ProcessToolButton");
    m_refreshButton->setText(QStringLiteral("↻"));
    m_refreshButton->setToolTip(ui::text("刷新进程"));
    m_refreshButton->setFixedSize(34, 34);
    m_refreshButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(m_refreshButton);
    m_stopButton = new QToolButton;
    m_stopButton->setObjectName("ProcessStopButton");
    m_stopButton->setText(QStringLiteral("×"));
    m_stopButton->setToolTip(ui::text("停止选中的应用"));
    m_stopButton->setFixedSize(34, 34);
    m_stopButton->setCursor(Qt::PointingHandCursor);
    m_stopButton->setEnabled(false);
    toolbarLayout->addWidget(m_stopButton);
    layout->addWidget(toolbar);

    m_model = new ProcessModel(this);
    m_proxy = new ProcessFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setSortRole(SortRole);
    m_proxy->setDynamicSortFilter(true);

    m_table = new QTableView;
    m_table->setObjectName("ProcessTable");
    m_table->setModel(m_proxy);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setSortingEnabled(true);
    m_table->setWordWrap(false);
    m_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_table->setIconSize(QSize(22, 22));
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(34);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->setColumnWidth(ProcessModel::Name, 460);
    m_table->setColumnWidth(ProcessModel::Cpu, 120);
    m_table->setColumnWidth(ProcessModel::CpuTime, 160);
    m_table->setColumnWidth(ProcessModel::Memory, 140);
    m_table->setColumnWidth(ProcessModel::Pid, 140);
    m_table->sortByColumn(ProcessModel::Cpu, Qt::DescendingOrder);
    layout->addWidget(m_table, 1);

    connect(m_filterInput, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_proxy->setSearchText(text);
        updateCount();
        updateSelection();
    });
    connect(m_onlyApps, &QCheckBox::toggled, this, [this](bool checked) {
        m_proxy->setOnlyApps(checked);
        updateCount();
        updateSelection();
    });
    connect(m_refreshButton, &QToolButton::clicked, this, &ProcessPage::refreshRequested);
    connect(m_stopButton, &QToolButton::clicked, this, &ProcessPage::stopSelectedPackage);
    connect(m_table->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &ProcessPage::updateSelection);

    m_scrollIdleTimer = new QTimer(this);
    m_scrollIdleTimer->setSingleShot(true);
    m_scrollIdleTimer->setInterval(180);
    connect(m_table->verticalScrollBar(), &QScrollBar::valueChanged, this, [this] {
        if (!m_applyingProcesses) {
            m_scrollIdleTimer->start();
        }
    });
    connect(m_scrollIdleTimer, &QTimer::timeout, this, [this] {
        if (!m_hasDeferredProcesses) {
            return;
        }
        m_hasDeferredProcesses = false;
        applyProcesses(std::move(m_deferredProcesses));
    });

    m_iconDecodeTimer = new QTimer(this);
    m_iconDecodeTimer->setSingleShot(true);
    connect(m_iconDecodeTimer, &QTimer::timeout, this, &ProcessPage::decodeNextIcon);
}

void ProcessPage::setDeviceConnected(bool connected, const QString &serial)
{
    const QString nextSerial = connected ? serial : QString();
    if (m_serial != nextSerial) {
        m_loadedIconPackages.clear();
        m_requestedIconPackages.clear();
        m_pendingIconApps.clear();
        m_iconDecodeTimer->stop();
        m_model->setApplicationIcons({});
    }
    m_connected = connected;
    m_serial = nextSerial;
    m_filterInput->setEnabled(connected);
    m_onlyApps->setEnabled(connected);
    m_refreshButton->setEnabled(connected && !m_sampling);
    if (!connected) {
        m_model->setEntries({});
        m_statusLabel->setText(ui::text("等待设备连接"));
        m_statusLabel->setStyleSheet(QStringLiteral("color:#7b8798;"));
        updateCount();
    } else {
        m_statusLabel->setText(ui::text("已连接 · %1").arg(serial));
        m_statusLabel->setStyleSheet(QStringLiteral("color:#459b47;"));
    }
    updateSelection();
}

void ProcessPage::setSampling(bool sampling)
{
    m_sampling = sampling;
    m_refreshButton->setEnabled(m_connected && !sampling);
    if (sampling) {
        m_statusLabel->setText(ui::text("正在读取进程…"));
        m_statusLabel->setStyleSheet(QStringLiteral("color:#596579;"));
    }
}

void ProcessPage::setProcesses(const QVector<DeviceProcessEntry> &processes)
{
    if (m_scrollIdleTimer->isActive()) {
        m_deferredProcesses = processes;
        m_hasDeferredProcesses = true;
        return;
    }
    applyProcesses(processes);
}

void ProcessPage::applyProcesses(QVector<DeviceProcessEntry> processes)
{
    const DeviceProcessEntry *selected = selectedProcess();
    const int selectedPid = selected != nullptr ? selected->pid : 0;
    const int sortColumn = m_table->horizontalHeader()->sortIndicatorSection();
    const Qt::SortOrder sortOrder = m_table->horizontalHeader()->sortIndicatorOrder();
    m_applyingProcesses = true;
    m_proxy->setDynamicSortFilter(false);
    m_model->setEntries(processes);
    m_proxy->setDynamicSortFilter(true);
    m_proxy->sort(sortColumn, sortOrder);
    QStringList packagesToLoad;
    for (const DeviceProcessEntry &process : processes) {
        if (!process.isApplication || process.packageName.isEmpty()
            || m_loadedIconPackages.contains(process.packageName)
            || m_requestedIconPackages.contains(process.packageName)) {
            continue;
        }
        m_requestedIconPackages.insert(process.packageName);
        packagesToLoad.append(process.packageName);
    }
    if (!packagesToLoad.isEmpty()) {
        emit applicationMetadataRequested(packagesToLoad);
    }
    updateCount();
    if (selectedPid > 0) {
        const int sourceRow = m_model->rowForPid(selectedPid);
        if (sourceRow >= 0) {
            const QModelIndex proxyIndex = m_proxy->mapFromSource(
                m_model->index(sourceRow, 0));
            if (proxyIndex.isValid()) {
                m_table->selectRow(proxyIndex.row());
            }
        }
    }
    m_statusLabel->setText(ui::text("进程列表已更新"));
    m_statusLabel->setStyleSheet(QStringLiteral("color:#459b47;"));
    updateSelection();
    m_applyingProcesses = false;
}

void ProcessPage::setApplications(const QVector<AndroidAppSummary> &apps)
{
    for (const AndroidAppSummary &app : apps) {
        m_pendingIconApps.enqueue(app);
    }
    if (!m_pendingIconApps.isEmpty() && !m_iconDecodeTimer->isActive()) {
        m_iconDecodeTimer->start(0);
    }
}

void ProcessPage::decodeNextIcon()
{
    if (m_pendingIconApps.isEmpty()) {
        return;
    }
    const AndroidAppSummary app = m_pendingIconApps.dequeue();
    QHash<QString, QIcon> icons;
    const QImage image = QImage::fromData(app.iconPng, "PNG");
    if (!image.isNull()) {
        icons.insert(app.packageName,
                     QIcon(QPixmap::fromImage(image.scaled(22,
                                                          22,
                                                          Qt::KeepAspectRatio,
                                                          Qt::SmoothTransformation))));
    }
    m_loadedIconPackages.insert(app.packageName);
    m_requestedIconPackages.remove(app.packageName);
    m_model->mergeApplicationIcons(icons);
    if (!m_pendingIconApps.isEmpty()) {
        m_iconDecodeTimer->start(8);
    }
}

void ProcessPage::showError(const QString &message)
{
    m_statusLabel->setText(ui::text("读取失败：%1").arg(message.left(90)));
    m_statusLabel->setToolTip(message);
    m_statusLabel->setStyleSheet(QStringLiteral("color:#d45b5b;"));
}

void ProcessPage::showStopResult(bool success,
                                 const QString &packageName,
                                 const QString &detail)
{
    m_statusLabel->setText(success ? ui::text("已停止 %1").arg(packageName)
                                   : ui::text("停止失败：%1").arg(detail.left(90)));
    m_statusLabel->setToolTip(detail);
    m_statusLabel->setStyleSheet(success ? QStringLiteral("color:#459b47;")
                                          : QStringLiteral("color:#d45b5b;"));
}

void ProcessPage::updateSelection()
{
    const DeviceProcessEntry *entry = selectedProcess();
    m_stopButton->setEnabled(m_connected && entry != nullptr
                             && !entry->packageName.isEmpty());
}

void ProcessPage::updateCount()
{
    m_countLabel->setText(ui::text("共 %1 个进程").arg(m_proxy->rowCount()));
}

void ProcessPage::stopSelectedPackage()
{
    const DeviceProcessEntry *entry = selectedProcess();
    if (entry == nullptr || entry->packageName.isEmpty()) {
        return;
    }
    const QMessageBox::StandardButton choice = QMessageBox::warning(
        this,
        ui::text("停止应用"),
        ui::text("确定要强制停止 %1 吗？").arg(entry->packageName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (choice == QMessageBox::Yes) {
        emit stopPackageRequested(entry->packageName);
    }
}

const DeviceProcessEntry *ProcessPage::selectedProcess() const
{
    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return nullptr;
    }
    const QModelIndex sourceIndex = m_proxy->mapToSource(rows.first());
    return m_model->entryAt(sourceIndex.row());
}
