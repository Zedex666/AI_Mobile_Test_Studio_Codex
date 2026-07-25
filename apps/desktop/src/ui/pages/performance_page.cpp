#include "ui/pages/performance_page.h"

#include "ui/common/widget_helpers.h"

#include <QBoxLayout>
#include <QColor>
#include <QDateTime>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHideEvent>
#include <QElapsedTimer>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QScrollArea>
#include <QShowEvent>

#include <algorithm>
#include <cmath>
#include <utility>

class PerformanceGraph : public QWidget
{
public:
    enum class ScaleMode {
        Percent,
        Memory,
        Frames
    };

    PerformanceGraph(QColor color,
                     ScaleMode scaleMode,
                     bool smooth,
                     QWidget *parent = nullptr)
        : QWidget(parent)
        , m_color(std::move(color))
        , m_scaleMode(scaleMode)
        , m_smooth(smooth)
        , m_pollTimer(this)
        , m_animationTimer(this)
    {
        m_monotonicClock.start();
        setMinimumSize(40, 36);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        m_pollTimer.setInterval(kPollIntervalMs);
        m_pollTimer.setTimerType(Qt::PreciseTimer);
        connect(&m_pollTimer, &QTimer::timeout, this, [this] {
            recordValue();
        });
        m_animationTimer.setInterval(16);
        m_animationTimer.setTimerType(Qt::PreciseTimer);
        connect(&m_animationTimer, &QTimer::timeout, this, qOverload<>(&QWidget::update));
    }

    QSize sizeHint() const override
    {
        return QSize(80, 72);
    }

    QSize minimumSizeHint() const override
    {
        return QSize(40, 36);
    }

    void setValue(double value)
    {
        m_latestValue = std::max(0.0, value);
        if (!m_hasValue) {
            m_hasValue = true;
            recordValue();
        }
    }

    void setMaximum(double maximum)
    {
        m_fixedMaximum = std::max(0.0, maximum);
        update();
    }

    void clear()
    {
        m_metrics.clear();
        m_latestValue = 0.0;
        m_hasValue = false;
        m_currentMaximum = 0.0;
        update();
    }

protected:
    void showEvent(QShowEvent *event) override
    {
        QWidget::showEvent(event);
        if (!m_pollTimer.isActive()) {
            m_pollTimer.start();
        }
        if (!m_animationTimer.isActive()) {
            m_animationTimer.start();
        }
        recordValue();
    }

    void hideEvent(QHideEvent *event) override
    {
        m_pollTimer.stop();
        m_animationTimer.stop();
        QWidget::hideEvent(event);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor(QStringLiteral("#f7f8fa")));
        if (width() <= 1 || height() <= 20) {
            return;
        }

        const qint64 monotonicNowMs = m_monotonicClock.elapsed();
        const qint64 wallNowMs = QDateTime::currentMSecsSinceEpoch();
        const QRectF plot(0.0, 18.0, width() - 1.0, height() - 19.0);
        drawTimeGrid(painter, plot, wallNowMs);

        const double maximum = chartMaximum(monotonicNowMs);
        const double chartMaximum = std::max(1.0, maximum * 1.05);
        drawMetric(painter, plot, monotonicNowMs, chartMaximum);
        drawValueGrid(painter, plot, chartMaximum);

        painter.setPen(QPen(QColor(QStringLiteral("#d9d9d9")), 1.0));
        painter.drawRect(QRectF(0.5, 0.5, width() - 1.0, height() - 1.0));
    }

