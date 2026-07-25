#include "ui/windows/main_window.h"

#include "services/adb_control_service.h"
#include "services/apps_service.h"
#include "services/file_manager_service.h"
#include "services/package_manager_service.h"
#include "services/performance_service.h"
#include "services/recovery_service.h"
#include "ui/common/widget_helpers.h"
#include "ui/components/main_window_sections.h"
#include "ui/pages/device_control_page.h"
#include "ui/pages/apps_page.h"
#include "ui/pages/files_page.h"
#include "ui/pages/package_manager_page.h"
#include "ui/pages/performance_page.h"
#include "ui/pages/recovery_page.h"
#include "ui/styles/app_style.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QStyle>

namespace {

const QString kDefaultScrcpyPath = QStringLiteral(
    "D:/myApp_666666666666666/scrcpy-win64-v4.0/scrcpy.exe");

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    configureScrcpy();
    configureDeviceControls();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi()
{
    setWindowTitle(ui::text("AI Mobile Test Studio"));
    setMinimumSize(1280, 760);
    resize(1600, 900);
    qApp->setStyleSheet(ui::appStyleSheet());

    auto *root = ui::makePanel("Root");
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(ui::createHeader());

    auto *content = new QWidget;
    auto *contentLayout = new QHBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    const ui::SidebarSection sidebar = ui::createSidebar();
    m_chatNavButton = sidebar.chatButton;
    m_deviceControlNavButton = sidebar.deviceControlButton;
    m_packageManagerNavButton = sidebar.packageManagerButton;
    m_appsNavButton = sidebar.appsButton;
    m_filesNavButton = sidebar.filesButton;
    m_recoveryNavButton = sidebar.recoveryButton;
    m_performanceNavButton = sidebar.performanceButton;
    m_sidebarStatusDot = sidebar.statusDot;
    m_sidebarStatusTitle = sidebar.statusTitle;
    m_sidebarStatusDetail = sidebar.statusDetail;
    contentLayout->addWidget(sidebar.widget);

    auto *workspace = new QWidget;
    auto *workspaceLayout = new QVBoxLayout(workspace);
    workspaceLayout->setContentsMargins(0, 0, 0, 0);
    workspaceLayout->setSpacing(0);
    const ui::ToolbarSection toolbar = ui::createToolbar();
    m_mirrorButton = toolbar.mirrorButton;
    m_deviceNameLabel = toolbar.deviceNameLabel;
    m_deviceStatusDot = toolbar.deviceStatusDot;
    m_deviceStatusLabel = toolbar.deviceStatusLabel;
    workspaceLayout->addWidget(toolbar.widget);

    auto *mainArea = new QWidget;
    auto *mainLayout = new QHBoxLayout(mainArea);
    mainLayout->setContentsMargins(16, 16, 16, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(ui::createChatPane(), 1);

    m_workspaceStack = new QStackedWidget;
    m_workspaceStack->setObjectName("WorkspaceStack");
    m_workspaceStack->addWidget(mainArea);
    m_deviceControlPage = new DeviceControlPage;
    m_workspaceStack->addWidget(m_deviceControlPage);
    m_packageManagerPage = new PackageManagerPage;
    m_workspaceStack->addWidget(m_packageManagerPage);
    m_appsPage = new AppsPage;
    m_workspaceStack->addWidget(m_appsPage);
    m_filesPage = new FilesPage;
    m_workspaceStack->addWidget(m_filesPage);
    m_recoveryPage = new RecoveryPage;
    m_workspaceStack->addWidget(m_recoveryPage);
    m_performancePage = new PerformancePage;
    m_workspaceStack->addWidget(m_performancePage);
    workspaceLayout->addWidget(m_workspaceStack, 1);

    contentLayout->addWidget(workspace, 1);
    rootLayout->addWidget(content, 1);
    setCentralWidget(root);
}

void MainWindow::configureScrcpy()
{
    QSettings settings(QStringLiteral("AI Mobile Test Studio"),
                       QStringLiteral("AI Mobile Test Studio"));
    QString scrcpyPath = qEnvironmentVariable("AI_MOBILE_TEST_SCRCPY_PATH");
    if (scrcpyPath.isEmpty()) {
        scrcpyPath = settings.value(QStringLiteral("runtime/scrcpyPath")).toString();
    }
    if (scrcpyPath.isEmpty()) {
        const QString bundledPath = QDir(QCoreApplication::applicationDirPath())
                                        .filePath(QStringLiteral("runtime/scrcpy/scrcpy.exe"));
        scrcpyPath = QFileInfo::exists(bundledPath) ? bundledPath : kDefaultScrcpyPath;
    }

    m_scrcpyService = new ScrcpyService(scrcpyPath, this);

    connect(m_mirrorButton, &QPushButton::clicked, this, [this] {
        if (m_scrcpyService->mirrorRunning()) {
            m_scrcpyService->stopMirror();
        } else {
            m_scrcpyService->startMirror();
        }
    });
    connect(m_scrcpyService,
            &ScrcpyService::deviceStateChanged,
            this,
            &MainWindow::updateDeviceUi);
    connect(m_scrcpyService,
            &ScrcpyService::mirrorRunningChanged,
            this,
            &MainWindow::updateMirrorUi);
    connect(m_scrcpyService, &ScrcpyService::operationError, this, [this](const QString &message) {
        QMessageBox::warning(this, ui::text("scrcpy 运行错误"), message);
    });

    m_scrcpyService->startMonitoring();
}

void MainWindow::configureDeviceControls()
{
    m_adbControlService = new AdbControlService(m_scrcpyService->adbExecutablePath(), this);

    connect(m_chatNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(0);
    });
    connect(m_deviceControlNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(1);
    });
    connect(m_packageManagerNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(2);
    });
    connect(m_appsNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(3);
    });
    connect(m_filesNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(4);
    });
    connect(m_recoveryNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(5);
    });
    connect(m_performanceNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(6);
    });
    connect(m_deviceControlPage,
            &DeviceControlPage::keyEventRequested,
            m_adbControlService,
            &AdbControlService::sendKeyEvent);
    connect(m_deviceControlPage,
            &DeviceControlPage::rebootRequested,
            this,
            [this](const QString &mode, const QString &label) {
                const QMessageBox::StandardButton choice = QMessageBox::question(
                    this,
                    ui::text("确认设备重启"),
                    ui::text("确定要执行“%1”吗？设备连接会暂时中断。").arg(label),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);
                if (choice == QMessageBox::Yes) {
                    m_adbControlService->rebootDevice(mode, label);
                }
            });
    connect(m_deviceControlPage,
            &DeviceControlPage::powerOffRequested,
            this,
            [this](const QString &label) {
                const QMessageBox::StandardButton choice = QMessageBox::question(
                    this,
                    ui::text("确认关闭设备"),
                    ui::text("确定要关闭当前 Android 设备吗？关闭后需要手动开机。"),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);
                if (choice == QMessageBox::Yes) {
                    m_adbControlService->powerOffDevice(label);
                }
            });
    connect(m_adbControlService,
            &AdbControlService::commandStarted,
            m_deviceControlPage,
            &DeviceControlPage::showCommandStarted);
    connect(m_adbControlService,
            &AdbControlService::commandFinished,
            m_deviceControlPage,
            &DeviceControlPage::showCommandResult);

    m_packageManagerService = new PackageManagerService(m_scrcpyService->adbExecutablePath(), this);
    connect(m_packageManagerPage,
            &PackageManagerPage::packageListRefreshRequested,
            this,
            [this](bool enabledOnly, bool disabledOnly, bool thirdPartyOnly, bool systemOnly) {
                PackageManagerService::PackageFilters filters;
                filters.enabledOnly = enabledOnly;
                filters.disabledOnly = disabledOnly;
                filters.thirdPartyOnly = thirdPartyOnly;
                filters.systemOnly = systemOnly;
                m_packageManagerService->loadPackages(filters);
            });
    connect(m_packageManagerPage,
            &PackageManagerPage::categoryRequested,
            m_packageManagerService,
            &PackageManagerService::loadCategory);
    connect(m_packageManagerPage,
            &PackageManagerPage::userRemoveRequested,
            m_packageManagerService,
            &PackageManagerService::removeUser);
    connect(m_packageManagerPage,
            &PackageManagerPage::packageDetailsRequested,
            m_packageManagerService,
            &PackageManagerService::loadPackageDetails);
    connect(m_packageManagerPage,
            &PackageManagerPage::uninstallRequested,
            m_packageManagerService,
            &PackageManagerService::uninstallPackage);
    connect(m_packageManagerPage,
            &PackageManagerPage::clearDataRequested,
            m_packageManagerService,
            &PackageManagerService::clearPackageData);
    connect(m_packageManagerPage,
            &PackageManagerPage::enableRequested,
            m_packageManagerService,
            &PackageManagerService::enablePackage);
    connect(m_packageManagerPage,
            &PackageManagerPage::disableRequested,
            m_packageManagerService,
            &PackageManagerService::disablePackage);
    connect(m_packageManagerService,
            &PackageManagerService::packagesLoaded,
            m_packageManagerPage,
            &PackageManagerPage::setPackages);
    connect(m_packageManagerService,
            &PackageManagerService::packageDetailsLoaded,
            m_packageManagerPage,
            &PackageManagerPage::setPackageDetails);
    connect(m_packageManagerService,
            &PackageManagerService::categoryLoaded,
            m_packageManagerPage,
            &PackageManagerPage::setCategoryResults);
    connect(m_packageManagerService,
            &PackageManagerService::busyChanged,
            m_packageManagerPage,
            &PackageManagerPage::setBusy);
    connect(m_packageManagerService,
            &PackageManagerService::commandStarted,
            m_packageManagerPage,
            &PackageManagerPage::showCommandStarted);
    connect(m_packageManagerService,
            &PackageManagerService::commandFinished,
            m_packageManagerPage,
            &PackageManagerPage::showCommandResult);
    connect(m_packageManagerService,
            &PackageManagerService::packageActionCompleted,
            m_packageManagerPage,
            &PackageManagerPage::refreshPackages);
    connect(m_packageManagerService,
            &PackageManagerService::categoryActionCompleted,
            m_packageManagerService,
            &PackageManagerService::loadCategory);

    m_appsService = new AppsService(m_scrcpyService->adbExecutablePath(), this);
    connect(m_appsPage,
            &AppsPage::appsRefreshRequested,
            m_appsService,
            &AppsService::loadApps);
    connect(m_appsPage,
            &AppsPage::appDetailsRequested,
            m_appsService,
            &AppsService::loadAppDetails);
    connect(m_appsPage,
            &AppsPage::installRequested,
            m_appsService,
            &AppsService::installPackages);
    connect(m_appsPage,
            &AppsPage::launchRequested,
            m_appsService,
            &AppsService::launchApp);
    connect(m_appsPage,
            &AppsPage::stopRequested,
            m_appsService,
            &AppsService::stopApp);
    connect(m_appsPage,
            &AppsPage::enabledRequested,
            m_appsService,
            &AppsService::setAppEnabled);
    connect(m_appsPage,
            &AppsPage::appInfoRequested,
            m_appsService,
            &AppsService::openAppInfo);
    connect(m_appsPage,
            &AppsPage::clearDataRequested,
            m_appsService,
            &AppsService::clearAppData);
    connect(m_appsPage,
            &AppsPage::uninstallRequested,
            m_appsService,
            &AppsService::uninstallApp);
    connect(m_appsPage,
            &AppsPage::reinstallRequested,
            m_appsService,
            &AppsService::reinstallApp);
    connect(m_appsPage,
            &AppsPage::backgroundModeRequested,
            m_appsService,
            &AppsService::setBackgroundMode);
    connect(m_appsPage,
            &AppsPage::permissionRequested,
            m_appsService,
            &AppsService::setPermission);
    connect(m_appsPage,
            &AppsPage::exportRequested,
            m_appsService,
            &AppsService::exportApk);
    connect(m_appsService,
            &AppsService::busyChanged,
            m_appsPage,
            &AppsPage::setBusy);
    connect(m_appsService,
            &AppsService::appsLoaded,
            m_appsPage,
            &AppsPage::setApps);
    connect(m_appsService,
            &AppsService::appDetailsLoaded,
            m_appsPage,
            &AppsPage::setAppDetails);
    connect(m_appsService,
            &AppsService::operationStarted,
            m_appsPage,
            &AppsPage::showOperationStarted);
    connect(m_appsService,
            &AppsService::operationFinished,
            m_appsPage,
            &AppsPage::showOperationFinished);
    connect(m_appsService,
            &AppsService::appStateChanged,
            m_appsPage,
            &AppsPage::handleAppStateChanged);

    m_fileManagerService = new FileManagerService(m_scrcpyService->adbExecutablePath(), this);
    connect(m_filesPage,
            &FilesPage::directoryRequested,
            m_fileManagerService,
            &FileManagerService::listDirectory);
    connect(m_filesPage,
            &FilesPage::createFolderRequested,
            m_fileManagerService,
            &FileManagerService::createFolder);
    connect(m_filesPage,
            &FilesPage::uploadRequested,
            m_fileManagerService,
            &FileManagerService::uploadFiles);
    connect(m_filesPage,
            &FilesPage::downloadRequested,
            m_fileManagerService,
            &FileManagerService::downloadFiles);
    connect(m_filesPage,
            &FilesPage::renameRequested,
            m_fileManagerService,
            &FileManagerService::renamePath);
    connect(m_filesPage,
            &FilesPage::duplicateRequested,
            m_fileManagerService,
            &FileManagerService::duplicatePath);
    connect(m_filesPage,
            &FilesPage::permissionsRequested,
            m_fileManagerService,
            &FileManagerService::changePermissions);
    connect(m_filesPage,
            &FilesPage::deleteRequested,
            m_fileManagerService,
            &FileManagerService::deletePaths);
    connect(m_fileManagerService,
            &FileManagerService::busyChanged,
            m_filesPage,
            &FilesPage::setBusy);
    connect(m_fileManagerService,
            &FileManagerService::directoryLoaded,
            m_filesPage,
            &FilesPage::setDirectory);
    connect(m_fileManagerService,
            &FileManagerService::operationStarted,
            m_filesPage,
            &FilesPage::showOperationStarted);
    connect(m_fileManagerService,
            &FileManagerService::operationFinished,
            m_filesPage,
            &FilesPage::showOperationFinished);
    connect(m_fileManagerService,
            &FileManagerService::refreshRequested,
            m_filesPage,
            &FilesPage::refresh);

    m_recoveryService = new RecoveryService(m_scrcpyService->adbExecutablePath(), this);
    connect(m_recoveryPage,
            &RecoveryPage::sideloadRequested,
            m_recoveryService,
            &RecoveryService::startSideload);
    connect(m_recoveryPage,
            &RecoveryPage::cancelRequested,
            m_recoveryService,
            &RecoveryService::cancelSideload);
    connect(m_recoveryService,
            &RecoveryService::busyChanged,
            m_recoveryPage,
            &RecoveryPage::setBusy);
    connect(m_recoveryService,
            &RecoveryService::sideloadStarted,
            m_recoveryPage,
            &RecoveryPage::showSideloadStarted);
    connect(m_recoveryService,
            &RecoveryService::outputChanged,
            m_recoveryPage,
            &RecoveryPage::setOutput);
    connect(m_recoveryService,
            &RecoveryService::progressChanged,
            m_recoveryPage,
            &RecoveryPage::setProgress);
    connect(m_recoveryService,
            &RecoveryService::sideloadFinished,
            m_recoveryPage,
            &RecoveryPage::showSideloadFinished);

    m_performanceService = new PerformanceService(m_scrcpyService->adbExecutablePath(), this);
    connect(m_performanceService,
            &PerformanceService::sampleReady,
            m_performancePage,
            &PerformancePage::applySample);
    connect(m_performanceService,
            &PerformanceService::samplingError,
            m_performancePage,
            &PerformancePage::showSamplingError);

    m_adbControlService->setDeviceSerial(m_deviceSerial);
    m_packageManagerService->setDeviceSerial(m_deviceSerial);
    m_appsService->setDeviceSerial(
        m_deviceState == ScrcpyService::DeviceState::Connected ? m_deviceSerial : QString());
    m_fileManagerService->setDeviceSerial(
        m_deviceState == ScrcpyService::DeviceState::Connected ? m_deviceSerial : QString());
    const bool initialSideload = m_deviceState == ScrcpyService::DeviceState::Sideload;
    const bool initialDeviceAvailable = initialSideload
        || m_deviceState == ScrcpyService::DeviceState::Connected;
    m_recoveryService->setSideloadDeviceSerial(initialSideload ? m_deviceSerial : QString());
    m_performanceService->setDeviceSerial(
        m_deviceState == ScrcpyService::DeviceState::Connected ? m_deviceSerial : QString());
    m_deviceControlPage->setDeviceConnected(
        m_deviceState == ScrcpyService::DeviceState::Connected,
        m_deviceSerial);
    m_packageManagerPage->setDeviceConnected(
        m_deviceState == ScrcpyService::DeviceState::Connected,
        m_deviceSerial);
    m_appsPage->setDeviceConnected(
        m_deviceState == ScrcpyService::DeviceState::Connected,
        m_deviceSerial);
    m_filesPage->setDeviceConnected(
        m_deviceState == ScrcpyService::DeviceState::Connected,
        m_deviceSerial);
    m_recoveryPage->setDeviceState(initialDeviceAvailable, initialSideload, m_deviceSerial);
    m_performancePage->setDeviceConnected(
        m_deviceState == ScrcpyService::DeviceState::Connected,
        m_deviceSerial);
    selectWorkspace(0);
}

