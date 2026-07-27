#include "services/scrcpy_service.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include <utility>

namespace {

constexpr int kDevicePollIntervalMs = 2500;
constexpr int kMirrorLogLimit = 8000;

} // namespace

ScrcpyService::ScrcpyService(QString scrcpyPath, QObject *parent)
    : QObject(parent)
    , m_scrcpyPath(QDir::cleanPath(std::move(scrcpyPath)))
    , m_probeProcess(this)
    , m_mirrorProcess(this)
    , m_cameraProcess(this)
    , m_pollTimer(this)
{
    m_pollTimer.setInterval(kDevicePollIntervalMs);

    connect(&m_pollTimer, &QTimer::timeout, this, &ScrcpyService::refreshDeviceState);
    connect(&m_probeProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &ScrcpyService::handleProbeFinished);
    connect(&m_probeProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            setDeviceState(DeviceState::ToolUnavailable,
                           QString(),
                           tr("无法启动 adb.exe：%1").arg(m_probeProcess.errorString()));
        }
    });

    m_mirrorProcess.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_mirrorProcess, &QProcess::started, this, [this] {
        setMirrorRunning(true);
    });
    connect(&m_mirrorProcess, &QProcess::readyReadStandardOutput, this, [this] {
        m_mirrorLog += QString::fromLocal8Bit(m_mirrorProcess.readAllStandardOutput());
        if (m_mirrorLog.size() > kMirrorLogLimit) {
            m_mirrorLog = m_mirrorLog.right(kMirrorLogLimit);
        }
    });
    connect(&m_mirrorProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &ScrcpyService::handleMirrorFinished);
    connect(&m_mirrorProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            setMirrorRunning(false);
            emit operationError(tr("scrcpy 启动失败：%1").arg(m_mirrorProcess.errorString()));
        }
    });

    m_cameraProcess.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_cameraProcess, &QProcess::readyReadStandardOutput, this, [this] {
        m_cameraOutput += QString::fromLocal8Bit(m_cameraProcess.readAllStandardOutput());
    });
    connect(&m_cameraProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                m_cameraOutput += QString::fromLocal8Bit(
                    m_cameraProcess.readAllStandardOutput());
                if (m_cameraQueryCancelled) {
                    m_cameraQueryCancelled = false;
                    m_cameraOutput.clear();
                    return;
                }
                QStringList cameras;
                const QStringList lines = m_cameraOutput.split(
                    QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
                for (const QString &line : lines) {
                    if (line.contains(QStringLiteral("--camera-id="))) {
                        cameras.append(line.trimmed());
                    }
                }
                cameras.removeDuplicates();
                if (!cameras.isEmpty() || (exitStatus == QProcess::NormalExit && exitCode == 0)) {
                    emit camerasLoaded(cameras);
                } else {
                    emit operationError(m_cameraOutput.trimmed().isEmpty()
                                            ? tr("无法读取设备摄像头列表。")
                                            : m_cameraOutput.trimmed().right(1200));
                }
            });
    connect(&m_cameraProcess,
            &QProcess::errorOccurred,
            this,
            [this](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart) {
                    emit operationError(tr("摄像头查询启动失败：%1")
                                            .arg(m_cameraProcess.errorString()));
                }
            });
}

ScrcpyService::~ScrcpyService()
{
    m_pollTimer.stop();
    m_cameraQueryCancelled = true;
    for (QProcess *process : {&m_mirrorProcess, &m_cameraProcess}) {
        if (process->state() != QProcess::NotRunning) {
            process->terminate();
            if (!process->waitForFinished(1000)) {
                process->kill();
                process->waitForFinished(1000);
            }
        }
    }
}

ScrcpyService::DeviceState ScrcpyService::deviceState() const
{
    return m_deviceState;
}

QString ScrcpyService::deviceSerial() const
{
    return m_deviceSerial;
}

