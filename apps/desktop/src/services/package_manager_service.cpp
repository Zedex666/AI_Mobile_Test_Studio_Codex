#include "services/package_manager_service.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

#include <utility>

namespace {

QStringList adbShellArguments(const QString &serial, const QStringList &shellArguments)
{
    QStringList arguments = {QStringLiteral("-s"), serial, QStringLiteral("shell")};
    arguments.append(shellArguments);
    return arguments;
}

struct CatalogCommand {
    const char *label;
    const char *displayCommand;
    QStringList arguments;
};

CatalogCommand catalogCommand(int category)
{
    switch (category) {
    case 0:
        return {"已知权限组",
                "adb shell pm list permission-groups",
                {QStringLiteral("pm"), QStringLiteral("list"), QStringLiteral("permission-groups")}};
    case 1:
        return {"已知权限",
                "adb shell pm list permissions",
                {QStringLiteral("pm"), QStringLiteral("list"), QStringLiteral("permissions")}};
    case 3:
        return {"系统功能",
                "adb shell pm list features",
                {QStringLiteral("pm"), QStringLiteral("list"), QStringLiteral("features")}};
    case 4:
        return {"库",
                "adb shell pm list libraries",
                {QStringLiteral("pm"), QStringLiteral("list"), QStringLiteral("libraries")}};
    case 5:
        return {"用户",
                "adb shell pm list users",
                {QStringLiteral("pm"), QStringLiteral("list"), QStringLiteral("users")}};
    default:
        return {nullptr, nullptr, {}};
    }
}

bool containsFailure(const QString &output)
{
    const QStringList lines = output.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                           Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("failure"), Qt::CaseInsensitive)
            || trimmed.startsWith(QStringLiteral("error"), Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

} // namespace

PackageManagerService::PackageManagerService(QString adbPath, QObject *parent)
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
            &PackageManagerService::handleFinished);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && m_request != Request::Idle) {
            failRequest(tr("adb 启动失败：%1").arg(m_process.errorString()));
        }
    });
}

void PackageManagerService::setDeviceSerial(const QString &serial)
{
    if (m_deviceSerial == serial) {
        return;
    }

    m_deviceSerial = serial;
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
    }
    m_request = Request::Idle;
    m_output.clear();
    m_currentPackage.clear();
    m_currentPath.clear();
    m_currentCategory = -1;
    emit busyChanged(false);
}

bool PackageManagerService::busy() const
{
    return m_request != Request::Idle;
}

void PackageManagerService::loadCategory(int category)
{
    if (category == 2) {
        loadPackages(PackageFilters{});
        return;
    }

    const CatalogCommand command = catalogCommand(category);
    if (command.label == nullptr) {
        return;
    }

    m_currentCategory = category;
    start(Request::Catalog,
          QString::fromUtf8(command.label),
          QString::fromUtf8(command.displayCommand),
          adbShellArguments(m_deviceSerial, command.arguments));
}

void PackageManagerService::loadPackages(PackageFilters filters)
{
    QStringList pmArguments = {QStringLiteral("pm"),
                               QStringLiteral("list"),
                               QStringLiteral("packages")};
    QStringList commandParts = {QStringLiteral("adb -s"),
                                m_deviceSerial,
                                QStringLiteral("shell pm list packages")};
    if (filters.enabledOnly) {
        pmArguments.append(QStringLiteral("-e"));
        commandParts.append(QStringLiteral("-e"));
    }
    if (filters.disabledOnly) {
        pmArguments.append(QStringLiteral("-d"));
        commandParts.append(QStringLiteral("-d"));
    }
    if (filters.thirdPartyOnly) {
        pmArguments.append(QStringLiteral("-3"));
        commandParts.append(QStringLiteral("-3"));
    }
    if (filters.systemOnly) {
        pmArguments.append(QStringLiteral("-s"));
        commandParts.append(QStringLiteral("-s"));
    }

    start(Request::PackageList,
          tr("刷新软件包列表"),
          commandParts.join(QLatin1Char(' ')),
          adbShellArguments(m_deviceSerial, pmArguments));
}

void PackageManagerService::loadPackageDetails(const QString &packageName)
{
    if (packageName.isEmpty()) {
        return;
    }

    m_currentPackage = packageName;
    start(Request::PackagePath,
          tr("读取软件包详情"),
          QStringLiteral("adb -s %1 shell pm list packages -f %2")
              .arg(m_deviceSerial, packageName),
          adbShellArguments(m_deviceSerial,
                            {QStringLiteral("pm"),
                             QStringLiteral("list"),
                             QStringLiteral("packages"),
                             QStringLiteral("-f"),
                             packageName}));
}

