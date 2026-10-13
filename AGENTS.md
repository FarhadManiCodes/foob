# Repository Guidelines

## Project Structure & Module Organization

Foob contains an M1 prototype for a minimal, keyboard-driven Wayland browser. `foob_architecture_decision.md` defines technical decisions; `foob_development_roadmap.md` defines milestones and acceptance criteria. Start with `docs/README.md`, then read `docs/validation/handoff.md` (current state, rules) and `docs/validation/findings.md` (decisions, findings, ideas) before resuming, and update them in the same commit as your change.

`docs/research/qvu/` contains historical Qt experiments, not current implementation requirements. Local mockups in `design-references/qvu/` are ignored by Git and may be absent from fresh checkouts.

The layout uses `src/` for C code, `tests/fixtures/` for HTML cases, and `tests/benchmark/` for measurement scripts. Start with `src/main.c` and `meson.build`. Modules (`src/`):

- `main.c`: argument check and startup only.
- `target.[ch]`: the startup-URI allowlist, `foob_uri_is_local()` (the single trust decision: `about:blank` and local `file:` are trusted, everything else is untrusted) and `foob_navigation_allowed()` (remote content never navigates to local content; required before any load foob makes on a page's behalf, of which there are none since F-51). GLib only, unit-tested by `tests/test_target.c`.
- `window_title.[ch]`: page title to compositor-visible window title (`<title> — foob`); remote titles are sanitized. GLib only, unit-tested by `tests/test_window_title.c`.
- `diagnostics.[ch]`: redacted logging; it must never print URLs, titles, error text, cookies or input contents.
- `browser.[ch]`: the view lifecycle, one window, all shutdown paths stop one main loop, teardown in dependency order (`foob_diagnostics_free()` cancels the deferred geometry source, so any new idle or timeout source must be cancelled on the same path); signal handlers for close, display loss, web-process death, downloads (cancelled until M3) and new-window policy (an injected script turns a user's `target=_blank` link or form into the page's own `_top` navigation; every remaining new-window request is ignored, F-51). It builds `WPEDisplayWayland` directly, so the `WPE_PLATFORM` environment variable cannot select another backend.
- `exit_status.h`: exit codes. 1 start failed, 2 rejected argument, 3 compositor connection lost, 4 web process died; `tests/check_cli.py` depends on 1 and 2.

Split further as responsibilities emerge (the policy handlers are the next candidate, roadmap M2-03). Tests: `meson test -C build` runs the two unit tests and `tests/check_cli.py` and needs no display. `python3 tests/nested_lifecycle.py build/foob [--keep] [scenario ...]` runs lifecycle, title, download, crash and fixture scenarios in a disposable nested niri (it opens a window on the desktop, removes its scratch directory after a pass); every scenario should keep a negative control that fails on the pre-fix behaviour. `tests/policy_server.py` serves the policy fixtures for guided checks (`docs/validation/guided-checks-m1-audit.md`); input cannot be synthesized here, so clicks stay guided checks. Milestone audits follow the roadmap's triage rule (section 7) and record evidence in `docs/validation/`.

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

No coverage threshold exists. Give each milestone task a repeatable acceptance test. Automate stable behavior and document manual niri resize, focus, scrolling, and shutdown checks. The `tests/benchmark/` scripts open real windows and read `/proc/*/smaps_rollup`, so run them only with desktop interaction available; each run records binary and source SHA256, the git revision and whether the tree is dirty, so commit before measuring and pick a new `--output` path every run. Update validation evidence in the same commit as the change it covers. Record package versions, viewport dimensions, workloads, and total browser process-tree PSS for benchmarks; the under-100-MiB goal must be assessed from the current validation evidence. Interactive checks remain required before Gate A.

## Commit & Pull Request Guidelines

Use concise, imperative subjects. Keep changes small and tied to roadmap outcomes, such as `M1-03`. Keep M0/M1 evidence with their implementation commits.

PR descriptions should explain behavior, link relevant tasks, and include validation evidence. Add screenshots when visual behavior changes.

## Architecture & Security Boundaries

Preserve the toolkit-free WPE direction: one view per window, no persistent chrome, and external PDF viewing. Keep custom engine packaging separate. Never disable TLS checks or sandbox protections to resolve compatibility failures, or commit browser profiles and credentials. Treat everything from outside this machine as untrusted (page titles, URLs, responses); no download may start or file be created without an explicit user decision, so downloads stay cancelled until roadmap M3-03. `window.open()` is unsupported until the popup policy is decided (M2-11, M3-01).
