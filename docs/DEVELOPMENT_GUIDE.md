# 开发指南

## 1. 开发基线

当前可运行产品是 Qt 6/C++17 桌面应用。ADB/OpenCode 统一终端、`node-pty` ConPTY 宿主、xterm.js/WebChannel、Appium Inspector 2026.5.1、锁定的 Windows x64 便携运行时和 `AppiumService` 已经接入。Python Automation Service、测试 Runner、OpenCode Server/SDK、Python/scrcpy 统一装配和正式发布制品仍属于后续里程碑。

开发原则：

- 优先保持真机设备能力可靠，不用静态假数据替代失败路径。
- 开发机可以安装编译工具，最终用户不能被要求安装运行依赖。
- Release 行为从第一天按便携路径和进程隔离设计。
- 规划功能的目录骨架不能被当成已实现能力。

## 2. 开发环境

当前桌面端需要：

- CMake 3.19 或更高。
- Qt 6.5 或更高，基础组件：Core、Gui、Network、Widgets。
- 完整终端和“布局”工作区还需要 WebChannel、WebEngineWidgets 及 WebEngine 的传递依赖（Qt 6.8.3 当前包括 Qt Positioning）；缺少时自动使用 Qt Widgets 基础显示降级。
- 支持 C++17 的 MinGW 或 MSVC 工具链。
- Android 真机和 USB 调试，用于设备能力验证。
- Windows 开发构建默认从锁文件 staging OpenCode、Node.js/npm、`node-pty`、Conda standalone、OpenJDK 8、Android command-line/platform-tools、Appium 和 UiAutomator2 driver；OpenCode 终端三组件也可显式配置本地路径。正式包必须使用随包 runtime。

Node.js 和 ABI 匹配的 `node-pty` 是 OpenCode ConPTY 宿主的运行时组成部分；同一 Node.js 还负责启动随包 Appium。所有组件必须随正式制品装配，不能要求最终用户自行安装。xterm.js 静态资源已提交，Appium npm 依赖由 `tools/runtime/appium/package-lock.json` 锁定；应用启动时不运行 npm 或联网安装 driver。

当前 `tools/runtime/runtime-lock.json` 锁定：

| 组件 | 版本 |
| --- | --- |
| OpenCode | 1.18.5 |
| Node.js / npm / `node-pty` | 24.18.0 / 11.16.0 / 1.1.0 |
| Conda standalone | 26.5.2 |
| OpenJDK | 8.0.502+7 |
| Android command-line tools / platform-tools | 8.0 / 37.0.1 |
| Appium / UiAutomator2 driver | 3.5.2 / 8.1.0 |

## 3. 构建

推荐使用独立构建目录，生成器和 Qt 工具链必须匹配：

```powershell
cmake -S . -B build-mingw -G Ninja `
  -DCMAKE_PREFIX_PATH="<Qt>/mingw_64"
cmake --build build-mingw --parallel
```

Windows 首次构建默认读取 `tools/runtime/runtime-lock.json` 和 `tools/runtime/appium/package-lock.json`，下载到构建目录缓存，校验 SHA-256 后原子 staging 完整的当前 Windows x64 私有运行时；脚本还会执行组件版本校验并生成 schema 2 `runtime/manifest.json`。应用启动时不会联网下载。

以下三个参数只覆盖 OpenCode 终端链路，必须同时提供；Conda、JDK、Android SDK 和 Appium 仍由锁文件 staging：

```powershell
cmake -S . -B build-terminal -G Ninja `
  -DCMAKE_PREFIX_PATH="<Qt>/mingw_64" `
  -DAI_MOBILE_TEST_OPENCODE_EXECUTABLE="<runtime>/opencode.exe" `
  -DAI_MOBILE_TEST_NODE_EXECUTABLE="<runtime>/node.exe" `
  -DAI_MOBILE_TEST_NODE_PTY_MODULE="<runtime>/node_modules/node-pty"