void PackageManagerService::uninstallPackage(const QString &packageName)
{
    if (packageName.isEmpty()) {
        return;
    }

    m_currentPackage = packageName;
    m_currentCategory = -1;
    start(Request::PackageAction,
          tr("卸载 %1").arg(packageName),
          QStringLiteral("adb -s %1 shell pm uninstall %2").arg(m_deviceSerial, packageName),
          adbShellArguments(m_deviceSerial,
                            {QStringLiteral("pm"), QStringLiteral("uninstall"), packageName}));
}

void PackageManagerService::clearPackageData(const QString &packageName)
{
    if (packageName.isEmpty()) {
        return;
    }

    m_currentPackage = packageName;
    m_currentCategory = -1;
    start(Request::PackageAction,
          tr("清除 %1 的数据").arg(packageName),
          QStringLiteral("adb -s %1 shell pm clear %2").arg(m_deviceSerial, packageName),
          adbShellArguments(m_deviceSerial,
                            {QStringLiteral("pm"), QStringLiteral("clear"), packageName}));
}

void PackageManagerService::enablePackage(const QString &packageName)
{
    if (packageName.isEmpty()) {
        return;
    }

    m_currentPackage = packageName;
    m_currentCategory = -1;
    start(Request::PackageAction,
          tr("启用 %1").arg(packageName),
          QStringLiteral("adb -s %1 shell pm enable %2").arg(m_deviceSerial, packageName),
          adbShellArguments(m_deviceSerial,
                            {QStringLiteral("pm"), QStringLiteral("enable"), packageName}));
}

void PackageManagerService::disablePackage(const QString &packageName)
{
    if (packageName.isEmpty()) {
        return;
    }

    m_currentPackage = packageName;
    m_currentCategory = -1;
    start(Request::PackageAction,
          tr("停用 %1").arg(packageName),
          QStringLiteral("adb -s %1 shell pm disable-user %2").arg(m_deviceSerial, packageName),
          adbShellArguments(m_deviceSerial,
                            {QStringLiteral("pm"),
                             QStringLiteral("disable-user"),
                             packageName}));
}

void PackageManagerService::removeUser(const QString &userId)
{
    if (userId.isEmpty()) {
        return;
    }

    m_currentPackage = userId;
    m_currentCategory = 5;
    start(Request::PackageAction,
          tr("删除用户 %1").arg(userId),
          QStringLiteral("adb -s %1 shell pm remove-user %2").arg(m_deviceSerial, userId),
          adbShellArguments(m_deviceSerial,
                            {QStringLiteral("pm"), QStringLiteral("remove-user"), userId}));
}

void PackageManagerService::start(Request request,
                                  const QString &label,
                                  const QString &displayCommand,
                                  const QStringList &arguments)
{
    if (busy()) {
        return;
    }
    if (m_deviceSerial.isEmpty()) {
        emit commandFinished(false, label, tr("当前没有已连接并已授权的设备。"));
        return;
    }
    if (!QFileInfo::exists(m_adbPath)) {
        emit commandFinished(false,
                             label,
                             tr("未找到 adb.exe：%1").arg(QDir::toNativeSeparators(m_adbPath)));
        return;
    }

    m_request = request;
    m_currentLabel = label;
    m_output.clear();
    m_process.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    emit busyChanged(true);
    emit commandStarted(label, displayCommand);
    m_process.start(m_adbPath, arguments);
}

void PackageManagerService::startInstallerQuery()
{
    m_request = Request::PackageInstaller;
    m_output.clear();
    m_process.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    m_process.start(m_adbPath,
                    adbShellArguments(m_deviceSerial,
                                      {QStringLiteral("pm"),
                                       QStringLiteral("list"),
                                       QStringLiteral("packages"),
                                       QStringLiteral("-i"),
                                       m_currentPackage}));
}

void PackageManagerService::finishRequest()
{
    m_request = Request::Idle;
    m_output.clear();
    emit busyChanged(false);
}

void PackageManagerService::failRequest(const QString &detail)
{
    const QString label = m_currentLabel;
    finishRequest();
    emit commandFinished(false, label, detail);
}

