# Desktop App

Qt6 桌面应用负责用户交互、设备画面渲染、对话展示和任务状态展示。

`src/ui` 只处理界面；设备、Agent 和附件能力通过桌面端服务接口访问，避免 UI 直接启动 ADB、Appium 或 opencode 进程。

当前主窗口位于 `src/ui/windows/`。`src/ui/forms/main_window.ui` 是早期 Qt Designer 文件，暂不参与构建；当前界面由 C++ 代码构建。

## scrcpy

`src/services/scrcpy_service.*` 负责真实设备探测和 scrcpy 进程管理。当前开发机回退路径为：

```text
D:\myApp_666666666666666\scrcpy-win64-v4.0\scrcpy.exe
```

路径优先级：

1. 环境变量 `AI_MOBILE_TEST_SCRCPY_PATH`。
2. QSettings 中的 `runtime/scrcpyPath`。
3. 应用目录下 `runtime/scrcpy/scrcpy.exe`。
4. 当前开发机回退路径。
