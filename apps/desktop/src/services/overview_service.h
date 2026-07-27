#ifndef AI_MOBILE_TEST_STUDIO_OVERVIEW_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_OVERVIEW_SERVICE_H

#include <QObject>
#include <QProcess>
#include <QByteArray>
#include <QString>

struct DeviceOverview {
    QString name;
    QString brand;
    QString manufacturer;
    QString model;
    QString deviceType;
    QString product;
    QString codename;
    QString serialNumber;
    QString androidVersion;
    QString sdkVersion;
    QString kernelVersion;
    QString processor;
    QString abi;
    int cpuCount = 0;
    qint64 storageUsedBytes = 0;
    qint64 storageTotalBytes = 0;
    qint64 memoryUsedBytes = 0;
    qint64 memoryTotalBytes = 0;
    int batteryLevel = -1;
    QString batteryHealth;
    qint64 uptimeSeconds = 0;
    QString physicalResolution;
    QString physicalDensity;
    QString resolution;
    QString density;
    double fontScale = 1.0;
    QString wifi;
    QString ipAddress;
    QString macAddress;
};

class OverviewService : public QObject
{
    Q_OBJECT

public:
    explicit OverviewService(QString adbPath, QObject *parent = nullptr);
    ~OverviewService() override;

    void setDeviceSerial(const QString &serial);

public slots:
    void refresh();
    void captureScreenshot();
    void startShizuku();
    void togglePower();

signals:
    void loadingChanged(bool loading);
    void overviewReady(const DeviceOverview &overview);
    void overviewError(const QString &message);
    void screenshotLoadingChanged(bool loading);
    void screenshotReady(const QByteArray &pngData);
    void actionFinished(bool success, const QString &label, const QString &detail);

private:
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void runAction(const QString &label, const QString &shellCommand);
    static DeviceOverview parseOverview(const QString &output);

    QString m_adbPath;
    QString m_deviceSerial;
    QString m_output;
    bool m_refreshPending = false;
    QProcess m_process;
    QProcess m_screenshotProcess;
    QProcess m_actionProcess;
    QByteArray m_screenshotOutput;
    QByteArray m_actionOutput;
    QString m_actionLabel;
};

#endif // AI_MOBILE_TEST_STUDIO_OVERVIEW_SERVICE_H
