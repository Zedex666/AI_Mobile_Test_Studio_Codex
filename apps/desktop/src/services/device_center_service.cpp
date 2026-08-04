#include "services/device_center_service.h"

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>

#include <utility>

namespace {

const QString kRemoteDevicesSettingsKey = QStringLiteral("deviceCenter/remoteDevices");

QString displayDeviceName(const QHash<QString, QString> &properties,
                          const QString &fallback)
{
    const QString manufacturer = properties.value(QStringLiteral("ro.product.manufacturer"));
    const QString model = properties.value(QStringLiteral("ro.product.model"));
    const QString marketName = properties.value(QStringLiteral("ro.product.marketname"));
    if (!marketName.trimmed().isEmpty()) {
        return marketName.trimmed();
    }

    QStringList parts;
    if (!manufacturer.trimmed().isEmpty()) {
        parts.append(manufacturer.trimmed());
    }
    if (!model.trimmed().isEmpty()
        && manufacturer.compare(model, Qt::CaseInsensitive) != 0) {
        parts.append(model.trimmed());
    }
    return parts.isEmpty() ? fallback : parts.join(QLatin1Char(' '));
}

QHash<QString, QString> parseProperties(const QString &output)
{
    QHash<QString, QString> properties;
    const QRegularExpression propertyExpression(
        QStringLiteral("^\\[([^]]+)\\]: \\[(.*)\\]$"));
    const QStringList lines = output.split(
        QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QRegularExpressionMatch match = propertyExpression.match(line.trimmed());
        if (match.hasMatch()) {
            properties.insert(match.captured(1), match.captured(2));
        }
    }
    return properties;
}

} // namespace

DeviceCenterService::DeviceCenterService(QString adbPath, QObject *parent)
    : QObject(parent)
    , m_adbPath(QDir::cleanPath(std::move(adbPath)))
    , m_listProcess(this)
    , m_detailProcess(this)
    , m_commandProcess(this)
    , m_screenshotProcess(this)
{
    connect(&m_listProcess, &QProcess::readyReadStandardOutput, this, [this] {
        m_listOutput += m_listProcess.readAllStandardOutput();
    });
    connect(&m_listProcess, &QProcess::readyReadStandardError, this, [this] {
        m_listError += m_listProcess.readAllStandardError();
    });
    connect(&m_listProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &DeviceCenterService::handleListFinished);
    connect(&m_listProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_devices.clear();
            emit devicesUpdated(m_devices);
            emit operationFinished(false,
                                   tr("刷新设备"),
                                   tr("无法启动 adb.exe：%1").arg(m_listProcess.errorString()));
        }
    });

    connect(&m_detailProcess, &QProcess::readyReadStandardOutput, this, [this] {
        m_detailOutput += m_detailProcess.readAllStandardOutput();
    });
    connect(&m_detailProcess, &QProcess::readyReadStandardError, this, [this] {
        m_detailError += m_detailProcess.readAllStandardError();
    });
    connect(&m_detailProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &DeviceCenterService::handleDetailFinished);
    connect(&m_detailProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            ++m_detailIndex;
            queryNextDeviceDetails();
        }
    });

    connect(&m_commandProcess, &QProcess::readyReadStandardOutput, this, [this] {
        m_commandOutput += m_commandProcess.readAllStandardOutput();
    });
    connect(&m_commandProcess, &QProcess::readyReadStandardError, this, [this] {
        m_commandError += m_commandProcess.readAllStandardError();
    });
    connect(&m_commandProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &DeviceCenterService::handleCommandFinished);
    connect(&m_commandProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            finishCommand(false,
                          tr("无法启动 adb.exe：%1").arg(m_commandProcess.errorString()));
        }
    });

    connect(&m_screenshotProcess, &QProcess::readyReadStandardOutput, this, [this] {
        m_screenshotOutput += m_screenshotProcess.readAllStandardOutput();
    });
    connect(&m_screenshotProcess, &QProcess::readyReadStandardError, this, [this] {
        m_screenshotError += m_screenshotProcess.readAllStandardError();
    });
    connect(&m_screenshotProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                m_screenshotOutput += m_screenshotProcess.readAllStandardOutput();
                m_screenshotError += m_screenshotProcess.readAllStandardError();
                const QString deviceId = m_screenshotDeviceId;
                if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                    const QImage image = QImage::fromData(m_screenshotOutput, "PNG");
                    if (!image.isNull()) {
                        emit screenshotReady(deviceId, image);
                    } else {
                        emit screenshotFailed(deviceId, tr("设备截图数据无效。"));
                    }
                } else {
                    emit screenshotFailed(
                        deviceId,
                        processFailureDetail(m_screenshotProcess,
                                             m_screenshotOutput,
                                             m_screenshotError,
                                             tr("无法获取设备截图。")));
                }
                m_screenshotOutput.clear();
                m_screenshotError.clear();
            });
    connect(&m_screenshotProcess,
            &QProcess::errorOccurred,
            this,
            [this](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart) {
                    emit screenshotFailed(
                        m_screenshotDeviceId,
                        tr("无法启动 adb.exe：%1").arg(m_screenshotProcess.errorString()));
                }
    });
}

