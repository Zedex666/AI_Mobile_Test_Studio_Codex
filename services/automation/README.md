# Automation Service

Python 本地服务负责 Appium、ADB、scrcpy、opencode、附件解析、脚本运行和报告生成。

开发模式安装：

```powershell
python -m pip install -e services/automation
```

模块边界：

- `api/`：面向 Qt 的 HTTP、WebSocket 或本地 IPC 接口。
- `agents/`：四类 Agent 及编排状态机。
- `attachments/`：附件解析与结果回填。
- `device/`：ADB、Appium 和设备观测。
- `runner/`：测试脚本执行和重试。
- `reports/`：报告数据聚合与导出。
- `runtime/`：内置工具链发现、启动和健康检查。
