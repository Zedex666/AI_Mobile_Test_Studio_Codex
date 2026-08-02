#include "services/apps_service.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>

#include <algorithm>
#include <utility>

namespace {

const QString kSeparator = QStringLiteral("---ADBAPPSEP---");
const QString kMetadataDevicePath = QStringLiteral(
    "/data/local/tmp/ai_mobile_test_studio_app_metadata.jar");
const QString kMetadataMainClass = QStringLiteral(
    "com.ai_mobile_test_studio.appmetadata.Main");
constexpr int kMetadataBatchSize = 24;

QStringList adbArguments(const QString &serial, const QStringList &arguments)
{
    QStringList result = {QStringLiteral("-s"), serial};
    result.append(arguments);
    return result;
}

QString displayCommand(const QString &serial, const QStringList &arguments)
{
    return QStringLiteral("adb -s %1 %2").arg(serial, arguments.join(QLatin1Char(' ')));
}

bool outputIndicatesFailure(const QString &output)
{
    const QString lower = output.trimmed().toLower();
    return lower.startsWith(QStringLiteral("failure"))
        || lower.startsWith(QStringLiteral("error"))
        || lower.contains(QStringLiteral("permission denied"))
        || lower.contains(QStringLiteral("unknown package"));
}

QSet<QString> packageSet(const QString &output)
{
    QSet<QString> packages;
    for (const QString &line : output.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                             Qt::SkipEmptyParts)) {
        QString value = line.trimmed();
        if (!value.startsWith(QStringLiteral("package:"))) {
            continue;
        }
        value.remove(0, QStringLiteral("package:").size());
        const int separator = value.lastIndexOf(QLatin1Char('='));
        packages.insert(separator >= 0 ? value.mid(separator + 1) : value);
    }
    return packages;
}

QString displayNameFromDump(const QString &output, const QString &fallback)
{
    for (const QString &line : output.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                             Qt::SkipEmptyParts)) {
        const QString trimmed = line.trimmed();
        QString label;
        if (trimmed.startsWith(QStringLiteral("application-label:"))) {
            label = trimmed.mid(QStringLiteral("application-label:").size()).trimmed();
        } else {
            const int position = trimmed.indexOf(QStringLiteral("nonLocalizedLabel="));
            if (position >= 0) {
                label = trimmed.mid(position + QStringLiteral("nonLocalizedLabel=").size())
                            .section(QStringLiteral(" icon="), 0, 0)
                            .trimmed();
            }
        }
        if (label.size() >= 2
            && ((label.startsWith(QLatin1Char('\'')) && label.endsWith(QLatin1Char('\'')))
                || (label.startsWith(QLatin1Char('"')) && label.endsWith(QLatin1Char('"'))))) {
            label = label.mid(1, label.size() - 2);
        }
        if (!label.isEmpty() && label != QStringLiteral("null")) {
            return label;
        }
    }
    return fallback;
}

} // namespace

AppsService::AppsService(QString adbPath, QObject *parent)
    : QObject(parent)
    , m_adbPath(QDir::cleanPath(std::move(adbPath)))
    , m_metadataJarPath(QDir(QCoreApplication::applicationDirPath())
                            .filePath(QStringLiteral("runtime/android/app_metadata.jar")))
    , m_process(this)
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] {
        m_output += m_process.readAllStandardOutput();
    });
    connect(&m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &AppsService::handleFinished);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (!m_cancellingProcess && error == QProcess::FailedToStart
            && m_request != Request::Idle) {
            failRequest(tr("adb 启动失败：%1").arg(m_process.errorString()));
        }
    });
}

void AppsService::setDeviceSerial(const QString &serial)
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
    if (m_request != Request::Idle) {
        finishRequest();
    }
    resetInstallBatch();
    resetMetadataLoad();
    m_queuedMetadataPackages.clear();
    m_labelCache.clear();
    m_iconCache.clear();
    m_cachedApps.clear();
    m_deviceSerial = serial;
    restoreActiveCache();
}

bool AppsService::busy() const
{
    return m_request != Request::Idle;
}

void AppsService::preloadApps()
{
    if (m_deviceSerial.isEmpty() || busy()) {
        return;
    }
    if (m_cachedApps.isEmpty()) {
        loadApps();
        return;
    }

    for (AndroidAppSummary &app : m_cachedApps) {
        app.displayName = m_labelCache.value(app.packageName, app.displayName);
        app.iconPng = m_iconCache.value(app.packageName, app.iconPng);
    }
    emit appsLoaded(m_cachedApps);
    emit appMetadataLoaded(m_cachedApps);

    const bool hasMissingIcons = std::any_of(
        m_cachedApps.cbegin(), m_cachedApps.cend(), [](const AndroidAppSummary &app) {
            return !app.uninstalled && app.iconPng.isEmpty();
        });
    if (hasMissingIcons) {
        beginMetadataLoad(m_cachedApps, true);
    }
}

