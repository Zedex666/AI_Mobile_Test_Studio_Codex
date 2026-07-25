#ifndef AI_MOBILE_TEST_STUDIO_PERFORMANCE_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_PERFORMANCE_SERVICE_H

#include <QHash>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVector>

struct CpuCoreSample {
    int index = 0;
    double usagePercent = 0.0;
    int frequencyMhz = 0;
};

struct PerformanceSample {
    qint64 sampleTimeMs = 0;
    double uptimeSeconds = 0.0;
    int batteryPercent = -1;
    double batteryTemperatureC = 0.0;
    double batteryVoltageV = 0.0;
    double cpuUsagePercent = 0.0;
    double cpuTemperatureC = 0.0;
    QVector<CpuCoreSample> cores;
    qint64 memoryUsedMb = 0;
    qint64 memoryTotalMb = 0;
    double fps = 0.0;
    QString foregroundPackage;
    QString foregroundLabel;
};

class PerformanceService : public QObject
{
    Q_OBJECT

public:
    explicit PerformanceService(QString adbPath, QObject *parent = nullptr);
    ~PerformanceService() override;

    void setDeviceSerial(const QString &serial);
    void setActive(bool active);

    struct CpuTimes {
        quint64 load = 0;
        quint64 tick = 0;
    };

    static CpuTimes parseCpuTimes(const QStringList &fields);

public slots:
    void sampleNow();

signals:
    void sampleReady(const PerformanceSample &sample);
    void samplingError(const QString &message);

private:
    void updateSamplingState();
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    PerformanceSample parseSample(const QString &output);
    static double cpuUsage(const CpuTimes &previous, const CpuTimes &current);
    double parseFps(const QHash<QString, QStringList> &sections,
                    const QString &foregroundPackage,
                    qint64 sampleTimeMs);

    QString m_adbPath;
    QString m_deviceSerial;
    QString m_output;
    bool m_active = false;
    QProcess m_process;
    QTimer m_timer;
    QString m_foregroundPackage;
    QString m_latencyPackage;
    QVector<QString> m_foregroundLayers;
    quint64 m_lastFlips = 0;
    qint64 m_lastFlipsTimeMs = 0;
};

#endif // AI_MOBILE_TEST_STUDIO_PERFORMANCE_SERVICE_H
