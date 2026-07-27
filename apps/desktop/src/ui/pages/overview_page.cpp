#include "ui/pages/overview_page.h"

#include "ui/common/widget_helpers.h"

#include <QApplication>
#include <QBoxLayout>
#include <QClipboard>
#include <QEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLocale>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStackedLayout>
#include <QToolButton>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

QLabel *makeLabel(const QString &text,
                  int size,
                  QFont::Weight weight,
                  const QString &color)
{
    auto *label = new QLabel(text);
    label->setFont(ui::appFont(size, weight));
    label->setStyleSheet(QStringLiteral("color:%1;").arg(color));
    return label;
}

QString formatBytes(qint64 bytes)
{
    if (bytes <= 0) {
        return QStringLiteral("--");
    }
    static const QStringList units = {QStringLiteral("B"),
                                      QStringLiteral("KB"),
                                      QStringLiteral("MB"),
                                      QStringLiteral("GB"),
                                      QStringLiteral("TB")};
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < units.size() - 1) {
        value /= 1024.0;
        ++unit;
    }
    const int precision = value >= 100.0 ? 1 : 2;
    return QLocale::c().toString(value, 'f', precision)
               .remove(QRegularExpression(QStringLiteral("\\.?0+$")))
        + QLatin1Char(' ') + units[unit];
}

QString valueOrUnknown(const QString &value)
{
    return value.trimmed().isEmpty() ? ui::text("未知") : value.trimmed();
}

QString formatUptime(qint64 seconds)
{
    if (seconds <= 0) {
        return QStringLiteral("--");
    }
    const qint64 days = seconds / 86400;
    const qint64 hours = (seconds % 86400) / 3600;
    const qint64 minutes = (seconds % 3600) / 60;
    if (days > 0) {
        return ui::text("%1 天 %2 小时").arg(days).arg(hours);
    }
    return ui::text("%1 小时 %2 分钟").arg(hours).arg(minutes);
}

int percentage(qint64 used, qint64 total)
{
    if (total <= 0) {
        return 0;
    }
    return std::clamp(qRound(static_cast<double>(used) * 100.0
                             / static_cast<double>(total)),
                      0,
                      100);
}

QFrame *makeMetricCard(const QString &icon,
                       const QString &title,
                       QLabel **value,
                       QLabel **detail,
                       QProgressBar **progress)
{
    auto *card = ui::makePanel("OverviewMetricCard");
    card->setMinimumHeight(150);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(22, 20, 22, 18);
    layout->setSpacing(16);

    auto *iconLabel = makeLabel(icon, 24, QFont::DemiBold, QStringLiteral("#59418d"));
    iconLabel->setObjectName("OverviewMetricIcon");
    iconLabel->setFixedSize(50, 50);
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel, 0, Qt::AlignVCenter);

    auto *content = new QWidget;
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(5);
    contentLayout->addWidget(makeLabel(title, 9, QFont::Normal, QStringLiteral("#666174")));
    *value = makeLabel(QStringLiteral("--"), 19, QFont::DemiBold, QStringLiteral("#1f1d28"));
    contentLayout->addWidget(*value);
    *detail = makeLabel(QStringLiteral("--"), 8, QFont::Normal, QStringLiteral("#777182"));
    contentLayout->addWidget(*detail);
    *progress = new QProgressBar;
    (*progress)->setObjectName("OverviewMetricProgress");
    (*progress)->setRange(0, 100);
    (*progress)->setValue(0);
    (*progress)->setTextVisible(false);
    (*progress)->setFixedHeight(5);
    contentLayout->addWidget(*progress);
    layout->addWidget(content, 1);
    return card;
}

} // namespace

