#include "ui/pages/apps_page.h"

#include "ui/common/widget_helpers.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStyle>
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

QLabel *makeSelectableValue()
{
    auto *label = makeLabel(QStringLiteral("-"), 8, QFont::Normal, "#596579");
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

QPushButton *makeActionButton(const QString &text, bool danger = false)
{
    auto *button = new QPushButton(text);
    button->setObjectName("AppsActionButton");
    button->setProperty("danger", danger);
    button->setCursor(Qt::PointingHandCursor);
    button->setFont(ui::appFont(8, QFont::DemiBold));
    button->setMinimumHeight(34);
    return button;
}

} // namespace

AppsPage::AppsPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("AppsPage");
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(24, 18, 24, 16);
    pageLayout->setSpacing(12);

    auto *titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->addWidget(makeLabel(ui::text("应用"), 16, QFont::DemiBold, "#111827"));
    titleRow->addStretch();
    m_deviceDot = makeLabel(ui::text("●"), 12, QFont::DemiBold, "#aab3c2");
    m_deviceStatus = makeLabel(ui::text("未连接设备"), 9, QFont::DemiBold, "#596579");
    titleRow->addWidget(m_deviceDot);
    titleRow->addWidget(m_deviceStatus);
    pageLayout->addLayout(titleRow);

    m_operationStatus = makeLabel(ui::text("连接设备后可管理应用"),
                                  9,
                                  QFont::Normal,
                                  "#7b8798");
    m_operationStatus->setMinimumHeight(24);
    pageLayout->addWidget(m_operationStatus);

    auto *content = new QHBoxLayout;
    content->setContentsMargins(0, 0, 0, 0);
    content->setSpacing(14);

    auto *catalog = ui::makePanel("AppsCatalog");
    auto *catalogLayout = new QVBoxLayout(catalog);
    catalogLayout->setContentsMargins(0, 0, 0, 0);
    catalogLayout->setSpacing(0);

    auto *toolbar = new QWidget;
    toolbar->setObjectName("AppsToolbar");
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(12, 10, 12, 10);
    toolbarLayout->setSpacing(8);
    m_searchInput = new QLineEdit;
    m_searchInput->setObjectName("AppsSearchInput");
    m_searchInput->setPlaceholderText(ui::text("搜索应用名称或软件包"));
    m_searchInput->setClearButtonEnabled(true);
    toolbarLayout->addWidget(m_searchInput, 1);
    m_refreshButton = new QToolButton;
    m_refreshButton->setObjectName("AppsToolButton");
    m_refreshButton->setText(ui::text("↻"));
    m_refreshButton->setToolTip(ui::text("刷新应用列表"));
    m_refreshButton->setCursor(Qt::PointingHandCursor);
    m_refreshButton->setFixedSize(36, 36);
    m_refreshButton->setFont(ui::appFont(15, QFont::DemiBold));
    toolbarLayout->addWidget(m_refreshButton);
    m_installButton = new QToolButton;
    m_installButton->setObjectName("AppsInstallButton");
    m_installButton->setText(ui::text("＋"));
    m_installButton->setToolTip(ui::text("安装 APK"));
    m_installButton->setCursor(Qt::PointingHandCursor);
    m_installButton->setFixedSize(36, 36);
    m_installButton->setFont(ui::appFont(15, QFont::DemiBold));
    toolbarLayout->addWidget(m_installButton);
    catalogLayout->addWidget(toolbar);

    auto *filters = new QWidget;
    filters->setObjectName("AppsFilters");
    auto *filtersLayout = new QHBoxLayout(filters);
    filtersLayout->setContentsMargins(10, 8, 10, 8);
    filtersLayout->setSpacing(6);
    const QStringList filterNames = {ui::text("用户"),
                                     ui::text("全部"),
                                     ui::text("系统"),
                                     ui::text("已停用"),
                                     ui::text("已卸载")};
    for (int index = 0; index < filterNames.size(); ++index) {
        auto *button = new QPushButton(filterNames[index]);
        button->setObjectName("AppsFilterButton");
        button->setProperty("active", index == 0);
        button->setCursor(Qt::PointingHandCursor);
        button->setFont(ui::appFont(8, index == 0 ? QFont::DemiBold : QFont::Normal));
        button->setMinimumHeight(32);
        filtersLayout->addWidget(button);
        m_filterButtons.append(button);
        connect(button, &QPushButton::clicked, this, [this, index] {
            setFilter(index);
        });
    }
    catalogLayout->addWidget(filters);

    m_appList = new QListWidget;
    m_appList->setObjectName("AppsList");
    m_appList->setViewMode(QListView::IconMode);
    m_appList->setResizeMode(QListView::Adjust);
    m_appList->setMovement(QListView::Static);
    m_appList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_appList->setIconSize(QSize(40, 40));
    m_appList->setGridSize(QSize(174, 86));
    m_appList->setSpacing(8);
    catalogLayout->addWidget(m_appList, 1);
    content->addWidget(catalog, 5);

    auto *detailPanel = ui::makePanel("AppsDetailPanel");
    auto *detailPanelLayout = new QVBoxLayout(detailPanel);
    detailPanelLayout->setContentsMargins(0, 0, 0, 0);
    detailPanelLayout->setSpacing(0);

    auto *emptyDetails = new QWidget;
    auto *emptyLayout = new QVBoxLayout(emptyDetails);
    emptyLayout->addStretch();
    auto *emptyIcon = makeLabel(ui::text("▦"), 32, QFont::Normal, "#9aa5b5");
    emptyIcon->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyIcon);
    auto *emptyText = makeLabel(ui::text("选择一个应用查看详情"),
                                11,
                                QFont::DemiBold,
                                "#596579");
    emptyText->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyText);
    emptyLayout->addStretch();

    auto *detailScroll = new QScrollArea;
    detailScroll->setObjectName("AppsDetailScroll");
    detailScroll->setWidgetResizable(true);
    detailScroll->setFrameShape(QFrame::NoFrame);
    auto *detailContent = new QWidget;
    auto *detailLayout = new QVBoxLayout(detailContent);
    detailLayout->setContentsMargins(16, 16, 16, 16);
    detailLayout->setSpacing(12);

    auto *hero = new QHBoxLayout;
    hero->setContentsMargins(0, 0, 0, 0);
    hero->setSpacing(12);
    m_appIcon = makeLabel(ui::text("AP"), 15, QFont::DemiBold, "#245fe0");
    m_appIcon->setObjectName("AppsHeroIcon");
    m_appIcon->setFixedSize(58, 58);
    m_appIcon->setAlignment(Qt::AlignCenter);
    hero->addWidget(m_appIcon);
    auto *heroText = new QVBoxLayout;
    heroText->setSpacing(3);
    m_appTitle = makeLabel(QStringLiteral("-"), 13, QFont::DemiBold, "#172033");
    m_appTitle->setWordWrap(true);
    m_packageName = makeLabel(QStringLiteral("-"), 8, QFont::Normal, "#7b8798");
    m_packageName->setWordWrap(true);
    m_appType = makeLabel(QStringLiteral("-"), 8, QFont::DemiBold, "#2f6df6");
    heroText->addWidget(m_appTitle);
    heroText->addWidget(m_packageName);
    heroText->addWidget(m_appType);
    hero->addLayout(heroText, 1);
    detailLayout->addLayout(hero);

    auto *actionsTitle = makeLabel(ui::text("操作"), 9, QFont::DemiBold, "#596579");
    detailLayout->addWidget(actionsTitle);
    auto *actionsGrid = new QGridLayout;
    actionsGrid->setHorizontalSpacing(8);
    actionsGrid->setVerticalSpacing(8);
    m_launchButton = makeActionButton(ui::text("启动"));
    m_stopButton = makeActionButton(ui::text("停止"));
    m_enableButton = makeActionButton(ui::text("停用"));
    m_infoButton = makeActionButton(ui::text("系统信息"));
    m_clearButton = makeActionButton(ui::text("清除数据"));
    m_uninstallButton = makeActionButton(ui::text("卸载"), true);
    m_reinstallButton = makeActionButton(ui::text("恢复安装"));
    m_exportButton = makeActionButton(ui::text("导出 APK"));
    const QVector<QPushButton *> actionButtons = {m_launchButton,
                                                  m_stopButton,
                                                  m_enableButton,
                                                  m_infoButton,
                                                  m_clearButton,
                                                  m_uninstallButton,
                                                  m_reinstallButton,
                                                  m_exportButton};
    for (int index = 0; index < actionButtons.size(); ++index) {
        actionsGrid->addWidget(actionButtons[index], index / 3, index % 3);
    }
    detailLayout->addLayout(actionsGrid);

    auto *energyRow = new QHBoxLayout;
    energyRow->addWidget(makeLabel(ui::text("后台模式"), 9, QFont::DemiBold, "#596579"));
    m_backgroundMode = new QComboBox;
    m_backgroundMode->setObjectName("AppsBackgroundMode");
    m_backgroundMode->addItem(ui::text("不受限制"), QStringLiteral("unrestricted"));
    m_backgroundMode->addItem(ui::text("已优化"), QStringLiteral("optimized"));
    m_backgroundMode->addItem(ui::text("受限制"), QStringLiteral("restricted"));
    energyRow->addWidget(m_backgroundMode, 1);
    detailLayout->addLayout(energyRow);

    auto *infoPanel = ui::makePanel("AppsInfoPanel");
    auto *infoForm = new QFormLayout(infoPanel);
    infoForm->setContentsMargins(12, 10, 12, 10);
    infoForm->setHorizontalSpacing(12);
    infoForm->setVerticalSpacing(8);
    m_versionValue = makeSelectableValue();
    m_sdkValue = makeSelectableValue();
    m_installerValue = makeSelectableValue();
    m_installDateValue = makeSelectableValue();
    m_updateDateValue = makeSelectableValue();
    m_sizeValue = makeSelectableValue();
    m_apkPathValue = makeSelectableValue();
    m_dataPathValue = makeSelectableValue();
    infoForm->addRow(ui::text("版本"), m_versionValue);
    infoForm->addRow(ui::text("SDK"), m_sdkValue);
    infoForm->addRow(ui::text("安装来源"), m_installerValue);
    infoForm->addRow(ui::text("安装时间"), m_installDateValue);
    infoForm->addRow(ui::text("更新时间"), m_updateDateValue);
    infoForm->addRow(ui::text("APK 大小"), m_sizeValue);
    infoForm->addRow(ui::text("APK 路径"), m_apkPathValue);
    infoForm->addRow(ui::text("数据路径"), m_dataPathValue);
    detailLayout->addWidget(infoPanel);

    auto *permissionsHeader = new QHBoxLayout;
    permissionsHeader->addWidget(makeLabel(ui::text("权限"), 9, QFont::DemiBold, "#596579"));
    permissionsHeader->addStretch();
    m_permissionCount = makeLabel(ui::text("0 / 0"), 8, QFont::DemiBold, "#7b8798");
    permissionsHeader->addWidget(m_permissionCount);
    detailLayout->addLayout(permissionsHeader);
    m_permissionsContainer = new QWidget;
    m_permissionsContainer->setObjectName("AppsPermissions");
    m_permissionsLayout = new QVBoxLayout(m_permissionsContainer);
    m_permissionsLayout->setContentsMargins(0, 0, 0, 0);
    m_permissionsLayout->setSpacing(6);
    detailLayout->addWidget(m_permissionsContainer);
    detailLayout->addStretch();
    detailScroll->setWidget(detailContent);

    m_detailStack = new QStackedWidget;
    m_detailStack->addWidget(emptyDetails);
    m_detailStack->addWidget(detailScroll);
    detailPanelLayout->addWidget(m_detailStack);
    content->addWidget(detailPanel, 4);
    pageLayout->addLayout(content, 1);

    connect(m_searchInput, &QLineEdit::textChanged, this, &AppsPage::applyFilter);
    connect(m_refreshButton, &QToolButton::clicked, this, &AppsPage::refresh);
    connect(m_installButton, &QToolButton::clicked, this, &AppsPage::installApps);
    connect(m_appList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current) {
        selectApp(current);
    });
    connect(m_launchButton, &QPushButton::clicked, this, [this] {
        emit launchRequested(m_selectedPackage);
    });
    connect(m_stopButton, &QPushButton::clicked, this, [this] {
        emit stopRequested(m_selectedPackage);
    });
    connect(m_enableButton, &QPushButton::clicked, this, [this] {
        emit enabledRequested(m_selectedPackage, m_details.disabled);
    });
    connect(m_infoButton, &QPushButton::clicked, this, [this] {
        emit appInfoRequested(m_selectedPackage);
    });
    connect(m_clearButton, &QPushButton::clicked, this, &AppsPage::clearAppData);
    connect(m_uninstallButton, &QPushButton::clicked, this, &AppsPage::uninstallApp);
    connect(m_reinstallButton, &QPushButton::clicked, this, [this] {
        emit reinstallRequested(m_selectedPackage);
    });
    connect(m_exportButton, &QPushButton::clicked, this, &AppsPage::exportApk);
    connect(m_backgroundMode,
            &QComboBox::currentIndexChanged,
            this,
            [this](int index) {
                if (!m_selectedPackage.isEmpty() && index >= 0 && !m_busy) {
                    emit backgroundModeRequested(m_selectedPackage,
                                                 m_backgroundMode->itemData(index).toString());
                }
            });
    updateControls();
}

