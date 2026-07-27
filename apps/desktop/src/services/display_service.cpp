#include "services/display_service.h"

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QLocale>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

const QString kSizeMarker = QStringLiteral("__AMTS_DISPLAY_SIZE__");
const QString kDensityMarker = QStringLiteral("__AMTS_DISPLAY_DENSITY__");
const QString kTimeoutMarker = QStringLiteral("__AMTS_DISPLAY_TIMEOUT__");
const QString kFontMarker = QStringLiteral("__AMTS_DISPLAY_FONT__");
const QString kWindowAnimationMarker = QStringLiteral("__AMTS_DISPLAY_WINDOW_ANIMATION__");
const QString kTransitionAnimationMarker = QStringLiteral("__AMTS_DISPLAY_TRANSITION_ANIMATION__");
const QString kAnimatorDurationMarker = QStringLiteral("__AMTS_DISPLAY_ANIMATOR_DURATION__");
const QString kDarkModeMarker = QStringLiteral("__AMTS_DISPLAY_DARK_MODE__");
const QString kSecureDarkModeMarker = QStringLiteral("__AMTS_DISPLAY_SECURE_DARK_MODE__");
const QString kPeakRateMarker = QStringLiteral("__AMTS_DISPLAY_PEAK_RATE__");
const QString kDisplayDumpMarker = QStringLiteral("__AMTS_DISPLAY_DUMP__");

QStringList adbArguments(const QString &serial, const QString &script)
{
    return {QStringLiteral("-s"), serial, QStringLiteral("shell"), script};
}

QString firstLine(const QHash<QString, QStringList> &sections, const QString &marker)
{
    const QStringList lines = sections.value(marker);
    return lines.isEmpty() ? QString() : lines.first().trimmed();
}

double parsedNumber(const QString &value, double fallback)
{
    bool ok = false;
    const double parsed = value.trimmed().toDouble(&ok);
    return ok ? parsed : fallback;
}

QString shellNumber(double value)
{
    QString number = QLocale::c().toString(value, 'f', 2);
    number.remove(QRegularExpression(QStringLiteral("\\.?0+$")));
    return number.isEmpty() ? QStringLiteral("0") : number;
}

double normalizedRate(double rate)
{
    const double rounded = std::round(rate);
    return std::abs(rate - rounded) < 0.06 ? rounded : std::round(rate * 100.0) / 100.0;
}

} // namespace

DisplayService::DisplayService(QString adbPath, QObject *parent)
    : QObject(parent)
    , m_adbPath(QDir::cleanPath(std::move(adbPath)))
    , m_process(this)
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] {
        m_output += QString::fromLocal8Bit(m_process.readAllStandardOutput());
    });
    connect(&m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &DisplayService::handleFinished);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart || m_request == Request::Idle) {
            return;
        }
        const QString message = tr("adb 启动失败：%1").arg(m_process.errorString());
        const Request failedRequest = m_request;
        const QString failedLabel = m_actionLabel;
        m_request = Request::Idle;
        setBusy(false);
        if (failedRequest == Request::Refresh) {
            emit settingsError(message);
        } else {
            emit operationFinished(false, failedLabel, message);
            QTimer::singleShot(0, this, &DisplayService::startNextAction);
        }
    });
}

void DisplayService::setDeviceSerial(const QString &serial)
{
    if (m_deviceSerial == serial) {
        return;
    }

    m_deviceSerial = serial;
    m_actions.clear();
    if (m_process.state() != QProcess::NotRunning) {
        m_cancelled = true;
        m_refreshAfterCancel = !serial.isEmpty();
        m_process.kill();
        return;
    }
    m_request = Request::Idle;
    setBusy(false);
}

