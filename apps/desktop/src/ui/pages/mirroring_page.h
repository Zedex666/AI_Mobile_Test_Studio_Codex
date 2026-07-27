#ifndef AI_MOBILE_TEST_STUDIO_MIRRORING_PAGE_H
#define AI_MOBILE_TEST_STUDIO_MIRRORING_PAGE_H

#include "services/apps_service.h"

#include <QStringList>
#include <QWidget>

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;

class MirroringPage : public QWidget
{
    Q_OBJECT

public:
    explicit MirroringPage(QWidget *parent = nullptr);

    void activate();

public slots:
    void setDeviceConnected(bool connected, const QString &serial);
    void setMirrorRunning(bool running);
    void setApplications(const QVector<AndroidAppSummary> &apps);
    void setCameras(const QStringList &cameras);
    void showError(const QString &message);

signals:
    void launchRequested(const QStringList &arguments);
    void stopRequested();
    void applicationListRequested();
    void cameraListRequested();

private:
    enum Mode {
        MainDisplay = 0,
        VirtualDisplay,
        Camera
    };

    void updateMode(int mode);
    void chooseRecordPath();
    void launchConfigured();
    void launchAdvancedOnly();
    bool buildArguments(QStringList *arguments);
    void setStatus(const QString &text, const QString &color);

    QButtonGroup *m_modeGroup = nullptr;
    QLineEdit *m_maxSizeInput = nullptr;
    QLineEdit *m_maxFpsInput = nullptr;
    QCheckBox *m_fullscreenCheck = nullptr;
    QCheckBox *m_screenOffCheck = nullptr;
    QCheckBox *m_viewOnlyCheck = nullptr;
    QCheckBox *m_recordCheck = nullptr;
    QLineEdit *m_recordPathInput = nullptr;
    QComboBox *m_audioCombo = nullptr;
    QComboBox *m_keyboardCombo = nullptr;
    QComboBox *m_mouseCombo = nullptr;
    QComboBox *m_appCombo = nullptr;
    QLineEdit *m_virtualWidthInput = nullptr;
    QLineEdit *m_virtualHeightInput = nullptr;
    QLineEdit *m_virtualDpiInput = nullptr;
    QCheckBox *m_virtualResizableCheck = nullptr;
    QComboBox *m_cameraCombo = nullptr;
    QLineEdit *m_cameraWidthInput = nullptr;
    QLineEdit *m_cameraHeightInput = nullptr;
    QLineEdit *m_advancedInput = nullptr;
    QFrame *m_virtualCard = nullptr;
    QFrame *m_cameraCard = nullptr;
    QFrame *m_startupCard = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_footerInfoLabel = nullptr;
    QPushButton *m_launchButton = nullptr;
    QPushButton *m_advancedButton = nullptr;
    bool m_connected = false;
    bool m_running = false;
    bool m_appsRequested = false;
    bool m_camerasRequested = false;
    QString m_serial;
};

#endif // AI_MOBILE_TEST_STUDIO_MIRRORING_PAGE_H
