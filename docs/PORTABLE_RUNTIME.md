# 便携运行时与无外部安装分发规范

## 1. 目标

发布包必须在受支持的干净 Windows 电脑上直接运行，不要求终端用户预先安装或再次下载以下工具：

- Qt 运行库和 Qt WebEngine 运行资源。
- Android platform-tools，包括应用实际使用的 `adb`。
- scrcpy 及其配套文件。
- OpenCode 可执行程序。
- OpenCode ConPTY 宿主需要的 Node.js 和 ABI 匹配的 `node-pty`。
- 后续自动化闭环需要的 Python、JDK、Appium 和 UiAutomator2 driver。
- xterm.js 及官方 addons 的静态前端资源。
- Appium Inspector 浏览器前端、语言包、云提供商图标和许可证。

“无需下载”指最终用户收到的安装包或便携压缩包已经包含运行所需工具。构建机或 CI 可以在制品生成阶段下载经过锁定和校验的上游制品。

## 2. 边界与不能预置的内容

完整分发不等于完全离线：

- 云端模型仍需要网络、服务商账号和用户自己的 API 凭据。
- 不得在安装包中预置用户 Token、API Key 或 OpenCode 登录信息。
- 某些 Android 设备在 Windows 上可能需要厂商 USB 驱动；应用应诊断并提示，但不能保证替代所有厂商驱动。
- 本地模型属于独立可选运行时，体积、硬件要求和模型许可证需要单独评审。
- Windows ConPTY 由操作系统提供，不随应用复制；最低 Windows 版本必须在发布矩阵中声明。

## 3. 当前差距

当前构建会复制 `app_metadata.jar`、xterm.js、Appium Inspector 2026.5.1 静态资源和 ConPTY 宿主脚本。Windows 构建默认按 `tools/runtime/runtime-lock.json` 下载并校验 OpenCode、Node.js 和 `node-pty`，将它们 staging 到构建目录并复制到应用旁的 `runtime/`；也可用三个 CMake 路径参数整体覆盖。主程序链接后还会运行 `windeployqt`，部署 Qt DLL、插件和 WebEngine 资源，使构建目录可以直接启动。scrcpy、ADB、Appium Server/driver 和其余组件仍未进入统一锁文件，且 `MainWindow` 仍保留开发机 scrcpy 绝对路径回退。

在正式发布前必须完成：

1. 删除发布构建中的开发机绝对路径回退。
2. 建立统一 `RuntimeLocator` 和 `RuntimeManager`。
3. 将现有终端 runtime 锁和 SHA-256 装配扩展到全部组件，并生成完整许可证清单。
4. 让所有服务通过组件 ID 获取绝对路径，不读取系统 `PATH`。
5. 在干净 Windows 沙箱中完成无外部工具验收。

## 4. 目标发布目录

```text
AI-Mobile-Test-Studio/
  AI_Mobile_Test_Studio_Codex.exe
  Qt6Core.dll
  Qt6Gui.dll
  Qt6Network.dll
  Qt6Widgets.dll
  Qt6WebEngineCore.dll
  QtWebEngineProcess.exe
  platforms/
  resources/
  translations/
  runtime/
    manifest.json
    windows-x64/
      android/
        platform-tools/
          adb.exe
          AdbWinApi.dll
          AdbWinUsbApi.dll
        app_metadata.jar
      scrcpy/
        scrcpy.exe
        ...上游发布包的完整配套文件
      opencode/
        opencode.exe
      python/
      node/
        node.exe
        node_modules/node-pty/
      jdk/
      appium/
      appium-inspector/
        index.html
        assets/
        locales/
        LICENSE
        NOTICE.md
      terminal-web/
        index.html
        vendor/
      terminal-host/
        conpty_host.js
  licenses/
    THIRD_PARTY_NOTICES.md
    components/
```

不得把可写缓存、日志、凭据或任务文件放进安装目录。它们应使用：

- 配置：`QStandardPaths::AppConfigLocation`
- 缓存：`QStandardPaths::CacheLocation`
- 应用数据：`QStandardPaths::AppLocalDataLocation`
- 用户选择的项目和测试产物：独立 workspace 目录

## 5. 运行时清单

`runtime/manifest.json` 是发布包内工具版本的唯一事实来源。建议字段：

```json
{
  "schemaVersion": 1,
  "platform": "windows-x64",
  "components": {
    "adb": {
      "version": "pinned-version",
      "path": "android/platform-tools/adb.exe",
      "sha256": "...",
      "source": "official-source-url",
      "license": "review-required"
    },
    "scrcpy": {
      "version": "pinned-version",
      "path": "scrcpy/scrcpy.exe",
      "sha256": "...",
      "source": "official-release-url",
      "license": "Apache-2.0"
    },
    "opencode": {
      "version": "pinned-version",
      "path": "opencode/opencode.exe",
      "sha256": "...",
      "source": "official-release-url",
      "license": "MIT"
    },
    "node-pty": {
      "version": "pinned-version",
      "path": "node/node_modules/node-pty",
      "sha256": "package-and-native-binary-hashes",
      "source": "https://github.com/microsoft/node-pty",
      "license": "MIT"
    }
  }
}
```

规则：

- 版本不能使用 `latest`。
- 下载 URL、SHA-256、许可证和目标架构必须同时锁定。
- 清单生成后进入发布制品；运行时只校验，不自行更新版本。
- 更新任一组件必须有独立变更记录和干净机回归。

## 6. 路径解析和无冲突原则

### 6.1 路径优先级

发布构建只允许使用随包组件。开发构建可按以下优先级解析：

1. 明确的开发者环境变量覆盖。
2. 构建输出旁的 `runtime/manifest.json`。
3. 失败并给出缺失或损坏诊断。