void AppsService::loadApps()
{
    const QString script = QStringLiteral(
        "pm list packages -f -u; echo '---ADBAPPSEP---'; "
        "pm list packages -s; echo '---ADBAPPSEP---'; "
        "pm list packages -d; echo '---ADBAPPSEP---'; "
        "pm list packages -f || true");
    start(Request::AppList,
          tr("刷新应用列表"),
          {QStringLiteral("shell"), script});
}

void AppsService::loadAppMetadata(const QStringList &packageNames)
{
    QSet<QString> uniquePackages;
    QVector<AndroidAppSummary> cachedApps;
    QStringList missingPackages;
    for (const QString &packageName : packageNames) {
        if (!validPackageName(packageName) || uniquePackages.contains(packageName)) {
            continue;
        }
        uniquePackages.insert(packageName);

        AndroidAppSummary app;
        app.packageName = packageName;
        app.displayName = m_labelCache.value(packageName, packageName);
        app.iconPng = m_iconCache.value(packageName);
        if (!app.iconPng.isEmpty()) {
            cachedApps.append(std::move(app));
        } else {
            missingPackages.append(packageName);
        }
    }

    if (!cachedApps.isEmpty()) {
        emit appMetadataLoaded(cachedApps);
    }
    if (missingPackages.isEmpty()) {
        return;
    }
    if (busy()) {
        for (const QString &packageName : std::as_const(missingPackages)) {
            if (!m_queuedMetadataPackages.contains(packageName)) {
                m_queuedMetadataPackages.append(packageName);
            }
        }
        return;
    }

    QVector<AndroidAppSummary> apps;
    apps.reserve(missingPackages.size());
    for (const QString &packageName : std::as_const(missingPackages)) {
        AndroidAppSummary app;
        app.packageName = packageName;
        app.displayName = m_labelCache.value(packageName, packageName);
        apps.append(std::move(app));
        m_queuedMetadataPackages.removeAll(packageName);
    }
    beginMetadataLoad(std::move(apps), false);
}

void AppsService::loadAppDetails(const QString &packageName)
{
    if (!validPackageName(packageName)) {
        emit operationFinished(false, tr("读取应用详情"), tr("无效的软件包名称。"));
        return;
    }
    m_currentPackage = packageName;
    const QString script = QStringLiteral(
        "PKG='%1'; dumpsys package \"$PKG\"; echo '---ADBAPPSEP---'; "
        "pm path \"$PKG\"; echo '---ADBAPPSEP---'; "
        "pm list packages -s \"$PKG\"; echo '---ADBAPPSEP---'; "
         "pm list packages -d \"$PKG\"; echo '---ADBAPPSEP---'; "
         "cmd appops get \"$PKG\"; echo '---ADBAPPSEP---'; "
         "pm list permissions -g -d; echo '---ADBAPPSEP---'; "
         "APK=$(pm path \"$PKG\" | sed 's/package://' | head -n 1); "
        "if [ -z \"$APK\" ]; then APK=/dev/null; fi; du -sk \"$APK\" 2>/dev/null || true")
                               .arg(packageName);
    start(Request::AppDetails,
          tr("读取 %1 详情").arg(packageName),
          {QStringLiteral("shell"), script});
}

void AppsService::installPackages(const QStringList &apkFiles,
                                  bool replaceExisting,
                                  bool grantPermissions,
                                  bool bypassLowTargetSdk)
{
    if (apkFiles.isEmpty()) {
        return;
    }
    for (const QString &file : apkFiles) {
        if (!QFileInfo(file).isFile()
            || QFileInfo(file).suffix().compare(QStringLiteral("apk"), Qt::CaseInsensitive) != 0) {
            emit operationFinished(false,
                                   tr("安装应用"),
                                   tr("仅支持有效的 .apk 文件：%1").arg(file));
            return;
        }
    }

    m_pendingInstallFiles = apkFiles;
    m_installTotal = apkFiles.size();
    m_installSucceeded = 0;
    m_installResults.clear();
    m_installReplaceExisting = replaceExisting;
    m_installGrantPermissions = grantPermissions;
    m_installBypassLowTargetSdk = bypassLowTargetSdk;
    startNextInstall();
}

