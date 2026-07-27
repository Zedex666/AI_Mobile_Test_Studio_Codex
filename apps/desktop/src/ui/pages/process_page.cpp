#include "ui/pages/process_page.h"

#include "ui/common/widget_helpers.h"

#include <QAbstractTableModel>
#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QFrame>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QStyle>
#include <QTableView>
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
            return QApplication::style()->standardIcon(entry.isApplication
                                                            ? QStyle::SP_ComputerIcon
                                                            : QStyle::SP_FileIcon);
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
        beginResetModel();
        m_entries = std::move(entries);
        endResetModel();
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
    QVector<DeviceProcessEntry> m_entries;
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
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(30);
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
}

void ProcessPage::setDeviceConnected(bool connected, const QString &serial)
{
    m_connected = connected;
    m_serial = connected ? serial : QString();
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
    const DeviceProcessEntry *selected = selectedProcess();
    const int selectedPid = selected != nullptr ? selected->pid : 0;
    m_model->setEntries(processes);
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
