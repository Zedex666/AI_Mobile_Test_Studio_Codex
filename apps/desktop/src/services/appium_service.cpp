#include "services/appium_service.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QStandardPaths>

#include <utility>

namespace {

constexpr int kInitialProbeLimit = 3;
constexpr int kProbeTimeoutMs = 900;
constexpr int kInitialProbeDelayMs = 250;
constexpr int kBundledProbeDelayMs = 400;
constexpr int kBundledStartupTimeoutMs = 15000;
constexpr int kProcessOutputLimit = 12000;

QUrl defaultStatusUrl()
{
    return QUrl(QStringLiteral("http://127.0.0.1:4723/status"));
}

bool isWebDriverStatusResponse(QNetworkReply *reply, const QByteArray &body)
{
    if (reply->error() != QNetworkReply::NoError) {
        return false;
    }
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode < 200 || statusCode >= 300) {
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    return parseError.error == QJsonParseError::NoError && document.isObject()
        && document.object().value(QStringLiteral("value")).isObject();
}

QString cleanAbsolutePath(const QString &path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

} // namespace

AppiumService::AppiumService(QString runtimeRoot, QObject *parent)
    : AppiumService(std::move(runtimeRoot), defaultStatusUrl(), parent)
{
}

AppiumService::AppiumService(QString runtimeRoot, QUrl statusUrl, QObject *parent)
    : QObject(parent)
    , m_runtimeRoot(cleanAbsolutePath(std::move(runtimeRoot)))
    , m_statusUrl(std::move(statusUrl))
    , m_networkManager(this)
    , m_probeTimeout(this)
    , m_probeDelay(this)
    , m_process(this)
{
    m_probeTimeout.setSingleShot(true);
    m_probeDelay.setSingleShot(true);
    m_process.setProcessChannelMode(QProcess::MergedChannels);

    connect(&m_probeTimeout, &QTimer::timeout, this, [this] {
        if (m_probeReply != nullptr) {
            m_probeReply->abort();
        }
    });
    connect(&m_probeDelay, &QTimer::timeout, this, &AppiumService::probeStatus);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &AppiumService::appendProcessOutput);
    connect(&m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                appendProcessOutput();
                if (m_stopping) {
                    return;
                }
                m_probeDelay.stop();
                m_waitingForBundledServer = false;
                const QString output = recentProcessOutput();
                const QString message = output.isEmpty()
                    ? tr("Bundled Appium exited before becoming ready (code %1, status %2).")
                          .arg(exitCode)
                          .arg(static_cast<int>(exitStatus))
                    : tr("Bundled Appium exited before becoming ready:\n%1").arg(output);
                setState(State::Failed, message);
            });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && !m_stopping) {
            m_probeDelay.stop();
            m_waitingForBundledServer = false;
            setState(State::Failed,
                     tr("Unable to start bundled Appium: %1").arg(m_process.errorString()));
        }
    });
}

AppiumService::~AppiumService()
{
    if (m_probeReply != nullptr) {
        m_probeReply->abort();
    }
    stopOwnedServer();
}

AppiumService::State AppiumService::state() const
{
    return m_state;
}

bool AppiumService::ownsServerProcess() const
{
    return m_process.state() != QProcess::NotRunning;
}

QString AppiumService::detail() const
{
    return m_detail;
}

void AppiumService::ensureStarted()
{
    if (m_state == State::Probing || m_state == State::ReusingExisting
        || m_state == State::StartingBundled || m_state == State::RunningBundled) {
        return;
    }

    m_initialProbeAttempts = 0;
    m_waitingForBundledServer = false;
    setState(State::Probing, tr("Checking for an existing Appium server."));
    probeStatus();
}

void AppiumService::probeStatus()
{
    if (m_probeReply != nullptr) {
        return;
    }

    QNetworkRequest request(m_statusUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("AI-Mobile-Test-Studio-Appium-Probe"));
    QNetworkReply *reply = m_networkManager.get(request);
    m_probeReply = reply;
    m_probeTimeout.start(kProbeTimeoutMs);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        handleProbeFinished(reply);
    });
}

