#include "ui/windows/main_window.h"

#include "services/adb_control_service.h"
#include "services/apps_service.h"
#include "services/device_center_service.h"
#include "services/display_service.h"
#include "services/file_manager_service.h"
#include "services/logcat_service.h"
#include "services/other_service.h"
#include "services/overview_service.h"
#include "services/package_manager_service.h"
#include "services/performance_service.h"
#include "services/process_service.h"
#include "services/recovery_service.h"
#include "services/terminal_service.h"
#include "ui/common/widget_helpers.h"
#include "ui/common/app_preferences.h"
#include "ui/components/main_window_sections.h"
#include "ui/pages/device_control_page.h"
#include "ui/pages/display_page.h"
#include "ui/pages/apps_page.h"
#include "ui/pages/files_page.h"
#include "ui/pages/package_manager_page.h"
#include "ui/pages/performance_page.h"
#include "ui/pages/recovery_page.h"
#include "ui/pages/settings_page.h"
#include "ui/pages/layout_page.h"
#include "ui/pages/logcat_page.h"
#include "ui/pages/mirroring_page.h"
#include "ui/pages/other_page.h"
#include "ui/pages/overview_page.h"
#include "ui/pages/process_page.h"
#include "ui/pages/terminal_page.h"
#include "ui/styles/app_style.h"
#include "ui/windows/device_center_window.h"

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>

namespace {

const QString kDefaultScrcpyPath = QStringLiteral(
    "D:/myApp_666666666666666/scrcpy-win64-v4.0/scrcpy.exe");

QString openCodeExecutablePath()
{
    QSettings settings(QStringLiteral("AI Mobile Test Studio"),
                       QStringLiteral("AI Mobile Test Studio"));
    QString configured = qEnvironmentVariable("AI_MOBILE_TEST_OPENCODE_PATH");
    if (configured.isEmpty()) {
        configured = settings.value(QStringLiteral("runtime/opencodePath")).toString();
    }
    if (!configured.isEmpty()) {
        return QFileInfo(configured).absoluteFilePath();
    }

    const QDir runtimeDirectory(
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("runtime/opencode")));
    const QStringList candidates = {
        runtimeDirectory.filePath(QStringLiteral("opencode.exe")),
        runtimeDirectory.filePath(QStringLiteral("opencode.cmd")),
        runtimeDirectory.filePath(QStringLiteral("bin/opencode.exe"))};
    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
    return candidates.first();
}

QString openCodeWorkingDirectory()
{
    QSettings settings(QStringLiteral("AI Mobile Test Studio"),
                       QStringLiteral("AI Mobile Test Studio"));
    QString directory = qEnvironmentVariable("AI_MOBILE_TEST_WORKSPACE");
    if (directory.isEmpty()) {
        directory = settings.value(QStringLiteral("workspace/path")).toString();
    }
    if (directory.isEmpty() || !QFileInfo(directory).isDir()) {
        directory = QDir::currentPath();
    }
    return QDir(directory).absolutePath();
}

QString nodeExecutablePath()
{
    QSettings settings(QStringLiteral("AI Mobile Test Studio"),
                       QStringLiteral("AI Mobile Test Studio"));
    QString configured = qEnvironmentVariable("AI_MOBILE_TEST_NODE_PATH");
    if (configured.isEmpty()) {
        configured = settings.value(QStringLiteral("runtime/nodePath")).toString();
    }
    if (!configured.isEmpty()) {
        return QFileInfo(configured).absoluteFilePath();
    }
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("runtime/node/node.exe"));
}

QString nodePtyModulePath()
{
    QSettings settings(QStringLiteral("AI Mobile Test Studio"),
                       QStringLiteral("AI Mobile Test Studio"));
    QString configured = qEnvironmentVariable("AI_MOBILE_TEST_NODE_PTY_PATH");
    if (configured.isEmpty()) {
        configured = settings.value(QStringLiteral("runtime/nodePtyPath")).toString();
    }
    if (!configured.isEmpty()) {
        return QFileInfo(configured).absoluteFilePath();
    }
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("runtime/node/node_modules/node-pty"));
}

