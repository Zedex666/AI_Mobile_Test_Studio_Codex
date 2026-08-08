#include "services/studio_control_server.h"

#include "core/workspace_catalog.h"
#include "services/studio_control_protocol.h"
#include "services/studio_operation_manager.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QList>
#include <QLocalServer>
#include <QLocalSocket>
#include <QRandomGenerator>

#include <algorithm>
#include <utility>

namespace {

constexpr int kMaximumClients = 8;
constexpr auto kProtocolVersion = "1";

QString randomHex(int bytes)
{
    QByteArray data;
    data.reserve(bytes);
    while (data.size() < bytes) {
        const quint32 value = QRandomGenerator::system()->generate();
        data.append(static_cast<char>(value & 0xff));
        data.append(static_cast<char>((value >> 8) & 0xff));
        data.append(static_cast<char>((value >> 16) & 0xff));
        data.append(static_cast<char>((value >> 24) & 0xff));
    }
    data.truncate(bytes);
    return QString::fromLatin1(data.toHex());
}

QString generatedServerName()
{
    return QStringLiteral("ai_mobile_test_studio_%1_%2")
        .arg(QCoreApplication::applicationPid())
        .arg(randomHex(8));
}

QJsonArray capabilities()
{
    return {QStringLiteral("workspace.list"),
            QStringLiteral("workspace.open"),
            QStringLiteral("device.snapshot"),
            QStringLiteral("device.apps.list"),
            QStringLiteral("device.action:keyEvent"),
            QStringLiteral("device.action:launchApp"),
            QStringLiteral("device.action:stopApp"),
            QStringLiteral("device.refresh"),
            QStringLiteral("operation.get"),
            QStringLiteral("operation.cancel")};
}

} // namespace

StudioControlServer::StudioControlServer(QString adbPath, QObject *parent)
    : QObject(parent)
    , m_server(new QLocalServer(this))
    , m_operationManager(new StudioOperationManager(std::move(adbPath), this))
    , m_accessToken(randomHex(32))
{
    m_server->setMaxPendingConnections(kMaximumClients);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    connect(m_server, &QLocalServer::newConnection, this, &StudioControlServer::acceptConnections);
}

StudioControlServer::~StudioControlServer()
{
    stop();
}

bool StudioControlServer::start(QString *error)
{
    if (m_server->isListening()) {
        return true;
    }

    for (int attempt = 0; attempt < 4; ++attempt) {
        m_serverName = generatedServerName();
        QLocalServer::removeServer(m_serverName);
        if (m_server->listen(m_serverName)) {
            return true;
        }
    }
    if (error != nullptr) {
        *error = m_server->errorString();
    }
    m_serverName.clear();
    return false;
}

void StudioControlServer::stop()
{
    const QList<QLocalSocket *> sockets = m_clientBuffers.keys();
    for (QLocalSocket *socket : sockets) {
        socket->disconnect(this);
        socket->disconnectFromServer();
        socket->deleteLater();
    }
    m_clientBuffers.clear();
    if (m_server->isListening()) {
        const QString name = m_serverName;
        m_server->close();
        QLocalServer::removeServer(name);
    }
}

bool StudioControlServer::isListening() const
{
    return m_server->isListening();
}

QString StudioControlServer::serverName() const
{
    return m_serverName;
}

QString StudioControlServer::accessToken() const
{
    return m_accessToken;
}

QString StudioControlServer::protocolVersion() const
{
    return QString::fromLatin1(kProtocolVersion);
}

void StudioControlServer::setDeviceState(const QString &state,
                                         const QString &serial,
                                         const QString &detail)
{
    m_deviceState = state;
    m_deviceSerial = serial;
    m_deviceDetail = detail;
}

void StudioControlServer::setActiveWorkspaceId(const QString &workspaceId)
{
    if (workspaceIndexForId(workspaceId) >= 0) {
        m_activeWorkspaceId = workspaceId;
    }
}

