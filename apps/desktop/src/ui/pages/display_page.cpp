#include "ui/pages/display_page.h"

#include "ui/common/widget_helpers.h"

#include <QAbstractButton>
#include <QBoxLayout>
#include <QButtonGroup>
#include <QFrame>
#include <QGridLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSlider>
#include <QStyle>
#include <QToolButton>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

QLabel *makeLabel(const QString &text,
                  int pointSize = 9,
                  QFont::Weight weight = QFont::Normal)
{
    auto *label = new QLabel(text);
    label->setFont(ui::appFont(pointSize, weight));
    return label;
}

QFrame *makeInfoCard(const QString &icon,
                     const QString &labelText,
                     QLabel **valueLabel)
{
    auto *card = new QFrame;
    card->setObjectName("DisplayInfoCard");
    card->setMinimumHeight(92);
    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(14);

    auto *iconLabel = makeLabel(icon, 15, QFont::DemiBold);
    iconLabel->setObjectName("DisplayInfoIcon");
    iconLabel->setFixedSize(44, 44);
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    auto *texts = new QVBoxLayout;
    texts->setContentsMargins(0, 2, 0, 2);
    texts->setSpacing(4);
    auto *title = makeLabel(labelText, 8, QFont::DemiBold);
    title->setObjectName("DisplayInfoLabel");
    *valueLabel = makeLabel(QStringLiteral("--"), 12, QFont::DemiBold);
    (*valueLabel)->setObjectName("DisplayInfoValue");
    texts->addWidget(title);
    texts->addWidget(*valueLabel);
    layout->addLayout(texts, 1);
    return card;
}

QFrame *makePanel(const QString &title, const char *objectName, QVBoxLayout **contentLayout)
{
    auto *panel = new QFrame;
    panel->setObjectName(objectName);
    panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(18, 16, 18, 18);
    layout->setSpacing(14);
    auto *heading = makeLabel(title, 11, QFont::DemiBold);
    heading->setObjectName("DisplayPanelTitle");
    layout->addWidget(heading);
    *contentLayout = layout;
    return panel;
}

QWidget *makeInputField(const QString &labelText,
                        QLineEdit **input,
                        const QString &suffix,
                        int minimum,
                        int maximum)
{
    auto *field = new QWidget;
    field->setObjectName("DisplayField");
    field->setMinimumWidth(0);
    auto *layout = new QVBoxLayout(field);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);
    auto *label = makeLabel(labelText, 8);
    label->setObjectName("DisplayFieldLabel");
    layout->addWidget(label);

    auto *inputFrame = new QFrame;
    inputFrame->setObjectName("DisplayInputFrame");
    inputFrame->setMinimumHeight(44);
    auto *inputLayout = new QHBoxLayout(inputFrame);
    inputLayout->setContentsMargins(12, 0, 12, 0);
    inputLayout->setSpacing(8);
    *input = new QLineEdit;
    (*input)->setObjectName("DisplayInput");
    (*input)->setFrame(false);
    (*input)->setValidator(new QIntValidator(minimum, maximum, *input));
    (*input)->setFont(ui::appFont(10));
    inputLayout->addWidget(*input, 1);
    auto *suffixLabel = makeLabel(suffix, 9);
    suffixLabel->setObjectName("DisplayInputSuffix");
    inputLayout->addWidget(suffixLabel);
    layout->addWidget(inputFrame);
    return field;
}

QPushButton *makeChip(const QString &text, const char *objectName)
{
    auto *button = new QPushButton(text);
    button->setObjectName(objectName);
    button->setCursor(Qt::PointingHandCursor);
    button->setFont(ui::appFont(9, QFont::DemiBold));
    button->setMinimumHeight(38);
    return button;
}

QFrame *makeSegment(QButtonGroup **group,
                    const QVector<QPair<QString, int>> &items,
                    int height,
                    QWidget *owner)
{
    auto *segment = new QFrame;
    segment->setObjectName("DisplaySegment");
    segment->setFixedHeight(height);
    auto *layout = new QHBoxLayout(segment);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    *group = new QButtonGroup(owner);
    (*group)->setExclusive(true);
    for (const auto &item : items) {
        auto *button = new QToolButton;
        button->setObjectName("DisplaySegmentButton");
        button->setText(item.first);
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        button->setFont(ui::appFont(9, QFont::DemiBold));
        (*group)->addButton(button, item.second);
        layout->addWidget(button);
    }
    return segment;
}