QString terminalHostScriptPath()
{
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("runtime/terminal-host/conpty_host.js"));
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    connect(&ui::AppPreferences::instance(),
            &ui::AppPreferences::languageChanged,
            this,
            [this] { applyLanguage(); });
    configureScrcpy();
    configureDeviceControls();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi()
{
    setWindowTitle(ui::text("AI Mobile Test Studio"));
    setMinimumSize(1080, 700);
    resize(1600, 900);
    qApp->setStyleSheet(ui::appStyleSheet());

    auto *root = ui::makePanel("Root");
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    const ui::HeaderSection header = ui::createHeader();
    m_deviceSelectorButton = header.deviceSelectorButton;
    m_headerDeviceCenterButton = header.deviceCenterButton;
    m_headerSettingsButton = header.settingsButton;
    m_deviceNameLabel = header.deviceNameLabel;
    m_deviceStatusDot = header.deviceStatusDot;
    m_deviceStatusLabel = header.deviceStatusLabel;
    rootLayout->addWidget(header.widget);

    auto *content = new QWidget;
    auto *contentLayout = new QHBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    const ui::SidebarSection sidebar = ui::createSidebar();
    m_overviewNavButton = sidebar.overviewButton;
    m_displayNavButton = sidebar.displayButton;
    m_mirroringNavButton = sidebar.mirroringButton;
    m_chatNavButton = sidebar.chatButton;
    m_deviceControlNavButton = sidebar.deviceControlButton;
    m_packageManagerNavButton = sidebar.packageManagerButton;
    m_appsNavButton = sidebar.appsButton;
    m_filesNavButton = sidebar.filesButton;
    m_processNavButton = sidebar.processButton;
    m_recoveryNavButton = sidebar.recoveryButton;
    m_performanceNavButton = sidebar.performanceButton;
    m_layoutNavButton = sidebar.layoutButton;
    m_logcatNavButton = sidebar.logcatButton;
    m_otherNavButton = sidebar.otherButton;
    m_settingsNavButton = sidebar.settingsButton;
    m_sidebarWidget = sidebar.widget;
    contentLayout->addWidget(sidebar.widget);

    auto *workspace = new QWidget;
    auto *workspaceLayout = new QVBoxLayout(workspace);
    workspaceLayout->setContentsMargins(0, 0, 0, 0);
    workspaceLayout->setSpacing(0);

    m_workspaceStack = new QStackedWidget;
    m_workspaceStack->setObjectName("WorkspaceStack");
    m_overviewPage = new OverviewPage;
    m_workspaceStack->addWidget(m_overviewPage);
    m_displayPage = new DisplayPage;
    m_workspaceStack->addWidget(m_displayPage);
    m_mirroringPage = new MirroringPage;
    m_workspaceStack->addWidget(m_mirroringPage);
    m_terminalPage = new TerminalPage;
    m_workspaceStack->addWidget(m_terminalPage);
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
    m_layoutPage = new LayoutPage;
    m_workspaceStack->addWidget(m_layoutPage);
    m_logcatPage = new LogcatPage;
    m_workspaceStack->addWidget(m_logcatPage);
    m_otherPage = new OtherPage;
    m_workspaceStack->addWidget(m_otherPage);
    m_processPage = new ProcessPage;
    m_workspaceStack->addWidget(m_processPage);
    m_settingsPage = new SettingsPage;
    m_workspaceStack->addWidget(m_settingsPage);
    workspaceLayout->addWidget(m_workspaceStack, 1);

    contentLayout->addWidget(workspace, 1);
    rootLayout->addWidget(content, 1);
    setCentralWidget(root);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (m_sidebarWidget != nullptr) {
        m_sidebarWidget->setFixedWidth(width() < 1280 ? 220 : 252);
    }
    if (m_workspacePreloadCover != nullptr) {
        m_workspacePreloadCover->setGeometry(m_workspaceStack->rect());
    }
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
    m_deviceSelectorMenu = new QMenu(this);

    connect(m_scrcpyService,
            &ScrcpyService::deviceStateChanged,
            this,
            &MainWindow::updateDeviceUi);
    connect(m_scrcpyService,
            &ScrcpyService::connectedDevicesChanged,
            this,
            [this](const QStringList &serials) {
                m_connectedDeviceSerials = serials;
                rebuildDeviceSelectorMenu();
            });
    connect(m_deviceSelectorButton,
            &QPushButton::clicked,
            this,
            &MainWindow::showDeviceSelectorMenu);
    connect(m_scrcpyService,
            &ScrcpyService::mirrorRunningChanged,
            m_mirroringPage,
            &MirroringPage::setMirrorRunning);
    connect(m_mirroringPage,
            &MirroringPage::launchRequested,
            m_scrcpyService,
            &ScrcpyService::startMirror);
    connect(m_mirroringPage,
            &MirroringPage::stopRequested,
            m_scrcpyService,
            &ScrcpyService::stopMirror);
    connect(m_mirroringPage,
            &MirroringPage::cameraListRequested,
            m_scrcpyService,
            &ScrcpyService::queryCameras);
    connect(m_scrcpyService,
            &ScrcpyService::camerasLoaded,
            m_mirroringPage,
            &MirroringPage::setCameras);
    connect(m_scrcpyService, &ScrcpyService::operationError, this, [this](const QString &message) {
        m_mirroringPage->showError(message);
        if (m_workspaceStack->currentWidget() != m_mirroringPage) {
            QMessageBox::warning(this, ui::text("scrcpy 运行错误"), message);
        }
    });

    m_scrcpyService->startMonitoring();
}

