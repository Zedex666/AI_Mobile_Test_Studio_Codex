#ifndef AI_MOBILE_TEST_STUDIO_APPIUM_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_APPIUM_SERVICE_H

#include <QElapsedTimer>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QString>
#include <QTimer>
#include <QUrl>

class QNetworkReply;

class AppiumService : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Idle,
        Probing,
        ReusingExisting,
        StartingBundled,
        RunningBundled,
        Failed
    };
    Q_ENUM(State)

    explicit AppiumService(QString runtimeRoot, QObject *parent = nullptr);
    AppiumService(QString runtimeRoot, QUrl statusUrl, QObject *parent = nullptr);
    ~AppiumService() override;

    State state() const;
    bool ownsServerProcess() const;
    QString detail() const;

public slots:
    void ensureStarted();

signals:
    void stateChanged(AppiumService::State state, const QString &detail);

private:
    void probeStatus();
    void handleProbeFinished(QNetworkReply *reply);
    void scheduleProbe(int delayMs);
    void startBundledServer();
    bool writeDriverManifest(const QString &appiumHome, QString *errorMessage) const;
    void stopOwnedServer();
    void setState(State state, const QString &detail);
    void appendProcessOutput();
    QString recentProcessOutput() const;

    QString m_runtimeRoot;
    QUrl m_statusUrl;
    State m_state = State::Idle;
    QString m_detail;
    QString m_processOutput;
    bool m_waitingForBundledServer = false;
    bool m_stopping = false;
    int m_initialProbeAttempts = 0;
    QElapsedTimer m_startupTimer;
    QNetworkAccessManager m_networkManager;
    QPointer<QNetworkReply> m_probeReply;
    QTimer m_probeTimeout;
    QTimer m_probeDelay;
    QProcess m_process;
};

#endif // AI_MOBILE_TEST_STUDIO_APPIUM_SERVICE_H
