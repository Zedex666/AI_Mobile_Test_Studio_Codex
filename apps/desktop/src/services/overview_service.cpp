#include "services/overview_service.h"

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QStringDecoder>

#include <algorithm>
#include <utility>

namespace {

const QString kPropertiesMarker = QStringLiteral("__OVERVIEW_PROPERTIES__");
const QString kKernelMarker = QStringLiteral("__OVERVIEW_KERNEL__");
const QString kCpuMarker = QStringLiteral("__OVERVIEW_CPU__");
const QString kStorageMarker = QStringLiteral("__OVERVIEW_STORAGE__");
const QString kMemoryMarker = QStringLiteral("__OVERVIEW_MEMORY__");
const QString kSizeMarker = QStringLiteral("__OVERVIEW_SIZE__");
const QString kDensityMarker = QStringLiteral("__OVERVIEW_DENSITY__");
const QString kFontMarker = QStringLiteral("__OVERVIEW_FONT__");
const QString kWifiMarker = QStringLiteral("__OVERVIEW_WIFI__");
const QString kNetworkMarker = QStringLiteral("__OVERVIEW_NETWORK__");
const QString kBatteryMarker = QStringLiteral("__OVERVIEW_BATTERY__");
const QString kUptimeMarker = QStringLiteral("__OVERVIEW_UPTIME__");

QString overviewCommand()
{
    return QStringLiteral(
        "printf '__OVERVIEW_PROPERTIES__\\n'; getprop; "
        "printf '__OVERVIEW_KERNEL__\\n'; uname -r; "
        "printf '__OVERVIEW_CPU__\\n'; "
        "grep -c '^processor' /proc/cpuinfo 2>/dev/null; "
        "printf '__OVERVIEW_STORAGE__\\n'; df -k /data 2>/dev/null; "
        "printf '__OVERVIEW_MEMORY__\\n'; cat /proc/meminfo 2>/dev/null; "
        "printf '__OVERVIEW_SIZE__\\n'; wm size 2>/dev/null; "
        "printf '__OVERVIEW_DENSITY__\\n'; wm density 2>/dev/null; "
        "printf '__OVERVIEW_FONT__\\n'; settings get system font_scale 2>/dev/null; "
        "printf '__OVERVIEW_WIFI__\\n'; cmd wifi status 2>/dev/null; "
        "dumpsys wifi 2>/dev/null | grep -m 1 'mWifiInfo'; "
        "printf '__OVERVIEW_NETWORK__\\n'; ip addr show wlan0 2>/dev/null; "
        "printf '__OVERVIEW_BATTERY__\\n'; dumpsys battery 2>/dev/null; "
        "printf '__OVERVIEW_UPTIME__\\n'; cat /proc/uptime 2>/dev/null;");
}

QString decodeAdbText(const QByteArray &data)
{
    QStringDecoder decoder(QStringDecoder::Utf8);
    const QString utf8 = decoder.decode(data);
    return decoder.hasError() ? QString::fromLocal8Bit(data) : utf8;
}

QHash<QString, QStringList> splitSections(const QString &output)
{
    QHash<QString, QStringList> sections;
    QString current;
    const QStringList lines = output.split(QLatin1Char('\n'));
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.startsWith(QStringLiteral("__OVERVIEW_"))
            && line.endsWith(QStringLiteral("__"))) {
            current = line;
        } else if (!current.isEmpty() && !line.isEmpty()) {
            sections[current].append(line);
        }
    }
    return sections;
}

QHash<QString, QString> parseProperties(const QStringList &lines)
{
    QHash<QString, QString> properties;
    static const QRegularExpression pattern(
        QStringLiteral("^\\[([^]]+)\\]:\\s*\\[(.*)\\]$"));
    for (const QString &line : lines) {
        const QRegularExpressionMatch match = pattern.match(line);
        if (match.hasMatch()) {
            properties.insert(match.captured(1), match.captured(2));
        }
    }
    return properties;
}

