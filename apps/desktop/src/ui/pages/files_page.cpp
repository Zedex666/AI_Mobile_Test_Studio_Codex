#include "ui/pages/files_page.h"

#include "ui/common/widget_helpers.h"

#include <QAbstractItemView>
#include <QBoxLayout>
#include <QDir>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QStackedWidget>
#include <QStyle>
#include <QTableWidget>
#include <QToolButton>

#include <algorithm>

namespace {

QLabel *makeLabel(const QString &value,
                  int size,
                  QFont::Weight weight,
                  const QString &color)
{
    auto *result = new QLabel(value);
    result->setFont(ui::appFont(size, weight));
    result->setStyleSheet(QStringLiteral("color:%1;").arg(color));
    return result;
}

QToolButton *makeToolButton(const QString &text,
                            const QString &tooltip,
                            const QString &objectName = QStringLiteral("FilesToolButton"))
{
    auto *button = new QToolButton;
    button->setObjectName(objectName);
    button->setText(text);
    button->setToolTip(tooltip);
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedSize(34, 34);
    button->setFont(ui::appFont(14, QFont::DemiBold));
    return button;
}

} // namespace

FilesPage::FilesPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("FilesPage");
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(24, 18, 24, 16);
    pageLayout->setSpacing(12);

    auto *titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->addWidget(makeLabel(ui::text("Files"), 16, QFont::DemiBold, "#111827"));
    titleRow->addStretch();
    m_deviceDot = makeLabel(ui::text("●"), 12, QFont::DemiBold, "#aab3c2");
    m_deviceStatus = makeLabel(ui::text("未连接设备"), 9, QFont::DemiBold, "#596579");
    titleRow->addWidget(m_deviceDot);
    titleRow->addWidget(m_deviceStatus);
    pageLayout->addLayout(titleRow);

    auto *explorer = ui::makePanel("FilesExplorer");
    auto *explorerLayout = new QVBoxLayout(explorer);
    explorerLayout->setContentsMargins(0, 0, 0, 0);
    explorerLayout->setSpacing(0);

    auto *navigation = new QWidget;
    navigation->setObjectName("FilesNavigationBar");
    auto *navigationLayout = new QHBoxLayout(navigation);
    navigationLayout->setContentsMargins(12, 10, 12, 10);
    navigationLayout->setSpacing(8);
    m_backButton = makeToolButton(ui::text("←"), ui::text("后退"));
    m_forwardButton = makeToolButton(ui::text("→"), ui::text("前进"));
    m_upButton = makeToolButton(ui::text("↑"), ui::text("上一级"));
    m_refreshButton = makeToolButton(ui::text("↻"), ui::text("刷新"));
    navigationLayout->addWidget(m_backButton);
    navigationLayout->addWidget(m_forwardButton);
    navigationLayout->addWidget(m_upButton);
    navigationLayout->addWidget(m_refreshButton);

    m_addressInput = new QLineEdit;
    m_addressInput->setObjectName("FilesAddressInput");
    m_addressInput->setText(m_currentPath);
    m_addressInput->setClearButtonEnabled(false);
    m_addressInput->setMinimumWidth(260);
    navigationLayout->addWidget(m_addressInput, 1);

    m_searchInput = new QLineEdit;
    m_searchInput->setObjectName("FilesSearchInput");
    m_searchInput->setPlaceholderText(ui::text("在当前目录中搜索"));
    m_searchInput->setClearButtonEnabled(true);
    m_searchInput->setMinimumWidth(180);
    m_searchInput->setMaximumWidth(300);
    navigationLayout->addWidget(m_searchInput);
    m_listViewButton = makeToolButton(ui::text("☷"), ui::text("列表视图"), "FilesViewButton");
    m_gridViewButton = makeToolButton(ui::text("▦"), ui::text("网格视图"), "FilesViewButton");
    m_listViewButton->setProperty("active", true);
    navigationLayout->addWidget(m_listViewButton);
    navigationLayout->addWidget(m_gridViewButton);
    explorerLayout->addWidget(navigation);

    auto *actions = new QWidget;
    actions->setObjectName("FilesActionBar");
    auto *actionsLayout = new QHBoxLayout(actions);
    actionsLayout->setContentsMargins(12, 10, 12, 10);
    actionsLayout->setSpacing(8);
    m_newFolderButton = new QPushButton(ui::text("＋ 新建文件夹"));
    m_uploadButton = new QPushButton(ui::text("↑ 上传"));
    m_downloadButton = new QPushButton(ui::text("↓ 下载"));
    for (QPushButton *button : {m_newFolderButton, m_uploadButton, m_downloadButton}) {
        button->setObjectName("FilesPrimaryButton");
        button->setCursor(Qt::PointingHandCursor);
        button->setFont(ui::appFont(9, QFont::DemiBold));
        button->setMinimumHeight(34);
        actionsLayout->addWidget(button);
    }
    m_uploadButton->setProperty("primary", true);
    actionsLayout->addStretch();
    m_renameButton = makeToolButton(ui::text("✎"), ui::text("重命名"));
    m_duplicateButton = makeToolButton(ui::text("▣"), ui::text("创建副本"));
    m_permissionsButton = makeToolButton(ui::text("♙"), ui::text("修改权限"));
    m_deleteButton = makeToolButton(ui::text("⌫"), ui::text("删除"), "FilesDeleteButton");
    actionsLayout->addWidget(m_renameButton);
    actionsLayout->addWidget(m_duplicateButton);
    actionsLayout->addWidget(m_permissionsButton);
    actionsLayout->addWidget(m_deleteButton);
    explorerLayout->addWidget(actions);

    m_table = new QTableWidget;
    m_table->setObjectName("FilesTable");
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({ui::text("名称"),
                                        ui::text("类型"),
                                        ui::text("大小"),
                                        ui::text("权限"),
                                        ui::text("修改时间")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int column = 1; column < 5; ++column) {
        m_table->horizontalHeader()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    }

    m_grid = new QListWidget;
    m_grid->setObjectName("FilesGrid");
    m_grid->setViewMode(QListView::IconMode);
    m_grid->setResizeMode(QListView::Adjust);
    m_grid->setMovement(QListView::Static);
    m_grid->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_grid->setIconSize(QSize(38, 38));
    m_grid->setGridSize(QSize(150, 92));
    m_grid->setSpacing(8);

    m_viewStack = new QStackedWidget;
    m_viewStack->setObjectName("FilesViewStack");
    m_viewStack->addWidget(m_table);
    m_viewStack->addWidget(m_grid);
    explorerLayout->addWidget(m_viewStack, 1);

    auto *statusBar = new QWidget;
    statusBar->setObjectName("FilesStatusBar");
    auto *statusLayout = new QHBoxLayout(statusBar);
    statusLayout->setContentsMargins(12, 6, 12, 6);
    statusLayout->setSpacing(18);
    m_itemCount = makeLabel(ui::text("0 项"), 8, QFont::Normal, "#7b8798");
    m_selectionStatus = makeLabel(ui::text("未选择"), 8, QFont::Normal, "#7b8798");
    m_operationStatus = makeLabel(ui::text("连接设备后可浏览文件"),
                                  8,
                                  QFont::Normal,
                                  "#7b8798");
    statusLayout->addWidget(m_itemCount);
    statusLayout->addWidget(m_selectionStatus);
    statusLayout->addStretch();
    statusLayout->addWidget(m_operationStatus);
    explorerLayout->addWidget(statusBar);
    pageLayout->addWidget(explorer, 1);

    connect(m_backButton, &QToolButton::clicked, this, &FilesPage::goBack);
    connect(m_forwardButton, &QToolButton::clicked, this, &FilesPage::goForward);
    connect(m_upButton, &QToolButton::clicked, this, &FilesPage::goUp);
    connect(m_refreshButton, &QToolButton::clicked, this, &FilesPage::refresh);
    connect(m_addressInput, &QLineEdit::returnPressed, this, [this] {
        navigateTo(m_addressInput->text());
    });
    connect(m_searchInput, &QLineEdit::textChanged, this, &FilesPage::applyFilter);
    connect(m_listViewButton, &QToolButton::clicked, this, [this] {
        setViewMode(ViewMode::List);
    });
    connect(m_gridViewButton, &QToolButton::clicked, this, [this] {
        setViewMode(ViewMode::Grid);
    });
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &FilesPage::updateSelection);
    connect(m_grid, &QListWidget::itemSelectionChanged, this, &FilesPage::updateSelection);
    connect(m_table, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem *item) {
        if (item != nullptr) {
            openEntry(item->data(Qt::UserRole).toInt());
        }
    });
    connect(m_grid, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        if (item != nullptr) {
            openEntry(item->data(Qt::UserRole).toInt());
        }
    });
    connect(m_newFolderButton, &QPushButton::clicked, this, &FilesPage::createFolder);
    connect(m_uploadButton, &QPushButton::clicked, this, &FilesPage::uploadFiles);
    connect(m_downloadButton, &QPushButton::clicked, this, &FilesPage::downloadFiles);
    connect(m_renameButton, &QToolButton::clicked, this, &FilesPage::renameEntry);
    connect(m_duplicateButton, &QToolButton::clicked, this, &FilesPage::duplicateEntry);
    connect(m_permissionsButton, &QToolButton::clicked, this, &FilesPage::changePermissions);
    connect(m_deleteButton, &QToolButton::clicked, this, &FilesPage::deleteEntries);
    updateControls();
}

