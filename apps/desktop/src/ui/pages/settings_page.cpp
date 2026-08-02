#include "ui/pages/settings_page.h"

#include "ui/common/app_preferences.h"
#include "ui/common/widget_helpers.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

QLabel *makeLabel(const QString &text,
                  int pointSize,
                  QFont::Weight weight,
                  const QString &objectName)
{
    auto *label = new QLabel(text);
    label->setObjectName(objectName);
    label->setFont(ui::appFont(pointSize, weight));
    label->setWordWrap(true);
    return label;
}

QFrame *makeSettingsCard()
{
    auto *card = new QFrame;
    card->setObjectName("SettingsCard");
    card->setFrameShape(QFrame::NoFrame);

    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(28.0);
    shadow->setOffset(0.0, 8.0);
    shadow->setColor(QColor(0, 0, 0, 24));
    card->setGraphicsEffect(shadow);
    return card;
}

QWidget *makeTextColumn(const QString &title, const QString &detail)
{
    auto *column = new QWidget;
    auto *layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(makeLabel(title, 12, QFont::DemiBold, "SettingsRowTitle"));
    layout->addWidget(makeLabel(detail, 10, QFont::Normal, "SettingsRowDetail"));
    return column;
}

} // namespace

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("SettingsPage");

    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    auto *scroll = new QScrollArea;
    scroll->setObjectName("SettingsScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *content = new QWidget;
    content->setObjectName("SettingsContent");
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(48, 38, 48, 48);
    contentLayout->setSpacing(18);

    contentLayout->addWidget(
        makeLabel(ui::text("外观与体验"), 22, QFont::DemiBold, "SettingsTitle"));
    contentLayout->addWidget(makeLabel(ui::text("个性化界面语言和动态效果"),
                                       11,
                                       QFont::Normal,
                                       "SettingsSubtitle"));
    contentLayout->addSpacing(6);

    auto *languageCard = makeSettingsCard();
    auto *languageLayout = new QHBoxLayout(languageCard);
    languageLayout->setContentsMargins(24, 20, 24, 20);
    languageLayout->setSpacing(20);
    languageLayout->addWidget(makeTextColumn(ui::text("语言"),
                                             ui::text("界面语言与字体会立即切换")),
                              1);

    auto *segments = new QFrame;
    segments->setObjectName("SettingsSegmentedControl");
    auto *segmentLayout = new QHBoxLayout(segments);
    segmentLayout->setContentsMargins(3, 3, 3, 3);
    segmentLayout->setSpacing(2);
    m_chineseButton = new QPushButton(ui::text("中文"));
    m_chineseButton->setObjectName("SettingsSegmentButton");
    m_chineseButton->setCheckable(true);
    m_englishButton = new QPushButton(QStringLiteral("English"));
    m_englishButton->setObjectName("SettingsSegmentButton");
    m_englishButton->setCheckable(true);
    segmentLayout->addWidget(m_chineseButton);
    segmentLayout->addWidget(m_englishButton);
    languageLayout->addWidget(segments);
    contentLayout->addWidget(languageCard);

    auto *fontCard = makeSettingsCard();
    auto *fontLayout = new QHBoxLayout(fontCard);
    fontLayout->setContentsMargins(24, 20, 24, 20);
    fontLayout->setSpacing(20);
    fontLayout->addWidget(makeTextColumn(ui::text("字体"),
                                         ui::text("跟随当前语言自动选择")),
                          1);
    m_fontValueLabel = makeLabel(QString(), 11, QFont::DemiBold, "SettingsValue");
    m_fontValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    fontLayout->addWidget(m_fontValueLabel);
    contentLayout->addWidget(fontCard);

    auto *motionCard = makeSettingsCard();
    auto *motionLayout = new QHBoxLayout(motionCard);
    motionLayout->setContentsMargins(24, 20, 24, 20);
    motionLayout->setSpacing(20);
    motionLayout->addWidget(makeTextColumn(ui::text("动态效果"),
                                           ui::text("页面过渡与按钮反馈")),
                            1);
    m_motionCheckBox = new QCheckBox(ui::text("启用自然动效"));
    m_motionCheckBox->setObjectName("SettingsSwitch");
    motionLayout->addWidget(m_motionCheckBox);
    contentLayout->addWidget(motionCard);
    contentLayout->addStretch();

    scroll->setWidget(content);
    pageLayout->addWidget(scroll);

    auto *languageGroup = new QButtonGroup(this);
    languageGroup->setExclusive(true);
    languageGroup->addButton(m_chineseButton);
    languageGroup->addButton(m_englishButton);

    connect(m_chineseButton, &QPushButton::clicked, this, [] {
        ui::AppPreferences::instance().setLanguage(ui::AppLanguage::Chinese);
    });
    connect(m_englishButton, &QPushButton::clicked, this, [] {
        ui::AppPreferences::instance().setLanguage(ui::AppLanguage::English);
    });
    connect(m_motionCheckBox, &QCheckBox::toggled, this, [](bool checked) {
        ui::AppPreferences::instance().setMotionEnabled(checked);
    });
    connect(&ui::AppPreferences::instance(),
            &ui::AppPreferences::languageChanged,
            this,
            [this] { refreshPreferences(); });
    connect(&ui::AppPreferences::instance(),
            &ui::AppPreferences::motionEnabledChanged,
            this,
            [this] { refreshPreferences(); });

    refreshPreferences();
}

void SettingsPage::refreshPreferences()
{
    const ui::AppPreferences &preferences = ui::AppPreferences::instance();
    const bool chinese = preferences.language() == ui::AppLanguage::Chinese;
    m_chineseButton->setChecked(chinese);
    m_englishButton->setChecked(!chinese);
    m_motionCheckBox->setChecked(preferences.motionEnabled());
    m_fontValueLabel->setText(preferences.fontFamily());
}
