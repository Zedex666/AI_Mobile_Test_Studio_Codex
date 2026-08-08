#include "services/studio_operation_manager.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QList>
#include <QProcess>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QTimer>

#include <algorithm>
#include <utility>

namespace {

constexpr qsizetype kMaximumOperationOutputBytes = 2 * 1024 * 1024;
constexpr int kMaximumRetainedOperations = 256;

QString operationId()
{
    const quint64 randomValue = QRandomGenerator::system()->generate64();
    return QStringLiteral("op_%1_%2")
        .arg(QDateTime::currentMSecsSinceEpoch(), 0, 16)
        .arg(randomValue, 0, 16);
}

QString decodeAdbText(const QByteArray &data)
{
    QStringDecoder decoder(QStringDecoder::Utf8);
    const QString utf8 = decoder.decode(data);
    return decoder.hasError() ? QString::fromLocal8Bit(data) : utf8;
}

QString snapshotCommand()
{
    return QStringLiteral(
        "printf '__AMTS_PROPERTIES__\\n'; getprop; "
        "printf '__AMTS_SIZE__\\n'; wm size 2>/dev/null; "
        "printf '__AMTS_DENSITY__\\n'; wm density 2>/dev/null; "
        "printf '__AMTS_BATTERY__\\n'; dumpsys battery 2>/dev/null; "
        "printf '__AMTS_MEMORY__\\n'; cat /proc/meminfo 2>/dev/null; "
        "printf '__AMTS_UPTIME__\\n'; cat /proc/uptime 2>/dev/null;");
}

QHash<QString, QStringList> splitSections(const QString &output)
{
    QHash<QString, QStringList> sections;
    QString current;
    for (const QString &rawLine : output.split(QLatin1Char('\n'))) {
        const QString line = rawLine.trimmed();
        if (line.startsWith(QStringLiteral("__AMTS_"))
            && line.endsWith(QStringLiteral("__"))) {
            current = line;
        } else if (!current.isEmpty() && !line.isEmpty()) {
            sections[current].append(line);
        }
    }
    return sections;
}

QHash<QString, QString> parseProperties(const QStringList &lines)
{
    QHash<QString, QString> properties;
    static const QRegularExpression pattern(
        QStringLiteral("^\\[([^]]+)\\]:\\s*\\[(.*)\\]$"));
    for (const QString &line : lines) {
        const QRegularExpressionMatch match = pattern.match(line);
        if (match.hasMatch()) {
            properties.insert(match.captured(1), match.captured(2));
        }
    }
    return properties;
}

QString firstProperty(const QHash<QString, QString> &properties,
                      const QStringList &keys)
{
    for (const QString &key : keys) {
        const QString value = properties.value(key).trimmed();
        if (!value.isEmpty()) {
            return value;
        }
    }
    return QString();
}

QString valueAfterColon(const QStringList &lines, const QString &prefix)
{
    for (const QString &line : lines) {
        if (line.startsWith(prefix, Qt::CaseInsensitive)) {
            return line.section(QLatin1Char(':'), 1).trimmed();
        }
    }
    return QString();
}

qint64 memoryKilobytes(const QStringList &lines, const QString &key)
{
    const QString prefix = key + QLatin1Char(':');
    for (const QString &line : lines) {
        if (!line.startsWith(prefix)) {
            continue;
        }
        bool ok = false;
        const qint64 value = line.mid(prefix.size())
                                 .trimmed()
                                 .section(QLatin1Char(' '), 0, 0)
                                 .toLongLong(&ok);
        return ok ? value : 0;
    }
    return 0;
}

int integerField(const QStringList &lines, const QString &key)
{
    const QString prefix = key + QLatin1Char(':');
    for (const QString &line : lines) {
        if (!line.startsWith(prefix, Qt::CaseInsensitive)) {
            continue;
        }
        bool ok = false;
        const int value = line.mid(prefix.size()).trimmed().toInt(&ok);
        return ok ? value : -1;
    }
    return -1;
}

QJsonObject snapshotResult(const QString &serial, const QString &output)
{
    const QHash<QString, QStringList> sections = splitSections(output);
    const QHash<QString, QString> properties = parseProperties(
        sections.value(QStringLiteral("__AMTS_PROPERTIES__")));
    const QStringList sizeLines = sections.value(QStringLiteral("__AMTS_SIZE__"));
    const QStringList densityLines = sections.value(QStringLiteral("__AMTS_DENSITY__"));
    const QStringList batteryLines = sections.value(QStringLiteral("__AMTS_BATTERY__"));
    const QStringList memoryLines = sections.value(QStringLiteral("__AMTS_MEMORY__"));
    const QStringList uptimeLines = sections.value(QStringLiteral("__AMTS_UPTIME__"));

    QJsonObject device;
    device.insert(QStringLiteral("serial"), serial);
    device.insert(QStringLiteral("name"),
                  firstProperty(properties,
                                {QStringLiteral("ro.product.marketname"),
                                 QStringLiteral("ro.product.model")}));
    device.insert(QStringLiteral("brand"), properties.value(QStringLiteral("ro.product.brand")));
    device.insert(QStringLiteral("manufacturer"),
                  properties.value(QStringLiteral("ro.product.manufacturer")));
    device.insert(QStringLiteral("model"), properties.value(QStringLiteral("ro.product.model")));
    device.insert(QStringLiteral("product"), properties.value(QStringLiteral("ro.product.name")));
    device.insert(QStringLiteral("device"), properties.value(QStringLiteral("ro.product.device")));
    device.insert(QStringLiteral("androidVersion"),
                  properties.value(QStringLiteral("ro.build.version.release")));
    device.insert(QStringLiteral("sdkVersion"),
                  properties.value(QStringLiteral("ro.build.version.sdk")));
    device.insert(QStringLiteral("buildId"), properties.value(QStringLiteral("ro.build.id")));
    device.insert(QStringLiteral("abi"),
                  firstProperty(properties,
                                {QStringLiteral("ro.product.cpu.abi"),
                                 QStringLiteral("ro.product.cpu.abilist")}));
    device.insert(QStringLiteral("characteristics"),
                  properties.value(QStringLiteral("ro.build.characteristics")));
    device.insert(QStringLiteral("resolution"),
                  valueAfterColon(sizeLines, QStringLiteral("Physical size")));
    device.insert(QStringLiteral("overrideResolution"),
                  valueAfterColon(sizeLines, QStringLiteral("Override size")));
    device.insert(QStringLiteral("density"),
                  valueAfterColon(densityLines, QStringLiteral("Physical density")));
    device.insert(QStringLiteral("overrideDensity"),
                  valueAfterColon(densityLines, QStringLiteral("Override density")));
    device.insert(QStringLiteral("batteryLevel"), integerField(batteryLines, QStringLiteral("level")));
    device.insert(QStringLiteral("batteryHealth"), integerField(batteryLines, QStringLiteral("health")));
    device.insert(QStringLiteral("memoryTotalKb"),
                  static_cast<double>(memoryKilobytes(memoryLines, QStringLiteral("MemTotal"))));
    device.insert(QStringLiteral("memoryAvailableKb"),
                  static_cast<double>(memoryKilobytes(memoryLines, QStringLiteral("MemAvailable"))));
    if (!uptimeLines.isEmpty()) {
        bool ok = false;
        const double uptime = uptimeLines.first().section(QLatin1Char(' '), 0, 0).toDouble(&ok);
        if (ok) {
            device.insert(QStringLiteral("uptimeSeconds"), uptime);
        }
    }

    return {{QStringLiteral("device"), device}};
}

QJsonObject appsResult(const QString &output)
{
    QJsonArray apps;
    QSet<QString> seenPackages;
    for (const QString &rawLine : output.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                                Qt::SkipEmptyParts)) {
        QString line = rawLine.trimmed();
        if (!line.startsWith(QStringLiteral("package:"))) {
            continue;
        }
        line.remove(0, QStringLiteral("package:").size());
        const qsizetype separator = line.lastIndexOf(QLatin1Char('='));
        const QString packageName = separator >= 0 ? line.mid(separator + 1) : line;
        const QString apkPath = separator >= 0 ? line.left(separator) : QString();
        if (packageName.isEmpty() || seenPackages.contains(packageName)) {
            continue;
        }
        seenPackages.insert(packageName);
        apps.append(QJsonObject{{QStringLiteral("packageName"), packageName},
                                {QStringLiteral("apkPath"), apkPath}});
    }
    return {{QStringLiteral("count"), apps.size()}, {QStringLiteral("apps"), apps}};
}

} // namespace

