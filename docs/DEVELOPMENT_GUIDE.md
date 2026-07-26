# 开发指南

## 1. 开发基线

当前可运行产品是 Qt 6/C++17 桌面应用。ADB/OpenCode 统一终端、`node-pty` ConPTY 宿主和 xterm.js/WebChannel 资源已经接入；Python 服务、Appium、OpenCode Server/SDK 和完整便携运行时仍属于后续里程碑。

开发原则：

- 优先保持真机设备能力可靠，不用静态假数据替代失败路径。
- 开发机可以安装编译工具，最终用户不能被要求安装运行依赖。
- Release 行为从第一天按便携路径和进程隔离设计。
- 规划功能的目录骨架不能被当成已实现能力。

## 2. 开发环境

当前桌面端需要：

- CMake 3.19 或更高。
- Qt 6.5 或更高，基础组件：Core、Gui、Network、Widgets。
- 完整终端显示还需要 WebChannel、WebEngineWidgets 及 WebEngine 的传递依赖（Qt 6.8.3 当前包括 Qt Positioning）；缺少时自动使用基础显示降级。
- 支持 C++17 的 MinGW 或 MSVC 工具链。
- Android 真机和 USB 调试，用于设备能力验证。
- Windows 开发构建默认从锁文件 staging OpenCode、Node.js 和 `node-pty`；也可显式配置三个本地路径。正式包必须使用随包 runtime。

Node.js 和 ABI 匹配的 `node-pty` 是 OpenCode ConPTY 宿主的运行时组成部分，必须随正式制品装配；不能要求最终用户自行安装。xterm.js 静态资源已提交，不在应用启动时运行 npm。

## 3. 构建

推荐使用独立构建目录，生成器和 Qt 工具链必须匹配：

```powershell
cmake -S . -B build-mingw -G Ninja `
  -DCMAKE_PREFIX_PATH="<Qt>/mingw_64"
cmake --build build-mingw --parallel
```

Windows 首次构建默认读取 `tools/runtime/runtime-lock.json`，下载到构建目录缓存，校验 SHA-256 后 staging 终端运行时；应用启动时不会联网下载。需要完全离线构建时，可同时提供三个本地路径覆盖：

```powershell
cmake -S . -B build-terminal -G Ninja `
  -DCMAKE_PREFIX_PATH="<Qt>/mingw_64" `
  -DAI_MOBILE_TEST_OPENCODE_EXECUTABLE="<runtime>/opencode.exe" `
  -DAI_MOBILE_TEST_NODE_EXECUTABLE="<runtime>/node.exe" `
  -DAI_MOBILE_TEST_NODE_PTY_MODULE="<runtime>/node_modules/node-pty"