QString formatRate(double rate)
{
    const double rounded = std::round(rate);
    if (std::abs(rate - rounded) < 0.05) {
        return QString::number(static_cast<int>(rounded));
    }
    return QLocale::c().toString(rate, 'f', 2)
        .remove(QRegularExpression(QStringLiteral("\\.?0+$")));
}

} // namespace

DisplayPage::DisplayPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("DisplayPage");
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *toolbar = new QWidget;
    toolbar->setObjectName("DisplayToolbar");
    toolbar->setFixedHeight(48);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(22, 0, 18, 0);
    toolbarLayout->setSpacing(12);
    toolbarLayout->addWidget(makeLabel(QStringLiteral("Display"), 11, QFont::DemiBold));
    m_statusLabel = makeLabel(ui::text("等待设备连接"), 9);
    m_statusLabel->setObjectName("DisplayStatus");
    toolbarLayout->addWidget(m_statusLabel);
    toolbarLayout->addStretch();
    m_refreshButton = new QToolButton;
    m_refreshButton->setObjectName("DisplayToolButton");
    m_refreshButton->setText(QStringLiteral("↻"));
    m_refreshButton->setToolTip(ui::text("刷新显示设置"));
    m_refreshButton->setFixedSize(34, 34);
    m_refreshButton->setCursor(Qt::PointingHandCursor);
    m_refreshButton->setFont(ui::appFont(15, QFont::DemiBold));
    connect(m_refreshButton, &QToolButton::clicked, this, &DisplayPage::refreshRequested);
    toolbarLayout->addWidget(m_refreshButton);
    rootLayout->addWidget(toolbar);

    auto *scroll = new QScrollArea;
    scroll->setObjectName("DisplayScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *body = new QWidget;
    body->setObjectName("DisplayBody");
    body->setMinimumWidth(820);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(22, 18, 22, 22);
    bodyLayout->setSpacing(14);

    auto *infoLayout = new QHBoxLayout;
    infoLayout->setSpacing(12);
    infoLayout->addWidget(makeInfoCard(QStringLiteral("▣"),
                                       ui::text("物理分辨率"),
                                       &m_physicalResolutionValue));
    infoLayout->addWidget(makeInfoCard(QStringLiteral("≡"),
                                       ui::text("物理密度"),
                                       &m_physicalDensityValue));
    infoLayout->addWidget(makeInfoCard(QStringLiteral("↔"),
                                       ui::text("最小宽度"),
                                       &m_smallestWidthValue));
    bodyLayout->addLayout(infoLayout);

    auto *columns = new QHBoxLayout;
    columns->setSpacing(14);
    auto *leftColumn = new QVBoxLayout;
    leftColumn->setSpacing(14);
    auto *rightColumn = new QVBoxLayout;
    rightColumn->setSpacing(14);

    QVBoxLayout *dimensionsLayout = nullptr;
    m_dimensionsCard = makePanel(ui::text("尺寸"), "DisplayPanel", &dimensionsLayout);
    auto *suggestions = new QHBoxLayout;
    suggestions->setSpacing(8);
    for (int index = 0; index < 4; ++index) {
        auto *button = makeChip(QStringLiteral("--"), "DisplaySuggestionChip");
        button->setVisible(false);
        connect(button, &QPushButton::clicked, this, [this, button] {
            m_widthInput->setText(QString::number(button->property("widthValue").toInt()));
            m_heightInput->setText(QString::number(button->property("heightValue").toInt()));
            m_densityInput->setText(QString::number(button->property("densityValue").toInt()));
        });
        m_suggestionButtons.append(button);
        suggestions->addWidget(button);
    }
    suggestions->addStretch();
    dimensionsLayout->addLayout(suggestions);

    auto *sizeFields = new QHBoxLayout;
    sizeFields->setSpacing(10);
    sizeFields->addWidget(makeInputField(ui::text("宽度"),
                                         &m_widthInput,
                                         QStringLiteral("px"),
                                         320,
                                         10000));
    sizeFields->addWidget(makeInputField(ui::text("高度"),
                                         &m_heightInput,
                                         QStringLiteral("px"),
                                         320,
                                         10000));
    dimensionsLayout->addLayout(sizeFields);
    dimensionsLayout->addWidget(makeInputField(ui::text("密度"),
                                                &m_densityInput,
                                                QStringLiteral("dpi"),
                                                120,
                                                1000));
    dimensionsLayout->addWidget(makeInputField(ui::text("屏幕超时"),
                                                &m_timeoutInput,
                                                ui::text("秒"),
                                                1,
                                                86400));
    auto *dimensionActions = new QHBoxLayout;
    dimensionActions->addStretch();
    m_resetButton = new QPushButton(ui::text("重置"));
    m_resetButton->setObjectName("DisplayResetButton");
    m_resetButton->setCursor(Qt::PointingHandCursor);
    connect(m_resetButton, &QPushButton::clicked, this, &DisplayPage::resetRequested);
    dimensionActions->addWidget(m_resetButton);
    m_applyButton = new QPushButton(ui::text("应用"));
    m_applyButton->setObjectName("DisplayApplyButton");
    m_applyButton->setCursor(Qt::PointingHandCursor);
    connect(m_applyButton, &QPushButton::clicked, this, &DisplayPage::applyInputs);
    dimensionActions->addWidget(m_applyButton);
    dimensionsLayout->addLayout(dimensionActions);
    leftColumn->addWidget(m_dimensionsCard);
    leftColumn->addStretch();

    QVBoxLayout *refreshLayout = nullptr;
    m_refreshRateCard = makePanel(ui::text("可用刷新率"), "DisplayPanel", &refreshLayout);
    m_refreshRateLayout = new QHBoxLayout;
    m_refreshRateLayout->setSpacing(8);
    refreshLayout->addLayout(m_refreshRateLayout);
    rightColumn->addWidget(m_refreshRateCard);

    QVBoxLayout *appearanceLayout = nullptr;
    m_appearanceCard = makePanel(ui::text("外观"), "DisplayPanel", &appearanceLayout);
    appearanceLayout->addWidget(makeSegment(
        &m_themeGroup,
        {{ui::text("☼  浅色"), 0}, {ui::text("☾  深色"), 1}},
        58,
        this));
    connect(m_themeGroup, &QButtonGroup::idClicked, this, [this](int id) {
        emit darkModeRequested(id == 1);
    });
    auto *fontScaleHeader = new QHBoxLayout;
    auto *fontScaleTitle = makeLabel(ui::text("字体缩放"), 9);
    fontScaleTitle->setObjectName("DisplaySliderLabel");
    fontScaleHeader->addWidget(fontScaleTitle);
    fontScaleHeader->addStretch();
    m_fontScaleValue = makeLabel(QStringLiteral("1.00x"), 9, QFont::DemiBold);
    fontScaleHeader->addWidget(m_fontScaleValue);
    appearanceLayout->addLayout(fontScaleHeader);
    m_fontScaleSlider = new QSlider(Qt::Horizontal);
    m_fontScaleSlider->setObjectName("DisplaySlider");
    m_fontScaleSlider->setRange(50, 200);
    m_fontScaleSlider->setSingleStep(5);
    m_fontScaleSlider->setPageStep(10);
    m_fontScaleSlider->setValue(100);
    connect(m_fontScaleSlider, &QSlider::valueChanged, this, [this](int value) {
        m_fontScaleValue->setText(QStringLiteral("%1x").arg(value / 100.0, 0, 'f', 2));
    });
    connect(m_fontScaleSlider, &QSlider::sliderReleased, this, [this] {
        emit fontScaleRequested(m_fontScaleSlider->value() / 100.0);
    });
    appearanceLayout->addWidget(m_fontScaleSlider);
    rightColumn->addWidget(m_appearanceCard);

    QVBoxLayout *animationsLayout = nullptr;
    m_animationsCard = makePanel(ui::text("动画"), "DisplayPanel", &animationsLayout);
    animationsLayout->addWidget(makeSegment(
        &m_animationGroup,
        {{ui::text("关闭"), 0},
         {ui::text("快速"), 5},
         {ui::text("正常"), 10},
         {ui::text("慢速"), 15}},
        46,
        this));
    connect(m_animationGroup, &QButtonGroup::idClicked, this, [this](int id) {
        emit animationScaleRequested(id / 10.0);
    });
    rightColumn->addWidget(m_animationsCard);
    rightColumn->addStretch();

    columns->addLayout(leftColumn, 1);
    columns->addLayout(rightColumn, 1);
    bodyLayout->addLayout(columns);
    bodyLayout->addStretch();
    scroll->setWidget(body);
    rootLayout->addWidget(scroll, 1);

    setDeviceConnected(false, QString());
}