StudioOperationManager::StudioOperationManager(QString adbPath, QObject *parent)
    : QObject(parent)
    , m_adbPath(QDir::cleanPath(std::move(adbPath)))
{
}

StudioOperationManager::~StudioOperationManager()
{
    const QList<Operation *> operations = m_operations.values();
    m_operations.clear();
    for (Operation *operation : operations) {
        if (operation->timeout != nullptr) {
            operation->timeout->stop();
            operation->timeout->disconnect(this);
            delete operation->timeout;
            operation->timeout = nullptr;
        }
        if (operation->process != nullptr) {
            operation->process->disconnect(this);
            if (operation->process->state() != QProcess::NotRunning) {
                operation->process->kill();
                operation->process->waitForFinished(100);
            }
            delete operation->process;
            operation->process = nullptr;
        }
        delete operation;
    }
    m_lockedResources.clear();
}

QJsonObject StudioOperationManager::startDeviceSnapshot(const QString &serial, QString *error)
{
    return startAdbOperation(serial,
                             Kind::DeviceSnapshot,
                             QStringLiteral("device.snapshot"),
                             {QStringLiteral("-s"),
                              serial,
                              QStringLiteral("shell"),
                              snapshotCommand()},
                             30000,
                             QString(),
                             error);
}

QJsonObject StudioOperationManager::startAppsList(const QString &serial, QString *error)
{
    return startAdbOperation(serial,
                             Kind::AppsList,
                             QStringLiteral("device.apps.list"),
                             {QStringLiteral("-s"),
                              serial,
                              QStringLiteral("shell"),
                              QStringLiteral("pm list packages -f -u")},
                             30000,
                             QString(),
                             error);
}

