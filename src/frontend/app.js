const starterProgram = `joePrint("Hello from JoeScript!");

joeInt answer = 40 + 2;
joePrint(answer);

joeInt count = 1;
joeWhile (count <= 3) {
    joePrint(count);
    ++count;
}`;

const editor = document.querySelector('#source-editor');
const lineNumbers = document.querySelector('#line-numbers');
const runButton = document.querySelector('#run-button');
const runLabel = document.querySelector('#run-label');
const outputState = document.querySelector('#output-state');
const terminal = document.querySelector('#terminal');
const runSummary = document.querySelector('#run-summary');
const cursorPosition = document.querySelector('#cursor-position');
const unsavedIndicator = document.querySelector('#unsaved-indicator');
let runCount = 0;
const apiBaseUrl = (window.JOESCRIPT_API_URL || '').replace(/\/$/, '');

function updateEditorDetails() {
  const lineCount = editor.value.split('\n').length;
  lineNumbers.textContent = Array.from({ length: lineCount }, (_, index) => index + 1).join('\n');
  const beforeCursor = editor.value.slice(0, editor.selectionStart).split('\n');
  cursorPosition.textContent = `Ln ${beforeCursor.length}, Col ${beforeCursor.at(-1).length + 1}`;
  unsavedIndicator.hidden = editor.value === starterProgram;
}

function setStatus(text, kind = '') {
  outputState.textContent = text;
  outputState.dataset.kind = kind;
}

function showOutput({ ok, phase, output, exitCode }) {
  const prompt = document.createElement('div');
  prompt.className = 'terminal-prompt';
  prompt.innerHTML = '<span>joe@studio <b>~</b> %</span> <span>./main.joe</span>';
  const result = document.createElement('pre');
  result.className = `terminal-output${ok ? '' : ' terminal-error'}`;
  result.textContent = output || (ok ? '(Program finished with no output)' : 'The program stopped without producing output.');
  const meta = document.createElement('div');
  meta.className = 'terminal-meta';
  meta.textContent = phase === 'compile'
    ? 'Compilation failed'
    : ok ? `Finished successfully · exit code ${exitCode}` : `Program exited with code ${exitCode ?? 'unknown'}`;
  terminal.replaceChildren(prompt, result, meta);
  terminal.scrollTop = terminal.scrollHeight;
}

async function runProgram() {
  if (runButton.disabled) return;
  runButton.disabled = true;
  runLabel.textContent = 'Compiling…';
  setStatus('WORKING', 'running');
  terminal.replaceChildren();
  const progress = document.createElement('div');
  progress.className = 'terminal-prompt';
  progress.textContent = 'Compiling JoeScript, then launching your program…';
  terminal.append(progress);
  const startedAt = performance.now();
  try {
    const response = await fetch(`${apiBaseUrl}/api/run`, {
      method: 'POST',
      headers: { 'content-type': 'application/json' },
      body: JSON.stringify({ source: editor.value }),
    });
    const result = await response.json();
    runCount += 1;
    showOutput(result);
    setStatus(result.ok ? 'SUCCESS' : result.phase === 'compile' ? 'COMPILE ERROR' : 'RUNTIME ERROR', result.ok ? 'success' : 'error');
    const duration = ((performance.now() - startedAt) / 1000).toFixed(2);
    runSummary.textContent = `Run ${runCount} · ${duration}s`;
  } catch {
    showOutput({ ok: false, phase: 'compile', output: 'Could not reach the local compiler service. Restart the frontend and try again.' });
    setStatus('OFFLINE', 'error');
    runSummary.textContent = 'Compiler service unavailable';
  } finally {
    runButton.disabled = false;
    runLabel.textContent = 'Run program';
  }
}

editor.value = starterProgram;
updateEditorDetails();
editor.addEventListener('input', updateEditorDetails);
editor.addEventListener('click', updateEditorDetails);
editor.addEventListener('keyup', updateEditorDetails);
editor.addEventListener('scroll', () => { lineNumbers.scrollTop = editor.scrollTop; });
editor.addEventListener('keydown', (event) => {
  if (event.key === 'Tab') {
    event.preventDefault();
    const start = editor.selectionStart;
    const end = editor.selectionEnd;
    editor.setRangeText('    ', start, end, 'end');
    updateEditorDetails();
  }
  if ((event.metaKey || event.ctrlKey) && event.key === 'Enter') {
    event.preventDefault();
    runProgram();
  }
});
runButton.addEventListener('click', runProgram);
document.querySelector('#reset-button').addEventListener('click', () => {
  editor.value = starterProgram;
  updateEditorDetails();
  editor.focus();
});
document.querySelector('#clear-button').addEventListener('click', () => {
  terminal.replaceChildren();
  const hint = document.createElement('div');
  hint.className = 'terminal-prompt';
  hint.textContent = 'Output cleared. Run your program to see fresh results.';
  terminal.append(hint);
  setStatus('IDLE');
  runSummary.textContent = 'Output cleared';
});