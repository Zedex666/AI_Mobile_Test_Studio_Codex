#ifndef AI_MOBILE_TEST_STUDIO_OTHER_PAGE_H
#define AI_MOBILE_TEST_STUDIO_OTHER_PAGE_H

#include <QVector>
#include <QWidget>

class QEvent;
class QFrame;
class QLabel;
class QToolButton;

class OtherPage : public QWidget
{
    Q_OBJECT

public:
    explicit OtherPage(QWidget *parent = nullptr);

public slots:
    void setDeviceConnected(bool connected, const QString &serial);
    void setBusy(bool busy);
    void showCommandStarted(const QString &label, const QString &displayCommand);
    void showCommandResult(bool success,
                           const QString &label,
                           const QString &output);

signals:
    void shellCommandRequested(const QString &label, const QString &command);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void addCommandRow(class QVBoxLayout *layout,
                       int index,
                       const QString &icon,
                       const QString &title,
                       const QString &command);
    void openCommand(int index);
    void runCustomCommand();
    void configureCaptivePortal();
    void configureAnimations();
    void configureSystemBars();
    void configureVibration();

    QVector<QFrame *> m_rows;
    QVector<QToolButton *> m_openButtons;
    QLabel *m_deviceStatus = nullptr;
    QLabel *m_operationStatus = nullptr;
    bool m_connected = false;
    bool m_busy = false;
    QString m_serial;
};

#endif // AI_MOBILE_TEST_STUDIO_OTHER_PAGE_H
