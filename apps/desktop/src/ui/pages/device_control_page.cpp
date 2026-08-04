#include "ui/pages/device_control_page.h"

#include "core/device_command_catalog.h"
#include "ui/common/widget_helpers.h"

#include <QBoxLayout>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>

#include <utility>

namespace {

QLabel *label(const QString &value,
              int size,
              QFont::Weight weight,
              const QString &color)
{
    auto *result = new QLabel(value);
    result->setFont(ui::appFont(size, weight));
    result->setStyleSheet(QStringLiteral("color:%1;").arg(color));
    return result;
}

QString commandCaption(const DeviceCommand &command)
{
    if (command.type == DeviceCommandType::KeyEvent) {
        return command.label + QLatin1Char('\n') + command.value;
    }
    if (command.type == DeviceCommandType::PowerOff) {
        return command.label + QStringLiteral("\nadb shell reboot -p");
    }
    return command.value.isEmpty()
        ? command.label + QStringLiteral("\nadb reboot")
        : command.label + QStringLiteral("\nadb reboot ") + command.value;
}

QString commandToolTip(const DeviceCommand &command)
{
    if (command.type == DeviceCommandType::KeyEvent) {
        return QStringLiteral("adb shell input keyevent %1").arg(command.value);
    }
    if (command.type == DeviceCommandType::PowerOff) {
        return QStringLiteral("adb shell reboot -p");
    }
    return command.value.isEmpty()
        ? QStringLiteral("adb reboot")
        : QStringLiteral("adb reboot %1").arg(command.value);
}

} // namespace

