"""Branches no test takes in the modules that read untrusted text (D-48).

meson setup <dir outside the repository> -Db_coverage=true
meson test -C <dir>
python3 tests/coverage_report.py <dir>

Prints each source line with a branch outcome no test reached. Each one gets a test,
is removed, or is known: an expansion of a GLib macro or inline function, or a
defensive check. Standard library only; needs gcov (part of GCC). There is no
percentage threshold.
"""
from pathlib import Path
import re
import subprocess
import sys

build = Path(sys.argv[1]).resolve()
for module in ('target', 'window_title'):
    notes = build / 'libfoob-core.a.p' / f'src_{module}.c.gcno'
    if not notes.exists():
        sys.exit(f'{notes} missing: configure with -Db_coverage=true and run meson test')
    # --stdout: no .gcov files are written.
    report = subprocess.run(['gcov', '--branch-probabilities', '--branch-counts', '--stdout',
                             str(notes.relative_to(build))],
                            cwd=build, check=True, capture_output=True, text=True).stdout
    source, untaken, inside, seen = None, [], False, False
    for line in report.splitlines():
        counted = re.match(r'\s*[^:]+:\s*(\d+):(.*)', line)
        if counted and counted[1] == '0' and counted[2].startswith('Source:'):
            # Not GLib's or libc's headers; the path is relative to the build directory.
            inside = re.search(rf'(^|/)src/{module}\.c$', counted[2].removeprefix('Source:')) is not None
            seen = seen or inside
        elif counted and counted[1] != '0' and inside:
            source = (int(counted[1]), counted[2].strip())
        elif re.match(r'branch\s+\d+ (taken 0\b|never executed)', line) and source and inside:
            untaken.append(source)
    if not seen:  # otherwise a path mismatch would read as full coverage
        sys.exit(f'gcov reported no lines of src/{module}.c')
    print(f'== src/{module}.c: {len(untaken)} untaken branch outcomes')
    for number, text in dict.fromkeys(untaken):
        print(f'{number:5}: {text[:100]}')
