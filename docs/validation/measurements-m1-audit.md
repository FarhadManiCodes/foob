# M1 audit: re-recorded process-tree PSS evidence

**The audit binary shows no memory or timing regression against the unmodified
`m0-m1` build, measured interleaved in the same session. The under-100-MiB stretch
target is still not met.** This file supplements [measurements.md](measurements.md);
it does not replace it. That file describes the original `m0-m1` binary, whose
source and binary hashes no longer match this branch.

Collected 2026-10-13 after the Codex fixes and both security reviews (with the
same-window script removed again, D-04); run start times are in the raw records. WPE
Arch `2.54.1-2`; GCC `16.2.1+r23+gd564253eb6c8-1`; Meson 1.12.1 `debugoptimized`,
`-std=c23 -O2 -g` plus the hardening flags in [audit-m1.md](audit-m1.md); kernel
`7.2.9-arch1-1`; niri `26.04-1`; Mesa `1:26.2.4-1`. Machine-readable records:
[results-m1-audit.json](results-m1-audit.json); raw `runs/*.json` and `.log` files
stay ignored and local (`docs/validation/runs/m1-audit-147ae45/`).

## Inputs

| | |
|---|---|
| Revision | `147ae452…` (full hash in every record; `m0-m1`, after the final cleanup review), tree clean (`source_dirty: false` in every record) |
| Source SHA256 (`src/*.c`, `src/*.h`) | `a7c70db4b04272bb7d8eb4710c09a3b435fe3247455054c32f2d7560f978943f` |
| Audit binary SHA256 | `f1c00899786e6581768dc3d8ca60d062dfab07d7a17f5c388ba8254f1382a974` |
| Control binary SHA256 | `3369eeadb352e812bfc7c3f7b1fd009f491ac663062fa3afccf3a424c9245bcb` (unchanged) |
| Fixtures | `viewport.html` unchanged from `m0-m1`; `linked.html` gained a back link and the policy fixtures changed after the first audit measurement, none of them used by these workloads except `viewport.html` |

Earlier measurements of this branch (revisions `2dfe12d`, `9c81515`, `1fae570`,
`9abb6fe`, `89c1ecf`, `0af4314` and `679fb9a` (source hash `667746fc…`), source hashes `7b3129c2…` for `1fae570`, `0c7dc10f…` for `9abb6fe`, `d306ff47…` for `89c1ecf` and `498cc823…` for `0af4314`) are
superseded: each change to C source (sanitizer, exit-status handling, the
`_FORTIFY_SOURCE` check, the new-window handler that once loaded the action's request, the
title sanitizer, the URI allowlist, the removal of the same-window script, the trimmed cut title, and now the merged close handlers and dead-code removal) invalidates the hashes. They showed the
same picture (audit and control within a few MiB). Later commits change only tests,
fixtures, documents and one comment in `src/target.h` (the rebuilt binary is byte-identical).

The control is a build of the unmodified `m0-m1` source made earlier in this audit,
not the binary hashed in `measurements.md` (`9a9225d8…`); it differs because it was
rebuilt. Its records carry the audit tree's `source_sha256` because the collector
hashes the working tree; only `binary_sha256` identifies the control. Any later C
change invalidates the audit hashes above. The binary hash depends on the build
directory (debug info embeds paths); a reviewer building elsewhere got a different one.
The source hash reproduces exactly and is the stable identifier.

## Method

The `tests/benchmark/run.py` collector and the protocol of `measurements.md`: explicit
ephemeral session, three samples one second apart starting at least ten seconds after
the observed successful load, process tree from `/proc` including reparented helpers,
`Pss` summed per PID from `smaps_rollup`. Every sample: **950x1188 logical viewport,
scale 1.0**, eight tracked processes, exit code 0, no survivors after ten seconds.
OS, filesystem and network caches are uncontrolled; Wikipedia is live content. The
maintainer was not using the desktop during this run.

**Interleaved design:** four launches per workload for each binary, alternating audit
and control (audit blank, control blank, audit fixture, control fixture, audit
Wikipedia, control Wikipedia, repeated four times), plus one warm-reload run for the
audit binary, so that session drift hits both builds equally. All 25 runs passed. (The
previous run on `9abb6fe` had one failed fixture launch for both builds, explained at the time:
the fixture reloaded itself, probably from input
to the new window.)