```

三个覆盖参数必须同时配置，避免 OpenCode、Node.js 和原生模块版本混用。要有意构建不含当前便携 runtime 的变体，可设置 `-DAI_MOBILE_TEST_STAGE_TERMINAL_RUNTIME=OFF`；该选项名称保留了早期“terminal runtime”命名，但目前控制的是整套已锁定运行时。

也可在开发启动时用 `AI_MOBILE_TEST_OPENCODE_PATH`、`AI_MOBILE_TEST_WORKSPACE`、`AI_MOBILE_TEST_NODE_PATH` 和 `AI_MOBILE_TEST_NODE_PTY_PATH` 覆盖；这些覆盖必须是明确的绝对路径。

MSVC 构建必须使用 MSVC 版 Qt，不能让 Visual Studio generator 链接 MinGW Qt 库。遇到 `mingw32.lib` 等错误时应重新配置匹配的构建目录，而不是在项目中硬补系统库。

完整 Appium Inspector 工作区推荐使用包含 WebEngineWidgets 的 MSVC Qt 套件。Qt 官方 MinGW 套件可能不包含 WebEngine，此时只会构建基础降级页：

```powershell
cmake -S . -B build-msvc-web -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="<Qt>/msvc2022_64"
cmake --build build-msvc-web --config Debug --parallel
```

当前可执行文件位于构建目录根部。Windows 构建默认在链接后运行 `windeployqt`，把当前配置对应的 Qt DLL、插件和 WebEngine 资源部署到可执行文件旁，因此可以从资源管理器直接双击运行。仅需编译、不需要可直接启动目录的特殊构建可设置 `-DAI_MOBILE_TEST_DEPLOY_QT_RUNTIME=OFF`。

CMake 通过 Qt 提供的导入目标 `Qt6::windeployqt` 定位部署工具。不得恢复对未定义 `QT_QMAKE_EXECUTABLE` 的路径推导，否则全新配置目录会在生成阶段失败。配置日志中的 `WrapVulkanHeaders` 和未设置 `VCINSTALLDIR` 提示在当前已验证构建中属于非致命警告。

UI PNG 图标位于 `resources/images/icons/`。链接完成后 CMake 会将整个目录复制到可执行文件旁的 `runtime/images/icons/`；如果界面退回字符图标或出现空图标，先确认启动的是最新构建，并检查该运行时目录是否完整包含图标文件。

## 4. Appium 运行时与 Inspector

侧边栏“布局”工作区优先加载随包的 Appium Inspector 2026.5.1 官方浏览器构建，源资源位于 `resources/appium-inspector/`，构建后复制到 `runtime/appium-inspector/`。该页面直接提供 Session Builder、云提供商配置、能力集、Attach to Session、Source、Commands、Gestures、Recorder 和 Session Information。

应用启动时会创建 `AppiumService` 并异步调用 `ensureStarted()`。服务先探测 `http://127.0.0.1:4723/status`：有效时复用现有 Appium 且不取得其所有权；不可用时检查随包 Node.js、Appium、UiAutomator2、JDK 和 Android SDK，随后启动随包 Appium。子进程使用私有 `PATH`、`JAVA_HOME`、`ANDROID_HOME`、`ANDROID_SDK_ROOT` 和 `APPIUM_HOME`，driver metadata、npm 缓存和 Conda 缓存写入 `QStandardPaths::AppLocalDataLocation`。应用退出时只停止自己启动的 Appium。

浏览器版 Inspector 直接连接开发者手工启动的外部 Appium Server 时可能受跨域策略约束。外部服务应允许 Inspector 来源访问，例如：

```powershell
appium --allow-cors
```

随包运行时使用 Appium 3.5.2 和 UiAutomator2 driver 8.1.0。Android command-line tools 固定为 8.0，因为该版本可与随包 JDK 8 共同工作；command-line tools 22 需要 Java 17，不能在当前 JDK 8 基线中直接替换。任何升级都必须成组验证 JDK、SDK tools、Appium 和 driver。

## 5. 当前真机运行

开发构建的 scrcpy 路径解析顺序目前是：

1. `AI_MOBILE_TEST_SCRCPY_PATH`
2. QSettings 中的开发配置
3. 构建输出旁 `runtime/scrcpy/scrcpy.exe`
4. 临时开发机回退路径

第 4 项是待删除技术债。新增代码不得复制这种绝对路径做法。

真机检查：

```powershell
& "<adb.exe>" devices -l
```

确认状态为 `device`，再启动应用。`unauthorized` 时需要在设备端确认 USB 调试授权。

## 6. 代码修改流程

1. 阅读 [README.md](README.md) 和相关专项文档。
2. 检查工作树，保留其他未提交修改。
3. 定位页面、service 和运行时所有权。
4. 先实现最小行为，再补错误、断开和取消路径。
5. 构建目标工具链。
6. 用真机或可复现 fixture 验证。
7. 运行 `git diff --check`。
8. 更新 `CHANGELOG.md` 和相关设计文档。

## 7. 桌面端开发规则

- `MainWindow` 只创建对象、连接信号和切换工作区。
- 页面通过 signals 发起动作，不创建 ADB/scrcpy/OpenCode 进程。
- 外部进程、socket 和任务必须有明确 owner。
- `QProcess` 使用 program + argument list，不拼接 shell 命令。
- 输出读取、设备采样和文件传输不阻塞 UI 线程。
- 设备缓存按序列号隔离并只存在于当前运行；设备切换时保存旧设备缓存、终止旧进程、抑制迟到回调，再恢复新设备缓存并更新页面。
- 自动预取不能阻塞页面交互；手动刷新必须明确绕过缓存，设备写操作必须使相关缓存失效。
- Release 代码只通过 `RuntimeLocator` 获取第三方组件路径。

