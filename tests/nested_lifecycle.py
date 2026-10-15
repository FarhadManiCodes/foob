"""Lifecycle checks against a disposable nested niri; the real compositor is never touched.

python3 tests/nested_lifecycle.py build/foob [--keep] [scenario-name ...]

Needs a desktop session with niri installed (the nested compositor opens as a
window) and no extra packages. Each scenario starts a fresh nested niri with an
empty config, so no user autostart program runs. Checked: exit code, and that
no browser helper process survives the exit. The scratch directory under
$TMPDIR is removed after a fully passing run; after a failure, or with --keep,
its path is printed so the logs can be read.
"""
import atexit
import functools
import http.server
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import threading
import time

sys.path.insert(0, str(Path(__file__).resolve().parent / 'benchmark'))
from process_tree import browser_tree, processes, survivors  # noqa: E402

if len(sys.argv) < 2 or sys.argv[1] in ('-h', '--help') or not os.access(sys.argv[1], os.X_OK):
    sys.exit(__doc__)  # usage, before any scratch directory or server exists
binary = str(Path(sys.argv[1]).resolve())
keep = '--keep' in sys.argv[2:]
selected = set(sys.argv[2:]) - {'--keep'}  # empty means every scenario
known = []  # every scenario name seen, to reject a misspelled or obsolete request
scratch = Path(tempfile.mkdtemp(prefix='foob-nested-'))
all_passed = False


def remove_scratch():
    if all_passed and not keep:
        shutil.rmtree(scratch, ignore_errors=True)
    else:
        print(f'scratch directory kept: {scratch}', file=sys.stderr)


atexit.register(remove_scratch)
(scratch / 'min.kdl').write_text('// empty nested test config\ninput { }\n')


def wait_for(predicate, seconds, message):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        value = predicate()
        if value:
            return value
        time.sleep(0.1)
    raise AssertionError(message)


def stop_compositor(compositor):
    compositor.terminate()
    try:
        compositor.wait(timeout=10)
    except subprocess.TimeoutExpired:
        compositor.kill()
        compositor.wait()


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
    try:
        display, socket = wait_for(endpoints, 15, 'nested niri did not start')
    except BaseException:
        # scenario() has no cleanup yet at this point; never leave the compositor behind.
        stop_compositor(compositor)
        raise
    return compositor, display, socket


def windows(socket):
    result = subprocess.run(['niri', 'msg', '-j', 'windows'], capture_output=True, text=True,
                            env={**os.environ, 'NIRI_SOCKET': socket})
    return json.loads(result.stdout) if result.returncode == 0 else []


def scenario(name, expected_exit, stop, uri='about:blank', expected_title='foob',
             load_status='ok', extra_env=None, after_load=None):
    known.append(name)
    if selected and name not in selected:
        return
    compositor, display, socket = start_compositor()
    errors = scratch / f'{name}.err'
    try:
        browser = subprocess.Popen([binary, uri], stderr=errors.open('w'),
                                   env={**os.environ, 'WAYLAND_DISPLAY': display,
                                        'FOOB_DIAGNOSTICS': '1', **(extra_env or {})},
                                   start_new_session=True)
        wait_for(lambda: f'load-finished status={load_status}' in errors.read_text(), 15,
                 f'{name}: expected load status {load_status!r}')
        window = wait_for(lambda: [w for w in windows(socket) if w['pid'] == browser.pid],
                          10, 'no compositor window for the browser')[0]
        wait_for(lambda: any(w['pid'] == browser.pid and w['title'] == expected_title
                             for w in windows(socket)),
                 10, f'{name}: window title never became {expected_title!r}')
        assert window['app_id'] == 'dev.foob.Browser', f"{name}: app-id {window['app_id']!r}"
        if after_load:
            after_load(browser, socket, errors)
        time.sleep(1)
        tracked = browser_tree(browser.pid, processes())
        stop(compositor, socket, window, browser)
        code = browser.wait(timeout=10)
        assert code == expected_exit, f'{name}: exit {code}, expected {expected_exit}'
        wait_for(lambda: not survivors(tracked), 10, f'{name}: helper processes survived')
        print(f'PASS {name}: exit {code}, no survivors')
    finally:
        try:
            os.killpg(browser.pid, signal.SIGKILL)  # first: a stuck compositor must not skip it
        except (ProcessLookupError, NameError):
            pass
        if compositor.poll() is None:
            stop_compositor(compositor)