QString ScrcpyService::deviceDetail() const
{
    return m_deviceDetail;
}

QString ScrcpyService::scrcpyPath() const
{
    return m_scrcpyPath;
}

QString ScrcpyService::adbExecutablePath() const
{
    return adbPath();
}

bool ScrcpyService::mirrorRunning() const
{
    return m_mirrorRunning;
}

void ScrcpyService::startMonitoring()
{
    refreshDeviceState();
    m_pollTimer.start();
}

void ScrcpyService::refreshDeviceState()
{
    if (m_probeProcess.state() != QProcess::NotRunning) {
        return;
    }

    if (!QFileInfo::exists(m_scrcpyPath)) {
        setDeviceState(DeviceState::ToolUnavailable,
                       QString(),
                       tr("未找到 scrcpy.exe：%1").arg(QDir::toNativeSeparators(m_scrcpyPath)));
        return;
    }

    const QString currentAdbPath = adbPath();
    if (!QFileInfo::exists(currentAdbPath)) {
        setDeviceState(DeviceState::ToolUnavailable,
                       QString(),
                       tr("未找到 adb.exe：%1").arg(QDir::toNativeSeparators(currentAdbPath)));
        return;
    }

    m_probeProcess.setWorkingDirectory(QFileInfo(currentAdbPath).absolutePath());
    m_probeProcess.start(currentAdbPath, {QStringLiteral("devices")});
}

void ScrcpyService::startMirror(const QStringList &extraArguments)
{
    if (m_mirrorProcess.state() != QProcess::NotRunning) {
        return;
    }

    if (m_deviceState != DeviceState::Connected || m_deviceSerial.isEmpty()) {
        emit operationError(tr("未检测到可启动镜像的 Android 设备。"));
        return;
    }

    if (!QFileInfo::exists(m_scrcpyPath)) {
        emit operationError(tr("未找到 scrcpy.exe：%1").arg(QDir::toNativeSeparators(m_scrcpyPath)));
        return;
    }

    for (const QString &argument : extraArguments) {
        if (argument == QStringLiteral("-s") || argument == QStringLiteral("--serial")
            || argument.startsWith(QStringLiteral("--serial="))) {
            emit operationError(tr("高级参数不能覆盖当前设备序列号。"));
            return;
        }
        if (argument.startsWith(QStringLiteral("--record="))) {
            const QFileInfo recordFile(argument.mid(QStringLiteral("--record=").size()));
            const QString parentPath = recordFile.absolutePath();
            if (!parentPath.isEmpty() && !QDir().mkpath(parentPath)) {
                emit operationError(tr("无法创建录制目录：%1")
                                        .arg(QDir::toNativeSeparators(parentPath)));
                return;
            }
        }
    }

    m_mirrorLog.clear();
    m_stopRequested = false;
    m_mirrorProcess.setWorkingDirectory(QFileInfo(m_scrcpyPath).absolutePath());
    QStringList arguments = {QStringLiteral("--serial"),
                             m_deviceSerial,
                             QStringLiteral("--window-title"),
                             tr("AI Mobile Test Studio - %1").arg(m_deviceSerial)};
    arguments.append(extraArguments);
    m_mirrorProcess.start(m_scrcpyPath, arguments);
}

void ScrcpyService::stopMirror()
{
    if (m_mirrorProcess.state() == QProcess::NotRunning) {
        return;
    }

    m_stopRequested = true;
    m_mirrorProcess.terminate();
    QTimer::singleShot(1500, this, [this] {
        if (m_mirrorProcess.state() != QProcess::NotRunning) {
            m_mirrorProcess.kill();
        }
    });
}

