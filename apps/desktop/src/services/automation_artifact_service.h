#ifndef AI_MOBILE_TEST_STUDIO_AUTOMATION_ARTIFACT_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_AUTOMATION_ARTIFACT_SERVICE_H

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

struct AutomationArtifact {
    QString absolutePath;
    QString relativePath;
    QString name;
    QString categoryId;
    qint64 size = 0;
    QDateTime modifiedAt;
};

class QFileSystemWatcher;
class QTimer;

class AutomationArtifactService : public QObject
{
    Q_OBJECT

public:
    explicit AutomationArtifactService(QObject *parent = nullptr);

    void setWorkspaceDirectory(const QString &workspaceDirectory);
    QString workspaceDirectory() const;
    QString automationDirectory() const;
    QString scriptsDirectory() const;
    QString reportsDirectory() const;
    QString assetsDirectory() const;
    QString runsDirectory() const;

    bool ensureDirectories(QString *error = nullptr) const;
    const QVector<AutomationArtifact> &artifacts() const;

public slots:
    void refresh();

signals:
    void artifactsChanged();
    void refreshFailed(const QString &message);

private:
    void watchDirectories();
    void scheduleRefresh();
    void scanDirectory(const QString &directory,
                       const QString &categoryId,
                       QVector<AutomationArtifact> *results) const;

    QString m_workspaceDirectory;
    QVector<AutomationArtifact> m_artifacts;
    QFileSystemWatcher *m_watcher = nullptr;
    QTimer *m_refreshTimer = nullptr;
};

#endif // AI_MOBILE_TEST_STUDIO_AUTOMATION_ARTIFACT_SERVICE_H
