#include "services/logcat_service.h"

#include <QDate>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include <utility>

namespace {

constexpr int kPidRefreshIntervalMs = 10000;

} // namespace

LogcatService::LogcatService(QString adbPath, QObject *parent)
    : QObject(parent)
    , m_adbPath(QDir::cleanPath(std::move(adbPath)))
    , m_logcatProcess(this)
    , m_pidProcess(this)
    , m_pidTimer(this)
{
    m_pidTimer.setInterval(kPidRefreshIntervalMs);
    connect(&m_pidTimer, &QTimer::timeout, this, &LogcatService::refreshPidNames);

    connect(&m_logcatProcess, &QProcess::started, this, [this] {
        emit streamStateChanged(true);
    });
    connect(&m_logcatProcess,
            &QProcess::readyReadStandardOutput,
            this,
            &LogcatService::consumeLogcatOutput);
    connect(&m_logcatProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                consumeLogcatOutput();
                emit streamStateChanged(false);
                const QString error = QString::fromLocal8Bit(
                                          m_logcatProcess.readAllStandardError())
                                          .trimmed();
                if (m_active && !m_restartRequested
                    && (exitStatus != QProcess::NormalExit || exitCode != 0)) {
                    emit streamError(error.isEmpty()
                                         ? tr("logcat 已退出，退出码：%1").arg(exitCode)
                                         : error.left(1000));
                }
                if (m_active && m_restartRequested) {
                    m_restartRequested = false;
                    startLogcat();
                }
            });
    connect(&m_logcatProcess,
            &QProcess::errorOccurred,
            this,
            [this](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart) {
                    emit streamError(tr("无法启动 adb logcat：%1")
                                         .arg(m_logcatProcess.errorString()));
                }
            });

    connect(&m_pidProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int, QProcess::ExitStatus) { parsePidOutput(); });
}

LogcatService::~LogcatService()
{
    m_pidTimer.stop();
    for (QProcess *process : {&m_logcatProcess, &m_pidProcess}) {
        if (process->state() != QProcess::NotRunning) {
            process->kill();
            process->waitForFinished(1000);
        }
    }
}

void LogcatService::setDeviceSerial(const QString &serial)
{
    if (m_deviceSerial == serial) {
        return;
    }
    m_deviceSerial = serial;
    m_pidNames.clear();
    m_pendingOutput.clear();
    m_restartRequested = m_active && !serial.isEmpty();
    stopLogcat();
    if (m_pidProcess.state() != QProcess::NotRunning) {
        m_pidProcess.kill();
    }
    updateStreamState();
}

void LogcatService::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    m_active = active;
    updateStreamState();
}

void LogcatService::restart()
{
    if (!m_active || m_deviceSerial.isEmpty()) {
        return;
    }
    m_pendingOutput.clear();
    refreshPidNames();
    if (m_logcatProcess.state() == QProcess::NotRunning) {
        startLogcat();
        return;
    }
    m_restartRequested = true;
    m_logcatProcess.terminate();
    QTimer::singleShot(800, this, [this] {
        if (m_restartRequested && m_logcatProcess.state() != QProcess::NotRunning) {
            m_logcatProcess.kill();
        }
    });
}

void LogcatService::updateStreamState()
{
    const bool shouldRun = m_active && !m_deviceSerial.isEmpty();
    if (!shouldRun) {
        m_pidTimer.stop();
        m_restartRequested = false;
        stopLogcat();
        return;
    }
    if (!QFileInfo::exists(m_adbPath)) {
        emit streamError(tr("未找到 adb：%1").arg(QDir::toNativeSeparators(m_adbPath)));
        return;
    }
    if (!m_pidTimer.isActive()) {
        m_pidTimer.start();
    }
    refreshPidNames();
    startLogcat();
}

void LogcatService::startLogcat()
{
    if (!m_active || m_deviceSerial.isEmpty()
        || m_logcatProcess.state() != QProcess::NotRunning) {
        return;
    }
    m_pendingOutput.clear();
    m_logcatProcess.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    m_logcatProcess.start(m_adbPath,
                          {QStringLiteral("-s"),
                           m_deviceSerial,
                           QStringLiteral("logcat"),
                           QStringLiteral("-v"),
                           QStringLiteral("threadtime"),
                           QStringLiteral("-T"),
                           QStringLiteral("200")});
}

void LogcatService::stopLogcat()
{
    if (m_logcatProcess.state() == QProcess::NotRunning) {
        emit streamStateChanged(false);
        return;
    }
    m_logcatProcess.terminate();
    QTimer::singleShot(800, this, [this] {
        if (!m_active && m_logcatProcess.state() != QProcess::NotRunning) {
            m_logcatProcess.kill();
        }
    });
}

void LogcatService::refreshPidNames()
{
    if (!m_active || m_deviceSerial.isEmpty()
        || m_pidProcess.state() != QProcess::NotRunning) {
        return;
    }
    m_pidProcess.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    m_pidProcess.start(m_adbPath,
                       {QStringLiteral("-s"),
                        m_deviceSerial,
                        QStringLiteral("shell"),
                        QStringLiteral("ps -A -o PID,NAME 2>/dev/null || ps")});
}

void LogcatService::consumeLogcatOutput()
{
    m_pendingOutput += m_logcatProcess.readAllStandardOutput();
    while (true) {
        const qsizetype newline = m_pendingOutput.indexOf('\n');
        if (newline < 0) {
            break;
        }
        QByteArray rawLine = m_pendingOutput.left(newline);
        m_pendingOutput.remove(0, newline + 1);
        if (rawLine.endsWith('\r')) {
            rawLine.chop(1);
        }
        LogcatEntry entry;
        if (parseEntry(QString::fromUtf8(rawLine), &entry)) {
            emit entryReady(entry);
        }
    }
}

void LogcatService::parsePidOutput()
{
    const QString output = QString::fromUtf8(m_pidProcess.readAllStandardOutput());
    const QStringList lines = output.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                           Qt::SkipEmptyParts);
    QHash<int, QString> names;
    for (const QString &line : lines) {
        const QString simplified = line.simplified();
        if (simplified.startsWith(QStringLiteral("PID "))
            || simplified == QStringLiteral("PID NAME")) {
            continue;
        }
        const QStringList fields = simplified.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (fields.size() < 2) {
            continue;
        }
        bool ok = false;
        const int pid = fields.first().toInt(&ok);
        if (ok) {
            names.insert(pid, fields.last());
        }
    }
    if (!names.isEmpty()) {
        m_pidNames = std::move(names);
    }
}

bool LogcatService::parseEntry(const QString &line, LogcatEntry *entry) const
{
    static const QRegularExpression pattern(QStringLiteral(
        "^(\\d{2}-\\d{2})\\s+(\\d{2}:\\d{2}:\\d{2}\\.\\d{3})\\s+"
        "(\\d+)\\s+(\\d+)\\s+([VDIWEFAS])\\s+(.+?):\\s?(.*)$"));
    const QRegularExpressionMatch match = pattern.match(line);
    if (!match.hasMatch()) {
        return false;
    }
    entry->time = QStringLiteral("%1-%2 %3")
                      .arg(QString::number(QDate::currentDate().year()),
                           match.captured(1),
                           match.captured(2));
    entry->pid = match.captured(3).toInt();
    entry->tid = match.captured(4).toInt();
    entry->priority = match.captured(5).at(0);
    entry->tag = match.captured(6).trimmed();
    entry->processName = m_pidNames.value(entry->pid,
                                          QStringLiteral("pid-%1").arg(entry->pid));
    entry->message = match.captured(7);
    return true;
}
