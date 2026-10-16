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

Default build: `debugoptimized`, warning level 3, `-std=c23`.
Use four spaces, lowercase function names and explicit object ownership.
GLib/GObject is the engine boundary; its convenience macros may use compiler
extensions. The application does not claim an extension-free ISO C build.

## License

foob is released under the [BSD 2-Clause License](LICENSE).

## Acknowledgements

foob is built on [WPE WebKit](https://wpewebkit.org/), developed by the WebKit project
and Igalia. Its MiniBrowser served as the reference implementation.

## Run and validate

```sh
./build/foob                              # about:blank
./build/foob https://en.wikipedia.org/wiki/Mathematics
FOOB_DIAGNOSTICS=1 ./build/foob file://$PWD/tests/fixtures/viewport.html
meson test -C build --print-errorlogs
```

`foob [URI]` accepts one explicit HTTP(S) URI, a local absolute `file:` URI,
or `about:blank`. HTTP credentials and non-local file hosts are rejected.
The M1 prototype is for trusted pages; comprehensive navigation/resource
policy belongs to M2/M4. It uses one native Wayland window and an explicit
ephemeral session. The `WPE_PLATFORM` environment cannot select another backend
because the program constructs `WPEDisplayWayland` directly. Closing the window,
SIGINT and SIGTERM release the view/session/display and exit.

Diagnostics report load results, elapsed navigation time, WPE logical viewport,
scale, focus/window state and abnormal web-process termination. They omit URLs,
error-message text, cookies and input contents. Navigation completion timing is
**not first-visible-frame timing**. Deferred/coalesced geometry callbacks are
cancelled on shutdown. Engine/system warnings can still appear with diagnostics
disabled.

The [desktop checklist](docs/validation/desktop-checklist.md) passes; the
[validation/handoff](docs/validation/handoff.md) records Gate A readiness.
The standard-library Python scripts under `tests/benchmark/` need desktop access
and readable process PSS. Run `python3 tests/benchmark/suite.py` for serial fresh
launches and a separate local warm-reload control; each suite run writes a new
`docs/validation/runs/suite-<UTC time>/` folder and never overwrites earlier runs
(`run.py` also refuses an existing output path). No Python packages are needed.
Each opens a test window; do this when desktop interaction is available.
The suite checks all browser descendants, including sandbox helpers and the D-Bus
proxy, and checks their disappearance after shutdown.

Generated `docs/validation/runs/` outputs are ignored. Commit reviewed Markdown
and compact `docs/validation/results.json`. `python3 tests/benchmark/summarize.py` regenerates it from the top-level runs; to summarize a suite run give its folder and a new output, for example
`python3 tests/benchmark/summarize.py docs/validation/runs/suite-<time> --output docs/validation/results-<name>.json` (`--replace` overwrites an existing file; empty input is rejected). See
[blank memory/GPU attribution](docs/validation/blank-memory.md) for the optional
`--memory-details` collector and current findings.

For guided interaction, `python3 tests/benchmark/interactive.py --uri URI
--output /tmp/foob-interactive.json` keeps the window open until the user closes
it, then records exit/process cleanup. Choose a new output path for each run.