OverviewPage::OverviewPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("OverviewPage");
    auto *pageLayout = new QHBoxLayout(this);
    pageLayout->setContentsMargins(22, 18, 22, 20);
    pageLayout->setSpacing(18);

    auto *scroll = new QScrollArea;
    scroll->setObjectName("OverviewScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *leftContent = new QWidget;
    leftContent->setObjectName("OverviewBody");
    auto *leftLayout = new QVBoxLayout(leftContent);
    leftLayout->setContentsMargins(0, 0, 4, 0);
    leftLayout->setSpacing(16);

    auto *hero = ui::makePanel("OverviewHero");
    hero->setMinimumHeight(154);
    auto *heroLayout = new QHBoxLayout(hero);
    heroLayout->setContentsMargins(24, 20, 24, 20);
    heroLayout->setSpacing(14);

    auto *identity = new QWidget;
    auto *identityLayout = new QVBoxLayout(identity);
    identityLayout->setContentsMargins(0, 0, 0, 0);
    identityLayout->setSpacing(8);
    m_deviceName = makeLabel(ui::text("等待设备连接"),
                             20,
                             QFont::DemiBold,
                             QStringLiteral("#211f29"));
    m_deviceSubtitle = makeLabel(ui::text("连接 Android 设备后读取详细信息"),
                                 9,
                                 QFont::Normal,
                                 QStringLiteral("#696475"));
    m_deviceSubtitle->setWordWrap(true);
    m_shizukuButton = new QPushButton(ui::text("●  启动 Shizuku"));
    m_shizukuButton->setObjectName("OverviewShizukuButton");
    m_shizukuButton->setCursor(Qt::PointingHandCursor);
    m_shizukuButton->setFixedSize(156, 40);
    m_shizukuButton->setEnabled(false);
    identityLayout->addWidget(m_deviceName);
    identityLayout->addWidget(m_deviceSubtitle);
    identityLayout->addStretch();
    identityLayout->addWidget(m_shizukuButton, 0, Qt::AlignLeft);
    heroLayout->addWidget(identity, 1);

    auto *actions = new QVBoxLayout;
    actions->setContentsMargins(0, 0, 0, 0);
    actions->setSpacing(8);
    m_powerButton = new QToolButton;
    m_powerButton->setObjectName("OverviewPowerButton");
    m_powerButton->setText(QStringLiteral("⏻"));
    m_powerButton->setToolTip(ui::text("切换设备电源状态"));
    m_powerButton->setFixedSize(42, 42);
    m_powerButton->setCursor(Qt::PointingHandCursor);
    m_powerButton->setEnabled(false);
    m_refreshButton = new QToolButton;
    m_refreshButton->setObjectName("OverviewRefreshButton");
    m_refreshButton->setText(QStringLiteral("↻"));
    m_refreshButton->setToolTip(ui::text("刷新概览"));
    m_refreshButton->setFixedSize(42, 42);
    m_refreshButton->setCursor(Qt::PointingHandCursor);
    m_refreshButton->setEnabled(false);
    actions->addWidget(m_powerButton);
    actions->addWidget(m_refreshButton);
    actions->addStretch();
    heroLayout->addLayout(actions);
    leftLayout->addWidget(hero);

    auto *metrics = new QWidget;
    metrics->setObjectName("OverviewMetrics");
    auto *metricsLayout = new QHBoxLayout(metrics);
    metricsLayout->setContentsMargins(0, 0, 0, 0);
    metricsLayout->setSpacing(14);
    metricsLayout->addWidget(makeMetricCard(QStringLiteral("▣"),
                                            ui::text("电池"),
                                            &m_batteryValue,
                                            &m_batteryDetail,
                                            &m_batteryProgress));
    metricsLayout->addWidget(makeMetricCard(QStringLiteral("▦"),
                                            QStringLiteral("RAM"),
                                            &m_memoryValue,
                                            &m_memoryDetail,
                                            &m_memoryProgress));
    metricsLayout->addWidget(makeMetricCard(QStringLiteral("▤"),
                                            ui::text("存储"),
                                            &m_storageValue,
                                            &m_storageDetail,
                                            &m_storageProgress));
    leftLayout->addWidget(metrics);

    auto *facts = new QWidget;
    facts->setObjectName("OverviewFacts");
    auto *factsLayout = new QGridLayout(facts);
    factsLayout->setContentsMargins(0, 0, 0, 0);
    factsLayout->setHorizontalSpacing(14);
    factsLayout->setVerticalSpacing(14);
    for (int column = 0; column < 4; ++column) {
        factsLayout->setColumnStretch(column, 1);
    }
    addFactCard(factsLayout, 0, 0, QStringLiteral("android"), QStringLiteral("⌁"), ui::text("Android"));
    addFactCard(factsLayout, 0, 1, QStringLiteral("type"), QStringLiteral("▣"), ui::text("类型"));
    addFactCard(factsLayout, 0, 2, QStringLiteral("model"), QStringLiteral("▯"), ui::text("型号"));
    addFactCard(factsLayout, 0, 3, QStringLiteral("manufacturer"), QStringLiteral("▦"), ui::text("制造商"));
    addFactCard(factsLayout, 1, 0, QStringLiteral("brand"), QStringLiteral("◇"), ui::text("品牌"));
    addFactCard(factsLayout, 1, 1, QStringLiteral("abi"), QStringLiteral("▥"), ui::text("架构"));
    addFactCard(factsLayout, 1, 2, QStringLiteral("product"), QStringLiteral("▤"), ui::text("产品"));
    addFactCard(factsLayout, 1, 3, QStringLiteral("codename"), QStringLiteral("#"), ui::text("代号"));
    addFactCard(factsLayout, 2, 0, QStringLiteral("serial"), QStringLiteral("№"), ui::text("序列号"));
    addFactCard(factsLayout, 2, 1, QStringLiteral("uptime"), QStringLiteral("◷"), ui::text("运行时间"));
    addFactCard(factsLayout, 2, 2, QStringLiteral("display"), QStringLiteral("▭"), ui::text("分辨率"));
    addFactCard(factsLayout, 2, 3, QStringLiteral("kernel"), QStringLiteral("⌘"), ui::text("内核"));
    leftLayout->addWidget(facts);

    m_statusLabel = makeLabel(ui::text("等待设备连接"),
                              8,
                              QFont::Normal,
                              QStringLiteral("#777182"));
    m_statusLabel->setObjectName("OverviewStatus");
    leftLayout->addWidget(m_statusLabel);
    leftLayout->addStretch();
    scroll->setWidget(leftContent);
    pageLayout->addWidget(scroll, 1);

    auto *preview = ui::makePanel("OverviewPreviewPanel");
    preview->setMinimumWidth(260);
    preview->setMaximumWidth(380);
    auto *previewLayout = new QVBoxLayout(preview);
    previewLayout->setContentsMargins(14, 14, 14, 16);
    previewLayout->setSpacing(12);
    auto *previewHeader = new QHBoxLayout;
    previewHeader->addWidget(makeLabel(ui::text("实时屏幕"),
                                       10,
                                       QFont::DemiBold,
                                       QStringLiteral("#302b3a")));
    previewHeader->addStretch();
    auto *previewState = makeLabel(ui::text("ADB 截图"),
                                   8,
                                   QFont::Normal,
                                   QStringLiteral("#777182"));
    previewHeader->addWidget(previewState);
    previewLayout->addLayout(previewHeader);

    auto *phone = ui::makePanel("OverviewPhoneFrame");
    phone->setMinimumSize(220, 400);
    auto *phoneLayout = new QVBoxLayout(phone);
    phoneLayout->setContentsMargins(7, 7, 7, 7);
    phoneLayout->setSpacing(0);
    m_screenshotLabel = new QLabel;
    m_screenshotLabel->setObjectName("OverviewScreenshot");
    m_screenshotLabel->setAlignment(Qt::AlignCenter);
    m_screenshotLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_screenshotLabel->installEventFilter(this);
    m_screenshotPlaceholder = makeLabel(ui::text("连接设备后\n显示屏幕预览"),
                                        10,
                                        QFont::Normal,
                                        QStringLiteral("#928c9d"));
    m_screenshotPlaceholder->setAlignment(Qt::AlignCenter);
    auto *phoneStack = new QStackedLayout;
    phoneStack->setStackingMode(QStackedLayout::StackAll);
    phoneStack->addWidget(m_screenshotLabel);
    phoneStack->addWidget(m_screenshotPlaceholder);
    phoneLayout->addLayout(phoneStack);
    previewLayout->addWidget(phone, 1);

    m_screenshotButton = new QPushButton(ui::text("▣  截图"));
    m_screenshotButton->setObjectName("OverviewScreenshotButton");
    m_screenshotButton->setCursor(Qt::PointingHandCursor);
    m_screenshotButton->setFixedSize(128, 38);
    m_screenshotButton->setEnabled(false);
    previewLayout->addWidget(m_screenshotButton, 0, Qt::AlignHCenter);
    pageLayout->addWidget(preview);

    connect(m_refreshButton, &QToolButton::clicked, this, &OverviewPage::refreshRequested);
    connect(m_screenshotButton, &QPushButton::clicked, this, &OverviewPage::screenshotRequested);
    connect(m_shizukuButton, &QPushButton::clicked, this, &OverviewPage::shizukuRequested);
    connect(m_powerButton, &QToolButton::clicked, this, &OverviewPage::powerRequested);
    resetValues();
}

