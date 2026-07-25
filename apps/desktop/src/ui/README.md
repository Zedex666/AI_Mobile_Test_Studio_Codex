# Desktop UI

- `windows/`：顶层窗口和页面组装。
- `components/`：窗口级区域和组合组件。
- `pages/`：可切换的主工作区页面。
- `widgets/`：可复用、自绘或领域专用控件。
- `common/`：字体、文本和基础控件帮助函数。
- `styles/`：应用级 QSS 样式。
- `forms/`：确实需要 Qt Designer 时使用的 `.ui` 文件。

窗口负责组合控件，不直接执行设备命令或测试任务。
