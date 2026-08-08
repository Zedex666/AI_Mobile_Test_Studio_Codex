#include "services/studio_operation_manager.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QTimer>

#include <iostream>

namespace {

bool require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << message << std::endl;
    }
    return condition;
}

QJsonObject waitForOperation(StudioOperationManager *manager, const QString &operationId)
{
    QJsonObject status;
    QEventLoop loop;
    QTimer poll;
    QTimer timeout;
    poll.setInterval(20);
    timeout.setSingleShot(true);
    QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
        QString error;
        status = manager->operationStatus(operationId, &error);
        if (status.value(QStringLiteral("status")).toString() != QStringLiteral("running")) {
            loop.quit();
        }
    });
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    poll.start();
    timeout.start(5000);
    loop.exec();
    return status;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    if (argc < 2) {
        std::cerr << "fake adb helper path is required" << std::endl;
        return 1;
    }

    QTemporaryDir coordination;
    if (!require(coordination.isValid(), "Failed to create coordination directory.")) {
        return 2;
    }
    qputenv("AMTS_FAKE_ADB_COORD_DIR", coordination.path().toUtf8());

    StudioOperationManager manager(QString::fromLocal8Bit(argv[1]));
    QString error;
    const QJsonObject snapshot = manager.startDeviceSnapshot(QStringLiteral("serial-1"), &error);
    const QJsonObject apps = manager.startAppsList(QStringLiteral("serial-1"), &error);
    if (!require(snapshot.value(QStringLiteral("status")).toString() == QStringLiteral("running")
                     && apps.value(QStringLiteral("status")).toString() == QStringLiteral("running"),
                 "Independent read operations did not start concurrently.")) {
        return 3;
    }

    const QJsonObject snapshotDone = waitForOperation(
        &manager, snapshot.value(QStringLiteral("operationId")).toString());
    const QJsonObject appsDone = waitForOperation(
        &manager, apps.value(QStringLiteral("operationId")).toString());
    if (!require(snapshotDone.value(QStringLiteral("status")).toString()
                         == QStringLiteral("completed")
                     && snapshotDone.value(QStringLiteral("result")).toObject()
                                .value(QStringLiteral("device")).toObject()
                                .value(QStringLiteral("model")).toString()
                            == QStringLiteral("Test Phone"),
                 "Device snapshot operation failed or was parsed incorrectly.")) {
        return 4;
    }
    if (!require(appsDone.value(QStringLiteral("status")).toString()
                         == QStringLiteral("completed")
                     && appsDone.value(QStringLiteral("result")).toObject()
                                .value(QStringLiteral("apps")).toArray().size()
                            == 2,
                 "Apps list operation failed or was parsed incorrectly.")) {
        return 5;
    }

    const QJsonObject actionParameters{{QStringLiteral("action"), QStringLiteral("keyEvent")},
                                       {QStringLiteral("keyCode"), QStringLiteral("KEYCODE_HOME")}};
    const QJsonObject firstAction = manager.startDeviceAction(QStringLiteral("serial-1"),
                                                               actionParameters,
                                                               &error);
    error.clear();
    const QJsonObject conflictingAction = manager.startDeviceAction(QStringLiteral("serial-1"),
                                                                     actionParameters,
                                                                     &error);
    if (!require(firstAction.value(QStringLiteral("status")).toString()
                         == QStringLiteral("running")
                     && conflictingAction.isEmpty() && !error.isEmpty(),
                 "Device control resource lock did not reject a conflicting action.")) {
        return 6;
    }
    const QJsonObject actionDone = waitForOperation(
        &manager, firstAction.value(QStringLiteral("operationId")).toString());
    if (!require(actionDone.value(QStringLiteral("status")).toString()
                     == QStringLiteral("completed"),
                 "Safe device action did not complete.")) {
        return 7;
    }

    error.clear();
    const QJsonObject nextAction = manager.startDeviceAction(QStringLiteral("serial-1"),
                                                              actionParameters,
                                                              &error);
    if (!require(nextAction.value(QStringLiteral("status")).toString()
                     == QStringLiteral("running"),
                 "Device control resource lock was not released.")) {
        return 8;
    }
    manager.cancelOperation(nextAction.value(QStringLiteral("operationId")).toString(), &error);
    return 0;
}
