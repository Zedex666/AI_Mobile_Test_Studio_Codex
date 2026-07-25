#include "core/device_command_catalog.h"

#include <QSet>

#include <utility>

namespace {

DeviceCommand keyEvent(const char *value, const char *label)
{
    return {QString::fromUtf8(label), QString::fromLatin1(value), DeviceCommandType::KeyEvent};
}

DeviceCommand reboot(const char *value, const char *label)
{
    return {QString::fromUtf8(label), QString::fromLatin1(value), DeviceCommandType::Reboot};
}

DeviceCommand powerOff(const char *label)
{
    return {QString::fromUtf8(label), QString(), DeviceCommandType::PowerOff};
}

DeviceCommandCategory keyCategory(const char *title,
                                  const char *icon,
                                  QVector<DeviceCommand> commands)
{
    return {QString::fromUtf8(title),
            QString::fromUtf8(icon),
            QStringLiteral("adb shell input keyevent <key>"),
            std::move(commands)};
}

void assertUniqueKeyCodes(const QVector<DeviceCommandCategory> &categories)
{
#ifndef QT_NO_DEBUG
    QSet<QString> keyCodes;
    for (const DeviceCommandCategory &category : categories) {
        for (const DeviceCommand &command : category.commands) {
            if (command.type != DeviceCommandType::KeyEvent) {
                continue;
            }
            Q_ASSERT_X(!keyCodes.contains(command.value),
                       "createDeviceCommandCatalog",
                       qPrintable(QStringLiteral("Duplicate KEYCODE: %1").arg(command.value)));
            keyCodes.insert(command.value);
        }
    }
#else
    Q_UNUSED(categories);
#endif
}

} // namespace

