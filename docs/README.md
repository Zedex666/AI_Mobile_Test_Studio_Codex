# 文档导航

本目录是 AI Mobile Test Studio 的工程事实和目标设计入口。文档基线日期为 2026-07-26。

## 状态约定

- **已实现**：当前仓库已有可运行代码，并经过至少一次构建或真机验证。
- **进行中**：代码骨架存在，但能力尚未形成完整闭环。
- **规划中**：目标已经确定，尚未实现；不得在产品说明中描述为现有能力。
- **决策**：后续实现应遵守的架构约束，变更时需要同步修改相关文档。

## 当前项目快照

当前可运行主体是 `apps/desktop/` 下的 Qt 6 Widgets 桌面应用。已经实现：

- Android 设备发现、连接状态和 scrcpy 镜像进程管理。
- 持久 ADB `shell,v2` 终端、多标签、输入输出、尺寸同步和快捷命令。
- ADB/OpenCode 统一终端会话、新建类型菜单和 `node-pty` ConPTY 宿主。
- 随包 xterm.js/QWebChannel 前端资源；Qt WebEngine 依赖完整时自动启用，其他工具链使用基础显示降级。
- 设备按键控制、软件包管理、应用管理、文件管理、Recovery sideload。
- 电池、CPU、内存、温度和前台应用 FPS 性能观测。
- Android 应用名称与图标元数据提取器。

尚未实现完整产品闭环的部分：

- Python 自动化服务、Appium Server 和测试 Runner。
- 完整 Qt WebEngine 部署、OpenCode 交互式 TUI 端到端验收和其余便携运行时装配。
- OpenCode Server/SDK 和 Agent 编排。
- 完整便携运行时装配、安装包、自检、修复和第三方许可证产物。

项目不实现独立插件系统或 Skill 运行器。AI 扩展统一使用 OpenCode 自带的 plugins、skills、agents 和 tools 机制，宿主只负责运行时装配、进程隔离、Server/SDK 接入和权限展示。

## 文档索引

| 文档 | 用途 |
| --- | --- |
| [PROJECT_DESIGN.md](PROJECT_DESIGN.md) | 产品定位、当前能力、目标工作流和成功标准 |
| [ARCHITECTURE.md](ARCHITECTURE.md) | 当前与目标系统架构、进程和数据边界 |
| [TERMINAL_ARCHITECTURE.md](TERMINAL_ARCHITECTURE.md) | ADB 终端、xterm.js、ConPTY 和 OpenCode 集成决策 |
| [PORTABLE_RUNTIME.md](PORTABLE_RUNTIME.md) | 无需终端用户安装工具的运行时装配与隔离规范 |
| [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md) | 仓库真实结构、模块所有权和依赖方向 |
| [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md) | 构建、调试、运行时更新和验收流程 |
| [CODING_STYLE.md](CODING_STYLE.md) | C++、Python、进程、终端和文档编码规范 |
| [ROADMAP.md](ROADMAP.md) | 按依赖顺序排列的实施路线图 |
| [CHANGELOG.md](CHANGELOG.md) | 已发生的工程变更，不记录尚未完成的功能 |

## 文档维护规则

1. 代码行为与文档冲突时，先核实代码，再在同一变更中修正文档。
2. 新增外部二进制时必须同步更新 `PORTABLE_RUNTIME.md`、版本清单和许可证说明。
3. 修改终端输入输出、PTY 或 OpenCode 集成时必须同步更新 `TERMINAL_ARCHITECTURE.md`。
4. 规划中的模块落地后，应将对应状态改为“已实现”，并在 `CHANGELOG.md` 记录。
5. 文档中的路径使用仓库相对路径；用户数据路径使用 `QStandardPaths` 概念，不写死开发机绝对路径。
6. OpenCode 扩展遵循其官方机制，不在本仓库重新设计 Plugin/Skill 协议。
