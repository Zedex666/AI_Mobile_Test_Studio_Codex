#include "services/performance_service.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

const QString kUptimeMarker = QStringLiteral("__PERF_UPTIME__");
const QString kCpuBaseMarker = QStringLiteral("__PERF_CPU_BASE__");
const QString kCpuNextMarker = QStringLiteral("__PERF_CPU_NEXT__");
const QString kFrequencyMarker = QStringLiteral("__PERF_FREQ__");
const QString kMemoryMarker = QStringLiteral("__PERF_MEM__");
const QString kBatteryMarker = QStringLiteral("__PERF_BATTERY__");
const QString kThermalMarker = QStringLiteral("__PERF_THERMAL__");
const QString kTopMarker = QStringLiteral("__PERF_TOP__");
const QString kFlipsMarker = QStringLiteral("__PERF_FLIPS__");
const QString kLayersMarker = QStringLiteral("__PERF_LAYERS__");
const QString kLatencyPrefix = QStringLiteral("__PERF_LATENCY_");

QString shellQuote(QString value)
{
    value.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + value + QLatin1Char('\'');
}

QString sectionCommand(const QVector<QString> &layers)
{
    QString command = QStringLiteral(
        "printf '__PERF_UPTIME__\\n'; cat /proc/uptime; "
        "printf '__PERF_CPU_BASE__\\n'; cat /proc/stat; "
        "printf '__PERF_FREQ__\\n'; "
        "for cpu in /sys/devices/system/cpu/cpu[0-9]*; do "
        "idx=${cpu##*cpu}; "
        "freq=$(cat \"$cpu/cpufreq/scaling_cur_freq\" 2>/dev/null); "
        "[ -z \"$freq\" ] && freq=$(cat \"$cpu/cpufreq/cpuinfo_cur_freq\" 2>/dev/null); "
        "printf '%s:%s\\n' \"$idx\" \"$freq\"; done; "
        "sleep 0.05; printf '__PERF_CPU_NEXT__\\n'; cat /proc/stat; "
        "printf '__PERF_MEM__\\n'; cat /proc/meminfo; "
        "printf '__PERF_BATTERY__\\n'; dumpsys battery 2>/dev/null; "
        "printf '__PERF_THERMAL__\\n'; dumpsys thermalservice 2>/dev/null; "
        "printf '__PERF_TOP__\\n'; "
        "dumpsys activity activities 2>/dev/null | "
        "grep -e 'top-activity' -e 'ResumedActivity:' | head -n 4; "
        "printf '__PERF_FLIPS__\\n'; "
        "dumpsys SurfaceFlinger 2>/dev/null | grep 'flips=' | head -n 1; "
        "printf '__PERF_LAYERS__\\n'; dumpsys SurfaceFlinger --list 2>/dev/null;");

    const int layerCount = std::min(static_cast<int>(layers.size()), 12);
    for (int index = 0; index < layerCount; ++index) {
        command += QStringLiteral(" printf '__PERF_LATENCY_%1__\\n'; ").arg(index);
        command += QStringLiteral("dumpsys SurfaceFlinger --latency %1 2>/dev/null;")
                       .arg(shellQuote(layers[index]));
    }
    return command;
}

QHash<QString, QStringList> splitSections(const QString &output)
{
    QHash<QString, QStringList> sections;
    QString current;
    const QStringList lines = output.split(QLatin1Char('\n'));
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.startsWith(QStringLiteral("__PERF_")) && line.endsWith(QStringLiteral("__"))) {
            current = line;
        } else if (!current.isEmpty() && !line.isEmpty()) {
            sections[current].append(line);
        }
    }
    return sections;
}

qint64 memoryValue(const QStringList &lines, const QString &name)
{
    const QString prefix = name + QLatin1Char(':');
    for (const QString &line : lines) {
        if (!line.startsWith(prefix)) {
            continue;
        }
        const QString value = line.mid(prefix.size()).trimmed().section(QLatin1Char(' '), 0, 0);
        bool ok = false;
        const qint64 kilobytes = value.toLongLong(&ok);
        return ok ? kilobytes : 0;
    }
    return 0;
}

double propertyValue(const QStringList &lines, const QString &name, bool *found = nullptr)
{
    const QString prefix = name + QLatin1Char(':');
    for (const QString &line : lines) {
        if (!line.startsWith(prefix)) {
            continue;
        }
        bool ok = false;
        const double value = line.mid(prefix.size()).trimmed().toDouble(&ok);
        if (ok) {
            if (found != nullptr) {
                *found = true;
            }
            return value;
        }
    }
    if (found != nullptr) {
        *found = false;
    }
    return 0.0;
}

