#ifndef AI_MOBILE_TEST_STUDIO_OTHER_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_OTHER_SERVICE_H

#include <QObject>
#include <QProcess>
#include <QString>

class OtherService : public QObject
{
    Q_OBJECT

public:
    explicit OtherService(QString adbPath, QObject *parent = nullptr);
    ~OtherService() override;

    void setDeviceSerial(const QString &serial);

public slots:
    void runShellCommand(const QString &label, const QString &command);

signals:
    void busyChanged(bool busy);
    void commandStarted(const QString &label, const QString &displayCommand);
    void commandFinished(bool success,
                         const QString &label,
                         const QString &output);

private:
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    static QString normalizeShellCommand(QString command);

    QString m_adbPath;
    QString m_deviceSerial;
    QString m_label;
    QByteArray m_output;
    QProcess m_process;
};

#endif // AI_MOBILE_TEST_STUDIO_OTHER_SERVICE_H
