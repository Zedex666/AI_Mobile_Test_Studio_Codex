# 编码规范

## 1. 总体原则

- 代码优先可读、可调试、可测试和可复现。
- 页面不拥有设备协议、外部进程和运行时发现逻辑。
- 所有跨进程接口使用结构化契约。
- 发布代码不依赖系统 PATH、开发机路径或启动时在线安装。
- 规划模块遵循本规范，但未实现前不制造空洞抽象。

## 2. C++/Qt 命名

- 类型：`PascalCase`，如 `TerminalService`。
- 函数：`camelCase`，如 `createSession()`。
- 成员：`m_` 前缀，如 `m_deviceSerial`。
- 常量：`k` 前缀，如 `kDefaultTimeoutMs`。
- 信号：事件或结果，如 `sessionStarted()`、`outputReady()`。
- 槽：动作，如 `restartSession()`、`handleProcessFinished()`。
- 文件：`snake_case.h/.cpp`。

## 3. 文件和模块

- 一个主要类型一组 `.h/.cpp`。
- 头文件放稳定接口，不放长算法实现。
- 超过单一职责的页面应拆出 widget、model 或 service。
- 协议解析、进程监督、manifest 和 terminal backend 不放进 UI 文件。
- `MainWindow` 不增长业务算法。

## 4. Qt 生命周期和线程

- QObject 优先使用父子生命周期。
- HANDLE、socket、ConPTY、文件和非 QObject 资源使用 RAII。
- lambda 捕获 QObject 指针时确保 context object 能取消连接。
- 跨线程 UI 更新通过 queued signal 回主线程。
- 禁止在 UI 线程调用长 `waitForFinished()`、同步网络或循环 sleep。
- 进程、server 和 session 都要定义 owner、停止顺序和强制终止兜底。

## 5. 错误类型

新增 service 优先返回或发出结构化错误：

```cpp
struct OperationError {
    QString code;
    QString message;
    QString detail;
    bool recoverable = false;
    QString action;
};
```

- `message` 面向用户。
- `detail` 面向诊断日志。
- `code` 稳定，可用于测试和协议。
- 不吞异常，不只返回 `false`。
- 原始 stderr 有长度上限，并落入对应任务或诊断日志。

## 6. 外部进程

- program 使用 `RuntimeLocator` 返回的绝对路径。
- arguments 使用字符串数组，不拼接 shell 命令。
- cwd 必须显式，尤其是 OpenCode 和测试 Runner。
- 环境由 `RuntimeManager` 生成，不修改系统永久环境。
- 日志记录 program component ID、参数摘要、cwd、PID、退出码和耗时。
- 参数和环境中的 Token、密码、Authorization 必须脱敏。
- 所有进程都有启动超时、运行取消和退出清理。

```cpp
process.setProgram(runtime.path(RuntimeComponent::Adb));
process.setArguments({QStringLiteral("-s"), serial, QStringLiteral("devices")});
process.setProcessEnvironment(runtime.environment(RuntimeRole::AdbClient));
```

## 7. 运行时路径

- 不写开发机绝对路径。
- 不通过 `where.exe`、注册表或系统 PATH 静默寻找发布依赖。
- 路径组合使用 `QDir`/`QFileInfo`，不手工连接分隔符。
- 安装目录视为只读。
- 用户配置、缓存和日志使用 `QStandardPaths`。
- runtime manifest 校验失败时明确失败，不自动下载替换。

## 8. 终端和二进制协议

- 终端输入输出使用 `QByteArray` 保持字节边界。
- UTF-8 解码只能发生在明确需要文本的显示或日志层。
- ADB packet 长度、packet id 和最大大小必须校验。
- partial frame 必须留在 buffer 等待下一次读取。
- resize 使用字符列/行，不使用像素。
- OpenCode TUI 必须运行在 ConPTY，不使用普通 QProcess 管道。
- xterm.js 输入、输出不剥离 ANSI，也不擅自转换换行。
- 高频输出需要背压、批量刷新和滚动缓冲上限。

## 9. UI

- 所有用户可感知错误有明确提示和恢复入口。
- 设备断开后立即禁用危险操作。
- 动态文本不改变固定工具栏和终端网格的稳定尺寸。
- 图标按钮有 tooltip；熟悉符号优于文字胶囊按钮。
- 高 DPI 下检查文字、关闭按钮、标签和图表不重叠。
- 终端快捷命令的危险等级可识别，高风险动作确认。

## 10. Python（规划模块）

- 模块和函数：`snake_case`；类型：`PascalCase`；常量：大写。
- 新代码使用类型标注。
- 跨模块数据使用 dataclass 或 Pydantic。
- 子进程统一由 runtime/process adapter 启动。
- 日志使用 JSON Lines 并带 `task_id`、`session_id`。
- 发布版不运行 pip，不读取全局 site-packages。

```python
@dataclass(frozen=True)
class DeviceContext:
    device_id: str
    activity: str | None
    screenshot_path: str | None
```

## 11. Appium 脚本（规划模块）

生成脚本必须包含显式等待、步骤日志、截图、异常上下文和结构化结果。

定位优先级：

1. accessibility id
2. resource-id
3. text
4. Android UIAutomator
5. XPath
6. 坐标兜底

坐标操作必须记录原因和目标分辨率。

## 12. 配置和凭据

- 默认配置可提交，用户配置不能提交。
- API Key、Token、账号密码不得出现在源码、manifest、任务 JSON 或普通日志。
- OpenCode Server 密码每次运行随机生成。
- workspace 权限和 Agent 权限策略必须可见、可审计。
- 测试 fixture 使用假密钥。

## 13. 文档

- 标题从一级标题开始。
- 代码块标注语言。
- 当前能力与目标能力明确区分。
- 修改协议、目录、运行时或终端架构时同步更新专项文档。
- 已发生变化写 `CHANGELOG.md`；计划写 `ROADMAP.md`，不要混用。
- 相对链接必须在仓库内可解析。

## 14. 审查清单

- 是否阻塞 UI 线程？
- 是否新增系统 PATH 或绝对路径依赖？
- 是否正确处理设备切换、取消和应用退出？
- 是否泄露凭据？
- 是否为外部二进制记录版本、校验和许可证？
- 是否有最小测试或可重复真机验证？
- 文档状态是否与代码一致？
