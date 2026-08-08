# AI Mobile Test Studio

AI Mobile Test Studio 是一个面向 Android 设备调试与 AI 自动化测试的 Windows 桌面应用。当前可运行主体使用 Qt 6/C++17，先提供稳定的设备工作台，再逐步形成“用例输入、AI 生成、自动执行、失败诊断、证据归档、报告输出”的完整测试闭环。

## 当前能力

- 发现和监控 Android 设备，管理 scrcpy 主屏幕、虚拟屏幕与摄像头镜像。
- 通过 ADB `shell,v2` 和 legacy 回退提供持久、多标签设备终端。
- 通过随包 OpenCode、Node.js、`node-pty` 和 Windows ConPTY 运行 OpenCode TUI。
- 为应用内 OpenCode 提供带随机 Token 的本地 Studio Control API，可打开稳定工作区、读取设备信息和应用列表、执行安全设备动作；API 使用独立 ADB 子进程，不阻塞用户继续使用其它工作区。
- 新增“自动化”工作区，自动发现 OpenCode 生成的脚本和报告 HTML；选中文件后可通过右上角“打开/运行”在系统默认浏览器中打开。
- 随包提供 npm、Conda、OpenJDK 8、Android SDK、Appium Server 和 UiAutomator2 driver；启动时复用默认端口上已有的 Appium，否则运行隔离的随包服务。
- 使用本地 xterm.js、Qt WebEngine 和 QWebChannel 渲染终端，支持输入输出、resize、复制粘贴、背压和后台预热。
- 提供设备控制、软件包、应用、文件、Recovery sideload、性能、进程、日志和其它 ADB 工具工作区。
- 批量读取真实应用名称和 PNG 图标，并在进程列表复用应用图标。
- 嵌入随包 Appium Inspector 2026.5.1 浏览器前端作为“布局”工作区。
- 支持中英文即时切换、动态效果偏好，以及随包霞鹜文楷和 JetBrains Mono 字体。

Python 自动化服务、Appium Runner、OpenCode Server/SDK、Agent 编排、报告回填和完整安装包仍在规划或建设中，不能视为已完成能力。

## 启动与缓存

应用启动后会立即预热“布局”工作区和默认 OpenCode 终端。检测到已授权设备后，还会异步预取应用列表、应用名称与图标、进程快照，以及 `/`、`/sdcard` 两个常用目录。

这些设备数据按设备序列号保存在当前桌面进程的内存中。用户在同一次运行中再次进入对应页面时会直接看到缓存；手动刷新仍会读取设备最新状态，文件变更会使目录缓存失效。退出应用后不会保留这批易变设备快照。

## 自动化 HTML 产物

应用会在 OpenCode 工作目录下自动创建并监听以下结构：

```text
automation/
  scripts/   # 自动化脚本的可交互 HTML 前端
  reports/   # 文档和测试报告 HTML
  assets/    # HTML 共享的 CSS、JavaScript、图片等资源
  runs/      # 后续执行记录、日志、截图和证据
```

随包 OpenCode 插件的 `amts_automation_paths` 工具返回这些绝对路径。自动化脚本最终文件必须写入 `scripts/`，文档或报告必须写入 `reports/`；交付入口必须是无需构建即可在默认浏览器直接打开的 `.html` 或 `.htm` 文件。目录扫描与终端、设备 API 分离，OpenCode 生成期间用户仍可切换并使用其它工作区。

## 架构概览

```text
Qt Widgets pages
    -> desktop services
        -> adb child processes / ADB shell socket / scrcpy
        -> AppiumService -> existing Appium or bundled Node.js/Appium
        -> TerminalService -> AdbShellSession
                           -> ConPtySession -> node-pty -> OpenCode
                           -> OpenCode Plugin -> StudioControlServer
                                              -> independent adb operations
        -> AutomationArtifactService -> workspace/automation HTML
    -> Qt WebEngine
        -> xterm.js terminal
        -> Appium Inspector
```

