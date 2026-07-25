# Desktop Services

放置 Bridge 客户端、设备会话客户端、附件客户端和任务事件流客户端。UI 通过这些接口访问后端能力。

当前 `ScrcpyService` 负责：

- 使用 scrcpy 同目录的 `adb.exe` 轮询设备状态。
- 设备连接后启动独立 scrcpy 镜像窗口。
- 跟踪进程状态并支持停止镜像。
- 将启动失败和异常退出反馈给 UI。

`AdbControlService` 负责：

- 将 KEYCODE 转换为参数化的 `adb shell input keyevent` 调用。
- 串行执行连续点击产生的命令队列。
- 执行经过确认的设备重启命令。
- 执行经过确认的 `adb shell reboot -p` 关机命令。
- 向设备控制页返回开始、成功和失败状态。
