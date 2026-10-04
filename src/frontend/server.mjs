import { spawn } from 'node:child_process';
import { access, mkdtemp, readFile, rm, writeFile } from 'node:fs/promises';
import { createServer } from 'node:http';
import { tmpdir } from 'node:os';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const frontendDirectory = dirname(fileURLToPath(import.meta.url));
const compilerPath = resolve(process.env.COMPILER_PATH || resolve(frontendDirectory, '../compiler'));
const port = Number(process.env.PORT || 4173);
const host = process.env.HOST || '127.0.0.1';
const allowedOrigin = process.env.ALLOWED_ORIGIN;
const maxSourceBytes = 64 * 1024;
const maxOutputBytes = 1024 * 1024;

function sendJson(response, status, body) {
  response.writeHead(status, {
    'content-type': 'application/json; charset=utf-8',
    'cache-control': 'no-store',
  });
  response.end(JSON.stringify(body));
}

function sendApiJson(request, response, status, body) {
  const origin = request.headers.origin;
  if (allowedOrigin && origin === allowedOrigin) {
    response.setHeader('access-control-allow-origin', allowedOrigin);
    response.setHeader('vary', 'Origin');
  }
  sendJson(response, status, body);
}

async function readRequestBody(request) {
  const chunks = [];
  let size = 0;
  for await (const chunk of request) {
    size += chunk.length;
    if (size > maxSourceBytes) {
      throw new Error('Your source file is over the 64 KB limit.');
    }
    chunks.push(chunk);
  }
  return Buffer.concat(chunks).toString('utf8');
}

function runProcess(command, args, cwd, timeoutMs) {
  return new Promise((resolveProcess, rejectProcess) => {
    const child = spawn(command, args, { cwd, stdio: ['ignore', 'pipe', 'pipe'] });
    let stdout = '';
    let stderr = '';
    let outputSize = 0;
    let timedOut = false;
    let outputLimited = false;
    const timer = setTimeout(() => {
      timedOut = true;
      child.kill('SIGKILL');
    }, timeoutMs);

    const collect = (target) => (chunk) => {
      outputSize += chunk.length;
      if (outputSize > maxOutputBytes) {
        outputLimited = true;
        child.kill('SIGKILL');
        return;
      }
      if (target === 'stdout') stdout += chunk.toString();
      else stderr += chunk.toString();
    };

    child.stdout.on('data', collect('stdout'));
    child.stderr.on('data', collect('stderr'));
    child.on('error', (error) => {
      clearTimeout(timer);
      rejectProcess(error);
    });
    child.on('close', (code, signal) => {
      clearTimeout(timer);
      resolveProcess({ code, signal, stdout, stderr, timedOut, outputLimited });
    });
  });
}

async function compileAndRun(source) {
  await access(compilerPath);
  const runDirectory = await mkdtemp(join(tmpdir(), 'joescript-'));
  const sourcePath = join(runDirectory, 'program.joe');
  try {
    await writeFile(sourcePath, source, 'utf8');
    const compilation = await runProcess(compilerPath, [sourcePath], runDirectory, 15000);
    if (compilation.timedOut) {
      return { ok: false, phase: 'compile', output: 'Compilation timed out after 15 seconds.' };
    }
    if (compilation.outputLimited) {
      return { ok: false, phase: 'compile', output: 'Compiler output exceeded the 1 MB limit.' };
    }
    if (compilation.code !== 0) {
      return {
        ok: false,
        phase: 'compile',
        output: (compilation.stderr || compilation.stdout || `Compiler exited with code ${compilation.code}.`).trim(),
      };
    }

    const executablePath = join(runDirectory, 'a.out');
    await access(executablePath);
    const execution = await runProcess(executablePath, [], runDirectory, 5000);
    if (execution.timedOut) {
      return { ok: false, phase: 'run', output: `${execution.stdout}${execution.stderr}\nProgram timed out after 5 seconds.`.trim() };
    }
    if (execution.outputLimited) {
      return { ok: false, phase: 'run', output: `${execution.stdout}\nProgram output exceeded the 1 MB limit.`.trim() };
    }
    return {
      ok: execution.code === 0,
      phase: 'run',
      exitCode: execution.code,
      output: `${execution.stdout}${execution.stderr}`.trimEnd(),
    };
  } finally {
    await rm(runDirectory, { recursive: true, force: true });
  }
}

const staticFiles = new Map([
  ['/', ['index.html', 'text/html; charset=utf-8']],
  ['/index.html', ['index.html', 'text/html; charset=utf-8']],
  ['/styles.css', ['styles.css', 'text/css; charset=utf-8']],
  ['/config.js', ['config.js', 'text/javascript; charset=utf-8']],
  ['/app.js', ['app.js', 'text/javascript; charset=utf-8']],
]);

const server = createServer(async (request, response) => {
  const pathname = new URL(request.url, 'http://localhost').pathname;
  if (request.method === 'GET' && pathname === '/health') {
    try {
      await access(compilerPath);
      sendJson(response, 200, { ok: true });
    } catch {
      sendJson(response, 503, { ok: false });
    }
    return;
  }

  if (pathname === '/api/run' && request.method === 'OPTIONS') {
    if (!allowedOrigin || request.headers.origin !== allowedOrigin) {
      response.writeHead(403);
      response.end();
      return;
    }
    response.writeHead(204, {
      'access-control-allow-origin': allowedOrigin,
      'access-control-allow-methods': 'POST, OPTIONS',
      'access-control-allow-headers': 'content-type',
      'access-control-max-age': '600',
      vary: 'Origin',
    });
    response.end();
    return;
  }

  if (request.method === 'POST' && pathname === '/api/run') {
    if (allowedOrigin && request.headers.origin && request.headers.origin !== allowedOrigin) {
      sendApiJson(request, response, 403, { ok: false, phase: 'compile', output: 'This origin is not allowed to use the compiler service.' });
      return;
    }
    try {
      const body = await readRequestBody(request);
      const { source } = JSON.parse(body);
      if (typeof source !== 'string' || source.trim().length === 0) {
        sendApiJson(request, response, 400, { ok: false, phase: 'compile', output: 'Add some JoeScript before running.' });
        return;
      }
      sendApiJson(request, response, 200, await compileAndRun(source));
    } catch (error) {
      const tooLarge = error.message.includes('64 KB');
      sendApiJson(request, response, tooLarge ? 413 : 500, {
        ok: false,
        phase: 'compile',
        output: tooLarge ? error.message : 'Could not compile this program. Check that the JoeScript compiler is built and executable.',
      });
    }
    return;
  }

  const asset = staticFiles.get(pathname);
  if (request.method !== 'GET' || !asset) {
    response.writeHead(404);
    response.end('Not found');
    return;
  }
  try {
    const content = await readFile(join(frontendDirectory, asset[0]));
    response.writeHead(200, {
      'content-type': asset[1],
      'x-content-type-options': 'nosniff',
      'content-security-policy': "default-src 'self'; style-src 'self' https://fonts.googleapis.com; font-src 'self' https://fonts.gstatic.com; script-src 'self'; connect-src 'self'; img-src 'self' data:",
    });
    response.end(content);
  } catch {
    response.writeHead(500);
    response.end('Could not load frontend files');
  }
});

server.listen(port, host, () => {
  process.stdout.write(`JoeScript Studio is ready at http://127.0.0.1:${port}\n`);
});

for (const signal of ['SIGINT', 'SIGTERM']) {
  process.on(signal, () => server.close(() => process.exit(0)));
}