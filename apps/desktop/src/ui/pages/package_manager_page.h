#ifndef AI_MOBILE_TEST_STUDIO_PACKAGE_MANAGER_PAGE_H
#define AI_MOBILE_TEST_STUDIO_PACKAGE_MANAGER_PAGE_H

#include <QStringList>
#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QProgressBar;
class QPushButton;
class QStackedWidget;
class QToolButton;
class QWidget;

class PackageManagerPage : public QWidget
{
    Q_OBJECT

public:
    explicit PackageManagerPage(QWidget *parent = nullptr);

    void setDeviceConnected(bool connected, const QString &serial);
    void setPackages(const QStringList &packages);
    void setPackageDetails(const QString &packageName,
                           const QString &path,
                           const QString &installer);
    void setCategoryResults(int category, const QStringList &items);
    void setBusy(bool busy);
    void showCommandStarted(const QString &label, const QString &command);
    void showCommandResult(bool success, const QString &label, const QString &detail);

public slots:
    void refreshPackages();
    void showOverview();

signals:
    void packageListRefreshRequested(bool enabledOnly,
                                     bool disabledOnly,
                                     bool thirdPartyOnly,
                                     bool systemOnly);
    void categoryRequested(int category);
    void userRemoveRequested(const QString &userId);
    void installRequested(const QString &apkPath);
    void packageDetailsRequested(const QString &packageName);
    void uninstallRequested(const QString &packageName);
    void clearDataRequested(const QString &packageName);
    void enableRequested(const QString &packageName);
    void disableRequested(const QString &packageName);

private:
    void applySearch();
    void clearDetails();
    void updateControls();
    void requestAction(const QString &title,
                       const QString &message,
                       void (PackageManagerPage::*signal)(const QString &));
    void showPackageWorkspace();
    void showInstallWorkspace();
    void showCatalogWorkspace(int category);
    void selectApkFile();
    void startInstall();
    void copyCommand(const QString &command);

    bool m_connected = false;
    bool m_busy = false;
    bool m_installRunning = false;
    QString m_serial;
    QString m_selectedPackage;
    QStringList m_packages;
    QLabel *m_deviceDot = nullptr;
    QLabel *m_deviceStatus = nullptr;
    QLabel *m_packageCount = nullptr;
    QLabel *m_commandStatus = nullptr;
    QLineEdit *m_searchInput = nullptr;
    QListWidget *m_packageList = nullptr;
    QLabel *m_packageName = nullptr;
    QLabel *m_packagePath = nullptr;
    QLabel *m_packageInstaller = nullptr;
    QLabel *m_resultTitle = nullptr;
    QLabel *m_resultStatus = nullptr;
    QToolButton *m_resultBackButton = nullptr;
    QPushButton *m_resultRemoveButton = nullptr;
    QToolButton *m_installBackButton = nullptr;
    QLineEdit *m_installFilePath = nullptr;
    QToolButton *m_installSelectButton = nullptr;
    QPushButton *m_installButton = nullptr;
    QProgressBar *m_installProgress = nullptr;
    QCheckBox *m_enabledFilter = nullptr;
    QCheckBox *m_disabledFilter = nullptr;
    QCheckBox *m_thirdPartyFilter = nullptr;
    QCheckBox *m_systemFilter = nullptr;
    QToolButton *m_refreshButton = nullptr;
    QPushButton *m_uninstallButton = nullptr;
    QPushButton *m_clearDataButton = nullptr;
    QPushButton *m_enableButton = nullptr;
    QPushButton *m_disableButton = nullptr;
    QListWidget *m_resultList = nullptr;
    QStackedWidget *m_workspaceStack = nullptr;
    QWidget *m_packageToolbar = nullptr;
    int m_resultCategory = -1;
};

#endif // AI_MOBILE_TEST_STUDIO_PACKAGE_MANAGER_PAGE_H
