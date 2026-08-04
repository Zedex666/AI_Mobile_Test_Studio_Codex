#include "ui/pages/other_page.h"

#include "ui/common/widget_helpers.h"

#include <QAbstractButton>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
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

QString outputValue(const QString &output, const QString &key)
{
    const QString prefix = key + QLatin1Char('=');
    for (const QString &line : output.split(QLatin1Char('\n'))) {
        const QString value = line.trimmed();
        if (value.startsWith(prefix)) {
            return value.mid(prefix.size()).trimmed();
        }
    }
    return {};
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

    m_contentStack = new QStackedWidget;
    m_contentStack->setObjectName("OtherContentStack");
    pageLayout->addWidget(m_contentStack, 1);

    auto *overviewScroll = new QScrollArea;
    overviewScroll->setObjectName("OtherScroll");
    overviewScroll->setWidgetResizable(true);
    overviewScroll->setFrameShape(QFrame::NoFrame);
    auto *overviewContent = new QWidget;
    overviewContent->setObjectName("OtherContent");
    auto *overviewLayout = new QVBoxLayout(overviewContent);
    overviewLayout->setContentsMargins(42, 22, 42, 28);
    overviewLayout->setSpacing(0);
    auto *list = ui::makePanel("OtherCommandList");
    list->setMinimumWidth(620);
    list->setMaximumWidth(1080);
    auto *listLayout = new QVBoxLayout(list);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(0);
    addCommandRow(listLayout,
                  0,
                  QStringLiteral("icons/其它/自定义命令.png"),
                  ui::text("运行自定义命令"),
                  QStringLiteral("<customize command>"));
    addCommandRow(listLayout,
                  1,
                  QStringLiteral("icons/其它/账户.png"),
                  ui::text("账户"),
                  QStringLiteral("adb shell dumpsys account"));
    addCommandRow(listLayout,
                  2,
                  QStringLiteral("icons/其它/去除叹号.png"),
                  ui::text("去除叹号"),
                  QStringLiteral("adb shell settings put global captive_portal_https_url <server>"));
    addCommandRow(listLayout,
                  3,
                  QStringLiteral("icons/其它/过渡动画.png"),
                  ui::text("过渡动画"),
                  QStringLiteral("adb shell settings put global <animation type> <value>"));
    addCommandRow(listLayout,
                  4,
                  QStringLiteral("icons/其它/状态栏与导航栏.png"),
                  ui::text("状态栏与导航栏"),
                  QStringLiteral("adb shell settings put secure icon_blacklist <icon name>"));
    addCommandRow(listLayout,
                  5,
                  QStringLiteral("icons/其它/分级调节振动强度.png"),
                  ui::text("分级调节震动强度"),
                  QStringLiteral("adb shell settings put system <vibrator> <value>"));
    auto *overviewRow = new QHBoxLayout;
    overviewRow->addWidget(list, 1);
    overviewLayout->addLayout(overviewRow);
    overviewLayout->addStretch();
    overviewScroll->setWidget(overviewContent);
    m_contentStack->addWidget(overviewScroll);

    auto makeDetailPage = [this](const QString &iconPath,
                                 const QString &title,
                                 const QString &subtitle,
                                 QVBoxLayout **contentLayout) {
        auto *scroll = new QScrollArea;
        scroll->setObjectName("OtherDetailScroll");
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        auto *canvas = new QWidget;
        canvas->setObjectName("OtherDetailCanvas");
        auto *canvasLayout = new QHBoxLayout(canvas);
        canvasLayout->setContentsMargins(42, 24, 42, 28);
        auto *column = new QWidget;
        column->setObjectName("OtherDetailContent");
        column->setMinimumWidth(620);
        column->setMaximumWidth(1020);
        auto *layout = new QVBoxLayout(column);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(14);

        auto *header = new QWidget;
        header->setObjectName("OtherDetailHeader");
        auto *headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(0, 0, 0, 8);
        headerLayout->setSpacing(16);
        auto *iconLabel = new QLabel;
        iconLabel->setObjectName("OtherDetailIcon");
        iconLabel->setFixedSize(54, 54);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setPixmap(ui::imagePixmap(iconPath, QSize(38, 38)));
        headerLayout->addWidget(iconLabel);
        auto *titles = new QVBoxLayout;
        titles->setSpacing(3);
        titles->addWidget(makeLabel(title,
                                    18,
                                    QFont::DemiBold,
                                    QStringLiteral("#1d1d1f")));
        auto *subtitleLabel = makeLabel(subtitle,
                                        9,
                                        QFont::Normal,
                                        QStringLiteral("#6e6e73"));
        subtitleLabel->setWordWrap(true);
        titles->addWidget(subtitleLabel);
        headerLayout->addLayout(titles, 1);
        layout->addWidget(header);

        canvasLayout->addWidget(column, 1);
        scroll->setWidget(canvas);
        *contentLayout = layout;
        m_contentStack->addWidget(scroll);
        return scroll;
    };

    auto makeActionButton = [this](const QString &text, bool primary = false) {
        auto *button = new QPushButton(text);
        button->setObjectName("OtherActionButton");
        button->setProperty("primary", primary);
        button->setMinimumHeight(40);
        button->setCursor(Qt::PointingHandCursor);
        m_actionButtons.append(button);
        return button;
    };

    auto addFooter = [this](QVBoxLayout *layout, QAbstractButton *action) {
        layout->addStretch();
        auto *footer = new QHBoxLayout;
        footer->setContentsMargins(0, 14, 0, 0);
        auto *back = new QToolButton;
        back->setObjectName("OtherBackButton");
        back->setText(QStringLiteral("←"));
        back->setToolTip(ui::text("后退"));
        back->setFixedSize(42, 42);
        back->setCursor(Qt::PointingHandCursor);
        connect(back, &QToolButton::clicked, this, &OtherPage::showOverview);
        footer->addWidget(back);
        footer->addStretch();
        if (action != nullptr) {
            footer->addWidget(action);
        }
        layout->addLayout(footer);
    };

    auto makeSettingRow = [](const QString &title) {
        auto *row = new QFrame;
        row->setObjectName("OtherSettingRow");
        row->setMinimumHeight(66);
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(16, 10, 12, 10);
        layout->setSpacing(12);
        auto *label = makeLabel(title, 10, QFont::DemiBold, QStringLiteral("#1d1d1f"));
        label->setMinimumWidth(150);
        layout->addWidget(label);
        return qMakePair(row, layout);
    };

    QVBoxLayout *detailLayout = nullptr;
    makeDetailPage(QStringLiteral("icons/其它/自定义命令.png"),
                   ui::text("运行自定义命令"),
                   ui::text("直接在当前设备上执行 ADB Shell 命令"),
                   &detailLayout);
    m_customCommandInput = new QLineEdit;
    m_customCommandInput->setObjectName("OtherCommandInput");
    m_customCommandInput->setPlaceholderText(ui::text("输入 ADB Shell 命令"));
    m_customCommandInput->setText(QStringLiteral("dumpsys battery"));
    detailLayout->addWidget(m_customCommandInput);
    m_customOutput = new QPlainTextEdit;
    m_customOutput->setObjectName("OtherResultOutput");
    m_customOutput->setReadOnly(true);
    m_customOutput->setPlaceholderText(ui::text("命令输出会显示在这里"));
    m_customOutput->setMinimumHeight(240);
    detailLayout->addWidget(m_customOutput, 1);
    auto *runCustom = makeActionButton(ui::text("执行命令"), true);
    connect(runCustom, &QPushButton::clicked, this, [this] {
        const QString command = m_customCommandInput->text().trimmed();
        if (!command.isEmpty()) {
            executeCommand(ui::text("自定义命令"), command);
        }
    });
    connect(m_customCommandInput, &QLineEdit::returnPressed, runCustom, &QPushButton::click);
    addFooter(detailLayout, runCustom);

    makeDetailPage(QStringLiteral("icons/其它/账户.png"),
                   ui::text("账户"),
                   ui::text("查看 Android 用户与已注册账户"),
                   &detailLayout);
    m_accountOutput = new QPlainTextEdit;
    m_accountOutput->setObjectName("OtherResultOutput");
    m_accountOutput->setReadOnly(true);
    m_accountOutput->setPlaceholderText(ui::text("正在等待账户信息"));
    m_accountOutput->setMinimumHeight(340);
    detailLayout->addWidget(m_accountOutput, 1);
    auto *refreshAccounts = makeActionButton(ui::text("刷新"), true);
    connect(refreshAccounts, &QPushButton::clicked, this, [this] {
        executeCommand(ui::text("读取账户"), QStringLiteral("dumpsys account"));
    });
    addFooter(detailLayout, refreshAccounts);

    makeDetailPage(QStringLiteral("icons/其它/去除叹号.png"),
                   ui::text("去除叹号"),
                   ui::text("修改网络可用性验证服务器"),
                   &detailLayout);
    auto versionRow = makeSettingRow(ui::text("Android 版本"));
    m_androidVersion = makeLabel(QStringLiteral("--"),
                                 10,
                                 QFont::Normal,
                                 QStringLiteral("#6e6e73"));
    versionRow.second->addWidget(m_androidVersion, 1);
    detailLayout->addWidget(versionRow.first);
    auto serverRow = makeSettingRow(ui::text("验证服务器"));
    m_serverCombo = new QComboBox;
    m_serverCombo->setObjectName("OtherServerCombo");
    m_serverCombo->setEditable(true);
    m_serverCombo->addItems({QStringLiteral("connect.rom.miui.com/generate_204"),
                             QStringLiteral("connectivitycheck.platform.hicloud.com/generate_204"),
                             QStringLiteral("wifi.vivo.com.cn/generate_204"),
                             QStringLiteral("cp.cloudflare.com/generate_204"),
                             QStringLiteral("g.cn/generate_204"),
                             QStringLiteral("connectivitycheck.gstatic.com/generate_204")});
    serverRow.second->addWidget(m_serverCombo, 1);
    auto *applyServer = makeActionButton(ui::text("应用设置"), true);
    applyServer->setMinimumWidth(84);
    serverRow.second->addWidget(applyServer);
    detailLayout->addWidget(serverRow.first);
    auto currentServerRow = makeSettingRow(ui::text("当前服务器"));
    m_currentServer = makeLabel(QStringLiteral("--"),
                                9,
                                QFont::Normal,
                                QStringLiteral("#6e6e73"));
    m_currentServer->setTextInteractionFlags(Qt::TextSelectableByMouse);
    currentServerRow.second->addWidget(m_currentServer, 1);
    detailLayout->addWidget(currentServerRow.first);
    connect(applyServer, &QPushButton::clicked, this, [this] {
        QString server = m_serverCombo->currentText().trimmed();
        server.remove(QStringLiteral("https://"));
        server.remove(QStringLiteral("http://"));
        if (server.isEmpty()) {
            return;
        }
        m_pendingSettingKey = QStringLiteral("captive_portal");
        m_pendingSettingValue = server;
        executeCommand(ui::text("修改网络验证服务器"),
                       QStringLiteral("settings put global captive_portal_mode 1; "
                                      "settings put global captive_portal_http_url http://%1; "
                                      "settings put global captive_portal_https_url https://%1")
                           .arg(server));
    });
    auto *restoreServer = makeActionButton(ui::text("恢复默认"));
    connect(restoreServer, &QPushButton::clicked, this, [this] {
        m_pendingSettingKey = QStringLiteral("captive_portal");
        m_pendingSettingValue = ui::text("系统默认");
        executeCommand(ui::text("恢复网络验证服务器"),
                       QStringLiteral("settings delete global captive_portal_http_url; "
                                      "settings delete global captive_portal_https_url; "
                                      "settings delete global captive_portal_fallback_url; "
                                      "settings put global captive_portal_mode 1"));
    });
    addFooter(detailLayout, restoreServer);

    makeDetailPage(QStringLiteral("icons/其它/过渡动画.png"),
                   ui::text("过渡动画"),
                   ui::text("调整 Android 系统动画缩放"),
                   &detailLayout);
    const QList<QPair<QString, QString>> animationSettings = {
        {QStringLiteral("animator_duration_scale"), ui::text("动画时长")},
        {QStringLiteral("transition_animation_scale"), ui::text("过渡动画")},
        {QStringLiteral("window_animation_scale"), ui::text("窗口动画")}};
    for (const auto &setting : animationSettings) {
        auto row = makeSettingRow(setting.second);
        auto *current = makeLabel(ui::text("当前：--"),
                                  9,
                                  QFont::Normal,
                                  QStringLiteral("#6e6e73"));
        current->setMinimumWidth(100);
        row.second->addWidget(current);
        auto *input = new QDoubleSpinBox;
        input->setObjectName("OtherNumericInput");
        input->setRange(0.0, 10.0);
        input->setSingleStep(0.5);
        input->setDecimals(1);
        input->setValue(1.0);
        row.second->addWidget(input, 1);
        auto *apply = makeActionButton(ui::text("执行"), true);
        apply->setMinimumWidth(74);
        row.second->addWidget(apply);
        m_animationCurrent.insert(setting.first, current);
        m_animationInputs.insert(setting.first, input);
        connect(apply, &QPushButton::clicked, this, [this, key = setting.first, input] {
            const QString value = QString::number(input->value(), 'f', 1);
            m_pendingSettingKey = key;
            m_pendingSettingValue = value;
            executeCommand(ui::text("修改动画缩放"),
                           QStringLiteral("settings put global %1 %2").arg(key, value));
        });
        detailLayout->addWidget(row.first);
    }
    auto *refreshAnimation = makeActionButton(ui::text("刷新"));
    connect(refreshAnimation, &QPushButton::clicked, this, &OtherPage::refreshAnimations);
    addFooter(detailLayout, refreshAnimation);

    makeDetailPage(QStringLiteral("icons/其它/状态栏与导航栏.png"),
                   ui::text("状态栏与导航栏"),
                   ui::text("隐藏状态图标或启用全局沉浸模式"),
                   &detailLayout);
    auto *iconsPanel = new QFrame;
    iconsPanel->setObjectName("OtherSettingsGroup");
    auto *iconsLayout = new QVBoxLayout(iconsPanel);
    iconsLayout->setContentsMargins(16, 14, 16, 14);
    iconsLayout->setSpacing(10);
    iconsLayout->addWidget(makeLabel(ui::text("隐藏状态栏图标"),
                                     11,
                                     QFont::DemiBold,
                                     QStringLiteral("#1d1d1f")));
    auto *iconGrid = new QGridLayout;
    iconGrid->setHorizontalSpacing(18);
    iconGrid->setVerticalSpacing(10);
    const QList<QPair<QString, QString>> statusIcons = {
        {ui::text("静音 / 震动"), QStringLiteral("volume")},
        {ui::text("定位"), QStringLiteral("location")},
        {ui::text("麦克风"), QStringLiteral("microphone")},
        {ui::text("录屏状态"), QStringLiteral("screen_record")},
        {QStringLiteral("Wi-Fi"), QStringLiteral("wifi")},
        {ui::text("热点"), QStringLiteral("hotspot")},
        {ui::text("飞行模式"), QStringLiteral("airplane")},
        {ui::text("耳机"), QStringLiteral("headset")},
        {ui::text("闹钟"), QStringLiteral("alarm_clock")},
        {ui::text("蓝牙"), QStringLiteral("bluetooth")},
        {QStringLiteral("NFC"), QStringLiteral("nfc")}};
    for (int index = 0; index < statusIcons.size(); ++index) {
        auto *check = new QCheckBox(statusIcons[index].first);
        check->setProperty("iconName", statusIcons[index].second);
        iconGrid->addWidget(check, index / 4, index % 4);
        m_statusIconChecks.append(check);
    }
    iconsLayout->addLayout(iconGrid);
    auto *iconActions = new QHBoxLayout;
    auto *hideIcons = makeActionButton(ui::text("隐藏所选"), true);
    auto *resetIcons = makeActionButton(ui::text("重置图标"));
    iconActions->addWidget(hideIcons);
    iconActions->addWidget(resetIcons);
    iconActions->addStretch();
    iconsLayout->addLayout(iconActions);
    detailLayout->addWidget(iconsPanel);
    connect(hideIcons, &QPushButton::clicked, this, [this] {
        QStringList icons;
        for (QCheckBox *check : std::as_const(m_statusIconChecks)) {
            if (check->isChecked()) {
                icons.append(check->property("iconName").toString());
            }
        }
        if (!icons.isEmpty()) {
            executeCommand(ui::text("隐藏状态栏图标"),
                           QStringLiteral("settings put secure icon_blacklist %1")
                               .arg(icons.join(QLatin1Char(','))));
        }
    });
    connect(resetIcons, &QPushButton::clicked, this, [this] {
        executeCommand(ui::text("重置状态栏图标"),
                       QStringLiteral("settings delete secure icon_blacklist"));
    });
    auto *barsPanel = new QFrame;
    barsPanel->setObjectName("OtherSettingsGroup");
    auto *barsLayout = new QVBoxLayout(barsPanel);
    barsLayout->setContentsMargins(16, 14, 16, 14);
    barsLayout->setSpacing(10);
    barsLayout->addWidget(makeLabel(ui::text("全局隐藏"),
                                    11,
                                    QFont::DemiBold,
                                    QStringLiteral("#1d1d1f")));
    auto *barActions = new QHBoxLayout;
    const QList<QPair<QString, QString>> barCommands = {
        {ui::text("隐藏状态栏"), QStringLiteral("settings put global policy_control immersive.status=*")},
        {ui::text("隐藏导航栏"), QStringLiteral("settings put global policy_control immersive.navigation=*")},
        {ui::text("同时隐藏"), QStringLiteral("settings put global policy_control immersive.full=*")},
        {ui::text("重置"), QStringLiteral("settings delete global policy_control")}};
    for (const auto &action : barCommands) {
        auto *button = makeActionButton(action.first, action.first == ui::text("同时隐藏"));
        connect(button, &QPushButton::clicked, this, [this, action] {
            executeCommand(action.first, action.second);
        });
        barActions->addWidget(button);
    }
    barsLayout->addLayout(barActions);
    detailLayout->addWidget(barsPanel);
    addFooter(detailLayout, nullptr);

    makeDetailPage(QStringLiteral("icons/其它/分级调节振动强度.png"),
                   ui::text("分级调节震动强度"),
                   ui::text("不同设备与 ROM 支持的设置项可能不同"),
                   &detailLayout);
    const QList<QPair<QString, QString>> vibrationSettings = {
        {QStringLiteral("vibrate_on"), ui::text("震动总开关")},
        {QStringLiteral("haptic_feedback_enabled"), ui::text("触感震动开关")},
        {QStringLiteral("ring_vibration_enabled"), ui::text("响铃震动开关")},
        {QStringLiteral("charging_vibration_enabled"), ui::text("充电震动开关")},
        {QStringLiteral("haptic_feedback_intensity"), ui::text("触感震动强度")},
        {QStringLiteral("hardware_haptic_feedback_intensity"), ui::text("硬件触感震动强度")},
        {QStringLiteral("notification_vibration_intensity"), ui::text("通知震动强度")},
        {QStringLiteral("alarm_vibration_intensity"), ui::text("闹钟震动强度")},
        {QStringLiteral("media_vibration_intensity"), ui::text("媒体震动强度")},
        {QStringLiteral("ring_vibration_intensity"), ui::text("铃声震动强度")}};
    for (int index = 0; index < vibrationSettings.size(); ++index) {
        const auto &setting = vibrationSettings[index];
        auto row = makeSettingRow(setting.second);
        auto *current = makeLabel(ui::text("当前：--"),
                                  9,
                                  QFont::Normal,
                                  QStringLiteral("#6e6e73"));
        current->setMinimumWidth(100);
        row.second->addWidget(current);
        auto *input = new QSpinBox;
        input->setObjectName("OtherNumericInput");
        input->setRange(0, index < 4 ? 1 : 3);
        input->setValue(index < 4 ? 1 : 2);
        row.second->addWidget(input, 1);
        auto *apply = makeActionButton(ui::text("执行"), true);
        apply->setMinimumWidth(74);
        row.second->addWidget(apply);
        m_vibrationCurrent.insert(setting.first, current);
        m_vibrationInputs.insert(setting.first, input);
        connect(apply, &QPushButton::clicked, this, [this, key = setting.first, input] {
            const QString value = QString::number(input->value());
            m_pendingSettingKey = key;
            m_pendingSettingValue = value;
            executeCommand(ui::text("修改震动强度"),
                           QStringLiteral("settings put system %1 %2").arg(key, value));
        });
        detailLayout->addWidget(row.first);
    }
    auto *refreshVibrationButton = makeActionButton(ui::text("刷新"));
    connect(refreshVibrationButton,
            &QPushButton::clicked,
            this,
            &OtherPage::refreshVibration);
    addFooter(detailLayout, refreshVibrationButton);

    showOverview();
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
    for (QAbstractButton *button : std::as_const(m_actionButtons)) {
        button->setEnabled(enabled);
    }
}

