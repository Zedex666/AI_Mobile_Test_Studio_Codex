#include "ui/pages/other_page.h"

#include "ui/common/widget_helpers.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFormLayout>
#include <QFrame>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QSpinBox>
#include <QToolButton>

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

} // namespace

OtherPage::OtherPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("OtherPage");
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    auto *toolbar = new QWidget;
    toolbar->setObjectName("OtherToolbar");
    toolbar->setFixedHeight(50);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(22, 0, 22, 0);
    toolbarLayout->setSpacing(12);
    toolbarLayout->addWidget(makeLabel(ui::text("其它"),
                                       12,
                                       QFont::DemiBold,
                                       QStringLiteral("#172033")));
    m_deviceStatus = makeLabel(ui::text("未连接设备"),
                               8,
                               QFont::Normal,
                               QStringLiteral("#7b8798"));
    toolbarLayout->addWidget(m_deviceStatus);
    toolbarLayout->addStretch();
    m_operationStatus = makeLabel(ui::text("选择一项设备工具"),
                                  8,
                                  QFont::Normal,
                                  QStringLiteral("#7b8798"));
    m_operationStatus->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    toolbarLayout->addWidget(m_operationStatus);
    pageLayout->addWidget(toolbar);

    auto *scroll = new QScrollArea;
    scroll->setObjectName("OtherScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget;
    content->setObjectName("OtherContent");
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(42, 22, 42, 28);
    contentLayout->setSpacing(0);

    auto *list = ui::makePanel("OtherCommandList");
    list->setMinimumWidth(620);
    list->setMaximumWidth(1080);
    auto *listLayout = new QVBoxLayout(list);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(0);
    addCommandRow(listLayout,
                  0,
                  QStringLiteral("</>"),
                  ui::text("运行自定义命令"),
                  QStringLiteral("<customize command>"));
    addCommandRow(listLayout,
                  1,
                  QStringLiteral("♙"),
                  ui::text("账户"),
                  QStringLiteral("adb shell dumpsys account"));
    addCommandRow(listLayout,
                  2,
                  QStringLiteral("⌁"),
                  ui::text("去除叹号"),
                  QStringLiteral("adb shell settings put global captive_portal_http_url <server available>"));
    addCommandRow(listLayout,
                  3,
                  QStringLiteral("◌"),
                  ui::text("过渡动画"),
                  QStringLiteral("adb shell settings put global <animation type>"));
    addCommandRow(listLayout,
                  4,
                  QStringLiteral("▭"),
                  ui::text("状态栏与导航栏"),
                  QStringLiteral("adb shell settings put secure icon_blacklist <icon name>"));
    addCommandRow(listLayout,
                  5,
                  QStringLiteral("◖"),
                  ui::text("分级调节震动强度"),
                  QStringLiteral("adb shell settings put system <vibrator> <value>"));
    listLayout->addStretch();

    auto *listRow = new QHBoxLayout;
    listRow->setContentsMargins(0, 0, 0, 0);
    listRow->addWidget(list, 1);
    contentLayout->addLayout(listRow);
    contentLayout->addStretch();
    scroll->setWidget(content);
    pageLayout->addWidget(scroll, 1);
}

void OtherPage::setDeviceConnected(bool connected, const QString &serial)
{
    m_connected = connected;
    m_serial = connected ? serial : QString();
    m_deviceStatus->setText(connected ? ui::text("已连接 · %1").arg(serial)
                                      : ui::text("未连接设备"));
    m_deviceStatus->setStyleSheet(connected ? QStringLiteral("color:#459b47;")
                                            : QStringLiteral("color:#7b8798;"));
    m_operationStatus->setText(connected ? ui::text("选择一项设备工具")
                                         : ui::text("连接设备后可执行命令"));
    setBusy(m_busy);
}

void OtherPage::setBusy(bool busy)
{
    m_busy = busy;
    const bool enabled = m_connected && !busy;
    for (QFrame *row : std::as_const(m_rows)) {
        row->setEnabled(enabled);
    }
    for (QToolButton *button : std::as_const(m_openButtons)) {
        button->setEnabled(enabled);
    }
}

void OtherPage::showCommandStarted(const QString &label,
                                   const QString &displayCommand)
{
    m_operationStatus->setStyleSheet(QStringLiteral("color:#2f6df6;"));
    m_operationStatus->setText(ui::text("正在执行：%1").arg(label));
    m_operationStatus->setToolTip(displayCommand);
}