void FilesPage::setDeviceConnected(bool connected, const QString &serial)
{
    m_connected = connected;
    m_serial = connected ? serial : QString();
    m_deviceDot->setStyleSheet(connected ? QStringLiteral("color:#66c95e;")
                                          : QStringLiteral("color:#aab3c2;"));
    m_deviceStatus->setText(connected
                                ? ui::text("已连接 · %1").arg(serial)
                                : ui::text("未连接设备"));
    if (!connected) {
        m_entries.clear();
        m_table->setRowCount(0);
        m_grid->clear();
        m_itemCount->setText(ui::text("0 项"));
        m_operationStatus->setText(ui::text("连接设备后可浏览文件"));
    }
    updateControls();
}

void FilesPage::activate()
{
    if (m_connected && !m_busy) {
        refresh();
    }
}

void FilesPage::refresh()
{
    if (m_connected && !m_busy) {
        navigateTo(m_currentPath, false);
    }
}

void FilesPage::setBusy(bool busy)
{
    m_busy = busy;
    updateControls();
}

void FilesPage::setDirectory(const QString &path, const QVector<DeviceFileEntry> &entries)
{
    m_currentPath = normalizePath(path);
    m_entries = entries;
    m_addressInput->setText(m_currentPath);

    if (m_pendingHistoryIndex >= 0) {
        m_historyIndex = m_pendingHistoryIndex;
    } else if (m_pendingAddHistory
               && (m_historyIndex >= m_history.size()
                   || m_history.value(m_historyIndex) != m_currentPath)) {
        m_history = m_history.mid(0, m_historyIndex + 1);
        m_history.append(m_currentPath);
        m_historyIndex = m_history.size() - 1;
    }
    m_pendingAddHistory = false;
    m_pendingHistoryIndex = -1;

    m_table->setRowCount(entries.size());
    m_grid->clear();
    for (int row = 0; row < entries.size(); ++row) {
        const DeviceFileEntry &entry = entries[row];
        const QIcon icon = style()->standardIcon(entry.isDirectory
                                                     ? QStyle::SP_DirIcon
                                                     : QStyle::SP_FileIcon);
        auto *name = new QTableWidgetItem(icon, entry.name);
        name->setData(Qt::UserRole, row);
        name->setToolTip(entry.isLink
                             ? ui::text("链接到 %1").arg(entry.linkTarget)
                             : entry.name);
        auto *type = new QTableWidgetItem(entry.isLink
                                               ? ui::text("链接")
                                               : entry.isDirectory ? ui::text("文件夹")
                                                                   : ui::text("文件"));
        type->setData(Qt::UserRole, row);
        auto *size = new QTableWidgetItem(entry.isDirectory || entry.isLink
                                              ? QStringLiteral("-")
                                              : formatBytes(entry.size));
        size->setData(Qt::UserRole, row);
        auto *permissions = new QTableWidgetItem(entry.permissions);
        permissions->setData(Qt::UserRole, row);
        auto *modified = new QTableWidgetItem(entry.modified);
        modified->setData(Qt::UserRole, row);
        m_table->setItem(row, 0, name);
        m_table->setItem(row, 1, type);
        m_table->setItem(row, 2, size);
        m_table->setItem(row, 3, permissions);
        m_table->setItem(row, 4, modified);

        auto *gridItem = new QListWidgetItem(icon, entry.name);
        gridItem->setData(Qt::UserRole, row);
        gridItem->setToolTip(ui::text("%1\n%2\n%3")
                                 .arg(entry.name,
                                      entry.permissions,
                                      entry.isDirectory ? ui::text("文件夹")
                                                        : formatBytes(entry.size)));
        gridItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        m_grid->addItem(gridItem);
    }

    m_itemCount->setText(ui::text("%1 项").arg(entries.size()));
    m_operationStatus->setStyleSheet(QStringLiteral("color:#459b47;"));
    m_operationStatus->setText(ui::text("已加载 %1").arg(m_currentPath));
    applyFilter();
    updateSelection();
}

