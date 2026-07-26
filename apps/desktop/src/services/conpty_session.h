#ifndef AI_MOBILE_TEST_STUDIO_CONPTY_SESSION_H
#define AI_MOBILE_TEST_STUDIO_CONPTY_SESSION_H

#include "services/terminal_session.h"

#include <QStringList>

class QProcess;

class ConPtySession : public TerminalSession
{
    Q_OBJECT

public:
    explicit ConPtySession(QString executablePath,
                           QString workingDirectory,
                           QString nodeExecutablePath,
                           QString nodePtyModulePath,
                           QString hostScriptPath,
                           QObject *parent = nullptr);
    ~ConPtySession() override;

    void setArguments(const QStringList &arguments);

    void start() override;
    void write(const QByteArray &data) override;
    void resize(int columns, int rows) override;
    void stop() override;

private:
    void sendFrame(char type, const QByteArray &payload);
    void processHostMessages();
    void finish(bool wasStarted, const QString &message);

    QString m_executablePath;
    QString m_workingDirectory;
    QString m_nodeExecutablePath;
    QString m_nodePtyModulePath;
    QString m_hostScriptPath;
    QStringList m_arguments;
    QProcess *m_process = nullptr;
    QByteArray m_hostMessageBuffer;
    QString m_hostError;
    int m_columns = 120;
    int m_rows = 36;
    bool m_started = false;
    bool m_finished = false;
};

#endif // AI_MOBILE_TEST_STUDIO_CONPTY_SESSION_H
