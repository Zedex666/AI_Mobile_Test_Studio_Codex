#include "services/file_manager_service.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>
#include <utility>

namespace {

QString adbDisplay(const QString &serial, const QStringList &arguments)
{
    return QStringLiteral("adb -s %1 %2").arg(serial, arguments.join(QLatin1Char(' ')));
}

bool outputIndicatesFailure(const QString &output)
{
    const QString lower = output.toLower();
    return lower.contains(QStringLiteral("permission denied"))
        || lower.contains(QStringLiteral("no such file or directory"))
        || lower.startsWith(QStringLiteral("error:"))
        || lower.startsWith(QStringLiteral("failure"));
}

} // namespace

FileManagerService::FileManagerService(QString adbPath, QObject *parent)
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
            &FileManagerService::handleFinished);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (!m_cancellingProcess && error == QProcess::FailedToStart && m_busy) {
            failCurrent(tr("adb 启动失败：%1").arg(m_process.errorString()));
        }
    });
}

void FileManagerService::setDeviceSerial(const QString &serial)
{
    if (m_deviceSerial == serial) {
        return;
    }

    saveActiveCache();
    m_cancellingProcess = true;
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
    m_cancellingProcess = false;
    m_queue.clear();
    m_current = {};
    m_directoryCache.clear();
    m_refreshAfterQueue = false;
    if (m_busy) {
        m_busy = false;
    }
    if (m_uiBusy) {
        m_uiBusy = false;
        emit busyChanged(false);
    }
    m_deviceSerial = serial;
    restoreActiveCache();
}

bool FileManagerService::busy() const
{
    return m_uiBusy;
}

void FileManagerService::preloadDirectories(const QStringList &paths)
{
    for (const QString &path : paths) {
        const QString normalizedPath = normalizeDirectoryPath(path);
        if (m_directoryCache.contains(normalizedPath)) {
            continue;
        }
        bool alreadyQueued = m_busy && m_current.kind == CommandKind::List
            && m_current.listingPath == normalizedPath;
        for (const PendingCommand &command : std::as_const(m_queue)) {
            alreadyQueued = alreadyQueued
                || (command.kind == CommandKind::List
                    && command.listingPath == normalizedPath);
        }
        if (!alreadyQueued) {
            queueDirectory(normalizedPath, false, false, false);
        }
    }
}

void FileManagerService::listDirectory(const QString &path)
{
    const QString normalizedPath = normalizeDirectoryPath(path);
    const auto cached = m_directoryCache.constFind(normalizedPath);
    if (cached != m_directoryCache.cend()) {
        emit directoryLoaded(normalizedPath, cached.value());
        return;
    }
    queueDirectory(normalizedPath, true, true, false);
}

void FileManagerService::refreshDirectory(const QString &path)
{
    queueDirectory(normalizeDirectoryPath(path), true, true, true);
}

void FileManagerService::queueDirectory(const QString &path,
                                        bool publishResult,
                                        bool notifyUi,
                                        bool forceRefresh)
{
    const QString listingPath = path == QStringLiteral("/")
        ? path
        : path + QLatin1Char('/');
    const QStringList arguments = {QStringLiteral("shell"),
                                   QStringLiteral("ls"),
                                   QStringLiteral("-la"),
                                   quoteRemotePath(listingPath)};
    enqueue({CommandKind::List,
             tr("读取目录"),
             adbDisplay(m_deviceSerial, arguments),
             arguments,
             path,
             false,
             publishResult,
             notifyUi,
             forceRefresh});
}

void FileManagerService::createFolder(const QString &remotePath)
{
    const QStringList arguments = {QStringLiteral("shell"),
                                   QStringLiteral("mkdir"),
                                   QStringLiteral("-p"),
                                   quoteRemotePath(remotePath)};
    enqueue({CommandKind::Action,
             tr("新建文件夹"),
             adbDisplay(m_deviceSerial, arguments),
             arguments,
             QString(),
             true});
}

void FileManagerService::uploadFiles(const QStringList &localPaths,
                                     const QString &remoteDirectory)
{
    for (const QString &localPath : localPaths) {
        const QFileInfo file(localPath);
        if (!file.exists()) {
            emit operationFinished(false,
                                   tr("上传 %1").arg(file.fileName()),
                                   tr("本地文件不存在：%1").arg(localPath));
            continue;
        }
        const QStringList arguments = {QStringLiteral("push"),
                                       file.absoluteFilePath(),
                                       remoteDirectory};
        enqueue({CommandKind::Transfer,
                 tr("上传 %1").arg(file.fileName()),
                 adbDisplay(m_deviceSerial, arguments),
                 arguments,
                 QString(),
                 true});
    }
}

