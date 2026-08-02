#ifndef AI_MOBILE_TEST_STUDIO_OTHER_PAGE_H
#define AI_MOBILE_TEST_STUDIO_OTHER_PAGE_H

#include <QHash>
#include <QVector>
#include <QWidget>

class QAbstractButton;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QEvent;
class QFrame;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QStackedWidget;
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
    void showOverview();
    void executeCommand(const QString &label, const QString &command);
    void refreshCaptivePortal();
    void refreshAnimations();
    void refreshVibration();

    QVector<QFrame *> m_rows;
    QVector<QToolButton *> m_openButtons;
    QVector<QAbstractButton *> m_actionButtons;
    QLabel *m_deviceStatus = nullptr;
    QLabel *m_operationStatus = nullptr;
    QStackedWidget *m_contentStack = nullptr;
    QLineEdit *m_customCommandInput = nullptr;
    QPlainTextEdit *m_customOutput = nullptr;
    QPlainTextEdit *m_accountOutput = nullptr;
    QLabel *m_androidVersion = nullptr;
    QLabel *m_currentServer = nullptr;
    QComboBox *m_serverCombo = nullptr;
    QHash<QString, QLabel *> m_animationCurrent;
    QHash<QString, QDoubleSpinBox *> m_animationInputs;
    QVector<QCheckBox *> m_statusIconChecks;
    QHash<QString, QLabel *> m_vibrationCurrent;
    QHash<QString, QSpinBox *> m_vibrationInputs;
    QString m_pendingSettingKey;
    QString m_pendingSettingValue;
    int m_currentDetail = -1;
    bool m_connected = false;
    bool m_busy = false;
    QString m_serial;
};

#endif // AI_MOBILE_TEST_STUDIO_OTHER_PAGE_H