void OtherPage::showCommandStarted(const QString &label, const QString &displayCommand)
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

    if (label == ui::text("自定义命令")) {
        m_customOutput->setPlainText(output);
    } else if (label == ui::text("读取账户")) {
        m_accountOutput->setPlainText(output);
    } else if (label == ui::text("读取网络验证配置") && success) {
        const QString version = outputValue(output, QStringLiteral("version"));
        const QString server = outputValue(output, QStringLiteral("server"));
        m_androidVersion->setText(version.isEmpty() ? QStringLiteral("--") : version);
        m_currentServer->setText(server.isEmpty() || server == QStringLiteral("null")
                                     ? ui::text("系统默认")
                                     : server);
    } else if (label == ui::text("读取动画缩放") && success) {
        for (auto iterator = m_animationCurrent.cbegin();
             iterator != m_animationCurrent.cend();
             ++iterator) {
            const QString value = outputValue(output, iterator.key());
            iterator.value()->setText(ui::text("当前：%1").arg(value.isEmpty()
                                                                   ? QStringLiteral("--")
                                                                   : value));
        }
    } else if (label == ui::text("读取震动设置") && success) {
        for (auto iterator = m_vibrationCurrent.cbegin();
             iterator != m_vibrationCurrent.cend();
             ++iterator) {
            const QString value = outputValue(output, iterator.key());
            iterator.value()->setText(ui::text("当前：%1").arg(value.isEmpty()
                                                                   ? QStringLiteral("--")
                                                                   : value));
        }
    } else if (success && !m_pendingSettingKey.isEmpty()) {
        if (QLabel *current = m_animationCurrent.value(m_pendingSettingKey)) {
            current->setText(ui::text("当前：%1").arg(m_pendingSettingValue));
        }
        if (QLabel *current = m_vibrationCurrent.value(m_pendingSettingKey)) {
            current->setText(ui::text("当前：%1").arg(m_pendingSettingValue));
        }
        if (m_pendingSettingKey == QStringLiteral("captive_portal")) {
            m_currentServer->setText(m_pendingSettingValue);
        }
    }

    m_pendingSettingKey.clear();
    m_pendingSettingValue.clear();
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
                              const QString &iconPath,
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

    auto *iconLabel = new QLabel;
    iconLabel->setObjectName("OtherCommandIcon");
    iconLabel->setFixedSize(76, 76);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setPixmap(ui::imagePixmap(iconPath, QSize(48, 48)));
    rowLayout->addWidget(iconLabel);
    auto *text = new QWidget;
    auto *textLayout = new QVBoxLayout(text);
    textLayout->setContentsMargins(0, 8, 0, 8);
    textLayout->setSpacing(9);
    textLayout->addWidget(makeLabel(title,
                                    12,
                                    QFont::Normal,
                                    QStringLiteral("#111827")));
    auto *commandLabel = makeLabel(QStringLiteral("▣  ") + command,
                                   9,
                                   QFont::Normal,
                                   QStringLiteral("#758092"));
    commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    textLayout->addWidget(commandLabel);
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
    if (!m_connected || m_busy || index < 0 || index > 5) {
        return;
    }
    m_currentDetail = index;
    m_contentStack->setCurrentIndex(index + 1);
    switch (index) {
    case 1:
        executeCommand(ui::text("读取账户"), QStringLiteral("dumpsys account"));
        break;
    case 2:
        refreshCaptivePortal();
        break;
    case 3:
        refreshAnimations();
        break;
    case 5:
        refreshVibration();
        break;
    default:
        break;
    }
}

