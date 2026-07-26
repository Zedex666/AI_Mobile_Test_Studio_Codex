#ifndef AI_MOBILE_TEST_STUDIO_ADB_SHELL_SESSION_H
#define AI_MOBILE_TEST_STUDIO_ADB_SHELL_SESSION_H

#include "services/terminal_session.h"

#include <QTcpSocket>

class AdbShellSession : public TerminalSession
{
    Q_OBJECT

public:
    explicit AdbShellSession(QString deviceSerial, QObject *parent = nullptr);

    void start() override;
    void write(const QByteArray &data) override;
    void resize(int columns, int rows) override;
    void stop() override;

private:
    enum class State {
        Connecting,
        SelectingTransport,
        OpeningShell,
        Streaming
    };

    void processIncomingData();
    bool processServiceResponse();
    void retryWithLegacyShell();
    void finish(bool wasStarted, const QString &message);

    QString m_deviceSerial;
    QTcpSocket m_socket;
    QByteArray m_buffer;
    State m_state = State::Connecting;
    bool m_useShellV2 = true;
    bool m_started = false;
    bool m_reconnecting = false;
    bool m_finished = false;
};

#endif // AI_MOBILE_TEST_STUDIO_ADB_SHELL_SESSION_H