void AppsPage::setDeviceConnected(bool connected, const QString &serial)
{
    m_connected = connected;
    m_serial = connected ? serial : QString();
    m_deviceDot->setStyleSheet(connected ? QStringLiteral("color:#66c95e;")
                                          : QStringLiteral("color:#aab3c2;"));
    m_deviceStatus->setText(connected
                                ? ui::text("已连接 · %1").arg(serial)
                                : ui::text("未连接设备"));
    if (!connected) {
        m_apps.clear();
        m_appList->clear();
        clearDetails();
        m_operationStatus->setText(ui::text("连接设备后可管理应用"));
    }
    updateControls();
}

void AppsPage::activate()
{
    if (m_connected && !m_busy && m_apps.isEmpty()) {
        refresh();
    }
}

void AppsPage::refresh()
{
    if (m_connected && !m_busy) {
        emit appsRefreshRequested();
    }
}

void AppsPage::setBusy(bool busy)
{
    m_busy = busy;
    updateControls();
}

void AppsPage::setApps(const QVector<AndroidAppSummary> &apps)
{
    m_apps = apps;
    applyFilter();
}

void AppsPage::setAppDetails(const AndroidAppDetails &details)
{
    if (details.packageName != m_selectedPackage) {
        return;
    }
    for (int index = 0; index < m_apps.size(); ++index) {
        AndroidAppSummary &app = m_apps[index];
        if (app.packageName != details.packageName || details.displayName.isEmpty()) {
            continue;
        }
        app.displayName = details.displayName;
        for (int row = 0; row < m_appList->count(); ++row) {
            QListWidgetItem *item = m_appList->item(row);
            if (item->data(Qt::UserRole).toInt() != index) {
                continue;
            }
            const QString state = app.uninstalled ? ui::text("已卸载")
                : app.disabled ? ui::text("已停用")
                               : app.systemApp ? ui::text("系统") : ui::text("用户");
            item->setText(app.displayName + QLatin1Char('\n') + app.packageName
                          + QStringLiteral("  ·  ") + state);
            break;
        }
        break;
    }
    m_details = details;
    rebuildDetails();
}