private:
    struct Metric {
        qint64 timestampMs = 0;
        double value = 0.0;
    };

    void recordValue()
    {
        if (!m_hasValue) {
            return;
        }
        const qint64 nowMs = m_monotonicClock.elapsed();
        m_metrics.append({nowMs, m_latestValue});

        const qint64 visibleMillis = qRound64(std::max(1, width()) / kPixelsPerMs);
        const qint64 oldestAllowed = nowMs - visibleMillis * 2 - kPollIntervalMs;
        while (m_metrics.size() > 2 && m_metrics[1].timestampMs < oldestAllowed) {
            m_metrics.removeFirst();
        }
        update();
    }

    double chartMaximum(qint64 nowMs)
    {
        if (m_fixedMaximum > 0.0) {
            return m_fixedMaximum;
        }

        const qint64 startTime = nowMs - kPollIntervalMs
            - qRound64(std::max(1, width()) / kPixelsPerMs);
        double maximum = 0.0;
        for (auto iterator = m_metrics.crbegin(); iterator != m_metrics.crend(); ++iterator) {
            maximum = std::max(maximum, iterator->value);
            if (iterator->timestampMs < startTime) {
                break;
            }
        }
        if (maximum <= 0.0) {
            maximum = m_scaleMode == ScaleMode::Frames ? 2.0 : 10.0;
        } else {
            const double base = std::pow(10.0, std::floor(std::log10(maximum)));
            maximum = std::ceil(maximum / base / 2.0) * base * 2.0;
        }

        constexpr double alpha = 0.2;
        m_currentMaximum = maximum * alpha
            + (m_currentMaximum > 0.0 ? m_currentMaximum : maximum) * (1.0 - alpha);
        return m_currentMaximum;
    }

    void drawMetric(QPainter &painter,
                    const QRectF &plot,
                    qint64 nowMs,
                    double maximum)
    {
        if (m_metrics.isEmpty()) {
            return;
        }

        const qint64 startTime = nowMs - kPollIntervalMs
            - qRound64(plot.width() / kPixelsPerMs);
        int firstIndex = 0;
        while (firstIndex + 1 < m_metrics.size()
               && m_metrics[firstIndex + 1].timestampMs < startTime) {
            ++firstIndex;
        }

        const auto pointFor = [&](const Metric &metric) {
            const qreal x = plot.left() + (metric.timestampMs - startTime) * kPixelsPerMs;
            const qreal y = std::round(plot.bottom()
                                       - plot.height()
                                           * std::clamp(metric.value / maximum, 0.0, 1.0))
                + 0.5;
            return QPointF(x, y);
        };

        QPainterPath linePath;
        QPointF previous = pointFor(m_metrics[firstIndex]);
        linePath.moveTo(previous);
        for (int index = firstIndex + 1; index < m_metrics.size(); ++index) {
            const QPointF point = pointFor(m_metrics[index]);
            if (m_smooth) {
                const qreal middleX = (previous.x() + point.x()) / 2.0;
                linePath.cubicTo(QPointF(middleX, previous.y()),
                                 QPointF(middleX, point.y()),
                                 point);
            } else {
                linePath.lineTo(point.x(), previous.y());
                linePath.lineTo(point);
            }
            previous = point;
        }
        const qreal rightEdge = plot.right() + 5.0;
        linePath.lineTo(rightEdge, previous.y());

        QPainterPath fillPath = linePath;
        fillPath.lineTo(rightEdge, plot.bottom());
        fillPath.lineTo(pointFor(m_metrics[firstIndex]).x(), plot.bottom());
        fillPath.closeSubpath();

        painter.save();
        painter.setClipRect(plot.adjusted(0, -18, 0, 1));
        QColor fillColor = m_color;
        fillColor.setAlpha(51);
        painter.fillPath(fillPath, fillColor);
        painter.setPen(QPen(m_color, 0.5));
        painter.drawPath(linePath);
        painter.restore();
    }

    void drawTimeGrid(QPainter &painter, const QRectF &plot, qint64 nowMs) const
    {
        painter.save();
        painter.setFont(ui::appFont(7));
        qint64 second = static_cast<qint64>(std::ceil(nowMs / 1000.0));
        while (--second > 0) {
            const qreal x = plot.right()
                - ((nowMs - second * 1000) - kPollIntervalMs) * kPixelsPerMs;
            if (x < plot.left() - 55.0) {
                break;
            }
            const bool major = second % 10 == 0;
            painter.setPen(QPen(major ? QColor(0, 0, 0, 20)
                                      : QColor(0, 0, 0, 5),
                                1.0));
            painter.drawLine(QPointF(x, 0), QPointF(x, plot.bottom()));
            if (major) {
                painter.setPen(QColor(QStringLiteral("#969ca5")));
                const QString label = QDateTime::fromMSecsSinceEpoch(second * 1000)
                                          .toString(QStringLiteral("HH:mm:ss"));
                painter.drawText(QRectF(x + 4, 1, 58, 14),
                                 Qt::AlignLeft | Qt::AlignVCenter,
                                 label);
            }
        }
        painter.restore();
    }

    void drawValueGrid(QPainter &painter, const QRectF &plot, double maximum) const
    {
        double base = std::pow(10.0, std::floor(std::log10(maximum)));
        const int firstDigit = static_cast<int>(std::floor(maximum / base));
        if (firstDigit != 1 && firstDigit % 2 == 1) {
            base *= 2.0;
        }
        double scaleValue = std::floor(maximum / base) * base;

        painter.save();
        painter.setFont(ui::appFont(7));
        for (int index = 0; index < 2; ++index) {
            const qreal y = std::round(plot.bottom() - plot.height() * scaleValue / maximum)
                + 0.5;
            const QString label = scaleLabel(scaleValue);
            const qreal labelWidth = painter.fontMetrics().horizontalAdvance(label);
            painter.setPen(QPen(QColor(0, 0, 0, 20), 1.0));
            painter.drawLine(QPointF(plot.left(), y), QPointF(plot.left() + 4, y));
            painter.drawLine(QPointF(plot.left() + labelWidth + 12, y),
                             QPointF(plot.right(), y));
            painter.setPen(QColor(QStringLiteral("#8a919c")));
            painter.drawText(QPointF(plot.left() + 8, y + 3), label);
            scaleValue /= 2.0;
        }
        painter.restore();
    }

    QString scaleLabel(double value) const
    {
        switch (m_scaleMode) {
        case ScaleMode::Percent:
            return QStringLiteral("%1%").arg(qRound(value));
        case ScaleMode::Memory:
            return QStringLiteral("%1MB").arg(qRound64(value));
        case ScaleMode::Frames:
            return QString::number(qRound(value));
        }
        return QString();
    }

    static constexpr int kPollIntervalMs = 500;
    static constexpr double kPixelsPerMs = 10.0 / 1000.0;

    QColor m_color;
    ScaleMode m_scaleMode;
    bool m_smooth = true;
    QVector<Metric> m_metrics;
    double m_latestValue = 0.0;
    double m_fixedMaximum = 0.0;
    double m_currentMaximum = 0.0;
    bool m_hasValue = false;
    QTimer m_pollTimer;
    QTimer m_animationTimer;
    QElapsedTimer m_monotonicClock;
};

