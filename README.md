# AI Mobile Test Studio

AI Mobile Test Studio 是一个基于 Qt6、Python、Appium 和 opencode 的移动端 AI 自动化测试平台。

## 仓库入口

- `apps/desktop/`：Qt6 桌面端。
- `services/automation/`：Python 自动化与 Agent 编排服务。
- `packages/contracts/`：C++ 与 Python 共享的数据契约。
- `resources/`：图片、图标和样式资源。
- `plugins/`：可安装插件。
- `skills/`：Agent Skill。
- `runtime/`：本地便携运行时的装配目录。
- `tests/`：C++、Python 和端到端测试。
- `tools/`：开发、打包和诊断脚本。
- `docs/`：项目设计与开发文档。

详细边界和依赖规则见 [项目目录结构](docs/PROJECT_STRUCTURE.md)。

## 构建桌面端

```powershell
cmake -S . -B build
cmake --build build
```