void AppsPage::showOperationStarted(const QString &label, const QString &displayCommand)
{
    m_operationStatus->setStyleSheet(QStringLiteral("color:#2f6df6;"));
    m_operationStatus->setText(ui::text("正在执行：%1").arg(label));
    m_operationStatus->setToolTip(displayCommand);
}

void AppsPage::showOperationFinished(bool success,
                                     const QString &label,
                                     const QString &detail)
{
    m_operationStatus->setStyleSheet(success ? QStringLiteral("color:#459b47;")
                                              : QStringLiteral("color:#d45b5b;"));
    const QString summary = detail.section(QLatin1Char('\n'), 0, 0).simplified().left(160);
    m_operationStatus->setText(success ? ui::text("%1完成").arg(label)
                                       : ui::text("%1失败：%2").arg(label, summary));
    m_operationStatus->setToolTip(detail);
}

void AppsPage::handleAppStateChanged(const QString &packageName,
                                     bool refreshList,
                                     bool refreshDetails)
{
    if (refreshList) {
        clearDetails();
        emit appsRefreshRequested();
    } else if (refreshDetails && packageName == m_selectedPackage) {
        emit appDetailsRequested(packageName);
    }
}

void AppsPage::setFilter(int filter)
{
    m_filter = filter;
    for (int index = 0; index < m_filterButtons.size(); ++index) {
        QPushButton *button = m_filterButtons[index];
        button->setProperty("active", index == filter);
        button->setFont(ui::appFont(8, index == filter ? QFont::DemiBold : QFont::Normal));
        button->style()->unpolish(button);
        button->style()->polish(button);
    }
    applyFilter();
}

