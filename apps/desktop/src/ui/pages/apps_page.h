#ifndef AI_MOBILE_TEST_STUDIO_APPS_PAGE_H
#define AI_MOBILE_TEST_STUDIO_APPS_PAGE_H

#include "services/apps_service.h"

#include <QVector>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QStackedWidget;
class QToolButton;
class QVBoxLayout;

class AppsPage : public QWidget
{
    Q_OBJECT

public:
    explicit AppsPage(QWidget *parent = nullptr);

    void setDeviceConnected(bool connected, const QString &serial);

public slots:
    void activate();
    void refresh();
    void setBusy(bool busy);
    void setApps(const QVector<AndroidAppSummary> &apps);
    void setAppDetails(const AndroidAppDetails &details);
    void showOperationStarted(const QString &label, const QString &displayCommand);
    void showOperationFinished(bool success, const QString &label, const QString &detail);
    void handleAppStateChanged(const QString &packageName,
                               bool refreshList,
                               bool refreshDetails);

signals:
    void appsRefreshRequested();
    void appDetailsRequested(const QString &packageName);
    void installRequested(const QStringList &apkFiles,
                          bool replaceExisting,
                          bool grantPermissions,
                          bool bypassLowTargetSdk);
    void launchRequested(const QString &packageName);
    void stopRequested(const QString &packageName);
    void enabledRequested(const QString &packageName, bool enabled);
    void appInfoRequested(const QString &packageName);
    void clearDataRequested(const QString &packageName);
    void uninstallRequested(const QString &packageName, bool systemApp);
    void reinstallRequested(const QString &packageName);
    void backgroundModeRequested(const QString &packageName, const QString &mode);
    void permissionRequested(const QString &packageName,
                             const QString &permissionName,
                             bool granted);
    void exportRequested(const QString &apkPath, const QString &destination);

private:
    void setFilter(int filter);
    void applyFilter();
    void selectApp(QListWidgetItem *item);
    void clearDetails();
    void rebuildDetails();
    void rebuildPermissions();
    void updateControls();
    void installApps();
    void clearAppData();
    void uninstallApp();
    void exportApk();

    static QString formatBytes(qint64 bytes);

    bool m_connected = false;
    bool m_busy = false;
    QString m_serial;
    QVector<AndroidAppSummary> m_apps;
    AndroidAppDetails m_details;
    QString m_selectedPackage;
    bool m_selectedUninstalled = false;
    int m_filter = 0;

    QLabel *m_deviceDot = nullptr;
    QLabel *m_deviceStatus = nullptr;
    QLabel *m_operationStatus = nullptr;
    QLineEdit *m_searchInput = nullptr;
    QToolButton *m_refreshButton = nullptr;
    QToolButton *m_installButton = nullptr;
    QVector<QPushButton *> m_filterButtons;
    QListWidget *m_appList = nullptr;
    QStackedWidget *m_detailStack = nullptr;
    QLabel *m_appIcon = nullptr;
    QLabel *m_appTitle = nullptr;
    QLabel *m_packageName = nullptr;
    QLabel *m_appType = nullptr;
    QPushButton *m_launchButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPushButton *m_enableButton = nullptr;
    QPushButton *m_infoButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QPushButton *m_uninstallButton = nullptr;
    QPushButton *m_reinstallButton = nullptr;
    QPushButton *m_exportButton = nullptr;
    QComboBox *m_backgroundMode = nullptr;
    QLabel *m_versionValue = nullptr;
    QLabel *m_sdkValue = nullptr;
    QLabel *m_installerValue = nullptr;
    QLabel *m_installDateValue = nullptr;
    QLabel *m_updateDateValue = nullptr;
    QLabel *m_sizeValue = nullptr;
    QLabel *m_apkPathValue = nullptr;
    QLabel *m_dataPathValue = nullptr;
    QLabel *m_permissionCount = nullptr;
    QWidget *m_permissionsContainer = nullptr;
    QVBoxLayout *m_permissionsLayout = nullptr;
};

#endif // AI_MOBILE_TEST_STUDIO_APPS_PAGE_H