namespace {

constexpr int kCoreColumns = 4;
const QColor kCpuTotalColor(QStringLiteral("#52c41a"));
const QColor kCpuCoreColor(QStringLiteral("#95de64"));
const QColor kMemoryColor(QStringLiteral("#722ed1"));
const QColor kFpsColor(QStringLiteral("#fa8c16"));

QLabel *makeTextLabel(const QString &text,
                      int size,
                      QFont::Weight weight,
                      const QString &color)
{
    auto *label = new QLabel(text);
    label->setFont(ui::appFont(size, weight));
    label->setStyleSheet(QStringLiteral("color:%1;").arg(color));
    return label;
}

QWidget *makeMetricSection(const QString &title,
                           QLabel **titleLabel,
                           QLabel **valueLabel,
                           PerformanceGraph *graph,
                           int graphHeight)
{
    auto *section = new QWidget;
    auto *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    auto *titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    *titleLabel = makeTextLabel(title, 11, QFont::Normal, QStringLiteral("#111827"));
    *valueLabel = makeTextLabel(QStringLiteral("--"),
                               11,
                               QFont::Normal,
                               kCpuTotalColor.name());
    titleRow->addWidget(*titleLabel);
    titleRow->addStretch();
    titleRow->addWidget(*valueLabel);
    layout->addLayout(titleRow);

    graph->setFixedHeight(graphHeight);
    layout->addWidget(graph);
    return section;
}

QString uptimeText(double totalSeconds)
{
    const qint64 seconds = std::max<qint64>(0, qFloor(totalSeconds));
    const qint64 days = seconds / 86400;
    const qint64 hours = (seconds / 3600) % 24;
    const qint64 minutes = (seconds / 60) % 60;
    const qint64 remainder = seconds % 60;
    return QStringLiteral("已开机 %1:%2:%3:%4")
        .arg(days)
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(remainder, 2, 10, QLatin1Char('0'));
}

} // namespace

