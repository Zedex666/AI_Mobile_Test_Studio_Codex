#ifndef AI_MOBILE_TEST_STUDIO_DISPLAY_SERVICE_H
#define AI_MOBILE_TEST_STUDIO_DISPLAY_SERVICE_H

#include <QMetaType>
#include <QObject>
#include <QProcess>
#include <QQueue>
#include <QString>
#include <QVector>

struct DisplaySettings {
    int physicalWidth = 0;
    int physicalHeight = 0;
    int physicalDensity = 0;
    int currentWidth = 0;
    int currentHeight = 0;
    int currentDensity = 0;
    int smallestWidthDp = 0;
    int screenTimeoutSeconds = 60;
    double refreshRateHz = 0.0;
    QVector<double> supportedRefreshRatesHz;
    bool darkModeEnabled = false;
    double fontScale = 1.0;
    double animationScale = 1.0;
};

Q_DECLARE_METATYPE(DisplaySettings)

class DisplayService : public QObject
{
    Q_OBJECT

public:
    explicit DisplayService(QString adbPath, QObject *parent = nullptr);

    void setDeviceSerial(const QString &serial);

public slots:
    void refresh();
    void applyDimensions(int width, int height, int density, int timeoutSeconds);
    void resetDimensions();
    void setRefreshRate(double rate);
    void setDarkMode(bool enabled);
    void setFontScale(double scale);
    void setAnimationScale(double scale);

signals:
    void busyChanged(bool busy);
    void settingsLoaded(const DisplaySettings &settings);
    void settingsError(const QString &message);
    void operationFinished(bool success, const QString &label, const QString &detail);

private:
    enum class Request {
        Idle,
        Refresh,
        Action
    };

    struct PendingAction {
        QString label;
        QString script;
    };

    void enqueueAction(const QString &label, const QString &script);
    void startAction(const PendingAction &action);
    void startNextAction();
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void setBusy(bool busy);
    static DisplaySettings parseSettings(const QString &output);

    QString m_adbPath;
    QString m_deviceSerial;
    QString m_output;
    QString m_actionLabel;
    QProcess m_process;
    QQueue<PendingAction> m_actions;
    Request m_request = Request::Idle;
    bool m_busy = false;
    bool m_cancelled = false;
    bool m_refreshAfterCancel = false;
};

#endif // AI_MOBILE_TEST_STUDIO_DISPLAY_SERVICE_H
