"""Monitor an interactive M1 test window until the user closes it.

Only application diagnostics/process identities are recorded; no input contents.
Interaction acceptance is recorded separately from lifecycle results.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

from process_tree import browser_tree, processes, survivors

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--uri', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
output = Path(args.output)
output.parent.mkdir(parents=True, exist_ok=True)
if output.exists() or output.with_suffix('.log').exists():
    raise SystemExit('Choose a new output path to preserve existing evidence')
binary = Path('build/foob').resolve()
record = {
    'date_utc': datetime.now(timezone.utc).isoformat(),
    'revision': subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip(),
    'source_sha256': hashlib.sha256(Path('src/main.c').read_bytes()).hexdigest(),
    'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
    'mode': 'user interaction; ephemeral session; acceptance recorded separately',
}
tracked = {}
with output.with_suffix('.log').open('w') as stream:
    process = subprocess.Popen([str(binary), args.uri],
                               env={**os.environ, 'FOOB_DIAGNOSTICS': '1'},
                               stdout=stream, stderr=stream, start_new_session=True)
    record['root_pid'] = process.pid
    output.write_text(json.dumps({**record, 'lifecycle_result': 'running'}, indent=2) + '\n')
    print(f'Interactive foob window launched; PID {process.pid}. Close it after the guided checks.', flush=True)
    try:
        while process.poll() is None:
            tracked.update(browser_tree(process.pid, processes()))
            time.sleep(0.1)
        record['exit_code'] = process.wait()
    finally:
        tracked.update(browser_tree(process.pid, processes()))
        if process.poll() is None:
            record['interrupted'] = True
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                record['cleanup_kill_required'] = True
                process.kill()
                process.wait()
        deadline = time.monotonic() + 10
        while survivors(tracked) and time.monotonic() < deadline:
            time.sleep(0.1)
        record['tracked_processes'] = tracked
        record['survivors_after_10s'] = survivors(tracked)
        record['lifecycle_result'] = ('pass' if record.get('exit_code') == 0
                                      and not record['survivors_after_10s'] else 'fail')
        output.write_text(json.dumps(record, indent=2) + '\n')
print(f"Interactive lifecycle: {record['lifecycle_result']}", flush=True)