```

三个覆盖参数必须同时配置，避免 OpenCode、Node.js 和原生模块版本混用。要有意构建不含 OpenCode runtime 的变体，可设置 `-DAI_MOBILE_TEST_STAGE_TERMINAL_RUNTIME=OFF`。

也可在开发启动时用 `AI_MOBILE_TEST_OPENCODE_PATH`、`AI_MOBILE_TEST_WORKSPACE`、`AI_MOBILE_TEST_NODE_PATH` 和 `AI_MOBILE_TEST_NODE_PTY_PATH` 覆盖；这些覆盖必须是明确的绝对路径。

MSVC 构建必须使用 MSVC 版 Qt，不能让 Visual Studio generator 链接 MinGW Qt 库。遇到 `mingw32.lib` 等错误时应重新配置匹配的构建目录，而不是在项目中硬补系统库。

当前可执行文件位于构建目录根部。Windows 构建默认在链接后运行 `windeployqt`，把当前配置对应的 Qt DLL、插件和 WebEngine 资源部署到可执行文件旁，因此可以从资源管理器直接双击运行。仅需编译、不需要可直接启动目录的特殊构建可设置 `-DAI_MOBILE_TEST_DEPLOY_QT_RUNTIME=OFF`。

## 4. 当前真机运行

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

## 5. 代码修改流程

1. 阅读 [README.md](README.md) 和相关专项文档。
2. 检查工作树，保留其他未提交修改。
3. 定位页面、service 和运行时所有权。
4. 先实现最小行为，再补错误、断开和取消路径。
5. 构建目标工具链。
6. 用真机或可复现 fixture 验证。
7. 运行 `git diff --check`。
8. 更新 `CHANGELOG.md` 和相关设计文档。

## 6. 桌面端开发规则

- `MainWindow` 只创建对象、连接信号和切换工作区。
- 页面通过 signals 发起动作，不创建 ADB/scrcpy/OpenCode 进程。
- 外部进程、socket 和任务必须有明确 owner。
- `QProcess` 使用 program + argument list，不拼接 shell 命令。
- 输出读取、设备采样和文件传输不阻塞 UI 线程。
- 设备切换时清理缓存、终止旧会话并更新全部相关页面。
- Release 代码只通过 `RuntimeLocator` 获取第三方组件路径。

## 7. 终端开发

当前 ADB 终端改动至少验证：

- shell 提示符和 UTF-8 输出。
- Enter、Backspace、方向键、Tab 和 Ctrl+C。
- 两个标签输出隔离。
- 标签新增、关闭和重置。
- resize 数据包。
- 设备断开、切换和远端 shell 退出。

xterm.js/ConPTY 开发以 [TERMINAL_ARCHITECTURE.md](TERMINAL_ARCHITECTURE.md) 为准。不要继续在页面里扩展完整 VT 解析器，也不要用普通 QProcess 管道直接运行 OpenCode TUI。当前 QProcess 只启动 `node-pty` 宿主，真正的 OpenCode 子进程仍在 ConPTY 中。

## 8. 运行时组件更新

正式 runtime 装配实现后，更新第三方组件必须：

1. 修改 `tools/runtime/runtime-lock.json`。
2. 只使用官方来源。
3. 更新 SHA-256 和架构。
4. 阅读 release notes 和许可证变化。
5. 在临时 staging 目录重新装配。
6. 生成 notices。
7. 执行组件级和干净机回归。
8. 在 `CHANGELOG.md` 记录版本变化。

禁止在应用启动时自动执行包管理器安装、npm install、pip install 或在线升级工具。

## 9. OpenCode 开发

- 运行随包绝对路径，不调用系统 `opencode`。
- TUI 通过 ConPTY；结构化集成通过 Server/SDK。
- ConPTY 使用 ABI 匹配的随包 Node.js + `node-pty`；更新任一版本时必须重跑 `conpty_session_smoke`。
- Server 绑定 `127.0.0.1` 动态端口并使用认证。
- workspace、模型配置和权限策略必须显式。
- 测试时使用无真实密钥的 fixture；真实凭据只存在用户配置存储。
- 不用终端截图或文本正则判断任务是否完成。

## 10. Python 自动化服务（规划）

实现后推荐开发方式：

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -e .\services\automation
pytest tests\python
```

开发虚拟环境只服务开发者。发布版使用预装、锁定并校验的便携 Python，不在用户电脑运行 pip。

## 11. 测试层级

| 层级 | 目标 |
| --- | --- |
| C++ 单元 | 协议解析、路径解析、命令目录、状态转换 |
| Service 测试 | fake process/socket 下的超时、取消、错误和重连 |
| Python 单元 | contracts、附件、报告和 Agent 状态机 |
| 真机冒烟 | ADB、终端、应用、文件、Recovery、性能和镜像 |
| Runtime 冒烟 | 清单、校验、私有端口和进程回收 |
| 干净机 E2E | 无系统依赖情况下完成启动和最小 AI 自动化 |

不可自动化的真机步骤必须写成可重复的验收清单。

## 12. 打包开发

目标流程见 [PORTABLE_RUNTIME.md](PORTABLE_RUNTIME.md)。关键要求：

- CMake install 只负责项目产物组合；第三方 runtime 由可复现脚本 staging。
- Qt 使用官方部署工具，WebEngine 资源不能漏包。
- 安装包和 portable ZIP 使用同一 runtime manifest。
- 打包脚本失败即停止，不允许生成缺组件但表面成功的制品。

## 13. 提交前检查

- 构建通过，目标程序可启动。
- 没有新增开发机绝对路径。
- 没有读取未知系统工具或修改永久 PATH。
- UI 线程没有同步等待外部进程。
- 错误、取消、断开和退出路径已处理。
- 新运行时有版本、校验、来源和许可证。
- 文档中的“已实现/规划中”与代码一致。