void StudioControlServer::acceptConnections()
{
    while (QLocalSocket *socket = m_server->nextPendingConnection()) {
        if (m_clientBuffers.size() >= kMaximumClients) {
            socket->disconnectFromServer();
            socket->deleteLater();
            continue;
        }
        m_clientBuffers.insert(socket, QByteArray());
        connect(socket, &QLocalSocket::readyRead, this, [this, socket] {
            readClient(socket);
        });
        connect(socket, &QLocalSocket::disconnected, this, [this, socket] {
            m_clientBuffers.remove(socket);
            socket->deleteLater();
        });
    }
}

void StudioControlServer::readClient(QLocalSocket *socket)
{
    auto bufferIterator = m_clientBuffers.find(socket);
    if (bufferIterator == m_clientBuffers.end()) {
        return;
    }
    bufferIterator.value() += socket->readAll();
    if (bufferIterator.value().size() > studio_control::kMaximumFrameBytes + 4) {
        sendError(socket, QJsonValue(), -32700, QStringLiteral("Frame buffer limit exceeded."));
        socket->disconnectFromServer();
        return;
    }

    while (true) {
        QJsonObject request;
        QString decodeError;
        const studio_control::DecodeResult result = studio_control::decodeNextFrame(
            &bufferIterator.value(), &request, &decodeError);
        if (result == studio_control::DecodeResult::Incomplete) {
            return;
        }
        if (result == studio_control::DecodeResult::Invalid) {
            sendError(socket, QJsonValue(), -32700, decodeError);
            socket->disconnectFromServer();
            return;
        }
        handleRequest(socket, request);
    }
}

void StudioControlServer::handleRequest(QLocalSocket *socket, const QJsonObject &request)
{
    const QJsonValue id = request.value(QStringLiteral("id"));
    const QString method = request.value(QStringLiteral("method")).toString();
    if (request.value(QStringLiteral("jsonrpc")).toString() != QStringLiteral("2.0")
        || method.isEmpty() || id.isUndefined()) {
        sendError(socket, id, -32600, QStringLiteral("Invalid JSON-RPC request."));
        return;
    }
    const QJsonValue paramsValue = request.value(QStringLiteral("params"));
    if (!paramsValue.isUndefined() && !paramsValue.isObject()) {
        sendError(socket, id, -32602, QStringLiteral("params must be an object."));
        return;
    }
    const QJsonObject parameters = paramsValue.toObject();
    if (!tokenMatches(parameters.value(QStringLiteral("token")).toString())) {
        sendError(socket, id, -32001, QStringLiteral("Authentication failed."));
        return;
    }

    if (method == QStringLiteral("studio.hello")) {
        sendResult(socket,
                   id,
                   {{QStringLiteral("protocolVersion"), protocolVersion()},
                    {QStringLiteral("serverVersion"), QStringLiteral("1.0.0")},
                    {QStringLiteral("capabilities"), capabilities()}});
        return;
    }
    if (method == QStringLiteral("studio.status")) {
        sendResult(socket, id, statusResult());
        return;
    }
    if (method == QStringLiteral("workspace.list")) {
        QJsonArray workspaces;
        for (const WorkspaceDescriptor &workspace : workspaceCatalog()) {
            workspaces.append(QJsonObject{{QStringLiteral("id"), workspace.id},
                                          {QStringLiteral("title"), workspace.title},
                                          {QStringLiteral("active"),
                                           workspace.id == m_activeWorkspaceId}});
        }
        sendResult(socket, id, {{QStringLiteral("workspaces"), workspaces}});
        return;
    }
    if (method == QStringLiteral("workspace.open")) {
        const QString workspaceId = parameters.value(QStringLiteral("workspaceId")).toString();
        if (workspaceIndexForId(workspaceId) < 0) {
            sendError(socket, id, -32602, QStringLiteral("Unknown workspaceId."));
            return;
        }
        emit workspaceOpenRequested(workspaceId);
        sendResult(socket,
                   id,
                   {{QStringLiteral("workspaceId"), workspaceId},
                    {QStringLiteral("active"), m_activeWorkspaceId == workspaceId}});
        return;
    }
    if (method == QStringLiteral("device.refresh")) {
        emit deviceRefreshRequested();
        sendResult(socket, id, {{QStringLiteral("accepted"), true}});
        return;
    }

    QString operationError;
    if (method == QStringLiteral("device.snapshot")) {
        const QJsonObject result = m_operationManager->startDeviceSnapshot(m_deviceSerial,
                                                                           &operationError);
        if (result.isEmpty()) {
            sendError(socket, id, -32010, operationError);
        } else {
            sendResult(socket, id, result);
        }
        return;
    }
    if (method == QStringLiteral("device.apps.list")) {
        const QJsonObject result = m_operationManager->startAppsList(m_deviceSerial,
                                                                     &operationError);
        if (result.isEmpty()) {
            sendError(socket, id, -32010, operationError);
        } else {
            sendResult(socket, id, result);
        }
        return;
    }
    if (method == QStringLiteral("device.action")) {
        const QJsonObject result = m_operationManager->startDeviceAction(m_deviceSerial,
                                                                         parameters,
                                                                         &operationError);
        if (result.isEmpty()) {
            sendError(socket, id, -32011, operationError);
        } else {
            sendResult(socket, id, result);
        }
        return;
    }
    if (method == QStringLiteral("operation.get")) {
        const QJsonObject result = m_operationManager->operationStatus(
            parameters.value(QStringLiteral("operationId")).toString(), &operationError);
        if (result.isEmpty()) {
            sendError(socket, id, -32020, operationError);
        } else {
            sendResult(socket, id, result);
        }
        return;
    }
    if (method == QStringLiteral("operation.cancel")) {
        const QJsonObject result = m_operationManager->cancelOperation(
            parameters.value(QStringLiteral("operationId")).toString(), &operationError);
        if (result.isEmpty()) {
            sendError(socket, id, -32020, operationError);
        } else {
            sendResult(socket, id, result);
        }
        return;
    }

    sendError(socket, id, -32601, QStringLiteral("Method not found."));
}

