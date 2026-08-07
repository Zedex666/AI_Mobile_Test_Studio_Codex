# UI 图标资源清单

本文档记录桌面端当前随包 PNG 图标、运行时加载约定以及后续新增和替换流程。图标资源的事实来源始终是 `resources/images/icons/`；本文档用于审查资源是否已经接入界面并随构建正确发布。

## 当前基线

- 基线日期：2026-08-04。
- PNG 图标总数：108。
- 已在界面中使用：101。
- 已随包保留：7。
- 源资源目录：`resources/images/icons/`。
- 构建及安装后的运行时目录：`runtime/images/icons/`。

“已使用”表示当前 C++ 界面代码会直接或按分类名称加载该文件；“预留”表示文件已经随包复制，但当前界面尚未引用。预留图标不应删除，后续对应入口实现后直接接入。

| 分类 | 总数 | 已使用 | 预留 |
| --- | ---: | ---: | ---: |
| 应用程序 | 1 | 1 | 0 |
| 标题栏 | 3 | 3 | 0 |
| 侧边栏 | 17 | 16 | 1 |
| 概览 | 15 | 15 | 0 |
| 显示 | 5 | 5 | 0 |
| 镜像 | 8 | 8 | 0 |
| 设备中心 | 4 | 4 | 0 |
| 设备控制 | 16 | 16 | 0 |
| 软件包管理器 | 6 | 6 | 0 |
| 文件 | 19 | 19 | 0 |
| 恢复 | 1 | 1 | 0 |
| 其它 | 6 | 6 | 0 |
| 设备 | 7 | 1 | 6 |
| 合计 | 108 | 101 | 7 |

## 完整资源清单

### 应用程序

| 文件 | 状态 |
| --- | --- |
| `resources/images/icons/app.png` | 已使用，应用窗口、任务栏与应用内标题栏品牌图标 |

### 标题栏

| 文件 | 状态 |
| --- | --- |
| `resources/images/icons/标题栏/设备中心.png` | 已使用 |
| `resources/images/icons/标题栏/设置.png` | 已使用 |
| `resources/images/icons/标题栏/通知.png` | 已使用 |

### 侧边栏

| 文件 | 状态 |
| --- | --- |
| `resources/images/icons/侧边栏/关于.png` | 已使用 |
| `resources/images/icons/侧边栏/其它.png` | 已使用 |
| `resources/images/icons/侧边栏/对话.png` | 预留 |
| `resources/images/icons/侧边栏/布局.png` | 已使用 |
| `resources/images/icons/侧边栏/应用.png` | 已使用 |
| `resources/images/icons/侧边栏/性能.png` | 已使用 |
| `resources/images/icons/侧边栏/恢复.png` | 已使用 |
| `resources/images/icons/侧边栏/文件.png` | 已使用 |
| `resources/images/icons/侧边栏/日志.png` | 已使用 |
| `resources/images/icons/侧边栏/显示.png` | 已使用 |
| `resources/images/icons/侧边栏/概览.png` | 已使用 |
| `resources/images/icons/侧边栏/终端.png` | 已使用 |
| `resources/images/icons/侧边栏/设备控制.png` | 已使用 |
| `resources/images/icons/侧边栏/设置.png` | 已使用 |
| `resources/images/icons/侧边栏/软件包管理器.png` | 已使用 |
| `resources/images/icons/侧边栏/进程.png` | 已使用 |
| `resources/images/icons/侧边栏/镜像.png` | 已使用 |

### 概览

| 文件 | 状态 |
| --- | --- |
| `resources/images/icons/概览/ram.png` | 已使用 |
| `resources/images/icons/概览/产品.png` | 已使用 |
| `resources/images/icons/概览/代号.png` | 已使用 |
| `resources/images/icons/概览/内核.png` | 已使用 |
| `resources/images/icons/概览/分辨率.png` | 已使用 |
| `resources/images/icons/概览/制造商.png` | 已使用 |
| `resources/images/icons/概览/品牌.png` | 已使用 |
| `resources/images/icons/概览/型号.png` | 已使用 |
| `resources/images/icons/概览/存储.png` | 已使用 |
| `resources/images/icons/概览/安卓.png` | 已使用 |
| `resources/images/icons/概览/序列号.png` | 已使用 |
| `resources/images/icons/概览/架构.png` | 已使用 |
| `resources/images/icons/概览/电池.png` | 已使用 |
| `resources/images/icons/概览/类型.png` | 已使用 |
| `resources/images/icons/概览/运行时间.png` | 已使用 |