QHash<int, PerformanceService::CpuTimes> cpuTimesByIndex(const QStringList &lines)
{
    QHash<int, PerformanceService::CpuTimes> result;
    static const QRegularExpression pattern(QStringLiteral("^cpu(\\d+)\\s+(.+)$"));
    for (const QString &line : lines) {
        const QRegularExpressionMatch match = pattern.match(line);
        if (!match.hasMatch()) {
            continue;
        }
        bool indexOk = false;
        const int index = match.captured(1).toInt(&indexOk);
        if (!indexOk) {
            continue;
        }
        const QStringList fields = match.captured(2).split(
            QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        result.insert(index, PerformanceService::parseCpuTimes(fields));
    }
    return result;
}

QString foregroundPackage(const QStringList &lines)
{
    static const QVector<QRegularExpression> patterns = {
        QRegularExpression(QStringLiteral("top-activity.*?\\s(\\d+):([A-Za-z0-9._]+)/")),
        QRegularExpression(QStringLiteral("(?:ResumedActivity|mResumedActivity):.*?\\su\\d+\\s+([A-Za-z0-9._]+)/")),
        QRegularExpression(QStringLiteral("(?:ResumedActivity|mResumedActivity):.*?\\s([A-Za-z0-9._]+)/"))};
    for (const QString &line : lines) {
        for (int index = 0; index < patterns.size(); ++index) {
            const QRegularExpressionMatch match = patterns[index].match(line);
            if (match.hasMatch()) {
                return match.captured(index == 0 ? 2 : 1);
            }
        }
    }
    return QString();
}

QString foregroundLabel(const QString &packageName)
{
    static const QStringList launchers = {
        QStringLiteral("com.miui.home"),
        QStringLiteral("com.android.launcher"),
        QStringLiteral("com.android.launcher3"),
        QStringLiteral("com.google.android.apps.nexuslauncher"),
        QStringLiteral("com.sec.android.app.launcher"),
        QStringLiteral("com.huawei.android.launcher"),
        QStringLiteral("com.oppo.launcher"),
        QStringLiteral("com.vivo.launcher")};
    for (const QString &launcher : launchers) {
        if (packageName == launcher || packageName.startsWith(launcher + QLatin1Char('.'))) {
            return QStringLiteral("系统桌面");
        }
    }
    return packageName.isEmpty() ? QStringLiteral("系统桌面") : packageName;
}

QString normalizedLayer(QString layer)
{
    if (!layer.startsWith(QStringLiteral("RequestedLayerState{"))) {
        return layer;
    }
    layer.remove(0, QStringLiteral("RequestedLayerState{").size());
    const QStringList keys = {QStringLiteral("parentId="),
                              QStringLiteral("relativeParentId="),
                              QStringLiteral("mirrorId="),
                              QStringLiteral("z="),
                              QStringLiteral("layerStack=")};
    int firstKey = -1;
    for (const QString &key : keys) {
        const int position = layer.indexOf(key);
        if (position >= 0 && (firstKey < 0 || position < firstKey)) {
            firstKey = position;
        }
    }
    if (firstKey > 0) {
        layer = layer.left(firstKey).trimmed();
    }
    if (layer.endsWith(QLatin1Char('}'))) {
        layer.chop(1);
    }
    return layer.trimmed();
}

bool usableLayer(const QString &layer, const QString &packageName)
{
    if (packageName.isEmpty() || !layer.contains(packageName)) {
        return false;
    }
    static const QStringList excluded = {QStringLiteral("ActivityRecordInputSink"),
                                         QStringLiteral("Background for SurfaceView"),
                                         QStringLiteral("ActivityRecord"),
                                         QStringLiteral("Bounds for -")};
    for (const QString &content : excluded) {
        if (layer.contains(content)) {
            return false;
        }
    }
    return true;
}

} // namespace

PerformanceService::PerformanceService(QString adbPath, QObject *parent)
    : QObject(parent)
    , m_adbPath(QDir::cleanPath(std::move(adbPath)))
    , m_process(this)
    , m_timer(this)
{
    m_timer.setInterval(1000);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &PerformanceService::sampleNow);

    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] {
        m_output += QString::fromLocal8Bit(m_process.readAllStandardOutput());
    });
    connect(&m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &PerformanceService::handleFinished);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            emit samplingError(tr("adb 启动失败：%1").arg(m_process.errorString()));
        }
    });
}

PerformanceService::~PerformanceService()
{
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}

void PerformanceService::setDeviceSerial(const QString &serial)
{
    if (m_deviceSerial == serial) {
        return;
    }
    m_deviceSerial = serial;
    m_foregroundPackage.clear();
    m_latencyPackage.clear();
    m_foregroundLayers.clear();
    m_lastFlips = 0;
    m_lastFlipsTimeMs = 0;
    if (m_deviceSerial.isEmpty() && m_process.state() != QProcess::NotRunning) {
        m_process.kill();
    }
    updateSamplingState();
}

