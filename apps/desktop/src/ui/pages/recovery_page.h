#ifndef AI_MOBILE_TEST_STUDIO_RECOVERY_PAGE_H
#define AI_MOBILE_TEST_STUDIO_RECOVERY_PAGE_H

#include <QString>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QStackedWidget;
class QToolButton;

class RecoveryPage : public QWidget
{
    Q_OBJECT

public:
    explicit RecoveryPage(QWidget *parent = nullptr);

    void setDeviceState(bool connected, bool sideloadMode, const QString &serial);

public slots:
    void showOverview();
    void setBusy(bool busy);
    void showSideloadStarted(const QString &displayCommand);
    void setOutput(const QString &output);
    void setProgress(int progress);
    void showSideloadFinished(bool success, const QString &detail);

signals:
    void sideloadRequested(const QString &zipPath);
    void cancelRequested();

private:
    void showSideloadWorkspace();
    void selectZipFile();
    void startSideload();
    void copyCommand();
    void updateControls();

    bool m_connected = false;
    bool m_sideloadMode = false;
    bool m_busy = false;
    QString m_serial;
    QString m_displayCommand;
    QLabel *m_deviceDot = nullptr;
    QLabel *m_deviceStatus = nullptr;
    QLabel *m_modeMessage = nullptr;
    QLabel *m_operationStatus = nullptr;
    QStackedWidget *m_workspaceStack = nullptr;
    QToolButton *m_backButton = nullptr;
    QLineEdit *m_filePath = nullptr;
    QToolButton *m_selectButton = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QPlainTextEdit *m_output = nullptr;
};

#endif // AI_MOBILE_TEST_STUDIO_RECOVERY_PAGE_H
