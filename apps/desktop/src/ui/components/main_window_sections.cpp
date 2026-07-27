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

QWidget *makeToolbarAction(const QString &icon, const QString &labelText)
{
    auto *widget = new QWidget;
    widget->setFixedWidth(72);
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 4, 0, 2);
    layout->setSpacing(2);

    auto *iconLabel = makeLabel(icon, 16, QFont::DemiBold, "#172033");
    iconLabel->setAlignment(Qt::AlignCenter);
    auto *textLabel = makeLabel(labelText, 8, QFont::Normal, "#172033");
    textLabel->setAlignment(Qt::AlignCenter);

    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);
    return widget;
}

DeviceSelector makeDeviceSelector()
{
    DeviceSelector selector;
    selector.widget = makePanel("DeviceSelector");
    selector.widget->setFixedSize(250, 44);
    auto *layout = new QHBoxLayout(selector.widget);
    layout->setContentsMargins(14, 0, 12, 0);
    layout->setSpacing(10);

    layout->addWidget(makeLabel(text("▯"), 17, QFont::DemiBold, "#1f2937"));
    selector.nameLabel = makeLabel(text("正在检测设备"), 10, QFont::DemiBold, "#172033");
    layout->addWidget(selector.nameLabel);
    layout->addStretch();
    selector.statusDot = makeLabel(text("●"), 13, QFont::DemiBold, "#aab3c2");
    selector.statusLabel = makeLabel(text("检测中"), 10, QFont::Normal, "#596579");
    layout->addWidget(selector.statusDot);
    layout->addWidget(selector.statusLabel);
    layout->addWidget(makeLabel(text("⌄"), 12, QFont::Normal, "#7b8798"));
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

QWidget *createHeader()
{
    auto *header = makePanel("Header");
    header->setFixedHeight(72);
    auto *layout = new QHBoxLayout(header);
    layout->setContentsMargins(28, 0, 28, 0);
    layout->setSpacing(14);

    layout->addWidget(new LogoBadge);
    layout->addWidget(makeLabel(text("AI 聊天窗口"), 13, QFont::DemiBold, "#111827"));
    layout->addStretch();
    layout->addWidget(makeIconButton(text("♢"), text("通知")));
    layout->addWidget(makeIconButton(text("▣"), text("设备中心")));
    layout->addWidget(makeDivider());
    layout->addWidget(makeIconButton(text("⚙"), text("设置")));
    layout->addWidget(makeDivider());
    layout->addWidget(new AvatarBadge);
    return header;
}

SidebarSection createSidebar()
{
    SidebarSection section;
    section.widget = makePanel("Sidebar");
    section.widget->setFixedWidth(252);
    auto *layout = new QVBoxLayout(section.widget);
    layout->setContentsMargins(22, 6, 22, 8);
    layout->setSpacing(2);
    layout->setAlignment(Qt::AlignTop);

    section.overviewButton = makeWorkspaceNavButton(text("◫"), text("概览"), true);
    section.chatButton = makeWorkspaceNavButton(text("▣"), text("终端"), false);
    section.deviceControlButton = makeWorkspaceNavButton(text("⌘"), text("设备控制"), false);
    section.packageManagerButton = makeWorkspaceNavButton(text("▤"), text("软件包管理器"), false);
    section.appsButton = makeWorkspaceNavButton(text("▦"), text("应用"), false);
    section.filesButton = makeWorkspaceNavButton(text("▱"), text("Files"), false);
    section.recoveryButton = makeWorkspaceNavButton(text("Ⓡ"), text("Recovery"), false);
    section.performanceButton = makeWorkspaceNavButton(text("⌁"), text("性能"), false);
    section.layoutButton = makeWorkspaceNavButton(text("▦"), text("布局"), false);
    section.logcatButton = makeWorkspaceNavButton(text("≡"), text("日志"), false);
    layout->addWidget(section.overviewButton);
    layout->addWidget(section.chatButton);
    layout->addWidget(section.deviceControlButton);
    layout->addWidget(section.packageManagerButton);
    layout->addWidget(section.appsButton);
    layout->addWidget(section.filesButton);
    layout->addWidget(section.recoveryButton);
    layout->addWidget(section.performanceButton);
    layout->addWidget(section.layoutButton);
    layout->addWidget(section.logcatButton);
    layout->addWidget(makeNavItem(text("⚙"), text("设置")));
    layout->addWidget(makeNavItem(text("i"), text("关于")));
    const SideStatus sideStatus = makeSideStatus();
    section.statusDot = sideStatus.statusDot;
    section.statusTitle = sideStatus.statusTitle;
    section.statusDetail = sideStatus.statusDetail;
    layout->addWidget(sideStatus.widget);
    return section;
}

ToolbarSection createToolbar()
{
    ToolbarSection section;
    section.widget = makePanel("Toolbar");
    section.widget->setFixedHeight(76);
    auto *layout = new QHBoxLayout(section.widget);
    layout->setContentsMargins(18, 14, 18, 14);
    layout->setSpacing(18);

    const DeviceSelector selector = makeDeviceSelector();
    section.deviceNameLabel = selector.nameLabel;
    section.deviceStatusDot = selector.statusDot;
    section.deviceStatusLabel = selector.statusLabel;
    layout->addWidget(selector.widget);

    section.mirrorButton = new QPushButton(text("▶  启动镜像"));
    section.mirrorButton->setObjectName("MirrorButton");
    section.mirrorButton->setFixedSize(118, 44);
    section.mirrorButton->setCursor(Qt::PointingHandCursor);
    section.mirrorButton->setToolTip(text("启动 scrcpy 手机镜像"));
    section.mirrorButton->setFont(appFont(10, QFont::DemiBold));
    section.mirrorButton->setEnabled(false);
    layout->addWidget(section.mirrorButton);

    layout->addWidget(makeDivider());
    layout->addWidget(makeToolbarAction(text("↻"), text("刷新")));
    layout->addWidget(makeToolbarAction(text("▣"), text("截图")));
    layout->addWidget(makeToolbarAction(text("▰"), text("录屏")));
    layout->addWidget(makeToolbarAction(text("…"), text("更多")));
    layout->addWidget(makeDivider());
    layout->addStretch();
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
