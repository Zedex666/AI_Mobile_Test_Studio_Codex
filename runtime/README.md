# Runtime

发布时在此装配便携版 Python、Node.js、`node-pty`、JDK、Appium、Android platform-tools、scrcpy 和 OpenCode。

终端开发构建的目标位置：

```text
runtime/
  opencode/opencode.exe
  node/node.exe
  node/node_modules/node-pty/
  terminal-host/conpty_host.js
  terminal-web/index.html
```

Windows 构建默认根据 `tools/runtime/runtime-lock.json` 下载并校验终端制品，将其 staging 到 `<build>/runtime-stage/windows-x64/`，再复制到可执行文件旁。下载缓存位于 `<build>/runtime-cache/`，应用启动时不会联网获取依赖。

也可同时设置 `AI_MOBILE_TEST_OPENCODE_EXECUTABLE`、`AI_MOBILE_TEST_NODE_EXECUTABLE` 和 `AI_MOBILE_TEST_NODE_PTY_MODULE`，用本地锁定制品覆盖自动 staging。`AI_MOBILE_TEST_STAGE_TERMINAL_RUNTIME=OFF` 可显式关闭自动装配。

二进制运行时不提交到 Git，由打包工具下载或复制。
