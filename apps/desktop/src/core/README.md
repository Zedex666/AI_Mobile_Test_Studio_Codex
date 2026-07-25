# Desktop Core

放置桌面端领域模型、任务状态和不依赖 Qt Widgets 的业务规则。该层不得依赖 `ui/`。

`device_command_catalog.*` 保存设备控制页使用的电源命令和 Android KEYCODE 分类数据。目录同时覆盖内部 KEYCODE 文档和 QtAdb 的设备控制快捷键，并按导航、实体键、编辑、显示、媒体、系统应用、电视遥控等使用场景分类。
