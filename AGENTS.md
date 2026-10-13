# Repository Guidelines

## Project Structure & Module Organization

Foob contains an M1 prototype for a minimal, keyboard-driven Wayland browser. `foob_architecture_decision.md` defines technical decisions; `foob_development_roadmap.md` defines milestones and acceptance criteria. Start with `docs/README.md` and check `docs/validation/handoff.md` before resuming.

`docs/research/qvu/` contains historical Qt experiments, not current implementation requirements. Local mockups in `design-references/qvu/` are ignored by Git and may be absent from fresh checkouts.

The layout uses `src/` for C code, `tests/fixtures/` for HTML cases, and `tests/benchmark/` for measurement scripts. Start with `src/main.c` and `meson.build`. Modules: `target` (startup allowlist, GLib only, unit-tested by `tests/test_target.c`), `window_title` (page title to window title; remote titles untrusted), `diagnostics` (redacted logging), `browser` (view lifecycle) and `exit_status.h`; split further as responsibilities emerge. `tests/nested_lifecycle.py build/foob` checks close/signal/compositor-loss behaviour against a disposable nested niri.

## Build, Test, and Development Commands

Meson configures a C23 `debugoptimized` build with warning level 3. Environment checks from milestone M0 include:

```sh
pacman -Q wpewebkit meson ninja pkgconf gcc
pkg-config --modversion wpe-webkit-2.0
pkg-config --modversion wpe-platform-2.0
```

These report installed packages and available API modules; the `2.0` suffix denotes the API namespace, not the engine release.

Use `meson setup build` to configure, `meson compile -C build` to build, and `meson test -C build --print-errorlogs` to run registered tests. Launch `./build/foob [URI]`; see README for diagnostics and trusted local fixtures. Python checks use only the standard library and need no environment installation.

## Coding Style & Naming Conventions

Target C23 with GLib/GObject at the WPE integration boundary. Keep application-owned logic in ordinary C and use public WPEPlatform APIs. Use four-space indentation and lowercase function/module names. No formatter or linter is configured. GLib cleanup macros may use documented compiler extensions; do not claim an extension-free build. Use descriptive fixture names such as `viewport.html`.

## Testing Guidelines

No coverage threshold exists. Give each milestone task a repeatable acceptance test. Automate stable behavior and document manual niri resize, focus, scrolling, and shutdown checks. Record package versions, viewport dimensions, workloads, and total browser process-tree PSS for benchmarks; the under-100-MiB goal must be assessed from the current validation evidence. Interactive checks remain required before Gate A.

## Commit & Pull Request Guidelines

Use concise, imperative subjects. Keep changes small and tied to roadmap outcomes, such as `M1-03`. Keep M0/M1 evidence with their implementation commits.

PR descriptions should explain behavior, link relevant tasks, and include validation evidence. Add screenshots when visual behavior changes.

## Architecture & Security Boundaries

Preserve the toolkit-free WPE direction: one view per window, no persistent chrome, and external PDF viewing. Keep custom engine packaging separate. Never disable TLS checks or sandbox protections to resolve compatibility failures, or commit browser profiles and credentials.