PerformancePage::PerformancePage(QWidget *parent)
    : QWidget(parent)
    , m_uptimeTimer(this)
{
    setObjectName("PerformancePage");
    setMinimumWidth(0);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
    setStyleSheet(QStringLiteral(
        "QWidget#PerformancePage, QWidget#PerformanceContent { background:#f7f8fa; }"
        "QScrollArea#PerformanceScroll { background:#f7f8fa; border:none; }"
        "QScrollArea#PerformanceScroll QWidget#qt_scrollarea_viewport { background:#f7f8fa; }"));

    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    m_scrollArea = new QScrollArea;
    m_scrollArea->setObjectName("PerformanceScroll");
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setMinimumWidth(0);
    m_scrollArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->viewport()->installEventFilter(this);
    pageLayout->addWidget(m_scrollArea);

    m_scrollContent = new QWidget;
    m_scrollContent->setObjectName("PerformanceContent");
    m_scrollContent->setMinimumWidth(0);
    m_scrollContent->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    auto *layout = new QVBoxLayout(m_scrollContent);
    layout->setContentsMargins(14, 10, 14, 12);
    layout->setSpacing(10);

    auto *deviceRow = new QHBoxLayout;
    deviceRow->setContentsMargins(0, 0, 0, 0);
    deviceRow->setSpacing(10);
    m_uptimeLabel = makeTextLabel(ui::text("已开机 --:--:--:--"),
                                 10,
                                 QFont::Normal,
                                 QStringLiteral("#111827"));
    m_statusLabel = makeTextLabel(ui::text("等待设备连接"),
                                 9,
                                 QFont::Normal,
                                 QStringLiteral("#8a919c"));
    m_batteryInfoLabel = makeTextLabel(QString(),
                                      9,
                                      QFont::Normal,
                                      QStringLiteral("#596579"));
    m_batteryLabel = makeTextLabel(ui::text("电量 --%"),
                                  10,
                                  QFont::Normal,
                                  QStringLiteral("#111827"));
    deviceRow->addWidget(m_uptimeLabel);
    deviceRow->addWidget(m_statusLabel);
    deviceRow->addStretch();
    deviceRow->addWidget(m_batteryInfoLabel);
    deviceRow->addWidget(m_batteryLabel);
    layout->addLayout(deviceRow);

    m_cpuGraph = new PerformanceGraph(kCpuTotalColor,
                                      PerformanceGraph::ScaleMode::Percent,
                                      true);
    m_cpuGraph->setMaximum(100.0);
    layout->addWidget(makeMetricSection(ui::text("CPU"),
                                        &m_cpuTitleLabel,
                                        &m_cpuValueLabel,
                                        m_cpuGraph,
                                        80));

    auto *coreArea = new QWidget;
    m_coreGrid = new QGridLayout(coreArea);
    m_coreGrid->setContentsMargins(0, 0, 0, 0);
    m_coreGrid->setHorizontalSpacing(14);
    m_coreGrid->setVerticalSpacing(10);
    for (int column = 0; column < kCoreColumns; ++column) {
        m_coreGrid->setColumnStretch(column, 1);
    }
    layout->addWidget(coreArea);

    m_memoryGraph = new PerformanceGraph(kMemoryColor,
                                         PerformanceGraph::ScaleMode::Memory,
                                         false);
    layout->addWidget(makeMetricSection(ui::text("内存 --%"),
                                        &m_memoryTitleLabel,
                                        &m_memoryValueLabel,
                                        m_memoryGraph,
                                        80));
    m_memoryValueLabel->setStyleSheet(QStringLiteral("color:#722ed1;"));

    m_fpsGraph = new PerformanceGraph(kFpsColor,
                                      PerformanceGraph::ScaleMode::Frames,
                                      false);
    layout->addWidget(makeMetricSection(ui::text("FPS 系统桌面"),
                                        &m_fpsTitleLabel,
                                        &m_fpsValueLabel,
                                        m_fpsGraph,
                                        80));
    m_fpsValueLabel->setStyleSheet(QStringLiteral("color:#fa8c16;"));

    layout->addStretch();
    m_scrollArea->setWidget(m_scrollContent);
    rebuildCorePanels(8);

    m_uptimeTimer.setInterval(250);
    m_uptimeTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_uptimeTimer, &QTimer::timeout, this, &PerformancePage::updateUptime);
    m_uptimeTimer.start();
    QTimer::singleShot(0, this, &PerformancePage::syncScrollContent);
}

