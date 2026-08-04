#include "ui/common/app_preferences.h"

#include <QAbstractButton>
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFontDatabase>
#include <QGroupBox>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSettings>
#include <QTabWidget>
#include <QTextEdit>
#include <QWidget>

namespace ui {
namespace {

const QHash<QString, QString> &translations()
{
    static const QHash<QString, QString> values = {
        {QStringLiteral("AI 聊天窗口"), QStringLiteral("AI Mobile Test Studio")},
        {QStringLiteral("概览"), QStringLiteral("Overview")},
        {QStringLiteral("显示"), QStringLiteral("Display")},
        {QStringLiteral("镜像"), QStringLiteral("Mirroring")},
        {QStringLiteral("恢复"), QStringLiteral("Recovery")},
        {QStringLiteral("终端"), QStringLiteral("Terminal")},
        {QStringLiteral("设备控制"), QStringLiteral("Device Control")},
        {QStringLiteral("软件包管理器"), QStringLiteral("Package Manager")},
        {QStringLiteral("应用"), QStringLiteral("Apps")},
        {QStringLiteral("文件"), QStringLiteral("Files")},
        {QStringLiteral("进程"), QStringLiteral("Processes")},
        {QStringLiteral("性能"), QStringLiteral("Performance")},
        {QStringLiteral("布局"), QStringLiteral("Inspector")},
        {QStringLiteral("日志"), QStringLiteral("Logcat")},
        {QStringLiteral("其它"), QStringLiteral("Utilities")},
        {QStringLiteral("设置"), QStringLiteral("Settings")},
        {QStringLiteral("关于"), QStringLiteral("About")},
        {QStringLiteral("通知"), QStringLiteral("Notifications")},
        {QStringLiteral("设备中心"), QStringLiteral("Device Center")},
        {QStringLiteral("设备管理器"), QStringLiteral("Device Manager")},
        {QStringLiteral("IP 地址"), QStringLiteral("IP address")},
        {QStringLiteral("端口"), QStringLiteral("Port")},
        {QStringLiteral("连接"), QStringLiteral("Connect")},
        {QStringLiteral("配对"), QStringLiteral("Pair")},
        {QStringLiteral("无线模式"), QStringLiteral("Wireless mode")},
        {QStringLiteral("断开设备"), QStringLiteral("Disconnect device")},
        {QStringLiteral("删除离线设备"), QStringLiteral("Remove offline device")},
        {QStringLiteral("状态"), QStringLiteral("Status")},
        {QStringLiteral("在线"), QStringLiteral("Online")},
        {QStringLiteral("离线"), QStringLiteral("Offline")},
        {QStringLiteral("设备离线"), QStringLiteral("Device offline")},
        {QStringLiteral("当前主设备"), QStringLiteral("Active device")},
        {QStringLiteral("设备配对"), QStringLiteral("Pair device")},
        {QStringLiteral("配对码"), QStringLiteral("Pairing code")},
        {QStringLiteral("6 位配对码"), QStringLiteral("6-digit pairing code")},
        {QStringLiteral("外观与体验"), QStringLiteral("Appearance & Experience")},
        {QStringLiteral("个性化界面语言和动态效果"),
         QStringLiteral("Personalize the interface language and motion")},
        {QStringLiteral("语言"), QStringLiteral("Language")},
        {QStringLiteral("界面语言与字体会立即切换"),
         QStringLiteral("Interface language and font update immediately")},
        {QStringLiteral("中文"), QStringLiteral("Chinese")},
        {QStringLiteral("动态效果"), QStringLiteral("Motion")},
        {QStringLiteral("页面过渡与按钮反馈"), QStringLiteral("Page transitions and button feedback")},
        {QStringLiteral("启用自然动效"), QStringLiteral("Enable natural motion")},
        {QStringLiteral("字体"), QStringLiteral("Typeface")},
        {QStringLiteral("跟随当前语言自动选择"),
         QStringLiteral("Selected automatically for the active language")},
        {QStringLiteral("已连接"), QStringLiteral("Connected")},
        {QStringLiteral("未连接"), QStringLiteral("Disconnected")},
        {QStringLiteral("未授权"), QStringLiteral("Unauthorized")},
        {QStringLiteral("不可用"), QStringLiteral("Unavailable")},
        {QStringLiteral("侧载模式"), QStringLiteral("Sideload mode")},
        {QStringLiteral("未检测到设备"), QStringLiteral("No device detected")},
        {QStringLiteral("未连接设备"), QStringLiteral("No device connected")},
        {QStringLiteral("Android 设备"), QStringLiteral("Android device")},
        {QStringLiteral("Recovery 设备"), QStringLiteral("Recovery device")},
        {QStringLiteral("ADB 不可用"), QStringLiteral("ADB unavailable")},
        {QStringLiteral("ADB 未连接"), QStringLiteral("ADB disconnected")},
        {QStringLiteral("ADB 等待授权"), QStringLiteral("ADB awaiting authorization")},
        {QStringLiteral("ADB 连接正常"), QStringLiteral("ADB connected")},
        {QStringLiteral("Recovery 侧载已就绪"), QStringLiteral("Recovery sideload ready")},
        {QStringLiteral("scrcpy 不可用"), QStringLiteral("scrcpy unavailable")},
        {QStringLiteral("scrcpy 运行错误"), QStringLiteral("scrcpy error")},
        {QStringLiteral("设备 %1"), QStringLiteral("Device %1")},
        {QStringLiteral("刷新"), QStringLiteral("Refresh")},
        {QStringLiteral("刷新概览"), QStringLiteral("Refresh overview")},
        {QStringLiteral("刷新显示设置"), QStringLiteral("Refresh display settings")},
        {QStringLiteral("刷新应用列表"), QStringLiteral("Refresh apps")},
        {QStringLiteral("刷新软件包列表"), QStringLiteral("Refresh packages")},
        {QStringLiteral("刷新进程"), QStringLiteral("Refresh processes")},
        {QStringLiteral("打开"), QStringLiteral("Open")},
        {QStringLiteral("关闭"), QStringLiteral("Close")},
        {QStringLiteral("启动"), QStringLiteral("Start")},
        {QStringLiteral("停止"), QStringLiteral("Stop")},
        {QStringLiteral("暂停"), QStringLiteral("Pause")},
        {QStringLiteral("继续"), QStringLiteral("Resume")},
        {QStringLiteral("重置"), QStringLiteral("Reset")},
        {QStringLiteral("取消"), QStringLiteral("Cancel")},
        {QStringLiteral("确定"), QStringLiteral("Confirm")},
        {QStringLiteral("删除"), QStringLiteral("Delete")},
        {QStringLiteral("重命名"), QStringLiteral("Rename")},
        {QStringLiteral("复制"), QStringLiteral("Copy")},
        {QStringLiteral("粘贴"), QStringLiteral("Paste")},
        {QStringLiteral("上传"), QStringLiteral("Upload")},
        {QStringLiteral("下载"), QStringLiteral("Download")},
        {QStringLiteral("搜索"), QStringLiteral("Search")},
        {QStringLiteral("过滤"), QStringLiteral("Filter")},
        {QStringLiteral("全部"), QStringLiteral("All")},
        {QStringLiteral("默认"), QStringLiteral("Default")},
        {QStringLiteral("自动"), QStringLiteral("Auto")},
        {QStringLiteral("未选择"), QStringLiteral("Not selected")},
        {QStringLiteral("未查询"), QStringLiteral("Not queried")},
        {QStringLiteral("正在查询"), QStringLiteral("Querying")},
        {QStringLiteral("正在检测设备"), QStringLiteral("Detecting device")},
        {QStringLiteral("正在读取设备信息…"), QStringLiteral("Reading device information...")},
        {QStringLiteral("正在更新设备信息…"), QStringLiteral("Updating device information...")},
        {QStringLiteral("正在获取屏幕截图…"), QStringLiteral("Capturing screenshot...")},
        {QStringLiteral("正在读取进程…"), QStringLiteral("Reading processes...")},
        {QStringLiteral("执行中..."), QStringLiteral("Working...")},
        {QStringLiteral("操作完成。"), QStringLiteral("Operation completed.")},
        {QStringLiteral("设备输出"), QStringLiteral("Device output")},
        {QStringLiteral("设备信息"), QStringLiteral("Device information")},
        {QStringLiteral("系统信息"), QStringLiteral("System information")},
        {QStringLiteral("电池信息"), QStringLiteral("Battery information")},
        {QStringLiteral("内存信息"), QStringLiteral("Memory information")},
        {QStringLiteral("实时屏幕"), QStringLiteral("Live screen")},
        {QStringLiteral("分辨率"), QStringLiteral("Resolution")},
        {QStringLiteral("密度"), QStringLiteral("Density")},
        {QStringLiteral("屏幕超时"), QStringLiteral("Screen timeout")},
        {QStringLiteral("可用刷新率"), QStringLiteral("Available refresh rates")},
        {QStringLiteral("外观"), QStringLiteral("Appearance")},
        {QStringLiteral("浅色"), QStringLiteral("Light")},
        {QStringLiteral("深色"), QStringLiteral("Dark")},
        {QStringLiteral("字体缩放"), QStringLiteral("Font scale")},
        {QStringLiteral("过渡动画"), QStringLiteral("Transition animation")},
        {QStringLiteral("窗口动画缩放"), QStringLiteral("Window animation scale")},
        {QStringLiteral("过渡动画缩放"), QStringLiteral("Transition animation scale")},
        {QStringLiteral("动画时长缩放"), QStringLiteral("Animator duration scale")},
        {QStringLiteral("应用或软件包名"), QStringLiteral("App or package name")},
        {QStringLiteral("搜索应用名称或软件包"), QStringLiteral("Search apps or packages")},
        {QStringLiteral("搜索软件包名称"), QStringLiteral("Search package name")},
        {QStringLiteral("仅显示应用"), QStringLiteral("Apps only")},
        {QStringLiteral("进程名称"), QStringLiteral("Process name")},
        {QStringLiteral("CPU 时间"), QStringLiteral("CPU time")},
        {QStringLiteral("内存"), QStringLiteral("Memory")},
        {QStringLiteral("用户"), QStringLiteral("User")},
        {QStringLiteral("名称"), QStringLiteral("Name")},
        {QStringLiteral("大小"), QStringLiteral("Size")},
        {QStringLiteral("类型"), QStringLiteral("Type")},
        {QStringLiteral("路径"), QStringLiteral("Path")},
        {QStringLiteral("操作"), QStringLiteral("Actions")},
        {QStringLiteral("欢迎使用 AI 助手"), QStringLiteral("Welcome to AI Assistant")},
        {QStringLiteral("发送"), QStringLiteral("Send")},
        {QStringLiteral("清空"), QStringLiteral("Clear")},
        {QStringLiteral("历史"), QStringLiteral("History")},
        {QStringLiteral("展开"), QStringLiteral("Expand")},
        {QStringLiteral("添加附件"), QStringLiteral("Add attachment")},
        {QStringLiteral("选择终端类型"), QStringLiteral("Choose terminal type")},
        {QStringLiteral("新建终端"), QStringLiteral("New terminal")},
        {QStringLiteral("关闭终端"), QStringLiteral("Close terminal")},
        {QStringLiteral("重置终端"), QStringLiteral("Reset terminal")},
        {QStringLiteral("快捷命令"), QStringLiteral("Quick commands")},
        {QStringLiteral("连接 Android 设备后可使用终端"),
         QStringLiteral("Connect an Android device to use the terminal")},
        {QStringLiteral("连接设备后可执行命令"),
         QStringLiteral("Connect a device to run commands")},
        {QStringLiteral("连接设备后可浏览文件"),
         QStringLiteral("Connect a device to browse files")},
        {QStringLiteral("连接设备后可管理应用"),
         QStringLiteral("Connect a device to manage apps")},
        {QStringLiteral("连接设备后可读取软件包列表"),
         QStringLiteral("Connect a device to read packages")},
        {QStringLiteral("设备主页"), QStringLiteral("Device Home")},
        {QStringLiteral("电池"), QStringLiteral("Battery")},
        {QStringLiteral("存储"), QStringLiteral("Storage")},
        {QStringLiteral("运行时间"), QStringLiteral("Uptime")},
        {QStringLiteral("制造商"), QStringLiteral("Manufacturer")},
        {QStringLiteral("型号"), QStringLiteral("Model")},
        {QStringLiteral("品牌"), QStringLiteral("Brand")},
        {QStringLiteral("序列号"), QStringLiteral("Serial number")},
        {QStringLiteral("代号"), QStringLiteral("Codename")},
        {QStringLiteral("内核"), QStringLiteral("Kernel")},
        {QStringLiteral("架构"), QStringLiteral("Architecture")},
        {QStringLiteral("实时"), QStringLiteral("Live")},
        {QStringLiteral("启动 Shizuku"), QStringLiteral("Start Shizuku")},
        {QStringLiteral("切换设备电源状态"), QStringLiteral("Toggle device power")},
        {QStringLiteral("设备信息已更新"), QStringLiteral("Device information updated")},
        {QStringLiteral("显示设置已同步"), QStringLiteral("Display settings synchronized")},
        {QStringLiteral("物理分辨率"), QStringLiteral("Physical resolution")},
        {QStringLiteral("物理密度"), QStringLiteral("Physical density")},
        {QStringLiteral("最小宽度"), QStringLiteral("Smallest width")},
        {QStringLiteral("快速"), QStringLiteral("Fast")},
        {QStringLiteral("慢速"), QStringLiteral("Slow")},
        {QStringLiteral("主屏幕"), QStringLiteral("Main display")},
        {QStringLiteral("虚拟屏幕"), QStringLiteral("Virtual display")},
        {QStringLiteral("摄像头"), QStringLiteral("Camera")},
        {QStringLiteral("图像"), QStringLiteral("Video")},
        {QStringLiteral("录制"), QStringLiteral("Recording")},
        {QStringLiteral("输入与声音"), QStringLiteral("Input & Sound")},
        {QStringLiteral("高级参数"), QStringLiteral("Advanced options")},
        {QStringLiteral("全屏"), QStringLiteral("Fullscreen")},
        {QStringLiteral("音频"), QStringLiteral("Audio")},
        {QStringLiteral("麦克风"), QStringLiteral("Microphone")},
        {QStringLiteral("鼠标"), QStringLiteral("Mouse")},
        {QStringLiteral("键盘"), QStringLiteral("Keyboard")},
        {QStringLiteral("最大 FPS"), QStringLiteral("Maximum FPS")},
        {QStringLiteral("最大尺寸 (px)"), QStringLiteral("Maximum size (px)")},
        {QStringLiteral("摄像头 ID"), QStringLiteral("Camera ID")},
        {QStringLiteral("刷新摄像头列表"), QStringLiteral("Refresh cameras")},
        {QStringLiteral("录制当前画面"), QStringLiteral("Record current display")},
        {QStringLiteral("停止镜像"), QStringLiteral("Stop mirroring")},
        {QStringLiteral("软件包列表"), QStringLiteral("Package list")},
        {QStringLiteral("软件包详情"), QStringLiteral("Package details")},
        {QStringLiteral("包名"), QStringLiteral("Package name")},
        {QStringLiteral("版本"), QStringLiteral("Version")},
        {QStringLiteral("安装时间"), QStringLiteral("Installed")},
        {QStringLiteral("更新时间"), QStringLiteral("Updated")},
        {QStringLiteral("安装来源"), QStringLiteral("Install source")},
        {QStringLiteral("APK 路径"), QStringLiteral("APK path")},
        {QStringLiteral("APK 大小"), QStringLiteral("APK size")},
        {QStringLiteral("用户应用"), QStringLiteral("User apps")},
        {QStringLiteral("系统应用"), QStringLiteral("System apps")},
        {QStringLiteral("第三方"), QStringLiteral("Third-party")},
        {QStringLiteral("仅启用"), QStringLiteral("Enabled only")},
        {QStringLiteral("仅停用"), QStringLiteral("Disabled only")},
        {QStringLiteral("启用"), QStringLiteral("Enable")},
        {QStringLiteral("停用"), QStringLiteral("Disable")},
        {QStringLiteral("卸载"), QStringLiteral("Uninstall")},
        {QStringLiteral("清除数据"), QStringLiteral("Clear data")},
        {QStringLiteral("停止应用"), QStringLiteral("Stop app")},
        {QStringLiteral("启动应用"), QStringLiteral("Launch app")},
        {QStringLiteral("安装应用"), QStringLiteral("Install apps")},
        {QStringLiteral("安装 APK"), QStringLiteral("Install APK")},
        {QStringLiteral("导出 APK"), QStringLiteral("Export APK")},
        {QStringLiteral("恢复安装"), QStringLiteral("Restore app")},
        {QStringLiteral("权限"), QStringLiteral("Permissions")},
        {QStringLiteral("后台模式"), QStringLiteral("Background mode")},
        {QStringLiteral("不受限制"), QStringLiteral("Unrestricted")},
        {QStringLiteral("受限制"), QStringLiteral("Restricted")},
        {QStringLiteral("已优化"), QStringLiteral("Optimized")},
        {QStringLiteral("选择一个应用查看详情"), QStringLiteral("Select an app to view details")},
        {QStringLiteral("文件资源管理器"), QStringLiteral("File Explorer")},
        {QStringLiteral("设备驱动器"), QStringLiteral("Device drives")},
        {QStringLiteral("内部存储"), QStringLiteral("Internal storage")},
        {QStringLiteral("系统文件与分区"), QStringLiteral("System files & partitions")},
        {QStringLiteral("上一级"), QStringLiteral("Up")},
        {QStringLiteral("后退"), QStringLiteral("Back")},
        {QStringLiteral("前进"), QStringLiteral("Forward")},
        {QStringLiteral("新建文件夹"), QStringLiteral("New folder")},
        {QStringLiteral("列表视图"), QStringLiteral("List view")},
        {QStringLiteral("网格视图"), QStringLiteral("Grid view")},
        {QStringLiteral("标准视图"), QStringLiteral("Standard view")},
        {QStringLiteral("紧凑视图"), QStringLiteral("Compact view")},
        {QStringLiteral("根目录"), QStringLiteral("Root")},
        {QStringLiteral("容量信息不可用"), QStringLiteral("Capacity information unavailable")},
        {QStringLiteral("选择单个项目来获取更多信息。"),
         QStringLiteral("Select one item for more information.")},
        {QStringLiteral("打开 Recovery 侧载"), QStringLiteral("Open Recovery sideload")},
        {QStringLiteral("返回 Recovery"), QStringLiteral("Back to Recovery")},
        {QStringLiteral("选择 Recovery ZIP 包"), QStringLiteral("Choose Recovery ZIP")},
        {QStringLiteral("开始侧载"), QStringLiteral("Start sideload")},
        {QStringLiteral("正在侧载，请勿断开设备"),
         QStringLiteral("Sideloading, keep the device connected")},
        {QStringLiteral("等待设备连接"), QStringLiteral("Waiting for device")},
        {QStringLiteral("等待设备状态"), QStringLiteral("Waiting for device status")},
        {QStringLiteral("确认开始 Recovery 侧载"), QStringLiteral("Confirm Recovery sideload")},
        {QStringLiteral("CPU"), QStringLiteral("CPU")},
        {QStringLiteral("FPS 系统桌面"), QStringLiteral("FPS system launcher")},
        {QStringLiteral("实时采样 · %1"), QStringLiteral("Live sampling · %1")},
        {QStringLiteral("正在采样 · %1"), QStringLiteral("Sampling · %1")},
        {QStringLiteral("日志流"), QStringLiteral("Log stream")},
        {QStringLiteral("重新启动日志流"), QStringLiteral("Restart log stream")},
        {QStringLiteral("清空日志"), QStringLiteral("Clear logs")},
        {QStringLiteral("保存日志"), QStringLiteral("Save logs")},
        {QStringLiteral("滚动到底部"), QStringLiteral("Scroll to bottom")},
        {QStringLiteral("连接 Android 设备后开始读取日志"),
         QStringLiteral("Connect an Android device to stream logs")},
        {QStringLiteral("自定义命令"), QStringLiteral("Custom command")},
        {QStringLiteral("运行自定义命令"), QStringLiteral("Run custom command")},
        {QStringLiteral("ADB Shell 命令"), QStringLiteral("ADB Shell command")},
        {QStringLiteral("直接在当前设备上执行 ADB Shell 命令"),
         QStringLiteral("Run an ADB Shell command on the current device")},
        {QStringLiteral("输入 ADB Shell 命令"), QStringLiteral("Enter an ADB Shell command")},
        {QStringLiteral("命令输出会显示在这里"), QStringLiteral("Command output appears here")},
        {QStringLiteral("执行命令"), QStringLiteral("Run command")},
        {QStringLiteral("执行"), QStringLiteral("Run")},
        {QStringLiteral("账户"), QStringLiteral("Accounts")},
        {QStringLiteral("读取账户"), QStringLiteral("Read accounts")},
        {QStringLiteral("查看 Android 用户与已注册账户"),
         QStringLiteral("View Android users and registered accounts")},
        {QStringLiteral("正在等待账户信息"), QStringLiteral("Waiting for account information")},
        {QStringLiteral("去除叹号"), QStringLiteral("Network validation")},
        {QStringLiteral("修改网络可用性验证服务器"),
         QStringLiteral("Change the network validation server")},
        {QStringLiteral("Android 版本"), QStringLiteral("Android version")},
        {QStringLiteral("验证服务器"), QStringLiteral("Validation server")},
        {QStringLiteral("当前服务器"), QStringLiteral("Current server")},
        {QStringLiteral("系统默认"), QStringLiteral("System default")},
        {QStringLiteral("应用设置"), QStringLiteral("Apply")},
        {QStringLiteral("恢复默认"), QStringLiteral("Restore defaults")},
        {QStringLiteral("读取网络验证配置"), QStringLiteral("Read network validation settings")},
        {QStringLiteral("修改网络验证服务器"), QStringLiteral("Change validation server")},
        {QStringLiteral("恢复网络验证服务器"), QStringLiteral("Restore validation server")},
        {QStringLiteral("修改动画缩放"), QStringLiteral("Change animation scales")},
        {QStringLiteral("调整 Android 系统动画缩放"),
         QStringLiteral("Adjust Android system animation scales")},
        {QStringLiteral("动画时长"), QStringLiteral("Animator duration")},
        {QStringLiteral("窗口动画"), QStringLiteral("Window animation")},
        {QStringLiteral("当前：--"), QStringLiteral("Current: --")},
        {QStringLiteral("读取动画缩放"), QStringLiteral("Read animation scales")},
        {QStringLiteral("状态栏与导航栏"), QStringLiteral("System bars")},
        {QStringLiteral("隐藏状态图标或启用全局沉浸模式"),
         QStringLiteral("Hide status icons or enable immersive mode")},
        {QStringLiteral("隐藏状态栏图标"), QStringLiteral("Hide status icons")},
        {QStringLiteral("静音 / 震动"), QStringLiteral("Silent / vibrate")},
        {QStringLiteral("定位"), QStringLiteral("Location")},
        {QStringLiteral("录屏状态"), QStringLiteral("Screen recording")},
        {QStringLiteral("热点"), QStringLiteral("Hotspot")},
        {QStringLiteral("飞行模式"), QStringLiteral("Airplane mode")},
        {QStringLiteral("耳机"), QStringLiteral("Headset")},
        {QStringLiteral("闹钟"), QStringLiteral("Alarm")},
        {QStringLiteral("蓝牙"), QStringLiteral("Bluetooth")},
        {QStringLiteral("隐藏所选"), QStringLiteral("Hide selected")},
        {QStringLiteral("重置图标"), QStringLiteral("Reset icons")},
        {QStringLiteral("重置状态栏图标"), QStringLiteral("Reset status icons")},
        {QStringLiteral("全局隐藏"), QStringLiteral("Global immersive mode")},
        {QStringLiteral("隐藏状态栏"), QStringLiteral("Hide status bar")},
        {QStringLiteral("隐藏导航栏"), QStringLiteral("Hide navigation bar")},
        {QStringLiteral("同时隐藏"), QStringLiteral("Hide both")},
        {QStringLiteral("恢复状态栏与导航栏"), QStringLiteral("Restore system bars")},
        {QStringLiteral("分级调节震动强度"), QStringLiteral("Vibration intensity")},
        {QStringLiteral("不同设备与 ROM 支持的设置项可能不同"),
         QStringLiteral("Available settings vary by device and ROM")},
        {QStringLiteral("震动总开关"), QStringLiteral("Master vibration")},
        {QStringLiteral("触感震动开关"), QStringLiteral("Haptic feedback")},
        {QStringLiteral("响铃震动开关"), QStringLiteral("Ring vibration")},
        {QStringLiteral("充电震动开关"), QStringLiteral("Charging vibration")},
        {QStringLiteral("触感震动强度"), QStringLiteral("Haptic intensity")},
        {QStringLiteral("硬件触感震动强度"), QStringLiteral("Hardware haptic intensity")},
        {QStringLiteral("通知震动强度"), QStringLiteral("Notification intensity")},
        {QStringLiteral("闹钟震动强度"), QStringLiteral("Alarm intensity")},
        {QStringLiteral("媒体震动强度"), QStringLiteral("Media intensity")},
        {QStringLiteral("铃声震动强度"), QStringLiteral("Ringtone intensity")},
        {QStringLiteral("读取震动设置"), QStringLiteral("Read vibration settings")},
        {QStringLiteral("震动类型"), QStringLiteral("Vibration type")},
        {QStringLiteral("修改震动强度"), QStringLiteral("Change vibration intensity")},
        {QStringLiteral("设备连接已断开"), QStringLiteral("Device disconnected")},
        {QStringLiteral("等待中"), QStringLiteral("Waiting")},
        {QStringLiteral("正常"), QStringLiteral("Healthy")},
        {QStringLiteral("未知"), QStringLiteral("Unknown")}
    };
    return values;
}

QString loadFontFamily(const QString &path)
{
    const int fontId = QFontDatabase::addApplicationFont(path);
    if (fontId < 0) {
        return {};
    }
    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    return families.isEmpty() ? QString() : families.first();
}

QString loadFontDirectory(const QString &path, const QString &preferredFile)
{
    const QDir directory(path);
    const QStringList files = directory.entryList({QStringLiteral("*.ttf"),
                                                   QStringLiteral("*.otf")},
                                                  QDir::Files | QDir::Readable,
                                                  QDir::Name);
    QString fallbackFamily;
    QString preferredFamily;
    for (const QString &file : files) {
        const QString family = loadFontFamily(directory.filePath(file));
        if (fallbackFamily.isEmpty() && !family.isEmpty()) {
            fallbackFamily = family;
        }
        if (file.compare(preferredFile, Qt::CaseInsensitive) == 0) {
            preferredFamily = family;
        }
    }
    return preferredFamily.isEmpty() ? fallbackFamily : preferredFamily;
}

QString localizedControlText(const AppPreferences &preferences, const QString &value)
{
    if (value.isEmpty()) {
        return value;
    }
    return preferences.translateExistingText(value);
}

} // namespace

AppPreferences &AppPreferences::instance()
{
    static AppPreferences preferences;
    return preferences;
}

AppPreferences::AppPreferences(QObject *parent)
    : QObject(parent)
{
}

void AppPreferences::initialize()
{
    if (m_initialized) {
        return;
    }
    m_initialized = true;

    QSettings settings(QStringLiteral("AI Mobile Test Studio"),
                       QStringLiteral("AI Mobile Test Studio"));
    m_language = settings.value(QStringLiteral("appearance/language"),
                                QStringLiteral("zh-CN"))
                         .toString()
                         .compare(QStringLiteral("en-US"), Qt::CaseInsensitive)
            == 0
        ? AppLanguage::English
        : AppLanguage::Chinese;
    m_motionEnabled = settings.value(QStringLiteral("appearance/motionEnabled"), true).toBool();
    loadFonts();
    applyFont();
}

AppLanguage AppPreferences::language() const
{
    return m_language;
}

void AppPreferences::setLanguage(AppLanguage language)
{
    if (m_language == language) {
        return;
    }
    m_language = language;
    QSettings settings(QStringLiteral("AI Mobile Test Studio"),
                       QStringLiteral("AI Mobile Test Studio"));
    settings.setValue(QStringLiteral("appearance/language"),
                      language == AppLanguage::Chinese ? QStringLiteral("zh-CN")
                                                       : QStringLiteral("en-US"));
    applyFont();
    emit languageChanged(language);
}

bool AppPreferences::motionEnabled() const
{
    return m_motionEnabled;
}

void AppPreferences::setMotionEnabled(bool enabled)
{
    if (m_motionEnabled == enabled) {
        return;
    }
    m_motionEnabled = enabled;
    QSettings settings(QStringLiteral("AI Mobile Test Studio"),
                       QStringLiteral("AI Mobile Test Studio"));
    settings.setValue(QStringLiteral("appearance/motionEnabled"), enabled);
    emit motionEnabledChanged(enabled);
}

QString AppPreferences::fontFamily() const
{
    const QString configured = m_language == AppLanguage::Chinese ? m_chineseFontFamily
                                                                   : m_englishFontFamily;
    if (!configured.isEmpty()) {
        return configured;
    }
    return m_language == AppLanguage::Chinese ? QStringLiteral("Microsoft YaHei UI")
                                               : QStringLiteral("Segoe UI");
}

QString AppPreferences::translate(const char *source) const
{
    return translatedValue(QString::fromUtf8(source));
}

QString AppPreferences::translatedValue(const QString &source) const
{
    if (m_language == AppLanguage::Chinese) {
        return source;
    }
    return translations().value(source, source);
}

QString AppPreferences::translateExistingText(const QString &value) const
{
    const auto &values = translations();
    for (auto iterator = values.cbegin(); iterator != values.cend(); ++iterator) {
        const QString &source = iterator.key();
        const QString &english = iterator.value();
        if (value == source || value == english) {
            return m_language == AppLanguage::Chinese ? source : english;
        }
    }

    const qsizetype spacing = value.lastIndexOf(QStringLiteral("  "));
    if (spacing >= 0) {
        const QString prefix = value.left(spacing + 2);
        const QString suffix = value.mid(spacing + 2).trimmed();
        for (auto iterator = values.cbegin(); iterator != values.cend(); ++iterator) {
            if (suffix == iterator.key() || suffix == iterator.value()) {
                return prefix + (m_language == AppLanguage::Chinese ? iterator.key()
                                                                    : iterator.value());
            }
        }
    }
    return value;
}

void AppPreferences::applyFont(QWidget *root) const
{
    QFont applicationFont = qApp->font();
    applicationFont.setFamily(fontFamily());
    applicationFont.setStyleHint(m_language == AppLanguage::English ? QFont::Monospace
                                                                    : QFont::SansSerif);
    qApp->setFont(applicationFont);

    if (root == nullptr) {
        return;
    }
    QList<QWidget *> widgets = {root};
    widgets.append(root->findChildren<QWidget *>());
    for (QWidget *widget : widgets) {
        QFont font = widget->font();
        font.setFamily(fontFamily());
        font.setStyleHint(m_language == AppLanguage::English ? QFont::Monospace
                                                             : QFont::SansSerif);
        widget->setFont(font);
    }
}

void AppPreferences::retranslate(QWidget *root) const
{
    if (root == nullptr) {
        return;
    }

    const QList<QWidget *> widgets = root->findChildren<QWidget *>();
    for (QWidget *widget : widgets) {
        if (auto *label = qobject_cast<QLabel *>(widget)) {
            label->setText(localizedControlText(*this, label->text()));
        } else if (auto *button = qobject_cast<QAbstractButton *>(widget)) {
            button->setText(localizedControlText(*this, button->text()));
        } else if (auto *group = qobject_cast<QGroupBox *>(widget)) {
            group->setTitle(localizedControlText(*this, group->title()));
        }

        widget->setToolTip(localizedControlText(*this, widget->toolTip()));
        widget->setStatusTip(localizedControlText(*this, widget->statusTip()));
        widget->setWhatsThis(localizedControlText(*this, widget->whatsThis()));

        if (auto *lineEdit = qobject_cast<QLineEdit *>(widget)) {
            lineEdit->setPlaceholderText(
                localizedControlText(*this, lineEdit->placeholderText()));
        } else if (auto *plainTextEdit = qobject_cast<QPlainTextEdit *>(widget)) {
            plainTextEdit->setPlaceholderText(
                localizedControlText(*this, plainTextEdit->placeholderText()));
        } else if (auto *textEdit = qobject_cast<QTextEdit *>(widget)) {
            textEdit->setPlaceholderText(
                localizedControlText(*this, textEdit->placeholderText()));
        } else if (auto *combo = qobject_cast<QComboBox *>(widget)) {
            for (int index = 0; index < combo->count(); ++index) {
                combo->setItemText(index,
                                   localizedControlText(*this, combo->itemText(index)));
            }
        } else if (auto *tabs = qobject_cast<QTabWidget *>(widget)) {
            for (int index = 0; index < tabs->count(); ++index) {
                tabs->setTabText(index,
                                 localizedControlText(*this, tabs->tabText(index)));
                tabs->setTabToolTip(index,
                                    localizedControlText(*this, tabs->tabToolTip(index)));
            }
        }
    }
    applyFont(root);
}

void AppPreferences::loadFonts()
{
    const QDir runtime(QCoreApplication::applicationDirPath());
    const QString chineseRegular = runtime.filePath(
        QStringLiteral("runtime/fonts/cn/LXGWWenKai-Regular.ttf"));
    const QString chineseMedium = runtime.filePath(
        QStringLiteral("runtime/fonts/cn/LXGWWenKai-Medium.ttf"));
    const QString englishDirectory = runtime.filePath(
        QStringLiteral("runtime/fonts/us/JetBrainsMono-2.304/fonts/ttf"));

    m_chineseFontFamily = loadFontFamily(chineseRegular);
    const QString chineseMediumFamily = loadFontFamily(chineseMedium);
    if (m_chineseFontFamily.isEmpty()) {
        m_chineseFontFamily = chineseMediumFamily;
    }

    m_englishFontFamily = loadFontDirectory(englishDirectory,
                                            QStringLiteral("JetBrainsMono-Regular.ttf"));
}

} // namespace ui