QJsonObject StudioOperationManager::startDeviceAction(const QString &serial,
                                                       const QJsonObject &parameters,
                                                       QString *error)
{
    const QString action = parameters.value(QStringLiteral("action")).toString();
    QStringList shellArguments;
    if (action == QStringLiteral("keyEvent")) {
        const QString keyCode = parameters.value(QStringLiteral("keyCode")).toString();
        static const QRegularExpression pattern(QStringLiteral("^KEYCODE_[A-Z0-9_]+$"));
        if (!pattern.match(keyCode).hasMatch()) {
            if (error != nullptr) {
                *error = QStringLiteral("keyCode must be a valid Android KEYCODE value.");
            }
            return {};
        }
        shellArguments = {QStringLiteral("input"), QStringLiteral("keyevent"), keyCode};
    } else if (action == QStringLiteral("launchApp")
               || action == QStringLiteral("stopApp")) {
        const QString packageName = parameters.value(QStringLiteral("packageName")).toString();
        static const QRegularExpression pattern(
            QStringLiteral("^[A-Za-z][A-Za-z0-9_]*(?:\\.[A-Za-z0-9_]+)+$"));
        if (!pattern.match(packageName).hasMatch()) {
            if (error != nullptr) {
                *error = QStringLiteral("packageName is invalid.");
            }
            return {};
        }
        if (action == QStringLiteral("launchApp")) {
            shellArguments = {QStringLiteral("monkey"),
                              QStringLiteral("-p"),
                              packageName,
                              QStringLiteral("-c"),
                              QStringLiteral("android.intent.category.LAUNCHER"),
                              QStringLiteral("1")};
        } else {
            shellArguments = {QStringLiteral("am"),
                              QStringLiteral("force-stop"),
                              packageName};
        }
    } else {
        if (error != nullptr) {
            *error = QStringLiteral("Unsupported action. Allowed: keyEvent, launchApp, stopApp.");
        }
        return {};
    }

    QStringList arguments = {QStringLiteral("-s"), serial, QStringLiteral("shell")};
    arguments.append(shellArguments);
    return startAdbOperation(serial,
                             Kind::DeviceAction,
                             QStringLiteral("device.action.%1").arg(action),
                             arguments,
                             15000,
                             QStringLiteral("device:%1:control").arg(serial),
                             error);
}

