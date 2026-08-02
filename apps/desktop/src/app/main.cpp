#include "ui/windows/main_window.h"

#include "ui/common/app_preferences.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("AI Mobile Test Studio"));
    QApplication::setApplicationName(QStringLiteral("AI Mobile Test Studio"));
    a.setStyle(QStringLiteral("Fusion"));
    ui::AppPreferences::instance().initialize();
    MainWindow w;
    w.show();
    return a.exec();
}
