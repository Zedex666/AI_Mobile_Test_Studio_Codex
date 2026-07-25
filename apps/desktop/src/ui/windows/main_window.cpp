#include "mainwindow.h"

#include <QApplication>
#include <QBoxLayout>
#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>

namespace {

QString t(const char *text)
{
    return QString::fromUtf8(text);
}

QFont appFont(int pointSize, QFont::Weight weight = QFont::Normal)
{
    QFont font(t("Microsoft YaHei UI"));
    font.setPointSize(pointSize);
    font.setWeight(weight);
    return font;
}

class LogoBadge : public QWidget
{
public:
    explicit LogoBadge(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(34, 34);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QLinearGradient gradient(rect().topLeft(), rect().bottomRight());
        gradient.setColorAt(0.0, QColor("#8bb7ff"));
        gradient.setColorAt(1.0, QColor("#2f6df6"));
        p.setBrush(gradient);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect().adjusted(2, 2, -2, -2), 8, 8);

        p.setPen(QPen(QColor("#ffffff"), 2.0));
        p.drawArc(QRectF(9, 9, 16, 16), 25 * 16, 310 * 16);
        p.drawEllipse(QPointF(13, 13), 2.2, 2.2);
        p.drawEllipse(QPointF(22, 14), 2.2, 2.2);
        p.drawEllipse(QPointF(17, 23), 2.2, 2.2);
        p.drawLine(QPointF(14, 18), QPointF(20, 18));
    }
};

class AvatarBadge : public QWidget
{
public:
    explicit AvatarBadge(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(42, 42);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor("#eef3fb"));
        p.setPen(QPen(QColor("#d6e0ef"), 1));
        p.drawEllipse(rect().adjusted(1, 1, -1, -1));

        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#1f2937"));
        p.drawEllipse(QPointF(width() / 2.0, 15), 7, 7);
        p.setBrush(QColor("#2d5fec"));
        p.drawRoundedRect(QRectF(10, 25, 22, 12), 6, 6);
        p.setBrush(QColor("#f4c7a1"));
        p.drawEllipse(QPointF(width() / 2.0, 16), 5, 6);
    }
};

