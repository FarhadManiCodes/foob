# foob

A minimal C23/WPE WebKit browser for native Wayland. The stock Arch engine is
the baseline. See [docs](docs/README.md), the
[architecture decision](foob_architecture_decision.md) and
[roadmap](foob_development_roadmap.md).

## Build

Required packages: `wpewebkit meson ninja pkgconf gcc` (GLib comes with WPE).
The tested engine is Arch `wpewebkit 2.54.1-2`; `2.0` in pkg-config names
is the API namespace, not the engine release. WPEPlatform with Wayland support
is required. Meson >= 1.12 and a C23 compiler are required.

```sh
meson setup build
meson compile -C build
./build/foob
```

The M0 executable checks public type-symbol linkage and exits; M1 adds the
browser window. Default build: `debugoptimized`, warning level 3, `-std=c23`.
Use four spaces, lowercase function names and explicit object ownership.
GLib/GObject is the engine boundary; its convenience macros may use compiler
extensions. The application does not claim an extension-free ISO C build.

Project license: **deferred to the maintainer (M0-05 / P1)**. No license has
been selected. WPE/WebKit and GLib retain their upstream licenses.