void AppsService::launchApp(const QString &packageName)
{
    if (!validPackageName(packageName)) {
        return;
    }
    m_currentPackage = packageName;
    const QString script = QStringLiteral(
        "input keyevent KEYCODE_WAKEUP; monkey -p %1 -c android.intent.category.LAUNCHER 1")
                               .arg(packageName);
    start(Request::Action,
          tr("启动 %1").arg(packageName),
          {QStringLiteral("shell"), script});
}

void AppsService::stopApp(const QString &packageName)
{
    if (!validPackageName(packageName)) {
        return;
    }
    m_currentPackage = packageName;
    start(Request::Action,
          tr("停止 %1").arg(packageName),
          {QStringLiteral("shell"), QStringLiteral("am"), QStringLiteral("force-stop"), packageName});
}

void AppsService::setAppEnabled(const QString &packageName, bool enabled)
{
    if (!validPackageName(packageName)) {
        return;
    }
    m_currentPackage = packageName;
    const QString action = enabled ? QStringLiteral("enable") : QStringLiteral("disable-user");
    start(Request::Action,
          enabled ? tr("启用 %1").arg(packageName) : tr("停用 %1").arg(packageName),
          {QStringLiteral("shell"),
           QStringLiteral("pm"),
           action,
           QStringLiteral("--user"),
           QStringLiteral("0"),
           packageName},
          true,
          false);
}

void AppsService::openAppInfo(const QString &packageName)
{
    if (!validPackageName(packageName)) {
        return;
    }
    m_currentPackage = packageName;
    start(Request::Action,
          tr("打开应用信息"),
          {QStringLiteral("shell"),
           QStringLiteral("am"),
           QStringLiteral("start"),
           QStringLiteral("-a"),
           QStringLiteral("android.settings.APPLICATION_DETAILS_SETTINGS"),
           QStringLiteral("-d"),
           QStringLiteral("package:%1").arg(packageName)});
}

void AppsService::clearAppData(const QString &packageName)
{
    if (!validPackageName(packageName)) {
        return;
    }
    m_currentPackage = packageName;
    start(Request::Action,
          tr("清除 %1 数据").arg(packageName),
          {QStringLiteral("shell"), QStringLiteral("pm"), QStringLiteral("clear"), packageName},
          false,
          true);
}

void AppsService::uninstallApp(const QString &packageName, bool systemApp)
{
    if (!validPackageName(packageName)) {
        return;
    }
    m_currentPackage = packageName;
    const QStringList arguments = systemApp
        ? QStringList{QStringLiteral("shell"),
                      QStringLiteral("pm"),
                      QStringLiteral("uninstall"),
                      QStringLiteral("-k"),
                      QStringLiteral("--user"),
                      QStringLiteral("0"),
                      packageName}
        : QStringList{QStringLiteral("uninstall"), packageName};
    start(Request::Action, tr("卸载 %1").arg(packageName), arguments, true, false);
}

void AppsService::reinstallApp(const QString &packageName)
{
    if (!validPackageName(packageName)) {
        return;
    }
    m_currentPackage = packageName;
    start(Request::Action,
          tr("恢复 %1").arg(packageName),
          {QStringLiteral("shell"),
           QStringLiteral("cmd"),
           QStringLiteral("package"),
           QStringLiteral("install-existing"),
           packageName},
          true,
          false);
}

void AppsService::setBackgroundMode(const QString &packageName, const QString &mode)
{
    if (!validPackageName(packageName)
        || (mode != QStringLiteral("unrestricted") && mode != QStringLiteral("optimized")
            && mode != QStringLiteral("restricted"))) {
        return;
    }
    m_currentPackage = packageName;
    const QString value = mode == QStringLiteral("unrestricted")
        ? QStringLiteral("allow")
        : mode == QStringLiteral("restricted") ? QStringLiteral("ignore")
                                                 : QStringLiteral("default");
    const QString script = QStringLiteral(
        "cmd appops set %1 RUN_ANY_IN_BACKGROUND %2; "
        "cmd appops set %1 RUN_IN_BACKGROUND %2")
                               .arg(packageName, value);
    start(Request::Action,
          tr("修改后台模式"),
          {QStringLiteral("shell"), script},
          false,
          true);
}