class PhonePreview : public QWidget
{
public:
    explicit PhonePreview(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumSize(300, 610);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), Qt::transparent);

        const int availableHeight = height() - 20;
        int phoneHeight = qMin(availableHeight, int(width() / 0.49));
        int phoneWidth = int(phoneHeight * 0.49);
        if (phoneWidth > width() - 24) {
            phoneWidth = width() - 24;
            phoneHeight = int(phoneWidth / 0.49);
        }

        QRectF shell((width() - phoneWidth) / 2.0,
                     (height() - phoneHeight) / 2.0,
                     phoneWidth,
                     phoneHeight);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(15, 23, 42, 45));
        p.drawRoundedRect(shell.translated(0, 5), 36, 36);

        QLinearGradient shellGradient(shell.topLeft(), shell.bottomRight());
        shellGradient.setColorAt(0.0, QColor("#2f3540"));
        shellGradient.setColorAt(0.45, QColor("#0b1018"));
        shellGradient.setColorAt(1.0, QColor("#697386"));
        p.setBrush(shellGradient);
        p.drawRoundedRect(shell, 34, 34);

        QRectF bezel = shell.adjusted(8, 8, -8, -8);
        p.setBrush(QColor("#0f172a"));
        p.drawRoundedRect(bezel, 29, 29);

        QRectF screen = bezel.adjusted(7, 7, -7, -7);
        QPainterPath clip;
        clip.addRoundedRect(screen, 24, 24);
        p.save();
        p.setClipPath(clip);
        p.fillRect(screen, QColor("#05070b"));
        drawWallpaper(&p, screen);
        drawPhoneStatus(&p, screen);
        drawPhoneWidgets(&p, screen);
        drawPhoneDock(&p, screen);
        p.restore();

        p.setPen(QPen(QColor("#b7c3d5"), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(shell.adjusted(3, 3, -3, -3), 32, 32);
    }

private:
    static QPointF mapPoint(const QRectF &r, qreal x, qreal y)
    {
        return QPointF(r.left() + r.width() * x, r.top() + r.height() * y);
    }

    void drawWallpaper(QPainter *p, const QRectF &screen)
    {
        const QVector<QColor> colors = {
            QColor("#121821"), QColor("#242d3a"), QColor("#344052"),
            QColor("#161d27"), QColor("#445166"), QColor("#101721")
        };

        const QVector<QVector<QPointF>> rocks = {
            {mapPoint(screen, .12, .18), mapPoint(screen, .54, .08), mapPoint(screen, .42, .44), mapPoint(screen, .05, .50)},
            {mapPoint(screen, .48, .08), mapPoint(screen, .91, .22), mapPoint(screen, .76, .59), mapPoint(screen, .42, .43)},
            {mapPoint(screen, .09, .50), mapPoint(screen, .47, .43), mapPoint(screen, .40, .90), mapPoint(screen, .03, .78)},
            {mapPoint(screen, .43, .43), mapPoint(screen, .89, .60), mapPoint(screen, .76, .92), mapPoint(screen, .38, .90)},
            {mapPoint(screen, .15, .22), mapPoint(screen, .36, .14), mapPoint(screen, .28, .45), mapPoint(screen, .08, .39)},
            {mapPoint(screen, .59, .30), mapPoint(screen, .91, .27), mapPoint(screen, .84, .68), mapPoint(screen, .55, .55)}
        };

        for (int i = 0; i < rocks.size(); ++i) {
            QPainterPath path;
            path.moveTo(rocks[i].first());
            for (int j = 1; j < rocks[i].size(); ++j) {
                path.lineTo(rocks[i][j]);
            }
            path.closeSubpath();
            p->setPen(QPen(QColor(255, 255, 255, 18), 1));
            p->setBrush(colors[i % colors.size()]);
            p->drawPath(path);
        }

        QLinearGradient glow(screen.topLeft(), screen.bottomRight());
        glow.setColorAt(0.0, QColor(60, 90, 130, 35));
        glow.setColorAt(1.0, QColor(0, 0, 0, 140));
        p->fillRect(screen, glow);
    }

    void drawPhoneStatus(QPainter *p, const QRectF &screen)
    {
        p->setPen(QColor("#ffffff"));
        p->setFont(appFont(9, QFont::DemiBold));
        p->drawText(QRectF(screen.left() + 25, screen.top() + 19, 70, 20), t("9:30"));
        p->drawText(QRectF(screen.right() - 76, screen.top() + 19, 70, 20), Qt::AlignRight, t("5G"));
        p->setBrush(QColor("#ffffff"));
        p->setPen(Qt::NoPen);
        p->drawRoundedRect(QRectF(screen.right() - 35, screen.top() + 21, 17, 9), 2, 2);
        p->drawRect(QRectF(screen.right() - 17, screen.top() + 24, 2, 4));
    }

    void drawPhoneWidgets(QPainter *p, const QRectF &screen)
    {
        p->setPen(QColor("#ffffff"));
        p->setFont(appFont(10, QFont::Normal));
        p->drawText(QRectF(screen.left() + 28, screen.top() + 74, screen.width() - 56, 24),
                    t("Coffee with Robin in 30 min"));
        p->setFont(appFont(8));
        p->drawText(QRectF(screen.left() + 28, screen.top() + 96, screen.width() - 56, 22),
                    t("10:00 - 11:00 AM     68°F"));

        QRectF weather(screen.left() + 28, screen.top() + screen.height() * .48, 92, 92);
        p->setBrush(QColor(30, 38, 57, 220));
        p->setPen(Qt::NoPen);
        p->drawEllipse(weather);
        p->setBrush(QColor("#ffd35b"));
        p->drawEllipse(QRectF(weather.left() + 18, weather.bottom() - 36, 42, 42));
        p->setPen(QColor("#d9e6ff"));
        p->setFont(appFont(27, QFont::DemiBold));
        p->drawText(weather.adjusted(12, 10, -4, -6), Qt::AlignCenter, t("68°"));

        QRectF agent(screen.right() - 118, screen.top() + 132, 92, 92);
        p->setBrush(QColor(44, 53, 76, 230));
        p->setPen(Qt::NoPen);
        p->drawEllipse(agent);
        p->setPen(QPen(QColor("#b7c8ff"), 6, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(QPointF(agent.center().x(), agent.top() + 28),
                    QPointF(agent.center().x(), agent.bottom() - 28));
        p->setBrush(QColor("#a9c4ff"));
        p->setPen(Qt::NoPen);
        p->drawEllipse(QPointF(agent.left() + 31, agent.bottom() - 31), 5, 5);
    }

    void drawPhoneDock(QPainter *p, const QRectF &screen)
    {
        const qreal bottom = screen.bottom() - 43;
        const qreal gap = screen.width() / 5.7;
        const qreal start = screen.left() + gap;
        const QStringList icons = {t("☎"), t("●"), t("G"), t("□")};

        p->setFont(appFont(15, QFont::DemiBold));
        for (int i = 0; i < icons.size(); ++i) {
            QPointF center(start + gap * i, bottom - 67);
            p->setPen(Qt::NoPen);
            p->setBrush(QColor("#c8d8ff"));
            p->drawEllipse(center, 21, 21);
            p->setPen(QColor("#1f2937"));
            p->drawText(QRectF(center.x() - 18, center.y() - 17, 36, 34), Qt::AlignCenter, icons[i]);
        }

        QRectF search(screen.left() + 25, screen.bottom() - 82, screen.width() - 50, 42);
        p->setBrush(QColor(28, 34, 44, 225));
        p->setPen(Qt::NoPen);
        p->drawRoundedRect(search, 18, 18);
        p->setPen(QColor("#eef5ff"));
        p->setFont(appFont(17, QFont::DemiBold));
        p->drawText(QRectF(search.left() + 16, search.top() + 3, 38, 36), Qt::AlignCenter, t("G"));
        p->drawText(QRectF(search.right() - 62, search.top() + 3, 42, 36), Qt::AlignCenter, t("⌕"));

        p->setPen(QPen(QColor("#ffffff"), 3, Qt::SolidLine, Qt::RoundCap));
        p->drawLine(QPointF(screen.center().x() - 34, screen.bottom() - 18),
                    QPointF(screen.center().x() + 34, screen.bottom() - 18));
    }
};

QLabel *makeLabel(const QString &text,
                  int size = 10,
                  QFont::Weight weight = QFont::Normal,
                  const QString &color = "#172033")
{
    auto *label = new QLabel(text);
    label->setFont(appFont(size, weight));
    label->setStyleSheet(QString("color:%1;").arg(color));
    return label;
}

QFrame *makePanel(const QString &objectName = QString())
{
    auto *panel = new QFrame;
    panel->setObjectName(objectName.isEmpty() ? "Panel" : objectName);
    panel->setFrameShape(QFrame::NoFrame);
    return panel;
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

QFrame *makeNavItem(const QString &icon, const QString &text, bool selected = false)
{
    auto *item = makePanel(selected ? "NavItemSelected" : "NavItem");
    item->setFixedHeight(46);
    auto *layout = new QHBoxLayout(item);
    layout->setContentsMargins(14, 0, 12, 0);
    layout->setSpacing(12);

    auto *iconLabel = makeLabel(icon, 15, QFont::DemiBold, selected ? "#2f6df6" : "#293243");
    iconLabel->setFixedWidth(22);
    iconLabel->setAlignment(Qt::AlignCenter);
    auto *textLabel = makeLabel(text, 11, selected ? QFont::DemiBold : QFont::Normal,
                                selected ? "#2f6df6" : "#293243");

    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);
    layout->addStretch();
    return item;
}

QWidget *makeToolbarAction(const QString &icon, const QString &text)
{
    auto *widget = new QWidget;
    widget->setFixedWidth(72);
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 4, 0, 2);
    layout->setSpacing(2);

    auto *iconLabel = makeLabel(icon, 16, QFont::DemiBold, "#172033");
    iconLabel->setAlignment(Qt::AlignCenter);
    auto *textLabel = makeLabel(text, 8, QFont::Normal, "#172033");
    textLabel->setAlignment(Qt::AlignCenter);

    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);
    return widget;
}