void FileManagerService::downloadFiles(const QStringList &remotePaths,
                                       const QString &localDirectory)
{
    for (const QString &remotePath : remotePaths) {
        const QStringList arguments = {QStringLiteral("pull"), remotePath, localDirectory};
        enqueue({CommandKind::Transfer,
                 tr("下载 %1").arg(QFileInfo(remotePath).fileName()),
                 adbDisplay(m_deviceSerial, arguments),
                 arguments,
                 QString(),
                 false});
    }
}

void FileManagerService::renamePath(const QString &sourcePath,
                                    const QString &destinationPath)
{
    const QStringList arguments = {QStringLiteral("shell"),
                                   QStringLiteral("mv"),
                                   quoteRemotePath(sourcePath),
                                   quoteRemotePath(destinationPath)};
    enqueue({CommandKind::Action,
             tr("重命名"),
             adbDisplay(m_deviceSerial, arguments),
             arguments,
             QString(),
             true});
}

void FileManagerService::duplicatePath(const QString &sourcePath,
                                       const QString &destinationPath)
{
    const QStringList arguments = {QStringLiteral("shell"),
                                   QStringLiteral("cp"),
                                   QStringLiteral("-r"),
                                   quoteRemotePath(sourcePath),
                                   quoteRemotePath(destinationPath)};
    enqueue({CommandKind::Action,
             tr("创建副本"),
             adbDisplay(m_deviceSerial, arguments),
             arguments,
             QString(),
             true});
}

void FileManagerService::changePermissions(const QStringList &remotePaths,
                                           const QString &mode)
{
    for (const QString &remotePath : remotePaths) {
        const QStringList arguments = {QStringLiteral("shell"),
                                       QStringLiteral("chmod"),
                                       mode,
                                       quoteRemotePath(remotePath)};
        enqueue({CommandKind::Action,
                 tr("修改权限"),
                 adbDisplay(m_deviceSerial, arguments),
                 arguments,
                 QString(),
                 true});
    }
}

void FileManagerService::deletePaths(const QStringList &remotePaths)
{
    for (const QString &remotePath : remotePaths) {
        const QStringList arguments = {QStringLiteral("shell"),
                                       QStringLiteral("rm"),
                                       QStringLiteral("-rf"),
                                       quoteRemotePath(remotePath)};
        enqueue({CommandKind::Action,
                 tr("删除 %1").arg(QFileInfo(remotePath).fileName()),
                 adbDisplay(m_deviceSerial, arguments),
                 arguments,
                 QString(),
                 true});
    }
}

void FileManagerService::enqueue(PendingCommand command)
{
    if (m_deviceSerial.isEmpty()) {
        emit operationFinished(false, command.label, tr("当前没有已连接并授权的设备。"));
        return;
    }
    if (!QFileInfo::exists(m_adbPath)) {
        emit operationFinished(false,
                               command.label,
                               tr("未找到 adb.exe：%1")
                                   .arg(QDir::toNativeSeparators(m_adbPath)));
        return;
    }

    m_refreshAfterQueue = m_refreshAfterQueue || command.refreshAfter;
    if (command.notifyUi && !m_uiBusy) {
        m_uiBusy = true;
        emit busyChanged(true);
    }
    m_queue.enqueue(std::move(command));
    if (!m_busy) {
        m_busy = true;
        startNext();
    }
}

void FileManagerService::startNext()
{
    if (m_process.state() != QProcess::NotRunning) {
        return;
    }
    while (!m_queue.isEmpty()) {
        m_current = m_queue.dequeue();
        if (m_current.kind != CommandKind::List || m_current.forceRefresh
            || !m_directoryCache.contains(m_current.listingPath)) {
            break;
        }
        if (m_current.publishResult) {
            emit directoryLoaded(m_current.listingPath,
                                 m_directoryCache.value(m_current.listingPath));
        }
        if (m_current.notifyUi) {
            emit operationFinished(true, m_current.label, tr("已使用本次运行缓存。"));
        }
        m_current = {};
    }
    if (m_queue.isEmpty() && m_current.arguments.isEmpty()) {
        completeQueue();
        return;
    }

    m_output.clear();
    m_process.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    if (m_current.notifyUi) {
        emit operationStarted(m_current.label, m_current.displayCommand);
    }
    m_process.start(m_adbPath, m_current.arguments);
}

void FileManagerService::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_cancellingProcess || !m_busy) {
        m_process.readAllStandardOutput();
        return;
    }

    m_output += m_process.readAllStandardOutput();
    const QString output = QString::fromUtf8(m_output).trimmed();
    const bool success = exitStatus == QProcess::NormalExit && exitCode == 0
        && !outputIndicatesFailure(output);
    if (!success) {
        if (!m_current.notifyUi) {
            m_current = {};
            startNext();
            return;
        }
        failCurrent(output.isEmpty()
                        ? tr("命令执行失败，退出码：%1").arg(exitCode)
                        : output);
        return;
    }

    if (m_current.kind == CommandKind::List) {
        const QVector<DeviceFileEntry> entries = parseDirectory(output);
        m_directoryCache.insert(m_current.listingPath, entries);
        saveActiveCache();
        if (m_current.publishResult) {
            emit directoryLoaded(m_current.listingPath, entries);
        }
    } else if (m_current.refreshAfter) {
        m_directoryCache.clear();
        saveActiveCache();
    }
    if (m_current.notifyUi) {
        emit operationFinished(true,
                               m_current.label,
                               output.isEmpty() ? tr("操作完成。") : output);
    }
    m_current = {};
    startNext();
}