## Results: audit binary

Values are the three samples per launch in MiB.

| Run | Sample 1 | Sample 2 | Sample 3 | Navigation ms |
|---|---:|---:|---:|---:|
| `blank-1` | 171.694 | 171.708 | 171.708 | 0.207 |
| `control-blank-1` | 169.536 | 169.549 | 169.549 | 0.247 |
| `fixture-1` | 194.226 | 194.230 | 194.230 | 16.686 |
| `control-fixture-1` | 196.412 | 196.415 | 196.415 | 15.560 |
| `wikipedia-1` | 331.699 | 330.078 | 330.055 | 1136.185 |
| `control-wikipedia-1` | 330.533 | 328.787 | 328.775 | 525.868 |
| `blank-2` | 169.832 | 169.845 | 169.845 | 0.358 |
| `control-blank-2` | 170.081 | 170.094 | 170.094 | 0.248 |
| `fixture-2` | 193.742 | 193.746 | 193.852 | 16.823 |
| `control-fixture-2` | 194.246 | 194.250 | 194.250 | 15.935 |
| `wikipedia-2` | 330.271 | 328.646 | 328.634 | 545.411 |
| `control-wikipedia-2` | 331.246 | 329.605 | 329.594 | 484.854 |
| `blank-3` | 169.776 | 169.789 | 169.789 | 0.234 |
| `control-blank-3` | 169.608 | 170.491 | 170.491 | 0.241 |
| `fixture-3` | 194.089 | 194.093 | 194.093 | 17.673 |
| `control-fixture-3` | 194.010 | 194.015 | 194.015 | 15.641 |
| `wikipedia-3` | 332.896 | 331.275 | 331.264 | 541.186 |
| `control-wikipedia-3` | 332.615 | 330.994 | 331.033 | 539.450 |
| `blank-4` | 169.766 | 169.778 | 169.778 | 0.276 |
| `control-blank-4` | 170.023 | 170.037 | 170.037 | 0.258 |
| `fixture-4` | 194.083 | 194.087 | 194.087 | 17.544 |
| `control-fixture-4` | 194.167 | 194.171 | 194.171 | 15.362 |
| `wikipedia-4` | 328.410 | 326.902 | 326.891 | 529.495 |
| `control-wikipedia-4` | 356.481 | 357.646 | 353.786 | 541.089 |
| `fixture-warm-reload` | 198.227 | 198.231 | 198.231 | 4.864 reload; 16.713 initial |

## Results: audit versus control, interleaved

| Workload | Audit mean (MiB) | Audit range | Control mean (MiB) | Control range |
|---|---:|---|---:|---|
| Blank | 170.276 (12 samples) | 169.766-171.708 | 169.966 (12) | 169.536-170.491 |
| Local fixture | 194.046 (12) | 193.742-194.230 | 194.711 (12) | 194.010-196.415 |
| Wikipedia | 329.752 (12) | 326.891-332.896 | 336.758 (12) | 328.775-357.646 |

Means differ by 0.31 MiB (audit higher), 0.67 MiB (audit lower) and 7.0 MiB (audit lower, caused by
one control launch at 357 MiB; without it the means are 329.8 and 330.4), all inside the spread of
the launches of each build. Navigation times: blank 0.21-0.36 ms against 0.24-0.26 ms, fixture
16.7-17.7 against 15.4-15.9, Wikipedia 530-545 against 485-541 ms, except one audit launch (the
first Wikipedia launch of the session, `wikipedia-1`) that took 1136 ms with the same memory as the others; a single
network or cold-cache outlier, not repeated in the other three launches. Twelve samples are repeated
observations of four launches, not twelve independent launches.

Single-launch spikes occur in both builds and in every attempt: blank launches 5-12 MiB
away from the others (here: the `control-wikipedia-4` launch at 357 MiB; the `wikipedia-3` audit launch at 356 MiB in the `0af4314` session was another)
and one Wikipedia launch 15-30 MiB high in earlier runs. They are session state, not a
property of either binary, and they are why launches are interleaved and repeated.

## Why these numbers differ from `measurements.md` and from the earlier runs