QString firstProperty(const QHash<QString, QString> &properties,
                      const QStringList &keys)
{
    for (const QString &key : keys) {
        const QString value = properties.value(key).trimmed();
        if (!value.isEmpty()) {
            return value;
        }
    }
    return QString();
}

qint64 memoryKilobytes(const QStringList &lines, const QString &key)
{
    const QString prefix = key + QLatin1Char(':');
    for (const QString &line : lines) {
        if (!line.startsWith(prefix)) {
            continue;
        }
        bool ok = false;
        const qint64 value = line.mid(prefix.size())
                                 .trimmed()
                                 .section(QLatin1Char(' '), 0, 0)
                                 .toLongLong(&ok);
        return ok ? value : 0;
    }
    return 0;
}

int integerField(const QStringList &lines, const QString &key, int fallback = -1)
{
    const QString prefix = key + QLatin1Char(':');
    for (const QString &line : lines) {
        if (!line.startsWith(prefix, Qt::CaseInsensitive)) {
            continue;
        }
        bool ok = false;
        const int value = line.mid(prefix.size()).trimmed().toInt(&ok);
        return ok ? value : fallback;
    }
    return fallback;
}

QString batteryHealthName(int health)
{
    switch (health) {
    case 2:
        return QObject::tr("良好");
    case 3:
        return QObject::tr("过热");
    case 4:
        return QObject::tr("故障");
    case 5:
        return QObject::tr("过压");
    case 6:
        return QObject::tr("异常");
    case 7:
        return QObject::tr("过冷");
    default:
        return QObject::tr("未知");
    }
}

QString screenValue(const QStringList &lines, const QString &prefix)
{
    for (const QString &line : lines) {
        if (line.startsWith(prefix, Qt::CaseInsensitive)) {
            return line.section(QLatin1Char(':'), 1).trimmed();
        }
    }
    return QString();
}

QString wifiName(const QStringList &lines)
{
    static const QVector<QRegularExpression> patterns = {
        QRegularExpression(QStringLiteral("SSID:\\s*\\\"?([^,\\\"}]+)")),
        QRegularExpression(QStringLiteral("ssid=\\\"?([^,\\\"}]+)"),
                           QRegularExpression::CaseInsensitiveOption)};
    for (const QString &line : lines) {
        for (const QRegularExpression &pattern : patterns) {
            const QRegularExpressionMatch match = pattern.match(line);
            if (!match.hasMatch()) {
                continue;
            }
            const QString value = match.captured(1).trimmed();
            if (value.compare(QStringLiteral("<unknown ssid>"), Qt::CaseInsensitive) != 0
                && value.compare(QStringLiteral("unknown"), Qt::CaseInsensitive) != 0) {
                return value;
            }
        }
    }
    return QString();
}

} // namespace

