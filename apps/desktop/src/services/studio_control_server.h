#ifndef AI_MOBILE_TEST_STUDIO_CONTROL_SERVER_H
#define AI_MOBILE_TEST_STUDIO_CONTROL_SERVER_H

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>

class QLocalServer;
class QLocalSocket;
class StudioOperationManager;

class StudioControlServer : public QObject
{
    Q_OBJECT

public:
    explicit StudioControlServer(QString adbPath, QObject *parent = nullptr);
    ~StudioControlServer() override;

    bool start(QString *error = nullptr);
    void stop();
    bool isListening() const;
    QString serverName() const;
    QString accessToken() const;
    QString protocolVersion() const;

    void setDeviceState(const QString &state,
                        const QString &serial,
                        const QString &detail);
    void setActiveWorkspaceId(const QString &workspaceId);

signals:
    void workspaceOpenRequested(const QString &workspaceId);
    void deviceRefreshRequested();

private:
    void acceptConnections();
    void readClient(QLocalSocket *socket);
    void handleRequest(QLocalSocket *socket, const QJsonObject &request);
    void sendResult(QLocalSocket *socket,
                    const QJsonValue &id,
                    const QJsonObject &result);
    void sendError(QLocalSocket *socket,
                   const QJsonValue &id,
                   int code,
                   const QString &message);
    bool tokenMatches(const QString &supplied) const;
    QJsonObject statusResult() const;

    QLocalServer *m_server = nullptr;
    StudioOperationManager *m_operationManager = nullptr;
    QHash<QLocalSocket *, QByteArray> m_clientBuffers;
    QString m_serverName;
    QString m_accessToken;
    QString m_deviceState = QStringLiteral("tool-unavailable");
    QString m_deviceSerial;
    QString m_deviceDetail;
    QString m_activeWorkspaceId = QStringLiteral("overview");
};

#endif // AI_MOBILE_TEST_STUDIO_CONTROL_SERVER_H
