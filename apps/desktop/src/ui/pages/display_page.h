#ifndef AI_MOBILE_TEST_STUDIO_DISPLAY_PAGE_H
#define AI_MOBILE_TEST_STUDIO_DISPLAY_PAGE_H

#include "services/display_service.h"

#include <QVector>
#include <QWidget>

class QButtonGroup;
class QFrame;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;
class QToolButton;

class DisplayPage : public QWidget
{
    Q_OBJECT

public:
    explicit DisplayPage(QWidget *parent = nullptr);

    void activate();

public slots:
    void setDeviceConnected(bool connected, const QString &serial);
    void setBusy(bool busy);
    void setSettings(const DisplaySettings &settings);
    void showError(const QString &message);
    void showOperationResult(bool success, const QString &label, const QString &detail);

signals:
    void refreshRequested();
    void applyRequested(int width, int height, int density, int timeoutSeconds);
    void resetRequested();
    void refreshRateRequested(double rate);
    void darkModeRequested(bool enabled);
    void fontScaleRequested(double scale);
    void animationScaleRequested(double scale);

private:
    void applyInputs();
    void updateSuggestions(const DisplaySettings &settings);
    void updateRefreshRates(const DisplaySettings &settings);
    void updateInteractiveState();
    void setStatus(const QString &text, const QString &color);
    void setSegmentSelection(QButtonGroup *group, int id);

    QLabel *m_statusLabel = nullptr;
    QLabel *m_physicalResolutionValue = nullptr;
    QLabel *m_physicalDensityValue = nullptr;
    QLabel *m_smallestWidthValue = nullptr;
    QLabel *m_fontScaleValue = nullptr;
    QLineEdit *m_widthInput = nullptr;
    QLineEdit *m_heightInput = nullptr;
    QLineEdit *m_densityInput = nullptr;
    QLineEdit *m_timeoutInput = nullptr;
    QToolButton *m_refreshButton = nullptr;
    QPushButton *m_applyButton = nullptr;
    QPushButton *m_resetButton = nullptr;
    QFrame *m_dimensionsCard = nullptr;
    QFrame *m_refreshRateCard = nullptr;
    QFrame *m_appearanceCard = nullptr;
    QFrame *m_animationsCard = nullptr;
    QHBoxLayout *m_refreshRateLayout = nullptr;
    QButtonGroup *m_themeGroup = nullptr;
    QButtonGroup *m_animationGroup = nullptr;
    QSlider *m_fontScaleSlider = nullptr;
    QVector<QPushButton *> m_suggestionButtons;
    QVector<QPushButton *> m_rateButtons;
    bool m_connected = false;
    bool m_busy = false;
    bool m_hasSettings = false;
    QString m_serial;
};

#endif // AI_MOBILE_TEST_STUDIO_DISPLAY_PAGE_H