def terminate(compositor, socket, window, browser):
    browser.send_signal(signal.SIGTERM)


def close_window(compositor, socket, window, browser):
    subprocess.run(['niri', 'msg', 'action', 'close-window', '--id', str(window['id'])],
                   check=True, env={**os.environ, 'NIRI_SOCKET': socket})


# A loopback HTTP origin is remote (untrusted) for foob; only file: and about:blank are local.
(scratch / 'hostile.html').write_text(
    '<!doctype html><meta charset=utf-8><title>  Pay\u202eevil   \u2066 page\x07  </title>x',
    encoding='utf-8')

# /stream is a 300 MiB attachment sent slowly; `stream` records how far the server got.
stream_size = 300 * 1024 * 1024
stream = {'sent': 0, 'outcome': None}
probes = []  # query strings the page reports through /probe
echoes = []  # each POST to /echo; the blank-form scenario expects none


class Handler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        path, _, query = self.path.partition('?')
        body = self.rfile.read(int(self.headers.get('Content-Length', '0')))
        if path != '/echo':
            return self.send_error(404)
        echoes.append({'from': query, 'bytes': len(body), 'site': self.headers.get('Sec-Fetch-Site')})
        page = b'<!doctype html><title>echo</title>echo'
        self.send_response(200)
        self.send_header('Content-Type', 'text/html')
        self.send_header('Content-Length', str(len(page)))
        super().end_headers()
        self.wfile.write(page)

    def do_GET(self):
        if self.path.startswith('/probe?'):
            probes.append(self.path.split('?', 1)[1])
            self.send_response(204)
            return super().end_headers()
        if self.path != '/stream':
            return super().do_GET()
        self.send_response(200)
        self.send_header('Content-Type', 'application/octet-stream')
        self.send_header('Content-Disposition', 'attachment; filename="stream.bin"')
        self.send_header('Content-Length', str(stream_size))
        super().end_headers()
        chunk = b'x' * 65536
        try:
            while stream['sent'] < stream_size:
                self.wfile.write(chunk)
                self.wfile.flush()
                stream['sent'] += len(chunk)
                time.sleep(0.01)
            stream['outcome'] = 'completed'
        except OSError:
            stream['outcome'] = 'aborted'

    def log_message(self, *arguments):
        pass


server = http.server.ThreadingHTTPServer(
    ('127.0.0.1', 0), functools.partial(Handler, directory=str(scratch)))
threading.Thread(target=server.serve_forever, daemon=True).start()

(scratch / 'local.html').write_text('<!doctype html><meta charset=utf-8><title>a\u200bb</title>x', encoding='utf-8')
scenario('title-local-unsanitized', 0, terminate,  # the remote scenario below strips the ZWSP
         uri=(scratch / 'local.html').as_uri(), expected_title='a\u200bb \u2014 foob')
scenario('title-remote-sanitized', 0, terminate,
         uri=f'http://127.0.0.1:{server.server_port}/hostile.html',
         expected_title='Payevil page \u2014 foob')
# Downloads are cancelled until M3 designs a policy. GLib reads the download
# directory from $XDG_CONFIG_HOME/user-dirs.dirs; the cache, data and temporary
# directories are redirected too, so a failure writes only below `scratch` and
# never to the real home, and the checks below can search every place a download
# or its partial file could land.
downloads = scratch / 'downloads'
sandbox = {name: scratch / name for name in ('xdg', 'cache', 'data', 'tmp')}
downloads.mkdir()
for directory in sandbox.values():
    directory.mkdir()
(sandbox['xdg'] / 'user-dirs.dirs').write_text(f'XDG_DOWNLOAD_DIR="{downloads}"\n')
download_env = {'XDG_CONFIG_HOME': str(sandbox['xdg']), 'XDG_CACHE_HOME': str(sandbox['cache']),
                'XDG_DATA_HOME': str(sandbox['data']), 'TMPDIR': str(sandbox['tmp'])}
# Socket buffers and cancel latency let a few MiB through under load (1.2 MiB seen once);
# an uncancelled download streams the full 300 MiB, so 16 MiB still separates the two.
stream_limit = 16 * 1024 * 1024


