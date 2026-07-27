#ifndef AI_MOBILE_TEST_STUDIO_PROCESS_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_PROCESS_SERVICE_H

#include <QObject>
#include <QProcess>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QVector>

struct DeviceProcessEntry {
    QString name;
    double cpuPercent = 0.0;
    QString cpuTime;
    QString memory;
    qint64 memoryBytes = 0;
    int pid = 0;
    QString user;
    QString arguments;
    QString packageName;
    bool isApplication = false;
};

Q_DECLARE_METATYPE(DeviceProcessEntry)
Q_DECLARE_METATYPE(QVector<DeviceProcessEntry>)

class ProcessService : public QObject
{
    Q_OBJECT

public:
    explicit ProcessService(QString adbPath, QObject *parent = nullptr);
    ~ProcessService() override;

    void setDeviceSerial(const QString &serial);
    void setActive(bool active);

public slots:
    void refresh();
    void stopPackage(const QString &packageName);

signals:
    void samplingChanged(bool sampling);
    void processesReady(const QVector<DeviceProcessEntry> &processes);
    void processError(const QString &message);
    void stopFinished(bool success,
                      const QString &packageName,
                      const QString &detail);

private:
    enum class QueryStage {
        None,
        Packages,
        ModernTop,
        LegacyTop
    };

    void updateSamplingState();
    void startPackagesQuery();
    void startTopQuery(bool legacy);
    void handleQueryFinished(int exitCode, QProcess::ExitStatus exitStatus);
    QVector<DeviceProcessEntry> parseProcesses(const QString &output) const;
    static qint64 parseMemoryBytes(const QString &value);

    QString m_adbPath;
    QString m_deviceSerial;
    QByteArray m_output;
    QSet<QString> m_packages;
    bool m_active = false;
    bool m_refreshPending = false;
    QueryStage m_stage = QueryStage::None;
    QProcess m_queryProcess;
    QProcess m_actionProcess;
    QTimer m_timer;
};

#endif // AI_MOBILE_TEST_STUDIO_PROCESS_SERVICE_H
