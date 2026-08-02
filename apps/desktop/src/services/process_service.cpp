#include "services/process_service.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

QString normalizedColumn(QString value)
{
    value.remove(QLatin1Char('['));
    value.remove(QLatin1Char(']'));
    value = value.toLower();
    if (value == QStringLiteral("cpu%")) {
        return QStringLiteral("%cpu");
    }
    if (value == QStringLiteral("rss")) {
        return QStringLiteral("res");
    }
    if (value == QStringLiteral("uid")) {
        return QStringLiteral("user");
    }
    if (value == QStringLiteral("cmdline") || value == QStringLiteral("command")
        || value == QStringLiteral("cmd")) {
        return QStringLiteral("args");
    }
    return value;
}

QString packageCandidate(const QString &value)
{
    static const QRegularExpression pattern(
        QStringLiteral("^([A-Za-z0-9_]+(?:\\.[A-Za-z0-9_]+)+)"));
    const QRegularExpressionMatch match = pattern.match(value.trimmed());
    return match.hasMatch() ? match.captured(1) : QString();
}

} // namespace

ProcessService::ProcessService(QString adbPath, QObject *parent)
    : QObject(parent)
    , m_adbPath(QDir::cleanPath(std::move(adbPath)))
    , m_queryProcess(this)
    , m_actionProcess(this)
    , m_timer(this)
{
    m_timer.setInterval(8000);
    connect(&m_timer, &QTimer::timeout, this, &ProcessService::refresh);

    m_queryProcess.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_queryProcess, &QProcess::readyReadStandardOutput, this, [this] {
        m_output += m_queryProcess.readAllStandardOutput();
    });
    connect(&m_queryProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &ProcessService::handleQueryFinished);
    connect(&m_queryProcess,
            &QProcess::errorOccurred,
            this,
            [this](QProcess::ProcessError error) {
                if (m_switchingDevice || error != QProcess::FailedToStart) {
                    return;
                }
                m_stage = QueryStage::None;
                m_preloading = false;
                emit samplingChanged(false);
                emit processError(tr("adb 启动失败：%1")
                                      .arg(m_queryProcess.errorString()));
            });

    m_actionProcess.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_actionProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                if (m_switchingDevice) {
                    m_actionProcess.readAllStandardOutput();
                    return;
                }
                const QString packageName = m_actionProcess.property("packageName").toString();
                const QString output = QString::fromUtf8(
                    m_actionProcess.readAllStandardOutput()).trimmed();
                const bool success = exitStatus == QProcess::NormalExit && exitCode == 0;
                emit stopFinished(success,
                                  packageName,
                                  output.isEmpty()
                                      ? (success ? tr("应用已停止。")
                                                 : tr("命令退出码：%1").arg(exitCode))
                                      : output.left(1000));
                if (success) {
                    refresh();
                }
            });
    connect(&m_actionProcess,
            &QProcess::errorOccurred,
            this,
            [this](QProcess::ProcessError error) {
                if (!m_switchingDevice && error == QProcess::FailedToStart) {
                    emit stopFinished(false,
                                      m_actionProcess.property("packageName").toString(),
                                      tr("adb 启动失败：%1")
                                          .arg(m_actionProcess.errorString()));
                }
            });
}

ProcessService::~ProcessService()
{
    for (QProcess *process : {&m_queryProcess, &m_actionProcess}) {
        if (process->state() != QProcess::NotRunning) {
            process->kill();
            process->waitForFinished(1000);
        }
    }
}

void ProcessService::setDeviceSerial(const QString &serial)
{
    if (m_deviceSerial == serial) {
        return;
    }
    saveActiveCache();
    m_switchingDevice = true;
    m_timer.stop();
    for (QProcess *process : {&m_queryProcess, &m_actionProcess}) {
        if (process->state() != QProcess::NotRunning) {
            process->kill();
            process->waitForFinished(1000);
        }
    }
    m_switchingDevice = false;
    m_deviceSerial = serial;
    m_packages.clear();
    m_cachedProcesses.clear();
    m_hasCachedSnapshot = false;
    m_preloading = false;
    m_refreshPending = false;
    restoreActiveCache();
    updateSamplingState();
}

void ProcessService::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    m_active = active;
    updateSamplingState();
}

void ProcessService::preload()
{
    if (m_deviceSerial.isEmpty()) {
        return;
    }
    if (m_hasCachedSnapshot) {
        emit processesReady(m_cachedProcesses);
        return;
    }
    if (m_queryProcess.state() != QProcess::NotRunning) {
        return;
    }
    m_preloading = true;
    refresh();
}

void ProcessService::refresh()
{
    if ((!m_active && !m_preloading) || m_deviceSerial.isEmpty()) {
        return;
    }
    if (!QFileInfo::exists(m_adbPath)) {
        m_preloading = false;
        m_stage = QueryStage::None;
        emit samplingChanged(false);
        emit processError(tr("未找到 adb：%1")
                              .arg(QDir::toNativeSeparators(m_adbPath)));
        return;
    }
    if (m_queryProcess.state() != QProcess::NotRunning) {
        m_refreshPending = true;
        return;
    }

    emit samplingChanged(true);
    if (m_packages.isEmpty()) {
        startPackagesQuery();
    } else {
        startTopQuery(false);
    }
}