void OtherPage::showOverview()
{
    m_currentDetail = -1;
    m_contentStack->setCurrentIndex(0);
}

void OtherPage::executeCommand(const QString &label, const QString &command)
{
    if (!m_connected || m_busy || command.trimmed().isEmpty()) {
        return;
    }
    emit shellCommandRequested(label, command);
}

void OtherPage::refreshCaptivePortal()
{
    executeCommand(ui::text("读取网络验证配置"),
                   QStringLiteral("printf 'version='; getprop ro.build.version.release; "
                                  "printf 'server='; settings get global captive_portal_https_url"));
}

void OtherPage::refreshAnimations()
{
    executeCommand(ui::text("读取动画缩放"),
                   QStringLiteral("printf 'animator_duration_scale='; "
                                  "settings get global animator_duration_scale; "
                                  "printf 'transition_animation_scale='; "
                                  "settings get global transition_animation_scale; "
                                  "printf 'window_animation_scale='; "
                                  "settings get global window_animation_scale"));
}

void OtherPage::refreshVibration()
{
    const QStringList keys = m_vibrationCurrent.keys();
    executeCommand(ui::text("读取震动设置"),
                   QStringLiteral("for key in %1; do printf \"$key=\"; "
                                  "settings get system \"$key\"; done")
                       .arg(keys.join(QLatin1Char(' '))));
}
