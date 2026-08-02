#ifndef AI_MOBILE_TEST_STUDIO_APPS_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_APPS_SERVICE_H

#include <QByteArray>
#include <QHash>
#include <QMetaType>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVector>

struct AndroidAppSummary {
    QString packageName;
    QString displayName;
    QString apkPath;
    bool systemApp = false;
    bool disabled = false;
    bool uninstalled = false;
    QByteArray iconPng;
};

struct AndroidAppPermission {
    QString name;
    bool granted = false;
    bool runtime = false;
    bool changeable = false;
};

struct AndroidAppDetails {
    QString packageName;
    QString displayName;
    QString apkPath;
    bool splitApk = false;
    bool systemApp = false;
    bool disabled = false;
    QString versionName;
    QString versionCode;
    QString targetSdk;
    QString minSdk;
    QString installer;
    QString dataDirectory;
    qint64 codeSize = -1;
    QString backgroundMode;
    QString installDate;
    QString updateDate;
    QVector<AndroidAppPermission> permissions;
};

Q_DECLARE_METATYPE(AndroidAppSummary)
Q_DECLARE_METATYPE(QVector<AndroidAppSummary>)
Q_DECLARE_METATYPE(AndroidAppPermission)
Q_DECLARE_METATYPE(QVector<AndroidAppPermission>)
Q_DECLARE_METATYPE(AndroidAppDetails)

class AppsService : public QObject
{
    Q_OBJECT

public:
    explicit AppsService(QString adbPath, QObject *parent = nullptr);

    void setDeviceSerial(const QString &serial);
    bool busy() const;

public slots:
    void loadApps();
    void loadAppMetadata(const QStringList &packageNames);
    void loadAppDetails(const QString &packageName);
    void installPackages(const QStringList &apkFiles,
                         bool replaceExisting,
                         bool grantPermissions,
                         bool bypassLowTargetSdk);
    void launchApp(const QString &packageName);
    void stopApp(const QString &packageName);
    void setAppEnabled(const QString &packageName, bool enabled);
    void openAppInfo(const QString &packageName);
    void clearAppData(const QString &packageName);
    void uninstallApp(const QString &packageName, bool systemApp);
    void reinstallApp(const QString &packageName);
    void setBackgroundMode(const QString &packageName, const QString &mode);
    void setPermission(const QString &packageName,
                       const QString &permissionName,
                       bool granted);
    void exportApk(const QString &apkPath, const QString &destination);

signals:
    void busyChanged(bool busy);
    void appsLoaded(const QVector<AndroidAppSummary> &apps);
    void appMetadataLoaded(const QVector<AndroidAppSummary> &apps);
    void appDetailsLoaded(const AndroidAppDetails &details);
    void operationStarted(const QString &label, const QString &displayCommand);
    void operationFinished(bool success, const QString &label, const QString &detail);
    void appStateChanged(const QString &packageName, bool refreshList, bool refreshDetails);

private:
    enum class Request {
        Idle,
        AppList,
        AppDetails,
        MetadataPush,
        AppMetadata,
        Action,
        Install,
        Export
    };

    void start(Request request,
               const QString &label,
               const QStringList &arguments,
               bool refreshList = false,
               bool refreshDetails = false);
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void finishRequest();
    void failRequest(const QString &detail);
    void startNextInstall();
    void resetInstallBatch();
    void beginMetadataLoad(QVector<AndroidAppSummary> apps, bool publishAppList);
    void startNextMetadataBatch();
    int applyMetadataResponse(const QString &output);
    void resetMetadataLoad();

    static bool validPackageName(const QString &packageName);
    static QVector<AndroidAppSummary> parseApps(const QString &output);
    static AndroidAppDetails parseDetails(const QString &packageName, const QString &output);
    static QString dumpValue(const QString &output, const QString &key);
    static QVector<AndroidAppPermission> parsePermissions(const QString &dumpOutput,
                                                           const QString &appOpsOutput,
                                                           const QString &changeableOutput);

    QString m_adbPath;
    QString m_metadataJarPath;
    QString m_deviceSerial;
    QString m_currentPackage;
    QString m_currentLabel;
    QByteArray m_output;
    QProcess m_process;
    Request m_request = Request::Idle;
    bool m_refreshList = false;
    bool m_refreshDetails = false;
    QStringList m_pendingInstallFiles;
    QStringList m_installResults;
    int m_installTotal = 0;
    int m_installSucceeded = 0;
    bool m_installReplaceExisting = true;
    bool m_installGrantPermissions = false;
    bool m_installBypassLowTargetSdk = true;
    QVector<AndroidAppSummary> m_metadataApps;
    QStringList m_pendingMetadataPackages;
    QStringList m_queuedMetadataPackages;
    QHash<QString, QString> m_labelCache;
    QHash<QString, QByteArray> m_iconCache;
    int m_metadataRequested = 0;
    int m_metadataLoaded = 0;
    bool m_metadataPublishesAppList = false;
};

#endif // AI_MOBILE_TEST_STUDIO_APPS_SERVICE_H