DeviceCenterService::~DeviceCenterService()
{
    for (QProcess *process : {&m_listProcess,
                              &m_detailProcess,
                              &m_commandProcess,
                              &m_screenshotProcess}) {
        if (process->state() == QProcess::NotRunning) {
            continue;
        }
        process->terminate();
        if (!process->waitForFinished(500)) {
            process->kill();
            process->waitForFinished(500);
        }
    }
}

QList<DeviceCenterDevice> DeviceCenterService::devices() const
{
    return m_devices;
}

void DeviceCenterService::refreshDevices()
{
    if (m_listProcess.state() != QProcess::NotRunning
        || m_detailProcess.state() != QProcess::NotRunning) {
        m_refreshQueued = true;
        return;
    }
    if (!QFileInfo::exists(m_adbPath)) {
        m_devices.clear();
        emit devicesUpdated(m_devices);
        emit operationFinished(false,
                               tr("刷新设备"),
                               tr("未找到 adb.exe：%1")
                                   .arg(QDir::toNativeSeparators(m_adbPath)));
        return;
    }

    m_refreshQueued = false;
    m_detectedDevices.clear();
    m_listOutput.clear();
    m_listError.clear();
    emit refreshStarted();
    startProcess(m_listProcess, {QStringLiteral("devices"), QStringLiteral("-l")});
}

void DeviceCenterService::connectDevice(const QString &host, int port)
{
    const QString endpoint = normalizedEndpoint(host, port);
    if (endpoint.isEmpty()) {
        emit operationFinished(false, tr("连接设备"), tr("请输入有效的 IP 地址。"));
        return;
    }
    startCommand(CommandAction::Connect,
                 {QStringLiteral("connect"), endpoint},
                 tr("连接设备"),
                 endpoint);
}

void DeviceCenterService::pairDevice(const QString &host,
                                     int port,
                                     const QString &pairingCode)
{
    const QString endpoint = normalizedEndpoint(host, port);
    if (endpoint.isEmpty() || port <= 0 || pairingCode.trimmed().size() < 6) {
        emit operationFinished(false, tr("配对设备"), tr("请填写 IP、端口和 6 位配对码。"));
        return;
    }
    startCommand(CommandAction::Pair,
                 {QStringLiteral("pair"), endpoint, pairingCode.trimmed()},
                 tr("配对设备"),
                 endpoint);
}

void DeviceCenterService::disconnectDevice(const QString &deviceId)
{
    if (!isRemoteDeviceId(deviceId)) {
        emit operationFinished(false, tr("断开设备"), tr("只能断开网络设备。"));
        return;
    }
    startCommand(CommandAction::Disconnect,
                 {QStringLiteral("disconnect"), deviceId},
                 tr("断开设备"),
                 deviceId);
}

