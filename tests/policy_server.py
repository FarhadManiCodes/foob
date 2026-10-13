"""Loopback server for the policy fixture, to be opened in foob as a remote page.

python3 tests/policy_server.py [port]   # prints the URL; Ctrl-C stops it

Serves tests/fixtures/policy-remote.html and its targets. Loopback counts as remote
for foob, so this exercises the untrusted-origin rules. Request paths are printed
so a blocked download can be seen to reach the server. The local-file links point
at tests/fixtures/linked.html.
"""
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
        self.rfile.read(length)
        if self.path.split('?')[0] == '/echo':
            self.reply_echo()
        else:
            self.reply(404, 'text/plain', b'not found')

    def reply_echo(self):
        # Item 9's target=_blank form: foob ignores new-window requests, so no request
        # should arrive. If one does, the window shows this red page.
        page = (b'<!doctype html><meta charset="utf-8"><title>Echo FAIL</title>'
                b'<body style="font:24px/1.5 sans-serif;margin:24px">'
                b'<h1 style="color:#b00020">FAIL: the target=_blank form was submitted</h1>'
                b'<p><a href="/">Back to the policy fixture</a></p>')
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
            print(self.command, self.path.split('?')[0], status_of(arguments), flush=True)


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