void FilesPage::showOperationStarted(const QString &label, const QString &displayCommand)
{
    m_operationStatus->setStyleSheet(QStringLiteral("color:#2f6df6;"));
    m_operationStatus->setText(ui::text("正在执行：%1").arg(label));
    m_operationStatus->setToolTip(displayCommand);
}

void FilesPage::showOperationFinished(bool success,
                                      const QString &label,
                                      const QString &detail)
{
    m_operationStatus->setStyleSheet(success ? QStringLiteral("color:#459b47;")
                                              : QStringLiteral("color:#d45b5b;"));
    m_operationStatus->setText(success ? ui::text("%1完成").arg(label)
                                       : ui::text("%1失败：%2").arg(label, detail));
}

void FilesPage::navigateTo(const QString &path, bool addHistory, int historyIndex)
{
    if (!m_connected || m_busy) {
        return;
    }
    m_pendingPath = normalizePath(path);
    m_pendingAddHistory = addHistory;
    m_pendingHistoryIndex = historyIndex;
    emit directoryRequested(m_pendingPath);
}

void FilesPage::goBack()
{
    if (m_historyIndex > 0) {
        navigateTo(m_history[m_historyIndex - 1], false, m_historyIndex - 1);
    }
}

void FilesPage::goForward()
{
    if (m_historyIndex + 1 < m_history.size()) {
        navigateTo(m_history[m_historyIndex + 1], false, m_historyIndex + 1);
    }
}

