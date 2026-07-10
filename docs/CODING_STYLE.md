# C++/Python 编码规范

## 1. 总体原则

- 代码优先可读、可调试、可复现。
- 所有跨进程、跨语言接口必须有结构化数据契约。
- UI 层不写设备自动化细节。
- Agent 层不直接操作设备，必须通过 Device Controller。
- Python 运行时不假设用户系统已安装任何依赖。

## 2. C++/Qt 编码规范

### 2.1 命名

- 类名使用 `PascalCase`：`DevicePanel`、`ChatPanel`。
- 函数名使用 `camelCase`：`startTask()`、`loadAttachment()`。
- 成员变量使用 `m_` 前缀：`m_deviceId`、`m_taskClient`。
- 常量使用 `k` 前缀：`kDefaultTimeoutMs`。
- 信号使用过去式或事件式：`taskStarted()`、`deviceDisconnected()`。
- 槽函数表达动作：`onSendClicked()`、`handleTaskEvent()`。

### 2.2 文件组织

每个主要类使用独立 `.h` 和 `.cpp` 文件。

```text
device_panel.h
device_panel.cpp
chat_panel.h
chat_panel.cpp
```

头文件只放接口，不放复杂实现。

### 2.3 Qt 对象管理

- 优先使用 Qt 父子对象生命周期。
- 非 QObject 资源使用 RAII。
- 长任务使用 `QThread`、`QProcess` 或本地 bridge，不阻塞主线程。
- 跨线程 UI 更新必须回到主线程。

示例：

```cpp
connect(client, &TaskClient::eventReceived,
        this, &TaskPanel::handleTaskEvent);
```

### 2.4 UI 规则

- UI 层只展示状态，不决策测试逻辑。
- 所有用户可感知的失败都要有明确提示。
- 长任务必须展示进度或状态。
- 对话框中展示的 Agent 输出应区分“计划、执行、结果、错误”。

### 2.5 错误处理

C++ 层建议使用明确返回类型：

```cpp
struct Result {
    bool ok;
    QString message;
};
```

对于本地 API 调用，必须处理：

- 超时。
- 连接失败。
- JSON 解析失败。
- bridge 返回错误。
- 任务被取消。

## 3. Python 编码规范

### 3.1 命名

- 模块名使用 `snake_case`：`device_controller.py`。
- 类名使用 `PascalCase`：`DeviceController`。
- 函数和变量使用 `snake_case`：`get_page_source()`。
- 常量使用大写：`DEFAULT_TIMEOUT_SECONDS`。

### 3.2 类型标注

新增 Python 代码必须尽量添加类型标注。

```python
from pathlib import Path

def create_task_dir(workspace: Path, task_name: str) -> Path:
    ...
```

### 3.3 数据模型

跨模块数据优先使用 `dataclass` 或 Pydantic 模型。

```python
from dataclasses import dataclass

@dataclass(frozen=True)
class DeviceContext:
    device_id: str
    activity: str | None
    page_source_path: str | None
    screenshot_path: str | None
```

### 3.4 子进程调用

所有工具调用集中封装，不在业务代码中散落 `subprocess`。

封装层必须记录：

- 命令。
- 工作目录。
- 环境变量摘要。
- 退出码。
- stdout 路径。
- stderr 路径。
- 耗时。

禁止把用户输入直接拼接进 shell 字符串。使用参数数组。

```python
subprocess.run(
    ["adb", "-s", device_id, "shell", "dumpsys", "activity"],
    cwd=work_dir,
    env=env,
    check=False,
    text=True,
    capture_output=True,
)
```

### 3.5 Appium 脚本规范

生成的测试脚本必须包含：

- 设备能力配置。
- 显式等待。
- 截图辅助函数。
- 步骤日志。
- 异常上下文采集。
- 用例结果输出。

定位控件优先级：

1. accessibility id
2. resource-id
3. text
4. Android UIAutomator
5. XPath
6. 坐标点击

坐标点击只能作为兜底，并且必须写明原因。

### 3.6 日志

Python 使用标准 `logging`，日志输出 JSON Lines。

```python
logger.info(
    "step_finished",
    extra={"task_id": task_id, "case_id": case_id, "elapsed_ms": elapsed_ms},
)
```

### 3.7 异常

异常需要转成明确错误类型：

- `DeviceNotFoundError`
- `AppiumSessionError`
- `ElementNotFoundError`
- `AgentCommandError`
- `AttachmentParseError`
- `ReportGenerateError`

不要吞掉异常。捕获后必须记录上下文。

## 4. Markdown 文档规范

- 标题从一级标题开始。
- 每个文档说明目标和适用范围。
- 代码块标注语言。
- 表格用于协议字段和状态码。
- 变更协议时同步更新 `CHANGELOG.md`。

## 5. 配置文件规范

配置文件优先使用 JSON 或 TOML。

敏感字段不得明文提交：

- API Key
- Token
- 账号密码
- 公司内部设备信息
- 未脱敏日志

建议配置分层：

```text
config/default.toml
config/local.toml
workspace/tasks/<taskId>/task.json
```

## 6. 代码审查清单

提交前检查：

- 是否阻塞 UI 线程。
- 是否依赖用户系统 PATH。
- 是否记录了任务 ID。
- 是否能在失败时保存截图和日志。
- 是否更新了相关文档。
- 是否有最小测试覆盖。
- 是否避免把敏感信息传给 Agent。

