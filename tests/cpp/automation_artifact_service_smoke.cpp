#include "services/automation_artifact_service.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include <iostream>

namespace {

bool require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << message << std::endl;
    }
    return condition;
}

bool writeFile(const QString &path, const QByteArray &content)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    return file.write(content) == content.size();
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    QTemporaryDir workspace;
    if (!require(workspace.isValid(), "Temporary workspace was not created.")) {
        return 1;
    }

    AutomationArtifactService service;
    service.setWorkspaceDirectory(workspace.path());
    if (!require(QFile::exists(service.scriptsDirectory())
                     && QFile::exists(service.reportsDirectory())
                     && QFile::exists(service.assetsDirectory())
                     && QFile::exists(service.runsDirectory()),
                 "Automation directory structure was not created.")) {
        return 2;
    }

    const QString scriptPath = QDir(service.scriptsDirectory()).filePath(QStringLiteral("flow.html"));
    const QString reportPath = QDir(service.reportsDirectory()).filePath(QStringLiteral("result.htm"));
    const QString ignoredPath = QDir(service.assetsDirectory()).filePath(QStringLiteral("ignored.html"));
    if (!require(writeFile(scriptPath, QByteArrayLiteral("<html>script</html>"))
                     && writeFile(reportPath, QByteArrayLiteral("<html>report</html>"))
                     && writeFile(ignoredPath, QByteArrayLiteral("<html>asset</html>")),
                 "Automation fixtures were not written.")) {
        return 3;
    }

    service.refresh();
    if (!require(service.artifacts().size() == 2,
                 "Only script and report HTML artifacts should be listed.")) {
        return 4;
    }

    bool foundScript = false;
    bool foundReport = false;
    for (const AutomationArtifact &artifact : service.artifacts()) {
        foundScript = foundScript
            || (artifact.absolutePath == QFileInfo(scriptPath).canonicalFilePath()
                && artifact.categoryId == QStringLiteral("scripts"));
        foundReport = foundReport
            || (artifact.absolutePath == QFileInfo(reportPath).canonicalFilePath()
                && artifact.categoryId == QStringLiteral("reports"));
    }
    if (!require(foundScript && foundReport,
                 "Automation artifacts were not categorized correctly.")) {
        return 5;
    }

    return 0;
}