bool PerformancePage::eventFilter(QObject *watched, QEvent *event)
{
    if (m_scrollArea != nullptr && watched == m_scrollArea->viewport()
        && event->type() == QEvent::Resize) {
        QTimer::singleShot(0, this, &PerformancePage::syncScrollContent);
    }
    return QWidget::eventFilter(watched, event);
}

void PerformancePage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    QTimer::singleShot(0, this, &PerformancePage::syncScrollContent);
}

void PerformancePage::setDeviceConnected(bool connected, const QString &serial)
{
    m_connected = connected;
    m_statusLabel->setText(connected ? ui::text("正在采样 · %1").arg(serial)
                                     : ui::text("等待设备连接"));
    m_statusLabel->setStyleSheet(connected ? QStringLiteral("color:#4f8f3d;")
                                           : QStringLiteral("color:#8a919c;"));
    if (!connected) {
        m_uptimeElapsed.invalidate();
        m_uptimeLabel->setText(ui::text("已开机 --:--:--:--"));
        m_batteryInfoLabel->clear();
        m_batteryLabel->setText(ui::text("电量 --%"));
        m_cpuTitleLabel->setText(ui::text("CPU"));
        m_cpuValueLabel->setText(QStringLiteral("--"));
        m_memoryTitleLabel->setText(ui::text("内存 --%"));
        m_memoryValueLabel->setText(QStringLiteral("--MB"));
        m_fpsTitleLabel->setText(ui::text("FPS 系统桌面"));
        m_fpsValueLabel->setText(QStringLiteral("--"));
        clearHistory();
    }
}

void PerformancePage::applySample(const PerformanceSample &sample)
{
    if (!m_connected) {
        return;
    }

    m_statusLabel->setText(ui::text("实时采样 · %1")
                               .arg(QDateTime::fromMSecsSinceEpoch(sample.sampleTimeMs)
                                        .toString(QStringLiteral("HH:mm:ss"))));
    m_statusLabel->setStyleSheet(QStringLiteral("color:#4f8f3d;"));
    m_sampledUptimeSeconds = sample.uptimeSeconds;
    m_uptimeElapsed.restart();
    updateUptime();

    m_batteryLabel->setText(sample.batteryPercent >= 0
                                ? ui::text("电量 %1%").arg(sample.batteryPercent)
                                : ui::text("电量 --%"));
    m_batteryInfoLabel->setText(sample.batteryVoltageV > 0.0
                                    ? QStringLiteral("%1V  %2°C")
                                          .arg(sample.batteryVoltageV, 0, 'f', 2)
                                          .arg(sample.batteryTemperatureC, 0, 'f', 1)
                                    : QString());

    m_cpuTitleLabel->setText(sample.cpuTemperatureC > 0.0
                                 ? QStringLiteral("CPU %1°C").arg(sample.cpuTemperatureC, 0, 'f', 1)
                                 : QStringLiteral("CPU"));
    m_cpuValueLabel->setText(QStringLiteral("%1%").arg(qFloor(sample.cpuUsagePercent)));
    m_cpuGraph->setValue(qFloor(sample.cpuUsagePercent));

    if (m_corePanels.size() != sample.cores.size() && !sample.cores.isEmpty()) {
        rebuildCorePanels(sample.cores.size());
    }
    const int coreCount = std::min(static_cast<int>(m_corePanels.size()),
                                   static_cast<int>(sample.cores.size()));
    for (int index = 0; index < coreCount; ++index) {
        const CpuCoreSample &core = sample.cores[index];
        CorePanel &panel = m_corePanels[index];
        panel.title->setText(core.frequencyMhz > 0
                                 ? QStringLiteral("CPU%1 %2MHz").arg(core.index).arg(core.frequencyMhz)
                                 : QStringLiteral("CPU%1 --MHz").arg(core.index));
        panel.value->setText(QStringLiteral("%1%").arg(qFloor(core.usagePercent)));
        panel.graph->setValue(qFloor(core.usagePercent));
    }

    const int memoryPercent = sample.memoryTotalMb > 0
        ? qRound(100.0 * sample.memoryUsedMb / sample.memoryTotalMb)
        : 0;
    m_memoryTitleLabel->setText(ui::text("内存 %1%").arg(memoryPercent));
    m_memoryValueLabel->setText(QStringLiteral("%1MB").arg(sample.memoryUsedMb));
    m_memoryGraph->setValue(sample.memoryUsedMb);

    m_fpsTitleLabel->setText(ui::text("FPS %1").arg(sample.foregroundLabel));
    m_fpsValueLabel->setText(QString::number(qRound(sample.fps)));
    m_fpsGraph->setValue(qRound(sample.fps));
}