QVector<DeviceCommandCategory> createDeviceCommandCatalog()
{
    QVector<DeviceCommandCategory> categories;

    categories.append({QString::fromUtf8("电源"),
                       QString::fromUtf8("⏻"),
                       QStringLiteral("adb reboot <mode>"),
                       {powerOff("关闭设备"),
                        reboot("", "重启系统"),
                        reboot("recovery", "重启到 Recovery"),
                        reboot("bootloader", "重启到 Bootloader"),
                        reboot("fastboot", "重启到 Fastboot")}});

    categories.append(keyCategory(
        "系统导航",
        "←",
        {keyEvent("KEYCODE_BACK", "返回"),
         keyEvent("KEYCODE_HOME", "主页"),
         keyEvent("KEYCODE_MENU", "菜单"),
         keyEvent("KEYCODE_APP_SWITCH", "切换应用"),
         keyEvent("KEYCODE_SEARCH", "搜索"),
         keyEvent("KEYCODE_NOTIFICATION", "通知面板"),
         keyEvent("KEYCODE_FORWARD", "前进"),
         keyEvent("KEYCODE_SOFT_LEFT", "左软键"),
         keyEvent("KEYCODE_SOFT_RIGHT", "右软键"),
         keyEvent("KEYCODE_WINDOW", "窗口")}));

    categories.append(keyCategory(
        "实体按键",
        "◉",
        {keyEvent("KEYCODE_POWER", "电源键"),
         keyEvent("KEYCODE_VOLUME_UP", "音量增加"),
         keyEvent("KEYCODE_VOLUME_DOWN", "音量减小"),
         keyEvent("KEYCODE_VOLUME_MUTE", "扬声器静音"),
         keyEvent("KEYCODE_MUTE", "话筒静音"),
         keyEvent("KEYCODE_CAMERA", "拍照"),
         keyEvent("KEYCODE_FOCUS", "拍照对焦"),
         keyEvent("KEYCODE_VOICE_ASSIST", "唤醒语音助手")}));

    categories.append(keyCategory(
        "电话按键",
        "☎",
        {keyEvent("KEYCODE_CALL", "拨号"),
         keyEvent("KEYCODE_ENDCALL", "挂机"),
         keyEvent("KEYCODE_HEADSETHOOK", "耳机接听 / 挂断")}));

    categories.append(keyCategory(
        "控制与编辑",
        "⌘",
        {keyEvent("KEYCODE_ENTER", "回车"),
         keyEvent("KEYCODE_ESCAPE", "Esc"),
         keyEvent("KEYCODE_DPAD_CENTER", "导航确定"),
         keyEvent("KEYCODE_DPAD_UP", "导航向上"),
         keyEvent("KEYCODE_DPAD_DOWN", "导航向下"),
         keyEvent("KEYCODE_DPAD_LEFT", "导航向左"),
         keyEvent("KEYCODE_DPAD_RIGHT", "导航向右"),
         keyEvent("KEYCODE_MOVE_HOME", "光标移到开始"),
         keyEvent("KEYCODE_MOVE_END", "光标移到末尾"),
         keyEvent("KEYCODE_PAGE_UP", "向上翻页"),
         keyEvent("KEYCODE_PAGE_DOWN", "向下翻页"),
         keyEvent("KEYCODE_DEL", "退格"),
         keyEvent("KEYCODE_FORWARD_DEL", "向前删除"),
         keyEvent("KEYCODE_INSERT", "插入"),
         keyEvent("KEYCODE_TAB", "Tab"),
         keyEvent("KEYCODE_NUM_LOCK", "小键盘锁"),
         keyEvent("KEYCODE_CAPS_LOCK", "大写锁定"),
         keyEvent("KEYCODE_BREAK", "Break / Pause"),
         keyEvent("KEYCODE_SCROLL_LOCK", "滚动锁定"),
         keyEvent("KEYCODE_ZOOM_IN", "放大"),
         keyEvent("KEYCODE_ZOOM_OUT", "缩小"),
         keyEvent("KEYCODE_CLEAR", "清除"),
         keyEvent("KEYCODE_SYSRQ", "系统请求 / 截屏")}));

    categories.append(keyCategory(
        "显示与休眠",
        "☼",
        {keyEvent("KEYCODE_BRIGHTNESS_DOWN", "减小亮度"),
         keyEvent("KEYCODE_BRIGHTNESS_UP", "增加亮度"),
         keyEvent("KEYCODE_SLEEP", "休眠系统"),
         keyEvent("KEYCODE_SOFT_SLEEP", "无 WakeLock 时休眠"),
         keyEvent("KEYCODE_WAKEUP", "点亮屏幕")}));

    categories.append(keyCategory(
        "修饰与组合",
        "⇧",
        {keyEvent("KEYCODE_ALT_LEFT", "左 Alt"),
         keyEvent("KEYCODE_ALT_RIGHT", "右 Alt"),
         keyEvent("KEYCODE_CTRL_LEFT", "左 Control"),
         keyEvent("KEYCODE_CTRL_RIGHT", "右 Control"),
         keyEvent("KEYCODE_SHIFT_LEFT", "左 Shift"),
         keyEvent("KEYCODE_SHIFT_RIGHT", "右 Shift"),
         keyEvent("KEYCODE_META_LEFT", "左 Meta"),
         keyEvent("KEYCODE_META_RIGHT", "右 Meta"),
         keyEvent("KEYCODE_NUM", "Number 修饰键"),
         keyEvent("KEYCODE_FUNCTION", "Function 修饰键"),
         keyEvent("KEYCODE_PICTSYMBOLS", "图片符号修饰键"),
         keyEvent("KEYCODE_SWITCH_CHARSET", "切换字符集"),
         keyEvent("KEYCODE_SYM", "符号修饰键")}));

    QVector<DeviceCommand> basicKeys;
    for (int number = 0; number <= 9; ++number) {
        basicKeys.append({QString::fromUtf8("数字 %1").arg(number),
                          QStringLiteral("KEYCODE_%1").arg(number),
                          DeviceCommandType::KeyEvent});
    }
    for (char letter = 'A'; letter <= 'Z'; ++letter) {
        basicKeys.append({QString::fromUtf8("字母 %1").arg(QChar(letter)),
                          QStringLiteral("KEYCODE_%1").arg(QChar(letter)),
                          DeviceCommandType::KeyEvent});
    }
    categories.append(keyCategory("字母与数字", "A", std::move(basicKeys)));

    categories.append(keyCategory(
        "符号按键",
        "#",
        {keyEvent("KEYCODE_PLUS", "加号 +"),
         keyEvent("KEYCODE_MINUS", "减号 -"),
         keyEvent("KEYCODE_STAR", "星号 *"),
         keyEvent("KEYCODE_SLASH", "斜杠 /"),
         keyEvent("KEYCODE_EQUALS", "等号 ="),
         keyEvent("KEYCODE_AT", "@"),
         keyEvent("KEYCODE_POUND", "井号 #"),
         keyEvent("KEYCODE_APOSTROPHE", "单引号"),
         keyEvent("KEYCODE_BACKSLASH", "反斜杠"),
         keyEvent("KEYCODE_COMMA", "逗号"),
         keyEvent("KEYCODE_PERIOD", "句点"),
         keyEvent("KEYCODE_LEFT_BRACKET", "左方括号"),
         keyEvent("KEYCODE_RIGHT_BRACKET", "右方括号"),
         keyEvent("KEYCODE_SEMICOLON", "分号"),
         keyEvent("KEYCODE_GRAVE", "反引号"),
         keyEvent("KEYCODE_SPACE", "空格")}));

    QVector<DeviceCommand> numpadKeys;
    for (int number = 0; number <= 9; ++number) {
        numpadKeys.append({QString::fromUtf8("小键盘 %1").arg(number),
                           QStringLiteral("KEYCODE_NUMPAD_%1").arg(number),
                           DeviceCommandType::KeyEvent});
    }
    numpadKeys.append(keyEvent("KEYCODE_NUMPAD_ADD", "小键盘 +"));
    numpadKeys.append(keyEvent("KEYCODE_NUMPAD_SUBTRACT", "小键盘 -"));
    numpadKeys.append(keyEvent("KEYCODE_NUMPAD_MULTIPLY", "小键盘 *"));
    numpadKeys.append(keyEvent("KEYCODE_NUMPAD_DIVIDE", "小键盘 /"));
    numpadKeys.append(keyEvent("KEYCODE_NUMPAD_EQUALS", "小键盘 ="));
    numpadKeys.append(keyEvent("KEYCODE_NUMPAD_COMMA", "小键盘逗号"));
    numpadKeys.append(keyEvent("KEYCODE_NUMPAD_DOT", "小键盘句点"));
    numpadKeys.append(keyEvent("KEYCODE_NUMPAD_LEFT_PAREN", "小键盘左括号"));
    numpadKeys.append(keyEvent("KEYCODE_NUMPAD_RIGHT_PAREN", "小键盘右括号"));
    numpadKeys.append(keyEvent("KEYCODE_NUMPAD_ENTER", "小键盘回车"));
    categories.append(keyCategory("小键盘", "123", std::move(numpadKeys)));

    QVector<DeviceCommand> functionKeys;
    for (int number = 1; number <= 12; ++number) {
        functionKeys.append({QStringLiteral("F%1").arg(number),
                             QStringLiteral("KEYCODE_F%1").arg(number),
                             DeviceCommandType::KeyEvent});
    }
    categories.append(keyCategory("功能按键", "Fn", std::move(functionKeys)));

    categories.append(keyCategory(
        "多媒体控制",
        "▶",
        {keyEvent("KEYCODE_MEDIA_PLAY", "播放"),
         keyEvent("KEYCODE_MEDIA_STOP", "停止"),
         keyEvent("KEYCODE_MEDIA_PAUSE", "暂停"),
         keyEvent("KEYCODE_MEDIA_PLAY_PAUSE", "播放 / 暂停"),
         keyEvent("KEYCODE_MEDIA_FAST_FORWARD", "快进"),
         keyEvent("KEYCODE_MEDIA_REWIND", "快退"),
         keyEvent("KEYCODE_MEDIA_NEXT", "下一首"),
         keyEvent("KEYCODE_MEDIA_PREVIOUS", "上一首"),
         keyEvent("KEYCODE_MEDIA_CLOSE", "关闭"),
         keyEvent("KEYCODE_MEDIA_EJECT", "弹出"),
         keyEvent("KEYCODE_MEDIA_RECORD", "录音")}));

    categories.append(keyCategory(
        "系统应用",
        "▦",
        {keyEvent("KEYCODE_SETTINGS", "打开设置"),
         keyEvent("KEYCODE_CONTACTS", "打开联系人"),
         keyEvent("KEYCODE_EXPLORER", "打开浏览器"),
         keyEvent("KEYCODE_MUSIC", "打开音乐"),
         keyEvent("KEYCODE_CALENDAR", "打开日历"),
         keyEvent("KEYCODE_CALCULATOR", "打开计算器"),
         keyEvent("KEYCODE_ENVELOPE", "打开邮件"),
         keyEvent("KEYCODE_BOOKMARK", "打开书签")}));

    categories.append(keyCategory(
        "电视与遥控",
        "TV",
        {keyEvent("KEYCODE_INFO", "信息"),
         keyEvent("KEYCODE_AVR_INPUT", "功放输入"),
         keyEvent("KEYCODE_AVR_POWER", "功放电源"),
         keyEvent("KEYCODE_CAPTIONS", "字幕开关"),
         keyEvent("KEYCODE_CHANNEL_DOWN", "频道减"),
         keyEvent("KEYCODE_CHANNEL_UP", "频道加"),
         keyEvent("KEYCODE_DVR", "DVR"),
         keyEvent("KEYCODE_GUIDE", "节目指南"),
         keyEvent("KEYCODE_PROG_BLUE", "蓝色可编程键"),
         keyEvent("KEYCODE_PROG_GREEN", "绿色可编程键"),
         keyEvent("KEYCODE_PROG_RED", "红色可编程键"),
         keyEvent("KEYCODE_PROG_YELLOW", "黄色可编程键"),
         keyEvent("KEYCODE_STB_INPUT", "机顶盒输入"),
         keyEvent("KEYCODE_STB_POWER", "机顶盒电源"),
         keyEvent("KEYCODE_TV", "电视"),
         keyEvent("KEYCODE_TV_INPUT", "电视输入"),
         keyEvent("KEYCODE_TV_POWER", "电视电源")}));

    QVector<DeviceCommand> gamepadKeys;
    for (int number = 1; number <= 16; ++number) {
        gamepadKeys.append({QString::fromUtf8("通用按钮 #%1").arg(number),
                            QStringLiteral("KEYCODE_BUTTON_%1").arg(number),
                            DeviceCommandType::KeyEvent});
    }
    gamepadKeys.append(keyEvent("KEYCODE_BUTTON_A", "按钮 A"));
    gamepadKeys.append(keyEvent("KEYCODE_BUTTON_B", "按钮 B"));
    gamepadKeys.append(keyEvent("KEYCODE_BUTTON_C", "按钮 C"));
    gamepadKeys.append(keyEvent("KEYCODE_BUTTON_X", "按钮 X"));
    gamepadKeys.append(keyEvent("KEYCODE_BUTTON_Y", "按钮 Y"));
    gamepadKeys.append(keyEvent("KEYCODE_BUTTON_Z", "按钮 Z"));
    gamepadKeys.append(keyEvent("KEYCODE_BUTTON_L1", "按钮 L1"));
    gamepadKeys.append(keyEvent("KEYCODE_BUTTON_L2", "按钮 L2"));
    gamepadKeys.append(keyEvent("KEYCODE_BUTTON_R1", "按钮 R1"));
    gamepadKeys.append(keyEvent("KEYCODE_BUTTON_R2", "按钮 R2"));
    gamepadKeys.append(keyEvent("KEYCODE_BUTTON_MODE", "Mode"));
    gamepadKeys.append(keyEvent("KEYCODE_BUTTON_SELECT", "Select"));
    gamepadKeys.append(keyEvent("KEYCODE_BUTTON_START", "Start"));
    gamepadKeys.append(keyEvent("KEYCODE_BUTTON_THUMBL", "左摇杆按键"));
    gamepadKeys.append(keyEvent("KEYCODE_BUTTON_THUMBR", "右摇杆按键"));
    categories.append(keyCategory("游戏手柄", "⊕", std::move(gamepadKeys)));

    categories.append(keyCategory(
        "特殊按键",
        "?",
        {keyEvent("KEYCODE_UNKNOWN", "未知按键（通常无作用）")}));

    assertUniqueKeyCodes(categories);
    return categories;
}
