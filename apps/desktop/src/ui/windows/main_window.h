#ifndef AI_MOBILE_TEST_STUDIO_UI_MAIN_WINDOW_H
#define AI_MOBILE_TEST_STUDIO_UI_MAIN_WINDOW_H

#include "services/scrcpy_service.h"

#include <QMainWindow>

class AdbControlService;
class AppsPage;
class AppsService;
class DeviceControlPage;
class DeviceCenterService;
class DeviceCenterWindow;
class DisplayPage;
class DisplayService;
class FileManagerService;
class FilesPage;
class PackageManagerPage;
class PackageManagerService;
class PerformancePage;
class PerformanceService;
class RecoveryPage;
class RecoveryService;
class SettingsPage;
class LayoutPage;
class LogcatPage;
class LogcatService;
class MirroringPage;
class OtherPage;
class OtherService;
class OverviewPage;
class OverviewService;
class ProcessPage;
class ProcessService;
class TerminalPage;
class TerminalService;
class QLabel;
class QMenu;
class QPushButton;
class QStackedWidget;
class QResizeEvent;
class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void buildUi();
    void configureScrcpy();
    void configureDeviceControls();
    void showDeviceSelectorMenu();
    void rebuildDeviceSelectorMenu();
    void showDeviceCenter();
    void preloadWebWorkspaces();
    void cancelWorkspacePreload();
    void preloadDeviceData(const QString &serial);
    void selectWorkspace(int index);
    void animateWorkspaceTransition(int index);
    void applyLanguage();
    void updateDeviceUi(ScrcpyService::DeviceState state,
                        const QString &serial,
                        const QString &detail);

    ScrcpyService *m_scrcpyService = nullptr;
    AdbControlService *m_adbControlService = nullptr;
    AppsService *m_appsService = nullptr;
    AppsPage *m_appsPage = nullptr;
    DisplayService *m_displayService = nullptr;
    DisplayPage *m_displayPage = nullptr;
    MirroringPage *m_mirroringPage = nullptr;
    DeviceControlPage *m_deviceControlPage = nullptr;
    DeviceCenterService *m_deviceCenterService = nullptr;
    DeviceCenterWindow *m_deviceCenterWindow = nullptr;
    FileManagerService *m_fileManagerService = nullptr;
    FilesPage *m_filesPage = nullptr;
    PackageManagerService *m_packageManagerService = nullptr;
    PackageManagerPage *m_packageManagerPage = nullptr;
    PerformanceService *m_performanceService = nullptr;
    PerformancePage *m_performancePage = nullptr;
    LayoutPage *m_layoutPage = nullptr;
    LogcatService *m_logcatService = nullptr;
    LogcatPage *m_logcatPage = nullptr;
    OtherService *m_otherService = nullptr;
    OtherPage *m_otherPage = nullptr;
    OverviewService *m_overviewService = nullptr;
    OverviewPage *m_overviewPage = nullptr;
    ProcessService *m_processService = nullptr;
    ProcessPage *m_processPage = nullptr;
    RecoveryService *m_recoveryService = nullptr;
    RecoveryPage *m_recoveryPage = nullptr;
    SettingsPage *m_settingsPage = nullptr;
    TerminalService *m_terminalService = nullptr;
    TerminalPage *m_terminalPage = nullptr;
    QStackedWidget *m_workspaceStack = nullptr;
    QPushButton *m_overviewNavButton = nullptr;
    QPushButton *m_displayNavButton = nullptr;
    QPushButton *m_mirroringNavButton = nullptr;
    QPushButton *m_chatNavButton = nullptr;
    QPushButton *m_deviceControlNavButton = nullptr;
    QPushButton *m_packageManagerNavButton = nullptr;
    QPushButton *m_appsNavButton = nullptr;
    QPushButton *m_filesNavButton = nullptr;
    QPushButton *m_processNavButton = nullptr;
    QPushButton *m_recoveryNavButton = nullptr;
    QPushButton *m_performanceNavButton = nullptr;
    QPushButton *m_layoutNavButton = nullptr;
    QPushButton *m_logcatNavButton = nullptr;
    QPushButton *m_otherNavButton = nullptr;
    QPushButton *m_settingsNavButton = nullptr;
    QPushButton *m_deviceSelectorButton = nullptr;
    QPushButton *m_headerSettingsButton = nullptr;
    QPushButton *m_headerDeviceCenterButton = nullptr;
    QMenu *m_deviceSelectorMenu = nullptr;
    QWidget *m_sidebarWidget = nullptr;
    QLabel *m_deviceNameLabel = nullptr;
    QLabel *m_deviceStatusDot = nullptr;
    QLabel *m_deviceStatusLabel = nullptr;
    QLabel *m_workspacePreloadCover = nullptr;
    int m_selectedWorkspaceIndex = 0;
    quint64 m_workspacePreloadGeneration = 0;
    bool m_workspacePreloading = false;
    ScrcpyService::DeviceState m_deviceState = ScrcpyService::DeviceState::ToolUnavailable;
    QString m_deviceSerial;
    QString m_deviceDetail;
    QStringList m_connectedDeviceSerials;
};
#endif // AI_MOBILE_TEST_STUDIO_UI_MAIN_WINDOW_H