void ProcessService::stopPackage(const QString &packageName)
{
    if (m_deviceSerial.isEmpty() || packageName.trimmed().isEmpty()) {
        emit stopFinished(false, packageName, tr("没有可停止的应用。"));
        return;
    }
    if (m_actionProcess.state() != QProcess::NotRunning) {
        emit stopFinished(false, packageName, tr("另一项停止操作正在执行。"));
        return;
    }
    m_actionProcess.setProperty("packageName", packageName);
    m_actionProcess.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    m_actionProcess.start(m_adbPath,
                          {QStringLiteral("-s"),
                           m_deviceSerial,
                           QStringLiteral("shell"),
                           QStringLiteral("am"),
                           QStringLiteral("force-stop"),
                           packageName});
}

void ProcessService::updateSamplingState()
{
    const bool shouldSample = m_active && !m_deviceSerial.isEmpty();
    if (shouldSample) {
        if (!m_timer.isActive()) {
            m_timer.start();
        }
        if (m_hasCachedSnapshot) {
            emit processesReady(m_cachedProcesses);
        }
        refresh();
        return;
    }

    m_timer.stop();
    m_refreshPending = false;
    if (!m_preloading && m_queryProcess.state() != QProcess::NotRunning) {
        m_queryProcess.kill();
    }
    if (!m_preloading) {
        m_stage = QueryStage::None;
        emit samplingChanged(false);
    }
}

void ProcessService::startPackagesQuery()
{
    m_stage = QueryStage::Packages;
    m_output.clear();
    m_queryProcess.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    m_queryProcess.start(m_adbPath,
                         {QStringLiteral("-s"),
                          m_deviceSerial,
                          QStringLiteral("shell"),
                          QStringLiteral("pm"),
                          QStringLiteral("list"),
                          QStringLiteral("packages")});
}

void ProcessService::startTopQuery(bool legacy)
{
    m_stage = legacy ? QueryStage::LegacyTop : QueryStage::ModernTop;
    m_output.clear();
    m_queryProcess.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    const QString command = legacy
        ? QStringLiteral("top -b -n 1")
        : QStringLiteral("top -b -n 1 -o PID,%CPU,TIME+,RES,USER,NAME,ARGS");
    m_queryProcess.start(m_adbPath,
                         {QStringLiteral("-s"),
                          m_deviceSerial,
                          QStringLiteral("shell"),
                          command});
}

void ProcessService::handleQueryFinished(int exitCode,
                                         QProcess::ExitStatus exitStatus)
{
    if (m_switchingDevice) {
        m_queryProcess.readAllStandardOutput();
        return;
    }
    m_output += m_queryProcess.readAllStandardOutput();
    const QString output = QString::fromUtf8(m_output);
    const bool success = exitStatus == QProcess::NormalExit && exitCode == 0;

    if (m_stage == QueryStage::None) {
        return;
    }

    if (m_stage == QueryStage::Packages) {
        if (success) {
            const QStringList lines = output.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                                   Qt::SkipEmptyParts);
            for (QString line : lines) {
                line = line.trimmed();
                if (line.startsWith(QStringLiteral("package:"))) {
                    m_packages.insert(line.mid(8));
                }
            }
        }
        if ((m_active || m_preloading) && !m_deviceSerial.isEmpty()) {
            startTopQuery(false);
        } else {
            m_stage = QueryStage::None;
            m_preloading = false;
            emit samplingChanged(false);
        }
        return;
    }

    if (!success) {
        if (m_stage == QueryStage::ModernTop && (m_active || m_preloading)) {
            startTopQuery(true);
            return;
        }
        m_stage = QueryStage::None;
        m_preloading = false;
        emit samplingChanged(false);
        emit processError(output.trimmed().isEmpty()
                              ? tr("进程采样失败，退出码：%1").arg(exitCode)
                              : output.trimmed().left(1000));
    } else {
        const QVector<DeviceProcessEntry> processes = parseProcesses(output);
        if (processes.isEmpty() && m_stage == QueryStage::ModernTop
            && (m_active || m_preloading)) {
            startTopQuery(true);
            return;
        }
        m_stage = QueryStage::None;
        m_preloading = false;
        m_cachedProcesses = processes;
        m_hasCachedSnapshot = true;
        saveActiveCache();
        emit samplingChanged(false);
        emit processesReady(m_cachedProcesses);
    }

    if (m_refreshPending && m_active && !m_deviceSerial.isEmpty()) {
        m_refreshPending = false;
        refresh();
    }
}

void ProcessService::saveActiveCache()
{
    if (m_deviceSerial.isEmpty()) {
        return;
    }
    DeviceCache &cache = m_deviceCaches[m_deviceSerial];
    cache.packages = m_packages;
    cache.processes = m_cachedProcesses;
    cache.hasSnapshot = m_hasCachedSnapshot;
}

