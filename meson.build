project('foob', 'c',
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
    '-fstack-protector-strong', '-fstack-clash-protection', '-fcf-protection=full',
    '-fno-plt'), language: 'c')
if get_option('optimization') != '0'
    # makepkg may already pass -D_FORTIFY_SOURCE through CFLAGS; undefine first.
    add_project_arguments('-U_FORTIFY_SOURCE', '-D_FORTIFY_SOURCE=3', language: 'c')
endif
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

foreach unit : ['target', 'window_title']
    test(unit, executable('test-' + unit, 'tests/test_' + unit + '.c',
        link_with: core, include_directories: 'src', dependencies: glib))
endforeach

python = find_program('python3', required: false)
if python.found()
    test('startup URI contract', python,
        args: [files('tests/check_cli.py'), foob], timeout: 30)
endif
