#ifndef AI_MOBILE_TEST_STUDIO_TERMINAL_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_TERMINAL_SERVICE_H

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>

#include <functional>

class TerminalSession;
enum class TerminalSessionKind;

class TerminalService : public QObject
{
    Q_OBJECT

public:
    explicit TerminalService(QObject *parent = nullptr);
    ~TerminalService() override;

    void setDeviceSerial(const QString &serial);
    void setOpenCodeConfiguration(const QString &executablePath,
                                  const QString &workingDirectory,
                                  const QString &nodeExecutablePath,
                                  const QString &nodePtyModulePath,
                                  const QString &hostScriptPath,
                                  const QHash<QString, QString> &environmentVariables = {});

public slots:
    void createSession(const QString &sessionId, const QString &kindId);
    void writeSession(const QString &sessionId, const QByteArray &data);
    void resizeSession(const QString &sessionId, int columns, int rows);
    void restartSession(const QString &sessionId);
    void closeSession(const QString &sessionId);

signals:
    void sessionStarted(const QString &sessionId);
    void sessionOutput(const QString &sessionId, const QByteArray &data);
    void sessionFailed(const QString &sessionId, const QString &message);
    void sessionClosed(const QString &sessionId, bool expected, const QString &message);

private:
    void removeSession(const QString &sessionId, bool notify);
    void closeSessions(bool notify, const std::function<bool(TerminalSession *)> &predicate);
    void closeAllSessions();

    QString m_deviceSerial;
    QString m_openCodeExecutablePath;
    QString m_openCodeWorkingDirectory;
    QString m_nodeExecutablePath;
    QString m_nodePtyModulePath;
    QString m_terminalHostScriptPath;
    QHash<QString, QString> m_openCodeEnvironment;
    QHash<QString, TerminalSession *> m_sessions;
    QHash<QString, TerminalSessionKind> m_sessionKinds;
};

#endif // AI_MOBILE_TEST_STUDIO_TERMINAL_SERVICE_H