void DeviceCenterService::removeRememberedDevice(const QString &deviceId)
{
    QList<DeviceCenterDevice> remembered = loadRememberedDevices();
    for (qsizetype index = remembered.size() - 1; index >= 0; --index) {
        if (remembered.at(index).id == deviceId) {
            remembered.removeAt(index);
        }
    }
    saveRememberedDevices(remembered);

    for (qsizetype index = m_devices.size() - 1; index >= 0; --index) {
        const DeviceCenterDevice &device = m_devices.at(index);
        if (device.id == deviceId && device.remote && !device.online) {
            m_devices.removeAt(index);
        }
    }
    emit devicesUpdated(m_devices);
}

void DeviceCenterService::enableWireless(const QString &deviceId)
{
    if (deviceId.trimmed().isEmpty() || isRemoteDeviceId(deviceId)) {
        emit operationFinished(false,
                               tr("无线模式"),
                               tr("请选择通过 USB 连接的在线设备。"));
        return;
    }
    m_wirelessIp.clear();
    startCommand(CommandAction::QueryWirelessIp,
                 {QStringLiteral("-s"),
                  deviceId,
                  QStringLiteral("shell"),
                  QStringLiteral("ip route")},
                 tr("无线模式"),
                 deviceId);
}

void DeviceCenterService::captureScreenshot(const QString &deviceId)
{
    if (deviceId.trimmed().isEmpty()) {
        return;
    }
    if (m_screenshotProcess.state() != QProcess::NotRunning) {
        m_screenshotProcess.kill();
        m_screenshotProcess.waitForFinished(250);
    }
    m_screenshotDeviceId = deviceId;
    m_screenshotOutput.clear();
    m_screenshotError.clear();
    startProcess(m_screenshotProcess,
                 {QStringLiteral("-s"),
                  deviceId,
                  QStringLiteral("exec-out"),
                  QStringLiteral("screencap"),
                  QStringLiteral("-p")});
}

void DeviceCenterService::handleListFinished(int exitCode,
                                             QProcess::ExitStatus exitStatus)
{
    m_listOutput += m_listProcess.readAllStandardOutput();
    m_listError += m_listProcess.readAllStandardError();
    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        m_devices.clear();
        emit devicesUpdated(m_devices);
        emit operationFinished(false,
                               tr("刷新设备"),
                               processFailureDetail(m_listProcess,
                                                    m_listOutput,
                                                    m_listError,
                                                    tr("ADB 设备检测失败。")));
        return;
    }

    const QString output = QString::fromLocal8Bit(m_listOutput);
    const QStringList lines = output.split(
        QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    for (const QString &rawLine : lines) {
        const QString line = rawLine.simplified();
        if (line.isEmpty() || line.startsWith(QStringLiteral("List of devices"))) {
            continue;
        }
        const QStringList columns = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (columns.size() < 2) {
            continue;
        }

        DeviceCenterDevice device;
        device.id = columns.at(0);
        device.serialNumber = device.id;
        device.state = columns.at(1);
        device.online = device.state == QStringLiteral("device");
        device.remote = isRemoteDeviceId(device.id);
        for (qsizetype index = 2; index < columns.size(); ++index) {
            if (columns.at(index).startsWith(QStringLiteral("model:"))) {
                device.name = columns.at(index).mid(6).replace(QLatin1Char('_'), QLatin1Char(' '));
                break;
            }
        }
        if (device.name.isEmpty()) {
            device.name = device.id;
        }
        m_detectedDevices.append(device);
    }

    m_detailIndex = 0;
    queryNextDeviceDetails();
}

void DeviceCenterService::queryNextDeviceDetails()
{
    while (m_detailIndex >= 0 && m_detailIndex < m_detectedDevices.size()
           && !m_detectedDevices.at(m_detailIndex).online) {
        ++m_detailIndex;
    }
    if (m_detailIndex < 0 || m_detailIndex >= m_detectedDevices.size()) {
        finishRefresh();
        return;
    }

    m_detailDeviceId = m_detectedDevices.at(m_detailIndex).id;
    m_detailOutput.clear();
    m_detailError.clear();
    startProcess(m_detailProcess,
                 {QStringLiteral("-s"),
                  m_detailDeviceId,
                  QStringLiteral("shell"),
                  QStringLiteral("getprop")});
}

