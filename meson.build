project('foob', 'c',
    version: '0.2.0-dev',  # milestone = minor version; 0.2.0 at the M2 exit (D-39)
    license: 'BSD-2-Clause', license_files: 'LICENSE',
    meson_version: '>=1.12.0',
    default_options: ['c_std=c23', 'buildtype=debugoptimized', 'warning_level=3', 'b_pie=true'])

cc = meson.get_compiler('c')
if not cc.compiles('''
    #if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #error C23 is required
    #endif
    int main(void) { bool ready = true; return ready ? 0 : 1; }
''', name: 'C23 language and version')
    error('foob requires C23; no older-language fallback is supported')
endif

# Extra diagnostics and exploit mitigations, each used only if the compiler
# accepts it. Optimization stays at the buildtype's level (-O2 by default).
add_project_arguments(cc.get_supported_arguments(
    '-Wshadow', '-Wformat=2', '-Wnull-dereference', '-Wstrict-prototypes',
    '-Wmissing-prototypes', '-Wundef', '-Wcast-align', '-Wwrite-strings',
    '-Wconversion', '-Wsign-conversion', '-Wcast-qual', '-ftrivial-auto-var-init=zero',
    '-fstack-protector-strong', '-fstack-clash-protection', '-fcf-protection=full',
    '-fno-plt'), language: 'c')
# Checked as a pair: GCC refuses -Werror=format-security on its own (it needs -Wformat),
# which once dropped it unnoticed (D-52).
if cc.has_multi_arguments('-Wformat', '-Werror=format-security')
    add_project_arguments('-Wformat', '-Werror=format-security', language: 'c')
endif
# Fortify needs optimization, and a caller may already choose a level (makepkg via
# CFLAGS, which Meson stores in c_args at first configure, or -Dc_args=...);
# respect that instead of redefining it.
fortify = get_option('optimization') in ['1', '2', '3', 's']
caller_args = ' '.join(get_option('c_args'))
if fortify and not caller_args.contains('_FORTIFY_SOURCE')
    add_project_arguments('-D_FORTIFY_SOURCE=3', language: 'c')
endif
# Whether glibc makes checked calls, so the binary contract expects one only then: it
# checks printf from fortify level 2 (D-62). The compiler answers with the caller's
# arguments, as the build reads them (the last of several levels, -Wp,... forms), and the
# build's optimization, without which glibc turns fortify off (D-63). Meson's own checks
# add -O0 -U_FORTIFY_SOURCE after the caller's arguments, so these come again after it.
fortified = fortify and cc.compiles('''
    #include <stdio.h>
    #if !defined(__USE_FORTIFY_LEVEL) || __USE_FORTIFY_LEVEL < 2
    #error no checked printf
    #endif
''', args: get_option('c_args') + ['-O' + get_option('optimization')] +
    (caller_args.contains('_FORTIFY_SOURCE') ? [] : ['-D_FORTIFY_SOURCE=3']),
    name: 'fortify level 2 or higher')
add_project_arguments('-DFOOB_VERSION="@0@"'.format(meson.project_version()), language: 'c')
add_project_link_arguments(cc.get_supported_link_arguments(
    '-Wl,-z,relro', '-Wl,-z,now', '-Wl,-z,noexecstack', '-Wl,--as-needed'), language: 'c')

# System headers: their own warnings (clang's __COUNTER__ in GLib's macros) are not foob's.
glib = dependency('glib-2.0', version: '>=2.74', include_type: 'system')
gobject = dependency('gobject-2.0', include_type: 'system')
wpe = dependency('wpe-webkit-2.0', version: '>=2.54.1', include_type: 'system')
platform = dependency('wpe-platform-2.0', version: '>=2.54.1', include_type: 'system')
wayland = dependency('wpe-platform-wayland-2.0', version: '>=2.54.1', include_type: 'system')

# Pure GLib logic, kept free of WPE so it is unit-testable without a display.
core_sources = files('src/target.c', 'src/window_title.c')
core = static_library('foob-core', core_sources, include_directories: 'src', dependencies: glib)