void DisplayPage::activate()
{
    if (m_connected && !m_busy) {
        emit refreshRequested();
    }
}

void DisplayPage::setDeviceConnected(bool connected, const QString &serial)
{
    const bool changed = m_serial != serial;
    m_connected = connected;
    m_serial = connected ? serial : QString();
    if (!connected || changed) {
        m_hasSettings = false;
        m_physicalResolutionValue->setText(QStringLiteral("--"));
        m_physicalDensityValue->setText(QStringLiteral("--"));
        m_smallestWidthValue->setText(QStringLiteral("--"));
        for (QLineEdit *input : {m_widthInput, m_heightInput, m_densityInput, m_timeoutInput}) {
            input->clear();
        }
        for (QPushButton *button : std::as_const(m_suggestionButtons)) {
            button->setVisible(false);
        }
        DisplaySettings emptySettings;
        updateRefreshRates(emptySettings);
    }
    setStatus(connected ? ui::text("已连接 · %1").arg(serial)
                        : ui::text("等待设备连接"),
              connected ? QStringLiteral("#4f8f3d") : QStringLiteral("#7b8798"));
    updateInteractiveState();
}

void DisplayPage::setBusy(bool busy)
{
    m_busy = busy;
    if (busy) {
        setStatus(ui::text("正在同步显示设置…"), QStringLiteral("#596579"));
    } else if (m_connected && m_hasSettings) {
        setStatus(ui::text("显示设置已同步"), QStringLiteral("#4f8f3d"));
    }
    updateInteractiveState();
}

