"""Loopback server for the policy fixture, to be opened in foob as a remote page.

python3 tests/policy_server.py [port]   # prints the URL; Ctrl-C stops it

Serves tests/fixtures/policy-remote.html and its targets. Loopback counts as remote
for foob, so this exercises the untrusted-origin rules. Request paths are printed
so a blocked download can be seen to reach the server. The local-file links point
at tests/fixtures/linked.html.
"""
import html
import http.server
from pathlib import Path
import sys
import threading

fixtures = Path(__file__).resolve().parent / 'fixtures'
local_file = (fixtures / 'linked.html').as_uri()


# Smallest valid one-page PDF (blank), served inline for the "PDF URL" record-only check.
PDF = (b'%PDF-1.1\n1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj\n'
       b'2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj\n'
       b'3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 200 100]>>endobj\n'
       b'trailer<</Root 1 0 R/Size 4>>\n%%EOF\n')


class Handler(http.server.BaseHTTPRequestHandler):
    quiet = False

    def do_GET(self):
        path = self.path.split('?')[0]
        pages = {'/': 'policy-remote.html', '/second.html': 'policy-second.html',
                 '/sandboxed.html': 'policy-sandboxed.html'}
        if path in pages:
            body = (fixtures / pages[path]).read_text(encoding='utf-8')
            body = body.replace('@LOCAL_FILE@', local_file).encode('utf-8')
            self.reply(200, 'text/html; charset=utf-8', body)
        elif path == '/attachment.bin':
            self.reply(200, 'application/octet-stream', b'x' * 1_000_000,
                       'attachment; filename="attachment.bin"')
        elif path == '/echo':
            self.reply_echo('GET', b'')
        elif path == '/sample.pdf':
            self.reply(200, 'application/pdf', PDF)
        elif path == '/plain.bin':
            self.reply(200, 'application/octet-stream', b'x' * 1_000_000)
        else:
            self.reply(404, 'text/plain', b'not found')

    def do_POST(self):
        try:
            length = int(self.headers.get('Content-Length', '0'))
        except ValueError:
            length = -1
        if not 0 <= length <= 65536:  # missing is 0; refuse malformed, negative or oversized bodies
            self.reply(400, 'text/plain', b'bad content length')
            return
        body = self.rfile.read(length)
        if self.path.split('?')[0] == '/echo':
            self.reply_echo('POST', body)
        else:
            self.reply(404, 'text/plain', b'not found')

    def reply_echo(self, method, body):
        # The title carries the result so it is readable in the window title. The form is on
        # this origin, so Sec-Fetch-Site must be same-origin; "none" means foob loaded the
        # request itself instead of letting the page navigate (F-52).
        site = self.headers.get('Sec-Fetch-Site', 'missing')
        passed = method == 'POST' and len(body) == 15 and site == 'same-origin'
        verdict = ('<h1 style="color:#0a7d2c">PASS: the form body arrived (POST, 15 bytes, same-origin)</h1>' if passed else
                   '<h1 style="color:#b00020">FAIL: expected POST with 15 bytes from the same origin</h1>')
        page = (f'<!doctype html><meta charset="utf-8"><title>Echo {method} {len(body)} bytes {html.escape(site)}</title>'
                f'<body style="font:24px/1.5 sans-serif;margin:24px">{verdict}'
                f'<p>Request method: <b>{method}</b>, body: <b>{len(body)} bytes</b>, '
                f'Sec-Fetch-Site: <b>{html.escape(site)}</b>. '
                f'A GET with 0 bytes means the new-window code dropped the form body; '
                f'"none" means foob sent the request in place of the page.</p>'
                f'<p><a href="/">Back to the policy fixture</a></p>').encode('utf-8')
        self.reply(200, 'text/html; charset=utf-8', page)

    def reply(self, status, content_type, body, disposition=None):
        self.send_response(status)
        self.send_header('Content-Type', content_type)
        self.send_header('Content-Length', str(len(body)))
        if disposition:
            self.send_header('Content-Disposition', disposition)
        self.end_headers()
        try:
            self.wfile.write(body)
        except OSError:
            pass  # the client cancelled the transfer

    def log_message(self, fmt, *arguments):
        if not self.quiet:
            print('GET', self.path.split('?')[0], status_of(arguments), flush=True)


def status_of(arguments):
    return arguments[1] if len(arguments) > 1 else ''


def serve(port=0, quiet=True):
    """Starts the server in a thread; returns (server, URL of the fixture page)."""
    Handler.quiet = quiet
    server = http.server.ThreadingHTTPServer(('127.0.0.1', port), Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    return server, f'http://127.0.0.1:{server.server_port}/'


if __name__ == '__main__':
    server, url = serve(int(sys.argv[1]) if len(sys.argv) > 1 else 8765, quiet=False)
    print(f'Open with: ./build/foob {url}', flush=True)
    try:
        threading.Event().wait()
    except KeyboardInterrupt:
        pass
