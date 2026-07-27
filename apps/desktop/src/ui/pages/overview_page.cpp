#include "ui/pages/overview_page.h"

#include "ui/common/widget_helpers.h"

#include <QApplication>
#include <QBoxLayout>
#include <QClipboard>
#include <QEvent>
#include <QFrame>
#include <QFontMetrics>
#include <QGridLayout>
#include <QLabel>
#include <QLocale>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QResizeEvent>
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
        + units[unit];
}

QString valueOrUnknown(const QString &value)
{
    return value.trimmed().isEmpty() ? ui::text("未知") : value.trimmed();
}

} // namespace

OverviewPage::OverviewPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("OverviewPage");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *toolbar = new QWidget;
    toolbar->setObjectName("OverviewToolbar");
    toolbar->setFixedHeight(48);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(22, 0, 18, 0);
    toolbarLayout->setSpacing(12);
    auto *title = makeLabel(ui::text("设备概览"), 11, QFont::DemiBold, QStringLiteral("#172033"));
    m_statusLabel = makeLabel(ui::text("等待设备连接"),
                              9,
                              QFont::Normal,
                              QStringLiteral("#7b8798"));
    m_refreshButton = new QToolButton;
    m_refreshButton->setObjectName("OverviewToolButton");
    m_refreshButton->setText(QStringLiteral("↻"));
    m_refreshButton->setToolTip(ui::text("刷新设备信息"));
    m_refreshButton->setFixedSize(34, 34);
    m_refreshButton->setFont(ui::appFont(15, QFont::DemiBold));
    m_refreshButton->setCursor(Qt::PointingHandCursor);
    m_refreshButton->setEnabled(false);
    connect(m_refreshButton, &QToolButton::clicked, this, &OverviewPage::refreshRequested);
    toolbarLayout->addWidget(title);
    toolbarLayout->addWidget(m_statusLabel);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(m_refreshButton);
    layout->addWidget(toolbar);

    auto *body = new QWidget;
    body->setObjectName("OverviewBody");
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(28, 24, 28, 24);
    bodyLayout->setSpacing(0);

    m_card = new QFrame;
    m_card->setObjectName("OverviewCard");
    m_card->setMinimumWidth(620);
    m_card->setMaximumWidth(1260);
    m_card->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    auto *grid = new QGridLayout(m_card);
    grid->setContentsMargins(26, 22, 26, 22);
    grid->setHorizontalSpacing(28);
    grid->setVerticalSpacing(14);
    for (int column = 0; column < 3; ++column) {
        grid->setColumnStretch(column, 1);
    }

    addInfoItem(grid, 0, 0, QStringLiteral("name"), QStringLiteral("▯"), ui::text("名称"));
    addInfoItem(grid, 0, 1, QStringLiteral("brand"), QStringLiteral("i"), ui::text("品牌"));
    addInfoItem(grid, 0, 2, QStringLiteral("model"), QStringLiteral("◇"), ui::text("型号"));
    addInfoItem(grid, 1, 0, QStringLiteral("serial"), QStringLiteral("№"), ui::text("序列号"));
    addInfoItem(grid, 1, 1, QStringLiteral("android"), QStringLiteral("⌁"), ui::text("Android 版本"));
    addInfoItem(grid, 1, 2, QStringLiteral("kernel"), QStringLiteral("⌁"), ui::text("内核版本"));
    addInfoItem(grid, 2, 0, QStringLiteral("processor"), QStringLiteral("▦"), ui::text("处理器"));
    addInfoItem(grid, 2, 1, QStringLiteral("storage"), QStringLiteral("▤"), ui::text("存储"));
    addInfoItem(grid, 2, 2, QStringLiteral("memory"), QStringLiteral("▥"), ui::text("内存"));
    addInfoItem(grid, 3, 0, QStringLiteral("physical"), QStringLiteral("▯"), ui::text("物理分辨率"));
    addInfoItem(grid, 3, 1, QStringLiteral("resolution"), QStringLiteral("▯"), ui::text("分辨率"));
    addInfoItem(grid, 3, 2, QStringLiteral("font"), QStringLiteral("A"), ui::text("字体缩放"));
    addInfoItem(grid, 4, 0, QStringLiteral("wifi"), QStringLiteral("⌁"), QStringLiteral("Wi-Fi"));
    addInfoItem(grid, 4, 1, QStringLiteral("ip"), QStringLiteral("◎"), ui::text("IP 地址"));
    addInfoItem(grid, 4, 2, QStringLiteral("mac"), QStringLiteral("◎"), ui::text("MAC 地址"));

    auto *cardRow = new QHBoxLayout;
    cardRow->setContentsMargins(0, 0, 0, 0);
    cardRow->addWidget(m_card, 0, Qt::AlignLeft | Qt::AlignTop);
    bodyLayout->addLayout(cardRow);

    m_emptyLabel = makeLabel(ui::text("连接 Android 设备后显示设备信息"),
                             10,
                             QFont::Normal,
                             QStringLiteral("#8b96a8"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    bodyLayout->addSpacing(12);
    bodyLayout->addWidget(m_emptyLabel);
    bodyLayout->addStretch();
    layout->addWidget(body, 1);
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
    m_refreshButton->setEnabled(connected);
    m_statusLabel->setText(connected ? ui::text("已连接 · %1").arg(serial)
                                     : ui::text("等待设备连接"));
    m_statusLabel->setStyleSheet(connected ? QStringLiteral("color:#4f8f3d;")
                                           : QStringLiteral("color:#7b8798;"));
    if (!connected || deviceChanged) {
        m_hasData = false;
        resetValues();
    }
    m_emptyLabel->setVisible(!connected);
}

void OverviewPage::setLoading(bool loading)
{
    m_refreshButton->setEnabled(m_connected && !loading);
    if (loading) {
        m_statusLabel->setText(ui::text("正在读取设备信息…"));
        m_statusLabel->setStyleSheet(QStringLiteral("color:#596579;"));
    } else if (m_connected) {
        m_statusLabel->setText(m_hasData ? ui::text("设备信息已更新")
                                         : ui::text("已连接 · %1").arg(m_serial));
        m_statusLabel->setStyleSheet(QStringLiteral("color:#4f8f3d;"));
    }
}

void OverviewPage::setOverview(const DeviceOverview &overview)
{
    m_hasData = true;
    setValue(QStringLiteral("name"), valueOrUnknown(overview.name));
    setValue(QStringLiteral("brand"), valueOrUnknown(overview.brand));
    setValue(QStringLiteral("model"), valueOrUnknown(overview.model));
    setValue(QStringLiteral("serial"), valueOrUnknown(overview.serialNumber));
    setValue(QStringLiteral("android"),
             overview.androidVersion.isEmpty()
                 ? ui::text("未知")
                 : QStringLiteral("Android %1 (API %2)")
                       .arg(overview.androidVersion,
                            overview.sdkVersion.isEmpty() ? QStringLiteral("--")
                                                          : overview.sdkVersion));
    setValue(QStringLiteral("kernel"), valueOrUnknown(overview.kernelVersion));
    setValue(QStringLiteral("processor"),
             ui::text("%1 %2 核 (%3)")
                 .arg(valueOrUnknown(overview.processor))
                 .arg(overview.cpuCount > 0 ? QString::number(overview.cpuCount)
                                            : QStringLiteral("--"))
                 .arg(valueOrUnknown(overview.abi)));
    setValue(QStringLiteral("storage"),
             overview.storageTotalBytes > 0
                 ? QStringLiteral("%1 / %2")
                       .arg(formatBytes(overview.storageUsedBytes),
                            formatBytes(overview.storageTotalBytes))
                 : QStringLiteral("--"));
    setValue(QStringLiteral("memory"), formatBytes(overview.memoryTotalBytes));
    setValue(QStringLiteral("physical"),
             overview.physicalResolution.isEmpty()
                 ? ui::text("未知")
                 : QStringLiteral("%1 (%2dpi)")
                       .arg(overview.physicalResolution,
                            overview.physicalDensity.isEmpty() ? QStringLiteral("--")
                                                               : overview.physicalDensity));
    setValue(QStringLiteral("resolution"),
             overview.resolution.isEmpty()
                 ? ui::text("未知")
                 : QStringLiteral("%1 (%2dpi)")
                       .arg(overview.resolution,
                            overview.density.isEmpty() ? QStringLiteral("--")
                                                       : overview.density));
    QString fontScale = QLocale::c().toString(overview.fontScale, 'f', 2);
    fontScale.remove(QRegularExpression(QStringLiteral("\\.?0+$")));
    setValue(QStringLiteral("font"), fontScale + QStringLiteral("x"));
    setValue(QStringLiteral("wifi"), valueOrUnknown(overview.wifi));
    setValue(QStringLiteral("ip"), valueOrUnknown(overview.ipAddress));
    setValue(QStringLiteral("mac"), valueOrUnknown(overview.macAddress));
    m_emptyLabel->setVisible(false);
    setLoading(false);
}

void OverviewPage::showError(const QString &message)
{
    if (!m_connected) {
        return;
    }
    m_statusLabel->setText(ui::text("读取失败：%1").arg(message.left(80)));
    m_statusLabel->setToolTip(message);
    m_statusLabel->setStyleSheet(QStringLiteral("color:#d45b5b;"));
}

bool OverviewPage::eventFilter(QObject *watched, QEvent *event)
{
    auto *label = qobject_cast<QLabel *>(watched);
    if (event->type() == QEvent::Resize && label != nullptr) {
        const QString value = label->property("copyValue").toString();
        if (!value.isEmpty()) {
            label->setText(QFontMetrics(label->font()).elidedText(
                value, Qt::ElideRight, label->width()));
        }
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        if (label != nullptr) {
            const QString value = label->property("copyValue").toString();
            if (!value.isEmpty() && value != QStringLiteral("--") && value != ui::text("未知")) {
                QApplication::clipboard()->setText(value);
                m_statusLabel->setText(ui::text("已复制：%1").arg(value.left(48)));
                m_statusLabel->setStyleSheet(QStringLiteral("color:#2f6df6;"));
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void OverviewPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_card != nullptr) {
        const int availableWidth = qRound(event->size().width()
                                          / std::max<qreal>(1.0, devicePixelRatioF()))
            - 56;
        m_card->setFixedWidth(std::clamp(availableWidth, 620, 1260));
    }
}

QLabel *OverviewPage::addInfoItem(QGridLayout *layout,
                                  int row,
                                  int column,
                                  const QString &key,
                                  const QString &icon,
                                  const QString &title)
{
    auto *item = new QWidget;
    item->setObjectName("OverviewInfoItem");
    item->setMinimumWidth(0);
    item->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    auto *itemLayout = new QVBoxLayout(item);
    itemLayout->setContentsMargins(0, 0, 0, 0);
    itemLayout->setSpacing(7);
    auto *titleLabel = makeLabel(icon + QStringLiteral("  ") + title,
                                 10,
                                 QFont::Normal,
                                 QStringLiteral("#596579"));
    titleLabel->setMinimumWidth(0);
    titleLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    auto *valueLabel = makeLabel(QStringLiteral("--"),
                                 12,
                                 QFont::Normal,
                                 QStringLiteral("#172033"));
    valueLabel->setMinimumWidth(0);
    valueLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    valueLabel->setObjectName("OverviewValue");
    valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    valueLabel->setCursor(Qt::PointingHandCursor);
    valueLabel->setToolTip(ui::text("单击复制"));
    valueLabel->installEventFilter(this);
    itemLayout->addWidget(titleLabel);
    itemLayout->addWidget(valueLabel);
    layout->addWidget(item, row, column);
    m_values.insert(key, valueLabel);
    return valueLabel;
}

void OverviewPage::resetValues()
{
    for (QLabel *label : std::as_const(m_values)) {
        label->setText(QStringLiteral("--"));
        label->setProperty("copyValue", QString());
        label->setToolTip(ui::text("单击复制"));
    }
}

void OverviewPage::setValue(const QString &key, const QString &value)
{
    QLabel *label = m_values.value(key);
    if (label == nullptr) {
        return;
    }
    label->setProperty("copyValue", value);
    label->setText(label->width() > 0
                       ? QFontMetrics(label->font()).elidedText(
                             value, Qt::ElideRight, label->width())
                       : value);
    label->setToolTip(value);
}
