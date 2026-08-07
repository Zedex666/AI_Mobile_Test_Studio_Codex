#ifndef AI_MOBILE_TEST_STUDIO_SCRCPY_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_SCRCPY_SERVICE_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
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
    QString preferredDeviceSerial() const;
    QStringList connectedDeviceSerials() const;
    bool mirrorRunning() const;

    void startMonitoring();

public slots:
    void refreshDeviceState();
    void setPreferredDeviceSerial(const QString &serial);
    void startMirror(const QStringList &extraArguments = QStringList());
    void stopMirror();
    void queryCameras();

signals:
    void deviceStateChanged(ScrcpyService::DeviceState state,
                            const QString &serial,
                            const QString &detail);
    void connectedDevicesChanged(const QStringList &serials);
    void mirrorRunningChanged(bool running);
    void camerasLoaded(const QStringList &cameras);
    void operationError(const QString &message);

private:
    void handleProbeFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleMirrorFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void setDeviceState(DeviceState state, const QString &serial, const QString &detail);
    void setConnectedDeviceSerials(const QStringList &serials);
    void setMirrorRunning(bool running);
    QString adbPath() const;
    QString recentMirrorLog() const;

    QString m_scrcpyPath;
    QString m_preferredDeviceSerial;
    QStringList m_connectedDeviceSerials;
    QString m_deviceSerial;
    QString m_deviceDetail;
    QString m_mirrorLog;
    QString m_cameraOutput;
    DeviceState m_deviceState = DeviceState::ToolUnavailable;
    bool m_mirrorRunning = false;
    bool m_stopRequested = false;
    bool m_cameraQueryCancelled = false;
    QProcess m_probeProcess;
    QProcess m_mirrorProcess;
    QProcess m_cameraProcess;
    QTimer m_pollTimer;
};

#endif // AI_MOBILE_TEST_STUDIO_SCRCPY_SERVICE_H