void AppiumService::handleProbeFinished(QNetworkReply *reply)
{
    if (reply != m_probeReply) {
        reply->deleteLater();
        return;
    }

    m_probeTimeout.stop();
    m_probeReply = nullptr;
    const QByteArray body = reply->readAll();
    const bool available = isWebDriverStatusResponse(reply, body);
    reply->deleteLater();

    if (available) {
        m_probeDelay.stop();
        m_waitingForBundledServer = false;
        if (m_process.state() == QProcess::NotRunning) {
            setState(State::ReusingExisting,
                     tr("Using the Appium server already running at %1.")
                         .arg(m_statusUrl.adjusted(QUrl::RemovePath | QUrl::RemoveQuery
                                                  | QUrl::RemoveFragment)
                                  .toString()));
        } else {
            setState(State::RunningBundled,
                     tr("Bundled Appium is ready at %1.")
                         .arg(m_statusUrl.adjusted(QUrl::RemovePath | QUrl::RemoveQuery
                                                  | QUrl::RemoveFragment)
                                  .toString()));
        }
        return;
    }

    if (m_waitingForBundledServer) {
        if (m_process.state() == QProcess::NotRunning) {
            return;
        }
        if (m_startupTimer.hasExpired(kBundledStartupTimeoutMs)) {
            const QString output = recentProcessOutput();
            stopOwnedServer();
            setState(State::Failed,
                     output.isEmpty()
                         ? tr("Bundled Appium did not become ready within %1 seconds.")
                               .arg(kBundledStartupTimeoutMs / 1000)
                         : tr("Bundled Appium did not become ready:\n%1").arg(output));
            return;
        }
        scheduleProbe(kBundledProbeDelayMs);
        return;
    }

    ++m_initialProbeAttempts;
    if (m_initialProbeAttempts < kInitialProbeLimit) {
        scheduleProbe(kInitialProbeDelayMs);
        return;
    }
    startBundledServer();
}

void AppiumService::scheduleProbe(int delayMs)
{
    if (!m_probeDelay.isActive()) {
        m_probeDelay.start(delayMs);
    }
}

void AppiumService::startBundledServer()
{
    const QString nodeExecutable = QDir(m_runtimeRoot).filePath(QStringLiteral("node/node.exe"));
    const QString appiumEntry = QDir(m_runtimeRoot).filePath(
        QStringLiteral("appium/node_modules/appium/index.js"));
    const QString driverPackage = QDir(m_runtimeRoot).filePath(
        QStringLiteral("appium/node_modules/appium-uiautomator2-driver/package.json"));
    const QString jdkRoot = QDir(m_runtimeRoot).filePath(QStringLiteral("jdk"));
    const QString androidSdkRoot = QDir(m_runtimeRoot).filePath(QStringLiteral("android-sdk"));
    const QString adbExecutable = QDir(androidSdkRoot).filePath(
        QStringLiteral("platform-tools/adb.exe"));

    const QStringList requiredFiles = {
        nodeExecutable,
        appiumEntry,
        driverPackage,
        QDir(jdkRoot).filePath(QStringLiteral("bin/java.exe")),
        adbExecutable,
    };
    for (const QString &path : requiredFiles) {
        if (!QFileInfo::exists(path)) {
            setState(State::Failed,
                     tr("Bundled Appium runtime is incomplete: %1")
                         .arg(QDir::toNativeSeparators(path)));
            return;
        }
    }

    const QString dataRoot = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString appiumHome = QDir(dataRoot).filePath(QStringLiteral("appium-runtime"));
    QString manifestError;
    if (!writeDriverManifest(appiumHome, &manifestError)) {
        setState(State::Failed, manifestError);
        return;
    }

    const QString condaRoot = QDir(m_runtimeRoot).filePath(QStringLiteral("conda"));
    const QStringList privatePaths = {
        QDir(m_runtimeRoot).filePath(QStringLiteral("node")),
        QDir(androidSdkRoot).filePath(QStringLiteral("platform-tools")),
        QDir(androidSdkRoot).filePath(QStringLiteral("cmdline-tools/latest/bin")),
        QDir(jdkRoot).filePath(QStringLiteral("bin")),
        condaRoot,
    };
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const QString inheritedPath = environment.value(QStringLiteral("PATH"));
    environment.insert(QStringLiteral("PATH"),
                       privatePaths.join(QDir::listSeparator()) + QDir::listSeparator()
                           + inheritedPath);
    environment.insert(QStringLiteral("APPIUM_HOME"), appiumHome);
    environment.insert(QStringLiteral("JAVA_HOME"), jdkRoot);
    environment.insert(QStringLiteral("ANDROID_HOME"), androidSdkRoot);
    environment.insert(QStringLiteral("ANDROID_SDK_ROOT"), androidSdkRoot);
    environment.insert(QStringLiteral("CONDA_PKGS_DIRS"),
                       QDir(dataRoot).filePath(QStringLiteral("conda/pkgs")));
    environment.insert(QStringLiteral("CONDARC"),
                       QDir(dataRoot).filePath(QStringLiteral("conda/condarc")));
    environment.insert(QStringLiteral("NPM_CONFIG_CACHE"),
                       QDir(dataRoot).filePath(QStringLiteral("npm/cache")));
    environment.insert(QStringLiteral("NPM_CONFIG_USERCONFIG"),
                       QDir(dataRoot).filePath(QStringLiteral("npm/npmrc")));

    const QString host = m_statusUrl.host().isEmpty() ? QStringLiteral("127.0.0.1")
                                                       : m_statusUrl.host();
    const int port = m_statusUrl.port(4723);
    m_processOutput.clear();
    m_process.setProcessEnvironment(environment);
    m_process.setWorkingDirectory(appiumHome);
    m_process.setProgram(nodeExecutable);
    m_process.setArguments({appiumEntry,
                            QStringLiteral("--address"),
                            host,
                            QStringLiteral("--port"),
                            QString::number(port),
                            QStringLiteral("--base-path"),
                            QStringLiteral("/")});
    m_waitingForBundledServer = true;
    m_startupTimer.restart();
    setState(State::StartingBundled, tr("Starting the bundled Appium server."));
    m_process.start();
    scheduleProbe(kBundledProbeDelayMs);
}

