#include "services/adb_control_service.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include <utility>

AdbControlService::AdbControlService(QString adbPath, QObject *parent)
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
            &AdbControlService::handleFinished);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) {
            return;
        }
        emit commandFinished(false,
                             m_currentCommand.label,
                             tr("adb 启动失败：%1").arg(m_process.errorString()));
        startNext();
    });
}

void AdbControlService::setDeviceSerial(const QString &serial)
{
    m_deviceSerial = serial;
    if (m_deviceSerial.isEmpty()) {
        m_queue.clear();
    }
}

void AdbControlService::sendKeyEvent(const QString &keyCode)
{
    static const QRegularExpression keyCodePattern(QStringLiteral("^KEYCODE_[A-Z0-9_]+$"));
    if (!keyCodePattern.match(keyCode).hasMatch()) {
        emit commandFinished(false, keyCode, tr("无效的 Android KEYCODE。"));
        return;
    }

    if (m_deviceSerial.isEmpty()) {
        emit commandFinished(false, keyCode, tr("当前没有已连接并授权的设备。"));
        return;
    }

    enqueue({keyCode,
             QStringLiteral("adb -s %1 shell input keyevent %2").arg(m_deviceSerial, keyCode),
             {QStringLiteral("-s"),
              m_deviceSerial,
              QStringLiteral("shell"),
              QStringLiteral("input"),
              QStringLiteral("keyevent"),
              keyCode}});
}

void AdbControlService::rebootDevice(const QString &mode, const QString &label)
{
    static const QRegularExpression modePattern(QStringLiteral("^(recovery|bootloader|fastboot)?$"));
    if (!modePattern.match(mode).hasMatch()) {
        emit commandFinished(false, label, tr("无效的重启模式。"));
        return;
    }

    if (m_deviceSerial.isEmpty()) {
        emit commandFinished(false, label, tr("当前没有已连接并授权的设备。"));
        return;
    }

    QStringList arguments = {QStringLiteral("-s"), m_deviceSerial, QStringLiteral("reboot")};
    if (!mode.isEmpty()) {
        arguments.append(mode);
    }
    const QString displayCommand = mode.isEmpty()
        ? QStringLiteral("adb -s %1 reboot").arg(m_deviceSerial)
        : QStringLiteral("adb -s %1 reboot %2").arg(m_deviceSerial, mode);
    enqueue({label, displayCommand, arguments});
}

void AdbControlService::powerOffDevice(const QString &label)
{
    if (m_deviceSerial.isEmpty()) {
        emit commandFinished(false, label, tr("当前没有已连接并授权的设备。"));
        return;
    }

    enqueue({label,
             QStringLiteral("adb -s %1 shell reboot -p").arg(m_deviceSerial),
             {QStringLiteral("-s"),
              m_deviceSerial,
              QStringLiteral("shell"),
              QStringLiteral("reboot"),
              QStringLiteral("-p")}});
}

void AdbControlService::enqueue(PendingCommand command)
{
    m_queue.enqueue(std::move(command));
    startNext();
}

void AdbControlService::startNext()
{
    if (m_process.state() != QProcess::NotRunning || m_queue.isEmpty()) {
        return;
    }

    if (!QFileInfo::exists(m_adbPath)) {
        const PendingCommand failedCommand = m_queue.dequeue();
        emit commandFinished(false,
                             failedCommand.label,
                             tr("未找到 adb.exe：%1").arg(QDir::toNativeSeparators(m_adbPath)));
        startNext();
        return;
    }

    m_currentCommand = m_queue.dequeue();
    m_output.clear();
    m_process.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    emit commandStarted(m_currentCommand.label, m_currentCommand.displayCommand);
    m_process.start(m_adbPath, m_currentCommand.arguments);
}

void AdbControlService::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_output += QString::fromLocal8Bit(m_process.readAllStandardOutput());
    const bool success = exitStatus == QProcess::NormalExit && exitCode == 0;
    QString detail = m_output.trimmed();
    if (detail.isEmpty()) {
        detail = success ? tr("命令执行成功") : tr("命令执行失败，退出码：%1").arg(exitCode);
    }
    emit commandFinished(success, m_currentCommand.label, detail);
    startNext();
}
