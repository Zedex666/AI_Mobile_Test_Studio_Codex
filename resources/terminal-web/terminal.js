(() => {
  'use strict';

  const terminal = new Terminal({
    allowProposedApi: false,
    convertEol: false,
    cursorBlink: true,
    cursorStyle: 'bar',
    fontFamily: '"AI JetBrains Mono", "AI LXGW WenKai", monospace',
    fontSize: 15,
    letterSpacing: 0,
    lineHeight: 1.15,
    scrollback: 5000,
    smoothScrollDuration: 0,
    theme: {
      background: '#0c0c0c',
      foreground: '#f2f2f2',
      cursor: '#f2f2f2',
      cursorAccent: '#0c0c0c',
      selectionBackground: '#264f78',
      black: '#0c0c0c',
      red: '#c50f1f',
      green: '#13a10e',
      yellow: '#c19c00',
      blue: '#3b78ff',
      magenta: '#881798',
      cyan: '#3a96dd',
      white: '#cccccc',
      brightBlack: '#767676',
      brightRed: '#e74856',
      brightGreen: '#16c60c',
      brightYellow: '#f9f1a5',
      brightBlue: '#61a5ff',
      brightMagenta: '#b4009e',
      brightCyan: '#61d6d6',
      brightWhite: '#f2f2f2'
    }
  });
  const fitAddon = new FitAddon.FitAddon();
  terminal.loadAddon(fitAddon);
  terminal.open(document.getElementById('terminal'));

  let bridge = null;
  let resizeFrame = 0;
  let pendingInput = '';
  let inputFlushScheduled = false;
  const fit = () => {
    cancelAnimationFrame(resizeFrame);
    resizeFrame = requestAnimationFrame(() => {
      fitAddon.fit();
    });
  };

  const decodeBase64 = (encoded) => {
    const binary = atob(encoded);
    const data = new Uint8Array(binary.length);
    for (let index = 0; index < binary.length; index += 1) {
      data[index] = binary.charCodeAt(index);
    }
    return data;
  };

  const flushInput = () => {
    inputFlushScheduled = false;
    if (!bridge || pendingInput.length === 0) {
      return;
    }
    const data = pendingInput;
    pendingInput = '';
    bridge.writeInput(data);
  };

  const sendInputNow = (data) => {
    flushInput();
    if (bridge) {
      bridge.writeInput(data);
    } else {
      pendingInput += data;
    }
  };

  const queueInput = (data) => {
    const firstCodePoint = data.codePointAt(0);
    const controlInput = data.length === 1
      && (firstCodePoint < 0x20 || firstCodePoint === 0x7f);
    const unicodeInput = /[^\x20-\x7e]/u.test(data);
    if (controlInput || data.startsWith('\x1b') || unicodeInput) {
      sendInputNow(data);
      return;
    }
    pendingInput += data;
    if (pendingInput.length >= 4096) {
      flushInput();
    } else if (!inputFlushScheduled) {
      inputFlushScheduled = true;
      queueMicrotask(flushInput);
    }
  };

  window.terminalHost = {
    clear() {
      terminal.clear();
      terminal.reset();
    },
    copySelection() {
      if (bridge && terminal.hasSelection()) {
        bridge.copyText(terminal.getSelection());
      }
    },
    paste() {
      if (bridge) {
        bridge.pasteClipboard();
      }
    },
    focus() {
      terminal.focus();
    }
  };

  terminal.attachCustomKeyEventHandler((event) => {
    if (event.type !== 'keydown' || !event.ctrlKey || !event.shiftKey) {
      return true;
    }
    if (event.code === 'KeyC') {
      window.terminalHost.copySelection();
      return false;
    }
    if (event.code === 'KeyV') {
      window.terminalHost.paste();
      return false;
    }
    return true;
  });

  terminal.onData((data) => {
    queueInput(data);
  });
  terminal.onResize((size) => {
    if (bridge) {
      bridge.resizeTerminal(size.cols, size.rows);
    }
  });

  new ResizeObserver(fit).observe(document.body);
  window.addEventListener('load', fit, { once: true });
  document.fonts.ready.then(fit);

  new QWebChannel(qt.webChannelTransport, (channel) => {
    bridge = channel.objects.terminalBridge;
    flushInput();
    terminal.options.fontFamily = bridge.fontFamily;
    bridge.fontFamilyChanged.connect((fontFamily) => {
      terminal.options.fontFamily = fontFamily;
      fit();
    });
    bridge.outputData.connect((encoded) => {
      terminal.write(decodeBase64(encoded), () => bridge.outputConsumed());
    });
    bridge.statusMessage.connect((message) => {
      terminal.write(`\r\n\x1b[38;5;245m[${message}]\x1b[0m\r\n`);
    });
    bridge.clearRequested.connect(() => window.terminalHost.clear());
    bridge.focusRequested.connect(() => window.terminalHost.focus());
    bridge.pasteData.connect((text) => terminal.paste(text));
    bridge.sessionReadyChanged.connect((ready) => {
      terminal.options.disableStdin = !ready;
    });
    fit();
    bridge.frontendReady(terminal.cols, terminal.rows);
  });
})();