void ProcessService::restoreActiveCache()
{
    if (m_deviceSerial.isEmpty()) {
        return;
    }
    const DeviceCache cache = m_deviceCaches.value(m_deviceSerial);
    m_packages = cache.packages;
    m_cachedProcesses = cache.processes;
    m_hasCachedSnapshot = cache.hasSnapshot;
}

QVector<DeviceProcessEntry> ProcessService::parseProcesses(const QString &output) const
{
    const QStringList lines = output.split(QLatin1Char('\n'));
    QStringList columns;
    int start = -1;
    for (int index = 0; index < lines.size(); ++index) {
        const QString line = lines[index].trimmed();
        if (!line.startsWith(QStringLiteral("PID"))) {
            continue;
        }
        columns = line.split(QRegularExpression(QStringLiteral("\\s+")),
                             Qt::SkipEmptyParts);
        for (QString &column : columns) {
            column = normalizedColumn(column);
        }
        start = index + 1;
        break;
    }
    if (start < 0) {
        return {};
    }

    const int pidIndex = columns.indexOf(QStringLiteral("pid"));
    const int cpuIndex = columns.indexOf(QStringLiteral("%cpu"));
    const int timeIndex = columns.indexOf(QStringLiteral("time+"));
    const int memoryIndex = columns.indexOf(QStringLiteral("res"));
    const int userIndex = columns.indexOf(QStringLiteral("user"));
    const int nameIndex = columns.indexOf(QStringLiteral("name"));
    const int argsIndex = columns.indexOf(QStringLiteral("args"));
    if (pidIndex < 0) {
        return {};
    }

    QVector<DeviceProcessEntry> result;
    result.reserve(std::max<qsizetype>(0, lines.size() - start));
    for (int lineIndex = start; lineIndex < lines.size(); ++lineIndex) {
        const QString line = lines[lineIndex].trimmed();
        if (line.isEmpty()) {
            continue;
        }
        const QStringList parts = line.split(QRegularExpression(QStringLiteral("\\s+")),
                                             Qt::SkipEmptyParts);
        if (pidIndex >= parts.size()) {
            continue;
        }
        bool pidOk = false;
        const int pid = parts[pidIndex].toInt(&pidOk);
        if (!pidOk || pid <= 0) {
            continue;
        }

        auto valueAt = [&parts](int index) {
            return index >= 0 && index < parts.size() ? parts[index] : QString();
        };
        DeviceProcessEntry entry;
        entry.pid = pid;
        entry.cpuPercent = valueAt(cpuIndex).remove(QLatin1Char('%')).toDouble();
        entry.cpuTime = valueAt(timeIndex);
        entry.memory = valueAt(memoryIndex);
        entry.memoryBytes = parseMemoryBytes(entry.memory);
        entry.user = valueAt(userIndex);
        entry.name = valueAt(nameIndex);
        if (argsIndex >= 0 && argsIndex < parts.size()) {
            entry.arguments = parts.mid(argsIndex).join(QLatin1Char(' '));
        }
        if (entry.name.isEmpty()) {
            entry.name = entry.arguments.section(QLatin1Char(' '), 0, 0);
        }
        if (entry.arguments.isEmpty()) {
            entry.arguments = entry.name;
        }
        if (entry.arguments.contains(QStringLiteral("top -b -n 1"))) {
            continue;
        }

        QString candidate = packageCandidate(entry.name);
        if (candidate.isEmpty() || !m_packages.contains(candidate)) {
            candidate = packageCandidate(entry.arguments);
        }
        if (!candidate.isEmpty() && m_packages.contains(candidate)) {
            entry.packageName = candidate;
            entry.isApplication = true;
        }
        result.append(entry);
    }

    std::sort(result.begin(), result.end(), [](const DeviceProcessEntry &left,
                                               const DeviceProcessEntry &right) {
        if (!qFuzzyCompare(left.cpuPercent + 1.0, right.cpuPercent + 1.0)) {
            return left.cpuPercent > right.cpuPercent;
        }
        return left.memoryBytes > right.memoryBytes;
    });
    return result;
}

qint64 ProcessService::parseMemoryBytes(const QString &value)
{
    static const QRegularExpression pattern(
        QStringLiteral("^([0-9]+(?:\\.[0-9]+)?)([KMGTP]?)$"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = pattern.match(value.trimmed());
    if (!match.hasMatch()) {
        return 0;
    }
    bool ok = false;
    double amount = match.captured(1).toDouble(&ok);
    if (!ok) {
        return 0;
    }
    const QString suffix = match.captured(2).toUpper();
    const QStringList units = {QString(),
                               QStringLiteral("K"),
                               QStringLiteral("M"),
                               QStringLiteral("G"),
                               QStringLiteral("T"),
                               QStringLiteral("P")};
    const int power = units.indexOf(suffix);
    for (int index = 0; index < std::max(0, power); ++index) {
        amount *= 1024.0;
    }
    return static_cast<qint64>(std::llround(amount));
}
