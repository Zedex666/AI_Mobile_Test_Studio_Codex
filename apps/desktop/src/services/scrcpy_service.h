#ifndef AI_MOBILE_TEST_STUDIO_SCRCPY_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_SCRCPY_SERVICE_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTimer>

class ScrcpyService : public QObject
{
    Q_OBJECT

public:
    enum class DeviceState {
        ToolUnavailable,
        Disconnected,
        Unauthorized,
        Connected,
        Sideload
    };
    Q_ENUM(DeviceState)

    explicit ScrcpyService(QString scrcpyPath, QObject *parent = nullptr);
    ~ScrcpyService() override;

    DeviceState deviceState() const;
    QString deviceSerial() const;
    QString deviceDetail() const;
    QString scrcpyPath() const;
    QString adbExecutablePath() const;
    bool mirrorRunning() const;

    void startMonitoring();

public slots:
    void refreshDeviceState();
    void startMirror();
    void stopMirror();

signals:
    void deviceStateChanged(ScrcpyService::DeviceState state,
                            const QString &serial,
                            const QString &detail);
    void mirrorRunningChanged(bool running);
    void operationError(const QString &message);

private:
    void handleProbeFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleMirrorFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void setDeviceState(DeviceState state, const QString &serial, const QString &detail);
    void setMirrorRunning(bool running);
    QString adbPath() const;
    QString recentMirrorLog() const;

    QString m_scrcpyPath;
    QString m_deviceSerial;
    QString m_deviceDetail;
    QString m_mirrorLog;
    DeviceState m_deviceState = DeviceState::ToolUnavailable;
    bool m_mirrorRunning = false;
    bool m_stopRequested = false;
    QProcess m_probeProcess;
    QProcess m_mirrorProcess;
    QTimer m_pollTimer;
};

#endif // AI_MOBILE_TEST_STUDIO_SCRCPY_SERVICE_H