void MainWindow::configureDeviceControls()
{
    m_terminalService = new TerminalService(this);
    m_terminalService->setOpenCodeConfiguration(openCodeExecutablePath(),
                                                openCodeWorkingDirectory(),
                                                nodeExecutablePath(),
                                                nodePtyModulePath(),
                                                terminalHostScriptPath());
    QTimer::singleShot(100, this, &MainWindow::preloadWebWorkspaces);
    m_adbControlService = new AdbControlService(m_scrcpyService->adbExecutablePath(), this);
    m_deviceCenterService = new DeviceCenterService(m_scrcpyService->adbExecutablePath(), this);
    m_deviceCenterWindow = new DeviceCenterWindow(m_deviceCenterService, this);
    m_deviceCenterWindow->setActiveDeviceSerial(m_scrcpyService->deviceSerial());

    connect(m_deviceCenterWindow,
            &DeviceCenterWindow::deviceActivationRequested,
            m_scrcpyService,
            &ScrcpyService::setPreferredDeviceSerial);

    connect(m_terminalPage,
            &TerminalPage::sessionCreateRequested,
            m_terminalService,
            &TerminalService::createSession);
    connect(m_terminalPage,
            &TerminalPage::sessionWriteRequested,
            m_terminalService,
            &TerminalService::writeSession);
    connect(m_terminalPage,
            &TerminalPage::sessionResizeRequested,
            m_terminalService,
            &TerminalService::resizeSession);
    connect(m_terminalPage,
            &TerminalPage::sessionRestartRequested,
            m_terminalService,
            &TerminalService::restartSession);
    connect(m_terminalPage,
            &TerminalPage::sessionCloseRequested,
            m_terminalService,
            &TerminalService::closeSession);
    connect(m_terminalService,
            &TerminalService::sessionStarted,
            m_terminalPage,
            &TerminalPage::handleSessionStarted);
    connect(m_terminalService,
            &TerminalService::sessionOutput,
            m_terminalPage,
            &TerminalPage::handleSessionOutput);
    connect(m_terminalService,
            &TerminalService::sessionFailed,
            m_terminalPage,
            &TerminalPage::handleSessionFailed);
    connect(m_terminalService,
            &TerminalService::sessionClosed,
            m_terminalPage,
            &TerminalPage::handleSessionClosed);

    connect(m_overviewNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(0);
    });
    connect(m_displayNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(1);
    });
    connect(m_mirroringNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(2);
    });
    connect(m_chatNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(3);
    });
    connect(m_deviceControlNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(4);
    });
    connect(m_packageManagerNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(5);
    });
    connect(m_appsNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(6);
    });
    connect(m_filesNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(7);
    });
    connect(m_recoveryNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(8);
    });
    connect(m_performanceNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(9);
    });
    connect(m_layoutNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(10);
    });
    connect(m_logcatNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(11);
    });
    connect(m_otherNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(12);
    });
    connect(m_processNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(13);
    });
    connect(m_settingsNavButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(14);
    });
    connect(m_headerSettingsButton, &QPushButton::clicked, this, [this] {
        selectWorkspace(14);
    });
    connect(m_headerDeviceCenterButton,
            &QPushButton::clicked,
            this,
            &MainWindow::showDeviceCenter);
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
            &PackageManagerPage::installRequested,
            m_packageManagerService,
            &PackageManagerService::installPackage);
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
            &AppsService::appMetadataLoaded,
            m_processPage,
            &ProcessPage::setApplications);
    connect(m_processPage,
            &ProcessPage::applicationMetadataRequested,
            m_appsService,
            &AppsService::loadAppMetadata);
    connect(m_appsService,
            &AppsService::appsLoaded,
            m_mirroringPage,
            &MirroringPage::setApplications);
    connect(m_mirroringPage,
            &MirroringPage::applicationListRequested,
            m_appsService,
            &AppsService::loadApps);
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
            &FilesPage::directoryRefreshRequested,
            m_fileManagerService,
            &FileManagerService::refreshDirectory);
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

    m_displayService = new DisplayService(m_scrcpyService->adbExecutablePath(), this);
    connect(m_displayPage,
            &DisplayPage::refreshRequested,
            m_displayService,
            &DisplayService::refresh);
    connect(m_displayPage,
            &DisplayPage::applyRequested,
            m_displayService,
            &DisplayService::applyDimensions);
    connect(m_displayPage,
            &DisplayPage::resetRequested,
            m_displayService,
            &DisplayService::resetDimensions);
    connect(m_displayPage,
            &DisplayPage::refreshRateRequested,
            m_displayService,
            &DisplayService::setRefreshRate);
    connect(m_displayPage,
            &DisplayPage::darkModeRequested,
            m_displayService,
            &DisplayService::setDarkMode);
    connect(m_displayPage,
            &DisplayPage::fontScaleRequested,
            m_displayService,
            &DisplayService::setFontScale);
    connect(m_displayPage,
            &DisplayPage::animationScaleRequested,
            m_displayService,
            &DisplayService::setAnimationScale);
    connect(m_displayService,
            &DisplayService::busyChanged,
            m_displayPage,
            &DisplayPage::setBusy);
    connect(m_displayService,
            &DisplayService::settingsLoaded,
            m_displayPage,
            &DisplayPage::setSettings);
    connect(m_displayService,
            &DisplayService::settingsError,
            m_displayPage,
            &DisplayPage::showError);
    connect(m_displayService,
            &DisplayService::operationFinished,
            m_displayPage,
            &DisplayPage::showOperationResult);

    m_overviewService = new OverviewService(m_scrcpyService->adbExecutablePath(), this);
    connect(m_overviewPage,
            &OverviewPage::refreshRequested,
            m_overviewService,
            &OverviewService::refresh);
    connect(m_overviewPage,
            &OverviewPage::refreshRequested,
            m_overviewService,
            &OverviewService::captureScreenshot);
    connect(m_overviewPage,
            &OverviewPage::screenshotRequested,
            m_overviewService,
            &OverviewService::captureScreenshot);
    connect(m_overviewPage,
            &OverviewPage::shizukuRequested,
            m_overviewService,
            &OverviewService::startShizuku);
    connect(m_overviewPage,
            &OverviewPage::powerRequested,
            m_overviewService,
            &OverviewService::togglePower);
    connect(m_overviewService,
            &OverviewService::loadingChanged,
            m_overviewPage,
            &OverviewPage::setLoading);
    connect(m_overviewService,
            &OverviewService::overviewReady,
            m_overviewPage,
            &OverviewPage::setOverview);
    connect(m_overviewService,
            &OverviewService::overviewReady,
            m_filesPage,
            &FilesPage::setDeviceOverview);
    connect(m_filesPage,
            &FilesPage::deviceInfoRequested,
            m_overviewService,
            &OverviewService::refresh);
    connect(m_overviewService,
            &OverviewService::screenshotLoadingChanged,
            m_overviewPage,
            &OverviewPage::setScreenshotLoading);
    connect(m_overviewService,
            &OverviewService::screenshotReady,
            m_overviewPage,
            &OverviewPage::setScreenshot);
    connect(m_overviewService,
            &OverviewService::actionFinished,
            m_overviewPage,
            &OverviewPage::showActionResult);
    connect(m_overviewService,
            &OverviewService::overviewError,
            m_overviewPage,
            &OverviewPage::showError);

    m_logcatService = new LogcatService(m_scrcpyService->adbExecutablePath(), this);
    connect(m_logcatService,
            &LogcatService::entryReady,
            m_logcatPage,
            &LogcatPage::appendEntry);
    connect(m_logcatService,
            &LogcatService::streamStateChanged,
            m_logcatPage,
            &LogcatPage::setStreamRunning);
    connect(m_logcatService,
            &LogcatService::streamError,
            m_logcatPage,
            &LogcatPage::showStreamError);
    connect(m_logcatPage,
            &LogcatPage::restartRequested,
            m_logcatService,
            &LogcatService::restart);

    m_otherService = new OtherService(m_scrcpyService->adbExecutablePath(), this);
    connect(m_otherPage,
            &OtherPage::shellCommandRequested,
            m_otherService,
            &OtherService::runShellCommand);
    connect(m_otherService,
            &OtherService::busyChanged,
            m_otherPage,
            &OtherPage::setBusy);
    connect(m_otherService,
            &OtherService::commandStarted,
            m_otherPage,
            &OtherPage::showCommandStarted);
    connect(m_otherService,
            &OtherService::commandFinished,
            m_otherPage,
            &OtherPage::showCommandResult);

    m_processService = new ProcessService(m_scrcpyService->adbExecutablePath(), this);
    connect(m_processPage,
            &ProcessPage::refreshRequested,
            m_processService,
            &ProcessService::refresh);
    connect(m_processPage,
            &ProcessPage::stopPackageRequested,
            m_processService,
            &ProcessService::stopPackage);
    connect(m_processService,
            &ProcessService::samplingChanged,
            m_processPage,
            &ProcessPage::setSampling);
    connect(m_processService,
            &ProcessService::processesReady,
            m_processPage,
            &ProcessPage::setProcesses);
    connect(m_processService,
            &ProcessService::processError,
            m_processPage,
            &ProcessPage::showError);
    connect(m_processService,
            &ProcessService::stopFinished,
            m_processPage,
            &ProcessPage::showStopResult);

    const bool initialConnected = m_deviceState == ScrcpyService::DeviceState::Connected;
    m_terminalService->setDeviceSerial(initialConnected ? m_deviceSerial : QString());
    m_terminalPage->setDeviceConnected(initialConnected, m_deviceSerial);
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
    m_overviewService->setDeviceSerial(initialConnected ? m_deviceSerial : QString());
    m_displayService->setDeviceSerial(initialConnected ? m_deviceSerial : QString());
    m_logcatService->setDeviceSerial(initialConnected ? m_deviceSerial : QString());
    m_otherService->setDeviceSerial(initialConnected ? m_deviceSerial : QString());
    m_processService->setDeviceSerial(initialConnected ? m_deviceSerial : QString());
    m_overviewPage->setDeviceConnected(initialConnected, m_deviceSerial);
    m_displayPage->setDeviceConnected(initialConnected, m_deviceSerial);
    m_mirroringPage->setDeviceConnected(initialConnected, m_deviceSerial);
    m_mirroringPage->setMirrorRunning(m_scrcpyService->mirrorRunning());
    m_logcatPage->setDeviceConnected(initialConnected, m_deviceSerial);
    m_otherPage->setDeviceConnected(initialConnected, m_deviceSerial);
    m_processPage->setDeviceConnected(initialConnected, m_deviceSerial);
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
    m_layoutPage->setDeviceConnected(
        m_deviceState == ScrcpyService::DeviceState::Connected,
        m_deviceSerial);
    if (initialConnected) {
        preloadDeviceData(m_deviceSerial);
    }
    selectWorkspace(0);
}