void FilesPage::goUp()
{
    if (m_currentPath == QStringLiteral("/")) {
        return;
    }
    const int separator = m_currentPath.lastIndexOf(QLatin1Char('/'));
    navigateTo(separator <= 0 ? QStringLiteral("/") : m_currentPath.left(separator));
}

void FilesPage::applyFilter()
{
    const QString query = m_searchInput->text().trimmed();
    for (int row = 0; row < m_entries.size(); ++row) {
        const bool visible = query.isEmpty()
            || m_entries[row].name.contains(query, Qt::CaseInsensitive)
            || m_entries[row].linkTarget.contains(query, Qt::CaseInsensitive);
        m_table->setRowHidden(row, !visible);
        if (QListWidgetItem *item = m_grid->item(row)) {
            item->setHidden(!visible);
        }
    }
}

void FilesPage::setViewMode(ViewMode mode)
{
    m_viewMode = mode;
    m_viewStack->setCurrentIndex(mode == ViewMode::List ? 0 : 1);
    m_listViewButton->setProperty("active", mode == ViewMode::List);
    m_gridViewButton->setProperty("active", mode == ViewMode::Grid);
    for (QToolButton *button : {m_listViewButton, m_gridViewButton}) {
        button->style()->unpolish(button);
        button->style()->polish(button);
    }
    updateSelection();
}

void FilesPage::openEntry(int index)
{
    if (index < 0 || index >= m_entries.size()) {
        return;
    }
    const DeviceFileEntry &entry = m_entries[index];
    if (entry.isDirectory) {
        navigateTo(remotePath(entry));
    } else if (entry.isLink && !entry.linkTarget.isEmpty()) {
        navigateTo(entry.linkTarget.startsWith(QLatin1Char('/'))
                       ? entry.linkTarget
                       : normalizePath(m_currentPath + QLatin1Char('/') + entry.linkTarget));
    }
}

