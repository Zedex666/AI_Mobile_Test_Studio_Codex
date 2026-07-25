#ifndef AI_MOBILE_TEST_STUDIO_RECOVERY_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_RECOVERY_SERVICE_H

#include <QByteArray>
#include <QObject>
#include <QProcess>
#include <QString>

class RecoveryService : public QObject
{
    Q_OBJECT

public:
    explicit RecoveryService(QString adbPath, QObject *parent = nullptr);
    ~RecoveryService() override;

    void setSideloadDeviceSerial(const QString &serial);
    bool busy() const;

public slots:
    void startSideload(const QString &zipPath);
    void cancelSideload();

signals:
    void busyChanged(bool busy);
    void sideloadStarted(const QString &displayCommand);
    void outputChanged(const QString &output);
    void progressChanged(int progress);
    void sideloadFinished(bool success, const QString &detail);

private:
    void handleReadyRead();
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void finish(bool success, const QString &detail);
    QString decodedOutput() const;

    QString m_adbPath;
    QString m_deviceSerial;
    QByteArray m_output;
    QProcess m_process;
    bool m_busy = false;
    bool m_cancelRequested = false;
    int m_lastProgress = -1;
};

#endif // AI_MOBILE_TEST_STUDIO_RECOVERY_SERVICE_H
