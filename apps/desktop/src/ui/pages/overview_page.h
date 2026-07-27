#ifndef AI_MOBILE_TEST_STUDIO_OVERVIEW_PAGE_H
#define AI_MOBILE_TEST_STUDIO_OVERVIEW_PAGE_H

#include "services/overview_service.h"

#include <QHash>
#include <QPixmap>
#include <QWidget>

class QEvent;
class QGridLayout;
class QLabel;
class QProgressBar;
class QPushButton;
class QResizeEvent;
class QToolButton;

class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QWidget *parent = nullptr);

    void activate();

public slots:
    void setDeviceConnected(bool connected, const QString &serial);
    void setLoading(bool loading);
    void setOverview(const DeviceOverview &overview);
    void setScreenshotLoading(bool loading);
    void setScreenshot(const QByteArray &pngData);
    void showActionResult(bool success, const QString &label, const QString &detail);
    void showError(const QString &message);

signals:
    void refreshRequested();
    void screenshotRequested();
    void shizukuRequested();
    void powerRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void addFactCard(QGridLayout *layout,
                     int row,
                     int column,
                     const QString &key,
                     const QString &icon,
                     const QString &title);
    void resetValues();
    void setValue(const QString &key, const QString &value);
    void updateScreenshotPixmap();

    QHash<QString, QLabel *> m_values;
    QLabel *m_deviceName = nullptr;
    QLabel *m_deviceSubtitle = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_batteryValue = nullptr;
    QLabel *m_batteryDetail = nullptr;
    QLabel *m_memoryValue = nullptr;
    QLabel *m_memoryDetail = nullptr;
    QLabel *m_storageValue = nullptr;
    QLabel *m_storageDetail = nullptr;
    QProgressBar *m_batteryProgress = nullptr;
    QProgressBar *m_memoryProgress = nullptr;
    QProgressBar *m_storageProgress = nullptr;
    QLabel *m_screenshotLabel = nullptr;
    QLabel *m_screenshotPlaceholder = nullptr;
    QPushButton *m_shizukuButton = nullptr;
    QPushButton *m_screenshotButton = nullptr;
    QToolButton *m_powerButton = nullptr;
    QToolButton *m_refreshButton = nullptr;
    QPixmap m_screenshot;
    bool m_connected = false;
    bool m_hasData = false;
    QString m_serial;
};

#endif // AI_MOBILE_TEST_STUDIO_OVERVIEW_PAGE_H
