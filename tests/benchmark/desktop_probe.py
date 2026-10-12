"""Scoped niri geometry/visual probe, without input injection or desktop edits.

Captures only the browser test window to /tmp; inspect/delete captures afterward.
This supplements but cannot replace user pointer/typing/scroll observations.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import time

from process_tree import browser_tree, processes, survivors

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--uri', required=True)
parser.add_argument('--label', required=True)
parser.add_argument('--resize', action='store_true')
args = parser.parse_args()
log = Path(f'/tmp/foob-{args.label}.log')
tracked = {}
record = {'result': 'fail', 'captures': []}


def action(*arguments):
    subprocess.run(['niri', 'msg', 'action', *arguments, '--id', str(window['id'])], check=True)


def settle():
    # Observe current identities while configure/frame work settles.
    deadline = time.monotonic() + 2
    while time.monotonic() < deadline:
        health()
        tracked.update(browser_tree(process.pid, processes()))
        time.sleep(0.05)


def capture(name):
    health()
    path = f'/tmp/foob-{args.label}-{name}.png'
    action('screenshot-window', '--path', path)
    record['captures'].append(path)
    record.setdefault('geometry', []).append(re.findall(r'viewport .*', log.read_text())[-1:])
    health()


def health():
    text = log.read_text()
    if process.poll() is not None or any(marker in text for marker in ['load-failed ', 'status=failed', 'web-process-terminated']):
        raise RuntimeError('browser/load/web-process failure')
    return text


with log.open('w') as stream:
    process = subprocess.Popen([str(Path('build/foob').resolve()), args.uri],
                               env={**os.environ, 'FOOB_DIAGNOSTICS': '1'},
                               stdout=stream, stderr=stream, start_new_session=True)
    try:
        deadline = time.monotonic() + 60
        while time.monotonic() < deadline:
            tracked.update(browser_tree(process.pid, processes()))
            if 'load-finished status=ok' in health():
                break
            time.sleep(0.1)
        else:
            raise RuntimeError('load timeout')
        windows = json.loads(subprocess.check_output(['niri', 'msg', '-j', 'windows'], text=True))
        matches = [item for item in windows if item.get('pid') == process.pid]
        if len(matches) != 1:
            raise RuntimeError('exactly one browser window required')
        window = matches[0]
        record['window_id'] = window['id']
        record['root_pid'] = process.pid
        settle()
        capture('initial')
        if args.resize:
            action('set-window-width', '700')
            settle()
            capture('narrow')
            action('fullscreen-window')
            settle()
            capture('fullscreen')
            action('fullscreen-window')
            settle()
            capture('restored')
        action('close-window')
        record['exit_code'] = process.wait(timeout=10)
        if any(marker in log.read_text() for marker in ['load-failed ', 'status=failed', 'web-process-terminated']):
            raise RuntimeError('late browser/load failure')
        record['result'] = 'pass' if record['exit_code'] == 0 else 'fail'
    except Exception as error:
        record['error'] = str(error)
    finally:
        tracked.update(browser_tree(process.pid, processes()))
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                record['result'] = 'fail'
                record['cleanup_error'] = 'termination timeout; killed disposable browser'
                process.kill()
                process.wait()
        deadline = time.monotonic() + 10
        while survivors(tracked) and time.monotonic() < deadline:
            time.sleep(0.1)
        record['survivors_after_10s'] = survivors(tracked)
        if record['survivors_after_10s']:
            record['result'] = 'fail'
        record['tracked_processes'] = tracked
Path(f'/tmp/foob-{args.label}.json').write_text(json.dumps(record, indent=2) + '\n')
print(json.dumps(record, indent=2))
raise SystemExit(0 if record['result'] == 'pass' else 1)