void PerformanceService::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    m_active = active;
    updateSamplingState();
}

void PerformanceService::sampleNow()
{
    if (!m_active || m_deviceSerial.isEmpty() || m_process.state() != QProcess::NotRunning) {
        return;
    }
    if (!QFileInfo::exists(m_adbPath)) {
        m_timer.stop();
        emit samplingError(tr("未找到 adb：%1").arg(QDir::toNativeSeparators(m_adbPath)));
        return;
    }

    m_latencyPackage = m_foregroundPackage;
    m_output.clear();
    m_process.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    m_process.start(m_adbPath,
                    {QStringLiteral("-s"),
                     m_deviceSerial,
                     QStringLiteral("shell"),
                     sectionCommand(m_foregroundLayers)});
}

void PerformanceService::updateSamplingState()
{
    const bool shouldRun = m_active && !m_deviceSerial.isEmpty();
    if (!shouldRun) {
        m_timer.stop();
        return;
    }
    if (!m_timer.isActive()) {
        m_timer.start();
        sampleNow();
    }
}

void PerformanceService::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_output += QString::fromLocal8Bit(m_process.readAllStandardOutput());
    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        const QString detail = m_output.trimmed();
        emit samplingError(detail.isEmpty()
                               ? tr("性能数据采集失败，退出码：%1").arg(exitCode)
                               : detail);
        return;
    }
    emit sampleReady(parseSample(m_output));
}

PerformanceSample PerformanceService::parseSample(const QString &output)
{
    const QHash<QString, QStringList> sections = splitSections(output);
    PerformanceSample sample;
    sample.sampleTimeMs = QDateTime::currentMSecsSinceEpoch();

    const QStringList uptimeLines = sections.value(kUptimeMarker);
    if (!uptimeLines.isEmpty()) {
        sample.uptimeSeconds = uptimeLines.first().section(QLatin1Char(' '), 0, 0).toDouble();
    }

    QHash<int, int> frequencies;
    for (const QString &line : sections.value(kFrequencyMarker)) {
        const QStringList parts = line.split(QLatin1Char(':'));
        if (parts.size() != 2) {
            continue;
        }
        bool indexOk = false;
        bool frequencyOk = false;
        const int index = parts[0].toInt(&indexOk);
        const int frequencyKhz = parts[1].toInt(&frequencyOk);
        if (indexOk && frequencyOk) {
            frequencies.insert(index, qFloor(frequencyKhz / 1000.0));
        }
    }

    const QHash<int, CpuTimes> baseTimes = cpuTimesByIndex(sections.value(kCpuBaseMarker));
    const QHash<int, CpuTimes> nextTimes = cpuTimesByIndex(sections.value(kCpuNextMarker));
    QList<int> cpuIndexes = nextTimes.keys();
    std::sort(cpuIndexes.begin(), cpuIndexes.end());
    double usageSum = 0.0;
    for (int index : std::as_const(cpuIndexes)) {
        if (!baseTimes.contains(index)) {
            continue;
        }
        const double usage = cpuUsage(baseTimes.value(index), nextTimes.value(index));
        sample.cores.append({index, usage, frequencies.value(index)});
        usageSum += usage;
    }
    if (!sample.cores.isEmpty()) {
        sample.cpuUsagePercent = usageSum / sample.cores.size();
    }

    const QStringList memoryLines = sections.value(kMemoryMarker);
    const qint64 totalKb = memoryValue(memoryLines, QStringLiteral("MemTotal"));
    qint64 freeKb = memoryValue(memoryLines, QStringLiteral("MemAvailable"));
    if (freeKb <= 0) {
        freeKb = memoryValue(memoryLines, QStringLiteral("MemFree"));
    }
    sample.memoryTotalMb = qRound64(totalKb / 1024.0);
    sample.memoryUsedMb = qRound64(std::max<qint64>(0, totalKb - freeKb) / 1024.0);

    const QStringList batteryLines = sections.value(kBatteryMarker);
    bool batteryFound = false;
    const double batteryLevel = propertyValue(batteryLines, QStringLiteral("level"), &batteryFound);
    if (batteryFound) {
        sample.batteryPercent = std::clamp(qRound(batteryLevel), 0, 100);
    }
    sample.batteryTemperatureC = propertyValue(batteryLines, QStringLiteral("temperature")) / 10.0;
    sample.batteryVoltageV = propertyValue(batteryLines, QStringLiteral("voltage")) / 1000.0;

    static const QRegularExpression temperaturePattern(
        QStringLiteral("Temperature\\{mValue=([\\d.]+),\\s*mType=(\\d+),\\s*mName=([^,}]+)"));
    double temperatureSum = 0.0;
    int temperatureCount = 0;
    for (const QString &line : sections.value(kThermalMarker)) {
        const QRegularExpressionMatch match = temperaturePattern.match(line);
        if (!match.hasMatch() || !match.captured(3).startsWith(QStringLiteral("CPU"),
                                                               Qt::CaseInsensitive)) {
            continue;
        }
        temperatureSum += match.captured(1).toDouble();
        ++temperatureCount;
    }
    if (temperatureCount > 0) {
        sample.cpuTemperatureC = temperatureSum / temperatureCount;
    }

    sample.foregroundPackage = foregroundPackage(sections.value(kTopMarker));
    sample.foregroundLabel = foregroundLabel(sample.foregroundPackage);
    sample.fps = parseFps(sections, sample.foregroundPackage, sample.sampleTimeMs);

    m_foregroundPackage = sample.foregroundPackage;
    m_foregroundLayers.clear();
    for (const QString &rawLayer : sections.value(kLayersMarker)) {
        if (!usableLayer(rawLayer, m_foregroundPackage)) {
            continue;
        }
        const QString layer = normalizedLayer(rawLayer);
        if (!layer.isEmpty() && !m_foregroundLayers.contains(layer)) {
            m_foregroundLayers.append(layer);
        }
    }
    return sample;
}

