# M1 audit: re-recorded process-tree PSS evidence

**The audit binary shows no memory or timing regression against the unmodified
`m0-m1` build measured in the same session. The under-100-MiB stretch target is
still not met.** This file supplements [measurements.md](measurements.md); it does
not replace it. That file describes the original `m0-m1` binary, whose source and
binary hashes no longer match this branch.

Collected 2026-10-13 (UTC 12:16-12:21, run start times). WPE Arch `2.54.1-2`; GCC
`16.2.1+r23+gd564253eb6c8-1`; Meson 1.12.1 `debugoptimized`, `-std=c23 -O2 -g`
plus the hardening flags in [audit-m1.md](audit-m1.md); kernel `7.2.9-arch1-1`;
niri `26.04-1`; Mesa `1:26.2.4-1`. Machine-readable records:
[results-m1-audit.json](results-m1-audit.json); raw `runs/*.json` and `.log` files
stay ignored and local.

## Inputs

| | |
|---|---|
| Revision | `2dfe12d787cf845e27488551e76bf9a652f676b6` (`m1-audit`), tree clean (`source_dirty: false`) |
| Source SHA256 (`src/*.c`, `src/*.h`) | `1143cc652ada6f4999707ae9dfc14846d1c8a0d7307d241aeff6140ce163cdb4` |
| Audit binary SHA256 | `38d0d86cc90da2caa071e2cc9e0232474d2ead67933e3d63a7c013949ef2e2fb` |
| Control binary SHA256 | `3369eeadb352e812bfc7c3f7b1fd009f491ac663062fa3afccf3a424c9245bcb` |
| Fixtures | unchanged from `m0-m1`: `viewport.html` `c6487daeb9c8…`, `linked.html` `0cc8e2b52c19…`, `mathml.html` `bafc38fe176d…` |

The control is a build of the unmodified `m0-m1` source made earlier in this audit,
not the binary hashed in `measurements.md` (`9a9225d8…`); the hash differs because
it was rebuilt. Its records carry the audit tree's `source_sha256` because the
collector hashes the working tree; only `binary_sha256` identifies the control.
Source and binary hashes in `docs/validation/` for the audit binary are the values
above; any later C change invalidates them.

## Method

Same protocol as `measurements.md`: `tests/benchmark/suite.py` (three fresh
processes per workload plus one warm-reload control), explicit ephemeral session,
three samples one second apart starting at least ten seconds after the observed
successful load, process tree from `/proc` including reparented helpers, `Pss`
summed per PID from `smaps_rollup`. Every sample: **950x1188 logical viewport,
scale 1.0**, eight tracked processes, exit code 0, no survivors after ten seconds.
OS, filesystem and network caches are uncontrolled; Wikipedia is live content.

The control used the same collector (`run.py --binary`) in the same session:
three launches each for blank and the local fixture, two for Wikipedia (the
suite's network workload; a third was not needed to see the agreement).

## Results: audit binary

Values are the three samples per launch in MiB.

| Run | Sample 1 | Sample 2 | Sample 3 | Navigation ms |
|---|---:|---:|---:|---:|
| `blank-1` | 168.342 | 168.354 | 168.354 | 0.393 |
| `blank-2` | 168.334 | 168.346 | 168.346 | 0.350 |
| `blank-3` | 168.396 | 168.409 | 168.409 | 0.482 |
| `fixture-1` | 194.637 | 194.641 | 194.641 | 15.216 |
| `fixture-2` | 192.728 | 192.732 | 192.732 | 16.668 |
| `fixture-3` | 192.801 | 192.805 | 192.805 | 15.553 |
| `wikipedia-1` | 327.816 | 327.547 | 327.523 | 2636.714 |
| `wikipedia-2` | 330.365 | 328.744 | 328.732 | 537.421 |
| `wikipedia-3` | 329.122 | 327.485 | 327.474 | 512.552 |
| `fixture-warm-reload` | 195.711 | 195.712 | 195.712 | 5.868 reload; 17.369 initial |

## Results: audit versus control, same session

| Workload | Audit mean (MiB) | Audit range | Control mean (MiB) | Control range |
|---|---:|---|---:|---|
| Blank | 168.366 (9 samples) | 168.334-168.409 | 168.802 (9) | 168.471-168.985 |
| Local fixture | 193.391 (9) | 192.728-194.641 | 193.538 (9) | 192.873-194.854 |
| Wikipedia | 328.312 (9) | 327.474-330.365 | 328.045 (6) | 325.281-331.342 |

Blank, fixture and Wikipedia differ by 0.4, 0.15 and 0.3 MiB, inside the spread
within each build. Navigation times for blank and fixture agree (control 0.19-0.30
ms and 15.3-16.4 ms). Wikipedia navigation times depend on the network and ranged
0.5-2.6 s for both builds. Nine samples are repeated observations of three
launches, not nine independent launches.

## Why these numbers are lower than `measurements.md`

`measurements.md` reports blank 174.3, fixture 202.4 and Wikipedia 333.7 MiB. This
session's **control** (the old code) reads 168.8, 193.5 and 328.0 MiB, so the
~6-9 MiB drop is session drift (system state, caches, library mapping sharing),
not an improvement from this branch. Do not cite it as one. The earlier
[blank-memory diagnosis](blank-memory.md) saw the same 168-171 MiB range for the
original binary. Compare builds only within one session, as above.

## Other evidence recorded with this run

- `meson test -C build`: 3 of 3 pass (`target` and `window_title` unit tests,
  startup URI contract).
- `python3 tests/nested_lifecycle.py build/foob` in a disposable nested niri: 11 of
  11 scenarios pass, each with exact-PID survivor checks and an app-id assertion:
  `title-follows-page`, `title-remote-sanitized`, `download-cancelled`
  (300 MiB stream aborted after under 16 MiB, nothing on disk), `new-window-without-gesture-blocked`,
  `compositor-close`, `sigterm`, `sigint`, `compositor-lost` (exit 3),
  `web-process-crash` (exit 4), plus `policy-fixture-remote` and `policy-fixture-local` (the guided-check fixtures load and keep their expected window titles; added after the measurements, no source change). The pre-fix binary fails the compositor-loss,
  download, app-id and web-process scenarios (negative controls).
- Startup, exit latency, sanitizer and static-analysis results: [audit-m1.md](audit-m1.md).

## Not covered

Not repeated here: the guided keyboard checks in the
[desktop checklist](desktop-checklist.md); multi-monitor and fractional scaling
(unavailable); Scholar and Semantic Scholar page loads; GPU participation beyond
the earlier amdgpu counter evidence. PSS does not account for all GPU memory and
the per-process snapshots are not atomic (see `measurements.md`).

## Reproduce

```sh
meson setup build && meson compile -C build
python3 tests/benchmark/suite.py   # new docs/validation/runs/ files; use a clean runs directory
python3 tests/benchmark/run.py --binary /path/to/control --label control-blank-1 \
    --uri about:blank --output docs/validation/runs/control-blank-1.json
```