QFrame *makeDeviceSelector()
{
    auto *selector = makePanel("DeviceSelector");
    selector->setFixedSize(250, 44);
    auto *layout = new QHBoxLayout(selector);
    layout->setContentsMargins(14, 0, 12, 0);
    layout->setSpacing(10);

    auto *phone = makeLabel(t("▯"), 17, QFont::DemiBold, "#1f2937");
    auto *name = makeLabel(t("Pixel 8 Pro"), 11, QFont::DemiBold, "#172033");
    auto *dot = makeLabel(t("●"), 13, QFont::DemiBold, "#79d464");
    auto *status = makeLabel(t("已连接"), 10, QFont::Normal, "#293243");
    auto *arrow = makeLabel(t("⌄"), 12, QFont::Normal, "#7b8798");

    layout->addWidget(phone);
    layout->addWidget(name);
    layout->addStretch();
    layout->addWidget(dot);
    layout->addWidget(status);
    layout->addWidget(arrow);
    return selector;
}

QFrame *makeSideStatus()
{
    auto *status = makePanel("SideStatus");
    status->setFixedHeight(72);
    auto *layout = new QHBoxLayout(status);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(10);

    auto *dot = makeLabel(t("●"), 12, QFont::DemiBold, "#66d45e");
    auto *texts = new QVBoxLayout;
    texts->setContentsMargins(0, 0, 0, 0);
    texts->setSpacing(4);
    texts->addWidget(makeLabel(t("ADB 连接正常"), 10, QFont::DemiBold, "#293243"));
    texts->addWidget(makeLabel(t("Android 14  |  剩余电量 87%"), 8, QFont::Normal, "#7b8798"));

    layout->addWidget(dot, 0, Qt::AlignTop);
    layout->addLayout(texts);
    return status;
}