void AppsPage::applyFilter()
{
    const QString selected = m_selectedPackage;
    const QString query = m_searchInput->text().trimmed();
    m_appList->clear();
    QVector<int> counts(5, 0);
    for (const AndroidAppSummary &app : m_apps) {
        if (!app.systemApp && !app.disabled && !app.uninstalled) {
            ++counts[0];
        }
        ++counts[1];
        if (app.systemApp && !app.disabled && !app.uninstalled) {
            ++counts[2];
        }
        if (app.disabled && !app.uninstalled) {
            ++counts[3];
        }
        if (app.uninstalled) {
            ++counts[4];
        }
    }
    const QStringList names = {ui::text("用户"),
                               ui::text("全部"),
                               ui::text("系统"),
                               ui::text("已停用"),
                               ui::text("已卸载")};
    for (int index = 0; index < m_filterButtons.size(); ++index) {
        m_filterButtons[index]->setText(QStringLiteral("%1 %2").arg(names[index]).arg(counts[index]));
    }

    for (int index = 0; index < m_apps.size(); ++index) {
        const AndroidAppSummary &app = m_apps[index];
        const bool matchesFilter = m_filter == 1
            || (m_filter == 0 && !app.systemApp && !app.disabled && !app.uninstalled)
            || (m_filter == 2 && app.systemApp && !app.disabled && !app.uninstalled)
            || (m_filter == 3 && app.disabled && !app.uninstalled)
            || (m_filter == 4 && app.uninstalled);
        const bool matchesSearch = query.isEmpty()
            || app.displayName.contains(query, Qt::CaseInsensitive)
            || app.packageName.contains(query, Qt::CaseInsensitive);
        if (!matchesFilter || !matchesSearch) {
            continue;
        }
        const QString state = app.uninstalled ? ui::text("已卸载")
            : app.disabled ? ui::text("已停用")
                           : app.systemApp ? ui::text("系统") : ui::text("用户");
        auto *item = new QListWidgetItem(style()->standardIcon(QStyle::SP_ComputerIcon),
                                         app.displayName + QLatin1Char('\n') + app.packageName
                                             + QStringLiteral("  ·  ") + state);
        item->setData(Qt::UserRole, index);
        item->setSizeHint(QSize(170, 78));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        m_appList->addItem(item);
        if (app.packageName == selected) {
            m_appList->setCurrentItem(item);
        }
    }
}

