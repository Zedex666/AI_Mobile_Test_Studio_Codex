#ifndef AI_MOBILE_TEST_STUDIO_ADB_CONTROL_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_ADB_CONTROL_SERVICE_H

#include <QObject>
#include <QProcess>
#include <QQueue>
#include <QString>
#include <QStringList>

class AdbControlService : public QObject
{
    Q_OBJECT

public:
    explicit AdbControlService(QString adbPath, QObject *parent = nullptr);

    void setDeviceSerial(const QString &serial);

public slots:
    void sendKeyEvent(const QString &keyCode);
    void rebootDevice(const QString &mode, const QString &label);
    void powerOffDevice(const QString &label);

signals:
    void commandStarted(const QString &label, const QString &command);
    void commandFinished(bool success, const QString &label, const QString &detail);

private:
    struct PendingCommand {
        QString label;
        QString displayCommand;
        QStringList arguments;
    };

    void enqueue(PendingCommand command);
    void startNext();
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);

    QString m_adbPath;
    QString m_deviceSerial;
    QString m_output;
    QProcess m_process;
    QQueue<PendingCommand> m_queue;
    PendingCommand m_currentCommand;
};

#endif // AI_MOBILE_TEST_STUDIO_ADB_CONTROL_SERVICE_H
