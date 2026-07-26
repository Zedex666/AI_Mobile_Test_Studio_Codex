#ifndef AI_MOBILE_TEST_STUDIO_PACKAGE_MANAGER_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_PACKAGE_MANAGER_SERVICE_H

#include <QByteArray>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

class PackageManagerService : public QObject
{
    Q_OBJECT

public:
    struct PackageFilters {
        bool enabledOnly = false;
        bool disabledOnly = false;
        bool thirdPartyOnly = false;
        bool systemOnly = false;
    };

    explicit PackageManagerService(QString adbPath, QObject *parent = nullptr);

    void setDeviceSerial(const QString &serial);
    bool busy() const;

public slots:
    void loadCategory(int category);
    void loadPackages(PackageFilters filters);
    void loadPackageDetails(const QString &packageName);
    void installPackage(const QString &apkPath);
    void removeUser(const QString &userId);
    void uninstallPackage(const QString &packageName);
    void clearPackageData(const QString &packageName);
    void enablePackage(const QString &packageName);
    void disablePackage(const QString &packageName);

signals:
    void busyChanged(bool busy);
    void commandStarted(const QString &label, const QString &command);
    void commandFinished(bool success, const QString &label, const QString &detail);
    void packagesLoaded(const QStringList &packages);
    void packageDetailsLoaded(const QString &packageName,
                              const QString &path,
                              const QString &installer);
    void categoryLoaded(int category, const QStringList &items);
    void categoryActionCompleted(int category);
    void packageActionCompleted(const QString &packageName);

private:
    enum class Request {
        Idle,
        PackageList,
        PackagePath,
        PackageInstaller,
        Install,
        PackageAction,
        Catalog
    };

    void start(Request request,
               const QString &label,
               const QString &displayCommand,
               const QStringList &arguments);
    void startInstallerQuery();
    void finishRequest();
    void failRequest(const QString &detail);
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);

    static QStringList parsePackageList(const QString &output);
    static QStringList parseCatalogItems(const QString &output);
    static QString packagePath(const QString &output, const QString &packageName);
    static QString packageInstaller(const QString &output);

    QString m_adbPath;
    QString m_deviceSerial;
    QByteArray m_output;
    QString m_currentPackage;
    QString m_currentPath;
    QString m_currentLabel;
    int m_currentCategory = -1;
    Request m_request = Request::Idle;
    QProcess m_process;
};

#endif // AI_MOBILE_TEST_STUDIO_PACKAGE_MANAGER_SERVICE_H