void DeviceCenterService::handleDetailFinished(int exitCode,
                                               QProcess::ExitStatus exitStatus)
{
    m_detailOutput += m_detailProcess.readAllStandardOutput();
    m_detailError += m_detailProcess.readAllStandardError();
    if (m_detailIndex >= 0 && m_detailIndex < m_detectedDevices.size()
        && exitStatus == QProcess::NormalExit && exitCode == 0) {
        DeviceCenterDevice &device = m_detectedDevices[m_detailIndex];
        const QHash<QString, QString> properties = parseProperties(
            QString::fromLocal8Bit(m_detailOutput));
        device.serialNumber = properties.value(QStringLiteral("ro.serialno"), device.id);
        device.name = displayDeviceName(properties, device.name);
        device.androidVersion = properties.value(QStringLiteral("ro.build.version.release"));
        device.sdkVersion = properties.value(QStringLiteral("ro.build.version.sdk"));
    }
    ++m_detailIndex;
    queryNextDeviceDetails();
}

void DeviceCenterService::finishRefresh()
{
    QList<DeviceCenterDevice> remembered = loadRememberedDevices();
    QHash<QString, qsizetype> rememberedIndexes;
    for (qsizetype index = 0; index < remembered.size(); ++index) {
        rememberedIndexes.insert(remembered.at(index).id, index);
    }

    QHash<QString, bool> currentIds;
    for (const DeviceCenterDevice &device : std::as_const(m_detectedDevices)) {
        currentIds.insert(device.id, true);
        if (!device.remote) {
            continue;
        }
        if (rememberedIndexes.contains(device.id)) {
            remembered[rememberedIndexes.value(device.id)] = device;
        } else {
            rememberedIndexes.insert(device.id, remembered.size());
            remembered.append(device);
        }
    }

    m_devices = m_detectedDevices;
    for (DeviceCenterDevice device : std::as_const(remembered)) {
        if (currentIds.contains(device.id)) {
            continue;
        }
        device.remote = true;
        device.online = false;
        device.state = QStringLiteral("offline");
        if (device.name.isEmpty()) {
            device.name = device.id;
        }
        m_devices.append(device);
    }
    saveRememberedDevices(remembered);
    emit devicesUpdated(m_devices);

    if (m_refreshQueued) {
        QTimer::singleShot(0, this, &DeviceCenterService::refreshDevices);
    }
}

void DeviceCenterService::startCommand(CommandAction action,
                                       const QStringList &arguments,
                                       const QString &label,
                                       const QString &context,
                                       bool announce)
{
    if (m_commandProcess.state() != QProcess::NotRunning) {
        emit operationFinished(false, label, tr("另一个 ADB 操作正在执行。"));
        return;
    }
    if (!QFileInfo::exists(m_adbPath)) {
        emit operationFinished(false,
                               label,
                               tr("未找到 adb.exe：%1")
                                   .arg(QDir::toNativeSeparators(m_adbPath)));
        return;
    }

    m_commandAction = action;
    m_commandLabel = label;
    m_commandContext = context;
    m_commandOutput.clear();
    m_commandError.clear();
    if (announce) {
        emit operationStarted(label);
    }
    startProcess(m_commandProcess, arguments);
}