## 8. 终端开发

当前 ADB 终端改动至少验证：

- shell 提示符和 UTF-8 输出。
- Enter、Backspace、方向键、Tab 和 Ctrl+C。
- 两个标签输出隔离。
- 标签新增、关闭和重置。
- resize 数据包。
- 中英文切换后字符网格宽度不变，中文不出现横向拉伸。
- 使用至少一种 Windows 中文输入法连续输入中文，候选、提交、删除和回车无明显卡顿或乱序。
- 应用启动后默认 OpenCode 会话已在后台完成前端初始化，首次进入终端不集中回放启动输出。
- 设备断开、切换和远端 shell 退出。

xterm.js/ConPTY 开发以 [TERMINAL_ARCHITECTURE.md](TERMINAL_ARCHITECTURE.md) 为准。不要继续在页面里扩展完整 VT 解析器，也不要用普通 QProcess 管道直接运行 OpenCode TUI。当前 QProcess 只启动 `node-pty` 宿主，真正的 OpenCode 子进程仍在 ConPTY 中。

## 9. 运行时组件更新

更新当前已实现的 runtime 组件必须：

1. 修改 `tools/runtime/runtime-lock.json`。
2. 只使用官方来源。
3. 更新 SHA-256 和架构。
4. 阅读 release notes 和许可证变化。
5. 在临时 staging 目录重新装配。
6. 生成 notices。
7. 执行组件级和干净机回归。
8. 在 `CHANGELOG.md` 记录版本变化。

Appium 或 driver 版本变化还必须更新 `tools/runtime/appium/package.json` 与 `package-lock.json`，并同步更新 `runtime-lock.json` 中的 npm lock SHA-256。staging 脚本会拒绝 lock hash 不一致的输入。

禁止在应用启动时自动执行包管理器安装、npm install、pip install 或在线升级工具。

## 10. OpenCode 开发

- 运行随包绝对路径，不调用系统 `opencode`。
- TUI 通过 ConPTY；结构化集成通过 Server/SDK。
- ConPTY 使用 ABI 匹配的随包 Node.js + `node-pty`；更新任一版本时必须重跑 `conpty_session_smoke`。
- Server 绑定 `127.0.0.1` 动态端口并使用认证。
- workspace、模型配置和权限策略必须显式。
- 测试时使用无真实密钥的 fixture；真实凭据只存在用户配置存储。
- 不用终端截图或文本正则判断任务是否完成。

## 11. Python 自动化服务（规划）

实现后推荐开发方式：

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -e .\services\automation
pytest tests\python
```

开发虚拟环境只服务开发者。发布版使用预装、锁定并校验的便携 Python，不在用户电脑运行 pip。

## 12. 测试层级

| 层级 | 目标 |
| --- | --- |
| C++ 单元 | 协议解析、路径解析、命令目录、状态转换 |
| Service 测试 | fake process/socket 下的超时、取消、错误和重连 |
| Python 单元 | contracts、附件、报告和 Agent 状态机 |
| 真机冒烟 | ADB、终端、应用、文件、Recovery、性能和镜像 |
| Runtime 冒烟 | 清单、校验、私有端口和进程回收 |
| 干净机 E2E | 无系统依赖情况下完成启动和最小 AI 自动化 |

不可自动化的真机步骤必须写成可重复的验收清单。

当前 CTest 已覆盖 5 个自动化冒烟并全部通过：`conpty_session_smoke`、`opencode_conpty_smoke`、`appium_reuses_existing_server`、`appium_missing_runtime` 和 `appium_bundled_runtime`。Appium 三项分别验证外部服务复用、缺失运行时失败以及随包服务启动。修改 Appium 启动参数、环境或 runtime 目录时必须重跑这三项。

启动预取的真机验收还应覆盖：应用和进程页首次进入已有数据、`/` 与 `/sdcard` 首次进入命中缓存、手动刷新重新访问设备、文件变更后目录回读，以及断开重连/切换设备时缓存不串设备。

## 13. 打包开发

当前装配基础与目标发布流程见 [PORTABLE_RUNTIME.md](PORTABLE_RUNTIME.md)。关键要求：

- 当前 CMake 构建和 install 会把完整 staged runtime 复制到可执行文件旁；第三方 runtime 由可复现脚本 staging。
- Qt 使用官方部署工具，WebEngine 资源不能漏包。
- 安装包和 portable ZIP 使用同一 runtime manifest。
- 打包脚本失败即停止，不允许生成缺组件但表面成功的制品。

## 14. 提交前检查

- 构建通过，目标程序可启动。
- 没有新增开发机绝对路径。
- 没有读取未知系统工具或修改永久 PATH。
- UI 线程没有同步等待外部进程。
- 错误、取消、断开和退出路径已处理。
- 新运行时有版本、校验、来源和许可证。
- 文档中的“已实现/规划中”与代码一致。