`measurements.md` reports blank 174.3, fixture 202.4 and Wikipedia 333.7 MiB. In the
session of an earlier run (`89c1ecf`) the **control** (the old code) read 166.2, 194.5 and 327.4 MiB; in this one it reads 169.8, 194.8 and 329.4 MiB, so the lower
numbers are session drift (system state, caches, library mapping sharing), not an
improvement from this branch. Do not cite it as one. Compare builds only within one
session, as above. The earlier [blank-memory diagnosis](blank-memory.md) saw the same 158-171 MiB
range for the original binary.

## Other evidence recorded with this run

- `meson test -C build`: 3 of 3 pass (`target` and `window_title` unit tests,
  startup URI contract). A clean `git archive` export builds without warnings.
- `python3 tests/nested_lifecycle.py build/foob` in a disposable nested niri: 12 of
  12 scenarios passed on `147ae45`; afterwards the suite was trimmed to 8 (test trim, see `handoff.md`) and the 8 passed again on the same binary. Each has exact-PID survivor checks and an app-id assertion:
  `title-local-unsanitized` (a zero-width space in a local page's title survives; renamed from `title-follows-page`), `title-remote-sanitized` (the same character is stripped from a remote title), `download-cancelled` (300 MiB
  stream aborted after under 16 MiB, nothing on disk),
  `blank-form-does-not-navigate` (a trusted `_blank` form submit from the page and from a
  sandboxed frame reaches no `/echo`; the window stays on the page; the script build `f40e4b9`
  fails it), `compositor-close`, `sigint`, `compositor-lost` (exit 3), `web-process-crash` (exit 4). The pre-fix
  binary fails the compositor-loss, download, app-id and web-process scenarios; those
  negative controls were run during the audit and are recorded in prose, not as
  committed logs.
- **Limit of the new-window scenario:** with default settings scripted requests (`window.open`, a
  script-made link click) never reach foob's own policy handler, because WebKit's popup blocker
  stops them first, so no automated scenario covers them (the former scripted scenario was removed
  for that reason; guided rows 5, 6 and 17 cover `window.open`). `blank-form-does-not-navigate`
  is the one scenario that reaches the handler: a trusted script submit of a `_blank` form does
  nothing, not what a real click does (guided rows 1, 3, 9, 15, 16 and 19, clicked by the
  maintainer after the D-04 update: all pass). The page reports `form-submitted` to the test server, so
  the scenario cannot pass without the probe having run (found in the Codex review). Since the D-04 update `decide_policy`
  ignores every new-window request; there is no gesture check and no remote-to-`file:` check on
  this path to test (`foob_navigation_allowed` is covered by its unit test only).
- Repeated on revision `147ae45`: `gcc -fanalyzer` with `-Wall -Wextra -Wconversion
  -Wshadow -Wformat=2 -Wnull-dereference` on every translation unit (exit 0, no
  warning); ASan+UBSan build (engine-owned leaks suppressed by library) passes the 3
  unit/CLI tests and all 12 nested scenarios, and the run with logs kept shows no
  `runtime error` and no sanitizer report in any scenario. Binary 22,520 B stripped
  (unchanged; text 13,061 B, 13,324 B at `89c1ecf`); a clean `git archive`
  build has no warnings and passes 3/3. Startup and exit latency results: [audit-m1.md](audit-m1.md).

## Not covered

Not repeated here: the original [desktop checklist](desktop-checklist.md) (the audit's own guided checks are done, see
[guided-checks-m1-audit.md](guided-checks-m1-audit.md)); multi-monitor and fractional
scaling (unavailable); Scholar and Semantic Scholar page loads; GPU participation
beyond the earlier amdgpu counter evidence. PSS does not account for all GPU memory
and the per-process snapshots are not atomic (see `measurements.md`).

## Reproduce

```sh
meson setup build && meson compile -C build
python3 tests/benchmark/run.py --binary build/foob --label blank-1 \
    --uri about:blank --output docs/validation/runs/blank-1.json
python3 tests/benchmark/run.py --binary /path/to/control --label control-blank-1 \
    --uri about:blank --output docs/validation/runs/control-blank-1.json
# repeat for the fixture and Wikipedia URIs, alternating the two binaries;
# use a clean runs directory and commit before measuring
```