def transfer_stopped_without_files(browser, socket, errors):
    wait_for(lambda: stream['outcome'], 10, 'server never saw the transfer end')
    assert stream['outcome'] == 'aborted', 'the whole attachment was transferred'
    assert stream['sent'] < stream_limit, f"server sent {stream['sent']} bytes before the abort"
    assert browser.poll() is None, 'browser exited during download'
    assert not list(downloads.iterdir()), f'download was saved: {list(downloads.iterdir())}'
    # Engine caches (Mesa, GStreamer) legitimately live below `sandbox`; look for the payload.
    def starts_with_payload(path):
        size = path.stat().st_size
        with path.open('rb') as handle:  # engine caches can be large; read only the start
            return size > 0 and handle.read(4096) == b'x' * min(size, 4096)
    saved = [path for directory in sandbox.values() for path in directory.rglob('*')
             if path.is_file() and starts_with_payload(path)]
    assert not saved, f'attachment data was written to disk: {saved}'


scenario('download-cancelled', 0, terminate,
         uri=f'http://127.0.0.1:{server.server_port}/stream', load_status='failed',
         extra_env=download_env, after_load=transfer_stopped_without_files)

# A trusted submit of a target=_blank form asks for a new window, which foob ignores: nothing
# is loaded on the page's behalf (D-04). requestSubmit() fires a trusted submit event without a
# click. A sandboxed frame with allow-popups but without allow-top-navigation submits first and
# must not reach the top. The control is the build that retargeted _blank forms to _top (script
# in browser.c, 93bdf41 to f40e4b9): both forms reached /echo and the window showed "echo".
(scratch / 'sandboxed-form.html').write_text(
    '<!doctype html><form id=f method=post action="/echo?sandbox" target=_blank>'
    '<input name=a value=1></form><script>document.getElementById("f").requestSubmit()</script>')
(scratch / 'blank-form.html').write_text(
    '<!doctype html><title>forms</title><body>'
    '<iframe sandbox="allow-scripts allow-forms allow-popups" src="sandboxed-form.html"></iframe>'
    f'<form id=f method=post action="http://localhost:{server.server_port}/echo?top" target=_blank>'
    '<input name=a value=0123456789></form><script>addEventListener("load", () =>'
    ' setTimeout(() => {document.getElementById("f").requestSubmit();'
    ' new Image().src = "/probe?form-submitted";}, 1000));</script></body>')


def blank_form_does_not_navigate(browser, socket, errors):
    wait_for(lambda: 'form-submitted' in probes, 10, 'the page never submitted its form')
    time.sleep(3)  # long enough for a wrongly started navigation to show
    assert browser.poll() is None, 'browser exited'
    assert any(w['pid'] == browser.pid and w['title'] == 'forms \u2014 foob' for w in windows(socket)), \
        'window title changed: a blocked new-window request navigated'
    loads = errors.read_text().count('foob: load-started')
    assert loads == 1, f'{loads} loads started; a blocked request navigated'
    assert not echoes, f'a _blank form reached /echo: {echoes}'


scenario('blank-form-does-not-navigate', 0, terminate,
         uri=f'http://127.0.0.1:{server.server_port}/blank-form.html',
         expected_title='forms \u2014 foob', after_load=blank_form_does_not_navigate)
scenario('compositor-close', 0, close_window)
scenario('sigint', 0, lambda c, s, w, b: b.send_signal(signal.SIGINT))
scenario('compositor-lost', 3, lambda c, s, w, b: c.kill())


def kill_web_process(compositor, socket, window, browser):
    # Exact PIDs from the browser's own process tree; never a name pattern.
    web = [pid for pid, item in browser_tree(browser.pid, processes()).items()
           if item['name'] == 'WPEWebProcess']
    assert len(web) == 1, f'expected one web process, found {web}'
    os.kill(web[0], signal.SIGKILL)


# A dead web process leaves nothing to show: the browser exits with status 4.
scenario('web-process-crash', 4, kill_web_process)
if selected - set(known):
    # A name that matched nothing ran nothing; never report that as a pass.
    sys.exit(f"unknown scenario(s): {', '.join(sorted(selected - set(known)))}; known: {', '.join(known)}")
all_passed = True