void DisplayService::refresh()
{
    if (m_deviceSerial.isEmpty()) {
        emit settingsError(tr("请先连接 Android 设备。"));
        return;
    }
    if (m_request != Request::Idle || m_process.state() != QProcess::NotRunning) {
        return;
    }
    if (!QFileInfo::exists(m_adbPath)) {
        emit settingsError(tr("未找到 adb.exe：%1")
                               .arg(QDir::toNativeSeparators(m_adbPath)));
        return;
    }

    QString script = QStringLiteral(
        "echo '%1'; wm size 2>&1; "
        "echo '%2'; wm density 2>&1; "
        "echo '%3'; settings get system screen_off_timeout 2>&1; "
        "echo '%4'; settings get system font_scale 2>&1; "
        "echo '%5'; settings get global window_animation_scale 2>&1; "
        "echo '%6'; settings get global transition_animation_scale 2>&1; "
        "echo '%7'; settings get global animator_duration_scale 2>&1; "
        "echo '%8'; cmd uimode night 2>&1; "
        "echo '%9'; settings get secure ui_night_mode 2>&1; "
        "echo '%10'; settings get system peak_refresh_rate 2>&1; "
        "echo '%11'; dumpsys display 2>&1");
    script = script.arg(kSizeMarker)
                 .arg(kDensityMarker)
                 .arg(kTimeoutMarker)
                 .arg(kFontMarker)
                 .arg(kWindowAnimationMarker)
                 .arg(kTransitionAnimationMarker)
                 .arg(kAnimatorDurationMarker)
                 .arg(kDarkModeMarker)
                 .arg(kSecureDarkModeMarker)
                 .arg(kPeakRateMarker)
                 .arg(kDisplayDumpMarker);
    m_output.clear();
    m_actionLabel.clear();
    m_request = Request::Refresh;
    setBusy(true);
    m_process.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    m_process.start(m_adbPath, adbArguments(m_deviceSerial, script));
}

void DisplayService::applyDimensions(int width,
                                     int height,
                                     int density,
                                     int timeoutSeconds)
{
    if (width < 320 || height < 320 || density < 120 || timeoutSeconds < 1) {
        emit operationFinished(false, tr("应用显示设置"), tr("显示参数超出允许范围。"));
        return;
    }
    const qint64 timeoutMs = static_cast<qint64>(timeoutSeconds) * 1000;
    enqueueAction(tr("应用显示设置"),
                  QStringLiteral("wm size %1x%2 && wm density %3 && "
                                 "settings put system screen_off_timeout %4")
                      .arg(width)
                      .arg(height)
                      .arg(density)
                      .arg(timeoutMs));
}

void DisplayService::resetDimensions()
{
    enqueueAction(tr("重置分辨率与密度"),
                  QStringLiteral("wm size reset && wm density reset"));
}

void DisplayService::setRefreshRate(double rate)
{
    if (!std::isfinite(rate) || rate < 20.0 || rate > 360.0) {
        emit operationFinished(false, tr("设置刷新率"), tr("刷新率超出允许范围。"));
        return;
    }
    const QString value = shellNumber(rate);
    enqueueAction(tr("设置刷新率为 %1 Hz").arg(value),
                  QStringLiteral("settings put system peak_refresh_rate %1 && "
                                 "settings put system min_refresh_rate %1")
                      .arg(value));
}

void DisplayService::setDarkMode(bool enabled)
{
    const QString mode = enabled ? QStringLiteral("yes") : QStringLiteral("no");
    const QString fallback = enabled ? QStringLiteral("2") : QStringLiteral("1");
    enqueueAction(enabled ? tr("切换为深色模式") : tr("切换为浅色模式"),
                  QStringLiteral("cmd uimode night %1 >/dev/null 2>&1 || "
                                 "settings put secure ui_night_mode %2")
                      .arg(mode, fallback));
}

void DisplayService::setFontScale(double scale)
{
    if (!std::isfinite(scale) || scale < 0.5 || scale > 2.0) {
        emit operationFinished(false, tr("设置字体缩放"), tr("字体缩放超出允许范围。"));
        return;
    }
    const QString value = shellNumber(scale);
    enqueueAction(tr("设置字体缩放为 %1x").arg(value),
                  QStringLiteral("settings put system font_scale %1").arg(value));
}

void DisplayService::setAnimationScale(double scale)
{
    if (!std::isfinite(scale) || scale < 0.0 || scale > 10.0) {
        emit operationFinished(false, tr("设置动画速度"), tr("动画缩放超出允许范围。"));
        return;
    }
    const QString value = shellNumber(scale);
    enqueueAction(tr("设置动画缩放为 %1x").arg(value),
                  QStringLiteral("settings put global window_animation_scale %1 && "
                                 "settings put global transition_animation_scale %1 && "
                                 "settings put global animator_duration_scale %1")
                      .arg(value));
}