PerformanceService::CpuTimes PerformanceService::parseCpuTimes(const QStringList &fields)
{
    QVector<quint64> values;
    values.reserve(fields.size());
    for (const QString &field : fields) {
        bool ok = false;
        const quint64 value = field.toULongLong(&ok);
        if (!ok) {
            break;
        }
        values.append(value);
    }

    CpuTimes result;
    if (values.size() < 7) {
        return result;
    }
    result.load = values[0] + values[1] + values[2] + values[4] + values[5] + values[6];
    result.tick = result.load + values[3];
    return result;
}

double PerformanceService::cpuUsage(const CpuTimes &previous, const CpuTimes &current)
{
    if (current.tick <= previous.tick || current.load < previous.load) {
        return 0.0;
    }
    const quint64 tickDelta = current.tick - previous.tick;
    const quint64 loadDelta = current.load - previous.load;
    return std::clamp(100.0 * static_cast<double>(loadDelta) / tickDelta, 0.0, 100.0);
}

double PerformanceService::parseFps(const QHash<QString, QStringList> &sections,
                                    const QString &foregroundPackage,
                                    qint64 sampleTimeMs)
{
    static const QRegularExpression flipsPattern(QStringLiteral("flips=(\\d+)"));
    for (const QString &line : sections.value(kFlipsMarker)) {
        const QRegularExpressionMatch match = flipsPattern.match(line);
        if (!match.hasMatch()) {
            continue;
        }
        const quint64 flips = match.captured(1).toULongLong();
        double fps = 0.0;
        if (m_lastFlipsTimeMs > 0 && sampleTimeMs > m_lastFlipsTimeMs && flips >= m_lastFlips) {
            fps = qRound((flips - m_lastFlips) * 1000.0
                         / (sampleTimeMs - m_lastFlipsTimeMs));
        }
        m_lastFlips = flips;
        m_lastFlipsTimeMs = sampleTimeMs;
        return std::clamp(fps, 0.0, 240.0);
    }

    if (foregroundPackage.isEmpty() || m_latencyPackage != foregroundPackage) {
        return 0.0;
    }

    double maximumFps = 0.0;
    const QStringList keys = sections.keys();
    for (const QString &key : keys) {
        if (!key.startsWith(kLatencyPrefix)) {
            continue;
        }
        QVector<double> timestamps;
        for (const QString &line : sections.value(key)) {
            const QStringList fields = line.split(QRegularExpression(QStringLiteral("\\s+")),
                                                  Qt::SkipEmptyParts);
            if (fields.size() < 3) {
                continue;
            }
            bool ok = false;
            const double timestamp = fields[1].toDouble(&ok) / 1e9;
            if (ok) {
                timestamps.append(timestamp);
            }
        }
        if (!timestamps.isEmpty()) {
            timestamps.removeLast();
        }
        if (timestamps.size() <= 1) {
            continue;
        }
        const double seconds = timestamps.last() - timestamps.first();
        if (seconds > 0.0) {
            maximumFps = std::max(maximumFps, (timestamps.size() - 1) / seconds);
        }
    }
    return std::clamp(static_cast<double>(qRound(maximumFps)), 0.0, 240.0);
}
