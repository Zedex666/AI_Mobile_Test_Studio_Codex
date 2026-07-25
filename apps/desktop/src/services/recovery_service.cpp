#include "services/recovery_service.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTimer>

#include <utility>

RecoveryService::RecoveryService(QString adbPath, QObject *parent)
    : QObject(parent)
    , m_adbPath(QDir::cleanPath(std::move(adbPath)))
    , m_process(this)
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &RecoveryService::handleReadyRead);
    connect(&m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &RecoveryService::handleFinished);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && m_busy) {
            finish(false, tr("adb 启动失败：%1").arg(m_process.errorString()));
        }
    });
}

RecoveryService::~RecoveryService()
{
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}

void RecoveryService::setSideloadDeviceSerial(const QString &serial)
{
    if (!m_busy) {
        m_deviceSerial = serial;
    }
}

bool RecoveryService::busy() const
{
    return m_busy;
}

void RecoveryService::startSideload(const QString &zipPath)
{
    if (m_busy) {
        return;
    }
    if (m_deviceSerial.isEmpty()) {
        emit sideloadFinished(false, tr("设备尚未进入 ADB Sideload 模式。"));
        return;
    }
    if (!QFileInfo::exists(m_adbPath)) {
        emit sideloadFinished(false,
                              tr("未找到 adb.exe：%1")
                                  .arg(QDir::toNativeSeparators(m_adbPath)));
        return;
    }

    const QFileInfo zipFile(zipPath);
    if (!zipFile.exists() || !zipFile.isFile()) {
        emit sideloadFinished(false, tr("所选 ZIP 文件不存在。"));
        return;
    }
    if (zipFile.suffix().compare(QStringLiteral("zip"), Qt::CaseInsensitive) != 0) {
        emit sideloadFinished(false, tr("Recovery 侧载仅支持 .zip 文件。"));
        return;
    }

    m_output.clear();
    m_cancelRequested = false;
    m_lastProgress = -1;
    m_busy = true;
    m_process.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());

    const QString displayCommand = QStringLiteral("adb -s %1 sideload \"%2\"")
                                       .arg(m_deviceSerial,
                                            QDir::toNativeSeparators(zipFile.absoluteFilePath()));
    emit busyChanged(true);
    emit sideloadStarted(displayCommand);
    m_process.start(m_adbPath,
                    {QStringLiteral("-s"),
                     m_deviceSerial,
                     QStringLiteral("sideload"),
                     zipFile.absoluteFilePath()});
}

void RecoveryService::cancelSideload()
{
    if (!m_busy || m_process.state() == QProcess::NotRunning) {
        return;
    }

    m_cancelRequested = true;
    m_process.terminate();
    QTimer::singleShot(1500, this, [this] {
        if (m_process.state() != QProcess::NotRunning) {
            m_process.kill();
        }
    });
}

void RecoveryService::handleReadyRead()
{
    m_output += m_process.readAllStandardOutput();
    const QString output = decodedOutput();
    emit outputChanged(output);

    static const QRegularExpression progressPattern(QStringLiteral("(\\d{1,3})%"));
    QRegularExpressionMatchIterator matches = progressPattern.globalMatch(output);
    int progress = -1;
    while (matches.hasNext()) {
        progress = matches.next().captured(1).toInt();
    }
    if (progress >= 0 && progress <= 100 && progress != m_lastProgress) {
        m_lastProgress = progress;
        emit progressChanged(progress);
    }
}

void RecoveryService::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!m_busy) {
        return;
    }

    handleReadyRead();
    const QString output = decodedOutput().trimmed();
    if (m_cancelRequested) {
        finish(false, tr("侧载已取消。"));
        return;
    }

    const bool success = exitStatus == QProcess::NormalExit && exitCode == 0;
    if (success) {
        finish(true,
               output.isEmpty() ? tr("侧载传输已结束，请在设备端确认结果。") : output);
        return;
    }

    finish(false,
           output.isEmpty() ? tr("侧载失败，退出码：%1").arg(exitCode) : output);
}

void RecoveryService::finish(bool success, const QString &detail)
{
    if (!m_busy && m_process.state() == QProcess::NotRunning) {
        emit sideloadFinished(success, detail);
        return;
    }

    m_busy = false;
    m_cancelRequested = false;
    emit busyChanged(false);
    emit sideloadFinished(success, detail);
}

QString RecoveryService::decodedOutput() const
{
    return QString::fromUtf8(m_output);
}