void AppsService::setPermission(const QString &packageName,
                                const QString &permissionName,
                                bool granted)
{
    static const QRegularExpression permissionPattern(
        QStringLiteral("^[A-Za-z0-9_.]+$") );
    if (!validPackageName(packageName)
        || !permissionPattern.match(permissionName).hasMatch()) {
        return;
    }
    m_currentPackage = packageName;

    if (permissionName == QStringLiteral("android.permission.REQUEST_INSTALL_PACKAGES")) {
        start(Request::Action,
              tr("修改应用权限"),
              {QStringLiteral("shell"),
               QStringLiteral("appops"),
               QStringLiteral("set"),
               packageName,
               QStringLiteral("REQUEST_INSTALL_PACKAGES"),
               granted ? QStringLiteral("allow") : QStringLiteral("deny")},
              false,
              true);
        return;
    }

    const QString action = granted ? QStringLiteral("grant") : QStringLiteral("revoke");
    const QString script = QStringLiteral(
        "pm %1 --user current %2 %3 >/dev/null 2>&1 || "
        "(pm clear-permission-flags --user current %2 %3 user-set user-fixed "
        ">/dev/null 2>&1; pm %1 --user current %2 %3 >/dev/null 2>&1 || pm %1 %2 %3)")
                               .arg(action, packageName, permissionName);
    start(Request::Action,
          tr("修改应用权限"),
          {QStringLiteral("shell"), script},
          false,
          true);
}

void AppsService::exportApk(const QString &apkPath, const QString &destination)
{
    if (apkPath.isEmpty() || apkPath == QStringLiteral("-") || destination.isEmpty()) {
        return;
    }
    start(Request::Export,
          tr("导出 APK"),
          {QStringLiteral("pull"), apkPath, destination});
}

void AppsService::start(Request request,
                        const QString &label,
                        const QStringList &arguments,
                        bool refreshList,
                        bool refreshDetails)
{
    if (busy()) {
        return;
    }
    if (m_deviceSerial.isEmpty()) {
        emit operationFinished(false, label, tr("当前没有已连接并授权的设备。"));
        return;
    }
    if (!QFileInfo::exists(m_adbPath)) {
        emit operationFinished(false,
                               label,
                               tr("未找到 adb.exe：%1")
                                   .arg(QDir::toNativeSeparators(m_adbPath)));
        return;
    }

    m_request = request;
    m_currentLabel = label;
    m_refreshList = refreshList;
    m_refreshDetails = refreshDetails;
    m_output.clear();
    m_process.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    emit busyChanged(true);
    emit operationStarted(label, displayCommand(m_deviceSerial, arguments));
    m_process.start(m_adbPath, adbArguments(m_deviceSerial, arguments));
}

void AppsService::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_cancellingProcess || m_request == Request::Idle) {
        m_process.readAllStandardOutput();
        return;
    }
    m_output += m_process.readAllStandardOutput();
    const QString output = QString::fromUtf8(m_output).trimmed();
    const bool success = exitStatus == QProcess::NormalExit && exitCode == 0
        && !outputIndicatesFailure(output);
    if (m_request == Request::Install) {
        const QString label = m_currentLabel;
        finishRequest();
        if (success) {
            ++m_installSucceeded;
            m_installResults.append(tr("%1：成功").arg(label));
        } else {
            const QString reason = output.isEmpty()
                ? tr("退出码 %1").arg(exitCode)
                : output.simplified().left(240);
            m_installResults.append(tr("%1：失败 - %2").arg(label, reason));
        }

        if (!m_pendingInstallFiles.isEmpty()) {
            startNextInstall();
            return;
        }

        const int total = m_installTotal;
        const int installed = m_installSucceeded;
        const bool allSucceeded = total > 0 && installed == total;
        const QString detail = tr("已安装 %1 / %2 个 APK。\n%3")
                                   .arg(installed)
                                   .arg(total)
                                   .arg(m_installResults.join(QLatin1Char('\n')));
        resetInstallBatch();
        emit operationFinished(allSucceeded, tr("安装应用"), detail);
        emit appStateChanged(QString(), true, false);
        return;
    }
    if (!success
        && (m_request == Request::MetadataPush || m_request == Request::AppMetadata)) {
        const QString detail = output.isEmpty()
            ? tr("应用图标提取命令失败，退出码：%1").arg(exitCode)
            : output;
        finishRequest();
        resetMetadataLoad();
        emit operationFinished(false, tr("加载应用图标"), detail);
        return;
    }
    if (!success) {
        failRequest(output.isEmpty()
                        ? tr("命令执行失败，退出码：%1").arg(exitCode)
                        : output);
        return;
    }

    const Request completedRequest = m_request;
    const QString label = m_currentLabel;
    const QString packageName = m_currentPackage;
    const bool refreshList = m_refreshList;
    const bool refreshDetails = m_refreshDetails;
    finishRequest();

    if (completedRequest == Request::AppList) {
        QVector<AndroidAppSummary> apps = parseApps(output);
        for (AndroidAppSummary &app : apps) {
            if (m_labelCache.contains(app.packageName)) {
                app.displayName = m_labelCache.value(app.packageName);
            }
            app.iconPng = m_iconCache.value(app.packageName);
        }
        m_cachedApps = apps;
        saveActiveCache();
        emit appsLoaded(m_cachedApps);
        emit operationFinished(true, label, tr("已读取 %1 个应用。").arg(apps.size()));
        beginMetadataLoad(m_cachedApps, true);
        return;
    }
    if (completedRequest == Request::MetadataPush) {
        startNextMetadataBatch();
        return;
    }
    if (completedRequest == Request::AppMetadata) {
        const int loaded = applyMetadataResponse(output);
        if (loaded < 0) {
            resetMetadataLoad();
            emit operationFinished(false,
                                   tr("加载应用图标"),
                                   tr("无法解析设备返回的应用图标数据。"));
            return;
        }
        m_metadataLoaded += loaded;
        startNextMetadataBatch();
        return;
    }
    if (completedRequest == Request::AppDetails) {
        emit appDetailsLoaded(parseDetails(packageName, output));
    }
    emit operationFinished(true,
                           label,
                           output.isEmpty() ? tr("操作完成。") : output);
    if (completedRequest == Request::Action || completedRequest == Request::Install) {
        emit appStateChanged(packageName, refreshList, refreshDetails);
    }
}

