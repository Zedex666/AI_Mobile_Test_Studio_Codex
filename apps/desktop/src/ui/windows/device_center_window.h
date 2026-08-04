#ifndef AI_MOBILE_TEST_STUDIO_DEVICE_CENTER_WINDOW_H
#define AI_MOBILE_TEST_STUDIO_DEVICE_CENTER_WINDOW_H

#include "services/device_center_service.h"

#include <QMainWindow>
#include <QPixmap>

class QLabel;
class QLineEdit;
class QModelIndex;
class QPushButton;
class QShowEvent;
class QHideEvent;
class QCloseEvent;
class QResizeEvent;
class QSplitter;
class QTableWidget;
class QTimer;
class QToolButton;

class DeviceCenterWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DeviceCenterWindow(DeviceCenterService *service, QWidget *parent = nullptr);

    void setActiveDeviceSerial(const QString &serial);

signals:
    void deviceActivationRequested(const QString &serial);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void buildUi();
    void applyLanguage();
    void populateDevices(const QList<DeviceCenterDevice> &devices);
    void applyFilter();
    void updateSelectionState();
    void updateActiveRow();
    void showPairDialog();
    void requestSelectedScreenshot();
    void showPreviewMessage(const QString &message);
    void updatePreviewPixmap();
    void saveWindowState();
    const DeviceCenterDevice *selectedDevice() const;
    static QString statusText(const DeviceCenterDevice &device);

    DeviceCenterService *m_service = nullptr;
    QList<DeviceCenterDevice> m_devices;
    QLineEdit *m_ipInput = nullptr;
    QLineEdit *m_portInput = nullptr;
    QPushButton *m_connectButton = nullptr;
    QPushButton *m_pairButton = nullptr;
    QToolButton *m_wirelessButton = nullptr;
    QToolButton *m_disconnectButton = nullptr;
    QToolButton *m_deleteButton = nullptr;
    QLineEdit *m_filterInput = nullptr;
    QToolButton *m_refreshButton = nullptr;
    QTableWidget *m_deviceTable = nullptr;
    QSplitter *m_splitter = nullptr;
    QLabel *m_previewLabel = nullptr;
    QTimer *m_refreshTimer = nullptr;
    QPixmap m_previewPixmap;
    QString m_selectedDeviceId;
    QString m_activeDeviceSerial;
    bool m_commandBusy = false;
};

#endif // AI_MOBILE_TEST_STUDIO_DEVICE_CENTER_WINDOW_H
