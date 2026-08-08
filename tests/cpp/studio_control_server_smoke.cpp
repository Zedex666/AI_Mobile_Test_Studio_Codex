#include "services/studio_control_protocol.h"
#include "services/studio_control_server.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonArray>
#include <QLocalSocket>
#include <QTimer>

#include <iostream>

namespace {

QJsonObject request(QLocalSocket *socket,
                    int id,
                    const QString &method,
                    const QJsonObject &parameters)
{
    socket->write(studio_control::encodeFrame(
        {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
         {QStringLiteral("id"), id},
         {QStringLiteral("method"), method},
         {QStringLiteral("params"), parameters}}));
    socket->flush();

    QByteArray buffer;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(socket, &QLocalSocket::readyRead, &loop, [&] {
        buffer += socket->readAll();
        QJsonObject response;
        QString error;
        if (studio_control::decodeNextFrame(&buffer, &response, &error)
            == studio_control::DecodeResult::Complete) {
            buffer = studio_control::encodeFrame(response);
            loop.quit();
        }
    });
    timeout.start(3000);
    loop.exec();
    if (!timeout.isActive()) {
        return {};
    }

    QJsonObject response;
    QString error;
    if (studio_control::decodeNextFrame(&buffer, &response, &error)
        != studio_control::DecodeResult::Complete) {
        return {};
    }
    return response;
}

bool require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << message << std::endl;
    }
    return condition;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);

    const QJsonObject sample{{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                             {QStringLiteral("id"), 7},
                             {QStringLiteral("method"), QStringLiteral("test")}};
    const QByteArray sampleFrame = studio_control::encodeFrame(sample);
    QByteArray partialFrame = sampleFrame.left(3);
    QJsonObject decoded;
    QString decodeError;
    if (!require(studio_control::decodeNextFrame(&partialFrame, &decoded, &decodeError)
                     == studio_control::DecodeResult::Incomplete,
                 "Fragmented frame was not reported as incomplete.")) {
        return 1;
    }
    partialFrame += sampleFrame.mid(3);
    if (!require(studio_control::decodeNextFrame(&partialFrame, &decoded, &decodeError)
                     == studio_control::DecodeResult::Complete
                     && decoded == sample,
                 "Complete frame did not round-trip.")) {
        return 2;
    }

    StudioControlServer server(QCoreApplication::applicationFilePath());
    QString startError;
    if (!require(server.start(&startError), "Control server failed to listen.")) {
        std::cerr << startError.toStdString() << std::endl;
        return 3;
    }
    QObject::connect(&server,
                     &StudioControlServer::workspaceOpenRequested,
                     &server,
                     [&server](const QString &workspaceId) {
                         server.setActiveWorkspaceId(workspaceId);
                     });

    QLocalSocket socket;
    socket.connectToServer(server.serverName());
    if (!require(socket.waitForConnected(3000), "Client failed to connect.")) {
        return 4;
    }

    QJsonObject response = request(&socket,
                                   1,
                                   QStringLiteral("studio.status"),
                                   {{QStringLiteral("token"), QStringLiteral("wrong")}});
    if (!require(response.value(QStringLiteral("error")).toObject()
                         .value(QStringLiteral("code")).toInt()
                     == -32001,
                 "Invalid token was not rejected.")) {
        return 5;
    }

    const QJsonObject authenticated{{QStringLiteral("token"), server.accessToken()}};
    response = request(&socket, 2, QStringLiteral("studio.hello"), authenticated);
    if (!require(response.value(QStringLiteral("result")).toObject()
                         .value(QStringLiteral("protocolVersion")).toString()
                     == QStringLiteral("1"),
                 "Hello response did not report protocol version 1.")) {
        return 6;
    }

    response = request(&socket, 3, QStringLiteral("workspace.list"), authenticated);
    if (!require(response.value(QStringLiteral("result")).toObject()
                         .value(QStringLiteral("workspaces")).toArray().size()
                     == 16,
                 "Workspace catalog size is incorrect.")) {
        return 7;
    }

    QJsonObject openParameters = authenticated;
    openParameters.insert(QStringLiteral("workspaceId"), QStringLiteral("settings"));
    response = request(&socket, 4, QStringLiteral("workspace.open"), openParameters);
    if (!require(response.value(QStringLiteral("result")).toObject()
                         .value(QStringLiteral("active")).toBool(),
                 "Workspace open was not applied synchronously.")) {
        return 8;
    }

    response = request(&socket, 5, QStringLiteral("studio.status"), authenticated);
    if (!require(response.value(QStringLiteral("result")).toObject()
                         .value(QStringLiteral("activeWorkspaceId")).toString()
                     == QStringLiteral("settings"),
                 "Status did not retain the active workspace.")) {
        return 9;
    }

    return 0;
}