void DeviceCenterService::handleCommandFinished(int exitCode,
                                                QProcess::ExitStatus exitStatus)
{
    m_commandOutput += m_commandProcess.readAllStandardOutput();
    m_commandError += m_commandProcess.readAllStandardError();
    const QString output = QString::fromLocal8Bit(m_commandOutput).trimmed();
    const QString combined = (output + QLatin1Char('\n')
                              + QString::fromLocal8Bit(m_commandError))
                                 .trimmed();
    const bool processSucceeded = exitStatus == QProcess::NormalExit && exitCode == 0;

    switch (m_commandAction) {
    case CommandAction::Connect: {
        const bool success = processSucceeded
            && !combined.contains(QStringLiteral("failed"), Qt::CaseInsensitive)
            && !combined.contains(QStringLiteral("cannot"), Qt::CaseInsensitive)
            && !combined.contains(QStringLiteral("unable"), Qt::CaseInsensitive);
        if (success) {
            QList<DeviceCenterDevice> remembered = loadRememberedDevices();
            bool exists = false;
            for (const DeviceCenterDevice &device : std::as_const(remembered)) {
                if (device.id == m_commandContext) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                DeviceCenterDevice device;
                device.id = m_commandContext;
                device.serialNumber = m_commandContext;
                device.name = m_commandContext;
                device.state = QStringLiteral("offline");
                device.remote = true;
                remembered.append(device);
                saveRememberedDevices(remembered);
            }
        }
        finishCommand(success,
                      success ? (combined.isEmpty() ? tr("设备连接命令已完成。") : combined)
                              : processFailureDetail(m_commandProcess,
                                                     m_commandOutput,
                                                     m_commandError,
                                                     tr("连接设备失败。")));
        break;
    }
    case CommandAction::Pair: {
        const bool success = processSucceeded
            && combined.contains(QStringLiteral("Successfully"), Qt::CaseInsensitive);
        finishCommand(success,
                      success ? combined
                              : processFailureDetail(m_commandProcess,
                                                     m_commandOutput,
                                                     m_commandError,
                                                     tr("设备配对失败。")));
        break;
    }
    case CommandAction::Disconnect:
        finishCommand(processSucceeded,
                      processSucceeded
                          ? (combined.isEmpty() ? tr("设备已断开。") : combined)
                          : processFailureDetail(m_commandProcess,
                                                 m_commandOutput,
                                                 m_commandError,
                                                 tr("断开设备失败。")));
        break;
    case CommandAction::QueryWirelessIp: {
        const QRegularExpressionMatch match = QRegularExpression(
            QStringLiteral("\\bsrc\\s+((?:\\d{1,3}\\.){3}\\d{1,3})\\b"))
                                                  .match(combined);
        if (!processSucceeded || !match.hasMatch()) {
            finishCommand(false, tr("无法读取设备的 Wi-Fi IP 地址。"));
            break;
        }
        m_wirelessIp = match.captured(1);
        const QString deviceId = m_commandContext;
        m_commandAction = CommandAction::None;
        QTimer::singleShot(0, this, [this, deviceId] {
            startCommand(CommandAction::EnableTcpip,
                         {QStringLiteral("-s"),
                          deviceId,
                          QStringLiteral("tcpip"),
                          QStringLiteral("5555")},
                         tr("无线模式"),
                         deviceId,
                         false);
        });
        break;
    }
    case CommandAction::EnableTcpip:
        if (!processSucceeded) {
            finishCommand(false,
                          processFailureDetail(m_commandProcess,
                                               m_commandOutput,
                                               m_commandError,
                                               tr("无法启用 ADB TCP/IP 模式。")));
            break;
        }
        m_commandAction = CommandAction::None;
        QTimer::singleShot(650, this, [this] {
            const QString endpoint = normalizedEndpoint(m_wirelessIp, 5555);
            startCommand(CommandAction::ConnectWireless,
                         {QStringLiteral("connect"), endpoint},
                         tr("无线模式"),
                         endpoint,
                         false);
        });
        break;
    case CommandAction::ConnectWireless: {
        const bool success = processSucceeded
            && !combined.contains(QStringLiteral("failed"), Qt::CaseInsensitive)
            && !combined.contains(QStringLiteral("cannot"), Qt::CaseInsensitive)
            && !combined.contains(QStringLiteral("unable"), Qt::CaseInsensitive);
        if (success) {
            QList<DeviceCenterDevice> remembered = loadRememberedDevices();
            bool exists = false;
            for (const DeviceCenterDevice &device : std::as_const(remembered)) {
                if (device.id == m_commandContext) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                DeviceCenterDevice device;
                device.id = m_commandContext;
                device.serialNumber = m_commandContext;
                device.name = m_commandContext;
                device.state = QStringLiteral("offline");
                device.remote = true;
                remembered.append(device);
                saveRememberedDevices(remembered);
            }
        }
        finishCommand(success,
                      success ? tr("无线设备已连接：%1").arg(m_commandContext)
                              : processFailureDetail(m_commandProcess,
                                                     m_commandOutput,
                                                     m_commandError,
                                                     tr("无线设备连接失败。")));
        break;
    }
    case CommandAction::None:
        break;
    }
}

