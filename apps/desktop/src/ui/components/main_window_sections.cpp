#include "ui/components/main_window_sections.h"

#include "ui/common/widget_helpers.h"
#include "ui/widgets/brand_badges.h"

#include <QBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVector>

namespace ui {
namespace {

struct DeviceSelector {
    QFrame *widget = nullptr;
    QLabel *nameLabel = nullptr;
    QLabel *statusDot = nullptr;
    QLabel *statusLabel = nullptr;
};

struct SideStatus {
    QFrame *widget = nullptr;
    QLabel *statusDot = nullptr;
    QLabel *statusTitle = nullptr;
    QLabel *statusDetail = nullptr;
};

QLabel *makeLabel(const QString &value,
                  int size = 10,
                  QFont::Weight weight = QFont::Normal,
                  const QString &color = "#172033")
{
    auto *label = new QLabel(value);
    label->setFont(appFont(size, weight));
    label->setStyleSheet(QString("color:%1;").arg(color));
    return label;
}

QPushButton *makeIconButton(const QString &icon, const QString &tooltip, int size = 38)
{
    auto *button = new QPushButton(icon);
    button->setToolTip(tooltip);
    button->setFixedSize(size, size);
    button->setCursor(Qt::PointingHandCursor);
    button->setFont(appFont(14, QFont::DemiBold));
    button->setObjectName("IconButton");
    return button;
}

QFrame *makeDivider(bool vertical = true)
{
    auto *line = new QFrame;
    line->setObjectName("Divider");
    if (vertical) {
        line->setFixedWidth(1);
        line->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    } else {
        line->setFixedHeight(1);
        line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    return line;
}

QFrame *makeNavItem(const QString &icon, const QString &labelText, bool selected = false)
{
    auto *item = makePanel(selected ? "NavItemSelected" : "NavItem");
    item->setFixedHeight(34);
    auto *layout = new QHBoxLayout(item);
    layout->setContentsMargins(14, 0, 12, 0);
    layout->setSpacing(12);

    auto *iconLabel = makeLabel(icon, 15, QFont::DemiBold, selected ? "#2f6df6" : "#293243");
    iconLabel->setFixedWidth(22);
    iconLabel->setAlignment(Qt::AlignCenter);
    auto *textLabel = makeLabel(labelText, 11, selected ? QFont::DemiBold : QFont::Normal,
                                selected ? "#2f6df6" : "#293243");

    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);
    layout->addStretch();
    return item;
}

QPushButton *makeWorkspaceNavButton(const QString &icon,
                                    const QString &labelText,
                                    bool active)
{
    auto *button = new QPushButton(icon + QStringLiteral("   ") + labelText);
    button->setObjectName("WorkspaceNavButton");
    button->setProperty("active", active);
    button->setFixedHeight(34);
    button->setCursor(Qt::PointingHandCursor);
    button->setFont(appFont(11, active ? QFont::DemiBold : QFont::Normal));
    return button;
}

DeviceSelector makeDeviceSelector()
{
    DeviceSelector selector;
    selector.widget = makePanel("DeviceSelector");
    selector.widget->setFixedHeight(44);
    selector.widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *layout = new QHBoxLayout(selector.widget);
    layout->setContentsMargins(10, 0, 9, 0);
    layout->setSpacing(6);

    auto *deviceIcon = makeLabel(text("▯"), 14, QFont::DemiBold, "#1f2937");
    deviceIcon->setFixedWidth(18);
    deviceIcon->setAlignment(Qt::AlignCenter);
    layout->addWidget(deviceIcon);
    selector.nameLabel = makeLabel(text("正在检测设备"), 9, QFont::DemiBold, "#172033");
    selector.nameLabel->setMinimumWidth(0);
    selector.nameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    layout->addWidget(selector.nameLabel, 1);
    selector.statusDot = makeLabel(text("●"), 10, QFont::DemiBold, "#aab3c2");
    selector.statusLabel = makeLabel(text("检测中"), 8, QFont::Normal, "#596579");
    layout->addWidget(selector.statusDot);
    layout->addWidget(selector.statusLabel);
    layout->addWidget(makeLabel(text("⌄"), 10, QFont::Normal, "#7b8798"));
    return selector;
}

SideStatus makeSideStatus()
{
    SideStatus status;
    status.widget = makePanel("SideStatus");
    status.widget->setFixedHeight(56);
    auto *layout = new QHBoxLayout(status.widget);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(10);

    auto *texts = new QVBoxLayout;
    texts->setContentsMargins(0, 0, 0, 0);
    texts->setSpacing(4);
    status.statusTitle = makeLabel(text("正在检测 ADB"), 10, QFont::DemiBold, "#293243");
    status.statusDetail = makeLabel(text("等待设备状态"), 8, QFont::Normal, "#7b8798");
    texts->addWidget(status.statusTitle);
    texts->addWidget(status.statusDetail);

    status.statusDot = makeLabel(text("●"), 12, QFont::DemiBold, "#aab3c2");
    layout->addWidget(status.statusDot, 0, Qt::AlignTop);
    layout->addLayout(texts);
    return status;
}

QFrame *makeStepRow(const QString &index,
                    const QString &labelText,
                    const QString &state,
                    const QString &color)
{
    auto *row = makePanel("StepRow");
    row->setFixedHeight(36);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->setSpacing(10);

    auto *mark = makeLabel(index == text("1") ? text("✓") : (index == text("2") ? text("◌") : text("●")),
                           11, QFont::DemiBold, color);
    mark->setFixedWidth(22);
    mark->setAlignment(Qt::AlignCenter);

    layout->addWidget(mark);
    layout->addWidget(makeLabel(index + text(". ") + labelText, 10, QFont::Normal, color));
    layout->addStretch();
    layout->addWidget(makeLabel(state, 8, QFont::Normal, color == "#aab3c2" ? "#aab3c2" : "#7b8798"));
    return row;
}

QFrame *makeProgressBubble()
{
    auto *bubble = makePanel("AssistantBubbleLarge");
    auto *layout = new QVBoxLayout(bubble);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(4);

    layout->addWidget(makeLabel(text("好的，我将为你生成蓝牙连接测试脚本并开始执行。"),
                                10, QFont::Normal, "#293243"));
    layout->addWidget(makeStepRow(text("1"), text("检查蓝牙是否开启"), text("已完成"), "#59bd5a"));
    layout->addWidget(makeStepRow(text("2"), text("搜索可用设备"), text("执行中..."), "#3f7a3a"));
    layout->addWidget(makeStepRow(text("3"), text("连接目标设备"), text("等待中"), "#aab3c2"));
    layout->addWidget(makeStepRow(text("4"), text("验证连接状态"), text("等待中"), "#aab3c2"));
    layout->addWidget(makeStepRow(text("5"), text("断开连接"), text("等待中"), "#aab3c2"));
    return bubble;
}

QFrame *makeMessageBubble(const QString &message, const QString &time, bool user)
{
    auto *bubble = makePanel(user ? "UserBubble" : "AssistantBubble");
    bubble->setMaximumWidth(user ? 265 : 460);

    auto *layout = new QVBoxLayout(bubble);
    layout->setContentsMargins(16, 11, 16, 10);
    layout->setSpacing(3);
    auto *body = makeLabel(message, 10, user ? QFont::DemiBold : QFont::Normal,
                           user ? "#2f6df6" : "#293243");
    body->setWordWrap(true);
    auto *stamp = makeLabel(time, 8, QFont::Normal, user ? "#5d83d8" : "#8b96a8");
    stamp->setAlignment(Qt::AlignRight);
    layout->addWidget(body);
    layout->addWidget(stamp);
    return bubble;
}

QFrame *makeAttachmentChip(const QString &kind, const QString &name, const QString &size)
{
    auto *chip = makePanel("AttachmentChip");
    chip->setFixedSize(245, 62);
    auto *layout = new QHBoxLayout(chip);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(10);

    auto *fileIcon = makePanel(kind == text("xlsx") ? "ExcelIcon" : "PdfIcon");
    fileIcon->setFixedSize(34, 38);
    auto *fileIconLayout = new QVBoxLayout(fileIcon);
    fileIconLayout->setContentsMargins(0, 0, 0, 0);
    auto *type = makeLabel(kind == text("xlsx") ? text("X") : text("≡"), 14, QFont::DemiBold, "#ffffff");
    type->setAlignment(Qt::AlignCenter);
    fileIconLayout->addWidget(type);

    auto *texts = new QVBoxLayout;
    texts->setContentsMargins(0, 0, 0, 0);
    texts->setSpacing(3);
    texts->addWidget(makeLabel(name, 10, QFont::DemiBold, "#293243"));
    texts->addWidget(makeLabel(size, 8, QFont::Normal, "#8b96a8"));

    layout->addWidget(fileIcon);
    layout->addLayout(texts);
    layout->addStretch();
    layout->addWidget(makeLabel(text("×"), 15, QFont::Normal, "#8b96a8"));
    return chip;
}

QWidget *makeInputBox()
{
    auto *box = makePanel("InputBox");
    box->setFixedHeight(132);
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(18, 12, 18, 14);
    layout->setSpacing(6);

    auto *input = new QPlainTextEdit;
    input->setObjectName("PromptInput");
    input->setPlaceholderText(text("输入你的需求或问题，支持拖拽文件，回车发送，Shift+Enter 换行"));
    input->setFrameShape(QFrame::NoFrame);
    input->setFont(appFont(10));
    input->setFixedHeight(58);

    auto *bottom = new QHBoxLayout;
    bottom->setContentsMargins(0, 0, 0, 0);
    bottom->setSpacing(16);
    bottom->addWidget(makeIconButton(text("+"), text("添加附件"), 32));
    bottom->addWidget(makeIconButton(text("□"), text("选择文件"), 32));
    bottom->addWidget(makeIconButton(text("</>"), text("插入脚本上下文"), 38));
    bottom->addWidget(makeIconButton(text("@"), text("选择 Agent"), 32));
    bottom->addStretch();

    auto *send = new QPushButton(text("▶"));
    send->setObjectName("SendButton");
    send->setFixedSize(68, 38);
    send->setCursor(Qt::PointingHandCursor);
    send->setToolTip(text("发送"));
    send->setFont(appFont(14, QFont::DemiBold));
    bottom->addWidget(send);

    layout->addWidget(input);
    layout->addLayout(bottom);
    return box;
}

} // namespace

HeaderSection createHeader()
{
    HeaderSection section;
    section.widget = makePanel("Header");
    section.widget->setFixedHeight(68);
    auto *layout = new QHBoxLayout(section.widget);
    layout->setContentsMargins(28, 0, 28, 0);
    layout->setSpacing(14);

    layout->addWidget(new LogoBadge);
    layout->addWidget(makeLabel(text("AI Mobile Test Studio"),
                                13,
                                QFont::DemiBold,
                                "#1d1d1f"));
    layout->addStretch();
    layout->addWidget(makeIconButton(text("♢"), text("通知")));
    layout->addWidget(makeIconButton(text("▣"), text("设备中心")));
    layout->addWidget(makeDivider());
    section.settingsButton = makeIconButton(text("⚙"), text("设置"));
    layout->addWidget(section.settingsButton);
    layout->addWidget(makeDivider());
    layout->addWidget(new AvatarBadge);
    return section;
}

SidebarSection createSidebar()
{
    SidebarSection section;
    section.widget = makePanel("Sidebar");
    section.widget->setFixedWidth(252);
    auto *layout = new QVBoxLayout(section.widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *navigationScroll = new QScrollArea;
    navigationScroll->setObjectName("SidebarNavigationScroll");
    navigationScroll->setWidgetResizable(true);
    navigationScroll->setFrameShape(QFrame::NoFrame);
    navigationScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navigationScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    auto *navigation = new QWidget;
    navigation->setObjectName("SidebarNavigation");
    auto *navigationLayout = new QVBoxLayout(navigation);
    navigationLayout->setContentsMargins(22, 6, 22, 6);
    navigationLayout->setSpacing(1);
    navigationLayout->setAlignment(Qt::AlignTop);

    section.overviewButton = makeWorkspaceNavButton(text("◫"), text("概览"), true);
    section.displayButton = makeWorkspaceNavButton(text("↔"), text("显示"), false);
    section.mirroringButton = makeWorkspaceNavButton(text("▣"), text("镜像"), false);
    section.chatButton = makeWorkspaceNavButton(text("▣"), text("终端"), false);
    section.deviceControlButton = makeWorkspaceNavButton(text("⌘"), text("设备控制"), false);
    section.packageManagerButton = makeWorkspaceNavButton(text("▤"), text("软件包管理器"), false);
    section.appsButton = makeWorkspaceNavButton(text("▦"), text("应用"), false);
    section.filesButton = makeWorkspaceNavButton(text("▱"), text("文件"), false);
    section.processButton = makeWorkspaceNavButton(text("▥"), text("进程"), false);
    section.recoveryButton = makeWorkspaceNavButton(text("Ⓡ"), text("恢复"), false);
    section.performanceButton = makeWorkspaceNavButton(text("⌁"), text("性能"), false);
    section.layoutButton = makeWorkspaceNavButton(text("▦"), text("布局"), false);
    section.logcatButton = makeWorkspaceNavButton(text("≡"), text("日志"), false);
    section.otherButton = makeWorkspaceNavButton(text("</>"), text("其它"), false);
    navigationLayout->addWidget(section.overviewButton);
    navigationLayout->addWidget(section.displayButton);
    navigationLayout->addWidget(section.mirroringButton);
    navigationLayout->addWidget(section.chatButton);
    navigationLayout->addWidget(section.deviceControlButton);
    navigationLayout->addWidget(section.packageManagerButton);
    navigationLayout->addWidget(section.appsButton);
    navigationLayout->addWidget(section.filesButton);
    navigationLayout->addWidget(section.processButton);
    navigationLayout->addWidget(section.recoveryButton);
    navigationLayout->addWidget(section.performanceButton);
    navigationLayout->addWidget(section.layoutButton);
    navigationLayout->addWidget(section.logcatButton);
    navigationLayout->addWidget(section.otherButton);
    section.settingsButton = makeWorkspaceNavButton(text("⚙"), text("设置"), false);
    navigationLayout->addWidget(section.settingsButton);
    navigationLayout->addWidget(makeNavItem(text("i"), text("关于")));
    navigationLayout->addStretch();
    navigationScroll->setWidget(navigation);
    layout->addWidget(navigationScroll, 1);

    auto *footer = new QWidget;
    footer->setObjectName("SidebarFooter");
    auto *footerLayout = new QVBoxLayout(footer);
    footerLayout->setContentsMargins(22, 4, 22, 8);
    footerLayout->setSpacing(3);
    const SideStatus sideStatus = makeSideStatus();
    section.statusDot = sideStatus.statusDot;
    section.statusTitle = sideStatus.statusTitle;
    section.statusDetail = sideStatus.statusDetail;
    footerLayout->addWidget(sideStatus.widget);
    const DeviceSelector selector = makeDeviceSelector();
    section.deviceNameLabel = selector.nameLabel;
    section.deviceStatusDot = selector.statusDot;
    section.deviceStatusLabel = selector.statusLabel;
    footerLayout->addWidget(selector.widget);
    layout->addWidget(footer);
    return section;
}

QWidget *createChatPane()
{
    auto *pane = makePanel("ChatPane");
    auto *layout = new QVBoxLayout(pane);
    layout->setContentsMargins(26, 22, 26, 16);
    layout->setSpacing(14);

    auto *titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->setSpacing(12);
    titleRow->addWidget(new LogoBadge);
    titleRow->addWidget(makeLabel(text("欢迎使用 AI 助手"), 15, QFont::DemiBold, "#111827"));
    titleRow->addStretch();
    titleRow->addWidget(makeIconButton(text("↗"), text("展开"), 32));
    titleRow->addWidget(makeIconButton(text("⌫"), text("清空"), 32));
    titleRow->addWidget(makeIconButton(text("◷"), text("历史"), 32));
    layout->addLayout(titleRow);

    auto *scroll = new QScrollArea;
    scroll->setObjectName("ChatScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *messages = new QWidget;
    auto *messagesLayout = new QVBoxLayout(messages);
    messagesLayout->setContentsMargins(0, 4, 0, 4);
    messagesLayout->setSpacing(14);
    messagesLayout->addWidget(
        makeMessageBubble(text("你好！我是你的 AI 测试助手，有什么可以帮助你的吗？"), text("10:30"), false),
        0,
        Qt::AlignLeft);

    auto *userRow = new QHBoxLayout;
    userRow->addStretch();
    userRow->addWidget(makeMessageBubble(text("请帮我测试蓝牙连接功能"), text("10:31"), true));
    userRow->addWidget(new AvatarBadge);
    auto *userWrap = new QWidget;
    userWrap->setLayout(userRow);
    messagesLayout->addWidget(userWrap);
    messagesLayout->addWidget(makeProgressBubble(), 0, Qt::AlignLeft);

    auto *attachments = new QHBoxLayout;
    attachments->setContentsMargins(0, 0, 0, 0);
    attachments->setSpacing(12);
    attachments->addWidget(makeAttachmentChip(text("xlsx"), text("蓝牙测试用例.xlsx"), text("12.4 KB")));
    attachments->addWidget(makeAttachmentChip(text("pdf"), text("蓝牙测试需求文档.pdf"), text("256 KB")));
    attachments->addStretch();
    auto *attachWrap = new QWidget;
    attachWrap->setLayout(attachments);
    messagesLayout->addWidget(attachWrap);
    messagesLayout->addStretch();

    scroll->setWidget(messages);
    layout->addWidget(scroll, 1);
    layout->addWidget(makeInputBox());
    return pane;
}

} // namespace ui