### 显示

| 文件 | 状态 |
| --- | --- |
| `resources/images/icons/显示/最小宽度.png` | 已使用 |
| `resources/images/icons/显示/浅色.png` | 已使用 |
| `resources/images/icons/显示/深色.png` | 已使用 |
| `resources/images/icons/显示/物理分辨率.png` | 已使用 |
| `resources/images/icons/显示/物理密度.png` | 已使用 |

### 镜像

| 文件 | 状态 |
| --- | --- |
| `resources/images/icons/镜像/主屏幕.png` | 已使用 |
| `resources/images/icons/镜像/启动应用.png` | 已使用 |
| `resources/images/icons/镜像/图像.png` | 已使用 |
| `resources/images/icons/镜像/录制.png` | 已使用 |
| `resources/images/icons/镜像/摄像头.png` | 已使用 |
| `resources/images/icons/镜像/虚拟屏幕.png` | 已使用 |
| `resources/images/icons/镜像/输入与声音.png` | 已使用 |
| `resources/images/icons/镜像/高级参数.png` | 已使用 |

### 设备中心

| 文件 | 状态 |
| --- | --- |
| `resources/images/icons/设备中心/删除离线设备.png` | 已使用 |
| `resources/images/icons/设备中心/刷新.png` | 已使用 |
| `resources/images/icons/设备中心/断开设备.png` | 已使用 |
| `resources/images/icons/设备中心/无线模式.png` | 已使用 |

### 设备控制

| 文件 | 状态 |
| --- | --- |
| `resources/images/icons/设备控制/修饰与组合.png` | 已使用 |
| `resources/images/icons/设备控制/功能按键.png` | 已使用 |
| `resources/images/icons/设备控制/多媒体控制.png` | 已使用 |
| `resources/images/icons/设备控制/字母与数字.png` | 已使用 |
| `resources/images/icons/设备控制/实体按键.png` | 已使用 |
| `resources/images/icons/设备控制/小键盘.png` | 已使用 |
| `resources/images/icons/设备控制/控制与编辑.png` | 已使用 |
| `resources/images/icons/设备控制/显示与休眠.png` | 已使用 |
| `resources/images/icons/设备控制/游戏手柄.png` | 已使用 |
| `resources/images/icons/设备控制/特殊按键.png` | 已使用 |
| `resources/images/icons/设备控制/电源.png` | 已使用 |
| `resources/images/icons/设备控制/电视与遥控.png` | 已使用 |
| `resources/images/icons/设备控制/电话按键.png` | 已使用 |
| `resources/images/icons/设备控制/符号按键.png` | 已使用 |
| `resources/images/icons/设备控制/系统导航.png` | 已使用 |
| `resources/images/icons/设备控制/系统应用.png` | 已使用 |

设备控制页面按分类标题拼接 `icons/设备控制/<分类>.png`，因此分类标题、文件名和翻译前的中文键必须保持一致。

### 软件包管理器

| 文件 | 状态 |
| --- | --- |
| `resources/images/icons/软件包管理器/安装应用.png` | 已使用 |
| `resources/images/icons/软件包管理器/已知权限组.png` | 已使用 |
| `resources/images/icons/软件包管理器/库.png` | 已使用 |
| `resources/images/icons/软件包管理器/用户.png` | 已使用 |
| `resources/images/icons/软件包管理器/系统功能.png` | 已使用 |
| `resources/images/icons/软件包管理器/软件包.png` | 已使用 |

### 文件

| 文件 | 状态 |
| --- | --- |
| `resources/images/icons/文件/上一级.png` | 已使用 |
| `resources/images/icons/文件/上传.png` | 已使用 |
| `resources/images/icons/文件/下载.png` | 已使用 |
| `resources/images/icons/文件/修改权限.png` | 已使用 |
| `resources/images/icons/文件/内部存储.png` | 已使用 |
| `resources/images/icons/文件/列表视图.png` | 已使用 |
| `resources/images/icons/文件/创建副本.png` | 已使用 |
| `resources/images/icons/文件/删除.png` | 已使用 |
| `resources/images/icons/文件/刷新.png` | 已使用 |
| `resources/images/icons/文件/前进.png` | 已使用 |
| `resources/images/icons/文件/后退.png` | 已使用 |
| `resources/images/icons/文件/已保存位置.png` | 已使用 |
| `resources/images/icons/文件/新建 .png` | 已使用 |
| `resources/images/icons/文件/根目录.png` | 已使用 |
| `resources/images/icons/文件/网格视图.png` | 已使用 |
| `resources/images/icons/文件/设备主页.png` | 已使用 |
| `resources/images/icons/文件/设备驱动器.png` | 已使用 |
| `resources/images/icons/文件/详情.png` | 已使用 |
| `resources/images/icons/文件/重命名.png` | 已使用 |