void AppsService::finishRequest()
{
    m_request = Request::Idle;
    m_output.clear();
    m_refreshList = false;
    m_refreshDetails = false;
    emit busyChanged(false);
    if (!m_queuedMetadataPackages.isEmpty()) {
        QTimer::singleShot(0, this, [this] {
            if (!busy() && !m_queuedMetadataPackages.isEmpty()) {
                loadAppMetadata(m_queuedMetadataPackages);
            }
        });
    }
}

void AppsService::failRequest(const QString &detail)
{
    const QString label = m_currentLabel;
    const bool refreshApps = m_request == Request::Install && m_installSucceeded > 0;
    const bool metadataRequest = m_request == Request::MetadataPush
        || m_request == Request::AppMetadata;
    if (m_request == Request::Install) {
        resetInstallBatch();
    }
    if (metadataRequest) {
        resetMetadataLoad();
    }
    finishRequest();
    emit operationFinished(false, metadataRequest ? tr("加载应用图标") : label, detail);
    if (refreshApps) {
        emit appStateChanged(QString(), true, false);
    }
}

void AppsService::startNextInstall()
{
    if (m_pendingInstallFiles.isEmpty()) {
        return;
    }
    const QString file = m_pendingInstallFiles.takeFirst();
    QStringList arguments = {QStringLiteral("install")};
    if (m_installReplaceExisting) {
        arguments.append(QStringLiteral("-r"));
    }
    if (m_installGrantPermissions) {
        arguments.append(QStringLiteral("-g"));
    }
    if (m_installBypassLowTargetSdk) {
        arguments.append(QStringLiteral("--bypass-low-target-sdk-block"));
    }
    arguments.append(file);
    m_currentPackage.clear();
    start(Request::Install,
          tr("安装 %1").arg(QFileInfo(file).fileName()),
          arguments,
          false,
          false);
    if (m_request != Request::Install) {
        resetInstallBatch();
    }
}

void AppsService::resetInstallBatch()
{
    m_pendingInstallFiles.clear();
    m_installResults.clear();
    m_installTotal = 0;
    m_installSucceeded = 0;
}