void DeviceCenterService::finishCommand(bool success, const QString &detail)
{
    const QString label = m_commandLabel;
    m_commandAction = CommandAction::None;
    m_commandContext.clear();
    m_wirelessIp.clear();
    emit operationFinished(success, label, detail.trimmed());
    if (success) {
        QTimer::singleShot(250, this, &DeviceCenterService::refreshDevices);
    }
}

void DeviceCenterService::startProcess(QProcess &process, const QStringList &arguments)
{
    process.setWorkingDirectory(QFileInfo(m_adbPath).absolutePath());
    process.start(m_adbPath, arguments);
}

QList<DeviceCenterDevice> DeviceCenterService::loadRememberedDevices() const
{
    QSettings settings;
    QList<DeviceCenterDevice> devices;
    const int size = settings.beginReadArray(kRemoteDevicesSettingsKey);
    devices.reserve(size);
    for (int index = 0; index < size; ++index) {
        settings.setArrayIndex(index);
        DeviceCenterDevice device;
        device.id = settings.value(QStringLiteral("id")).toString();
        if (device.id.isEmpty()) {
            continue;
        }
        device.serialNumber = settings.value(QStringLiteral("serialNumber"), device.id).toString();
        device.name = settings.value(QStringLiteral("name"), device.id).toString();
        device.androidVersion = settings.value(QStringLiteral("androidVersion")).toString();
        device.sdkVersion = settings.value(QStringLiteral("sdkVersion")).toString();
        device.state = QStringLiteral("offline");
        device.remote = true;
        devices.append(device);
    }
    settings.endArray();
    return devices;
}

void DeviceCenterService::saveRememberedDevices(
    const QList<DeviceCenterDevice> &devices) const
{
    QSettings settings;
    settings.beginWriteArray(kRemoteDevicesSettingsKey);
    int outputIndex = 0;
    QHash<QString, bool> written;
    for (const DeviceCenterDevice &device : devices) {
        if (!device.remote || device.id.isEmpty() || written.contains(device.id)) {
            continue;
        }
        settings.setArrayIndex(outputIndex++);
        settings.setValue(QStringLiteral("id"), device.id);
        settings.setValue(QStringLiteral("serialNumber"), device.serialNumber);
        settings.setValue(QStringLiteral("name"), device.name);
        settings.setValue(QStringLiteral("androidVersion"), device.androidVersion);
        settings.setValue(QStringLiteral("sdkVersion"), device.sdkVersion);
        written.insert(device.id, true);
    }
    settings.endArray();
}

bool DeviceCenterService::isRemoteDeviceId(const QString &deviceId)
{
    return deviceId.contains(QLatin1Char(':'))
        && !deviceId.startsWith(QStringLiteral("emulator-"));
}

QString DeviceCenterService::normalizedEndpoint(const QString &host, int port)
{
    QString endpoint = host.trimmed();
    if (endpoint.isEmpty()) {
        return {};
    }
    if (port > 0 && !endpoint.contains(QLatin1Char(':'))) {
        endpoint += QStringLiteral(":%1").arg(port);
    }
    return endpoint;
}

QString DeviceCenterService::processFailureDetail(QProcess &process,
                                                  const QByteArray &stdoutData,
                                                  const QByteArray &stderrData,
                                                  const QString &fallback)
{
    const QString detail = (QString::fromLocal8Bit(stdoutData) + QLatin1Char('\n')
                            + QString::fromLocal8Bit(stderrData))
                               .trimmed();
    return detail.isEmpty() ? QStringLiteral("%1 (%2)").arg(fallback, process.errorString())
                            : detail.right(1600);
}