OverviewService::OverviewService(QString adbPath, QObject *parent)
    : QObject(parent)
    , m_adbPath(QDir::cleanPath(std::move(adbPath)))
    , m_process(this)
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] {
        m_output += m_process.readAllStandardOutput();
    });
    connect(&m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &OverviewService::handleFinished);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            emit loadingChanged(false);
            emit overviewError(tr("adb 启动失败：%1").arg(m_process.errorString()));
        }
    });

    m_screenshotProcess.setProcessChannelMode(QProcess::SeparateChannels);
    connect(&m_screenshotProcess, &QProcess::readyReadStandardOutput, this, [this] {
        m_screenshotOutput += m_screenshotProcess.readAllStandardOutput();
    });
    connect(&m_screenshotProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                m_screenshotOutput += m_screenshotProcess.readAllStandardOutput();
                const QString error = decodeAdbText(
                    m_screenshotProcess.readAllStandardError()).trimmed();
                emit screenshotLoadingChanged(false);
                if (exitStatus == QProcess::NormalExit && exitCode == 0
                    && !m_screenshotOutput.isEmpty()) {
                    emit screenshotReady(m_screenshotOutput);
                } else {
                    emit overviewError(error.isEmpty()
                                           ? tr("屏幕截图失败，退出码：%1").arg(exitCode)
                                           : error.left(500));
                }
            });
    connect(&m_screenshotProcess,
            &QProcess::errorOccurred,
            this,
            [this](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart) {
                    emit screenshotLoadingChanged(false);
                    emit overviewError(tr("截图命令启动失败：%1")
                                           .arg(m_screenshotProcess.errorString()));
                }
            });

    m_actionProcess.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_actionProcess, &QProcess::readyReadStandardOutput, this, [this] {
        m_actionOutput += m_actionProcess.readAllStandardOutput();
    });
    connect(&m_actionProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                m_actionOutput += m_actionProcess.readAllStandardOutput();
                const QString detail = decodeAdbText(m_actionOutput).trimmed();
                const bool success = exitStatus == QProcess::NormalExit && exitCode == 0;
                emit actionFinished(success,
                                    m_actionLabel,
                                    detail.isEmpty()
                                        ? (success ? tr("操作完成。")
                                                   : tr("命令退出码：%1").arg(exitCode))
                                        : detail.left(1000));
            });
    connect(&m_actionProcess,
            &QProcess::errorOccurred,
            this,
            [this](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart) {
                    emit actionFinished(false,
                                        m_actionLabel,
                                        tr("adb 启动失败：%1")
                                            .arg(m_actionProcess.errorString()));
                }
            });
}

OverviewService::~OverviewService()
{
    for (QProcess *process : {&m_process, &m_screenshotProcess, &m_actionProcess}) {
        if (process->state() != QProcess::NotRunning) {
            process->kill();
            process->waitForFinished(1000);
        }
    }
}

void OverviewService::setDeviceSerial(const QString &serial)
{
    if (m_deviceSerial == serial) {
        return;
    }
    m_deviceSerial = serial;
    m_refreshPending = false;
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
    }
    if (m_screenshotProcess.state() != QProcess::NotRunning) {
        m_screenshotProcess.kill();
        emit screenshotLoadingChanged(false);
    }
    if (m_actionProcess.state() != QProcess::NotRunning) {
        m_actionProcess.kill();
    }
}

void OverviewService::refresh()
{
    if (m_deviceSerial.isEmpty()) {
        emit overviewError(tr("请先连接 Android 设备。"));
        return;
    }
    if (m_process.state() != QProcess::NotRunning) {
        m_refreshPending = true;
        return;
    }
    if (!QFileInfo::exists(m_adbPath)) {
        emit overviewError(tr("未找到 adb：%1").arg(QDir::toNativeSeparators(m_adbPath)));
        return;
    }

    m_output.clear();
    emit loadingChanged(true);
    m_process.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    m_process.start(m_adbPath,
                    {QStringLiteral("-s"),
                     m_deviceSerial,
                     QStringLiteral("shell"),
                     overviewCommand()});
}

void OverviewService::captureScreenshot()
{
    if (m_deviceSerial.isEmpty() || m_screenshotProcess.state() != QProcess::NotRunning) {
        return;
    }
    if (!QFileInfo::exists(m_adbPath)) {
        emit overviewError(tr("未找到 adb：%1").arg(QDir::toNativeSeparators(m_adbPath)));
        return;
    }

    m_screenshotOutput.clear();
    emit screenshotLoadingChanged(true);
    m_screenshotProcess.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    m_screenshotProcess.start(m_adbPath,
                              {QStringLiteral("-s"),
                               m_deviceSerial,
                               QStringLiteral("exec-out"),
                               QStringLiteral("screencap"),
                               QStringLiteral("-p")});
}

void OverviewService::startShizuku()
{
    runAction(tr("启动 Shizuku"),
              QStringLiteral("sh /sdcard/Android/data/moe.shizuku.privileged.api/start.sh"));
}

void OverviewService::togglePower()
{
    runAction(tr("切换电源状态"), QStringLiteral("input keyevent 26"));
}