app_sources = files('src/main.c', 'src/browser.c', 'src/diagnostics.c')
foob = executable('foob', app_sources, link_with: core, include_directories: 'src',
    dependencies: [glib, gobject, wpe, platform, wayland])

# Each module that reads untrusted text has a unit test and a fuzz target (D-46), whose
# stored inputs every build replays; -Dfuzz=true (clang) also builds the libFuzzer
# binaries, with the code under test instrumented. The address module's tests also link
# WebKit, to check that WebKit's parser keeps foob's canonical form.
if get_option('fuzz') and cc.get_id() != 'clang'
    error('-Dfuzz=true needs clang (libFuzzer)')
endif
replay = files('tests/fuzz/replay.c')
test_sources = [replay]
fuzz_core = get_option('fuzz') ? static_library('foob-core-fuzz', core_sources,
    include_directories: 'src', dependencies: glib, c_args: '-fsanitize=fuzzer-no-link') : core
foreach unit : ['target', 'window_title']
    unit_test = files('tests/test_' + unit + '.c')
    fuzz_target = files('tests/fuzz/fuzz_' + unit + '.c')
    test_sources += [unit_test, fuzz_target]
    deps = unit == 'target' ? [glib, wpe] : glib
    test(unit, executable('test-' + unit, unit_test,
        link_with: core, include_directories: 'src', dependencies: deps))
    test('fuzz seeds ' + unit, executable('replay-' + unit, fuzz_target, replay,
        link_with: core, include_directories: 'src', dependencies: deps),
        args: meson.project_source_root() / 'tests/fuzz/seeds' / unit)
    if get_option('fuzz')
        executable('fuzz-' + unit, fuzz_target, link_with: fuzz_core, include_directories: 'src',
            dependencies: deps, c_args: '-fsanitize=fuzzer', link_args: '-fsanitize=fuzzer')
    endif
endforeach

# Required: Meson itself runs on Python 3, and a missing interpreter must not drop tests.
python = find_program('python3')
test('startup URI contract', python, args: [files('tests/check_cli.py'), foob], timeout: 30)
test('binary contract', python,
    args: [files('tests/check_binary.py'), foob] + (fortified ? ['--fortify'] : []))
# The logging ban (D-61): no URL, host, title or page text in foob's output. It reads
# only sources, so it belongs to the suite "source" below, and needs no tool.
test('logging ban', python, args: files('tests/check_logging.py'),
    workdir: meson.project_source_root(), suite: 'source')

# Source checks, each skipped (77) where its tool is not installed: static analysis
# (clang-tidy with .clang-tidy, through this build's compile_commands.json, D-52),
# duplication (jscpd, D-53), McCabe complexity (lizard, D-51) and the lint of the Python
# (ruff with ruff.toml) and shell scripts (shellcheck, D-59), run in the source tree.
# They read only sources, so they form the suite "source", which a second build of the
# same tree can leave out (tests/ci.sh).
foreach check : [
    ['static analysis', 'clang-tidy',
        ['-p', meson.project_build_root(), '--quiet', app_sources, core_sources, test_sources]],
    ['duplication', 'jscpd', ['--no-tips', '--no-colors', '--exit-code', '-f', 'c,python', '--ignore-identifiers', 'src', 'tests']],
    ['complexity', 'lizard', ['-l', 'c', '-w', '-C', '15', 'src']],
    ['python lint', 'ruff', ['check', '--no-cache', 'tests']],
    ['shell lint', 'shellcheck', files('tests/ci.sh')],
]
    tool = find_program(check[1], required: false)
    if tool.found()
        test(check[0], tool, args: check[2], workdir: meson.project_source_root(), timeout: 120,
            suite: 'source')
    else
        test(check[0], python, suite: 'source',
            args: ['-c', 'print("SKIP: @0@ missing"); raise SystemExit(77)'.format(check[1])])
    endif
endforeach
