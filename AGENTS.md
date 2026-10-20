# Repository Guidelines

## Project Structure & Module Organization

Foob contains an M1 prototype for a minimal, keyboard-driven Wayland browser. `foob_architecture_decision.md` defines technical decisions; `foob_development_roadmap.md` defines milestones and acceptance criteria. Start with `docs/README.md`, then read `docs/validation/handoff.md` (current state, rules) and `docs/validation/findings.md` (decisions, findings, ideas) before resuming, and update them in the same commit as your change.

`docs/research/qvu/` contains historical Qt experiments, not current implementation requirements. Local mockups in `design-references/qvu/` are ignored by Git and may be absent from fresh checkouts.

The layout uses `src/` for C code, `tests/fixtures/` for HTML cases, and `tests/benchmark/` for measurement scripts. Start with `src/main.c` and `meson.build`. Modules (`src/`):

- `main.c`: argument check (and `--version`, D-32), the core-dump guard (F-77: core limit 1 for every process, so the kernel aborts any dump, D-41; where 1 cannot work, limit 0 and a non-dumpable foob, and the helpers then rely on the collector honouring their limit, D-42; skipped with `FOOB_DIAGNOSTICS=1`; it runs before the argument is parsed) and startup only.
- `target.[ch]`: the startup-URI allowlist, `foob_target_canonical()`, which returns the form foob loads (ASCII, and one WebKit's parser leaves unchanged, M2-02) or nullptr, `foob_target_from_argument()` (the command line only: a `/`, `./` or `../` argument is a path, relative ones against the kernel's current directory, D-32, D-44), `foob_uri_is_local()` (the single trust decision: `about:blank` and local `file:` are trusted, everything else is untrusted; it reads only scheme and host, so WebKit's reported address need not pass the startup rules, D-45) and `foob_navigation_allowed()` (remote content never navigates to local content; required before any load foob makes on a page's behalf, of which there are none, D-04). GLib and POSIX libc only (no WebKit), unit-tested by `tests/test_target.c`, which also links WebKit to check that WebKit's parser keeps each canonical form.
- `window_title.[ch]`: page title to compositor-visible window title (`<title> — foob`); remote titles are sanitized. GLib only, unit-tested by `tests/test_window_title.c`.
- `diagnostics.[ch]`: redacted logging; it must never print URLs, hosts, titles, error text, cookies or input contents. The only exception is the on-demand dump decided in D-26 (not built yet): hosts but never paths or queries, written only on the user's key to a `0600` file in the profile.
- `browser.[ch]`: the view lifecycle, one window, all shutdown paths stop one main loop, teardown in dependency order (`foob_diagnostics_free()` cancels the deferred geometry source, so any new idle or timeout source must be cancelled on the same path); signal handlers for close, display loss, web-process death, downloads (cancelled, D-02), permission requests (all denied, F-78) and new windows (ignored, D-04; the unused `foob_navigation_allowed()` is the required check for any load M2-03 makes on a page's behalf). It builds `WPEDisplayWayland` directly, so the `WPE_PLATFORM` environment variable cannot select another backend.
- `exit_status.h`: exit codes. 1 start failed (the Wayland display was unreachable, or core dumps could not be turned off), 2 rejected argument, 3 compositor connection lost, 4 web process died; `tests/check_cli.py` depends on 1 and 2.

Split further as responsibilities emerge (the policy handlers are the next candidate, roadmap M2-03). Tests: `meson test -C build` runs the two unit tests, the replay of the stored fuzz inputs and `tests/check_cli.py` and needs no display. `python3 tests/nested_lifecycle.py build/foob [--keep] [scenario ...]` runs lifecycle, title, core-dump, permission, download, crash and new-window scenarios in a disposable nested niri (it opens a window on the desktop, removes its scratch directory after a pass); every scenario should keep a negative control that fails on the pre-fix behaviour. `tests/fuzz/` holds the fuzz targets (D-46): `meson test` replays their stored inputs; for a task commit, build them with `CC=clang meson setup <dir outside the repository> -Dfuzz=true -Db_sanitize=address,undefined -Db_lundef=false` and run each for 5 minutes, `<dir>/fuzz-target -jobs=4 -workers=4 -max_total_time=300 -dict=tests/fuzz/uri.dict <scratch corpus> tests/fuzz/seeds/target` (likewise `fuzz-window_title` with `-max_len=20000`), under `ulimit -c 0`; add each failing input to `tests/fuzz/seeds/` with its fix. `tests/policy_server.py` serves the policy fixtures for guided checks (`docs/validation/guided-checks-m1-audit.md`); input cannot be synthesized here, so clicks stay guided checks. Milestone audits follow the roadmap's triage rule (under *Scope control*) and record evidence in `docs/validation/`.

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

No coverage threshold exists. Give each milestone task a repeatable acceptance test. Automate stable behavior and document manual niri resize, focus, scrolling, and shutdown checks. The `tests/benchmark/` scripts open real windows and read `/proc/*/smaps_rollup`, so run them only with desktop interaction available; each run records binary and source SHA256, the git revision and whether the tree is dirty, so commit before measuring and pick a new `--output` path every run. Update validation evidence in the same commit as the change it covers. Record package versions, viewport dimensions, workloads, and total browser process-tree PSS for benchmarks; the under-100-MiB goal must be assessed from the current validation evidence. Interactive checks remain required before Gate A. Never fake keyboard or mouse input (no `ydotool`, `wtype` or similar); a test that needs a real click is a guided check done by the maintainer. In M2, each task's guided rows go into `docs/validation/guided-checks-m2.md` when its code is done and are run on that task's commit (D-33). Open Google Scholar only on purpose, when a check needs it (M2-08), at most once or twice a minute; never in an automated test, script, benchmark, loop or retry, so Google does not block this machine's IP. Use Wikipedia, Semantic Scholar and local fixtures for automated or repeated checks. Track binary size as a quality signal: record the stripped size and `size` per object each milestone and explain growth over about 10%; in M2, record and explain it at each task's commit, and from M2-04 on measure startup time as well (D-35, D-39).

## Commit & Pull Request Guidelines

Use concise, imperative subjects. Keep changes small and tied to roadmap outcomes, such as `M1-03`. Keep M0/M1 evidence with their implementation commits.

PR descriptions should explain behavior, link relevant tasks, and include validation evidence. Add screenshots when visual behavior changes.

## Architecture & Security Boundaries

Preserve the toolkit-free WPE direction: one view per window, no persistent chrome, and PDFs in an external viewer as the target (M3-04). The only chrome is foob's own line, hidden by default and drawn with fcft and pixman (D-28, D-29); never a toolkit, and never a box inside the page. Keep custom engine packaging separate. Never disable TLS checks or sandbox protections, whether for compatibility, speed or memory, and never commit browser profiles and credentials. Any program foob starts gets an argument vector, never a shell, and never `xdg-open` with page-supplied text (D-31). Treat everything from outside this machine as untrusted (page titles, URLs, responses).

Interim defaults, each a maintainer decision that holds until the roadmap task named replaces it:

- Downloads: no download may start and no file be created without an explicit user decision, so every download is cancelled until M3-03 (D-02).
- New windows: every new-window request is ignored (`window.open()` returns `null`, `target=_blank` links and forms do nothing) and foob loads nothing on a page's behalf, until M2-03, M2-11 and M3-01 (D-04). Its M2 replacement is decided but not built: a real click on a `_blank` link opens a private foob process (D-23).
- Permissions: every permission request is denied (pointer lock included, which WebKit would allow unanswered) until M3-05 decides each one (F-78).
- PDFs: they open inline in the stock PDF.js viewer for now (roadmap M3-04 revisits it if a custom build turns PDF.js off); its save button must still obey the download rule (D-12).
