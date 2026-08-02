# 更新日志

本文件记录 AI Mobile Test Studio 的重要变更。

格式参考 Keep a Changelog，版本号遵循语义化版本。

## [Unreleased]

### Added

- 应用启动后预热 Appium Inspector 和默认 OpenCode 会话；默认 OpenCode 标签在后台消费初始输出，首次进入“布局”和“终端”工作区可直接展示已初始化内容。
- 授权设备连接后后台预取应用名称/图标、进程快照以及 `/`、`/sdcard`，并按设备序列号保存本次运行内存缓存；手动刷新和文件变更仍读取最新目录。
- 新增 Apple 风格前端设计规范，统一浅色材质、圆角、阴影、响应式布局、页面过渡和按钮自然动效。
- 新增设置工作区，支持中英文即时切换、语言字体自动加载和动态效果偏好持久化。
- 左侧导航新增“其它”工作区，参考 QtAdb 提供自定义 ADB Shell、账户查询、网络验证服务器、动画缩放、系统栏和震动强度工具目录。
- 新增 `OtherService`，异步执行其它工具页提交的真实设备命令，并统一回传执行状态与输出。
- 左侧导航新增“进程”工作区，参考 AYA 展示进程名称、CPU、CPU 时间、内存、PID 和用户，支持排序、文本过滤、仅显示应用及强制停止选中应用。
- 新增 `ProcessService`，异步缓存设备软件包并每 8 秒采样 `top`，离开进程页后自动停止采集，同时兼容不支持结构化列参数的旧版 `top`。
- 左侧导航新增 `Display` 工作区，参考 ADB-App 提供物理显示摘要、推荐分辨率、尺寸/密度/超时设置、刷新率、浅色/深色模式、字体缩放和动画速度控制。
- 新增 `DisplayService`，通过异步 ADB 查询和串行动作队列读写真实设备显示设置，并在写入成功后自动回读状态。
- 左侧导航新增 `Mirroring` 工作区，参考 ADB-App 提供主屏幕、虚拟屏幕和摄像头三种 scrcpy 来源，以及图像、录制、输入、音频、启动应用和高级参数配置。
- `ScrcpyService` 新增结构化参数启动、摄像头查询、录制目录创建和设备切换取消处理；`Mirroring` 与顶部镜像按钮共享运行状态。
- 左侧导航新增“布局”工作区，直接嵌入 Appium Inspector 2026.5.1 官方浏览器前端，提供 Appium Server、云提供商、Capability Builder、Saved Capability Sets、Attach to Session、Source、Commands、Gestures、Recorder 和 Session Information 等完整工作区功能。
- 新增随包 Appium Inspector 静态运行资源，包含官方脚本、样式、语言包、云提供商图标、Apache-2.0 许可证和来源说明；应用启动时不联网下载界面资源。
- “布局”工作区新增 WebEngine 文件下载和外部链接处理：截图、源码及会话文件可通过本机保存对话框导出，Capabilities Documentation 等外部链接交由系统浏览器打开。
- 缺少 Qt WebEngineWidgets 时保留 Qt Widgets 原生布局检查降级页，支持能力编辑、JSON 导入导出、Appium 会话创建、Source、Commands、Gestures、Recorder 和 Session Information 基础操作。
- 新增统一 `TerminalSession` 契约，将 ADB shell 与 OpenCode 会话接入同一 `TerminalService`，设备切换只回收 ADB 标签。
- 终端“+”新建菜单支持 OpenCode 与 ADB Shell，标签按类型命名，OpenCode 在无设备时仍可创建。
- 新增 `node-pty` ConPTY 终端宿主和二进制帧协议，支持 OpenCode 输入、输出、resize、停止和错误回传。
- 新增本地 xterm.js 6.0.0、FitAddon 0.11.0、Qt WebChannel 桥接、离线 CSP、复制粘贴和暗色终端主题；缺少 WebEngine 时保留基础显示降级。
- 新增 `tools/runtime/runtime-lock.json`，锁定 Windows x64 OpenCode 1.18.5、Node.js 24.18.0 和 `node-pty` 1.1.0 的版本、官方来源、SHA-256 与许可证信息。
- 新增 `stage-terminal-runtime.ps1`，在构建阶段下载、缓存、校验并原子 staging 终端运行时；应用启动阶段不联网下载依赖。
- 新增构建产物 `runtime/manifest.json`，记录终端组件版本、相对路径、归档校验值和来源。
- 新增 `opencode_conpty_smoke`，通过 Qt `ConPtySession`、随包 Node.js 和 `node-pty` 启动真实 `opencode.exe --version`；原有 `conpty_session_smoke` 继续覆盖输入、输出、resize 和退出。
- 新增 ADB 持久终端工作区，支持 `shell,v2`、legacy 回退、多标签、输入输出、窗口尺寸同步、重置、复制粘贴和快捷命令。
- 新增 `TerminalService`，直接通过 ADB server transport 管理独立设备 shell 会话，并在设备断开或切换时统一回收。
- 新增终端与 OpenCode 集成专项设计，确定未来采用 xterm.js、Qt WebEngine/QWebChannel、Windows ConPTY 和 OpenCode Server/SDK。
- 新增便携运行时与无外部安装分发规范，覆盖 runtime manifest、版本锁定、SHA-256、私有 ADB server、OpenCode 隔离、许可证门禁和干净机验收。
- 新增 docs 导航和“已实现、进行中、规划中、决策”状态约定。
- 左侧导航新增“性能”工作区，实时展示设备开机时长、电池电量、电压与温度、CPU 总占用、各核心频率与占用、内存占用及前台应用 FPS。
- 新增 `PerformanceService`，通过异步 ADB 采样读取 `/proc/stat`、CPU 频率、`/proc/meminfo`、thermalservice、battery 和 SurfaceFlinger 数据，离开性能页后自动停止采集。
- 新增时间驱动的性能曲线控件，支持 CPU、各核心、内存和 FPS 历史数据的四列布局与连续滚动显示。
- 左侧导航新增“应用”工作区，支持按用户、系统、停用和已卸载状态筛选并搜索设备应用。
- 新增 `AppsService`，支持应用详情、启动/停止、启停用、清除数据、卸载/恢复、权限与后台模式管理、APK 安装和导出。
- 应用列表和详情面板支持从连接设备批量加载真实应用名称与 PNG 图标。
- 新增 Android 端应用元数据提取器及可复现构建源码，通过 `app_process` 批量读取本地化名称和应用图标。
- CMake 构建与安装流程自动将应用元数据提取器部署到桌面端 `runtime/android/` 目录。
- APK 多选安装改为逐个排队执行，并在批次结束后统一刷新应用列表和汇总结果。
- 参考 QtAdb 补充关机、亮度、休眠、唤醒、语音助手和系统应用快捷键。
- 设备控制页新增“系统快捷控制”和“系统应用”分类。
- 左侧导航新增“设备控制”，支持在对话和设备控制工作区之间切换。
- 新增 11 个可折叠设备命令分类，覆盖电源及 Word 文档中的全部 Android KEYCODE 类别。
- 新增 `AdbControlService`，支持 KEYCODE 命令队列、执行状态和错误反馈。
- 电源重启命令增加二次确认。
- 新增 `ScrcpyService`，支持真实 ADB 设备探测、scrcpy 启动、停止和异常反馈。
- 顶部工具栏新增“启动镜像”按钮，仅在设备已连接时启用。
- 新增项目目录结构与依赖规则文档。
- 新增 Qt 桌面端、Python 自动化服务、共享协议、运行时和测试目录边界。
- 新增根目录 `README.md` 和 `.gitignore`。
- 新增项目文档目录 `docs/`。
- 新增项目设计文档。
- 新增系统架构文档。
- 新增开发规范文档。
- 新增 C++/Python 编码规范。
- 新增开发路线图。
- 明确 Qt6 + Python + Appium + opencode 的总体技术方向。
- 明确四类核心 Agent：脚本生成、UI 理解、错误修复、测试报告。
- 明确测试报告需要包含成功率、失败原因、截图、日志、耗时和建议。
- 明确附件回填目标。