禁止静默回退到系统 `PATH`、注册表中的未知版本或开发机绝对路径。所有子进程使用绝对可执行文件路径和参数数组。

### 6.2 私有 ADB server

应用应使用随包 `adb` 启动应用私有的 ADB server，避免占用或终止用户默认的 `5037` server：

- 启动时动态分配本机端口并记录到 `RuntimeSession`。
- 所有 `adb` 子进程显式传入同一 server 端口。
- 给 scrcpy、Appium、Python Runner 和桌面服务注入同一进程级 ADB 环境。
- `TerminalService` 使用同一 host/port 连接 ADB server。
- 退出时只关闭本应用启动的 server，不执行会影响全局实例的无条件 `adb kill-server`。

需要在真机上验证系统 ADB 与私有 ADB 并存、设备切换、unauthorized、sideload 和重启恢复场景。

### 6.3 OpenCode 隔离

- 使用随包 OpenCode 的绝对路径，不调用系统同名命令。
- 使用随包 Node.js 和 ABI 匹配的 `node-pty` 创建 ConPTY，不读取系统 Node/npm 或 VS Code 内置模块。
- 为每个 workspace 设置明确工作目录。
- OpenCode Server 仅绑定 `127.0.0.1`，端口由应用分配。
- 设置随机会话密码；不得监听公网地址。
- OpenCode 配置、TUI 配置和凭据写入用户数据目录，不写入安装目录。
- 日志必须脱敏，不能记录服务商 Token。

### 6.4 子进程环境

`RuntimeManager` 为每个子进程从最小环境开始构造 `QProcessEnvironment`：

- 只在子进程环境中追加运行时目录。
- 不修改系统或当前用户的永久 `PATH`。
- 明确传递 workspace、临时目录、语言和端口。
- 记录环境变量名称和非敏感摘要，不记录密钥值。

## 7. 构建和装配流水线

建议新增：

```text
tools/runtime/
  runtime-lock.json             # 已实现：OpenCode/Node.js/node-pty
  stage-terminal-runtime.ps1    # 已实现
  fetch-runtime.ps1
  verify-runtime.ps1
  stage-runtime.ps1
  generate-notices.ps1
tools/package/
  package-windows.ps1
  smoke-test-clean-windows.ps1
```

流水线顺序：

1. 从锁文件读取组件版本和官方来源。
2. 下载到构建缓存，不下载到源码目录。
3. 校验 SHA-256 和签名（上游提供时）。
4. 解压到临时 staging 目录，禁止直接覆盖现有制品。
5. 运行恶意软件扫描和二进制架构检查。
6. 生成 `manifest.json` 和第三方 notices。
7. 使用 CMake install 和 `windeployqt` 部署 Qt；引入 WebEngine 后必须包含 `QtWebEngineProcess`、resources、locales 和依赖 DLL。
8. 生成完整安装包和便携 ZIP。
9. 在干净虚拟机中执行启动、设备、终端和 OpenCode 冒烟测试。

## 8. 启动自检

启动自检只检查本地制品，不联网下载：

| 检查 | 失败行为 |
| --- | --- |
| 清单存在且 schema 支持 | 阻止依赖相关功能，提示修复安装 |
| 关键文件存在、SHA-256 正确 | 标记安装损坏，不回退系统工具 |
| 二进制架构匹配 | 给出 x64/arm64 不匹配信息 |
| 私有端口可分配 | 更换端口并重试有限次数 |
| ADB server 可启动 | 保留日志并显示诊断入口 |
| OpenCode 可启动 | 终端页显示结构化错误和修复建议 |
| 用户数据目录可写 | 阻止任务启动，避免数据丢失 |

诊断页应显示组件版本、路径、校验状态、进程 PID、端口和日志位置，但隐藏所有凭据。

## 9. 第三方许可证门禁

打包第三方工具前必须由项目维护者完成许可证审查。最低要求：

- 保存上游许可证原文和源码/发布地址。
- 生成 `THIRD_PARTY_NOTICES.md`。
- 遵守 Qt 所采用商业版、LGPL 或 GPL 方案的分发义务。
- Qt WebEngine 还需要包含 Chromium 等第三方 notices。
- Android platform-tools 的预编译制品需要单独确认再分发权利；不能仅因源码仓库开放就推定预编译包可随意再分发。
- 不采用与产品分发策略冲突的组件；例如 QTermWidget 官方声明整体为 GPLv2+，且不支持 Windows。

许可证审查未完成时，组件状态只能是“待评审”，不能进入正式安装包。

## 10. 发布验收

必须在没有安装 Qt、ADB、scrcpy、OpenCode、Python、Node.js、JDK 和 Appium 的干净 Windows 机器上验证：

1. 安装或解压后直接启动。
2. 不读取系统同名工具，不修改永久环境变量。
3. 连接 Android 设备并显示设备状态。
4. ADB 终端执行命令并正确调整尺寸。
5. scrcpy 能启动和停止。
6. OpenCode TUI 能在指定 workspace 启动、输入、退出和恢复。
7. OpenCode Server/SDK 能获取结构化会话状态。
8. Appium 最小用例可运行。
9. 卸载后不删除用户 workspace，不残留后台进程和监听端口。

## 11. 参考资料

- [scrcpy 官方仓库](https://github.com/Genymobile/scrcpy)
- [OpenCode 官方文档](https://opencode.ai/docs/)
- [OpenCode Server 文档](https://opencode.ai/docs/server/)
- [Qt 部署文档](https://doc.qt.io/qt-6/deployment.html)
- [Qt Licensing](https://doc.qt.io/qt-6/licensing.html)
- [Windows ConPTY](https://learn.microsoft.com/en-us/windows/console/creating-a-pseudoconsole-session)
