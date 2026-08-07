# Runtime

发布时在此装配应用私有的 Windows x64 便携运行时。当前锁定并自动装配：

- OpenCode、Node.js、npm 和 ABI 匹配的 `node-pty`。
- `conda-standalone`、OpenJDK 8。
- Android SDK command-line tools 和 platform-tools。
- Appium Server 和 UiAutomator2 driver。

开发构建的目标结构：

```text
runtime/
  manifest.json
  opencode/opencode.exe
  node/node.exe
  node/npm.cmd
  node/node_modules/node-pty/
  conda/conda.exe
  jdk/bin/java.exe
  android-sdk/cmdline-tools/latest/bin/sdkmanager.bat
  android-sdk/platform-tools/adb.exe
  appium/node_modules/appium/index.js
  appium/node_modules/appium-uiautomator2-driver/
  terminal-host/conpty_host.js
  terminal-web/index.html
```

Windows 构建默认根据 `tools/runtime/runtime-lock.json` 和锁定的 npm package lock 下载、校验并装配制品，将其 staging 到 `<build>/runtime-stage/windows-x64/`，再复制到可执行文件旁。下载缓存位于 `<build>/runtime-cache/`，应用启动时不会联网安装这些依赖。Android command-line tools 固定为仍可由 JDK 8 运行的 8.0；staging 会执行版本检查并拒绝错误或不匹配的输出。

## 隔离规则

- 应用使用随包可执行文件的绝对路径，不安装或覆盖用户电脑上的 Node.js、npm、Conda、JDK、Android SDK 或 Appium。
- 不写系统/用户 `PATH`、`JAVA_HOME`、`ANDROID_HOME`、npm 全局 prefix、Conda 配置或注册表。
- Appium 子进程才会收到私有运行时优先的 `PATH`、`JAVA_HOME`、`ANDROID_HOME` 和 `ANDROID_SDK_ROOT`。
- `APPIUM_HOME`、npm 缓存和 Conda 缓存/配置写入 `QStandardPaths::AppLocalDataLocation`，不使用用户已有的 `~/.appium`。
- 启动时先探测 `http://127.0.0.1:4723/status`。已有可用服务时不创建进程；没有服务时才运行随包 Appium。应用退出只停止自己启动的 Appium。

也可同时设置 `AI_MOBILE_TEST_OPENCODE_EXECUTABLE`、`AI_MOBILE_TEST_NODE_EXECUTABLE` 和 `AI_MOBILE_TEST_NODE_PTY_MODULE`，用本地锁定制品覆盖自动 staging。`AI_MOBILE_TEST_STAGE_TERMINAL_RUNTIME=OFF` 可显式关闭自动装配。

二进制运行时不提交到 Git，由打包工具下载或复制。