### Changed

- 移除主工作区顶部设备与快捷操作栏，将设备选择和连接状态控件迁移到侧边栏底部；镜像启动与停止继续由 `Mirroring` 工作区提供。
- “概览”主工作区参考 ADB-App Home 重建为设备标题、电池/RAM/存储指标、设备属性卡片和实时屏幕截图布局，并补充电源切换与 Shizuku 启动入口。
- 侧边栏 `Files` 更名为“文件”；文件主工作区参考 ADB Explorer 重建双层资源管理器工具栏、设备驱动器主页、文件列表/网格和右侧详情面板，同时保留原有真实文件操作能力。
- CMake 构建、安装和增量链接依赖新增 `resources/appium-inspector/`，构建后自动复制到应用旁的 `runtime/appium-inspector/`；完整 Inspector 推荐使用包含 WebEngineWidgets 的 MSVC Qt 套件。
- 更新开发与便携运行时文档，补充 Appium Inspector 资源版本、MSVC WebEngine 构建方式、运行目录和浏览器版连接 Appium Server 所需的 `--allow-cors` 配置。
- Windows 构建默认自动 staging 锁定的 OpenCode 终端运行时；仍支持同时使用 `AI_MOBILE_TEST_OPENCODE_EXECUTABLE`、`AI_MOBILE_TEST_NODE_EXECUTABLE` 和 `AI_MOBILE_TEST_NODE_PTY_MODULE` 覆盖，禁止只覆盖其中一部分造成版本混用。
- Windows 主程序链接后默认执行 `windeployqt`，将当前 Debug/Release 配置对应的 Qt DLL、平台插件、MSVC runtime、`QtWebEngineProcess`、Chromium resources 和 locales 部署到可执行文件旁。
- xterm.js、ConPTY 宿主脚本和应用元数据 JAR 现在作为链接依赖参与增量构建；资源变化会触发重新复制，不再继续使用构建目录中的旧静态文件。
- 终端后端从页面内嵌 ADB 专用逻辑改为独立 ADB/OpenCode 会话实现；OpenCode 子进程由 `node-pty` 放入真实 Windows ConPTY，Qt `QProcess` 只承载宿主帧协议。
- 首页侧栏入口由“对话”更名为“终端”，原静态聊天占位主工作区替换为真实 ADB 终端。
- CMake 增加 Qt Network 依赖，用于 ADB server socket 会话。
- 全量校准 `docs/`：明确当前 Qt 设备工作台与规划中的 Python/Appium/OpenCode 能力，更新目录、架构、开发规范和实施顺序。
- 正式分发目标调整为构建期装配完整运行时、终端用户不下载工程工具；发布构建禁止静默使用系统 PATH 或开发机绝对路径。
- 性能指标计算与曲线绘制参考 AYA：CPU 使用两次 `/proc/stat` 差分并取各核心平均值，内存优先使用 `MemAvailable`，FPS 优先使用 SurfaceFlinger `flips` 增量并回退到前台应用图层 latency。
- 性能曲线改为每 500ms 记录指标并以 16ms 动画时钟按单调时间连续向左移动，同时对齐 AYA 的整数化输入、主题色、网格、填充、边框和半像素绘制方式。
- 性能工作区根据滚动视口动态计算宽度与四列核心面板尺寸，在最大化窗口和高 DPI 屏幕下使用完整可用空间。
- 应用元数据按批次异步加载，并按当前连接设备在内存中缓存名称与图标；切换设备时自动清理缓存。
- 启动 Android 应用前先唤醒设备屏幕，再执行 Launcher 启动命令。
- 重新整理设备按键分类，拆分系统导航、实体按键、电话、控制与编辑、显示与休眠、修饰与组合、电视与遥控等 16 类。
- 移除含义模糊的“待查按键”分类，将其中按键迁移到明确场景，并增加重复 KEYCODE 调试断言。
- 删除前端自绘手机模拟器，改用 scrcpy 4.0 的独立镜像窗口。
- 设备选择器和侧边栏 ADB 状态改为真实设备状态。
- 将 Qt 桌面端源码从仓库根目录迁移到 `apps/desktop/`。
- 将根 CMake 配置改为子项目组合模式。
- 将构建产物、本地运行时和任务工作区排除出版本控制。

