"""Scoped niri geometry/visual probe, without input injection or desktop edits.

Captures only the browser test window. The log, record and screenshots go into a new
directory readable only by you (--output-dir, which must not exist yet, or a fresh
$TMPDIR/foob-probe-<label>-* by default), so a rerun never replaces an earlier probe and
other local users cannot read the captures; inspect/delete it afterward.
This supplements but cannot replace user pointer/typing/scroll observations.
Probe a fixture, not about:blank: that load ends before niri lists the window.
"""
import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time

from process_tree import FAILURE_MARKERS, browser_tree, processes, provenance, stop, survivors_after_wait

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--uri', required=True)
parser.add_argument('--label', required=True)
parser.add_argument('--binary', default='build/foob')
parser.add_argument('--resize', action='store_true')
parser.add_argument('--output-dir', type=Path, help='new directory for the outputs (must not exist)')
args = parser.parse_args()
if args.output_dir:
    args.output_dir.mkdir(mode=0o700)  # refuses an existing directory
    output = args.output_dir.resolve()  # niri writes the screenshots, from its own directory
else:
    output = Path(tempfile.mkdtemp(prefix=f'foob-probe-{args.label}-'))  # mode 0700
print(f'writing to {output}', flush=True)
log = output / f'foob-{args.label}.log'
tracked = {}
record = {'date_utc': datetime.now(timezone.utc).isoformat(), **provenance(Path(args.binary).resolve()),
          'result': 'fail', 'captures': []}  # provenance is taken before the browser starts


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
    path = str(output / f'foob-{args.label}-{name}.png')
    action('screenshot-window', '--path', path)
    record['captures'].append(path)
    record.setdefault('geometry', []).append(re.findall(r'viewport .*', log.read_text())[-1:])
    health()


def health():
    text = log.read_text()
    if process.poll() is not None or any(marker in text for marker in FAILURE_MARKERS):
        raise RuntimeError('browser/load/web-process failure')
    return text


with log.open('w') as stream:
    process = subprocess.Popen([str(Path(args.binary).resolve()), args.uri],
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
        if any(marker in log.read_text() for marker in FAILURE_MARKERS):
            raise RuntimeError('late browser/load failure')
        record['result'] = 'pass' if record['exit_code'] == 0 else 'fail'
    except Exception as error:
        record['error'] = str(error)
    finally:
        tracked.update(browser_tree(process.pid, processes()))
        if stop(process):
            record['result'] = 'fail'
            record['cleanup_error'] = 'termination timeout; killed disposable browser'
        record['survivors_after_10s'] = survivors_after_wait(tracked)
        if record['survivors_after_10s']:
            record['result'] = 'fail'
        record['tracked_processes'] = tracked
(output / f'foob-{args.label}.json').write_text(json.dumps(record, indent=2) + '\n')
print(json.dumps(record, indent=2))
raise SystemExit(0 if record['result'] == 'pass' else 1)
