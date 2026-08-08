#include "services/conpty_session.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>

#include <algorithm>
#include <limits>
#include <utility>

namespace {

constexpr qsizetype kFrameHeaderSize = 5;
constexpr qsizetype kMaximumHostMessageBytes = 64 * 1024;

void appendLittleEndian32(QByteArray *data, quint32 value)
{
    data->append(static_cast<char>(value & 0xff));
    data->append(static_cast<char>((value >> 8) & 0xff));
    data->append(static_cast<char>((value >> 16) & 0xff));
    data->append(static_cast<char>((value >> 24) & 0xff));
}

QByteArray resizePayload(int columns, int rows)
{
    QByteArray payload;
    payload.reserve(8);
    appendLittleEndian32(&payload, static_cast<quint32>(columns));
    appendLittleEndian32(&payload, static_cast<quint32>(rows));
    return payload;
}

bool runtimePathExists(const QString &path)
{
    if (QFileInfo::exists(path)) {
        return true;
    }
    const QString normalized = QDir::fromNativeSeparators(path);
    const qsizetype archiveEnd = normalized.indexOf(QStringLiteral(".asar/"));
    return archiveEnd >= 0
        && QFileInfo::exists(normalized.left(archiveEnd + QStringLiteral(".asar").size()));
}

} // namespace

ConPtySession::ConPtySession(QString executablePath,
                             QString workingDirectory,
                             QString nodeExecutablePath,
                             QString nodePtyModulePath,
                             QString hostScriptPath,
                             QObject *parent)
    : TerminalSession(TerminalSessionKind::OpenCode, parent)
    , m_executablePath(std::move(executablePath))
    , m_workingDirectory(std::move(workingDirectory))
    , m_nodeExecutablePath(std::move(nodeExecutablePath))
    , m_nodePtyModulePath(std::move(nodePtyModulePath))
    , m_hostScriptPath(std::move(hostScriptPath))
    , m_process(new QProcess(this))
{
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this] {
        const QByteArray data = m_process->readAllStandardOutput();
        if (!data.isEmpty()) {
            emit outputReady(data);
        }
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this] {
        m_hostMessageBuffer += m_process->readAllStandardError();
        if (m_hostMessageBuffer.size() > kMaximumHostMessageBytes) {
            m_hostMessageBuffer.remove(0,
                                       m_hostMessageBuffer.size()
                                           - kMaximumHostMessageBytes);
        }
        processHostMessages();
    });
    connect(m_process,
            &QProcess::errorOccurred,
            this,
            [this](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart && !m_finished) {
                    finish(false,
                           QStringLiteral("无法启动 OpenCode 终端宿主：%1")
                               .arg(m_process->errorString()));
                }
            });
    connect(m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                if (m_finished) {
                    return;
                }
                const QByteArray remainingOutput = m_process->readAllStandardOutput();
                if (!remainingOutput.isEmpty()) {
                    emit outputReady(remainingOutput);
                }
                m_hostMessageBuffer += m_process->readAllStandardError();
                processHostMessages();

                QString message = m_hostError;
                if (message.isEmpty()) {
                    message = exitStatus == QProcess::CrashExit
                        ? QStringLiteral("OpenCode 终端宿主异常退出")
                        : QStringLiteral("OpenCode 已退出（代码 %1）").arg(exitCode);
                }
                finish(m_started, message);
            });
}

ConPtySession::~ConPtySession()
{
    stop();
}

void ConPtySession::setArguments(const QStringList &arguments)
{
    if (!m_started && m_process->state() == QProcess::NotRunning) {
        m_arguments = arguments;
    }
}

void ConPtySession::setEnvironmentVariables(
    const QHash<QString, QString> &environmentVariables)
{
    if (!m_started && m_process->state() == QProcess::NotRunning) {
        m_environmentVariables = environmentVariables;
    }
}