void MainWindow::showDeviceCenter()
{
    if (m_deviceCenterWindow == nullptr) {
        return;
    }
    m_deviceCenterWindow->setActiveDeviceSerial(m_deviceSerial);
    m_deviceCenterWindow->showNormal();
    m_deviceCenterWindow->show();
    m_deviceCenterWindow->raise();
    m_deviceCenterWindow->activateWindow();
}

void MainWindow::showDeviceSelectorMenu()
{
    if (m_scrcpyService == nullptr || m_deviceSelectorButton == nullptr
        || m_deviceSelectorMenu == nullptr) {
        return;
    }

    m_scrcpyService->refreshDeviceState();
    rebuildDeviceSelectorMenu();
    const QPoint menuPosition = m_deviceSelectorButton->mapToGlobal(
        QPoint(0, m_deviceSelectorButton->height() + 4));
    m_deviceSelectorMenu->popup(menuPosition);
}

void MainWindow::rebuildDeviceSelectorMenu()
{
    if (m_deviceSelectorMenu == nullptr) {
        return;
    }

    m_deviceSelectorMenu->clear();
    if (m_deviceSelectorButton != nullptr) {
        m_deviceSelectorMenu->setMinimumWidth(m_deviceSelectorButton->width());
    }

    if (m_connectedDeviceSerials.isEmpty()) {
        QAction *emptyAction = m_deviceSelectorMenu->addAction(
            ui::text("未检测到已连接设备"));
        emptyAction->setEnabled(false);
        return;
    }

    for (const QString &serial : m_connectedDeviceSerials) {
        QAction *deviceAction = m_deviceSelectorMenu->addAction(serial);
        deviceAction->setCheckable(true);
        deviceAction->setChecked(serial == m_deviceSerial);
        connect(deviceAction, &QAction::triggered, this, [this, serial] {
            m_scrcpyService->setPreferredDeviceSerial(serial);
        });
    }
}

