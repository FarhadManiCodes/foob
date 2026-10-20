#!/usr/bin/env python3
"""The logging ban (D-61): foob's output never carries a URL, host, title, error text,
cookie or input contents. Reads src/ and fails, with file:line, on:

- an output call (stdio, write, perror, GLib's print and log functions) outside main.c
  and diagnostics.c;
- in main.c, an output argument other than a string literal, FOOB_VERSION, WebKit's
  version numbers, stdout or stderr;
- the word char in diagnostics.h, whose interface takes no text;
- in diagnostics.c, any output call but the one in log_line(), and a log_line() whose
  format is not literal, has a %c, %p or %n, or gives a %s anything but a literal, a
  choice between two literals, an event_names[] entry, g_quark_to_string(name) or
  g_type_name(name).

A text check of a small source tree, not a C parser: a macro could hide a call from it.
The nested suite's logs-redacted scenarios check the real output.

    python3 tests/check_logging.py [source directory]
"""

from pathlib import Path
import re
import sys

OUTPUT = re.compile(
    r'\b(printf|fprintf|dprintf|v[a-z]*printf|puts|fputs|putc|putchar|fputc|fwrite|write|perror'
    r'|syslog|g_print|g_printerr|g_log\w*|g_message|g_warning\w*|g_critical|g_debug|g_info'
    r'|g_error)\s*\(')
LITERALS = r'(?:"(?:[^"\\]|\\.)*"\s*)+'
MAIN_ARGUMENT = re.compile(rf'{LITERALS}|FOOB_VERSION|webkit_get_\w+_version\(\)|stdout|stderr')
NAME = r'\w+(?:->\w+|\.\w+)*'
STRING_ARGUMENT = re.compile(
    rf'{LITERALS}|[^?]+\?\s*{LITERALS}:\s*{LITERALS}|event_names\[{NAME}\]'
    rf'|g_quark_to_string\({NAME}\)|g_type_name\({NAME}\)')
CONVERSION = re.compile(r'%(?:%|[-+ #0]*(?:\d+|\*)?(?:\.(?:\d+|\*))?(?:hh|h|ll|l|L|z|j|t)?([a-zA-Z]))')


def strip_comments(text):
    """Blanks comments, keeping string literals and line numbers."""
    pattern = re.compile(r'"(?:[^"\\\n]|\\.)*"|\'(?:[^\'\\\n]|\\.)*\'|/\*.*?\*/|//[^\n]*', re.S)
    return pattern.sub(lambda m: m[0] if m[0][0] in '"\'' else re.sub(r'[^\n]', ' ', m[0]), text)


def arguments(text, start):
    """The top-level arguments of the call whose '(' is at start, and the end offset."""
    depth, current, found, i = 0, [], [], start
    while i < len(text):
        c = text[i]
        if c in '"\'':
            end = re.compile(rf'{c}(?:[^{c}\\]|\\.)*{c}', re.S).match(text, i).end()
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


def literal_text(argument):
    return ''.join(re.findall(r'"((?:[^"\\]|\\.)*)"', argument))


def check(source):
    problems = []
    for path in sorted(source.glob('*.[ch]')):
        text = strip_comments(path.read_text(encoding='utf-8'))

        def report(offset, message, path=path, text=text):
            problems.append(f'{path}:{text.count(chr(10), 0, offset) + 1}: {message}')

        calls = list(OUTPUT.finditer(text))
        if path.name == 'diagnostics.h' and re.search(r'\bchar\b', text):
            report(re.search(r'\bchar\b', text).start(), 'the diagnostics interface takes no text')
        if path.name not in ('main.c', 'diagnostics.c'):
            for m in calls:
                report(m.start(), f'{m[1]}() outside main.c and diagnostics.c')
        elif path.name == 'main.c':
            for m in calls:
                for argument in arguments(text, m.end() - 1)[0]:
                    if not MAIN_ARGUMENT.fullmatch(argument):
                        report(m.start(), f'{m[1]}() prints {argument!r}')
        else:
            if [re.sub(r'\s', '', text[m.start():arguments(text, m.end() - 1)[1] + 1]) for m in calls] \
                    != ['g_printerr("foob:%s\\n",message)']:
                report(calls[0].start() if calls else 0,
                       'log_line()\'s g_printerr must be the only output call')
            for m in re.finditer(r'\blog_line\s*\(', text):
                if re.search(r'\bvoid\s+$', text[:m.start()]):
                    continue  # the definition
                found = arguments(text, m.end() - 1)[0]
                if len(found) < 2 or not re.fullmatch(LITERALS, found[1]):
                    report(m.start(), 'log_line() needs a literal format')
                    continue
                conversions = [c for c in CONVERSION.findall(literal_text(found[1])) if c]
                if len(conversions) != len(found) - 2:
                    report(m.start(), 'log_line(): format and arguments differ in number')
                    continue
                for conversion, argument in zip(conversions, found[2:], strict=True):
                    if conversion in 'cpn':
                        report(m.start(), f'log_line(): %{conversion} is not allowed')
                    elif conversion == 's' and not STRING_ARGUMENT.fullmatch(argument):
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
