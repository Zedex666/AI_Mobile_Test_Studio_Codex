#ifndef AI_MOBILE_TEST_STUDIO_FILE_MANAGER_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_FILE_MANAGER_SERVICE_H

#include <QByteArray>
#include <QMetaType>
#include <QObject>
#include <QProcess>
#include <QQueue>
#include <QString>
#include <QStringList>
#include <QVector>

struct DeviceFileEntry {
    QString name;
    QString permissions;
    qint64 size = 0;
    QString modified;
    bool isDirectory = false;
    bool isLink = false;
    QString linkTarget;
};

Q_DECLARE_METATYPE(DeviceFileEntry)
Q_DECLARE_METATYPE(QVector<DeviceFileEntry>)

class FileManagerService : public QObject
{
    Q_OBJECT

public:
    explicit FileManagerService(QString adbPath, QObject *parent = nullptr);

    void setDeviceSerial(const QString &serial);
    bool busy() const;

public slots:
    void listDirectory(const QString &path);
    void createFolder(const QString &remotePath);
    void uploadFiles(const QStringList &localPaths, const QString &remoteDirectory);
    void downloadFiles(const QStringList &remotePaths, const QString &localDirectory);
    void renamePath(const QString &sourcePath, const QString &destinationPath);
    void duplicatePath(const QString &sourcePath, const QString &destinationPath);
    void changePermissions(const QStringList &remotePaths, const QString &mode);
    void deletePaths(const QStringList &remotePaths);

signals:
    void busyChanged(bool busy);
    void directoryLoaded(const QString &path, const QVector<DeviceFileEntry> &entries);
    void operationStarted(const QString &label, const QString &displayCommand);
    void operationFinished(bool success, const QString &label, const QString &detail);
    void refreshRequested();

private:
    enum class CommandKind {
        List,
        Action,
        Transfer
    };

    struct PendingCommand {
        CommandKind kind = CommandKind::Action;
        QString label;
        QString displayCommand;
        QStringList arguments;
        QString listingPath;
        bool refreshAfter = false;
    };

    void enqueue(PendingCommand command);
    void startNext();
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void failCurrent(const QString &detail);
    void completeQueue();

    static QString quoteRemotePath(const QString &path);
    static QVector<DeviceFileEntry> parseDirectory(const QString &output);

    QString m_adbPath;
    QString m_deviceSerial;
    QByteArray m_output;
    QProcess m_process;
    QQueue<PendingCommand> m_queue;
    PendingCommand m_current;
    bool m_busy = false;
    bool m_refreshAfterQueue = false;
};

#endif // AI_MOBILE_TEST_STUDIO_FILE_MANAGER_SERVICE_H
