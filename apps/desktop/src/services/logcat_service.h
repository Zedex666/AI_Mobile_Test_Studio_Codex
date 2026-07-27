#ifndef AI_MOBILE_TEST_STUDIO_LOGCAT_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_LOGCAT_SERVICE_H

#include <QHash>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QTimer>

struct LogcatEntry {
    QString time;
    int pid = 0;
    int tid = 0;
    QChar priority;
    QString tag;
    QString processName;
    QString message;
};

class LogcatService : public QObject
{
    Q_OBJECT

public:
    explicit LogcatService(QString adbPath, QObject *parent = nullptr);
    ~LogcatService() override;

    void setDeviceSerial(const QString &serial);
    void setActive(bool active);

public slots:
    void restart();

signals:
    void entryReady(const LogcatEntry &entry);
    void streamStateChanged(bool running);
    void streamError(const QString &message);

private:
    void updateStreamState();
    void startLogcat();
    void stopLogcat();
    void refreshPidNames();
    void consumeLogcatOutput();
    void parsePidOutput();
    bool parseEntry(const QString &line, LogcatEntry *entry) const;

    QString m_adbPath;
    QString m_deviceSerial;
    QProcess m_logcatProcess;
    QProcess m_pidProcess;
    QTimer m_pidTimer;
    QByteArray m_pendingOutput;
    QHash<int, QString> m_pidNames;
    bool m_active = false;
    bool m_restartRequested = false;
};

#endif // AI_MOBILE_TEST_STUDIO_LOGCAT_SERVICE_H
