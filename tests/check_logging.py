#!/usr/bin/env python3
"""The logging ban (D-61, D-63): foob's output never carries a URL, host, title, error
text, cookie or input contents. Reads src/ and fails, with file:line, on:

- an output call (the names and prefixes below) outside main.c and diagnostics.c;
- in main.c, an output call other than printf or fputs, an argument other than a string
  literal, FOOB_VERSION, WebKit's version numbers, stdout or stderr, or a printf format
  the rule below refuses;
- in diagnostics.h, a parameter of a type other than foob's own, GType and the two views,
  a '...' or a function pointer: the interface takes no text;
- in diagnostics.c, any output call but the one in log_line(), and a log_line() whose
  format is not literal or refused, or that gives a %s anything but a literal, a choice
  between two literals, an event_names[] entry, g_quark_to_string(error->domain) or
  g_type_name(name).

A format is refused for a numeric escape (\\x25 is a hidden %) or a conversion other than
an integer, a floating-point number or a plain %s (so no %m, %ls, %c, %p, %n or '*').

A text check of a small source tree, not a C parser: a macro could hide a call from it.
The nested suite's logs-redacted scenarios check the real output.

    python3 tests/check_logging.py [source directory]
"""

from pathlib import Path
import re
import sys

# Calls that write somewhere: stdio (each also as _unlocked), POSIX writes and sockets,
# error reporters, syslog, and GLib's print, log and file writers; then whole prefixes
# (GLib's assertions and test messages print the values they compare).
NAMES = '''printf fprintf dprintf vprintf vfprintf vdprintf wprintf fwprintf vwprintf vfwprintf
    puts fputs putws fputws putc fputc putwc fputwc putchar putwchar fwrite
    write writev pwrite pwritev pwrite64 pwritev2 send sendto sendmsg
    perror psignal psiginfo err errx verr verrx warn warnx vwarn vwarnx error error_at_line
    syslog vsyslog g_print g_printerr g_printf g_fprintf g_vprintf g_vfprintf g_message
    g_critical g_debug g_info g_error g_io_channel_write_chars g_io_channel_write_unichar'''
PREFIXES = '''sd_journal_ g_log g_warning g_warn_ g_assert g_test_ g_output_stream_
    g_data_output_stream_put_ g_file_set_contents g_file_replace'''
OUTPUT = re.compile(rf"\b((?:{'|'.join(NAMES.split())})(?:_unlocked)?"
                    rf"|(?:{'|'.join(PREFIXES.split())})\w*)\s*\(")
LITERALS = r'(?:"(?:[^"\\]|\\.)*"\s*)+'
MAIN_ARGUMENT = re.compile(rf'{LITERALS}|FOOB_VERSION|webkit_get_\w+_version\(\)|stdout|stderr')
STRING_ARGUMENT = re.compile(
    rf'{LITERALS}|[^?]+\?\s*{LITERALS}:\s*{LITERALS}|event_names\[\w+\]'
    r'|g_quark_to_string\(error->domain\)|g_type_name\(\w+\)')
HEADER_TYPES = {'FoobDiagnostics', 'FoobEvent', 'GType', 'WebKitWebView', 'WPEView', 'void'}
CONVERSION = re.compile(r"%[-+ #0-9.*'I]*(?:hh|h|ll|l|L|z|j|t|q)?(?:.|$)", re.S)
ALLOWED = re.compile(r'%%|%[-+ #0]*\d*(?:\.\d+)?(?:(?:hh|h|ll|l|z|j|t)?[diuxX]|[fFeEgG]|s)')
QUOTED = {q: re.compile(rf'{q}(?:[^{q}\\]|\\.)*{q}', re.S) for q in '"\''}


def strip_comments(text):
    """Blanks comments, keeping string literals and line numbers."""
    pattern = re.compile(r'"(?:[^"\\\n]|\\.)*"|\'(?:[^\'\\\n]|\\.)*\'|/\*.*?\*/|//[^\n]*', re.S)
    return pattern.sub(lambda m: m[0] if m[0][0] in '"\'' else re.sub(r'[^\n]', ' ', m[0]), text)


