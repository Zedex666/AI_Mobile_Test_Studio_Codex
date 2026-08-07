#include "services/appium_service.h"

#include <QCoreApplication>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QStandardPaths>

#include <cstdio>

namespace {

QUrl statusUrl(quint16 port)
{
    return QUrl(QStringLiteral("http://127.0.0.1:%1/status").arg(port));
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    const bool missingRuntime = argc > 1
        && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--missing-runtime");
    const bool bundledRuntime = argc > 2
        && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--bundled-runtime");

    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) {
        return 2;
    }
    const quint16 port = server.serverPort();
    if (missingRuntime || bundledRuntime) {
        server.close();
    } else {
        QObject::connect(&server, &QTcpServer::newConnection, &application, [&server] {
            while (QTcpSocket *socket = server.nextPendingConnection()) {
                QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket] {
                    socket->readAll();
                    const QByteArray body = QByteArrayLiteral(
                        "{\"value\":{\"ready\":true,\"build\":{\"version\":\"test\"}}}");
                    const QByteArray response = QByteArrayLiteral("HTTP/1.1 200 OK\r\n")
                        + QByteArrayLiteral("Content-Type: application/json\r\n")
                        + QByteArrayLiteral("Connection: close\r\nContent-Length: ")
                        + QByteArray::number(body.size()) + QByteArrayLiteral("\r\n\r\n") + body;
                    socket->write(response);
                    socket->disconnectFromHost();
                });
            }
        });
    }

    const QString runtimeRoot = bundledRuntime
        ? QString::fromLocal8Bit(argv[2])
        : QStringLiteral("Z:/runtime-that-does-not-exist");
    AppiumService service(runtimeRoot, statusUrl(port));
    QObject::connect(&service,
                     &AppiumService::stateChanged,
                     &application,
                     [&](AppiumService::State state, const QString &detail) {
                         if (!missingRuntime && state == AppiumService::State::ReusingExisting) {
                             if (service.ownsServerProcess()) {
                                 std::fprintf(stderr, "Existing server was not reused.\n");
                                 application.exit(3);
                                 return;
                             }
                             application.exit(0);
                         }
                         if (missingRuntime && state == AppiumService::State::Failed) {
                             if (!detail.contains(QStringLiteral("incomplete"),
                                                  Qt::CaseInsensitive)) {
                                 std::fprintf(stderr, "%s\n", detail.toUtf8().constData());
                                 application.exit(4);
                                 return;
                             }
                             application.exit(0);
                             return;
                         }
                         if (bundledRuntime && state == AppiumService::State::RunningBundled) {
                             if (!service.ownsServerProcess()) {
                                 std::fprintf(stderr, "Bundled Appium process is not owned.\n");
                                 application.exit(6);
                                 return;
                             }
                             application.exit(0);
                         }
                     });

    QTimer::singleShot(bundledRuntime ? 25000 : 6000, &application, [&application] {
        application.exit(5);
    });
    QTimer::singleShot(0, &service, &AppiumService::ensureStarted);
    return application.exec();
}