QWidget *makeHeader()
{
    auto *header = makePanel("Header");
    header->setFixedHeight(72);
    auto *layout = new QHBoxLayout(header);
    layout->setContentsMargins(28, 0, 28, 0);
    layout->setSpacing(14);

    layout->addWidget(new LogoBadge);
    layout->addWidget(makeLabel(t("AI 聊天窗口"), 13, QFont::DemiBold, "#111827"));
    layout->addStretch();
    layout->addWidget(makeIconButton(t("♢"), t("通知")));
    layout->addWidget(makeIconButton(t("▣"), t("设备中心")));
    layout->addWidget(makeDivider());
    layout->addWidget(makeIconButton(t("⚙"), t("设置")));
    layout->addWidget(makeDivider());
    layout->addWidget(new AvatarBadge);
    return header;
}

QWidget *makeSidebar()
{
    auto *sidebar = makePanel("Sidebar");
    sidebar->setFixedWidth(252);
    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(26, 20, 26, 26);
    layout->setSpacing(12);

    layout->addWidget(makeNavItem(t("▣"), t("对话"), true));
    layout->addWidget(makeNavItem(t("☰"), t("对话列表")));

    const QStringList conversations = {t("对话1"), t("对话2")};
    for (const auto &conversation : conversations) {
        auto *label = makeLabel(conversation, 10, QFont::Normal, "#293243");
        label->setContentsMargins(48, 10, 0, 6);
        layout->addWidget(label);
    }

    auto *newChat = makePanel("PlainRow");
    auto *newChatLayout = new QHBoxLayout(newChat);
    newChatLayout->setContentsMargins(48, 6, 12, 6);
    newChatLayout->addWidget(makeLabel(t("新对话"), 10, QFont::Normal, "#293243"));
    newChatLayout->addStretch();
    newChatLayout->addWidget(makeLabel(t("+"), 16, QFont::DemiBold, "#293243"));
    layout->addWidget(newChat);

    layout->addSpacing(8);
    layout->addWidget(makeNavItem(t("▤"), t("对案")));
    layout->addWidget(makeNavItem(t("◇"), t("插件")));
    layout->addWidget(makeNavItem(t("⌘"), t("技能")));
    layout->addStretch();
    layout->addWidget(makeNavItem(t("⚙"), t("设置")));
    layout->addWidget(makeNavItem(t("i"), t("关于")));
    layout->addWidget(makeSideStatus());
    return sidebar;
}

QWidget *makeToolbar()
{
    auto *toolbar = makePanel("Toolbar");
    toolbar->setFixedHeight(76);
    auto *layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(18, 14, 18, 14);
    layout->setSpacing(18);

    layout->addWidget(makeDeviceSelector());
    layout->addWidget(makeDivider());
    layout->addWidget(makeToolbarAction(t("↻"), t("刷新")));
    layout->addWidget(makeToolbarAction(t("▣"), t("截图")));
    layout->addWidget(makeToolbarAction(t("▰"), t("录屏")));
    layout->addWidget(makeToolbarAction(t("…"), t("更多")));
    layout->addWidget(makeDivider());
    layout->addStretch();
    return toolbar;
}

