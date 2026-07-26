#ifndef AI_MOBILE_TEST_STUDIO_UI_MAIN_WINDOW_H
#define AI_MOBILE_TEST_STUDIO_UI_MAIN_WINDOW_H

#include "services/scrcpy_service.h"

#include <QMainWindow>

class AdbControlService;
class AppsPage;
class AppsService;
class DeviceControlPage;
class FileManagerService;
class FilesPage;
class PackageManagerPage;
class PackageManagerService;
class PerformancePage;
class PerformanceService;
class RecoveryPage;
class RecoveryService;
class TerminalPage;
class TerminalService;
class QLabel;
class QPushButton;
class QStackedWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void buildUi();
    void configureScrcpy();
    void configureDeviceControls();
    void selectWorkspace(int index);
    void updateDeviceUi(ScrcpyService::DeviceState state,
                        const QString &serial,
                        const QString &detail);
    void updateMirrorUi(bool running);

    ScrcpyService *m_scrcpyService = nullptr;
    AdbControlService *m_adbControlService = nullptr;
    AppsService *m_appsService = nullptr;
    AppsPage *m_appsPage = nullptr;
    DeviceControlPage *m_deviceControlPage = nullptr;
    FileManagerService *m_fileManagerService = nullptr;
    FilesPage *m_filesPage = nullptr;
    PackageManagerService *m_packageManagerService = nullptr;
    PackageManagerPage *m_packageManagerPage = nullptr;
    PerformanceService *m_performanceService = nullptr;
    PerformancePage *m_performancePage = nullptr;
    RecoveryService *m_recoveryService = nullptr;
    RecoveryPage *m_recoveryPage = nullptr;
    TerminalService *m_terminalService = nullptr;
    TerminalPage *m_terminalPage = nullptr;
    QStackedWidget *m_workspaceStack = nullptr;
    QPushButton *m_chatNavButton = nullptr;
    QPushButton *m_deviceControlNavButton = nullptr;
    QPushButton *m_packageManagerNavButton = nullptr;
    QPushButton *m_appsNavButton = nullptr;
    QPushButton *m_filesNavButton = nullptr;
    QPushButton *m_recoveryNavButton = nullptr;
    QPushButton *m_performanceNavButton = nullptr;
    QPushButton *m_mirrorButton = nullptr;
    QLabel *m_deviceNameLabel = nullptr;
    QLabel *m_deviceStatusDot = nullptr;
    QLabel *m_deviceStatusLabel = nullptr;
    QLabel *m_sidebarStatusDot = nullptr;
    QLabel *m_sidebarStatusTitle = nullptr;
    QLabel *m_sidebarStatusDetail = nullptr;
    ScrcpyService::DeviceState m_deviceState = ScrcpyService::DeviceState::ToolUnavailable;
    QString m_deviceSerial;
    QString m_deviceDetail;
};
#endif // AI_MOBILE_TEST_STUDIO_UI_MAIN_WINDOW_H
