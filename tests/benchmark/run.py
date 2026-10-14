"""Native Wayland PSS/lifecycle runner. Opens only the requested test window.

python3 tests/benchmark/run.py --binary build/foob --label blank --uri about:blank
Requires desktop access and readable /proc/*/smaps_rollup. Never disables sandbox.
"""
import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time

from process_tree import FAILURE_MARKERS, browser_tree, processes, provenance, sample, stop, survivors_after_wait

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', default='build/foob')
parser.add_argument('--label', required=True)
parser.add_argument('--uri', required=True)
parser.add_argument('--output', required=True)
parser.add_argument('--stop', choices=['sigterm', 'sigint', 'compositor'], default='sigterm')
parser.add_argument('--lifecycle-only', action='store_true')
parser.add_argument('--loads', type=int, default=1, help='2 for the fixture warm reload')
parser.add_argument('--fault-web-process', action='store_true', help='negative control: kill own test web process after load')
parser.add_argument('--memory-details', action='store_true', help='read own process smaps/DRM fdinfo after each PSS sample')
args = parser.parse_args()
binary = Path(args.binary).resolve()
record = {
    'date_utc': datetime.now(timezone.utc).isoformat(), 'workload': args.label,
    'condition': ('warm reload within a fresh ephemeral session' if args.loads > 1
                  else 'fresh process, ephemeral session') + '; OS caches uncontrolled',
    **provenance(binary),
    'protocol': '3 samples 1s apart, first >=10s after successful final load completion',
    'stop': args.stop, 'samples': [],
}
output = Path(args.output)
log = output.with_suffix('.log')
if output.exists() or log.exists():
    raise SystemExit(f'{output} or its log already exists; choose a new output path to preserve existing evidence')
output.parent.mkdir(parents=True, exist_ok=True)
tracked = {}
process = None
VIEWPORT = r'viewport width=(\d+) height=(\d+) scale=([0-9.]+)'
geometry_reference = None
geometry_guard_offset = 0


def check_health(expected_loads=None, allow_exit=False):
    text = log.read_text()
    if any(marker in text for marker in FAILURE_MARKERS):
        raise RuntimeError('navigation failure or abnormal web-process termination')
    if not allow_exit and process.poll() is not None:
        raise RuntimeError('browser exited before requested shutdown')
    if expected_loads is not None:
        if text.count('foob: load-started') != expected_loads or text.count('foob: load-finished status=ok') != expected_loads:
            raise RuntimeError('navigation changed after requested completion; idle interval invalid')
        if geometry_reference is not None:
            geometry = re.findall(VIEWPORT, text[geometry_guard_offset:])
            if any(item != geometry_reference for item in geometry):
                raise RuntimeError('viewport/scale changed after completion; fixed-geometry interval invalid')
    return text


