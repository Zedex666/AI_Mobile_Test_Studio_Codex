'use strict';

const FRAME_HEADER_SIZE = 5;
const MAX_FRAME_SIZE = 16 * 1024 * 1024;

function fail(error) {
  const message = error instanceof Error ? error.message : String(error);
  process.stderr.write(`ERROR ${Buffer.from(message, 'utf8').toString('base64')}\n`);
  process.exitCode = 1;
}

function parseArguments(argv) {
  const result = { args: [], cols: 120, rows: 36 };
  for (let index = 0; index < argv.length; index += 1) {
    const key = argv[index];
    const value = argv[index + 1];
    if (key === '--arg') {
      result.args.push(value ?? '');
      index += 1;
    } else if (key === '--file' || key === '--cwd' || key === '--module'
               || key === '--cols' || key === '--rows') {
      result[key.slice(2)] = value;
      index += 1;
    }
  }
  result.cols = Number.parseInt(result.cols, 10);
  result.rows = Number.parseInt(result.rows, 10);
  if (!result.file || !result.cwd || !result.module
      || !Number.isInteger(result.cols) || !Number.isInteger(result.rows)) {
    throw new Error('Invalid terminal host arguments');
  }
  return result;
}

try {
  const options = parseArguments(process.argv.slice(2));
  const pty = require(options.module);
  const terminal = pty.spawn(options.file, options.args, {
    cols: options.cols,
    rows: options.rows,
    cwd: options.cwd,
    name: 'xterm-256color',
    env: {
      ...process.env,
      TERM: 'xterm-256color',
      COLORTERM: 'truecolor',
      FORCE_COLOR: '1'
    },
    useConptyDll: false
  });

  let ready = false;
  terminal.onData((data) => {
    if (!ready) {
      ready = true;
      process.stderr.write('READY\n');
    }
    if (!process.stdout.write(Buffer.from(data, 'utf8'))) {
      terminal.pause();
      process.stdout.once('drain', () => terminal.resume());
    }
  });
  terminal.onExit(({ exitCode }) => {
    process.exit(exitCode ?? 0);
  });

  let input = Buffer.alloc(0);
  process.stdin.on('data', (chunk) => {
    input = Buffer.concat([input, chunk]);
    while (input.length >= FRAME_HEADER_SIZE) {
      const type = String.fromCharCode(input[0]);
      const size = input.readUInt32LE(1);
      if (size > MAX_FRAME_SIZE) {
        throw new Error(`Terminal frame is too large: ${size}`);
      }
      if (input.length < FRAME_HEADER_SIZE + size) {
        return;
      }
      const payload = input.subarray(FRAME_HEADER_SIZE, FRAME_HEADER_SIZE + size);
      input = input.subarray(FRAME_HEADER_SIZE + size);
      if (type === 'i') {
        terminal.write(payload.toString('utf8'));
      } else if (type === 'r' && payload.length === 8) {
        terminal.resize(payload.readUInt32LE(0), payload.readUInt32LE(4));
      } else if (type === 'x') {
        terminal.kill();
      }
    }
  });
  process.stdin.on('end', () => terminal.kill());
  process.on('SIGTERM', () => {
    terminal.kill();
    process.exit(0);
  });
} catch (error) {
  fail(error);
}
