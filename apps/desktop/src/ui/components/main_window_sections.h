#ifndef AI_MOBILE_TEST_STUDIO_MAIN_WINDOW_SECTIONS_H
#define AI_MOBILE_TEST_STUDIO_MAIN_WINDOW_SECTIONS_H

class QWidget;
class QLabel;
class QPushButton;

namespace ui {

struct HeaderSection {
    QWidget *widget = nullptr;
    QPushButton *deviceSelectorButton = nullptr;
    QPushButton *deviceCenterButton = nullptr;
    QPushButton *settingsButton = nullptr;
    QLabel *deviceNameLabel = nullptr;
    QLabel *deviceStatusDot = nullptr;
    QLabel *deviceStatusLabel = nullptr;
};

struct SidebarSection {
    QWidget *widget = nullptr;
    QPushButton *overviewButton = nullptr;
    QPushButton *displayButton = nullptr;
    QPushButton *mirroringButton = nullptr;
    QPushButton *chatButton = nullptr;
    QPushButton *deviceControlButton = nullptr;
    QPushButton *packageManagerButton = nullptr;
    QPushButton *appsButton = nullptr;
    QPushButton *filesButton = nullptr;
    QPushButton *processButton = nullptr;
    QPushButton *recoveryButton = nullptr;
    QPushButton *performanceButton = nullptr;
    QPushButton *layoutButton = nullptr;
    QPushButton *logcatButton = nullptr;
    QPushButton *otherButton = nullptr;
    QPushButton *settingsButton = nullptr;
};

HeaderSection createHeader();
SidebarSection createSidebar();
QWidget *createChatPane();

} // namespace ui

#endif // AI_MOBILE_TEST_STUDIO_MAIN_WINDOW_SECTIONS_H