void DisplayService::enqueueAction(const QString &label, const QString &script)
{
    if (m_deviceSerial.isEmpty()) {
        emit operationFinished(false, label, tr("请先连接 Android 设备。"));
        return;
    }
    m_actions.enqueue({label, script});
    startNextAction();
}

void DisplayService::startAction(const PendingAction &action)
{
    if (!QFileInfo::exists(m_adbPath)) {
        emit operationFinished(false,
                               action.label,
                               tr("未找到 adb.exe：%1")
                                   .arg(QDir::toNativeSeparators(m_adbPath)));
        startNextAction();
        return;
    }
    m_output.clear();
    m_actionLabel = action.label;
    m_request = Request::Action;
    setBusy(true);
    m_process.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    m_process.start(m_adbPath, adbArguments(m_deviceSerial, action.script));
}

void DisplayService::startNextAction()
{
    if (m_request != Request::Idle || m_process.state() != QProcess::NotRunning
        || m_actions.isEmpty()) {
        return;
    }
    startAction(m_actions.dequeue());
}

void DisplayService::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_output += QString::fromLocal8Bit(m_process.readAllStandardOutput());
    if (m_cancelled) {
        const bool refreshAfterCancel = m_refreshAfterCancel;
        m_cancelled = false;
        m_refreshAfterCancel = false;
        m_request = Request::Idle;
        m_actionLabel.clear();
        m_output.clear();
        setBusy(false);
        if (refreshAfterCancel) {
            QTimer::singleShot(0, this, &DisplayService::refresh);
        }
        return;
    }

    const Request finishedRequest = m_request;
    const QString label = m_actionLabel;
    const QString detail = m_output.trimmed();
    const bool success = exitStatus == QProcess::NormalExit && exitCode == 0;
    m_request = Request::Idle;
    m_actionLabel.clear();
    setBusy(false);

    if (finishedRequest == Request::Refresh) {
        if (success) {
            const DisplaySettings settings = parseSettings(m_output);
            if (settings.currentWidth > 0 && settings.currentHeight > 0
                && settings.currentDensity > 0) {
                emit settingsLoaded(settings);
            } else {
                emit settingsError(tr("设备返回的显示参数不完整。"));
            }
        } else {
            emit settingsError(detail.isEmpty()
                                   ? tr("读取设备显示设置失败，退出码：%1").arg(exitCode)
                                   : detail.right(1200));
        }
        startNextAction();
        return;
    }

    if (finishedRequest == Request::Action) {
        emit operationFinished(success,
                               label,
                               success ? tr("操作已完成。")
                                       : (detail.isEmpty()
                                              ? tr("命令执行失败，退出码：%1").arg(exitCode)
                                              : detail.right(1200)));
        if (success) {
            refresh();
        } else {
            startNextAction();
        }
    }
}

void DisplayService::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    emit busyChanged(busy);
}

