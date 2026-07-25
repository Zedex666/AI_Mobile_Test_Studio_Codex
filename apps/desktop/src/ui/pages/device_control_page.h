#ifndef AI_MOBILE_TEST_STUDIO_DEVICE_CONTROL_PAGE_H
#define AI_MOBILE_TEST_STUDIO_DEVICE_CONTROL_PAGE_H

#include <QVector>
#include <QWidget>

class QLabel;
class QPushButton;

class DeviceControlPage : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceControlPage(QWidget *parent = nullptr);

    void setDeviceConnected(bool connected, const QString &serial);
    void showCommandStarted(const QString &label, const QString &command);
    void showCommandResult(bool success, const QString &label, const QString &detail);

signals:
    void keyEventRequested(const QString &keyCode);
    void rebootRequested(const QString &mode, const QString &label);
    void powerOffRequested(const QString &label);

private:
    QLabel *m_deviceDot = nullptr;
    QLabel *m_deviceStatus = nullptr;
    QLabel *m_commandStatus = nullptr;
    QVector<QPushButton *> m_commandButtons;
};

#endif // AI_MOBILE_TEST_STUDIO_DEVICE_CONTROL_PAGE_H
