#ifndef AI_MOBILE_TEST_STUDIO_TERMINAL_SESSION_H
#define AI_MOBILE_TEST_STUDIO_TERMINAL_SESSION_H

#include <QByteArray>
#include <QObject>
#include <QString>

enum class TerminalSessionKind {
    AdbShell,
    OpenCode
};

inline QString terminalSessionKindId(TerminalSessionKind kind)
{
    return kind == TerminalSessionKind::OpenCode ? QStringLiteral("opencode")
                                                 : QStringLiteral("adb-shell");
}

inline bool terminalSessionKindFromId(const QString &id, TerminalSessionKind *kind)
{
    if (id == QStringLiteral("adb-shell")) {
        *kind = TerminalSessionKind::AdbShell;
        return true;
    }
    if (id == QStringLiteral("opencode")) {
        *kind = TerminalSessionKind::OpenCode;
        return true;
    }
    return false;
}

class TerminalSession : public QObject
{
    Q_OBJECT

public:
    explicit TerminalSession(TerminalSessionKind kind, QObject *parent = nullptr)
        : QObject(parent)
        , m_kind(kind)
    {
    }

    TerminalSessionKind kind() const
    {
        return m_kind;
    }

    virtual void start() = 0;
    virtual void write(const QByteArray &data) = 0;
    virtual void resize(int columns, int rows) = 0;
    virtual void stop() = 0;

signals:
    void started();
    void outputReady(const QByteArray &data);
    void exited(bool wasStarted, const QString &message);

private:
    TerminalSessionKind m_kind;
};

#endif // AI_MOBILE_TEST_STUDIO_TERMINAL_SESSION_H