void ConPtySession::start()
{
    if (m_started || m_finished || m_process->state() != QProcess::NotRunning) {
        return;
    }

    const QList<QPair<QString, QString>> requiredPaths = {
        {m_executablePath, QStringLiteral("OpenCode 可执行程序")},
        {m_nodeExecutablePath, QStringLiteral("Node.js 运行时")},
        {m_nodePtyModulePath, QStringLiteral("node-pty 模块")},
        {m_hostScriptPath, QStringLiteral("ConPTY 终端宿主脚本")}};
    for (const auto &required : requiredPaths) {
        if (required.first.isEmpty() || !runtimePathExists(required.first)) {
            finish(false,
                   QStringLiteral("未找到%1：%2")
                       .arg(required.second,
                            required.first.isEmpty() ? QStringLiteral("未配置")
                                                     : required.first));
            return;
        }
    }
    if (!QFileInfo(m_workingDirectory).isDir()) {
        finish(false, QStringLiteral("OpenCode 工作目录不存在：%1").arg(m_workingDirectory));
        return;
    }

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("TERM"), QStringLiteral("xterm-256color"));
    environment.insert(QStringLiteral("COLORTERM"), QStringLiteral("truecolor"));
    environment.insert(QStringLiteral("FORCE_COLOR"), QStringLiteral("1"));
    for (auto iterator = m_environmentVariables.cbegin();
         iterator != m_environmentVariables.cend();
         ++iterator) {
        if (iterator.value().isEmpty()) {
            environment.remove(iterator.key());
        } else {
            environment.insert(iterator.key(), iterator.value());
        }
    }
    if (QFileInfo(m_nodeExecutablePath).fileName().compare(QStringLiteral("Code.exe"),
                                                          Qt::CaseInsensitive)
        == 0) {
        environment.insert(QStringLiteral("ELECTRON_RUN_AS_NODE"), QStringLiteral("1"));
    }
    m_process->setProcessEnvironment(environment);
    m_process->setWorkingDirectory(m_workingDirectory);

    QStringList hostArguments = {m_hostScriptPath,
                                 QStringLiteral("--file"),
                                 QDir::toNativeSeparators(m_executablePath),
                                 QStringLiteral("--cwd"),
                                 QDir::toNativeSeparators(m_workingDirectory),
                                 QStringLiteral("--module"),
                                 QDir::toNativeSeparators(m_nodePtyModulePath),
                                 QStringLiteral("--cols"),
                                 QString::number(m_columns),
                                 QStringLiteral("--rows"),
                                 QString::number(m_rows)};
    for (const QString &argument : std::as_const(m_arguments)) {
        hostArguments.append(QStringLiteral("--arg"));
        hostArguments.append(argument);
    }
    m_process->setProgram(QFileInfo(m_nodeExecutablePath).absoluteFilePath());
    m_process->setArguments(hostArguments);
    m_process->start();
}

void ConPtySession::write(const QByteArray &data)
{
    if (m_started && !m_finished && !data.isEmpty()) {
        sendFrame('i', data);
    }
}

void ConPtySession::resize(int columns, int rows)
{
    if (columns <= 0 || rows <= 0) {
        return;
    }
    const int maximum = std::numeric_limits<qint16>::max();
    m_columns = std::min(columns, maximum);
    m_rows = std::min(rows, maximum);
    if (m_started && !m_finished) {
        sendFrame('r', resizePayload(m_columns, m_rows));
    }
}

void ConPtySession::stop()
{
    if (m_finished) {
        return;
    }
    m_finished = true;
    if (m_process->state() != QProcess::NotRunning) {
        sendFrame('x', QByteArray());
        m_process->closeWriteChannel();
        m_process->terminate();
    }
}

void ConPtySession::sendFrame(char type, const QByteArray &payload)
{
    if (m_process->state() == QProcess::NotRunning) {
        return;
    }
    QByteArray frame;
    frame.reserve(kFrameHeaderSize + payload.size());
    frame.append(type);
    appendLittleEndian32(&frame, static_cast<quint32>(payload.size()));
    frame.append(payload);
    m_process->write(frame);
}

void ConPtySession::processHostMessages()
{
    while (true) {
        const qsizetype newline = m_hostMessageBuffer.indexOf('\n');
        if (newline < 0) {
            return;
        }
        QByteArray line = m_hostMessageBuffer.left(newline).trimmed();
        m_hostMessageBuffer.remove(0, newline + 1);
        if (line == QByteArrayLiteral("READY")) {
            if (!m_started && !m_finished) {
                m_started = true;
                emit started();
            }
        } else if (line.startsWith(QByteArrayLiteral("ERROR "))) {
            m_hostError = QString::fromUtf8(
                QByteArray::fromBase64(line.sliced(6)));
        }
    }
}

void ConPtySession::finish(bool wasStarted, const QString &message)
{
    if (m_finished) {
        return;
    }
    m_finished = true;
    emit exited(wasStarted, message);
}
