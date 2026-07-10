# 开发规范

## 1. 开发目标

本项目以“能稳定执行真实手机自动化测试”为第一优先级。界面、Agent、脚本生成、报告都必须服务于测试闭环，而不是只展示 AI 对话。

开发时遵循以下原则：

- 先保证设备连接、页面观测和脚本执行可靠。
- UI 不直接做耗时任务，所有长任务进入后台进程或任务队列。
- 每个测试任务都有独立工作目录，便于复盘。
- 任何 Agent 输出都必须能落盘、能追踪、能复现。
- 支持用户无环境安装使用，开发阶段也要按可打包方式设计路径。

## 2. 推荐目录结构

```text
AI_Mobile_Test_Studio_Codex/
  CMakeLists.txt
  main.cpp
  mainwindow.cpp
  mainwindow.h
  mainwindow.ui
  docs/
  src/
    app/
    ui/
    core/
    device/
    agent/
    attachment/
    report/
  bridge/
    app/
    api/
    agents/
    device/
    runner/
    attachments/
    reports/
    runtime/
    tests/
  runtime/
    README.md
  plugins/
  skills/
  workspace/
```

现有工程仍处于 Qt 默认模板阶段，后续开发可以逐步迁移到上述结构。

## 3. 技术栈约定

### 3.1 桌面端

- Qt 版本：Qt 6.5 或更高。
- 构建系统：CMake。
- UI 技术：优先 Qt Widgets，后续如需复杂动画或流媒体渲染可引入 Qt Quick。
- 本地通信：优先 WebSocket + HTTP，或 QLocalSocket。

### 3.2 自动化端

- Python 版本：建议 3.11 或 3.12 便携版。
- 自动化框架：Appium Python Client。
- Android 通信：ADB、Appium UiAutomator2 driver。
- 画面显示：scrcpy 或 minicap 类方案，第一阶段建议 scrcpy。
- 日志采集：adb logcat。

### 3.3 Agent 端

- Agent CLI：opencode。
- Agent 类型：脚本生成、UI 理解、错误修复、测试报告。
- Agent 调用：统一由 Agent Orchestrator 触发。
- 上下文输入：用户需求、附件解析结果、设备上下文、执行日志。

## 4. 分支与提交

如果后续初始化 Git，建议采用：

- `main`：稳定版本。
- `develop`：日常集成。
- `feature/<name>`：功能开发。
- `fix/<name>`：缺陷修复。
- `docs/<name>`：文档更新。

提交信息格式：

```text
type(scope): summary
```

示例：

```text
feat(device): add adb device discovery
fix(agent): stop retry loop when appium session dies
docs(architecture): add agent orchestration diagram
```

常用类型：

- `feat`
- `fix`
- `docs`
- `refactor`
- `test`
- `build`
- `chore`

## 5. 开发流程

1. 明确测试场景和输入输出。
2. 编写最小接口契约。
3. 实现后端能力。
4. 用命令行验证后端能力。
5. 接入 Qt UI。
6. 补充日志、错误状态和用户提示。
7. 编写最小测试。
8. 更新文档。

不要先做复杂界面，再补自动化能力。设备闭环优先。

## 6. 任务工作目录规范

每次测试任务必须创建独立目录：

```text
workspace/tasks/<timestamp>-<short-name>/
  task.json
  input/
  parsed/
  scripts/
  runs/
  screenshots/
  logs/
  reports/
  filled/
```

`task.json` 建议字段：

```json
{
  "taskId": "20260710-230000-bt-connect",
  "createdAt": "2026-07-10T23:00:00+08:00",
  "deviceId": "R5CT0000000",
  "message": "生成蓝牙连接测试脚本",
  "status": "running",
  "retryCount": 0
}
```

## 7. 设备接入规范

设备接入必须经过 Device Controller，不允许 UI 或 Agent 直接拼接 ADB/Appium 命令。

Device Controller 对外提供：

- `list_devices`
- `get_device_info`
- `start_mirror`
- `stop_mirror`
- `take_screenshot`
- `get_current_activity`
- `get_current_fragment`
- `get_page_source`
- `get_logcat`
- `get_toast`
- `get_crash_info`
- `get_anr_info`
- `tap`
- `swipe`
- `input_text`
- `press_key`

## 8. Agent 接入规范

所有 Agent 必须通过 Agent Orchestrator 调用。

Agent 输入应包含：

- 用户原始需求。
- 附件解析摘要。
- 当前设备上下文。
- 当前任务文件列表。
- 允许修改的文件范围。
- 最大运行时间。

Agent 输出应包含：

- 结构化结果。
- 产物路径。
- 错误信息。
- 下一步建议。

## 9. 测试规范

### 9.1 后端单元测试

优先覆盖：

- 附件解析。
- 任务目录创建。
- 报告生成。
- 设备上下文结构化。
- Agent 输出解析。

### 9.2 设备集成测试

至少覆盖：

- ADB 设备发现。
- 截图。
- 页面树采集。
- Activity 采集。
- Appium 启动和关闭。
- 简单点击流程。

### 9.3 端到端测试

端到端测试应包含：

- 上传用例。
- 生成脚本。
- 执行脚本。
- 采集失败上下文。
- 自动修复一次。
- 生成报告。

## 10. 日志规范

日志分为四类：

- 应用日志：Qt 和 bridge 运行状态。
- 设备日志：ADB、logcat、Appium。
- Agent 日志：opencode 命令、输入摘要、输出摘要。
- 测试日志：用例步骤、断言结果、耗时、截图路径。

日志必须包含：

- 时间。
- 任务 ID。
- 设备 ID。
- 模块名。
- 级别。
- 消息。

示例：

```json
{
  "time": "2026-07-10T23:00:00+08:00",
  "taskId": "20260710-230000-bt-connect",
  "deviceId": "R5CT0000000",
  "module": "device",
  "level": "info",
  "message": "current activity collected"
}
```

## 11. 打包规范

最终交付应做到用户免安装依赖。

Windows 打包建议：

- Qt 使用 `windeployqt` 或 CMake install 部署。
- Python 使用 embeddable package 或独立便携目录。
- Node.js、JDK、platform-tools、scrcpy、opencode 放入 `runtime/`。
- 首次启动执行运行时自检。
- 所有工具路径从应用目录解析，不依赖系统 PATH。

## 12. 文档维护

每次变更以下内容时必须更新文档：

- 架构边界。
- 运行时依赖。
- Agent 协议。
- 插件协议。
- 测试报告字段。
- 打包方式。
- 目录结构。