void AppsService::beginMetadataLoad(QVector<AndroidAppSummary> apps, bool publishAppList)
{
    resetMetadataLoad();
    m_metadataApps = std::move(apps);
    m_metadataPublishesAppList = publishAppList;
    for (const AndroidAppSummary &app : std::as_const(m_metadataApps)) {
        if (!app.uninstalled && app.iconPng.isEmpty()) {
            m_pendingMetadataPackages.append(app.packageName);
        }
    }
    m_metadataRequested = m_pendingMetadataPackages.size();
    if (m_metadataRequested == 0) {
        const QVector<AndroidAppSummary> completedApps = m_metadataApps;
        resetMetadataLoad();
        if (!completedApps.isEmpty()) {
            emit appMetadataLoaded(completedApps);
        }
        return;
    }
    if (!QFileInfo::exists(m_metadataJarPath)) {
        resetMetadataLoad();
        emit operationFinished(false,
                               tr("加载应用图标"),
                               tr("未找到应用图标提取器：%1")
                                   .arg(QDir::toNativeSeparators(m_metadataJarPath)));
        return;
    }

    start(Request::MetadataPush,
          tr("准备应用图标"),
          {QStringLiteral("push"), m_metadataJarPath, kMetadataDevicePath});
    if (m_request != Request::MetadataPush) {
        resetMetadataLoad();
    }
}

void AppsService::startNextMetadataBatch()
{
    if (m_pendingMetadataPackages.isEmpty()) {
        const int requested = m_metadataRequested;
        const int loaded = m_metadataLoaded;
        const QVector<AndroidAppSummary> completedApps = m_metadataApps;
        const bool publishAppList = m_metadataPublishesAppList;
        resetMetadataLoad();
        if (publishAppList) {
            m_cachedApps = completedApps;
        } else {
            mergeCachedMetadata(completedApps);
        }
        saveActiveCache();
        if (publishAppList) {
            emit appsLoaded(m_cachedApps);
        }
        emit appMetadataLoaded(completedApps);
        emit operationFinished(true,
                               tr("加载应用图标"),
                               tr("已加载 %1 / %2 个应用图标。").arg(loaded).arg(requested));
        return;
    }

    QStringList batch;
    while (!m_pendingMetadataPackages.isEmpty() && batch.size() < kMetadataBatchSize) {
        batch.append(m_pendingMetadataPackages.takeFirst());
    }
    const QString command = QStringLiteral("CLASSPATH=%1 app_process / %2")
                                .arg(kMetadataDevicePath, kMetadataMainClass);
    QStringList arguments = {QStringLiteral("shell"), command};
    arguments.append(batch);
    start(Request::AppMetadata,
          tr("加载应用图标 %1 / %2")
              .arg(m_metadataRequested - m_pendingMetadataPackages.size())
              .arg(m_metadataRequested),
          arguments);
    if (m_request != Request::AppMetadata) {
        resetMetadataLoad();
    }
}