void PackageManagerService::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_request == Request::Idle) {
        return;
    }

    m_output += m_process.readAllStandardOutput();
    const QString output = QString::fromUtf8(m_output).trimmed();
    const bool processSuccess = exitStatus == QProcess::NormalExit && exitCode == 0;
    if (!processSuccess || containsFailure(output)) {
        failRequest(output.isEmpty()
                        ? tr("命令执行失败，退出码：%1").arg(exitCode)
                        : output);
        return;
    }

    switch (m_request) {
    case Request::PackageList: {
        const QStringList packages = parsePackageList(output);
        const QString label = m_currentLabel;
        finishRequest();
        emit packagesLoaded(packages);
        emit commandFinished(true, label, tr("已加载 %1 个软件包。").arg(packages.size()));
        return;
    }
    case Request::PackagePath:
        m_currentPath = packagePath(output, m_currentPackage);
        startInstallerQuery();
        return;
    case Request::PackageInstaller: {
        const QString packageName = m_currentPackage;
        const QString path = m_currentPath;
        const QString installer = packageInstaller(output);
        const QString label = m_currentLabel;
        finishRequest();
        emit packageDetailsLoaded(packageName, path, installer);
        emit commandFinished(true, label, tr("已读取 %1 的软件包详情。").arg(packageName));
        return;
    }
    case Request::PackageAction: {
        const QString packageName = m_currentPackage;
        const int category = m_currentCategory;
        const QString label = m_currentLabel;
        const QString detail = output.isEmpty() ? tr("命令执行成功。") : output;
        finishRequest();
        emit commandFinished(true, label, detail);
        if (category >= 0) {
            emit categoryActionCompleted(category);
        } else {
            emit packageActionCompleted(packageName);
        }
        return;
    }
    case Request::Catalog: {
        const int category = m_currentCategory;
        const QString label = m_currentLabel;
        const QStringList items = parseCatalogItems(output);
        finishRequest();
        emit categoryLoaded(category, items);
        emit commandFinished(true, label, tr("已加载 %1 项。").arg(items.size()));
        return;
    }
    case Request::Idle:
        return;
    }
}

QStringList PackageManagerService::parsePackageList(const QString &output)
{
    QSet<QString> uniquePackages;
    const QStringList lines = output.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                           Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QString packageName = line.trimmed();
        if (!packageName.startsWith(QStringLiteral("package:"))) {
            continue;
        }
        packageName.remove(0, QStringLiteral("package:").size());
        const int separator = packageName.lastIndexOf(QLatin1Char('='));
        if (separator >= 0) {
            packageName = packageName.mid(separator + 1);
        }
        if (!packageName.isEmpty()) {
            uniquePackages.insert(packageName);
        }
    }

    QStringList packages = uniquePackages.values();
    packages.sort(Qt::CaseInsensitive);
    return packages;
}

QStringList PackageManagerService::parseCatalogItems(const QString &output)
{
    QSet<QString> uniqueItems;
    const QStringList lines = output.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                           Qt::SkipEmptyParts);
    const QStringList prefixes = {QStringLiteral("permission group:"),
                                  QStringLiteral("group:"),
                                  QStringLiteral("permission:"),
                                  QStringLiteral("feature:"),
                                  QStringLiteral("library:")};
    for (const QString &line : lines) {
        QString item = line.trimmed();
        if (item.isEmpty() || item.endsWith(QLatin1Char(':'))) {
            continue;
        }
        for (const QString &prefix : prefixes) {
            if (item.startsWith(prefix)) {
                item.remove(0, prefix.size());
                break;
            }
        }
        if (!item.isEmpty()) {
            uniqueItems.insert(item);
        }
    }

    QStringList items = uniqueItems.values();
    items.sort(Qt::CaseInsensitive);
    return items;
}

QString PackageManagerService::packagePath(const QString &output, const QString &packageName)
{
    const QStringList lines = output.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                           Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QString value = line.trimmed();
        const QString prefix = QStringLiteral("package:");
        const int separator = value.lastIndexOf(QLatin1Char('='));
        if (value.startsWith(prefix) && separator > prefix.size()
            && value.mid(separator + 1) == packageName) {
            return value.mid(prefix.size(), separator - prefix.size());
        }
    }
    return tr("未返回 APK 路径");
}

QString PackageManagerService::packageInstaller(const QString &output)
{
    static const QRegularExpression installerPattern(QStringLiteral("(?:^|\\s)installer=([^\\s]+)"));
    const QRegularExpressionMatch match = installerPattern.match(output);
    if (!match.hasMatch() || match.captured(1) == QStringLiteral("null")) {
        return tr("系统或未知来源");
    }
    return match.captured(1);
}