void OverviewPage::activate()
{
    if (m_connected) {
        emit refreshRequested();
    }
}

void OverviewPage::setDeviceConnected(bool connected, const QString &serial)
{
    const bool deviceChanged = m_serial != serial;
    m_connected = connected;
    m_serial = connected ? serial : QString();
    for (QWidget *control : {static_cast<QWidget *>(m_refreshButton),
                             static_cast<QWidget *>(m_powerButton),
                             static_cast<QWidget *>(m_shizukuButton),
                             static_cast<QWidget *>(m_screenshotButton)}) {
        control->setEnabled(connected);
    }
    if (!connected || deviceChanged) {
        m_hasData = false;
        m_screenshot = QPixmap();
        resetValues();
    }
    m_deviceName->setText(connected ? serial : ui::text("等待设备连接"));
    m_deviceSubtitle->setText(connected ? ui::text("正在读取设备信息…")
                                         : ui::text("连接 Android 设备后读取详细信息"));
    m_statusLabel->setText(connected ? ui::text("已连接 · %1").arg(serial)
                                     : ui::text("等待设备连接"));
    m_screenshotPlaceholder->setText(connected ? ui::text("正在获取屏幕截图…")
                                                : ui::text("连接设备后\n显示屏幕预览"));
    m_screenshotPlaceholder->setVisible(m_screenshot.isNull());
    updateScreenshotPixmap();
}