void DisplayPage::setSettings(const DisplaySettings &settings)
{
    m_hasSettings = true;
    m_physicalResolutionValue->setText(
        QStringLiteral("%1 × %2").arg(settings.physicalWidth).arg(settings.physicalHeight));
    m_physicalDensityValue->setText(
        QStringLiteral("%1 dpi").arg(settings.physicalDensity));
    m_smallestWidthValue->setText(
        QStringLiteral("%1 dp").arg(settings.smallestWidthDp));
    m_widthInput->setText(QString::number(settings.currentWidth));
    m_heightInput->setText(QString::number(settings.currentHeight));
    m_densityInput->setText(QString::number(settings.currentDensity));
    m_timeoutInput->setText(QString::number(settings.screenTimeoutSeconds));
    updateSuggestions(settings);
    updateRefreshRates(settings);
    setSegmentSelection(m_themeGroup, settings.darkModeEnabled ? 1 : 0);
    const int fontValue = std::clamp(qRound(settings.fontScale * 100.0), 50, 200);
    m_fontScaleSlider->setValue(fontValue);
    const int animationId = qRound(settings.animationScale * 10.0);
    setSegmentSelection(m_animationGroup, animationId);
    setStatus(ui::text("显示设置已同步 · %1").arg(m_serial), QStringLiteral("#4f8f3d"));
    updateInteractiveState();
}

void DisplayPage::showError(const QString &message)
{
    setStatus(ui::text("读取失败：%1").arg(message.left(100)), QStringLiteral("#d45b5b"));
    m_statusLabel->setToolTip(message);
}

void DisplayPage::showOperationResult(bool success,
                                      const QString &label,
                                      const QString &detail)
{
    setStatus(success ? ui::text("%1已完成").arg(label)
                      : ui::text("%1失败：%2").arg(label, detail.left(80)),
              success ? QStringLiteral("#4f8f3d") : QStringLiteral("#d45b5b"));
    m_statusLabel->setToolTip(detail);
}

