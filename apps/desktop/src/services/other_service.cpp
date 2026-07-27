#include "services/other_service.h"

#include <QDir>
#include <QFileInfo>

#include <utility>

OtherService::OtherService(QString adbPath, QObject *parent)
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
            &OtherService::handleFinished);
    connect(&m_process,
            &QProcess::errorOccurred,
            this,
            [this](QProcess::ProcessError error) {
                if (error != QProcess::FailedToStart) {
                    return;
                }
                emit busyChanged(false);
                emit commandFinished(false,
                                     m_label,
                                     tr("adb 启动失败：%1").arg(m_process.errorString()));
            });
}

OtherService::~OtherService()
{
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}

void OtherService::setDeviceSerial(const QString &serial)
{
    if (m_deviceSerial == serial) {
        return;
    }
    m_deviceSerial = serial;
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        emit busyChanged(false);
    }
}

void OtherService::runShellCommand(const QString &label, const QString &command)
{
    if (m_process.state() != QProcess::NotRunning) {
        emit commandFinished(false, label, tr("另一项设备命令正在执行。"));
        return;
    }
    if (m_deviceSerial.isEmpty()) {
        emit commandFinished(false, label, tr("请先连接 Android 设备。"));
        return;
    }
    if (!QFileInfo::exists(m_adbPath)) {
        emit commandFinished(false,
                             label,
                             tr("未找到 adb：%1")
                                 .arg(QDir::toNativeSeparators(m_adbPath)));
        return;
    }

    const QString shellCommand = normalizeShellCommand(command);
    if (shellCommand.isEmpty()) {
        emit commandFinished(false, label, tr("命令不能为空。"));
        return;
    }

    m_label = label;
    m_output.clear();
    emit busyChanged(true);
    emit commandStarted(label,
                        QStringLiteral("adb -s %1 shell %2")
                            .arg(m_deviceSerial, shellCommand));
    m_process.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    m_process.start(m_adbPath,
                    {QStringLiteral("-s"),
                     m_deviceSerial,
                     QStringLiteral("shell"),
                     shellCommand});
}

void OtherService::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_output += m_process.readAllStandardOutput();
    const QString output = QString::fromUtf8(m_output).trimmed();
    const bool success = exitStatus == QProcess::NormalExit && exitCode == 0;
    emit busyChanged(false);
    emit commandFinished(success,
                         m_label,
                         output.isEmpty()
                             ? (success ? tr("操作完成。")
                                        : tr("命令退出码：%1").arg(exitCode))
                             : output.left(200000));
}

QString OtherService::normalizeShellCommand(QString command)
{
    command = command.trimmed();
    if (command.startsWith(QStringLiteral("adb shell "), Qt::CaseInsensitive)) {
        command.remove(0, QStringLiteral("adb shell ").size());
    } else if (command.compare(QStringLiteral("adb shell"), Qt::CaseInsensitive) == 0) {
        command.clear();
    }
    return command.trimmed();
}