int AppsService::applyMetadataResponse(const QString &output)
{
    const int jsonStart = output.indexOf(QLatin1Char('['));
    const int jsonEnd = output.lastIndexOf(QLatin1Char(']'));
    if (jsonStart < 0 || jsonEnd < jsonStart) {
        return -1;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(
        output.mid(jsonStart, jsonEnd - jsonStart + 1).toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
        return -1;
    }

    int loaded = 0;
    for (const QJsonValue &value : document.array()) {
        const QJsonObject object = value.toObject();
        const QString packageName = object.value(QStringLiteral("package")).toString();
        if (!validPackageName(packageName)) {
            continue;
        }

        const QString label = object.value(QStringLiteral("label")).toString().trimmed();
        if (!label.isEmpty()) {
            m_labelCache.insert(packageName, label);
        }

        const QString iconDataUrl = object.value(QStringLiteral("icon")).toString();
        const int comma = iconDataUrl.indexOf(QLatin1Char(','));
        QByteArray iconPng;
        if (iconDataUrl.startsWith(QStringLiteral("data:image/png;base64,")) && comma >= 0) {
            iconPng = QByteArray::fromBase64(iconDataUrl.mid(comma + 1).toLatin1());
        }
        if (!iconPng.isEmpty()) {
            m_iconCache.insert(packageName, iconPng);
            ++loaded;
        }

        for (AndroidAppSummary &app : m_metadataApps) {
            if (app.packageName != packageName) {
                continue;
            }
            if (!label.isEmpty()) {
                app.displayName = label;
            }
            if (!iconPng.isEmpty()) {
                app.iconPng = iconPng;
            }
            break;
        }
    }
    return loaded;
}

void AppsService::saveActiveCache()
{
    if (m_deviceSerial.isEmpty()) {
        return;
    }
    DeviceCache &cache = m_deviceCaches[m_deviceSerial];
    cache.apps = m_cachedApps;
    cache.labels = m_labelCache;
    cache.icons = m_iconCache;
}

void AppsService::restoreActiveCache()
{
    if (m_deviceSerial.isEmpty()) {
        return;
    }
    const DeviceCache cache = m_deviceCaches.value(m_deviceSerial);
    m_cachedApps = cache.apps;
    m_labelCache = cache.labels;
    m_iconCache = cache.icons;
    mergeCachedMetadata(m_cachedApps);
}

void AppsService::mergeCachedMetadata(const QVector<AndroidAppSummary> &apps)
{
    for (const AndroidAppSummary &metadata : apps) {
        for (AndroidAppSummary &cached : m_cachedApps) {
            if (cached.packageName != metadata.packageName) {
                continue;
            }
            cached.displayName = m_labelCache.value(metadata.packageName,
                                                    metadata.displayName);
            cached.iconPng = m_iconCache.value(metadata.packageName, metadata.iconPng);
            break;
        }
    }
}

void AppsService::resetMetadataLoad()
{
    m_metadataApps.clear();
    m_pendingMetadataPackages.clear();
    m_metadataRequested = 0;
    m_metadataLoaded = 0;
    m_metadataPublishesAppList = false;
}

bool AppsService::validPackageName(const QString &packageName)
{
    static const QRegularExpression pattern(QStringLiteral("^[A-Za-z0-9._]+$"));
    return pattern.match(packageName).hasMatch();
}

QVector<AndroidAppSummary> AppsService::parseApps(const QString &output)
{
    const QStringList sections = output.split(kSeparator);
    if (sections.size() < 4) {
        return {};
    }
    const QSet<QString> systemPackages = packageSet(sections[1]);
    const QSet<QString> disabledPackages = packageSet(sections[2]);
    const QSet<QString> installedPackages = packageSet(sections[3]);

    QVector<AndroidAppSummary> apps;
    for (const QString &line : sections[0].split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                                  Qt::SkipEmptyParts)) {
        QString value = line.trimmed();
        if (!value.startsWith(QStringLiteral("package:"))) {
            continue;
        }
        value.remove(0, QStringLiteral("package:").size());
        const int separator = value.lastIndexOf(QLatin1Char('='));
        if (separator <= 0) {
            continue;
        }
        AndroidAppSummary app;
        app.apkPath = value.left(separator);
        app.packageName = value.mid(separator + 1);
        app.displayName = app.packageName;
        app.systemApp = systemPackages.contains(app.packageName);
        app.disabled = disabledPackages.contains(app.packageName);
        app.uninstalled = !installedPackages.contains(app.packageName);
        apps.append(app);
    }
    std::sort(apps.begin(), apps.end(), [](const AndroidAppSummary &left,
                                           const AndroidAppSummary &right) {
        return left.packageName.compare(right.packageName, Qt::CaseInsensitive) < 0;
    });
    return apps;
}

AndroidAppDetails AppsService::parseDetails(const QString &packageName, const QString &output)
{
    const QStringList sections = output.split(kSeparator);
    AndroidAppDetails details;
    details.packageName = packageName;
    details.displayName = packageName;
    if (sections.size() < 7) {
        return details;
    }

    const QString dump = sections[0];
    details.displayName = displayNameFromDump(dump, packageName);
    const QStringList paths = sections[1].split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                                 Qt::SkipEmptyParts);
    QStringList apkPaths;
    for (const QString &line : paths) {
        const QString value = line.trimmed();
        if (value.startsWith(QStringLiteral("package:"))) {
            apkPaths.append(value.mid(QStringLiteral("package:").size()));
        }
    }
    details.apkPath = apkPaths.value(0, QStringLiteral("-"));
    details.splitApk = apkPaths.size() > 1;
    details.systemApp = packageSet(sections[2]).contains(packageName);
    details.disabled = packageSet(sections[3]).contains(packageName);
    details.versionName = dumpValue(dump, QStringLiteral("versionName"));
    details.versionCode = dumpValue(dump, QStringLiteral("versionCode"));
    details.targetSdk = dumpValue(dump, QStringLiteral("targetSdk"));
    details.minSdk = dumpValue(dump, QStringLiteral("minSdk"));
    details.installer = dumpValue(dump, QStringLiteral("installerPackageName"));
    details.dataDirectory = dumpValue(dump, QStringLiteral("dataDir"));
    details.installDate = dumpValue(dump, QStringLiteral("firstInstallTime"));
    details.updateDate = dumpValue(dump, QStringLiteral("lastUpdateTime"));
    const QString appOps = sections[4];
    details.backgroundMode = appOps.contains(QStringLiteral("RUN_ANY_IN_BACKGROUND: allow"))
        ? QStringLiteral("unrestricted")
        : appOps.contains(QStringLiteral("RUN_ANY_IN_BACKGROUND: ignore"))
                || appOps.contains(QStringLiteral("RUN_IN_BACKGROUND: ignore"))
            ? QStringLiteral("restricted")
            : QStringLiteral("optimized");
    details.permissions = parsePermissions(dump, appOps, sections[5]);

    const QString duOutput = sections[6].trimmed();
    const QString firstValue = duOutput.section(QRegularExpression(QStringLiteral("\\s+")), 0, 0);
    bool ok = false;
    const qint64 kilobytes = firstValue.toLongLong(&ok);
    details.codeSize = ok ? kilobytes * 1024 : -1;
    return details;
}

