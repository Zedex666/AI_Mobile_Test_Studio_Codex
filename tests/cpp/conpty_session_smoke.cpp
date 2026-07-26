#include "services/conpty_session.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

#include <cstdio>

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    if (argc < 5) {
        return 2;
    }
    const bool commandSmoke = argc >= 6;
    const QString echoHelper = QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
    const QString nodeExecutable = QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath();
    const QString nodePtyModule = QFileInfo(QString::fromLocal8Bit(argv[3])).absoluteFilePath();
    const QString hostScript = QFileInfo(QString::fromLocal8Bit(argv[4])).absoluteFilePath();
    if (!QFileInfo::exists(echoHelper) || !QFileInfo::exists(nodeExecutable)
        || !QFileInfo::exists(hostScript)) {
        return 2;
    }

    ConPtySession session(echoHelper,
                          QDir::tempPath(),
                          nodeExecutable,
                          nodePtyModule,
                          hostScript);
    if (commandSmoke) {
        QStringList arguments;
        for (int index = 6; index < argc; ++index) {
            arguments.append(QString::fromLocal8Bit(argv[index]));
        }
        session.setArguments(arguments);
    }
    QByteArray output;
    bool markerSeen = false;
    const QByteArray expectedMarker = commandSmoke
        ? QString::fromLocal8Bit(argv[5]).toUtf8()
        : QByteArrayLiteral("__CONPTY_SMOKE_OK__");

    QObject::connect(&session,
                     &TerminalSession::started,
                     &application,
                     [&session, commandSmoke] {
        session.resize(100, 30);
        if (!commandSmoke) {
            session.write(QByteArrayLiteral("OK\r"));
        }
    });

    QObject::connect(&session,
                     &TerminalSession::outputReady,
                     &application,
                     [&](const QByteArray &data) {
                         output += data;
                         markerSeen = output.contains(expectedMarker);
                     });
    QObject::connect(&session,
                     &TerminalSession::exited,
                     &application,
                     [&](bool wasStarted, const QString &message) {
                         if (!wasStarted || !markerSeen) {
                             std::fprintf(stderr,
                                          "%s\n%s\n",
                                          message.toUtf8().constData(),
                                          output.constData());
                         }
                         application.exit(wasStarted && markerSeen ? 0 : 3);
                     });

    QTimer::singleShot(10000, &application, [&application] {
        application.exit(4);
    });
    QTimer::singleShot(0, &session, &ConPtySession::start);
    return application.exec();
}