QWidget *makePhoneControls()
{
    auto *controls = makePanel("PhoneControls");
    controls->setFixedWidth(72);
    auto *layout = new QVBoxLayout(controls);
    layout->setContentsMargins(0, 28, 0, 28);
    layout->setSpacing(14);

    const QVector<QPair<QString, QString>> actions = {
        {t("←"), t("返回")}, {t("⌂"), t("Home")}, {t("▱"), t("多任务")},
        {t("↻"), t("旋转")}, {t("▰+"), t("音量+")}, {t("▰-"), t("音量-")},
        {t("…"), t("更多")}
    };

    for (const auto &action : actions) {
        auto *item = new QWidget;
        auto *itemLayout = new QVBoxLayout(item);
        itemLayout->setContentsMargins(0, 0, 0, 0);
        itemLayout->setSpacing(2);
        auto *icon = makeLabel(action.first, 16, QFont::DemiBold, "#293243");
        icon->setAlignment(Qt::AlignCenter);
        auto *text = makeLabel(action.second, 8, QFont::Normal, "#293243");
        text->setAlignment(Qt::AlignCenter);
        itemLayout->addWidget(icon);
        itemLayout->addWidget(text);
        layout->addWidget(item);
    }

    layout->addStretch();
    return controls;
}

QWidget *makeDevicePane()
{
    auto *pane = makePanel("DevicePane");
    pane->setMinimumWidth(455);
    pane->setMaximumWidth(520);
    auto *layout = new QHBoxLayout(pane);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(14);

    auto *phoneHost = makePanel("PhoneHost");
    auto *phoneLayout = new QHBoxLayout(phoneHost);
    phoneLayout->setContentsMargins(12, 8, 12, 8);
    phoneLayout->addWidget(new PhonePreview);

    layout->addWidget(phoneHost, 1);
    layout->addWidget(makePhoneControls());
    return pane;
}

QFrame *makeStepRow(const QString &index, const QString &text, const QString &state, const QString &color)
{
    auto *row = makePanel("StepRow");
    row->setFixedHeight(36);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->setSpacing(10);

    auto *mark = makeLabel(index == t("1") ? t("✓") : (index == t("2") ? t("◌") : t("●")),
                           11, QFont::DemiBold, color);
    mark->setFixedWidth(22);
    mark->setAlignment(Qt::AlignCenter);

    layout->addWidget(mark);
    layout->addWidget(makeLabel(index + t(". ") + text, 10, QFont::Normal, color));
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

    layout->addWidget(makeLabel(t("好的，我将为你生成蓝牙连接测试脚本并开始执行。"),
                                10, QFont::Normal, "#293243"));
    layout->addWidget(makeStepRow(t("1"), t("检查蓝牙是否开启"), t("已完成"), "#59bd5a"));
    layout->addWidget(makeStepRow(t("2"), t("搜索可用设备"), t("执行中..."), "#3f7a3a"));
    layout->addWidget(makeStepRow(t("3"), t("连接目标设备"), t("等待中"), "#aab3c2"));
    layout->addWidget(makeStepRow(t("4"), t("验证连接状态"), t("等待中"), "#aab3c2"));
    layout->addWidget(makeStepRow(t("5"), t("断开连接"), t("等待中"), "#aab3c2"));
    return bubble;
}