def arguments(text, start):
    """The top-level arguments of the call whose '(' is at start, and the end offset."""
    depth, current, found, i = 0, [], [], start
    while i < len(text):
        c = text[i]
        if c in QUOTED:
            end = QUOTED[c].match(text, i).end()
            current.append(text[i:end])
            i = end
            continue
        if c in '([{':
            depth += 1
            if depth == 1:
                i += 1
                continue
        elif c in ')]}':
            depth -= 1
            if depth == 0:
                found.append(''.join(current).strip())
                return [a for a in found if a], i
        elif c == ',' and depth == 1:
            found.append(''.join(current).strip())
            current = []
            i += 1
            continue
        current.append(c)
        i += 1
    raise ValueError('unbalanced call')


def format_conversions(literal):
    """The conversion letters of a literal format, or the reason it is refused."""
    text = ''.join(re.findall(r'"((?:[^"\\]|\\.)*)"', literal))
    if re.search(r'\\[0-7xuU]', text):
        return 'a numeric escape'
    specs = [c for c in CONVERSION.findall(text) if c != '%%']
    refused = [c for c in specs if not ALLOWED.fullmatch(c)]
    return ', '.join(refused) if refused else [c[-1] for c in specs]


def check(source):
    problems = []
    for path in sorted(source.glob('*.[ch]')):
        text = strip_comments(path.read_text(encoding='utf-8'))

        def report(offset, message, path=path, text=text):
            problems.append(f'{path}:{text.count(chr(10), 0, offset) + 1}: {message}')

        calls = list(OUTPUT.finditer(text))
        if path.name == 'diagnostics.h':
            for m in re.finditer(r'\b\w+\s*\(([^;]*)\)\s*;', text):
                if '...' in m[1] or '(' in m[1]:
                    report(m.start(), 'the diagnostics interface takes no variadic or function parameter')
                    continue
                for parameter in m[1].split(','):
                    kind = [w for w in re.findall(r'\w+', parameter) if w != 'const'][:1]
                    if kind and kind[0] not in HEADER_TYPES:
                        report(m.start(), f'the diagnostics interface takes no {kind[0]}')
        if path.name not in ('main.c', 'diagnostics.c'):
            for m in calls:
                report(m.start(), f'{m[1]}() outside main.c and diagnostics.c')
        elif path.name == 'main.c':
            for m in calls:
                found = arguments(text, m.end() - 1)[0]
                if m[1] not in ('printf', 'fputs'):
                    report(m.start(), f'{m[1]}() in main.c')
                for argument in found:
                    if not MAIN_ARGUMENT.fullmatch(argument):
                        report(m.start(), f'{m[1]}() prints {argument!r}')
                conversions = format_conversions(found[0]) if m[1] == 'printf' and found else []
                if isinstance(conversions, str):
                    report(m.start(), f'printf(): {conversions} not allowed')
        else:
            if [re.sub(r'\s', '', text[m.start():arguments(text, m.end() - 1)[1] + 1]) for m in calls] \
                    != ['g_printerr("foob:%s\\n",message)']:
                report(calls[0].start() if calls else 0,
                       'log_line()\'s g_printerr must be the only output call')
            for m in re.finditer(r'\blog_line\s*\(', text):
                if re.search(r'\bvoid\s+$', text[max(0, m.start() - 16):m.start()]):
                    continue  # the definition
                found = arguments(text, m.end() - 1)[0]
                if len(found) < 2 or not re.fullmatch(LITERALS, found[1]):
                    report(m.start(), 'log_line() needs a literal format')
                    continue
                conversions = format_conversions(found[1])
                if isinstance(conversions, str):
                    report(m.start(), f'log_line(): {conversions} not allowed')
                elif len(conversions) != len(found) - 2:
                    report(m.start(), 'log_line(): format and arguments differ in number')
                else:
                    for conversion, argument in zip(conversions, found[2:], strict=True):
                        if conversion == 's' and not STRING_ARGUMENT.fullmatch(argument):
                            report(m.start(), f'log_line(): %s of {argument!r}')
    return problems


def main():
    source = Path(sys.argv[1] if len(sys.argv) > 1 else 'src')
    problems = check(source)
    for problem in problems:
        print(problem, file=sys.stderr)
    sys.exit(1 if problems else 0)


if __name__ == '__main__':
    main()
