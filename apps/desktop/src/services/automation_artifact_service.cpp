#include "services/automation_artifact_service.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QTimer>

#include <algorithm>
#include <utility>

namespace {

const QStringList kAutomationDirectories = {
    QStringLiteral("scripts"),
    QStringLiteral("reports"),
    QStringLiteral("assets"),
    QStringLiteral("runs")};

bool isChildPath(const QString &path, const QString &parent)
{
    const QString normalizedPath = QDir::fromNativeSeparators(QDir::cleanPath(path));
    const QString normalizedParent = QDir::fromNativeSeparators(QDir::cleanPath(parent));
#ifdef Q_OS_WIN
    constexpr Qt::CaseSensitivity pathCaseSensitivity = Qt::CaseInsensitive;
#else
    constexpr Qt::CaseSensitivity pathCaseSensitivity = Qt::CaseSensitive;
#endif
    if (normalizedPath.compare(normalizedParent, pathCaseSensitivity) == 0) {
        return true;
    }
    return normalizedPath.startsWith(normalizedParent + QLatin1Char('/'), pathCaseSensitivity);
}

} // namespace

AutomationArtifactService::AutomationArtifactService(QObject *parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
    , m_refreshTimer(new QTimer(this))
{
    m_refreshTimer->setInterval(2000);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] {
        scheduleRefresh();
    });
    connect(m_refreshTimer, &QTimer::timeout, this, &AutomationArtifactService::refresh);
    m_refreshTimer->start();
}

void AutomationArtifactService::setWorkspaceDirectory(const QString &workspaceDirectory)
{
    const QString normalized = QDir::cleanPath(QFileInfo(workspaceDirectory).absoluteFilePath());
    if (normalized == m_workspaceDirectory) {
        ensureDirectories();
        watchDirectories();
        refresh();
        return;
    }

    m_workspaceDirectory = normalized;
    const QStringList watched = m_watcher->directories();
    if (!watched.isEmpty()) {
        m_watcher->removePaths(watched);
    }
    ensureDirectories();
    watchDirectories();
    refresh();
}

QString AutomationArtifactService::workspaceDirectory() const
{
    return m_workspaceDirectory;
}

QString AutomationArtifactService::automationDirectory() const
{
    return QDir(m_workspaceDirectory).filePath(QStringLiteral("automation"));
}

QString AutomationArtifactService::scriptsDirectory() const
{
    return QDir(automationDirectory()).filePath(QStringLiteral("scripts"));
}

QString AutomationArtifactService::reportsDirectory() const
{
    return QDir(automationDirectory()).filePath(QStringLiteral("reports"));
}

QString AutomationArtifactService::assetsDirectory() const
{
    return QDir(automationDirectory()).filePath(QStringLiteral("assets"));
}

QString AutomationArtifactService::runsDirectory() const
{
    return QDir(automationDirectory()).filePath(QStringLiteral("runs"));
}

bool AutomationArtifactService::ensureDirectories(QString *error) const
{
    if (m_workspaceDirectory.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("Automation workspace directory is empty.");
        }
        return false;
    }
    QDir automation(automationDirectory());
    if (!automation.mkpath(QStringLiteral("."))) {
        if (error != nullptr) {
            *error = QStringLiteral("Unable to create automation directory: %1")
                         .arg(automationDirectory());
        }
        return false;
    }
    for (const QString &directory : kAutomationDirectories) {
        if (!automation.mkpath(directory)) {
            if (error != nullptr) {
                *error = QStringLiteral("Unable to create automation directory: %1")
                             .arg(automation.filePath(directory));
            }
            return false;
        }
    }
    return true;
}

const QVector<AutomationArtifact> &AutomationArtifactService::artifacts() const
{
    return m_artifacts;
}

void AutomationArtifactService::refresh()
{
    QString error;
    if (!ensureDirectories(&error)) {
        emit refreshFailed(error);
        return;
    }

    QVector<AutomationArtifact> next;
    scanDirectory(scriptsDirectory(), QStringLiteral("scripts"), &next);
    scanDirectory(reportsDirectory(), QStringLiteral("reports"), &next);
    std::sort(next.begin(), next.end(), [](const AutomationArtifact &left,
                                           const AutomationArtifact &right) {
        if (left.modifiedAt != right.modifiedAt) {
            return left.modifiedAt > right.modifiedAt;
        }
        return left.relativePath < right.relativePath;
    });

    m_artifacts = std::move(next);
    emit artifactsChanged();
    watchDirectories();
    m_refreshTimer->start(2000);
}

void AutomationArtifactService::watchDirectories()
{
    if (m_workspaceDirectory.isEmpty()) {
        return;
    }
    QStringList paths;
    paths.append(automationDirectory());
    paths.append(scriptsDirectory());
    paths.append(reportsDirectory());
    paths.append(assetsDirectory());
    paths.append(runsDirectory());
    for (const QString &path : paths) {
        if (QFileInfo(path).isDir() && !m_watcher->directories().contains(path)) {
            m_watcher->addPath(path);
        }
    }
}

void AutomationArtifactService::scheduleRefresh()
{
    m_refreshTimer->start(150);
}

void AutomationArtifactService::scanDirectory(const QString &directory,
                                              const QString &categoryId,
                                              QVector<AutomationArtifact> *results) const
{
    if (results == nullptr) {
        return;
    }
    const QString canonicalRoot = QFileInfo(automationDirectory()).canonicalFilePath();
    if (canonicalRoot.isEmpty()) {
        return;
    }
    QDirIterator iterator(directory,
                          {QStringLiteral("*.html"), QStringLiteral("*.htm")},
                          QDir::Files | QDir::Readable,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QFileInfo info(iterator.next());
        const QString canonicalPath = info.canonicalFilePath();
        if (canonicalPath.isEmpty() || !isChildPath(canonicalPath, canonicalRoot)) {
            continue;
        }
        const QString relativePath = QDir::fromNativeSeparators(
            QDir(automationDirectory()).relativeFilePath(canonicalPath));
        if (!relativePath.startsWith(categoryId + QLatin1Char('/'))
            && relativePath != categoryId) {
            continue;
        }
        results->append({canonicalPath,
                         relativePath,
                         info.fileName(),
                         categoryId,
                         info.size(),
                         info.lastModified()});
    }
}
