#ifndef AI_MOBILE_TEST_STUDIO_DEVICE_CENTER_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_DEVICE_CENTER_SERVICE_H

#include <QImage>
#include <QList>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

struct DeviceCenterDevice
{
    QString id;
    QString serialNumber;
    QString name;
    QString androidVersion;
    QString sdkVersion;
    QString state;
    bool remote = false;
    bool online = false;
};

class DeviceCenterService : public QObject
{
    Q_OBJECT

public:
    explicit DeviceCenterService(QString adbPath, QObject *parent = nullptr);
    ~DeviceCenterService() override;

    QList<DeviceCenterDevice> devices() const;

public slots:
    void refreshDevices();
    void connectDevice(const QString &host, int port = 0);
    void pairDevice(const QString &host, int port, const QString &pairingCode);
    void disconnectDevice(const QString &deviceId);
    void removeRememberedDevice(const QString &deviceId);
    void enableWireless(const QString &deviceId);
    void captureScreenshot(const QString &deviceId);

signals:
    void refreshStarted();
    void devicesUpdated(const QList<DeviceCenterDevice> &devices);
    void operationStarted(const QString &label);
    void operationFinished(bool success, const QString &label, const QString &detail);
    void screenshotReady(const QString &deviceId, const QImage &image);
    void screenshotFailed(const QString &deviceId, const QString &detail);

private:
    enum class CommandAction {
        None,
        Connect,
        Pair,
        Disconnect,
        QueryWirelessIp,
        EnableTcpip,
        ConnectWireless
    };

    void handleListFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void queryNextDeviceDetails();
    void handleDetailFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void finishRefresh();
    void startCommand(CommandAction action,
                      const QStringList &arguments,
                      const QString &label,
                      const QString &context,
                      bool announce = true);
    void handleCommandFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void finishCommand(bool success, const QString &detail);
    void startProcess(QProcess &process, const QStringList &arguments);
    QList<DeviceCenterDevice> loadRememberedDevices() const;
    void saveRememberedDevices(const QList<DeviceCenterDevice> &devices) const;
    static bool isRemoteDeviceId(const QString &deviceId);
    static QString normalizedEndpoint(const QString &host, int port);
    static QString processFailureDetail(QProcess &process,
                                        const QByteArray &stdoutData,
                                        const QByteArray &stderrData,
                                        const QString &fallback);

    QString m_adbPath;
    QList<DeviceCenterDevice> m_devices;
    QList<DeviceCenterDevice> m_detectedDevices;
    int m_detailIndex = -1;
    bool m_refreshQueued = false;
    QByteArray m_listOutput;
    QByteArray m_listError;
    QByteArray m_detailOutput;
    QByteArray m_detailError;
    QByteArray m_commandOutput;
    QByteArray m_commandError;
    QByteArray m_screenshotOutput;
    QByteArray m_screenshotError;
    QString m_detailDeviceId;
    QString m_screenshotDeviceId;
    QString m_commandLabel;
    QString m_commandContext;
    QString m_wirelessIp;
    CommandAction m_commandAction = CommandAction::None;
    QProcess m_listProcess;
    QProcess m_detailProcess;
    QProcess m_commandProcess;
    QProcess m_screenshotProcess;
};

#endif // AI_MOBILE_TEST_STUDIO_DEVICE_CENTER_SERVICE_H