页面只提交操作和展示状态，ADB、ConPTY、文件传输和设备采样由 service 异步执行。发布目标是所有运行时均由应用携带，不依赖用户的系统 `PATH`，也不在首次启动时执行 `npm install`、`pip install` 或其它在线安装。

## 开发环境

- CMake 3.19 或更高。
- Qt 6.5 或更高，基础组件为 Core、Gui、Network 和 Widgets。
- 完整终端与“布局”工作区需要 WebChannel、WebEngineWidgets 及其传递依赖。
- 与 Qt 套件匹配的 MSVC 或 MinGW C++17 工具链。
- Android 真机、USB 调试授权，以及对应设备需要的 OEM 驱动。

Windows 构建默认依据 `tools/runtime/runtime-lock.json` 和 npm package lock 下载、校验并暂存隔离的便携运行时，包括 OpenCode、Node.js/npm、Conda、JDK 8、Android SDK 和 Appium。构建过程可联网，应用运行过程不会下载或安装这些工程依赖，也不会修改用户电脑上的同名工具和永久环境变量。

## 构建

推荐使用带 Qt WebEngine 的 MSVC Qt 套件：

```powershell
cmake -S . -B build-msvc-web -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="<Qt>/msvc2022_64"
cmake --build build-msvc-web --config Debug --parallel
```

Debug 可执行文件通常位于：

```text
build-msvc-web/Debug/AI_Mobile_Test_Studio_Codex.exe
```

Windows 构建默认在链接后运行 `windeployqt`，并复制 xterm.js、Appium Inspector、字体、应用元数据提取器和已配置的便携运行时。仅需编译且不需要可直接启动目录时，可设置 `-DAI_MOBILE_TEST_DEPLOY_QT_RUNTIME=OFF`；有意跳过便携 runtime staging 时，可设置 `-DAI_MOBILE_TEST_STAGE_TERMINAL_RUNTIME=OFF`。

## 测试

```powershell
ctest --test-dir build-msvc-web -C Debug --output-on-failure
```

测试包含 Appium 已有服务复用、运行时缺失和随包服务启动三种场景。ConPTY 冒烟测试只有在构建配置提供 Node.js 和 ABI 匹配的 `node-pty` 后才会注册。设备相关能力还需要真机检查，包括授权、终端 UTF-8/中文输入、应用图标、进程、文件操作、断开重连和设备切换。

## 仓库入口

| 路径 | 职责 |
| --- | --- |
| `apps/desktop/` | Qt 6 桌面端页面、服务和终端会话 |
| `resources/` | xterm.js、Appium Inspector、字体、图标和工具资源 |
| `services/automation/` | 规划中的 Python 自动化与 Agent 编排服务 |
| `packages/contracts/` | 规划中的跨进程共享契约 |
| `runtime/` | 本地便携运行时装配边界，不提交构建产物 |
| `tests/` | C++、Python 和端到端测试入口 |
| `tools/` | 运行时 staging、构建、打包和诊断脚本 |
| `docs/` | 产品、架构、终端、运行时和开发文档 |

## 文档

- [文档导航](docs/README.md)
- [项目设计](docs/PROJECT_DESIGN.md)
- [系统架构](docs/ARCHITECTURE.md)
- [终端与 OpenCode 架构](docs/TERMINAL_ARCHITECTURE.md)
- [Studio Control API](docs/STUDIO_CONTROL_API.md)
- [便携运行时](docs/PORTABLE_RUNTIME.md)
- [项目目录结构](docs/PROJECT_STRUCTURE.md)
- [开发指南](docs/DEVELOPMENT_GUIDE.md)
- [更新日志](docs/CHANGELOG.md)

项目工程事实以当前代码和 `docs/` 为准。规划能力必须在形成可运行闭环并完成验证后，才能标记为已实现。
