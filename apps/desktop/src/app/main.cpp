#include "ui/windows/main_window.h"

#include "services/appium_service.h"
#include "ui/common/app_preferences.h"
#include "ui/common/widget_helpers.h"

#include <QApplication>
#include <QDir>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("AI Mobile Test Studio"));
    QApplication::setApplicationName(QStringLiteral("AI Mobile Test Studio"));
    QApplication::setWindowIcon(ui::imageIcon(QStringLiteral("icons/app.png")));
    a.setStyle(QStringLiteral("Fusion"));
    ui::AppPreferences::instance().initialize();
    AppiumService appiumService(
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("runtime")),
        &a);
    QTimer::singleShot(0, &appiumService, &AppiumService::ensureStarted);
    MainWindow w;
    w.show();
    return a.exec();
}