try:
    with log.open('w') as stream:
        process = subprocess.Popen([str(binary), args.uri], env={**os.environ, 'FOOB_DIAGNOSTICS': '1'},
                                   stdout=stream, stderr=stream, start_new_session=True)
        record['root_pid'] = process.pid
        deadline = time.monotonic() + 90
        while time.monotonic() < deadline:
            tracked.update(browser_tree(process.pid, processes()))
            text = check_health()
            finished = re.findall(r'load-finished status=ok elapsed_ms=([0-9.]+)', text)
            if len(finished) >= args.loads:
                geometry_guard_offset = text.rfind('foob: load-finished status=ok')
                geometry = re.findall(VIEWPORT, text[geometry_guard_offset:])
                if not geometry:
                    time.sleep(0.05)
                    continue
                geometry_reference = geometry[0]
                check_health(args.loads)
                completion = time.monotonic()
                record['navigation_elapsed_ms'] = [float(item) for item in finished]
                break
            time.sleep(0.05)
        else:
            raise RuntimeError('navigation completion timeout')
        if args.fault_web_process:
            children = browser_tree(process.pid, processes())
            targets = [pid for pid, item in children.items() if item['name'] == 'WPEWebProcess']
            if len(targets) != 1:
                raise RuntimeError('negative control cannot identify own web process')
            os.kill(targets[0], signal.SIGKILL)
            record['fault_injected_pid'] = targets[0]
        if not args.lifecycle_only:
            for index in range(3):
                target = completion + 10 + index
                while time.monotonic() < target:
                    check_health(args.loads)
                    tracked.update(browser_tree(process.pid, processes()))
                    time.sleep(max(0, min(0.1, target - time.monotonic())))
                check_health(args.loads)
                item = sample(process.pid)
                if args.memory_details:
                    from memory_details import details
                    item['memory_details'] = [details(row['pid']) for row in item['processes']]
                check_health(args.loads)
                item['seconds_after_observed_completion'] = time.monotonic() - completion
                tracked.update({row['pid']: row for row in item['processes']})
                geometry = re.findall(VIEWPORT, log.read_text())
                if not geometry:
                    raise RuntimeError('missing viewport diagnostic')
                item['viewport'] = {'width': int(geometry[-1][0]), 'height': int(geometry[-1][1]),
                                    'scale': float(geometry[-1][2])}
                record['samples'].append(item)
                print(args.label, index + 1, item['viewport'], round(item['total_pss_kib']/1024, 3), 'MiB', flush=True)
            if any(item['viewport'] != record['samples'][0]['viewport'] for item in record['samples']):
                raise RuntimeError('viewport/scale changed during measurement')
        # Metadata only: inspect rendering device, loaded driver, confinement.
        record['runtime_evidence'] = []
        for pid, identity in browser_tree(process.pid, processes()).items():
            evidence = {'pid': pid, 'name': identity['name']}
            try:
                evidence['dri_fds'] = sorted({os.readlink(fd) for fd in Path(f'/proc/{pid}/fd').iterdir()
                                              if os.readlink(fd).startswith('/dev/dri/')})
                maps = Path(f'/proc/{pid}/maps').read_text()
                evidence['graphics_libraries'] = sorted({line.split()[-1] for line in maps.splitlines()
                                                         if any(x in line for x in ['libEGL', 'libGL', 'radeonsi', 'libvulkan', 'libgbm'])})
                status = Path(f'/proc/{pid}/status').read_text()
                evidence['confinement'] = [line for line in status.splitlines()
                                           if line.startswith(('Seccomp:', 'NoNewPrivs:', 'NSpid:'))]
                evidence['namespaces'] = {kind: os.readlink(f'/proc/{pid}/ns/{kind}')
                                          for kind in ['mnt', 'pid', 'user', 'net']}
            except OSError as error:
                evidence['inspection_error'] = type(error).__name__
            record['runtime_evidence'].append(evidence)
        check_health(args.loads)
        if args.stop == 'compositor':
            windows = json.loads(subprocess.check_output(['niri', 'msg', '-j', 'windows'], text=True))
            matches = [window for window in windows if window.get('pid') == process.pid]
            if len(matches) != 1:
                raise RuntimeError('cannot identify exactly one test window by PID')
            record['window_id'] = matches[0]['id']
            subprocess.run(['niri', 'msg', 'action', 'close-window', '--id', str(matches[0]['id'])], check=True)
        else:
            process.send_signal(signal.SIGTERM if args.stop == 'sigterm' else signal.SIGINT)
        record['exit_code'] = process.wait(timeout=10)
        if record['exit_code'] != 0:
            raise RuntimeError('nonzero browser exit')
        check_health(args.loads, allow_exit=True)
        record['result'] = 'pass'
except Exception as error:
    record['result'] = 'fail'
    record['error'] = str(error)
finally:
    if process is not None:
        tracked.update(browser_tree(process.pid, processes()))
        stop(process)
        record['survivors_after_10s'] = survivors_after_wait(tracked)
        if record['survivors_after_10s']:
            record['result'] = 'fail'
        record['tracked_processes'] = tracked
    output.write_text(json.dumps(record, indent=2) + '\n')
print(args.label, record['result'], flush=True)
raise SystemExit(0 if record['result'] == 'pass' else 1)