void OtherPage::showCommandResult(bool success,
                                  const QString &label,
                                  const QString &output)
{
    m_operationStatus->setStyleSheet(success ? QStringLiteral("color:#459b47;")
                                              : QStringLiteral("color:#d45b5b;"));
    m_operationStatus->setText(success ? ui::text("%1完成").arg(label)
                                       : ui::text("%1失败").arg(label));
    m_operationStatus->setToolTip(output);

    if (!success) {
        QMessageBox::warning(this, ui::text("命令执行失败"), output);
        return;
    }
    if (output == ui::text("操作完成。") || output.trimmed().isEmpty()) {
        return;
    }

    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(label);
    dialog->resize(760, 480);
    auto *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    auto *viewer = new QPlainTextEdit(output);
    viewer->setObjectName("OtherOutput");
    viewer->setReadOnly(true);
    viewer->setFont(ui::appFont(9));
    layout->addWidget(viewer, 1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    layout->addWidget(buttons);
    dialog->show();
}

bool OtherPage::eventFilter(QObject *watched, QEvent *event)
{
    if (!watched->property("commandIndex").isValid()) {
        return QWidget::eventFilter(watched, event);
    }
    if (event->type() == QEvent::MouseButtonPress) {
        const auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            watched->setProperty("commandPressed", true);
        }
        return false;
    }
    if (event->type() == QEvent::Leave) {
        watched->setProperty("commandPressed", false);
        return false;
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        const auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const bool pressed = watched->property("commandPressed").toBool();
        watched->setProperty("commandPressed", false);
        if (pressed && mouseEvent->button() == Qt::LeftButton) {
            openCommand(watched->property("commandIndex").toInt());
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void OtherPage::addCommandRow(QVBoxLayout *layout,
                              int index,
                              const QString &icon,
                              const QString &title,
                              const QString &command)
{
    auto *row = ui::makePanel("OtherCommandRow");
    row->setProperty("commandIndex", index);
    row->setMinimumHeight(110);
    row->setCursor(Qt::PointingHandCursor);
    row->installEventFilter(this);
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(10, 12, 14, 12);
    rowLayout->setSpacing(20);

    auto *iconLabel = makeLabel(icon, 20, QFont::DemiBold, QStringLiteral("#171b22"));
    iconLabel->setObjectName("OtherCommandIcon");
    iconLabel->setFixedSize(76, 76);
    iconLabel->setAlignment(Qt::AlignCenter);
    rowLayout->addWidget(iconLabel);
    auto *text = new QWidget;
    auto *textLayout = new QVBoxLayout(text);
    textLayout->setContentsMargins(0, 8, 0, 8);
    textLayout->setSpacing(9);
    textLayout->addWidget(makeLabel(title,
                                    12,
                                    QFont::Normal,
                                    QStringLiteral("#111827")));
    textLayout->addWidget(makeLabel(QStringLiteral("▣  ") + command,
                                    9,
                                    QFont::Normal,
                                    QStringLiteral("#758092")));
    rowLayout->addWidget(text, 1);
    auto *open = new QToolButton;
    open->setObjectName("OtherOpenButton");
    open->setText(QStringLiteral("›"));
    open->setToolTip(ui::text("打开"));
    open->setFixedSize(34, 34);
    open->setCursor(Qt::PointingHandCursor);
    connect(open, &QToolButton::clicked, this, [this, index] { openCommand(index); });
    rowLayout->addWidget(open, 0, Qt::AlignVCenter);
    layout->addWidget(row);
    m_rows.append(row);
    m_openButtons.append(open);
}

void OtherPage::openCommand(int index)
{
    if (!m_connected || m_busy) {
        return;
    }
    switch (index) {
    case 0:
        runCustomCommand();
        break;
    case 1:
        emit shellCommandRequested(ui::text("读取账户"), QStringLiteral("dumpsys account"));
        break;
    case 2:
        configureCaptivePortal();
        break;
    case 3:
        configureAnimations();
        break;
    case 4:
        configureSystemBars();
        break;
    case 5:
        configureVibration();
        break;
    default:
        break;
    }
}

void OtherPage::runCustomCommand()
{
    bool accepted = false;
    const QString command = QInputDialog::getText(this,
                                                  ui::text("运行自定义命令"),
                                                  ui::text("ADB Shell 命令"),
                                                  QLineEdit::Normal,
                                                  QStringLiteral("dumpsys battery"),
                                                  &accepted)
                                .trimmed();
    if (accepted && !command.isEmpty()) {
        emit shellCommandRequested(ui::text("自定义命令"), command);
    }
}

void OtherPage::configureCaptivePortal()
{
    const QStringList servers = {QStringLiteral("connect.rom.miui.com/generate_204"),
                                 QStringLiteral("connectivitycheck.platform.hicloud.com/generate_204"),
                                 QStringLiteral("wifi.vivo.com.cn/generate_204"),
                                 QStringLiteral("cp.cloudflare.com/generate_204"),
                                 QStringLiteral("g.cn/generate_204"),
                                 QStringLiteral("google.cn/generate_204")};
    bool accepted = false;
    const QString server = QInputDialog::getItem(this,
                                                 ui::text("去除网络叹号"),
                                                 ui::text("可用性验证服务器"),
                                                 servers,
                                                 0,
                                                 true,
                                                 &accepted)
                               .trimmed();
    if (!accepted || server.isEmpty()) {
        return;
    }
    emit shellCommandRequested(ui::text("修改网络验证服务器"),
                               QStringLiteral("settings put global captive_portal_http_url http://%1; "
                                              "settings put global captive_portal_https_url https://%1")
                                   .arg(server));
}

void OtherPage::configureAnimations()
{
    QDialog dialog(this);
    dialog.setWindowTitle(ui::text("过渡动画"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    auto makeScale = [&dialog] {
        auto *spin = new QDoubleSpinBox(&dialog);
        spin->setRange(0.0, 10.0);
        spin->setSingleStep(0.5);
        spin->setDecimals(1);
        spin->setValue(1.0);
        return spin;
    };
    auto *window = makeScale();
    auto *transition = makeScale();
    auto *animator = makeScale();
    form->addRow(ui::text("窗口动画缩放"), window);
    form->addRow(ui::text("过渡动画缩放"), transition);
    form->addRow(ui::text("动画时长缩放"), animator);
    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    emit shellCommandRequested(ui::text("修改动画缩放"),
                               QStringLiteral("settings put global window_animation_scale %1; "
                                              "settings put global transition_animation_scale %2; "
                                              "settings put global animator_duration_scale %3")
                                   .arg(window->value(), 0, 'f', 1)
                                   .arg(transition->value(), 0, 'f', 1)
                                   .arg(animator->value(), 0, 'f', 1));
}

void OtherPage::configureSystemBars()
{
    const QStringList actions = {ui::text("隐藏状态栏"),
                                 ui::text("隐藏导航栏"),
                                 ui::text("全屏沉浸模式"),
                                 ui::text("恢复状态栏与导航栏"),
                                 ui::text("配置隐藏图标")};
    bool accepted = false;
    const QString action = QInputDialog::getItem(this,
                                                 ui::text("状态栏与导航栏"),
                                                 ui::text("操作"),
                                                 actions,
                                                 0,
                                                 false,
                                                 &accepted);
    if (!accepted) {
        return;
    }
    QString command;
    if (action == actions[0]) {
        command = QStringLiteral("settings put global policy_control immersive.status=*");
    } else if (action == actions[1]) {
        command = QStringLiteral("settings put global policy_control immersive.navigation=*");
    } else if (action == actions[2]) {
        command = QStringLiteral("settings put global policy_control immersive.full=*");
    } else if (action == actions[3]) {
        command = QStringLiteral("settings put global policy_control null; "
                                 "settings put secure icon_blacklist null");
    } else {
        const QString icons = QInputDialog::getText(
                                  this,
                                  ui::text("配置隐藏图标"),
                                  ui::text("图标名称，以逗号分隔"),
                                  QLineEdit::Normal,
                                  QStringLiteral("volume,location,alarm_clock"),
                                  &accepted)
                                  .trimmed();
        if (!accepted) {
            return;
        }
        command = QStringLiteral("settings put secure icon_blacklist %1").arg(icons);
    }
    emit shellCommandRequested(action, command);
}

void OtherPage::configureVibration()
{
    QDialog dialog(this);
    dialog.setWindowTitle(ui::text("分级调节震动强度"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    auto *setting = new QComboBox(&dialog);
    setting->addItem(ui::text("触觉反馈"), QStringLiteral("haptic_feedback_intensity"));
    setting->addItem(ui::text("硬件触觉反馈"), QStringLiteral("hardware_haptic_feedback_intensity"));
    setting->addItem(ui::text("通知震动"), QStringLiteral("notification_vibration_intensity"));
    setting->addItem(ui::text("闹钟震动"), QStringLiteral("alarm_vibration_intensity"));
    setting->addItem(ui::text("媒体震动"), QStringLiteral("media_vibration_intensity"));
    setting->addItem(ui::text("铃声震动"), QStringLiteral("ring_vibration_intensity"));
    auto *level = new QSpinBox(&dialog);
    level->setRange(0, 3);
    level->setValue(2);
    form->addRow(ui::text("震动类型"), setting);
    form->addRow(ui::text("强度等级（0-3）"), level);
    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    emit shellCommandRequested(ui::text("修改震动强度"),
                               QStringLiteral("settings put system %1 %2")
                                   .arg(setting->currentData().toString())
                                   .arg(level->value()));
}
