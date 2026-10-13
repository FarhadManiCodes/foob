"""Lifecycle checks against a disposable nested niri; the real compositor is never touched.

python3 tests/nested_lifecycle.py build/foob

Needs a desktop session with niri installed (the nested compositor opens as a
window) and no extra packages. Each scenario starts a fresh nested niri with an
empty config, so no user autostart program runs. Checked: exit code, and that
no browser helper process survives the exit.
"""
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, str(Path(__file__).resolve().parent / 'benchmark'))
from process_tree import browser_tree, processes, survivors  # noqa: E402

binary = str(Path(sys.argv[1]).resolve())
scratch = Path(tempfile.mkdtemp(prefix='foob-nested-'))
(scratch / 'min.kdl').write_text('// empty nested test config\ninput { }\n')


def wait_for(predicate, seconds, message):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        value = predicate()
        if value:
            return value
        time.sleep(0.1)
    raise AssertionError(message)


def start_compositor():
    log = scratch / 'niri.log'
    env = {key: value for key, value in os.environ.items() if key != 'NIRI_SOCKET'}
    compositor = subprocess.Popen(['niri', '-c', str(scratch / 'min.kdl')], env=env,
                                  stdout=log.open('w'), stderr=subprocess.STDOUT,
                                  start_new_session=True)

    def endpoints():
        text = log.read_text()
        display = re.search(r'listening on Wayland socket: (\S+)', text)
        socket = re.search(r'IPC listening on: (\S+)', text)
        return (display[1], socket[1]) if display and socket else None
    display, socket = wait_for(endpoints, 15, 'nested niri did not start')
    return compositor, display, socket


def windows(socket):
    result = subprocess.run(['niri', 'msg', '-j', 'windows'], capture_output=True, text=True,
                            env={**os.environ, 'NIRI_SOCKET': socket})
    return json.loads(result.stdout) if result.returncode == 0 else []


def scenario(name, expected_exit, stop):
    compositor, display, socket = start_compositor()
    errors = scratch / f'{name}.err'
    try:
        browser = subprocess.Popen([binary, 'about:blank'], stderr=errors.open('w'),
                                   env={**os.environ, 'WAYLAND_DISPLAY': display,
                                        'FOOB_DIAGNOSTICS': '1'},
                                   start_new_session=True)
        wait_for(lambda: 'load-finished status=ok' in errors.read_text(), 15, 'page did not load')
        window = wait_for(lambda: [w for w in windows(socket) if w['pid'] == browser.pid],
                          10, 'no compositor window for the browser')[0]
        time.sleep(1)
        tracked = browser_tree(browser.pid, processes())
        stop(compositor, socket, window, browser)
        code = browser.wait(timeout=10)
        assert code == expected_exit, f'{name}: exit {code}, expected {expected_exit}'
        wait_for(lambda: not survivors(tracked), 10, f'{name}: helper processes survived')
        print(f'PASS {name}: exit {code}, no survivors')
    finally:
        for process in (compositor,):
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=10)
        try:
            os.killpg(browser.pid, signal.SIGKILL)
        except (ProcessLookupError, NameError):
            pass


def close_window(compositor, socket, window, browser):
    subprocess.run(['niri', 'msg', 'action', 'close-window', '--id', str(window['id'])],
                   check=True, env={**os.environ, 'NIRI_SOCKET': socket})


scenario('compositor-close', 0, close_window)
scenario('sigterm', 0, lambda c, s, w, b: b.send_signal(signal.SIGTERM))
scenario('sigint', 0, lambda c, s, w, b: b.send_signal(signal.SIGINT))
scenario('compositor-lost', 3, lambda c, s, w, b: c.kill())