void MainWindow::preloadDeviceData(const QString &serial)
{
    QTimer::singleShot(0, this, [this, serial] {
        if (m_deviceState != ScrcpyService::DeviceState::Connected
            || m_deviceSerial != serial) {
            return;
        }
        m_appsService->preloadApps();
        m_processService->preload();
        m_fileManagerService->preloadDirectories(
            {QStringLiteral("/"), QStringLiteral("/sdcard")});
    });
}

void MainWindow::preloadWebWorkspaces()
{
    if (m_workspacePreloading || m_workspaceStack == nullptr
        || m_layoutPage == nullptr || m_terminalPage == nullptr
        || !m_workspaceStack->isVisible() || m_workspaceStack->size().isEmpty()) {
        return;
    }
    if (m_selectedWorkspaceIndex != 0) {
        return;
    }

    m_terminalPage->preloadOpenCode();
    m_workspacePreloading = true;
    const quint64 generation = ++m_workspacePreloadGeneration;

    auto *cover = new QLabel(m_workspaceStack);
    cover->setObjectName(QStringLiteral("WorkspacePreloadCover"));
    cover->setPixmap(m_workspaceStack->grab());
    cover->setScaledContents(true);
    cover->setGeometry(m_workspaceStack->rect());
    cover->show();
    cover->raise();
    m_workspacePreloadCover = cover;

    m_workspaceStack->setCurrentWidget(m_layoutPage);
    cover->raise();

    const auto preloadTerminal = [this, generation] {
        if (!m_workspacePreloading || generation != m_workspacePreloadGeneration) {
            return;
        }
        if (m_workspaceStack->currentWidget() != m_layoutPage) {
            return;
        }
        m_layoutPage->finishPreload();
        m_workspaceStack->setCurrentWidget(m_terminalPage);
        if (m_workspacePreloadCover != nullptr) {
            m_workspacePreloadCover->raise();
        }

        QTimer::singleShot(0, this, [this, generation] {
            if (!m_workspacePreloading || generation != m_workspacePreloadGeneration) {
                return;
            }
            m_workspaceStack->setCurrentIndex(m_selectedWorkspaceIndex);
            m_terminalPage->finishBackgroundPreload();
            m_workspacePreloading = false;
            if (m_workspacePreloadCover != nullptr) {
                m_workspacePreloadCover->hide();
                m_workspacePreloadCover->deleteLater();
                m_workspacePreloadCover = nullptr;
            }
        });
    };
    connect(m_layoutPage,
            &LayoutPage::preloadReady,
            this,
            preloadTerminal,
            Qt::SingleShotConnection);
    m_layoutPage->preload();
    QTimer::singleShot(8000, this, preloadTerminal);
}