bool AppiumService::writeDriverManifest(const QString &appiumHome,
                                        QString *errorMessage) const
{
    const QString driverRoot = QDir(m_runtimeRoot).filePath(
        QStringLiteral("appium/node_modules/appium-uiautomator2-driver"));
    const QString packagePath = QDir(driverRoot).filePath(QStringLiteral("package.json"));
    QFile packageFile(packagePath);
    if (!packageFile.open(QIODevice::ReadOnly)) {
        *errorMessage = tr("Unable to read bundled UiAutomator2 metadata: %1")
                            .arg(packageFile.errorString());
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument packageDocument = QJsonDocument::fromJson(packageFile.readAll(),
                                                                  &parseError);
    if (parseError.error != QJsonParseError::NoError || !packageDocument.isObject()) {
        *errorMessage = tr("Bundled UiAutomator2 metadata is invalid: %1")
                            .arg(parseError.errorString());
        return false;
    }

    const QJsonObject package = packageDocument.object();
    QJsonObject driver = package.value(QStringLiteral("appium")).toObject();
    const QString driverName = driver.take(QStringLiteral("driverName")).toString();
    if (driverName.isEmpty()) {
        *errorMessage = tr("Bundled UiAutomator2 metadata does not declare a driver name.");
        return false;
    }
    driver.insert(QStringLiteral("pkgName"), package.value(QStringLiteral("name")));
    driver.insert(QStringLiteral("version"), package.value(QStringLiteral("version")));
    driver.insert(QStringLiteral("installType"), QStringLiteral("npm"));
    driver.insert(QStringLiteral("installSpec"), driverName);
    driver.insert(QStringLiteral("installPath"), cleanAbsolutePath(driverRoot));
    driver.insert(QStringLiteral("appiumVersion"),
                  package.value(QStringLiteral("peerDependencies"))
                      .toObject()
                      .value(QStringLiteral("appium")));

    QJsonObject drivers;
    drivers.insert(driverName, driver);
    QJsonObject manifest;
    manifest.insert(QStringLiteral("drivers"), drivers);
    manifest.insert(QStringLiteral("plugins"), QJsonObject());
    manifest.insert(QStringLiteral("schemaRev"), 4);

    const QString cacheDirectory = QDir(appiumHome).filePath(
        QStringLiteral("node_modules/.cache/appium"));
    if (!QDir().mkpath(cacheDirectory)) {
        *errorMessage = tr("Unable to create the isolated Appium data directory: %1")
                            .arg(QDir::toNativeSeparators(cacheDirectory));
        return false;
    }

    QSaveFile manifestFile(QDir(cacheDirectory).filePath(QStringLiteral("extensions.yaml")));
    if (!manifestFile.open(QIODevice::WriteOnly)) {
        *errorMessage = tr("Unable to write isolated Appium driver metadata: %1")
                            .arg(manifestFile.errorString());
        return false;
    }
    manifestFile.write(QJsonDocument(manifest).toJson(QJsonDocument::Indented));
    if (!manifestFile.commit()) {
        *errorMessage = tr("Unable to commit isolated Appium driver metadata: %1")
                            .arg(manifestFile.errorString());
        return false;
    }
    return true;
}

void AppiumService::stopOwnedServer()
{
    m_probeDelay.stop();
    m_probeTimeout.stop();
    m_waitingForBundledServer = false;
    if (m_process.state() == QProcess::NotRunning) {
        return;
    }

    m_stopping = true;
    m_process.terminate();
    if (!m_process.waitForFinished(2000)) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
    m_stopping = false;
}

void AppiumService::setState(State state, const QString &detail)
{
    if (m_state == state && m_detail == detail) {
        return;
    }
    m_state = state;
    m_detail = detail;
    emit stateChanged(state, detail);
}

void AppiumService::appendProcessOutput()
{
    m_processOutput += QString::fromLocal8Bit(m_process.readAllStandardOutput());
    if (m_processOutput.size() > kProcessOutputLimit) {
        m_processOutput = m_processOutput.right(kProcessOutputLimit);
    }
}

QString AppiumService::recentProcessOutput() const
{
    return m_processOutput.trimmed().right(4000);
}
