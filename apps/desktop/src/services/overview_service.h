#ifndef AI_MOBILE_TEST_STUDIO_OVERVIEW_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_OVERVIEW_SERVICE_H

#include <QObject>
#include <QProcess>
#include <QString>

struct DeviceOverview {
    QString name;
    QString brand;
    QString model;
    QString serialNumber;
    QString androidVersion;
    QString sdkVersion;
    QString kernelVersion;
    QString processor;
    QString abi;
    int cpuCount = 0;
    qint64 storageUsedBytes = 0;
    qint64 storageTotalBytes = 0;
    qint64 memoryTotalBytes = 0;
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

signals:
    void loadingChanged(bool loading);
    void overviewReady(const DeviceOverview &overview);
    void overviewError(const QString &message);

private:
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    static DeviceOverview parseOverview(const QString &output);

    QString m_adbPath;
    QString m_deviceSerial;
    QString m_output;
    bool m_refreshPending = false;
    QProcess m_process;
};

#endif // AI_MOBILE_TEST_STUDIO_OVERVIEW_SERVICE_H
