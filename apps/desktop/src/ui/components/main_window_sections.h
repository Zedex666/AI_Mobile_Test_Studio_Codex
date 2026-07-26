#ifndef AI_MOBILE_TEST_STUDIO_MAIN_WINDOW_SECTIONS_H
#define AI_MOBILE_TEST_STUDIO_MAIN_WINDOW_SECTIONS_H

class QWidget;
class QLabel;
class QPushButton;

namespace ui {

struct ToolbarSection {
    QWidget *widget = nullptr;
    QPushButton *mirrorButton = nullptr;
    QLabel *deviceNameLabel = nullptr;
    QLabel *deviceStatusDot = nullptr;
    QLabel *deviceStatusLabel = nullptr;
};

struct SidebarSection {
    QWidget *widget = nullptr;
    QPushButton *chatButton = nullptr;
    QPushButton *deviceControlButton = nullptr;
    QPushButton *packageManagerButton = nullptr;
    QPushButton *appsButton = nullptr;
    QPushButton *filesButton = nullptr;
    QPushButton *recoveryButton = nullptr;
    QPushButton *performanceButton = nullptr;
    QPushButton *layoutButton = nullptr;
    QLabel *statusDot = nullptr;
    QLabel *statusTitle = nullptr;
    QLabel *statusDetail = nullptr;
};

QWidget *createHeader();
SidebarSection createSidebar();
ToolbarSection createToolbar();
QWidget *createChatPane();

} // namespace ui

#endif // AI_MOBILE_TEST_STUDIO_MAIN_WINDOW_SECTIONS_H