void AppsPage::selectApp(QListWidgetItem *item)
{
    if (item == nullptr) {
        clearDetails();
        return;
    }
    const int index = item->data(Qt::UserRole).toInt();
    if (index < 0 || index >= m_apps.size()) {
        return;
    }
    const AndroidAppSummary &app = m_apps[index];
    m_selectedPackage = app.packageName;
    m_selectedUninstalled = app.uninstalled;
    m_details = {};
    m_details.packageName = app.packageName;
    m_details.displayName = app.displayName;
    m_details.apkPath = app.apkPath;
    m_details.systemApp = app.systemApp;
    m_details.disabled = app.disabled;
    rebuildDetails();
    if (!app.uninstalled) {
        emit appDetailsRequested(app.packageName);
    }
}

void AppsPage::clearDetails()
{
    m_selectedPackage.clear();
    m_selectedUninstalled = false;
    m_details = {};
    m_detailStack->setCurrentIndex(0);
    m_appList->clearSelection();
    updateControls();
}

void AppsPage::rebuildDetails()
{
    m_detailStack->setCurrentIndex(1);
    const QString initials = m_details.displayName.left(2).toUpper();
    m_appIcon->setText(initials.isEmpty() ? ui::text("AP") : initials);
    m_appTitle->setText(m_details.displayName.isEmpty() ? m_selectedPackage
                                                        : m_details.displayName);
    m_packageName->setText(m_selectedPackage);
    m_appType->setText(m_selectedUninstalled
                           ? ui::text("已卸载（当前用户）")
                           : m_details.disabled ? ui::text("已停用")
                                               : m_details.systemApp ? ui::text("系统应用")
                                                                     : ui::text("用户应用"));
    m_enableButton->setText(m_details.disabled ? ui::text("启用") : ui::text("停用"));
    m_versionValue->setText(QStringLiteral("%1 (%2)")
                                .arg(m_details.versionName.isEmpty() ? QStringLiteral("-")
                                                                     : m_details.versionName,
                                     m_details.versionCode.isEmpty() ? QStringLiteral("-")
                                                                     : m_details.versionCode));
    m_sdkValue->setText(QStringLiteral("min %1 / target %2")
                            .arg(m_details.minSdk.isEmpty() ? QStringLiteral("-")
                                                           : m_details.minSdk,
                                 m_details.targetSdk.isEmpty() ? QStringLiteral("-")
                                                              : m_details.targetSdk));
    m_installerValue->setText(m_details.installer.isEmpty() ? QStringLiteral("-")
                                                             : m_details.installer);
    m_installDateValue->setText(m_details.installDate.isEmpty() ? QStringLiteral("-")
                                                                 : m_details.installDate);
    m_updateDateValue->setText(m_details.updateDate.isEmpty() ? QStringLiteral("-")
                                                               : m_details.updateDate);
    m_sizeValue->setText(formatBytes(m_details.codeSize));
    m_apkPathValue->setText(m_details.apkPath.isEmpty() ? QStringLiteral("-")
                                                        : m_details.apkPath);
    m_dataPathValue->setText(m_details.dataDirectory.isEmpty() ? QStringLiteral("-")
                                                               : m_details.dataDirectory);
    const int backgroundIndex = m_backgroundMode->findData(m_details.backgroundMode);
    if (backgroundIndex >= 0) {
        const QSignalBlocker blocker(m_backgroundMode);
        m_backgroundMode->setCurrentIndex(backgroundIndex);
    }
    rebuildPermissions();
    updateControls();
}