void OverviewPage::setLoading(bool loading)
{
    m_refreshButton->setEnabled(m_connected && !loading);
    if (loading) {
        m_statusLabel->setText(ui::text("正在读取设备信息…"));
        m_statusLabel->setStyleSheet(QStringLiteral("color:#696475;"));
    } else if (m_connected) {
        m_statusLabel->setText(m_hasData ? ui::text("设备信息已更新")
                                         : ui::text("已连接 · %1").arg(m_serial));
        m_statusLabel->setStyleSheet(QStringLiteral("color:#4f8f3d;"));
    }
}

void OverviewPage::setOverview(const DeviceOverview &overview)
{
    m_hasData = true;
    m_deviceName->setText(valueOrUnknown(overview.name));
    QStringList subtitle;
    const QString vendorModel = (overview.manufacturer + QLatin1Char(' ')
                                 + overview.model).trimmed();
    if (!vendorModel.isEmpty()) {
        subtitle.append(vendorModel);
    }
    if (!overview.processor.isEmpty()) {
        subtitle.append(overview.processor);
    }
    QString summary = subtitle.join(QStringLiteral(" · "));
    if (!overview.codename.isEmpty()) {
        summary += QStringLiteral(" (%1)").arg(overview.codename);
    }
    m_deviceSubtitle->setText(summary.isEmpty() ? m_serial : summary);

    m_batteryValue->setText(overview.batteryLevel >= 0
                                ? QStringLiteral("%1%").arg(overview.batteryLevel)
                                : QStringLiteral("--"));
    m_batteryDetail->setText(ui::text("健康状态：%1")
                                 .arg(valueOrUnknown(overview.batteryHealth)));
    m_batteryProgress->setValue(std::max(0, overview.batteryLevel));
    m_memoryValue->setText(formatBytes(overview.memoryUsedBytes));
    m_memoryDetail->setText(ui::text("总计：%1").arg(formatBytes(overview.memoryTotalBytes)));
    m_memoryProgress->setValue(percentage(overview.memoryUsedBytes,
                                          overview.memoryTotalBytes));
    m_storageValue->setText(formatBytes(overview.storageUsedBytes));
    m_storageDetail->setText(ui::text("总计：%1").arg(formatBytes(overview.storageTotalBytes)));
    m_storageProgress->setValue(percentage(overview.storageUsedBytes,
                                           overview.storageTotalBytes));

    setValue(QStringLiteral("android"),
             overview.androidVersion.isEmpty()
                 ? ui::text("未知")
                 : QStringLiteral("%1 (API %2)")
                       .arg(overview.androidVersion,
                            overview.sdkVersion.isEmpty() ? QStringLiteral("--")
                                                          : overview.sdkVersion));
    setValue(QStringLiteral("type"), valueOrUnknown(overview.deviceType));
    setValue(QStringLiteral("model"), valueOrUnknown(overview.model));
    setValue(QStringLiteral("manufacturer"), valueOrUnknown(overview.manufacturer));
    setValue(QStringLiteral("brand"), valueOrUnknown(overview.brand));
    setValue(QStringLiteral("abi"), valueOrUnknown(overview.abi));
    setValue(QStringLiteral("product"), valueOrUnknown(overview.product));
    setValue(QStringLiteral("codename"), valueOrUnknown(overview.codename));
    setValue(QStringLiteral("serial"), valueOrUnknown(overview.serialNumber));
    setValue(QStringLiteral("uptime"), formatUptime(overview.uptimeSeconds));
    setValue(QStringLiteral("display"),
             overview.resolution.isEmpty()
                 ? valueOrUnknown(overview.physicalResolution)
                 : overview.resolution);
    setValue(QStringLiteral("kernel"), valueOrUnknown(overview.kernelVersion));
    setLoading(false);
}