### 恢复

| 文件 | 状态 |
| --- | --- |
| `resources/images/icons/恢复/侧载.png` | 已使用 |

### 其它

| 文件 | 状态 |
| --- | --- |
| `resources/images/icons/其它/分级调节振动强度.png` | 已使用 |
| `resources/images/icons/其它/去除叹号.png` | 已使用 |
| `resources/images/icons/其它/状态栏与导航栏.png` | 已使用 |
| `resources/images/icons/其它/自定义命令.png` | 已使用 |
| `resources/images/icons/其它/账户.png` | 已使用 |
| `resources/images/icons/其它/过渡动画.png` | 已使用 |

### 设备

| 文件 | 状态 |
| --- | --- |
| `resources/images/icons/设备/手机.png` | 已使用，顶部标题栏设备选择器 |
| `resources/images/icons/设备/平板电脑.png` | 预留 |
| `resources/images/icons/设备/手表.png` | 预留 |
| `resources/images/icons/设备/智能眼镜.png` | 预留 |
| `resources/images/icons/设备/机器人.png` | 预留 |
| `resources/images/icons/设备/汽车相关-车机.png` | 预留 |
| `resources/images/icons/设备/电视机.png` | 预留 |

## 加载与发布约定

界面统一使用 `apps/desktop/src/ui/common/widget_helpers.h` 中的接口：

- `ui::imageResourcePath(relativePath)`：将相对于 `runtime/images/` 的路径解析为可执行文件旁的绝对路径。
- `ui::imageIcon(relativePath)`：加载 `QIcon`，文件不存在时返回空图标。
- `ui::imagePixmap(relativePath, size)`：按目标尺寸生成 `QPixmap`。

调用方传入 `icons/` 开头的相对路径，例如 `icons/侧边栏/概览.png`。界面代码不得写死开发机绝对路径，也不得绕过统一接口直接依赖源资源目录。

`apps/desktop/CMakeLists.txt` 将图标目录加入链接依赖，并执行以下两种复制：

- 普通构建：链接完成后复制到可执行文件旁的 `runtime/images/icons/`。
- 安装构建：安装到安装根目录的 `runtime/images/icons/`。

## 新增或替换流程

1. 将 PNG 放入现有语义分类目录；只有出现新的稳定工作区或对象类别时才新建目录。
2. 文件名使用稳定、可读的界面语义名称，保持 `.png` 小写后缀；不要使用 `新建文件`、数字副本或仅大小写不同的名称。
3. 在界面中通过 `ui::imageIcon()` 或 `ui::imagePixmap()` 接入。按标题动态拼接路径的页面还要保证标题键与文件名一致。
4. 新增资源后重新构建目标，确认源文件被复制到最新可执行文件旁的 `runtime/images/icons/`。替换同名文件也必须重新构建，避免运行旧副本。
5. 同步更新本文档中的分类数量、总数、状态和完整清单；已真正接入界面后再将状态标为“已使用”。
6. 在 `docs/CHANGELOG.md` 的 `Unreleased` 中记录用户可见的图标新增或整体替换。
7. 启动最新构建，至少检查侧边栏、目标主工作区、浅色背景、禁用状态和高 DPI 缩放下的清晰度；确认没有空图标、错误占位、裁切或文字挤压。

替换图标时优先保留原文件名和语义，这样无需修改调用代码。若文件名必须变化，应在同一变更中更新所有代码引用、本文档和运行时副本。

## 资源一致性检查

源目录统计：

```powershell
(Get-ChildItem -LiteralPath 'resources/images/icons' -Recurse -File -Filter '*.png').Count
```

构建后应比较源目录和运行时目录的相对路径及 SHA-256。两边文件数、相对路径集合和同名文件哈希必须完全一致；任何缺失或哈希差异都表示正在运行的不是最新资源副本。
