# Appium Inspector Browser Assets

- Upstream: Appium Inspector 2026.5.1
- Source: https://github.com/appium/appium-inspector
- License: Apache License 2.0; see `LICENSE` in this directory.
- Build command: `npx vite build --base ./`

The generated browser bundle is stored locally so the desktop application does not download UI code at runtime. The bundled browser polyfill uses `./locales` so language files resolve correctly when loaded from the application directory.