void OverviewPage::setScreenshotLoading(bool loading)
{
    m_screenshotButton->setEnabled(m_connected && !loading);
    if (loading && m_screenshot.isNull()) {
        m_screenshotPlaceholder->setText(ui::text("正在获取屏幕截图…"));
        m_screenshotPlaceholder->setVisible(true);
    }
}

void OverviewPage::setScreenshot(const QByteArray &pngData)
{
    QPixmap screenshot;
    if (!screenshot.loadFromData(pngData, "PNG")) {
        showError(ui::text("设备返回了无效的截图数据。"));
        return;
    }
    m_screenshot = screenshot;
    m_screenshotPlaceholder->setVisible(false);
    updateScreenshotPixmap();
}

void OverviewPage::showActionResult(bool success,
                                    const QString &label,
                                    const QString &detail)
{
    m_statusLabel->setStyleSheet(success ? QStringLiteral("color:#4f8f3d;")
                                          : QStringLiteral("color:#c84f55;"));
    m_statusLabel->setText(success ? ui::text("%1完成").arg(label)
                                   : ui::text("%1失败：%2").arg(label, detail.left(100)));
    m_statusLabel->setToolTip(detail);
    if (success && label.contains(QStringLiteral("电源"))) {
        emit screenshotRequested();
    }
}