void FilesPage::updateSelection()
{
    const int count = selectedIndexes().size();
    m_selectionStatus->setText(count == 0 ? ui::text("未选择")
                                          : ui::text("已选择 %1 项").arg(count));
    updateControls();
}

void FilesPage::updateControls()
{
    const int selectionCount = selectedIndexes().size();
    const bool available = m_connected && !m_busy;
    m_backButton->setEnabled(available && m_historyIndex > 0);
    m_forwardButton->setEnabled(available && m_historyIndex + 1 < m_history.size());
    m_upButton->setEnabled(available && m_currentPath != QStringLiteral("/"));
    m_refreshButton->setEnabled(available);
    m_addressInput->setEnabled(available);
    m_newFolderButton->setEnabled(available);
    m_uploadButton->setEnabled(available);
    m_downloadButton->setEnabled(available && selectionCount > 0);
    m_renameButton->setEnabled(available && selectionCount == 1);
    m_duplicateButton->setEnabled(available && selectionCount == 1);
    m_permissionsButton->setEnabled(available && selectionCount > 0);
    m_deleteButton->setEnabled(available && selectionCount > 0);
    m_table->setEnabled(available);
    m_grid->setEnabled(available);
}

QVector<int> FilesPage::selectedIndexes() const
{
    QSet<int> indexes;
    if (m_viewMode == ViewMode::List) {
        for (QTableWidgetItem *item : m_table->selectedItems()) {
            indexes.insert(item->data(Qt::UserRole).toInt());
        }
    } else {
        for (QListWidgetItem *item : m_grid->selectedItems()) {
            indexes.insert(item->data(Qt::UserRole).toInt());
        }
    }
    QVector<int> result(indexes.begin(), indexes.end());
    std::sort(result.begin(), result.end());
    return result;
}

QStringList FilesPage::selectedRemotePaths() const
{
    QStringList paths;
    for (int index : selectedIndexes()) {
        if (index >= 0 && index < m_entries.size()) {
            paths.append(remotePath(m_entries[index]));
        }
    }
    return paths;
}

QString FilesPage::remotePath(const DeviceFileEntry &entry) const
{
    return normalizePath(m_currentPath + QLatin1Char('/') + entry.name);
}

void FilesPage::createFolder()
{
    bool accepted = false;
    const QString name = QInputDialog::getText(this,
                                               ui::text("新建文件夹"),
                                               ui::text("文件夹名称"),
                                               QLineEdit::Normal,
                                               ui::text("新建文件夹"),
                                               &accepted)
                             .trimmed();
    if (!accepted) {
        return;
    }
    if (!validEntryName(name)) {
        QMessageBox::warning(this,
                             ui::text("无效名称"),
                             ui::text("名称不能为空，且不能包含 / 或 \\。"));
        return;
    }
    emit createFolderRequested(normalizePath(m_currentPath + QLatin1Char('/') + name));
}

void FilesPage::uploadFiles()
{
    const QStringList files = QFileDialog::getOpenFileNames(this, ui::text("选择要上传的文件"));
    if (!files.isEmpty()) {
        emit uploadRequested(files, m_currentPath);
    }
}

void FilesPage::downloadFiles()
{
    const QStringList paths = selectedRemotePaths();
    if (paths.isEmpty()) {
        return;
    }
    const QString directory = QFileDialog::getExistingDirectory(this, ui::text("选择下载目录"));
    if (!directory.isEmpty()) {
        emit downloadRequested(paths, directory);
    }
}

void FilesPage::renameEntry()
{
    const QVector<int> indexes = selectedIndexes();
    if (indexes.size() != 1) {
        return;
    }
    const DeviceFileEntry &entry = m_entries[indexes.first()];
    bool accepted = false;
    const QString name = QInputDialog::getText(this,
                                               ui::text("重命名"),
                                               ui::text("新名称"),
                                               QLineEdit::Normal,
                                               entry.name,
                                               &accepted)
                             .trimmed();
    if (!accepted || name == entry.name) {
        return;
    }
    if (!validEntryName(name)) {
        QMessageBox::warning(this, ui::text("无效名称"), ui::text("请输入有效的文件名。"));
        return;
    }
    emit renameRequested(remotePath(entry),
                         normalizePath(m_currentPath + QLatin1Char('/') + name));
}