void MainWindow::selectWorkspace(int index)
{
    m_workspaceStack->setCurrentIndex(index);
    m_chatNavButton->setProperty("active", index == 0);
    m_deviceControlNavButton->setProperty("active", index == 1);
    m_packageManagerNavButton->setProperty("active", index == 2);
    m_appsNavButton->setProperty("active", index == 3);
    m_filesNavButton->setProperty("active", index == 4);
    m_recoveryNavButton->setProperty("active", index == 5);
    m_performanceNavButton->setProperty("active", index == 6);
    m_chatNavButton->setFont(ui::appFont(11, index == 0 ? QFont::DemiBold : QFont::Normal));
    m_deviceControlNavButton->setFont(
        ui::appFont(11, index == 1 ? QFont::DemiBold : QFont::Normal));
    m_packageManagerNavButton->setFont(
        ui::appFont(11, index == 2 ? QFont::DemiBold : QFont::Normal));
    m_appsNavButton->setFont(
        ui::appFont(11, index == 3 ? QFont::DemiBold : QFont::Normal));
    m_filesNavButton->setFont(
        ui::appFont(11, index == 4 ? QFont::DemiBold : QFont::Normal));
    m_recoveryNavButton->setFont(
        ui::appFont(11, index == 5 ? QFont::DemiBold : QFont::Normal));
    m_performanceNavButton->setFont(
        ui::appFont(11, index == 6 ? QFont::DemiBold : QFont::Normal));
    for (QPushButton *button : {m_chatNavButton,
                                m_deviceControlNavButton,
                                m_packageManagerNavButton,
                                m_appsNavButton,
                                m_filesNavButton,
                                m_recoveryNavButton,
                                m_performanceNavButton}) {
        button->style()->unpolish(button);
        button->style()->polish(button);
    }
    if (index == 2) {
        m_packageManagerPage->showOverview();
    }
    if (index == 3) {
        m_appsPage->activate();
    }
    if (index == 4) {
        m_filesPage->activate();
    }
    if (index == 5) {
        m_recoveryPage->showOverview();
    }
    if (m_performanceService != nullptr) {
        m_performanceService->setActive(index == 6);
    }
}