QString AppsService::dumpValue(const QString &output, const QString &key)
{
    const QString marker = key + QLatin1Char('=');
    for (const QString &line : output.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                             Qt::SkipEmptyParts)) {
        const int position = line.indexOf(marker);
        if (position < 0) {
            continue;
        }
        QString value = line.mid(position + marker.size()).trimmed();
        if (key != QStringLiteral("firstInstallTime") && key != QStringLiteral("lastUpdateTime")) {
            value = value.section(QRegularExpression(QStringLiteral("\\s+")), 0, 0);
        }
        return value.isEmpty() ? QStringLiteral("-") : value;
    }
    return QStringLiteral("-");
}

QVector<AndroidAppPermission> AppsService::parsePermissions(const QString &dumpOutput,
                                                            const QString &appOpsOutput,
                                                            const QString &changeableOutput)
{
    QHash<QString, AndroidAppPermission> permissions;
    QSet<QString> changeablePermissions;
    for (const QString &line : changeableOutput.split(
             QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts)) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("permission:"))) {
            changeablePermissions.insert(
                trimmed.mid(QStringLiteral("permission:").size()).trimmed());
        }
    }
    QString section;
    for (const QString &line : dumpOutput.split(QRegularExpression(QStringLiteral("[\\r\\n]+")))) {
        const QString trimmed = line.trimmed();
        if (trimmed == QStringLiteral("requested permissions:")) {
            section = QStringLiteral("requested");
            continue;
        }
        if (trimmed == QStringLiteral("install permissions:")) {
            section = QStringLiteral("install");
            continue;
        }
        if (trimmed == QStringLiteral("runtime permissions:")) {
            section = QStringLiteral("runtime");
            continue;
        }
        if (trimmed.endsWith(QLatin1Char(':')) && !trimmed.contains(QLatin1Char('.'))) {
            section.clear();
            continue;
        }
        if (section.isEmpty() || !trimmed.startsWith(QStringLiteral("android.permission."))) {
            continue;
        }

        const QString name = trimmed.section(QLatin1Char(':'), 0, 0).trimmed();
        AndroidAppPermission permission = permissions.value(name);
        permission.name = name;
        permission.granted = permission.granted || section == QStringLiteral("install")
            || trimmed.contains(QStringLiteral("granted=true"));
        const bool runtime = section == QStringLiteral("runtime")
            || (section == QStringLiteral("requested")
                && changeablePermissions.contains(name));
        permission.runtime = permission.runtime || runtime;
        const bool fixed = trimmed.contains(QStringLiteral("SYSTEM_FIXED"))
            || trimmed.contains(QStringLiteral("POLICY_FIXED"))
            || trimmed.contains(QStringLiteral("HARD_RESTRICTED"));
        permission.changeable = (permission.changeable || runtime)
            && !fixed;
        permissions.insert(name, permission);
    }

    const QString installPackagesPermission = QStringLiteral(
        "android.permission.REQUEST_INSTALL_PACKAGES");
    if (permissions.contains(installPackagesPermission)) {
        AndroidAppPermission permission = permissions.value(installPackagesPermission);
        permission.changeable = true;
        const int position = appOpsOutput.indexOf(QStringLiteral("REQUEST_INSTALL_PACKAGES: "));
        permission.granted = position >= 0
            && appOpsOutput.mid(position + 26).startsWith(QStringLiteral("allow"));
        permissions.insert(installPackagesPermission, permission);
    }

    QVector<AndroidAppPermission> result = permissions.values();
    std::sort(result.begin(), result.end(), [](const AndroidAppPermission &left,
                                               const AndroidAppPermission &right) {
        if (left.changeable != right.changeable) {
            return left.changeable;
        }
        return left.name.compare(right.name, Qt::CaseInsensitive) < 0;
    });
    return result;
}