void FilesPage::duplicateEntry()
{
    const QVector<int> indexes = selectedIndexes();
    if (indexes.size() != 1) {
        return;
    }
    const DeviceFileEntry &entry = m_entries[indexes.first()];
    const int extension = entry.isDirectory ? -1 : entry.name.lastIndexOf(QLatin1Char('.'));
    const QString suggested = extension > 0
        ? entry.name.left(extension) + ui::text(" - 副本") + entry.name.mid(extension)
        : entry.name + ui::text(" - 副本");
    bool accepted = false;
    const QString name = QInputDialog::getText(this,
                                               ui::text("创建副本"),
                                               ui::text("副本名称"),
                                               QLineEdit::Normal,
                                               suggested,
                                               &accepted)
                             .trimmed();
    if (!accepted) {
        return;
    }
    if (!validEntryName(name)) {
        QMessageBox::warning(this, ui::text("无效名称"), ui::text("请输入有效的文件名。"));
        return;
    }
    emit duplicateRequested(remotePath(entry),
                            normalizePath(m_currentPath + QLatin1Char('/') + name));
}

void FilesPage::changePermissions()
{
    const QStringList paths = selectedRemotePaths();
    if (paths.isEmpty()) {
        return;
    }
    bool accepted = false;
    const QString mode = QInputDialog::getText(this,
                                               ui::text("修改权限"),
                                               ui::text("权限模式（例如 755）"),
                                               QLineEdit::Normal,
                                               QStringLiteral("755"),
                                               &accepted)
                             .trimmed();
    static const QRegularExpression modePattern(QStringLiteral("^[0-7]{3,4}$"));
    if (!accepted) {
        return;
    }
    if (!modePattern.match(mode).hasMatch()) {
        QMessageBox::warning(this,
                             ui::text("无效权限"),
                             ui::text("请输入 3 或 4 位八进制权限，例如 755。"));
        return;
    }
    emit permissionsRequested(paths, mode);
}

void FilesPage::deleteEntries()
{
    const QStringList paths = selectedRemotePaths();
    if (paths.isEmpty()) {
        return;
    }
    const QMessageBox::StandardButton choice = QMessageBox::warning(
        this,
        ui::text("确认删除"),
        ui::text("确定要永久删除选中的 %1 项吗？此操作不可撤销。").arg(paths.size()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (choice == QMessageBox::Yes) {
        emit deleteRequested(paths);
    }
}

QString FilesPage::normalizePath(const QString &path)
{
    QString value = path.trimmed();
    value.replace(QLatin1Char('\\'), QLatin1Char('/'));
    QStringList parts;
    for (const QString &part : value.split(QLatin1Char('/'), Qt::SkipEmptyParts)) {
        if (part == QStringLiteral(".")) {
            continue;
        }
        if (part == QStringLiteral("..")) {
            if (!parts.isEmpty()) {
                parts.removeLast();
            }
        } else {
            parts.append(part);
        }
    }
    return QLatin1Char('/') + parts.join(QLatin1Char('/'));
}

QString FilesPage::formatBytes(qint64 bytes)
{
    static const QStringList units = {QStringLiteral("B"),
                                      QStringLiteral("KB"),
                                      QStringLiteral("MB"),
                                      QStringLiteral("GB")};
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit + 1 < units.size()) {
        value /= 1024.0;
        ++unit;
    }
    return unit == 0 ? QStringLiteral("%1 %2").arg(bytes).arg(units[unit])
                     : QStringLiteral("%1 %2").arg(value, 0, 'f', 1).arg(units[unit]);
}

bool FilesPage::validEntryName(const QString &name)
{
    return !name.isEmpty() && name != QStringLiteral(".") && name != QStringLiteral("..")
        && !name.contains(QLatin1Char('/')) && !name.contains(QLatin1Char('\\'))
        && !name.contains(QLatin1Char('\n')) && !name.contains(QLatin1Char('\r'));
}
