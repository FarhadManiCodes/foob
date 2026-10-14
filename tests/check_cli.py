"""Startup contract checks; standard library only, no display needed."""
import os
from pathlib import Path
import subprocess
import sys

binary = str(Path(sys.argv[1]).resolve())
environment = {**os.environ, "WAYLAND_DISPLAY": "foob-no-such-display"}
# Wiring only: the validity rules themselves are unit-tested in tests/test_target.c.
rejected = [[""], ["example.org"], ["javascript:alert(1)"], ["https://user:secret@example.org"],
            ["about:blank", "extra"]]
accepted = [[], ["https://example.org/"], ["file:///tmp/a%20b.html"]]
for arguments, expected in [(a, 2) for a in rejected] + [(a, 1) for a in accepted]:
    result = subprocess.run([binary, *arguments], env=environment, capture_output=True, timeout=5)
    assert result.returncode == expected, (arguments, result.returncode, result.stderr)
    # User-provided text is absent from application usage/error output.
    assert b"secret" not in result.stderr
print(f"PASS: {len(rejected)} rejected inputs; {len(accepted)} accepted targets reach native Wayland check")