void MainWindow::cancelWorkspacePreload()
{
    if (!m_workspacePreloading) {
        return;
    }

    ++m_workspacePreloadGeneration;
    m_workspacePreloading = false;
    m_layoutPage->finishPreload();
    m_terminalPage->finishBackgroundPreload();
    if (m_workspacePreloadCover != nullptr) {
        m_workspacePreloadCover->hide();
        m_workspacePreloadCover->deleteLater();
        m_workspacePreloadCover = nullptr;
    }
}

void MainWindow::selectWorkspace(int index)
{
    if (m_workspaceStack == nullptr || index < 0 || index >= m_workspaceStack->count()) {
        return;
    }
    m_selectedWorkspaceIndex = index;
    cancelWorkspacePreload();
    animateWorkspaceTransition(index);
    const QList<QPushButton *> workspaceButtons = {m_overviewNavButton,
                                                   m_displayNavButton,
                                                   m_mirroringNavButton,
                                                   m_chatNavButton,
                                                   m_deviceControlNavButton,
                                                   m_packageManagerNavButton,
                                                   m_appsNavButton,
                                                   m_filesNavButton,
                                                   m_recoveryNavButton,
                                                   m_performanceNavButton,
                                                   m_layoutNavButton,
                                                   m_logcatNavButton,
                                                   m_otherNavButton,
                                                   m_processNavButton,
                                                   m_settingsNavButton};
    for (int buttonIndex = 0; buttonIndex < workspaceButtons.size(); ++buttonIndex) {
        QPushButton *button = workspaceButtons[buttonIndex];
        const bool active = buttonIndex == index;
        if (button->property("active").toBool() == active) {
            continue;
        }
        button->setProperty("active", active);
        button->setFont(ui::appFont(11, active ? QFont::DemiBold : QFont::Normal));
        button->style()->unpolish(button);
        button->style()->polish(button);
        button->update();
    }
    const bool settingsActive = index == 14;
    if (m_headerSettingsButton->property("active").toBool() != settingsActive) {
        m_headerSettingsButton->setProperty("active", settingsActive);
        m_headerSettingsButton->style()->unpolish(m_headerSettingsButton);
        m_headerSettingsButton->style()->polish(m_headerSettingsButton);
        m_headerSettingsButton->update();
    }
    if (index == 0 && m_overviewPage != nullptr) {
        m_overviewPage->activate();
    }
    if (index == 1 && m_displayPage != nullptr) {
        m_displayPage->activate();
    }
    if (index == 2 && m_mirroringPage != nullptr) {
        m_mirroringPage->activate();
    }
    if (index == 5) {
        m_packageManagerPage->showOverview();
    }
    if (index == 6) {
        m_appsPage->activate();
    }
    if (index == 7) {
        m_filesPage->activate();
    }
    if (index == 8) {
        m_recoveryPage->showOverview();
    }
    if (m_performanceService != nullptr) {
        m_performanceService->setActive(index == 9);
    }
    if (m_logcatService != nullptr) {
        m_logcatService->setActive(index == 11);
    }
    if (m_processService != nullptr) {
        m_processService->setActive(index == 13);
    }
    if (index == 3 && m_terminalPage != nullptr) {
        m_terminalPage->activate();
    }
    if (index == 14 && m_settingsPage != nullptr) {
        m_settingsPage->refreshPreferences();
    }
}