DisplaySettings DisplayService::parseSettings(const QString &output)
{
    const QSet<QString> markers = {kSizeMarker,
                                   kDensityMarker,
                                   kTimeoutMarker,
                                   kFontMarker,
                                   kWindowAnimationMarker,
                                   kTransitionAnimationMarker,
                                   kAnimatorDurationMarker,
                                   kDarkModeMarker,
                                   kSecureDarkModeMarker,
                                   kPeakRateMarker,
                                   kDisplayDumpMarker};
    QHash<QString, QStringList> sections;
    QString currentMarker;
    const QStringList lines = output.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                           Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (markers.contains(trimmed)) {
            currentMarker = trimmed;
        } else if (!currentMarker.isEmpty()) {
            sections[currentMarker].append(trimmed);
        }
    }

    DisplaySettings settings;
    static const QRegularExpression physicalSizePattern(
        QStringLiteral("Physical size:\\s*(\\d+)x(\\d+)"));
    static const QRegularExpression overrideSizePattern(
        QStringLiteral("Override size:\\s*(\\d+)x(\\d+)"));
    for (const QString &line : sections.value(kSizeMarker)) {
        QRegularExpressionMatch match = physicalSizePattern.match(line);
        if (match.hasMatch()) {
            settings.physicalWidth = match.captured(1).toInt();
            settings.physicalHeight = match.captured(2).toInt();
            settings.currentWidth = settings.physicalWidth;
            settings.currentHeight = settings.physicalHeight;
        }
        match = overrideSizePattern.match(line);
        if (match.hasMatch()) {
            settings.currentWidth = match.captured(1).toInt();
            settings.currentHeight = match.captured(2).toInt();
        }
    }

    static const QRegularExpression physicalDensityPattern(
        QStringLiteral("Physical density:\\s*(\\d+)"));
    static const QRegularExpression overrideDensityPattern(
        QStringLiteral("Override density:\\s*(\\d+)"));
    for (const QString &line : sections.value(kDensityMarker)) {
        QRegularExpressionMatch match = physicalDensityPattern.match(line);
        if (match.hasMatch()) {
            settings.physicalDensity = match.captured(1).toInt();
            settings.currentDensity = settings.physicalDensity;
        }
        match = overrideDensityPattern.match(line);
        if (match.hasMatch()) {
            settings.currentDensity = match.captured(1).toInt();
        }
    }

    if (settings.physicalWidth == 0) {
        settings.physicalWidth = settings.currentWidth;
        settings.physicalHeight = settings.currentHeight;
    }
    if (settings.physicalDensity == 0) {
        settings.physicalDensity = settings.currentDensity;
    }
    if (settings.currentDensity > 0) {
        settings.smallestWidthDp = qRound(
            std::min(settings.currentWidth, settings.currentHeight) * 160.0
            / settings.currentDensity);
    }

    bool timeoutOk = false;
    const qint64 timeoutMs = firstLine(sections, kTimeoutMarker).toLongLong(&timeoutOk);
    if (timeoutOk && timeoutMs > 0) {
        settings.screenTimeoutSeconds = std::max<qint64>(1, qRound(timeoutMs / 1000.0));
    }
    settings.fontScale = parsedNumber(firstLine(sections, kFontMarker), 1.0);
    settings.animationScale = parsedNumber(firstLine(sections, kWindowAnimationMarker), 1.0);

    const QString darkMode = sections.value(kDarkModeMarker).join(QLatin1Char(' ')).toLower();
    const QString secureDarkMode = firstLine(sections, kSecureDarkModeMarker);
    settings.darkModeEnabled = darkMode.contains(QStringLiteral("yes"))
        || secureDarkMode == QStringLiteral("2");

    settings.refreshRateHz = parsedNumber(firstLine(sections, kPeakRateMarker), 0.0);
    QVector<double> rates;
    static const QRegularExpression ratePattern(
        QStringLiteral("(?:fps|refreshRate|renderFrameRate)\\s*[=:]\\s*(\\d+(?:\\.\\d+)?)"));
    const QString displayDump = sections.value(kDisplayDumpMarker).join(QLatin1Char('\n'));
    QRegularExpressionMatchIterator iterator = ratePattern.globalMatch(displayDump);
    while (iterator.hasNext()) {
        const double rate = iterator.next().captured(1).toDouble();
        if (rate >= 20.0 && rate <= 360.0) {
            rates.append(normalizedRate(rate));
        }
    }
    std::sort(rates.begin(), rates.end());
    rates.erase(std::unique(rates.begin(), rates.end(), [](double left, double right) {
                    return std::abs(left - right) < 0.05;
                }),
                rates.end());
    if (settings.refreshRateHz > 0.0
        && std::none_of(rates.begin(), rates.end(), [&settings](double rate) {
               return std::abs(rate - settings.refreshRateHz) < 0.6;
           })) {
        rates.append(normalizedRate(settings.refreshRateHz));
        std::sort(rates.begin(), rates.end());
    }
    settings.supportedRefreshRatesHz = rates;
    if (settings.refreshRateHz <= 0.0 && !rates.isEmpty()) {
        settings.refreshRateHz = rates.constLast();
    }
    return settings;
}