void OverviewService::runAction(const QString &label, const QString &shellCommand)
{
    if (m_deviceSerial.isEmpty()) {
        emit actionFinished(false, label, tr("请先连接 Android 设备。"));
        return;
    }
    if (m_actionProcess.state() != QProcess::NotRunning) {
        emit actionFinished(false, label, tr("另一项设备操作正在执行。"));
        return;
    }
    if (!QFileInfo::exists(m_adbPath)) {
        emit actionFinished(false,
                            label,
                            tr("未找到 adb：%1").arg(QDir::toNativeSeparators(m_adbPath)));
        return;
    }

    m_actionLabel = label;
    m_actionOutput.clear();
    m_actionProcess.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    m_actionProcess.start(m_adbPath,
                          {QStringLiteral("-s"),
                           m_deviceSerial,
                           QStringLiteral("shell"),
                           shellCommand});
}

void OverviewService::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_output += m_process.readAllStandardOutput();
    const QString output = decodeAdbText(m_output);
    emit loadingChanged(false);

    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        const QString detail = output.trimmed();
        emit overviewError(detail.isEmpty()
                               ? tr("设备概览采集失败，退出码：%1").arg(exitCode)
                               : detail.left(1000));
    } else {
        DeviceOverview overview = parseOverview(output);
        if (overview.serialNumber.isEmpty()) {
            overview.serialNumber = m_deviceSerial;
        }
        emit overviewReady(overview);
    }

    if (m_refreshPending && !m_deviceSerial.isEmpty()) {
        m_refreshPending = false;
        refresh();
    }
}

