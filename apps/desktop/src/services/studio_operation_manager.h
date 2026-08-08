#ifndef AI_MOBILE_TEST_STUDIO_OPERATION_MANAGER_H
#define AI_MOBILE_TEST_STUDIO_OPERATION_MANAGER_H

#include <QDateTime>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QStringList>

class QProcess;
class QTimer;

class StudioOperationManager : public QObject
{
    Q_OBJECT

public:
    explicit StudioOperationManager(QString adbPath, QObject *parent = nullptr);
    ~StudioOperationManager() override;

    QJsonObject startDeviceSnapshot(const QString &serial, QString *error);
    QJsonObject startAppsList(const QString &serial, QString *error);
    QJsonObject startDeviceAction(const QString &serial,
                                  const QJsonObject &parameters,
                                  QString *error);
    QJsonObject operationStatus(const QString &operationId, QString *error) const;
    QJsonObject cancelOperation(const QString &operationId, QString *error);

private:
    enum class Kind {
        DeviceSnapshot,
        AppsList,
        DeviceAction
    };

    struct Operation {
        QString id;
        QString type;
        QString serial;
        QString status;
        QString resourceKey;
        Kind kind = Kind::DeviceSnapshot;
        QDateTime startedAt;
        QDateTime finishedAt;
        QByteArray output;
        QJsonObject result;
        QJsonObject error;
        QProcess *process = nullptr;
        QTimer *timeout = nullptr;
    };

    QJsonObject startAdbOperation(const QString &serial,
                                  Kind kind,
                                  const QString &type,
                                  const QStringList &arguments,
                                  int timeoutMilliseconds,
                                  const QString &resourceKey,
                                  QString *error);
    QJsonObject statusObject(const Operation *operation) const;
    void finishOperation(Operation *operation,
                         const QString &status,
                         const QJsonObject &result,
                         const QString &errorCode = QString(),
                         const QString &errorMessage = QString());
    QJsonObject parseResult(const Operation *operation, bool success, int exitCode) const;
    void pruneOperations();

    QString m_adbPath;
    QHash<QString, Operation *> m_operations;
    QSet<QString> m_lockedResources;
};

#endif // AI_MOBILE_TEST_STUDIO_OPERATION_MANAGER_H
