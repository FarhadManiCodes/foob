"""Startup contract checks; standard library only, no display needed."""
import os
from pathlib import Path
import subprocess
import sys

binary = str(Path(sys.argv[1]).resolve())
environment = {**os.environ, "WAYLAND_DISPLAY": "foob-no-such-display"}
rejected = [
    [""], ["example.org"], ["/tmp/page.html"], ["about:config"],
    ["javascript:alert(1)"], ["data:text/html,test"], ["ftp://example.org"],
    ["http:"], ["https:///"], ["https://"], ["https://user:secret@example.org"],
    ["file:relative.html"], ["file://remote-host/tmp/a.html"], ["file:///tmp/%00"],
    ["https://example.org/%xx"], ["about:blank", "extra"],
]
accepted = [[], ["about:blank"], ["https://example.org/"], ["http://example.org/"],
            ["HTTPS://example.org/"], ["file:///tmp/a%20b.html"],
            ["file://localhost/tmp/a.html"]]
for arguments, expected in [(a, 2) for a in rejected] + [(a, 1) for a in accepted]:
    result = subprocess.run([binary, *arguments], env=environment, capture_output=True, timeout=5)
    assert result.returncode == expected, (arguments, result.returncode, result.stderr)
    # User-provided text is absent from application usage/error output.
    assert b"secret" not in result.stderr
print(f"PASS: {len(rejected)} rejected inputs; {len(accepted)} accepted targets reach native Wayland check")