DeviceControlPage::DeviceControlPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("DeviceControlPage");
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(24, 20, 24, 18);
    pageLayout->setSpacing(14);

    auto *titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->setSpacing(12);
    titleRow->addWidget(label(ui::text("设备控制"), 16, QFont::DemiBold, "#111827"));
    titleRow->addStretch();
    m_deviceDot = label(ui::text("●"), 12, QFont::DemiBold, "#aab3c2");
    m_deviceStatus = label(ui::text("未连接设备"), 9, QFont::DemiBold, "#596579");
    titleRow->addWidget(m_deviceDot);
    titleRow->addWidget(m_deviceStatus);
    pageLayout->addLayout(titleRow);

    m_commandStatus = label(ui::text("等待下发命令"), 9, QFont::Normal, "#7b8798");
    m_commandStatus->setObjectName("CommandStatus");
    m_commandStatus->setMinimumHeight(30);
    pageLayout->addWidget(m_commandStatus);

    auto *scroll = new QScrollArea;
    scroll->setObjectName("DeviceControlScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *content = new QWidget;
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 4, 0);
    contentLayout->setSpacing(10);

    const QVector<DeviceCommandCategory> categories = createDeviceCommandCatalog();
    for (const DeviceCommandCategory &category : categories) {
        auto *section = ui::makePanel("CommandCategory");
        auto *sectionLayout = new QVBoxLayout(section);
        sectionLayout->setContentsMargins(0, 0, 0, 0);
        sectionLayout->setSpacing(0);

        auto *header = ui::makePanel("CommandCategoryHeader");
        header->setFixedHeight(78);
        auto *headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(14, 10, 14, 10);
        headerLayout->setSpacing(12);

        auto *iconFrame = ui::makePanel("CommandCategoryIcon");
        iconFrame->setFixedSize(52, 52);
        auto *iconLayout = new QVBoxLayout(iconFrame);
        iconLayout->setContentsMargins(0, 0, 0, 0);
        auto *iconLabel = new QLabel;
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setPixmap(ui::imagePixmap(
            QStringLiteral("icons/设备控制/%1.png").arg(category.title),
            QSize(36, 36)));
        iconLayout->addWidget(iconLabel);
        headerLayout->addWidget(iconFrame);

        auto *headerButton = new QPushButton(category.title + QLatin1Char('\n') + category.commandTemplate);
        headerButton->setObjectName("CommandCategoryButton");
        headerButton->setCursor(Qt::PointingHandCursor);
        headerButton->setFont(ui::appFont(10, QFont::DemiBold));
        headerButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        headerLayout->addWidget(headerButton, 1);

        auto *toggleButton = new QToolButton;
        toggleButton->setObjectName("CommandCategoryToggle");
        toggleButton->setText(QStringLiteral("＋"));
        toggleButton->setToolTip(ui::text("展开分类"));
        toggleButton->setCursor(Qt::PointingHandCursor);
        toggleButton->setFixedSize(34, 34);
        headerLayout->addWidget(toggleButton);
        sectionLayout->addWidget(header);

        auto *commandsPanel = ui::makePanel("CommandCategoryContent");
        commandsPanel->setVisible(false);
        auto *commandsLayout = new QGridLayout(commandsPanel);
        commandsLayout->setContentsMargins(16, 14, 16, 16);
        commandsLayout->setHorizontalSpacing(10);
        commandsLayout->setVerticalSpacing(10);

        constexpr int kColumns = 4;
        for (int index = 0; index < category.commands.size(); ++index) {
            const DeviceCommand command = category.commands[index];
            auto *commandButton = new QPushButton(commandCaption(command));
            commandButton->setObjectName("DeviceCommandButton");
            commandButton->setProperty("danger", command.type != DeviceCommandType::KeyEvent);
            commandButton->setToolTip(commandToolTip(command));
            commandButton->setCursor(Qt::PointingHandCursor);
            commandButton->setFont(ui::appFont(9, QFont::DemiBold));
            commandButton->setMinimumHeight(56);
            commandButton->setEnabled(false);
            m_commandButtons.append(commandButton);
            commandsLayout->addWidget(commandButton, index / kColumns, index % kColumns);

            if (command.type == DeviceCommandType::KeyEvent) {
                connect(commandButton, &QPushButton::clicked, this, [this, command] {
                    emit keyEventRequested(command.value);
                });
            } else if (command.type == DeviceCommandType::Reboot) {
                connect(commandButton, &QPushButton::clicked, this, [this, command] {
                    emit rebootRequested(command.value, command.label);
                });
            } else {
                connect(commandButton, &QPushButton::clicked, this, [this, command] {
                    emit powerOffRequested(command.label);
                });
            }
        }
        for (int column = 0; column < kColumns; ++column) {
            commandsLayout->setColumnStretch(column, 1);
        }
        sectionLayout->addWidget(commandsPanel);

        const auto toggle = [commandsPanel, toggleButton] {
            const bool expanded = !commandsPanel->isVisible();
            commandsPanel->setVisible(expanded);
            toggleButton->setText(expanded ? QStringLiteral("－") : QStringLiteral("＋"));
            toggleButton->setToolTip(expanded ? ui::text("收起分类") : ui::text("展开分类"));
        };
        connect(headerButton, &QPushButton::clicked, this, toggle);
        connect(toggleButton, &QToolButton::clicked, this, toggle);
        contentLayout->addWidget(section);
    }

    contentLayout->addStretch();
    scroll->setWidget(content);
    pageLayout->addWidget(scroll, 1);
}

void DeviceControlPage::setDeviceConnected(bool connected, const QString &serial)
{
    m_deviceDot->setStyleSheet(connected ? QStringLiteral("color:#66c95e;")
                                         : QStringLiteral("color:#aab3c2;"));
    m_deviceStatus->setText(connected
                                ? ui::text("已连接 · %1").arg(serial)
                                : ui::text("未连接设备"));
    for (QPushButton *button : std::as_const(m_commandButtons)) {
        button->setEnabled(connected);
    }
}

void DeviceControlPage::showCommandStarted(const QString &label, const QString &command)
{
    m_commandStatus->setStyleSheet(QStringLiteral("color:#2f6df6;"));
    m_commandStatus->setText(ui::text("正在执行：%1  ·  %2").arg(label, command));
}

void DeviceControlPage::showCommandResult(bool success,
                                          const QString &label,
                                          const QString &detail)
{
    m_commandStatus->setStyleSheet(success ? QStringLiteral("color:#459b47;")
                                           : QStringLiteral("color:#d45b5b;"));
    m_commandStatus->setText(success
                                 ? ui::text("执行成功：%1").arg(label)
                                 : ui::text("执行失败：%1 · %2").arg(label, detail));
}
