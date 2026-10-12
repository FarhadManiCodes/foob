project('foob', 'c',
    meson_version: '>=1.12.0',
    default_options: ['c_std=c23', 'buildtype=debugoptimized', 'warning_level=3'])

cc = meson.get_compiler('c')
if not cc.compiles('''
    #if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #error C23 is required
    #endif
    int main(void) { bool ready = true; return ready ? 0 : 1; }
''', name: 'C23 language and version')
    error('foob requires C23; no older-language fallback is supported')
endif

wpe = dependency('wpe-webkit-2.0', version: '>=2.54.1')
platform = dependency('wpe-platform-2.0', version: '>=2.54.1')
wayland = dependency('wpe-platform-wayland-2.0', version: '>=2.54.1')

executable('foob', 'src/main.c', dependencies: [wpe, platform, wayland])