void AppsPage::rebuildPermissions()
{
    while (QLayoutItem *item = m_permissionsLayout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
    int granted = 0;
    for (const AndroidAppPermission &permission : m_details.permissions) {
        granted += permission.granted ? 1 : 0;
        auto *row = new QWidget;
        row->setObjectName("AppsPermissionRow");
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(10, 7, 10, 7);
        layout->setSpacing(8);
        auto *name = makeLabel(permission.name.section(QLatin1Char('.'), -1),
                               8,
                               QFont::DemiBold,
                               "#293243");
        name->setToolTip(permission.name);
        layout->addWidget(name, 1);
        auto *toggle = new QCheckBox;
        toggle->setChecked(permission.granted);
        toggle->setEnabled(permission.changeable && !m_busy);
        toggle->setToolTip(permission.changeable
                               ? ui::text("允许修改")
                               : ui::text("设备固定或只读权限"));
        layout->addWidget(toggle);
        connect(toggle, &QCheckBox::clicked, this, [this, permission](bool checked) {
            emit permissionRequested(m_selectedPackage, permission.name, checked);
        });
        m_permissionsLayout->addWidget(row);
    }
    if (m_details.permissions.isEmpty()) {
        auto *empty = makeLabel(ui::text("没有可显示的权限"), 8, QFont::Normal, "#7b8798");
        empty->setAlignment(Qt::AlignCenter);
        m_permissionsLayout->addWidget(empty);
    }
    m_permissionCount->setText(QStringLiteral("%1 / %2")
                                   .arg(granted)
                                   .arg(m_details.permissions.size()));
}

void AppsPage::updateControls()
{
    const bool available = m_connected && !m_busy;
    const bool selected = available && !m_selectedPackage.isEmpty();
    m_searchInput->setEnabled(available);
    m_refreshButton->setEnabled(available);
    m_installButton->setEnabled(available);
    m_appList->setEnabled(available);
    for (QPushButton *button : m_filterButtons) {
        button->setEnabled(available);
    }
    m_launchButton->setEnabled(selected && !m_selectedUninstalled);
    m_stopButton->setEnabled(selected && !m_selectedUninstalled);
    m_enableButton->setEnabled(selected && !m_selectedUninstalled);
    m_infoButton->setEnabled(selected && !m_selectedUninstalled);
    m_clearButton->setEnabled(selected && !m_selectedUninstalled);
    m_uninstallButton->setEnabled(selected && !m_selectedUninstalled);
    m_reinstallButton->setEnabled(selected && m_selectedUninstalled);
    m_reinstallButton->setVisible(m_selectedUninstalled);
    m_exportButton->setEnabled(selected && !m_selectedUninstalled
                               && !m_details.apkPath.isEmpty()
                               && m_details.apkPath != QStringLiteral("-"));
    m_backgroundMode->setEnabled(selected && !m_selectedUninstalled);
    if (!m_selectedPackage.isEmpty()) {
        rebuildPermissions();
    }
}

void AppsPage::installApps()
{
    const QStringList files = QFileDialog::getOpenFileNames(this,
                                                            ui::text("选择 APK 文件"),
                                                            QString(),
                                                            ui::text("Android APK (*.apk)"));
    if (files.isEmpty()) {
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(ui::text("安装选项"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *replace = new QCheckBox(ui::text("替换已安装的应用"));
    replace->setChecked(true);
    auto *grant = new QCheckBox(ui::text("授予运行时权限"));
    auto *bypass = new QCheckBox(ui::text("允许安装低 targetSdk 应用"));
    bypass->setChecked(true);
    layout->addWidget(replace);
    layout->addWidget(grant);
    layout->addWidget(bypass);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() == QDialog::Accepted) {
        emit installRequested(files, replace->isChecked(), grant->isChecked(), bypass->isChecked());
    }
}

void AppsPage::clearAppData()
{
    const QMessageBox::StandardButton choice = QMessageBox::warning(
        this,
        ui::text("确认清除应用数据"),
        ui::text("确定要清除“%1”的所有数据吗？此操作不可撤销。").arg(m_selectedPackage),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (choice == QMessageBox::Yes) {
        emit clearDataRequested(m_selectedPackage);
    }
}

void AppsPage::uninstallApp()
{
    const QString detail = m_details.systemApp
        ? ui::text("系统应用将仅为当前用户卸载，并保留应用数据。")
        : ui::text("应用及其数据将从设备中删除。");
    const QMessageBox::StandardButton choice = QMessageBox::warning(
        this,
        ui::text("确认卸载"),
        ui::text("确定要卸载“%1”吗？\n%2").arg(m_selectedPackage, detail),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (choice == QMessageBox::Yes) {
        emit uninstallRequested(m_selectedPackage, m_details.systemApp);
    }
}

void AppsPage::exportApk()
{
    if (m_details.splitApk) {
        QMessageBox::information(this,
                                 ui::text("拆分 APK"),
                                 ui::text("该应用包含拆分 APK，本次仅导出基础 APK。"));
    }
    const QString destination = QFileDialog::getSaveFileName(
        this,
        ui::text("导出 APK"),
        m_selectedPackage + QStringLiteral(".apk"),
        ui::text("Android APK (*.apk)"));
    if (!destination.isEmpty()) {
        emit exportRequested(m_details.apkPath, destination);
    }
}

QString AppsPage::formatBytes(qint64 bytes)
{
    if (bytes < 0) {
        return QStringLiteral("-");
    }
    double value = static_cast<double>(bytes);
    const QStringList units = {QStringLiteral("B"),
                               QStringLiteral("KB"),
                               QStringLiteral("MB"),
                               QStringLiteral("GB")};
    int unit = 0;
    while (value >= 1024.0 && unit + 1 < units.size()) {
        value /= 1024.0;
        ++unit;
    }
    return unit == 0 ? QStringLiteral("%1 B").arg(bytes)
                     : QStringLiteral("%1 %2").arg(value, 0, 'f', 1).arg(units[unit]);
}