QFrame *makeMessageBubble(const QString &text, const QString &time, bool user)
{
    auto *bubble = makePanel(user ? "UserBubble" : "AssistantBubble");
    bubble->setMaximumWidth(user ? 265 : 460);

    auto *layout = new QVBoxLayout(bubble);
    layout->setContentsMargins(16, 11, 16, 10);
    layout->setSpacing(3);
    auto *body = makeLabel(text, 10, user ? QFont::DemiBold : QFont::Normal,
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

    auto *fileIcon = makePanel(kind == t("xlsx") ? "ExcelIcon" : "PdfIcon");
    fileIcon->setFixedSize(34, 38);
    auto *fileIconLayout = new QVBoxLayout(fileIcon);
    fileIconLayout->setContentsMargins(0, 0, 0, 0);
    auto *type = makeLabel(kind == t("xlsx") ? t("X") : t("≡"), 14, QFont::DemiBold, "#ffffff");
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
    layout->addWidget(makeLabel(t("×"), 15, QFont::Normal, "#8b96a8"));
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
    input->setPlaceholderText(t("输入你的需求或问题，支持拖拽文件，回车发送，Shift+Enter 换行"));
    input->setFrameShape(QFrame::NoFrame);
    input->setFont(appFont(10));
    input->setFixedHeight(58);

    auto *bottom = new QHBoxLayout;
    bottom->setContentsMargins(0, 0, 0, 0);
    bottom->setSpacing(16);
    bottom->addWidget(makeIconButton(t("+"), t("添加附件"), 32));
    bottom->addWidget(makeIconButton(t("□"), t("选择文件"), 32));
    bottom->addWidget(makeIconButton(t("</>"), t("插入脚本上下文"), 38));
    bottom->addWidget(makeIconButton(t("@"), t("选择 Agent"), 32));
    bottom->addStretch();

    auto *send = new QPushButton(t("▶"));
    send->setObjectName("SendButton");
    send->setFixedSize(68, 38);
    send->setCursor(Qt::PointingHandCursor);
    send->setToolTip(t("发送"));
    send->setFont(appFont(14, QFont::DemiBold));
    bottom->addWidget(send);

    layout->addWidget(input);
    layout->addLayout(bottom);
    return box;
}

QWidget *makeChatPane()
{
    auto *pane = makePanel("ChatPane");
    auto *layout = new QVBoxLayout(pane);
    layout->setContentsMargins(26, 22, 26, 16);
    layout->setSpacing(14);

    auto *titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->setSpacing(12);
    auto *smallLogo = new LogoBadge;
    smallLogo->setFixedSize(34, 34);
    titleRow->addWidget(smallLogo);
    titleRow->addWidget(makeLabel(t("欢迎使用 AI 助手"), 15, QFont::DemiBold, "#111827"));
    titleRow->addStretch();
    titleRow->addWidget(makeIconButton(t("↗"), t("展开"), 32));
    titleRow->addWidget(makeIconButton(t("⌫"), t("清空"), 32));
    titleRow->addWidget(makeIconButton(t("◷"), t("历史"), 32));
    layout->addLayout(titleRow);

    auto *scroll = new QScrollArea;
    scroll->setObjectName("ChatScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *messages = new QWidget;
    auto *messagesLayout = new QVBoxLayout(messages);
    messagesLayout->setContentsMargins(0, 4, 0, 4);
    messagesLayout->setSpacing(14);

    messagesLayout->addWidget(makeMessageBubble(t("你好！我是你的 AI 测试助手，有什么可以帮助你的吗？"), t("10:30"), false),
                              0, Qt::AlignLeft);

    auto *userRow = new QHBoxLayout;
    userRow->addStretch();
    userRow->addWidget(makeMessageBubble(t("请帮我测试蓝牙连接功能"), t("10:31"), true));
    userRow->addWidget(new AvatarBadge);
    auto *userWrap = new QWidget;
    userWrap->setLayout(userRow);
    messagesLayout->addWidget(userWrap);

    messagesLayout->addWidget(makeProgressBubble(), 0, Qt::AlignLeft);

    auto *attachments = new QHBoxLayout;
    attachments->setContentsMargins(0, 0, 0, 0);
    attachments->setSpacing(12);
    attachments->addWidget(makeAttachmentChip(t("xlsx"), t("蓝牙测试用例.xlsx"), t("12.4 KB")));
    attachments->addWidget(makeAttachmentChip(t("pdf"), t("蓝牙测试需求文档.pdf"), t("256 KB")));
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

QWidget *makeFooter()
{
    auto *footer = makePanel("Footer");
    footer->setFixedHeight(70);
    auto *layout = new QHBoxLayout(footer);
    layout->setContentsMargins(30, 0, 30, 0);
    layout->setSpacing(20);

    layout->addWidget(makeNavItem(t("▤"), t("日志")));
    layout->addWidget(makeNavItem(t("⌬"), t("终端")));
    layout->addWidget(makeNavItem(t("▣"), t("运行结果"), true));
    layout->addStretch();
    layout->addWidget(makeLabel(t("●"), 12, QFont::DemiBold, "#2f6df6"));
    layout->addWidget(makeLabel(t("已选中 1 个设备"), 10, QFont::DemiBold, "#293243"));
    layout->addWidget(makeLabel(t("⌄"), 12, QFont::Normal, "#7b8798"));
    return footer;
}

QString globalStyle()
{
    return t(R"(
        QWidget {
            font-family: "Microsoft YaHei UI", "Segoe UI", sans-serif;
            color: #172033;
            background: transparent;
        }
        QMainWindow {
            background: #f7faff;
        }
        QFrame#Root {
            background: #f7faff;
        }
        QFrame#Header {
            background: #fbfdff;
            border-bottom: 1px solid #dfe7f3;
        }
        QFrame#Sidebar {
            background: #fbfdff;
            border-right: 1px solid #dfe7f3;
        }
        QFrame#Toolbar {
            background: #ffffff;
            border-bottom: 1px solid #dfe7f3;
        }
        QFrame#Footer {
            background: #fbfdff;
            border-top: 1px solid #dfe7f3;
        }
        QFrame#Panel, QFrame#DevicePane, QFrame#ChatPane, QFrame#PhoneHost {
            background: #ffffff;
            border: 1px solid #dfe7f3;
            border-radius: 8px;
        }
        QFrame#DevicePane, QFrame#ChatPane {
            background: #ffffff;
        }
        QFrame#PhoneHost {
            background: #fbfdff;
        }
        QFrame#PhoneControls {
            background: #ffffff;
            border: none;
        }
        QFrame#DeviceSelector {
            background: #ffffff;
            border: 1px solid #d9e4f4;
            border-radius: 8px;
        }
        QFrame#NavItem, QFrame#PlainRow {
            background: transparent;
            border: none;
            border-radius: 8px;
        }
        QFrame#NavItemSelected {
            background: #eaf2ff;
            border: none;
            border-radius: 8px;
        }
        QFrame#SideStatus {
            background: #f6f9ff;
            border: 1px solid #dfe7f3;
            border-radius: 8px;
        }
        QFrame#Divider {
            background: #dfe7f3;
            border: none;
        }
        QPushButton#IconButton {
            background: transparent;
            border: none;
            border-radius: 8px;
            color: #172033;
        }
        QPushButton#IconButton:hover {
            background: #eef5ff;
        }
        QFrame#AssistantBubble, QFrame#AssistantBubbleLarge {
            background: #f2f5fa;
            border: 1px solid #edf2f8;
            border-radius: 8px;
        }
        QFrame#UserBubble {
            background: #dceaff;
            border: 1px solid #c9dcfb;
            border-radius: 8px;
        }
        QFrame#StepRow {
            background: rgba(255, 255, 255, 150);
            border: none;
            border-radius: 6px;
        }
        QFrame#AttachmentChip {
            background: #ffffff;
            border: 1px solid #dbe5f4;
            border-radius: 8px;
        }
        QFrame#ExcelIcon {
            background: #40b95f;
            border-radius: 6px;
        }
        QFrame#PdfIcon {
            background: #4b83f6;
            border-radius: 6px;
        }
        QFrame#InputBox {
            background: #ffffff;
            border: 1px solid #c7d8f1;
            border-radius: 8px;
        }
        QPlainTextEdit#PromptInput {
            background: #ffffff;
            border: none;
            color: #293243;
            selection-background-color: #dceaff;
        }
        QPushButton#SendButton {
            background: #2f6df6;
            border: none;
            border-radius: 8px;
            color: white;
        }
        QPushButton#SendButton:hover {
            background: #245fe0;
        }
        QScrollArea#ChatScroll {
            background: transparent;
            border: none;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 4px 0 4px 0;
        }
        QScrollBar::handle:vertical {
            background: #d6dfec;
            border-radius: 4px;
            min-height: 32px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi()
{
    setWindowTitle(t("AI Mobile Test Studio"));
    setMinimumSize(1280, 760);
    resize(1600, 900);
    qApp->setStyleSheet(globalStyle());

    auto *root = makePanel("Root");
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    rootLayout->addWidget(makeHeader());

    auto *content = new QWidget;
    auto *contentLayout = new QHBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    contentLayout->addWidget(makeSidebar());

    auto *workspace = new QWidget;
    auto *workspaceLayout = new QVBoxLayout(workspace);
    workspaceLayout->setContentsMargins(0, 0, 0, 0);
    workspaceLayout->setSpacing(0);
    workspaceLayout->addWidget(makeToolbar());

    auto *mainArea = new QWidget;
    auto *mainLayout = new QHBoxLayout(mainArea);
    mainLayout->setContentsMargins(16, 16, 16, 0);
    mainLayout->setSpacing(14);
    mainLayout->addWidget(makeDevicePane());
    mainLayout->addWidget(makeChatPane(), 1);
    workspaceLayout->addWidget(mainArea, 1);
    workspaceLayout->addWidget(makeFooter());

    contentLayout->addWidget(workspace, 1);
    rootLayout->addWidget(content, 1);
    setCentralWidget(root);
}