void MainWindow::updateDeviceUi(ScrcpyService::DeviceState state,
                                const QString &serial,
                                const QString &detail)
{
    m_deviceState = state;
    m_deviceSerial = serial;
    m_deviceDetail = detail;

    const bool connected = state == ScrcpyService::DeviceState::Connected;
    if (m_adbControlService != nullptr) {
        m_adbControlService->setDeviceSerial(connected ? serial : QString());
        m_deviceControlPage->setDeviceConnected(connected, serial);
    }
    if (m_packageManagerService != nullptr) {
        m_packageManagerService->setDeviceSerial(connected ? serial : QString());
        m_packageManagerPage->setDeviceConnected(connected, serial);
    }
    if (m_appsService != nullptr) {
        m_appsService->setDeviceSerial(connected ? serial : QString());
        m_appsPage->setDeviceConnected(connected, serial);
    }
    if (m_fileManagerService != nullptr) {
        m_fileManagerService->setDeviceSerial(connected ? serial : QString());
        m_filesPage->setDeviceConnected(connected, serial);
    }
    if (m_recoveryService != nullptr) {
        const bool sideloadMode = state == ScrcpyService::DeviceState::Sideload;
        const bool recoveryDeviceAvailable = connected || sideloadMode;
        m_recoveryService->setSideloadDeviceSerial(sideloadMode ? serial : QString());
        m_recoveryPage->setDeviceState(recoveryDeviceAvailable, sideloadMode, serial);
    }
    if (m_performanceService != nullptr) {
        m_performanceService->setDeviceSerial(connected ? serial : QString());
        m_performancePage->setDeviceConnected(connected, serial);
    }

    QString deviceName;
    QString statusText;
    QString statusColor;
    QString sidebarTitle;
    QString sidebarDetail = detail;

    switch (state) {
    case ScrcpyService::DeviceState::ToolUnavailable:
        deviceName = ui::text("scrcpy 不可用");
        statusText = ui::text("不可用");
        statusColor = QStringLiteral("#d45b5b");
        sidebarTitle = ui::text("ADB 不可用");
        break;
    case ScrcpyService::DeviceState::Disconnected:
        deviceName = ui::text("未检测到设备");
        statusText = ui::text("未连接");
        statusColor = QStringLiteral("#aab3c2");
        sidebarTitle = ui::text("ADB 未连接");
        break;
    case ScrcpyService::DeviceState::Unauthorized:
        deviceName = serial.isEmpty() ? ui::text("Android 设备") : serial;
        statusText = ui::text("未授权");
        statusColor = QStringLiteral("#e2a43a");
        sidebarTitle = ui::text("ADB 等待授权");
        sidebarDetail = serial;
        break;
    case ScrcpyService::DeviceState::Sideload:
        deviceName = serial.isEmpty() ? ui::text("Recovery 设备") : serial;
        statusText = ui::text("侧载模式");
        statusColor = QStringLiteral("#e2a43a");
        sidebarTitle = ui::text("Recovery 侧载已就绪");
        sidebarDetail = serial;
        break;
    case ScrcpyService::DeviceState::Connected:
        deviceName = serial;
        statusText = ui::text("已连接");
        statusColor = QStringLiteral("#66c95e");
        sidebarTitle = ui::text("ADB 连接正常");
        sidebarDetail = ui::text("设备 %1").arg(serial);
        break;
    }

    m_deviceNameLabel->setText(deviceName);
    m_deviceStatusLabel->setText(statusText);
    m_deviceStatusDot->setStyleSheet(QStringLiteral("color:%1;").arg(statusColor));
    m_sidebarStatusDot->setStyleSheet(QStringLiteral("color:%1;").arg(statusColor));
    m_sidebarStatusTitle->setText(sidebarTitle);
    m_sidebarStatusDetail->setText(sidebarDetail);
    m_mirrorButton->setEnabled(m_scrcpyService->mirrorRunning()
                               || state == ScrcpyService::DeviceState::Connected);
}

void MainWindow::updateMirrorUi(bool running)
{
    m_mirrorButton->setProperty("running", running);
    m_mirrorButton->setText(running ? ui::text("■  停止镜像") : ui::text("▶  启动镜像"));
    m_mirrorButton->style()->unpolish(m_mirrorButton);
    m_mirrorButton->style()->polish(m_mirrorButton);

    if (running) {
        m_mirrorButton->setEnabled(true);
        return;
    }

    updateDeviceUi(m_deviceState, m_deviceSerial, m_deviceDetail);
}