void PerformancePage::showSamplingError(const QString &message)
{
    if (!m_connected) {
        return;
    }
    m_statusLabel->setText(ui::text("采样失败：%1").arg(message.left(80)));
    m_statusLabel->setToolTip(message);
    m_statusLabel->setStyleSheet(QStringLiteral("color:#d45b5b;"));
}

void PerformancePage::rebuildCorePanels(int count)
{
    while (QLayoutItem *item = m_coreGrid->takeAt(0)) {
        if (item->widget() != nullptr) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    m_corePanels.clear();

    for (int index = 0; index < count; ++index) {
        CorePanel panel;
        panel.widget = new QWidget;
        panel.widget->setMinimumWidth(0);
        auto *layout = new QVBoxLayout(panel.widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        auto *titleRow = new QHBoxLayout;
        titleRow->setContentsMargins(0, 0, 0, 0);
        panel.title = makeTextLabel(QStringLiteral("CPU%1 --MHz").arg(index),
                                   10,
                                   QFont::Normal,
                                   QStringLiteral("#111827"));
        panel.value = makeTextLabel(QStringLiteral("0%"),
                                   10,
                                   QFont::Normal,
                                   kCpuCoreColor.name());
        titleRow->addWidget(panel.title);
        titleRow->addStretch();
        titleRow->addWidget(panel.value);
        layout->addLayout(titleRow);

        panel.graph = new PerformanceGraph(kCpuCoreColor,
                                           PerformanceGraph::ScaleMode::Percent,
                                           true);
        panel.graph->setMaximum(100.0);
        panel.graph->setFixedHeight(50);
        layout->addWidget(panel.graph);
        m_coreGrid->addWidget(panel.widget, index / kCoreColumns, index % kCoreColumns);
        m_corePanels.append(panel);
    }
    QTimer::singleShot(0, this, &PerformancePage::syncScrollContent);
}

void PerformancePage::clearHistory()
{
    m_cpuGraph->clear();
    m_memoryGraph->clear();
    m_fpsGraph->clear();
    for (int index = 0; index < m_corePanels.size(); ++index) {
        CorePanel &panel = m_corePanels[index];
        panel.title->setText(QStringLiteral("CPU%1 --MHz").arg(index));
        panel.value->setText(QStringLiteral("0%"));
        panel.graph->clear();
    }
}

void PerformancePage::syncScrollContent()
{
    if (m_scrollArea == nullptr || m_scrollContent == nullptr) {
        return;
    }
    const QSize viewportSize = m_scrollArea->viewport()->size();
    if (viewportSize.width() <= 0) {
        return;
    }
    m_scrollContent->setFixedWidth(viewportSize.width());
    const int horizontalMargins = 28;
    const int gridSpacing = m_coreGrid != nullptr
        ? m_coreGrid->horizontalSpacing() * (kCoreColumns - 1)
        : 0;
    const int panelWidth = std::max(120,
                                    (viewportSize.width() - horizontalMargins - gridSpacing)
                                        / kCoreColumns);
    for (CorePanel &panel : m_corePanels) {
        panel.widget->setFixedWidth(panelWidth);
    }
    if (m_scrollContent->layout() != nullptr) {
        m_scrollContent->layout()->activate();
    }
    m_scrollContent->resize(viewportSize.width(),
                            std::max(viewportSize.height(), m_scrollContent->sizeHint().height()));
}

void PerformancePage::updateUptime()
{
    if (!m_connected || !m_uptimeElapsed.isValid()) {
        return;
    }
    m_uptimeLabel->setText(uptimeText(m_sampledUptimeSeconds
                                      + m_uptimeElapsed.elapsed() / 1000.0));
}