QJsonObject StudioOperationManager::operationStatus(const QString &operationId,
                                                     QString *error) const
{
    const Operation *operation = m_operations.value(operationId, nullptr);
    if (operation == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("Unknown operationId.");
        }
        return {};
    }
    return statusObject(operation);
}

QJsonObject StudioOperationManager::cancelOperation(const QString &operationId, QString *error)
{
    Operation *operation = m_operations.value(operationId, nullptr);
    if (operation == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("Unknown operationId.");
        }
        return {};
    }
    if (operation->status == QStringLiteral("running")) {
        if (operation->process != nullptr) {
            operation->process->kill();
        }
        finishOperation(operation,
                        QStringLiteral("canceled"),
                        {},
                        QStringLiteral("CANCELED"),
                        QStringLiteral("Operation canceled by the client."));
    }
    return statusObject(operation);
}

QJsonObject StudioOperationManager::startAdbOperation(const QString &serial,
                                                      Kind kind,
                                                      const QString &type,
                                                      const QStringList &arguments,
                                                      int timeoutMilliseconds,
                                                      const QString &resourceKey,
                                                      QString *error)
{
    if (serial.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("No authorized Android device is active.");
        }
        return {};
    }
    if (!resourceKey.isEmpty() && m_lockedResources.contains(resourceKey)) {
        if (error != nullptr) {
            *error = QStringLiteral("Another OpenCode device control operation is running.");
        }
        return {};
    }

    pruneOperations();
    auto *operation = new Operation;
    operation->id = operationId();
    operation->type = type;
    operation->serial = serial;
    operation->status = QStringLiteral("running");
    operation->resourceKey = resourceKey;
    operation->kind = kind;
    operation->startedAt = QDateTime::currentDateTimeUtc();
    operation->process = new QProcess(this);
    operation->timeout = new QTimer(this);
    operation->timeout->setSingleShot(true);
    m_operations.insert(operation->id, operation);
    if (!resourceKey.isEmpty()) {
        m_lockedResources.insert(resourceKey);
    }

    if (!QFileInfo::exists(m_adbPath)) {
        finishOperation(operation,
                        QStringLiteral("failed"),
                        {},
                        QStringLiteral("ADB_UNAVAILABLE"),
                        QStringLiteral("adb executable does not exist: %1").arg(m_adbPath));
        return statusObject(operation);
    }

    operation->process->setProcessChannelMode(QProcess::MergedChannels);
    operation->process->setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    connect(operation->process, &QProcess::readyReadStandardOutput, this, [this, operation] {
        if (operation->status != QStringLiteral("running")) {
            return;
        }
        operation->output += operation->process->readAllStandardOutput();
        if (operation->output.size() > kMaximumOperationOutputBytes) {
            operation->process->kill();
            finishOperation(operation,
                            QStringLiteral("failed"),
                            {},
                            QStringLiteral("OUTPUT_LIMIT"),
                            QStringLiteral("Operation output exceeded the allowed size."));
        }
    });
    connect(operation->process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this, operation](int exitCode, QProcess::ExitStatus exitStatus) {
                if (operation->status != QStringLiteral("running")) {
                    return;
                }
                operation->output += operation->process->readAllStandardOutput();
                const bool success = exitStatus == QProcess::NormalExit && exitCode == 0;
                const QJsonObject result = parseResult(operation, success, exitCode);
                if (success) {
                    finishOperation(operation, QStringLiteral("completed"), result);
                } else {
                    finishOperation(operation,
                                    QStringLiteral("failed"),
                                    result,
                                    QStringLiteral("ADB_COMMAND_FAILED"),
                                    result.value(QStringLiteral("detail")).toString());
                }
            });
    connect(operation->process,
            &QProcess::errorOccurred,
            this,
            [this, operation](QProcess::ProcessError processError) {
                if (processError == QProcess::FailedToStart
                    && operation->status == QStringLiteral("running")) {
                    finishOperation(operation,
                                    QStringLiteral("failed"),
                                    {},
                                    QStringLiteral("ADB_START_FAILED"),
                                    operation->process->errorString());
                }
            });
    connect(operation->timeout, &QTimer::timeout, this, [this, operation] {
        if (operation->status != QStringLiteral("running")) {
            return;
        }
        operation->process->kill();
        finishOperation(operation,
                        QStringLiteral("failed"),
                        {},
                        QStringLiteral("TIMEOUT"),
                        QStringLiteral("Operation timed out."));
    });

    operation->timeout->start(timeoutMilliseconds);
    operation->process->start(m_adbPath, arguments);
    return statusObject(operation);
}