void StudioControlServer::sendResult(QLocalSocket *socket,
                                     const QJsonValue &id,
                                     const QJsonObject &result)
{
    socket->write(studio_control::encodeFrame(
        {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
         {QStringLiteral("id"), id},
         {QStringLiteral("result"), result}}));
    socket->flush();
}

void StudioControlServer::sendError(QLocalSocket *socket,
                                    const QJsonValue &id,
                                    int code,
                                    const QString &message)
{
    socket->write(studio_control::encodeFrame(
        {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
         {QStringLiteral("id"), id.isUndefined() ? QJsonValue() : id},
         {QStringLiteral("error"),
          QJsonObject{{QStringLiteral("code"), code},
                      {QStringLiteral("message"), message}}}}));
    socket->flush();
}

bool StudioControlServer::tokenMatches(const QString &supplied) const
{
    const QByteArray expectedBytes = m_accessToken.toLatin1();
    const QByteArray suppliedBytes = supplied.toLatin1();
    const qsizetype comparisonLength = std::max(expectedBytes.size(), suppliedBytes.size());
    unsigned int difference = static_cast<unsigned int>(expectedBytes.size()
                                                        ^ suppliedBytes.size());
    for (qsizetype index = 0; index < comparisonLength; ++index) {
        const unsigned char expected = index < expectedBytes.size()
            ? static_cast<unsigned char>(expectedBytes.at(index))
            : 0;
        const unsigned char actual = index < suppliedBytes.size()
            ? static_cast<unsigned char>(suppliedBytes.at(index))
            : 0;
        difference |= expected ^ actual;
    }
    return difference == 0;
}

QJsonObject StudioControlServer::statusResult() const
{
    return {{QStringLiteral("protocolVersion"), protocolVersion()},
            {QStringLiteral("activeWorkspaceId"), m_activeWorkspaceId},
            {QStringLiteral("device"),
             QJsonObject{{QStringLiteral("state"), m_deviceState},
                         {QStringLiteral("serial"), m_deviceSerial},
                         {QStringLiteral("detail"), m_deviceDetail},
                         {QStringLiteral("connected"),
                          m_deviceState == QStringLiteral("connected")}}},
            {QStringLiteral("capabilities"), capabilities()}};
}
