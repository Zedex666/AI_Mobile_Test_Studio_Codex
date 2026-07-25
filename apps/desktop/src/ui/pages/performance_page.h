#ifndef AI_MOBILE_TEST_STUDIO_PERFORMANCE_PAGE_H
#define AI_MOBILE_TEST_STUDIO_PERFORMANCE_PAGE_H

#include "services/performance_service.h"

#include <QElapsedTimer>
#include <QTimer>
#include <QVector>
#include <QWidget>

class QLabel;
class QGridLayout;
class QEvent;
class QResizeEvent;
class QScrollArea;
class PerformanceGraph;

class PerformancePage : public QWidget
{
    Q_OBJECT

public:
    explicit PerformancePage(QWidget *parent = nullptr);

    void setDeviceConnected(bool connected, const QString &serial);

public slots:
    void applySample(const PerformanceSample &sample);
    void showSamplingError(const QString &message);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct CorePanel {
        QWidget *widget = nullptr;
        QLabel *title = nullptr;
        QLabel *value = nullptr;
        PerformanceGraph *graph = nullptr;
    };

    void rebuildCorePanels(int count);
    void clearHistory();
    void syncScrollContent();
    void updateUptime();

    bool m_connected = false;
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_scrollContent = nullptr;
    QLabel *m_uptimeLabel = nullptr;
    QLabel *m_batteryLabel = nullptr;
    QLabel *m_batteryInfoLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_cpuTitleLabel = nullptr;
    QLabel *m_cpuValueLabel = nullptr;
    QLabel *m_memoryTitleLabel = nullptr;
    QLabel *m_memoryValueLabel = nullptr;
    QLabel *m_fpsValueLabel = nullptr;
    QLabel *m_fpsTitleLabel = nullptr;
    PerformanceGraph *m_cpuGraph = nullptr;
    PerformanceGraph *m_memoryGraph = nullptr;
    PerformanceGraph *m_fpsGraph = nullptr;
    QGridLayout *m_coreGrid = nullptr;
    QVector<CorePanel> m_corePanels;
    QTimer m_uptimeTimer;
    QElapsedTimer m_uptimeElapsed;
    double m_sampledUptimeSeconds = 0.0;
};

#endif // AI_MOBILE_TEST_STUDIO_PERFORMANCE_PAGE_H
