"""Binary contract (D-47): hardening flags, linked libraries and banned imports.

python3 tests/check_binary.py build/foob [--fortify]

Reads the ELF with binutils (readelf, nm, strip, size); standard library only. Exit 77
(a skip for meson) when binutils are missing or the binary is a sanitizer or coverage
build, whose runtime adds its own imports. Prints the stripped size and the section sizes
for the per-task record (D-35).
"""
import re
import shutil
import subprocess
import sys
import tempfile

binary = sys.argv[1]
fortify = '--fortify' in sys.argv[2:]
if not all(shutil.which(tool) for tool in ('readelf', 'nm', 'strip', 'size')):
    print('SKIP: binutils missing')
    sys.exit(77)


def run(*arguments):
    return subprocess.run(arguments, check=True, capture_output=True, text=True).stdout


dynamic = run('readelf', '-W', '-d', binary)
segments = run('readelf', '-W', '-l', binary)
header = run('readelf', '-W', '-h', binary)
notes = run('readelf', '-W', '-n', binary)
symbols = run('nm', '-D', binary).splitlines()
imports = {line.split()[-1].split('@')[0] for line in symbols if line.split()[-2:-1] == ['U']}
defined = {line.split()[-1] for line in symbols if len(line.split()) == 3}
# The full symbol table, where present, also shows a statically linked coverage runtime.
static = subprocess.run(['nm', binary], check=False, capture_output=True, text=True).stdout.split()  # stripped: none
if any(name.startswith(('__asan_', '__ubsan_', '__msan_', '__tsan_', '__gcov_', '__llvm_profile_'))
       for name in imports | defined | set(static)):
    print('SKIP: sanitizer or coverage build')
    sys.exit(77)

needed = set(re.findall(r'\(NEEDED\)\s+Shared library: \[([^\]]+)\]', dynamic))
# Programs foob starts get an argument vector, never a shell or an opener given page text
# (D-31); unbounded string writes and loading code at run time have no use here. A task
# that needs one changes this list and says why.
banned = {
    'system', 'popen', 'wordexp', 'g_spawn_command_line_sync', 'g_spawn_command_line_async',
    'g_shell_parse_argv', 'g_app_info_launch_default_for_uri',
    'g_app_info_launch_default_for_uri_async', 'dlopen', 'dlmopen', 'gets', 'strcpy',
    'strcat', 'sprintf', 'vsprintf',
}
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


check(re.search(r'Type:\s+DYN\b', header) and re.search(r'FLAGS_1\).*\bPIE\b', dynamic),
      'not a position-independent executable')
check(re.search(r'\(FLAGS\).*\bBIND_NOW\b', dynamic) and re.search(r'FLAGS_1\).*\bNOW\b', dynamic),
      'lazy binding: link with -z now (full RELRO)')
check('GNU_RELRO' in segments, 'no RELRO segment')
stack = re.search(r'GNU_STACK(?:\s+\S+){5}\s+(\S+)', segments)
check(stack and 'E' not in stack[1], 'executable stack')
check(not re.search(r'\((RPATH|RUNPATH|TEXTREL)\)', dynamic), 'RPATH, RUNPATH or text relocations')
if 'X86-64' in header or 'x86-64' in header:
    check(re.search(r'x86 feature: .*IBT', notes) and re.search(r'x86 feature: .*SHSTK', notes),
          'no CET (IBT and SHSTK) property')
check('__stack_chk_fail' in imports, 'no stack protector')
# A fortified build imports a *_chk function only where glibc can check a call; today
# that is one, __printf_chk, from main.c's --version output. If a change leaves no such
# call, this fails although fortify is on: drop --fortify in meson.build and say why.
check(not fortify or any(name.endswith('_chk') for name in imports),
      'no fortified call (_FORTIFY_SOURCE; see the comment above this check)')
allowed = {'libglib-2.0.so.0', 'libgobject-2.0.so.0', 'libWPEWebKit-2.0.so.1', 'libc.so.6'}
check(needed <= allowed and 'libWPEWebKit-2.0.so.1' in needed,
      f'linked libraries {sorted(needed)}, expected a subset of {sorted(allowed)}')
check(not imports & banned, f'banned imports: {sorted(imports & banned)}')

with tempfile.TemporaryDirectory() as scratch:
    stripped = f'{scratch}/foob'
    run('strip', '-o', stripped, binary)
    with open(stripped, 'rb') as handle:
        stripped_size = len(handle.read())
sections = run('size', binary).splitlines()[1].split()
if failures:
    sys.exit('FAIL: ' + '; '.join(failures))
print(f'PASS: hardening, {len(needed)} libraries, {len(imports)} imports, none banned; '
      f'stripped {stripped_size} B, text {sections[0]} B, data {sections[1]} B, bss {sections[2]} B')