void OverviewPage::showError(const QString &message)
{
    if (!m_connected) {
        return;
    }
    m_statusLabel->setText(ui::text("读取失败：%1").arg(message.left(100)));
    m_statusLabel->setToolTip(message);
    m_statusLabel->setStyleSheet(QStringLiteral("color:#c84f55;"));
}

bool OverviewPage::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_screenshotLabel && event->type() == QEvent::Resize) {
        updateScreenshotPixmap();
    }
    auto *label = qobject_cast<QLabel *>(watched);
    if (label != nullptr && event->type() == QEvent::MouseButtonRelease) {
        const QString value = label->property("copyValue").toString();
        if (!value.isEmpty() && value != QStringLiteral("--") && value != ui::text("未知")) {
            QApplication::clipboard()->setText(value);
            m_statusLabel->setText(ui::text("已复制：%1").arg(value.left(48)));
            m_statusLabel->setStyleSheet(QStringLiteral("color:#5b458f;"));
        }
    }
    return QWidget::eventFilter(watched, event);
}

void OverviewPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateScreenshotPixmap();
}

void OverviewPage::addFactCard(QGridLayout *layout,
                               int row,
                               int column,
                               const QString &key,
                               const QString &icon,
                               const QString &title)
{
    auto *card = ui::makePanel("OverviewFactCard");
    card->setMinimumSize(132, 152);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(18, 16, 18, 16);
    cardLayout->setSpacing(8);
    auto *iconLabel = makeLabel(icon, 19, QFont::DemiBold, QStringLiteral("#382653"));
    iconLabel->setObjectName("OverviewFactIcon");
    iconLabel->setFixedSize(46, 46);
    iconLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(iconLabel);
    cardLayout->addWidget(makeLabel(title, 9, QFont::DemiBold, QStringLiteral("#625d6e")));
    auto *value = makeLabel(QStringLiteral("--"),
                            13,
                            QFont::DemiBold,
                            QStringLiteral("#24212c"));
    value->setObjectName("OverviewFactValue");
    value->setMinimumWidth(0);
    value->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    value->setCursor(Qt::PointingHandCursor);
    value->installEventFilter(this);
    cardLayout->addWidget(value);
    cardLayout->addStretch();
    layout->addWidget(card, row, column);
    m_values.insert(key, value);
}

void OverviewPage::resetValues()
{
    for (QLabel *label : std::as_const(m_values)) {
        label->setText(QStringLiteral("--"));
        label->setProperty("copyValue", QString());
        label->setToolTip(QString());
    }
    for (QLabel *label : {m_batteryValue, m_memoryValue, m_storageValue}) {
        label->setText(QStringLiteral("--"));
    }
    for (QLabel *label : {m_batteryDetail, m_memoryDetail, m_storageDetail}) {
        label->setText(QStringLiteral("--"));
    }
    for (QProgressBar *progress : {m_batteryProgress, m_memoryProgress, m_storageProgress}) {
        progress->setValue(0);
    }
    m_screenshotLabel->clear();
}

void OverviewPage::setValue(const QString &key, const QString &value)
{
    QLabel *label = m_values.value(key);
    if (label == nullptr) {
        return;
    }
    label->setProperty("copyValue", value);
    label->setText(QFontMetrics(label->font()).elidedText(value,
                                                          Qt::ElideRight,
                                                          std::max(80, label->width())));
    label->setToolTip(value);
}

void OverviewPage::updateScreenshotPixmap()
{
    if (m_screenshot.isNull() || m_screenshotLabel->size().isEmpty()) {
        m_screenshotLabel->clear();
        return;
    }
    m_screenshotLabel->setPixmap(m_screenshot.scaled(m_screenshotLabel->size(),
                                                      Qt::KeepAspectRatio,
                                                      Qt::SmoothTransformation));
}
