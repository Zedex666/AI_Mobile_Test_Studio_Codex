#ifndef AI_MOBILE_TEST_STUDIO_PROCESS_PAGE_H
#define AI_MOBILE_TEST_STUDIO_PROCESS_PAGE_H

#include "services/process_service.h"

#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QModelIndex;
class QTableView;
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
    void showError(const QString &message);
    void showStopResult(bool success,
                        const QString &packageName,
                        const QString &detail);

signals:
    void refreshRequested();
    void stopPackageRequested(const QString &packageName);

private:
    void updateSelection();
    void updateCount();
    void stopSelectedPackage();
    const DeviceProcessEntry *selectedProcess() const;

    bool m_connected = false;
    bool m_sampling = false;
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
};

#endif // AI_MOBILE_TEST_STUDIO_PROCESS_PAGE_H
