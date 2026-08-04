# 开发路线图

## 1. 状态

- `完成`：当前代码已实现并验证。
- `进行中`：已有部分代码，但不满足阶段验收。
- `待开始`：只有文档或目录骨架。

## 2. M0 工程骨架与设备工作台

状态：**完成**

已完成：

- Qt 6/CMake 子项目结构。
- 主窗口、侧栏、设备状态和工作区切换。
- 设备控制、包管理、应用、文件、Recovery 和性能页面。
- scrcpy 进程管理和 Android 应用元数据工具。

剩余维护项进入后续阶段，不回退到静态演示数据。

## 3. M1 ADB 持久终端

状态：**完成**

已完成：

- ADB `shell,v2` 和 legacy 回退。
- 多会话、多标签、输入输出和窗口尺寸同步。
- 设备断开、切换、重置和快捷命令。
- 真机命令和多标签冒烟验证。

自研显示层的完整 TUI 兼容不属于本阶段，后续由 xterm.js 替换。

## 4. M2 便携运行时基础

状态：**进行中，下一优先级**

已完成：OpenCode、Node.js 和 `node-pty` 的 Windows x64 锁文件、SHA-256 下载校验、构建期 staging、应用目录复制和 ConPTY 冒烟。

任务：

- 实现 `RuntimeManifest`、`RuntimeLocator`、`RuntimeManager`。
- 删除发布构建的开发机硬编码路径。
- 建立 runtime lock、下载、SHA-256 校验和 staging 脚本。
- 随包装配 Qt、ADB、scrcpy 和 OpenCode。
- 建立第三方 notices 和许可证门禁。
- 使用私有 ADB server 端口，所有组件共享同一连接参数。
- 完成启动自检和诊断页。

验收：

- 干净 Windows 机器无需安装/下载 ADB、scrcpy、OpenCode 或 Qt 即可启动相关能力。
- 不读取系统 PATH，不终止用户已有 ADB server。
- 安装损坏时给出结构化诊断，不静默回退。

## 5. M3 成熟终端显示层

状态：**进行中**

任务：

- 引入 Qt WebEngine、QWebChannel 和本地 xterm.js 资源。
- 抽取 `ITerminalSession` 和 `TerminalSessionManager`。
- 让现有 ADB session 接入统一接口。
- 验证 Unicode、IME、鼠标、备用屏幕、搜索和大输出。
- 将 WebEngine 完整部署进安装包。

已完成：统一 `TerminalSession`、ADB 会话迁移、本地 xterm.js/FitAddon、QWebChannel 桥接、离线资源复制、MSVC Qt WebEngine 部署和无 WebEngine 降级构建；输出链路具备 xterm 写入回执、64KB 单帧在途背压、Qt/Node.js 同事件循环调度、后台输出暂停，以及 WebEngine renderer 常驻活动与快速恢复。

剩余：完成真实备用屏幕、鼠标、IME、Unicode 和长时间高输出验收，并在发布安装包与干净机上复验 WebEngine 部署。

验收：

- ADB shell 行为不回退。
- `vim`、全屏 TUI、中文输入、复制粘贴和 resize 正常。
- 所有 Web 资源离线随包加载。

## 6. M4 OpenCode 内嵌与结构化集成

状态：**进行中**

任务：

- 实现 Windows ConPTY 后端。
- 在选定 workspace 启动随包 OpenCode TUI。
- 接入 OpenCode Server/OpenAPI/SDK。
- 使用 loopback 动态端口和随机认证。
- 把会话、权限、问题和完成状态映射到产品 UI。
- 禁止通过终端文字判断 Agent 状态。

已完成：Windows Terminal 风格“+”终端类型菜单、OpenCode/ADB 标签隔离、`node-pty` ConPTY 宿主、输入/输出/resize/退出帧协议、自动化 ConPTY 冒烟，以及随包 OpenCode 1.18.5 基本交互链路手动验证。

剩余：完成 OpenCode TUI 的 IME、鼠标、备用屏幕和长时间高输出兼容性矩阵，并接入 Server/SDK 与认证。

验收：

- OpenCode TUI 的布局、命令面板、鼠标、滚动和中文输入正常。
- 产品能通过 API 提交提示词并获得结构化状态。
- 关闭标签和退出应用后无残留 ConPTY/OpenCode 进程。

## 7. M5 自动化运行时

状态：**待开始**

任务：

- 实现 Python Automation Service。
- 随包 Python、Node.js、JDK、Appium 和 UiAutomator2 driver。
- 实现本机 API、任务队列、事件流和进程监督。
- 创建 Appium Session 并运行最小点击用例。
- 采集截图、page source、logcat 和失败上下文。

验收：

- 干净电脑无需安装 Python/Appium/JDK/Node.js。
- UI 不阻塞，任务可取消，退出后无孤儿进程。
- 最小 Appium 用例能生成证据目录。

## 8. M6 附件与统一用例

状态：**待开始**

任务：

- 支持 `xlsx`、`csv`、`md`、`txt`。
- 支持 `docx` 和 `pdf` 文本抽取。
- 生成稳定统一用例 Schema。
- UI 展示解析摘要和错误。

验收：上传用例后可识别 ID、前置条件、步骤、期望结果和优先级。

## 9. M7 Agent 测试闭环

状态：**待开始**

任务：

- 脚本生成、UI 理解、错误修复和报告 Agent。
- OpenCode 会话与任务目录绑定。
- 直接配置并使用 OpenCode 自带的 plugins、skills、agents 和 tools，不开发宿主插件或 Skill 运行器。
- 设备上下文和附件上下文结构化输入。
- Runner 执行和最多 2 次自动修复。
- 权限和危险操作由用户确认。

验收：一句话加用例可以生成、执行、诊断并产出脚本差异。

## 10. M8 报告和回填

状态：**待开始**

任务：

- Markdown 报告。
- 成功率、失败原因、耗时和性能汇总。
- 截图和日志证据链接。
- Excel、Markdown、Word 回填。
- PDF 生成旁路报告。

验收：测试结束自动生成可提交报告，原始证据可追溯。

## 11. M9 正式发布

状态：**待开始**

任务：

- 代码签名、安装包和便携 ZIP。
- 完整 runtime manifest 和 notices。
- 干净机、升级、卸载和损坏修复测试。
- 组件版本升级回归矩阵。
- 诊断包导出。

验收：满足 [PORTABLE_RUNTIME.md](PORTABLE_RUNTIME.md) 的全部发布门禁。

## 12. 领域增强

便携发布和最小 AI 闭环稳定后再实现：

- 蓝牙连接耗时、断连重连和稳定性模板。
- Wi-Fi 切换、弱网和后台恢复。
- 眼镜设备识别和配对策略。
- 多设备并发和任务队列。
- 企业私有模型和真机云。
- 缺陷管理和团队报告。

## 13. 每阶段共同完成定义

- 有自动化或可重复冒烟步骤。
- 不引入系统 PATH 和开发机绝对路径依赖。
- 更新相关 docs 和 `CHANGELOG.md`。
- 新第三方组件完成版本锁定、校验和许可证记录。
- 失败有诊断信息，退出无残留进程。
