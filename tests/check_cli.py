"""Startup contract checks; standard library only, no display needed."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

binary = str(Path(sys.argv[1]).resolve())
environment = {**os.environ, "WAYLAND_DISPLAY": "foob-no-such-display"}
environment.pop("FOOB_DIAGNOSTICS", None)  # the core-dump guard runs only without diagnostics
# Wiring only: the validity rules themselves are unit-tested in tests/test_target.c.
rejected = [["https://user:secret@example.org"], ["about:blank", "extra"], ["http://%20/"],
            ["example.com"], ["--version", "extra"], [b"/tmp/\xe9.html"]]  # not UTF-8 (D-40)
accepted = [[], ["https://example.org/"], ["./foob-no-such-page.html"], ["/tmp/a b#?.html"]]
# A hard core limit already 0 must not stop foob (D-42): it falls back to a non-dumpable foob.
launchers = [[]] + ([["prlimit", "--core=0:0"]] if shutil.which("prlimit") else [])
for launcher in launchers:
    for arguments, expected in [(a, 2) for a in rejected] + [(a, 1) for a in accepted]:
        result = subprocess.run([*launcher, binary, *arguments], env=environment,
                                capture_output=True, timeout=5)
        assert result.returncode == expected, (launcher, arguments, result.returncode, result.stderr)
        # Exit 1 must come from the display check, not from a refused core-dump guard.
        assert expected != 1 or b"native Wayland" in result.stderr, (launcher, arguments, result.stderr)
        # User-provided text is absent from application usage/error output.
        assert b"secret" not in result.stderr
# --version needs no display: a connection attempt would end in exit 1 (D-32).
result = subprocess.run([binary, "--version"], env=environment, capture_output=True, timeout=5)
assert result.returncode == 0 and not result.stderr, (result.returncode, result.stderr)
assert re.fullmatch(rb"foob \S+\nWPE WebKit \d+\.\d+\.\d+\n", result.stdout), result.stdout
print(f"PASS: {len(rejected)} rejected inputs; {len(accepted)} accepted targets reach native Wayland check;"
      f" {len(launchers)} core-limit setups")
