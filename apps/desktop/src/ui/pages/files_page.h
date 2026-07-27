#ifndef AI_MOBILE_TEST_STUDIO_FILES_PAGE_H
#define AI_MOBILE_TEST_STUDIO_FILES_PAGE_H

#include "services/file_manager_service.h"
#include "services/overview_service.h"

#include <QStringList>
#include <QWidget>

class QLabel;
class QFrame;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QProgressBar;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QTableWidgetItem;
class QToolButton;

class FilesPage : public QWidget
{
    Q_OBJECT

public:
    explicit FilesPage(QWidget *parent = nullptr);

    void setDeviceConnected(bool connected, const QString &serial);

public slots:
    void activate();
    void refresh();
    void setBusy(bool busy);
    void setDirectory(const QString &path, const QVector<DeviceFileEntry> &entries);
    void setDeviceOverview(const DeviceOverview &overview);
    void showOperationStarted(const QString &label, const QString &displayCommand);
    void showOperationFinished(bool success, const QString &label, const QString &detail);

signals:
    void directoryRequested(const QString &path);
    void createFolderRequested(const QString &remotePath);
    void uploadRequested(const QStringList &localPaths, const QString &remoteDirectory);
    void downloadRequested(const QStringList &remotePaths, const QString &localDirectory);
    void renameRequested(const QString &sourcePath, const QString &destinationPath);
    void duplicateRequested(const QString &sourcePath, const QString &destinationPath);
    void permissionsRequested(const QStringList &remotePaths, const QString &mode);
    void deleteRequested(const QStringList &remotePaths);
    void deviceInfoRequested();

private:
    enum class ViewMode {
        List,
        Grid
    };

    void navigateTo(const QString &path, bool addHistory = true, int historyIndex = -1);
    void goBack();
    void goForward();
    void goUp();
    void showDeviceHome();
    void applyFilter();
    void setViewMode(ViewMode mode);
    void openEntry(int index);
    void updateSelection();
    void updateDetails();
    void updateControls();
    QVector<int> selectedIndexes() const;
    QStringList selectedRemotePaths() const;
    QString remotePath(const DeviceFileEntry &entry) const;
    void createFolder();
    void uploadFiles();
    void downloadFiles();
    void renameEntry();
    void duplicateEntry();
    void changePermissions();
    void deleteEntries();

    static QString normalizePath(const QString &path);
    static QString formatBytes(qint64 bytes);
    static bool validEntryName(const QString &name);

    bool m_connected = false;
    bool m_busy = false;
    QString m_serial;
    QString m_currentPath = QStringLiteral("/");
    QString m_pendingPath;
    bool m_pendingAddHistory = false;
    int m_pendingHistoryIndex = -1;
    QStringList m_history = {QStringLiteral("/")};
    int m_historyIndex = 0;
    QVector<DeviceFileEntry> m_entries;
    ViewMode m_viewMode = ViewMode::List;
    bool m_showingDeviceHome = true;

    QLabel *m_deviceDot = nullptr;
    QLabel *m_deviceStatus = nullptr;
    QLabel *m_itemCount = nullptr;
    QLabel *m_selectionStatus = nullptr;
    QLabel *m_operationStatus = nullptr;
    QToolButton *m_backButton = nullptr;
    QToolButton *m_homeButton = nullptr;
    QToolButton *m_forwardButton = nullptr;
    QToolButton *m_upButton = nullptr;
    QToolButton *m_refreshButton = nullptr;
    QLineEdit *m_addressInput = nullptr;
    QLineEdit *m_searchInput = nullptr;
    QToolButton *m_listViewButton = nullptr;
    QToolButton *m_gridViewButton = nullptr;
    QPushButton *m_newFolderButton = nullptr;
    QPushButton *m_uploadButton = nullptr;
    QPushButton *m_downloadButton = nullptr;
    QToolButton *m_renameButton = nullptr;
    QToolButton *m_duplicateButton = nullptr;
    QToolButton *m_permissionsButton = nullptr;
    QToolButton *m_deleteButton = nullptr;
    QStackedWidget *m_viewStack = nullptr;
    QStackedWidget *m_contentStack = nullptr;
    QTableWidget *m_table = nullptr;
    QListWidget *m_grid = nullptr;
    QPushButton *m_rootDriveButton = nullptr;
    QPushButton *m_internalDriveButton = nullptr;
    QLabel *m_internalDriveSpace = nullptr;
    QProgressBar *m_internalDriveProgress = nullptr;
    QLabel *m_detailsIcon = nullptr;
    QLabel *m_detailsName = nullptr;
    QLabel *m_detailsHint = nullptr;
    QLabel *m_detailType = nullptr;
    QLabel *m_detailPath = nullptr;
    QLabel *m_detailSize = nullptr;
    QLabel *m_detailPermissions = nullptr;
    QLabel *m_detailModified = nullptr;
    QLabel *m_deviceBatteryStatus = nullptr;
    QLabel *m_androidStatus = nullptr;
};

#endif // AI_MOBILE_TEST_STUDIO_FILES_PAGE_H