void ScrcpyService::queryCameras()
{
    if (m_cameraProcess.state() != QProcess::NotRunning) {
        return;
    }
    if (m_deviceState != DeviceState::Connected || m_deviceSerial.isEmpty()) {
        emit operationError(tr("请先连接 Android 设备。"));
        return;
    }
    if (!QFileInfo::exists(m_scrcpyPath)) {
        emit operationError(tr("未找到 scrcpy.exe：%1")
                                .arg(QDir::toNativeSeparators(m_scrcpyPath)));
        return;
    }

    m_cameraOutput.clear();
    m_cameraQueryCancelled = false;
    m_cameraProcess.setWorkingDirectory(QFileInfo(m_scrcpyPath).absolutePath());
    m_cameraProcess.start(m_scrcpyPath,
                          {QStringLiteral("--serial"),
                           m_deviceSerial,
                           QStringLiteral("--list-cameras")});
}

void ScrcpyService::handleProbeFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        const QString error = QString::fromLocal8Bit(m_probeProcess.readAllStandardError()).trimmed();
        setDeviceState(DeviceState::ToolUnavailable,
                       QString(),
                       error.isEmpty() ? tr("adb 设备检测失败。") : error);
        return;
    }

    const QString output = QString::fromLocal8Bit(m_probeProcess.readAllStandardOutput());
    const QStringList lines = output.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                           Qt::SkipEmptyParts);

    QString unauthorizedSerial;
    for (const QString &line : lines) {
        if (line.startsWith(QStringLiteral("List of devices"))) {
            continue;
        }

        const QStringList columns = line.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (columns.size() < 2) {
            continue;
        }

        const QString serial = columns[0];
        const QString state = columns[1];
        if (state == QStringLiteral("device")) {
            setDeviceState(DeviceState::Connected, serial, tr("Android 设备已连接"));
            return;
        }
        if (state == QStringLiteral("sideload")) {
            setDeviceState(DeviceState::Sideload,
                           serial,
                           tr("设备已进入 ADB Sideload 模式"));
            return;
        }
        if (state == QStringLiteral("unauthorized")) {
            unauthorizedSerial = serial;
        }
    }

    if (!unauthorizedSerial.isEmpty()) {
        setDeviceState(DeviceState::Unauthorized,
                       unauthorizedSerial,
                       tr("设备正在等待 USB 调试授权"));
        return;
    }

    setDeviceState(DeviceState::Disconnected, QString(), tr("未检测到 Android 设备"));
}

void ScrcpyService::handleMirrorFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_mirrorLog += QString::fromLocal8Bit(m_mirrorProcess.readAllStandardOutput());
    const bool reportFailure = !m_stopRequested
        && (exitStatus != QProcess::NormalExit || exitCode != 0);

    setMirrorRunning(false);
    if (reportFailure) {
        const QString log = recentMirrorLog();
        emit operationError(log.isEmpty()
                                ? tr("scrcpy 已异常退出，退出码：%1").arg(exitCode)
                                : tr("scrcpy 已异常退出：\n%1").arg(log));
    }

    m_stopRequested = false;
    refreshDeviceState();
}

void ScrcpyService::setDeviceState(DeviceState state,
                                   const QString &serial,
                                   const QString &detail)
{
    if (m_deviceState == state && m_deviceSerial == serial && m_deviceDetail == detail) {
        return;
    }

    if (m_deviceSerial != serial && m_cameraProcess.state() != QProcess::NotRunning) {
        m_cameraQueryCancelled = true;
        m_cameraProcess.kill();
    }
    m_deviceState = state;
    m_deviceSerial = serial;
    m_deviceDetail = detail;
    emit deviceStateChanged(state, serial, detail);
}

void ScrcpyService::setMirrorRunning(bool running)
{
    if (m_mirrorRunning == running) {
        return;
    }

    m_mirrorRunning = running;
    emit mirrorRunningChanged(running);
}

QString ScrcpyService::adbPath() const
{
    return QDir(QFileInfo(m_scrcpyPath).absolutePath()).filePath(QStringLiteral("adb.exe"));
}

QString ScrcpyService::recentMirrorLog() const
{
    return m_mirrorLog.trimmed().right(1200);
}
