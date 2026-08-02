#ifndef AI_MOBILE_TEST_STUDIO_PROCESS_PAGE_H
#define AI_MOBILE_TEST_STUDIO_PROCESS_PAGE_H

#include "services/apps_service.h"
#include "services/process_service.h"

#include <QWidget>
#include <QQueue>
#include <QSet>

class QCheckBox;
class QLabel;
class QLineEdit;
class QModelIndex;
class QTableView;
class QTimer;
class QToolButton;
class ProcessFilterProxyModel;
class ProcessModel;

class ProcessPage : public QWidget
{
    Q_OBJECT

public:
    explicit ProcessPage(QWidget *parent = nullptr);

public slots:
    void setDeviceConnected(bool connected, const QString &serial);
    void setSampling(bool sampling);
    void setProcesses(const QVector<DeviceProcessEntry> &processes);
    void setApplications(const QVector<AndroidAppSummary> &apps);
    void showError(const QString &message);
    void showStopResult(bool success,
                        const QString &packageName,
                        const QString &detail);

signals:
    void refreshRequested();
    void stopPackageRequested(const QString &packageName);
    void applicationMetadataRequested(const QStringList &packageNames);

private:
    void applyProcesses(QVector<DeviceProcessEntry> processes);
    void decodeNextIcon();
    void updateSelection();
    void updateCount();
    void stopSelectedPackage();
    const DeviceProcessEntry *selectedProcess() const;

    bool m_connected = false;
    bool m_sampling = false;
    bool m_applyingProcesses = false;
    bool m_hasDeferredProcesses = false;
    QVector<DeviceProcessEntry> m_deferredProcesses;
    QQueue<AndroidAppSummary> m_pendingIconApps;
    QSet<QString> m_loadedIconPackages;
    QSet<QString> m_requestedIconPackages;
    QString m_serial;
    ProcessModel *m_model = nullptr;
    ProcessFilterProxyModel *m_proxy = nullptr;
    QLineEdit *m_filterInput = nullptr;
    QCheckBox *m_onlyApps = nullptr;
    QLabel *m_countLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QToolButton *m_refreshButton = nullptr;
    QToolButton *m_stopButton = nullptr;
    QTableView *m_table = nullptr;
    QTimer *m_scrollIdleTimer = nullptr;
    QTimer *m_iconDecodeTimer = nullptr;
};

#endif // AI_MOBILE_TEST_STUDIO_PROCESS_PAGE_H