void FileManagerService::failCurrent(const QString &detail)
{
    const QString label = m_current.label;
    m_queue.clear();
    m_refreshAfterQueue = false;
    m_busy = false;
    m_current = {};
    if (m_uiBusy) {
        m_uiBusy = false;
        emit busyChanged(false);
    }
    emit operationFinished(false, label, detail);
}

void FileManagerService::completeQueue()
{
    const bool refresh = m_refreshAfterQueue;
    m_refreshAfterQueue = false;
    m_busy = false;
    m_current = {};
    if (m_uiBusy) {
        m_uiBusy = false;
        emit busyChanged(false);
    }
    if (refresh) {
        emit refreshRequested();
    }
}

QString FileManagerService::quoteRemotePath(const QString &path)
{
    QString escaped = path;
    escaped.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + escaped + QLatin1Char('\'');
}

QString FileManagerService::normalizeDirectoryPath(const QString &path)
{
    QString normalized = QDir::cleanPath(path.trimmed());
    normalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (normalized.isEmpty() || normalized == QStringLiteral(".")) {
        return QStringLiteral("/");
    }
    if (!normalized.startsWith(QLatin1Char('/'))) {
        normalized.prepend(QLatin1Char('/'));
    }
    return normalized;
}

void FileManagerService::saveActiveCache()
{
    if (!m_deviceSerial.isEmpty()) {
        m_deviceCaches.insert(m_deviceSerial, m_directoryCache);
    }
}

void FileManagerService::restoreActiveCache()
{
    if (!m_deviceSerial.isEmpty()) {
        m_directoryCache = m_deviceCaches.value(m_deviceSerial);
    }
}

QVector<DeviceFileEntry> FileManagerService::parseDirectory(const QString &output)
{
    QVector<DeviceFileEntry> entries;
    const QStringList lines = output.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                           Qt::SkipEmptyParts);
    static const QRegularExpression standardPattern(
        QStringLiteral("^\\s*([dl-]\\S*)\\s+\\S+\\s+\\S+\\s+\\S+\\s+(\\d+)\\s+(\\S+)\\s+(\\S+)\\s+(.+?)\\s*$"));
    static const QRegularExpression restrictedPattern(
        QStringLiteral("^\\s*([dl-]\\?+)\\s+\\?\\s+\\?\\s+\\?\\s+\\?\\s+\\?\\s+(.+?)\\s*$"));
    for (const QString &line : lines) {
        QString permissions;
        QString rawName;
        QString modified;
        qint64 size = 0;

        const QRegularExpressionMatch standardMatch = standardPattern.match(line);
        if (standardMatch.hasMatch()) {
            permissions = standardMatch.captured(1);
            size = standardMatch.captured(2).toLongLong();
            modified = standardMatch.captured(3) + QLatin1Char(' ')
                + standardMatch.captured(4);
            rawName = standardMatch.captured(5);
        } else {
            const QRegularExpressionMatch restrictedMatch = restrictedPattern.match(line);
            if (!restrictedMatch.hasMatch()) {
                continue;
            }
            permissions = restrictedMatch.captured(1);
            modified = QStringLiteral("-");
            rawName = restrictedMatch.captured(2);
        }

        const QChar type = permissions.front();
        if (type != QLatin1Char('d') && type != QLatin1Char('-') && type != QLatin1Char('l')) {
            continue;
        }

        QString linkTarget;
        if (type == QLatin1Char('l')) {
            const int separator = rawName.indexOf(QStringLiteral(" -> "));
            if (separator >= 0) {
                linkTarget = rawName.mid(separator + 4);
                rawName = rawName.left(separator);
            }
        }
        if (rawName == QStringLiteral(".") || rawName == QStringLiteral("..")) {
            continue;
        }

        DeviceFileEntry entry;
        entry.name = rawName;
        entry.permissions = permissions;
        entry.size = size;
        entry.modified = modified;
        entry.isDirectory = type == QLatin1Char('d');
        entry.isLink = type == QLatin1Char('l');
        entry.linkTarget = linkTarget;
        entries.append(entry);
    }

    std::sort(entries.begin(), entries.end(), [](const DeviceFileEntry &left,
                                                  const DeviceFileEntry &right) {
        if (left.isDirectory != right.isDirectory) {
            return left.isDirectory;
        }
        return left.name.compare(right.name, Qt::CaseInsensitive) < 0;
    });
    return entries;
}
