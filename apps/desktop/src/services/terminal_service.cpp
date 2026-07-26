#include "services/terminal_service.h"

#include "services/adb_shell_session.h"
#include "services/conpty_session.h"
#include "services/terminal_session.h"

#include <QTimer>

#include <functional>

TerminalService::TerminalService(QObject *parent)
    : QObject(parent)
{
}

TerminalService::~TerminalService()
{
    closeAllSessions();
}

void TerminalService::setDeviceSerial(const QString &serial)
{
    if (serial == m_deviceSerial) {
        return;
    }
    closeSessions(true, [](TerminalSession *session) {
        return session->kind() == TerminalSessionKind::AdbShell;
    });
    m_deviceSerial = serial;
}

void TerminalService::setOpenCodeConfiguration(const QString &executablePath,
                                               const QString &workingDirectory,
                                               const QString &nodeExecutablePath,
                                               const QString &nodePtyModulePath,
                                               const QString &hostScriptPath)
{
    m_openCodeExecutablePath = executablePath;
    m_openCodeWorkingDirectory = workingDirectory;
    m_nodeExecutablePath = nodeExecutablePath;
    m_nodePtyModulePath = nodePtyModulePath;
    m_terminalHostScriptPath = hostScriptPath;
}

void TerminalService::createSession(const QString &sessionId, const QString &kindId)
{
    if (sessionId.isEmpty() || m_sessions.contains(sessionId)) {
        return;
    }

    TerminalSessionKind kind = TerminalSessionKind::AdbShell;
    if (!terminalSessionKindFromId(kindId, &kind)) {
        emit sessionFailed(sessionId, QStringLiteral("未知终端类型：%1").arg(kindId));
        return;
    }
    m_sessionKinds.insert(sessionId, kind);

    TerminalSession *session = nullptr;
    if (kind == TerminalSessionKind::AdbShell) {
        if (m_deviceSerial.isEmpty()) {
            emit sessionFailed(sessionId, QStringLiteral("未连接 Android 设备"));
            return;
        }
        session = new AdbShellSession(m_deviceSerial, this);
    } else {
        session = new ConPtySession(m_openCodeExecutablePath,
                                    m_openCodeWorkingDirectory,
                                    m_nodeExecutablePath,
                                    m_nodePtyModulePath,
                                    m_terminalHostScriptPath,
                                    this);
    }

    m_sessions.insert(sessionId, session);
    connect(session, &TerminalSession::started, this, [this, sessionId, session] {
        if (m_sessions.value(sessionId) == session) {
            emit sessionStarted(sessionId);
        }
    });
    connect(session,
            &TerminalSession::outputReady,
            this,
            [this, sessionId, session](const QByteArray &data) {
                if (m_sessions.value(sessionId) == session) {
                    emit sessionOutput(sessionId, data);
                }
            });
    connect(session,
            &TerminalSession::exited,
            this,
            [this, sessionId, session](bool wasStarted, const QString &message) {
                QTimer::singleShot(0, this, [this, sessionId, session, wasStarted, message] {
                    if (m_sessions.value(sessionId) != session) {
                        return;
                    }
                    m_sessions.remove(sessionId);
                    session->deleteLater();
                    if (wasStarted) {
                        emit sessionClosed(sessionId, false, message);
                    } else {
                        emit sessionFailed(sessionId, message);
                    }
                });
            });
    session->start();
}

void TerminalService::writeSession(const QString &sessionId, const QByteArray &data)
{
    if (TerminalSession *session = m_sessions.value(sessionId, nullptr)) {
        session->write(data);
    }
}

void TerminalService::resizeSession(const QString &sessionId, int columns, int rows)
{
    if (TerminalSession *session = m_sessions.value(sessionId, nullptr)) {
        session->resize(columns, rows);
    }
}

void TerminalService::restartSession(const QString &sessionId)
{
    if (!m_sessionKinds.contains(sessionId)) {
        return;
    }
    const QString kindId = terminalSessionKindId(m_sessionKinds.value(sessionId));
    if (m_sessions.contains(sessionId)) {
        removeSession(sessionId, false);
    }
    createSession(sessionId, kindId);
}

void TerminalService::closeSession(const QString &sessionId)
{
    removeSession(sessionId, true);
    m_sessionKinds.remove(sessionId);
}

void TerminalService::removeSession(const QString &sessionId, bool notify)
{
    TerminalSession *session = m_sessions.take(sessionId);
    if (session == nullptr) {
        return;
    }
    session->stop();
    session->deleteLater();
    if (notify) {
        emit sessionClosed(sessionId, true, QString());
    }
}

void TerminalService::closeSessions(
    bool notify,
    const std::function<bool(TerminalSession *)> &predicate)
{
    const QStringList sessionIds = m_sessions.keys();
    for (const QString &sessionId : sessionIds) {
        TerminalSession *session = m_sessions.value(sessionId, nullptr);
        if (session != nullptr && predicate(session)) {
            removeSession(sessionId, notify);
        }
    }
}

void TerminalService::closeAllSessions()
{
    closeSessions(false, [](TerminalSession *) {
        return true;
    });
    m_sessionKinds.clear();
}