void DisplayPage::applyInputs()
{
    bool widthOk = false;
    bool heightOk = false;
    bool densityOk = false;
    bool timeoutOk = false;
    const int width = m_widthInput->text().toInt(&widthOk);
    const int height = m_heightInput->text().toInt(&heightOk);
    const int density = m_densityInput->text().toInt(&densityOk);
    const int timeout = m_timeoutInput->text().toInt(&timeoutOk);
    if (!widthOk || !heightOk || !densityOk || !timeoutOk || width < 320 || height < 320
        || density < 120 || timeout < 1) {
        showError(ui::text("请填写有效的宽度、高度、密度和屏幕超时。"));
        return;
    }
    emit applyRequested(width, height, density, timeout);
}

void DisplayPage::updateSuggestions(const DisplaySettings &settings)
{
    const QVector<double> scales = {0.9, 0.8, 0.7, 0.6};
    for (int index = 0; index < m_suggestionButtons.size(); ++index) {
        QPushButton *button = m_suggestionButtons[index];
        if (settings.physicalWidth <= 0 || settings.physicalHeight <= 0
            || settings.physicalDensity <= 0) {
            button->setVisible(false);
            continue;
        }
        const double scale = scales[index];
        const int width = qRound(settings.physicalWidth * scale / 8.0) * 8;
        const int height = qRound(settings.physicalHeight * scale / 8.0) * 8;
        const int density = std::max(120,
                                     qRound(settings.physicalDensity * scale / 8.0) * 8);
        button->setText(QStringLiteral("%1×%2 (%3dpi)")
                            .arg(width)
                            .arg(height)
                            .arg(density));
        button->setProperty("widthValue", width);
        button->setProperty("heightValue", height);
        button->setProperty("densityValue", density);
        button->setVisible(true);
    }
}

void DisplayPage::updateRefreshRates(const DisplaySettings &settings)
{
    while (QLayoutItem *item = m_refreshRateLayout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
    m_rateButtons.clear();
    if (settings.supportedRefreshRatesHz.isEmpty()) {
        auto *empty = makeLabel(ui::text("设备未报告可用刷新率"), 9);
        empty->setObjectName("DisplayRatesEmpty");
        m_refreshRateLayout->addWidget(empty);
        m_refreshRateLayout->addStretch();
        return;
    }
    for (double rate : settings.supportedRefreshRatesHz) {
        const bool selected = std::abs(rate - settings.refreshRateHz) < 0.6;
        auto *button = makeChip(
            QStringLiteral("%1  %2 Hz").arg(selected ? QStringLiteral("◉")
                                                     : QStringLiteral("○"),
                                               formatRate(rate)),
            "DisplayRateChip");
        button->setProperty("selected", selected);
        connect(button, &QPushButton::clicked, this, [this, rate] {
            emit refreshRateRequested(rate);
        });
        m_rateButtons.append(button);
        m_refreshRateLayout->addWidget(button);
    }
    m_refreshRateLayout->addStretch();
}

void DisplayPage::updateInteractiveState()
{
    const bool enabled = m_connected && !m_busy;
    m_refreshButton->setEnabled(enabled);
    m_dimensionsCard->setEnabled(enabled);
    m_refreshRateCard->setEnabled(enabled && m_hasSettings);
    m_appearanceCard->setEnabled(enabled && m_hasSettings);
    m_animationsCard->setEnabled(enabled && m_hasSettings);
    m_applyButton->setEnabled(enabled && m_hasSettings);
    m_resetButton->setEnabled(enabled && m_hasSettings);
}

void DisplayPage::setStatus(const QString &text, const QString &color)
{
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(QStringLiteral("color:%1;").arg(color));
}

void DisplayPage::setSegmentSelection(QButtonGroup *group, int id)
{
    for (QAbstractButton *button : group->buttons()) {
        const bool selected = group->id(button) == id;
        button->setChecked(selected);
        button->setProperty("selected", selected);
        button->style()->unpolish(button);
        button->style()->polish(button);
    }
}
