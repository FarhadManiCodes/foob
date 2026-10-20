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
    '-Werror=format-security', '-ftrivial-auto-var-init=zero',
    '-fstack-protector-strong', '-fstack-clash-protection', '-fcf-protection=full',
    '-fno-plt'), language: 'c')
# Fortify needs optimization, and a caller may already choose a level (makepkg via
# CFLAGS, which Meson stores in c_args at first configure, or -Dc_args=...);
# respect that instead of redefining it.
if get_option('optimization') in ['1', '2', '3', 's'] and not ' '.join(get_option('c_args')).contains('_FORTIFY_SOURCE')
    add_project_arguments('-D_FORTIFY_SOURCE=3', language: 'c')
endif
add_project_arguments('-DFOOB_VERSION="@0@"'.format(meson.project_version()), language: 'c')
add_project_link_arguments(cc.get_supported_link_arguments(
    '-Wl,-z,relro', '-Wl,-z,now', '-Wl,-z,noexecstack', '-Wl,--as-needed'), language: 'c')

glib = dependency('glib-2.0', version: '>=2.74')
gobject = dependency('gobject-2.0')
wpe = dependency('wpe-webkit-2.0', version: '>=2.54.1')
platform = dependency('wpe-platform-2.0', version: '>=2.54.1')
wayland = dependency('wpe-platform-wayland-2.0', version: '>=2.54.1')

# Pure GLib logic, kept free of WPE so it is unit-testable without a display.
core = static_library('foob-core', 'src/target.c', 'src/window_title.c',
    include_directories: 'src', dependencies: glib)

foob = executable('foob', 'src/main.c', 'src/browser.c', 'src/diagnostics.c',
    link_with: core, include_directories: 'src',
    dependencies: [glib, gobject, wpe, platform, wayland])

# test-target also links WebKit, to check that WebKit's parser keeps foob's canonical form.
foreach unit : ['target', 'window_title']
    test(unit, executable('test-' + unit, 'tests/test_' + unit + '.c',
        link_with: core, include_directories: 'src',
        dependencies: unit == 'target' ? [glib, wpe] : glib))
endforeach

# Fuzz targets (D-46). Every build replays the stored inputs as a test; -Dfuzz=true
# (clang) also builds the libFuzzer binaries, with the code under test instrumented.
if get_option('fuzz') and cc.get_id() != 'clang'
    error('-Dfuzz=true needs clang (libFuzzer)')
endif
fuzz_core = get_option('fuzz') ? static_library('foob-core-fuzz', 'src/target.c', 'src/window_title.c',
    include_directories: 'src', dependencies: glib, c_args: '-fsanitize=fuzzer-no-link') : core
foreach unit : ['target', 'window_title']
    source = 'tests/fuzz/fuzz_' + unit + '.c'
    deps = unit == 'target' ? [glib, wpe] : glib
    test('fuzz seeds ' + unit, executable('replay-' + unit, source, 'tests/fuzz/replay.c',
        link_with: core, include_directories: 'src', dependencies: deps),
        args: meson.project_source_root() / 'tests/fuzz/seeds' / unit)
    if get_option('fuzz')
        executable('fuzz-' + unit, source, link_with: fuzz_core, include_directories: 'src',
            dependencies: deps, c_args: '-fsanitize=fuzzer', link_args: '-fsanitize=fuzzer')
    endif
endforeach

python = find_program('python3', required: false)
if python.found()
    test('startup URI contract', python,
        args: [files('tests/check_cli.py'), foob], timeout: 30)
endif