QJsonObject StudioOperationManager::statusObject(const Operation *operation) const
{
    QJsonObject object{{QStringLiteral("operationId"), operation->id},
                       {QStringLiteral("type"), operation->type},
                       {QStringLiteral("status"), operation->status},
                       {QStringLiteral("deviceSerial"), operation->serial},
                       {QStringLiteral("startedAt"),
                        operation->startedAt.toString(Qt::ISODateWithMs)}};
    if (operation->finishedAt.isValid()) {
        object.insert(QStringLiteral("finishedAt"),
                      operation->finishedAt.toString(Qt::ISODateWithMs));
    }
    if (!operation->result.isEmpty()) {
        object.insert(QStringLiteral("result"), operation->result);
    }
    if (!operation->error.isEmpty()) {
        object.insert(QStringLiteral("error"), operation->error);
    }
    return object;
}

void StudioOperationManager::finishOperation(Operation *operation,
                                             const QString &status,
                                             const QJsonObject &result,
                                             const QString &errorCode,
                                             const QString &errorMessage)
{
    if (operation == nullptr || operation->status != QStringLiteral("running")) {
        return;
    }
    operation->status = status;
    operation->finishedAt = QDateTime::currentDateTimeUtc();
    operation->result = result;
    if (!errorCode.isEmpty()) {
        operation->error = {{QStringLiteral("code"), errorCode},
                            {QStringLiteral("message"), errorMessage}};
    }
    if (!operation->resourceKey.isEmpty()) {
        m_lockedResources.remove(operation->resourceKey);
    }
    if (operation->timeout != nullptr) {
        operation->timeout->stop();
        operation->timeout->deleteLater();
        operation->timeout = nullptr;
    }
    if (operation->process != nullptr) {
        operation->process->deleteLater();
        operation->process = nullptr;
    }
}

QJsonObject StudioOperationManager::parseResult(const Operation *operation,
                                                bool success,
                                                int exitCode) const
{
    const QString output = decodeAdbText(operation->output).trimmed();
    if (!success) {
        const QString detail = output.isEmpty()
            ? QStringLiteral("adb exited with code %1.").arg(exitCode)
            : output.left(10000);
        return {{QStringLiteral("detail"), detail},
                {QStringLiteral("exitCode"), exitCode}};
    }

    switch (operation->kind) {
    case Kind::DeviceSnapshot:
        return snapshotResult(operation->serial, output);
    case Kind::AppsList:
        return appsResult(output);
    case Kind::DeviceAction:
        return {{QStringLiteral("detail"),
                 output.isEmpty() ? QStringLiteral("Operation completed.")
                                  : output.left(10000)}};
    }
    return {};
}

void StudioOperationManager::pruneOperations()
{
    while (m_operations.size() >= kMaximumRetainedOperations) {
        Operation *oldest = nullptr;
        for (Operation *candidate : std::as_const(m_operations)) {
            if (candidate->status == QStringLiteral("running")) {
                continue;
            }
            if (oldest == nullptr || candidate->finishedAt < oldest->finishedAt) {
                oldest = candidate;
            }
        }
        if (oldest == nullptr) {
            return;
        }
        m_operations.remove(oldest->id);
        delete oldest;
    }
}