### Fixed

- 修复启动预热期间将整个主工作区切换为 `StackAll`，造成 Appium Inspector 与多个页面同时显示、标题和内容重叠的问题；预热改为在当前页面快照下依次激活布局和终端，完成后恢复用户页面，侧边栏操作会立即取消预热。
- 修复设备概览将 Android UTF-8 属性输出按 Windows 本地编码分块解码，导致中文营销名称乱码的问题；ADB 文本现在先完整收集字节，再统一解码。
- 修复中文模式使用比例字体作为 xterm 主字体导致终端字符横向拉伸的问题；终端改为 JetBrains Mono 等宽主字体和霞鹜文楷 CJK 回退，并在字体就绪后重新适配行列。
- 修复 OpenCode 中文输入法提交后文字延迟显示的问题；中文、emoji 等 Unicode 输入绕过 WebEngine 定时合并并即时发送，ASCII 连续输入仍以 4ms 窗口有序合并，控制序列继续即时发送。
- 修复首次启动预热 OpenCode 后终端标签栏仍保持单标签宽度、导致默认 ADB Shell 被滚动隐藏的问题；标签栏现在会占用可用工具栏宽度，超过上限后才启用滚动。
- 修复设备切换时被终止的应用、进程或目录查询可能迟到并污染新设备会话状态的问题。
- 修复进程列表刷新和应用图标加载导致的滚动卡顿：模型改为按 PID 差量增删改，应用元数据小批次提取，PNG 图标按 8ms 间隔逐个解码并只刷新命中行；滚动期间暂存最新采样，停止滚动 180ms 后再合并。
- 修复文件工作区“根目录”和“内部存储”卡片点击无响应的问题；驱动器入口改为整卡按钮，分别进入 `/` 与 `/sdcard`。
- 修复 OpenCode/xterm.js 高频输出时界面卡顿的问题：WebChannel 增加 xterm 写入完成回执和单帧在途背压，输出按 32KB/8ms 分块；ConPTY 宿主按 64KB/8ms 合并输出，隐藏页面和后台终端标签暂停前端投递。
- 移除终端工具栏中重复的下拉选择按钮，终端类型统一从“+”按钮菜单创建。
- 修复英文模式仅加载部分 JetBrains Mono 字体的问题；构建和安装现在复制英文目录内全部 32 个 TTF，原生 Qt 控件、xterm.js 与 Appium Inspector 均按当前语言使用本地字体。
- 修复 Appium Inspector 官方浏览器包从本地 `file://` 路径加载时语言资源定位错误、界面显示 `startSession` 和 `attachToSession` 等翻译键的问题。
- 修复 Qt Widgets 降级页中根 Remote Path 被拼接为 `//session`、删除已保存能力集时行定位不可靠，以及能力值编辑器初始化时重复插入控件的问题。
- 修复完整 Qt WebEngine 部署后终端区域变成白屏的问题；本地页面导航校验不再混用 Qt 规范路径的正斜杠和 Windows 目录分隔符，CSP 允许 xterm.js 必需的动态样式，`terminal-web/index.html`、xterm.js 与 QWebChannel 可以正常加载和渲染。
- 修复 Qt Creator 中可以启动、但从构建目录双击主程序会依次提示缺少 `Qt6Widgetsd.dll`、`Qt6WebChanneld.dll` 和 `Qt6WebEngineCored.dll` 等运行库的问题；Windows 构建现在链接后自动执行 `windeployqt`，同时部署 WebEngine 进程和资源。
- 修复终端页始终显示 OpenCode 入口、但默认构建未复制 `opencode.exe`、Node.js 和 `node-pty`，导致新建 OpenCode 标签立即报“未找到可执行程序”的问题；Windows 构建现在使用锁定版本和 SHA-256 自动 staging 完整 ConPTY 运行时。
- 修复性能工作区右侧存在大块空白、第四列 CPU 核心可能超出可视区域的问题。
- 修复性能曲线每次采样才跳动一次的问题，时间网格和曲线现在会在两次采样之间平滑连续移动。
- 修复性能曲线时间轴、实时采样时刻与开机时长未同步更新的问题。
- 修复仅使用无图层 SurfaceFlinger latency 时部分设备 FPS 长期为零的问题，增加前台应用识别、`flips` 增量和应用图层 latency 回退。
- 修复应用列表和详情面板仅显示通用占位图标的问题。
- 修复读取应用详情后包名回退值覆盖已提取本地化名称的问题。
- 修复切换 Android 设备后可能继续复用上一台设备应用名称与图标缓存的问题。
- 修复部分运行时权限因用户固定标记导致首次授权或撤销失败的问题，失败时会清理相关标记并重试。
- 修复多选独立 APK 被当作同一应用拆分包安装的问题，改为逐个执行并汇总结果。
- 补充 MSVC 构建所需的标准 `__cplusplus` 和严格一致性编译选项。

### Removed

- 移除独立 Plugin/Skill 规范和对应实现规划；AI 扩展统一使用 OpenCode 自带的 plugins、skills、agents 和 tools 机制。

## [0.1.0] - 2026-07-10

### Added

- 初始化 Qt6 CMake 桌面应用工程。
