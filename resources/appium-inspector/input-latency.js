(() => {
  'use strict';

  window.__aiMobileTestStudioInputLatency = true;

  const replayDelayMs = 80;
  const pendingInputs = new Map();
  const composingInputs = new WeakSet();

  const flushInput = (target) => {
    if (composingInputs.has(target)) {
      return;
    }
    const pending = pendingInputs.get(target);
    if (!pending) {
      return;
    }
    window.clearTimeout(pending.timer);
    pendingInputs.delete(target);
    if (!target.isConnected) {
      return;
    }

    const event = new InputEvent('input', {
      bubbles: true,
      composed: true,
      data: pending.data,
      inputType: pending.inputType
    });
    Object.defineProperty(event, '__aiMobileTestStudioReplay', { value: true });
    target.dispatchEvent(event);
  };

  const flushAllInputs = () => {
    for (const target of Array.from(pendingInputs.keys())) {
      flushInput(target);
    }
  };

  document.addEventListener('input', (event) => {
    if (event.__aiMobileTestStudioReplay) {
      return;
    }
    const target = event.target;
    if (!(target instanceof HTMLInputElement || target instanceof HTMLTextAreaElement)
        || !target.matches('input.ant-input, textarea.ant-input')) {
      return;
    }

    event.stopImmediatePropagation();
    const previous = pendingInputs.get(target);
    if (previous) {
      window.clearTimeout(previous.timer);
    }
    const pending = {
      data: event.data,
      inputType: event.inputType,
      timer: 0
    };
    if (event.isComposing || composingInputs.has(target)) {
      pendingInputs.set(target, pending);
      return;
    }
    pending.timer = window.setTimeout(() => flushInput(target), replayDelayMs);
    pendingInputs.set(target, pending);
  }, true);

  document.addEventListener('pointerdown', flushAllInputs, true);
  document.addEventListener('keydown', (event) => {
    if (event.key === 'Enter' || event.key === 'Tab') {
      flushAllInputs();
    }
  }, true);
  document.addEventListener('compositionstart', (event) => {
    composingInputs.add(event.target);
    const pending = pendingInputs.get(event.target);
    if (pending) {
      window.clearTimeout(pending.timer);
      pending.timer = 0;
    }
  }, true);
  document.addEventListener('compositionend', (event) => {
    composingInputs.delete(event.target);
    window.setTimeout(() => flushInput(event.target), 0);
  }, true);
  document.addEventListener('focusout', (event) => flushInput(event.target), true);
})();