void MainWindow::animateWorkspaceTransition(int index)
{
    if (m_workspaceStack == nullptr || index < 0 || index >= m_workspaceStack->count()) {
        return;
    }
    if (m_workspaceStack->currentIndex() != index) {
        m_workspaceStack->setCurrentIndex(index);
    }
}

void MainWindow::applyLanguage()
{
    ui::AppPreferences::instance().retranslate(this);
    setWindowTitle(ui::text("AI Mobile Test Studio"));
    qApp->setStyleSheet(ui::appStyleSheet());
    if (m_settingsPage != nullptr) {
        m_settingsPage->refreshPreferences();
    }

}

void MainWindow::updateDeviceUi(ScrcpyService::DeviceState state,
                                const QString &serial,
                                const QString &detail)
{
    const bool newlyConnected = state == ScrcpyService::DeviceState::Connected
        && (m_deviceState != ScrcpyService::DeviceState::Connected
            || m_deviceSerial != serial);
    m_deviceState = state;
    m_deviceSerial = serial;
    m_deviceDetail = detail;
    if (m_deviceCenterWindow != nullptr) {
        m_deviceCenterWindow->setActiveDeviceSerial(serial);
    }

    const bool connected = state == ScrcpyService::DeviceState::Connected;
    if (m_terminalService != nullptr) {
        m_terminalService->setDeviceSerial(connected ? serial : QString());
        m_terminalPage->setDeviceConnected(connected, serial);
    }
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
    if (m_displayService != nullptr) {
        m_displayService->setDeviceSerial(connected ? serial : QString());
        m_displayPage->setDeviceConnected(connected, serial);
        if (connected && m_workspaceStack->currentWidget() == m_displayPage) {
            m_displayPage->activate();
        }
    }
    if (m_mirroringPage != nullptr) {
        m_mirroringPage->setDeviceConnected(connected, serial);
        m_mirroringPage->setMirrorRunning(m_scrcpyService->mirrorRunning());
        if (connected && m_workspaceStack->currentWidget() == m_mirroringPage) {
            m_mirroringPage->activate();
        }
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
    if (m_overviewService != nullptr) {
        m_overviewService->setDeviceSerial(connected ? serial : QString());
        m_overviewPage->setDeviceConnected(connected, serial);
        if (connected && m_workspaceStack->currentWidget() == m_overviewPage) {
            m_overviewPage->activate();
        }
    }
    if (m_logcatService != nullptr) {
        m_logcatService->setDeviceSerial(connected ? serial : QString());
        m_logcatPage->setDeviceConnected(connected, serial);
    }
    if (m_otherService != nullptr) {
        m_otherService->setDeviceSerial(connected ? serial : QString());
        m_otherPage->setDeviceConnected(connected, serial);
    }
    if (m_processService != nullptr) {
        m_processService->setDeviceSerial(connected ? serial : QString());
        m_processPage->setDeviceConnected(connected, serial);
    }
    if (m_layoutPage != nullptr) {
        m_layoutPage->setDeviceConnected(connected, serial);
    }
    if (newlyConnected && m_appsService != nullptr && m_processService != nullptr
        && m_fileManagerService != nullptr) {
        preloadDeviceData(serial);
    }

    QString deviceName;
    QString statusText;
    QString statusColor;

    switch (state) {
    case ScrcpyService::DeviceState::ToolUnavailable:
        deviceName = ui::text("scrcpy 不可用");
        statusText = ui::text("不可用");
        statusColor = QStringLiteral("#d45b5b");
        break;
    case ScrcpyService::DeviceState::Disconnected:
        deviceName = ui::text("未检测到设备");
        statusText = ui::text("未连接");
        statusColor = QStringLiteral("#aab3c2");
        break;
    case ScrcpyService::DeviceState::Unauthorized:
        deviceName = serial.isEmpty() ? ui::text("Android 设备") : serial;
        statusText = ui::text("未授权");
        statusColor = QStringLiteral("#e2a43a");
        break;
    case ScrcpyService::DeviceState::Sideload:
        deviceName = serial.isEmpty() ? ui::text("Recovery 设备") : serial;
        statusText = ui::text("侧载模式");
        statusColor = QStringLiteral("#e2a43a");
        break;
    case ScrcpyService::DeviceState::Connected:
        deviceName = serial;
        statusText = ui::text("已连接");
        statusColor = QStringLiteral("#66c95e");
        break;
    }

    m_deviceNameLabel->setText(deviceName);
    m_deviceStatusLabel->setText(statusText);
    m_deviceStatusDot->setStyleSheet(QStringLiteral("color:%1;").arg(statusColor));
    rebuildDeviceSelectorMenu();
}