DeviceOverview OverviewService::parseOverview(const QString &output)
{
    const QHash<QString, QStringList> sections = splitSections(output);
    const QHash<QString, QString> properties = parseProperties(
        sections.value(kPropertiesMarker));

    DeviceOverview overview;
    overview.name = firstProperty(
        properties,
        {QStringLiteral("ro.oppo.market.name"),
         QStringLiteral("ro.config.marketing_name"),
         QStringLiteral("ro.vendor.oplus.market.enname"),
         QStringLiteral("ro.vivo.market.name"),
         QStringLiteral("ro.product.marketname"),
         QStringLiteral("ro.asus.product.mkt_name")});
    overview.brand = properties.value(QStringLiteral("ro.product.brand"));
    overview.manufacturer = properties.value(QStringLiteral("ro.product.manufacturer"));
    if (overview.brand.isEmpty()) {
        overview.brand = overview.manufacturer;
    }
    overview.model = properties.value(QStringLiteral("ro.product.model"));
    if (overview.name.isEmpty()) {
        overview.name = (overview.brand + QLatin1Char(' ') + overview.model).trimmed();
    }
    const QString characteristics = properties.value(
        QStringLiteral("ro.build.characteristics"));
    overview.deviceType = characteristics.contains(QStringLiteral("tablet"),
                                                    Qt::CaseInsensitive)
        ? QObject::tr("平板电脑")
        : QObject::tr("手机");
    overview.product = properties.value(QStringLiteral("ro.product.name"));
    overview.codename = properties.value(QStringLiteral("ro.product.device"));
    overview.serialNumber = firstProperty(
        properties,
        {QStringLiteral("ro.serialno"), QStringLiteral("ro.boot.serialno")});
    overview.androidVersion = properties.value(QStringLiteral("ro.build.version.release"));
    overview.sdkVersion = properties.value(QStringLiteral("ro.build.version.sdk"));
    overview.processor = firstProperty(
        properties,
        {QStringLiteral("ro.soc.model"),
         QStringLiteral("ro.board.platform"),
         QStringLiteral("ro.hardware"),
         QStringLiteral("ro.product.board")});
    overview.abi = properties.value(QStringLiteral("ro.product.cpu.abi"));

    const QStringList kernelLines = sections.value(kKernelMarker);
    if (!kernelLines.isEmpty()) {
        overview.kernelVersion = kernelLines.first();
    }
    const QStringList cpuLines = sections.value(kCpuMarker);
    if (!cpuLines.isEmpty()) {
        overview.cpuCount = cpuLines.first().toInt();
    }

    const QStringList storageLines = sections.value(kStorageMarker);
    for (int index = storageLines.size() - 1; index >= 0; --index) {
        const QStringList fields = storageLines[index].split(
            QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (fields.size() < 4 || fields.first().contains(QStringLiteral("Filesystem"))) {
            continue;
        }
        bool totalOk = false;
        bool usedOk = false;
        const qint64 totalKb = fields[1].toLongLong(&totalOk);
        const qint64 usedKb = fields[2].toLongLong(&usedOk);
        if (totalOk && usedOk) {
            overview.storageTotalBytes = totalKb * 1024;
            overview.storageUsedBytes = usedKb * 1024;
            break;
        }
    }

    overview.memoryTotalBytes = memoryKilobytes(sections.value(kMemoryMarker),
                                                 QStringLiteral("MemTotal"))
        * 1024;
    qint64 availableKb = memoryKilobytes(sections.value(kMemoryMarker),
                                        QStringLiteral("MemAvailable"));
    if (availableKb <= 0) {
        availableKb = memoryKilobytes(sections.value(kMemoryMarker),
                                      QStringLiteral("MemFree"))
            + memoryKilobytes(sections.value(kMemoryMarker), QStringLiteral("Buffers"))
            + memoryKilobytes(sections.value(kMemoryMarker), QStringLiteral("Cached"));
    }
    overview.memoryUsedBytes = std::max<qint64>(0,
                                               overview.memoryTotalBytes
                                                   - availableKb * 1024);

    const QStringList batteryLines = sections.value(kBatteryMarker);
    overview.batteryLevel = integerField(batteryLines, QStringLiteral("level"));
    overview.batteryHealth = batteryHealthName(
        integerField(batteryLines, QStringLiteral("health")));

    const QStringList uptimeLines = sections.value(kUptimeMarker);
    if (!uptimeLines.isEmpty()) {
        bool ok = false;
        const double seconds = uptimeLines.first().section(QLatin1Char(' '), 0, 0)
                                   .toDouble(&ok);
        if (ok && seconds >= 0.0) {
            overview.uptimeSeconds = static_cast<qint64>(seconds);
        }
    }
    const QStringList sizeLines = sections.value(kSizeMarker);
    overview.physicalResolution = screenValue(sizeLines, QStringLiteral("Physical size"));
    overview.resolution = screenValue(sizeLines, QStringLiteral("Override size"));
    if (overview.resolution.isEmpty()) {
        overview.resolution = overview.physicalResolution;
    }
    const QStringList densityLines = sections.value(kDensityMarker);
    overview.physicalDensity = screenValue(densityLines, QStringLiteral("Physical density"));
    overview.density = screenValue(densityLines, QStringLiteral("Override density"));
    if (overview.density.isEmpty()) {
        overview.density = overview.physicalDensity;
    }

    const QStringList fontLines = sections.value(kFontMarker);
    if (!fontLines.isEmpty()) {
        bool ok = false;
        const double scale = fontLines.first().toDouble(&ok);
        if (ok && scale > 0.0) {
            overview.fontScale = scale;
        }
    }
    overview.wifi = wifiName(sections.value(kWifiMarker));

    const QString network = sections.value(kNetworkMarker).join(QLatin1Char('\n'));
    const QRegularExpressionMatch ipMatch = QRegularExpression(
        QStringLiteral("\\binet\\s+(\\d+\\.\\d+\\.\\d+\\.\\d+)"))
                                                .match(network);
    if (ipMatch.hasMatch()) {
        overview.ipAddress = ipMatch.captured(1);
    }
    const QRegularExpressionMatch macMatch = QRegularExpression(
        QStringLiteral("link/ether\\s+([0-9A-Fa-f:]{17})"))
                                                 .match(network);
    if (macMatch.hasMatch()) {
        overview.macAddress = macMatch.captured(1);
    }
    return overview;
}
