# M1 process-tree PSS measurements

**The audit binary shows no memory or timing regression against the unmodified `m0-m1`
build, measured interleaved in the same session. The under-100-MiB stretch target is still
not met.** This is the M1 measurement record (roadmap M1-06). The first baseline, taken
before the audit on binary `9a9225d8…`, read blank 174.3, local fixture 202.4 and Wikipedia
333.7 MiB; it is superseded, see [below](#why-these-numbers-differ-from-the-first-baseline)
(compact results in [results.json](results.json)).

Collected 2026-10-14, 07:24-07:29 UTC, on WPE `2.54.1-2`, GCC `16.2.1`, Meson `1.12.1`
`debugoptimized` with the hardening flags of `meson.build`; environment in [m1.md](m1.md).
Every run's values: [results-m1-audit.json](results-m1-audit.json); raw records stay local
(`docs/validation/runs/m1-audit-147ae45/`).

## Inputs

| | |
|---|---|
| Revision | `147ae452b5746ae02a3a47279829d6d19a24f53c`, tree clean in every record |
| Source SHA256 (`src/*.c`, `src/*.h`) | `a7c70db4b04272bb7d8eb4710c09a3b435fe3247455054c32f2d7560f978943f` |
| Audit binary SHA256 | `f1c00899786e6581768dc3d8ca60d062dfab07d7a17f5c388ba8254f1382a974` |
| Control binary SHA256 | `3369eeadb352e812bfc7c3f7b1fd009f491ac663062fa3afccf3a424c9245bcb` |
| Fixture | `tests/fixtures/viewport.html`, unchanged from `m0-m1` |

The control is a rebuild of the unmodified `m0-m1` source, so its hash differs from the first
baseline's. Its records carry the audit tree's `source_sha256` (the collector hashes the
working tree); only `binary_sha256` identifies it. The binary hash also depends on the build
directory (debug info embeds paths), so the source hash is the stable identifier. Since
`147ae45` only tests, documents and two comments changed: the binary is byte-identical, the
source hash is now `ed80ae36…` (handoff).

## Method

The `tests/benchmark/run.py` collector. Workloads: `about:blank`,
`tests/fixtures/viewport.html` through `file:`, and
`https://en.wikipedia.org/wiki/Mathematics` (live content); one window at a time, explicit
ephemeral session, no login or cookie import. After the observed successful load, three
samples one second apart starting at least ten seconds later; this is navigation-completion
timing, not first frame or startup latency. The process tree comes from `/proc`, including
reparented bubblewrap helpers and the D-Bus proxy (eight processes: UI, network, web, four
helpers, one proxy), with `Pss` summed once per PID from `smaps_rollup`; an unreadable value
fails the run. Every sample: **950x1188 logical viewport, scale 1.0**, and any geometry
change after the load fails the run. Every run exits 0 with no survivor (PID plus start
time) after ten seconds; a load failure, crash, extra navigation or early exit fails it.
OS, filesystem and network caches are uncontrolled, so a fresh process is not a cold start.
The maintainer was not using the desktop.

**Interleaved:** four launches per workload for each binary, alternating audit and control
(blank, fixture, Wikipedia, four rounds), plus one warm-reload run, so that session drift hits
both builds equally. All 25 runs passed.

## Results

| Workload | Audit mean (MiB) | Audit range | Control mean (MiB) | Control range |
|---|---:|---|---:|---|
| Blank | 170.276 | 169.766-171.708 | 169.966 | 169.536-170.491 |
| Local fixture | 194.046 | 193.742-194.230 | 194.711 | 194.010-196.415 |
| Wikipedia | 329.752 | 326.891-332.896 | 336.758 | 328.775-357.646 |

Each mean is 12 samples from four launches. The differences (0.31 MiB higher, 0.67 and 7.0
lower) are inside the spread of each build; the Wikipedia gap comes from one control launch
at 357 MiB (without it, 329.8 against 330.4). Such single-launch spikes appear in both builds
in every session, which is why launches are interleaved and repeated. Navigation times: blank
0.21-0.36 ms against 0.24-0.26, fixture 16.7-17.7 against 15.4-15.9, Wikipedia 530-545
against 485-541 ms, except the session's first audit Wikipedia launch (1136 ms, same memory).
Warm reload of the fixture: 198.2 MiB, reload 4.9 ms.

## Re-check after the signal change (`a4b8e38`, 2026-10-17)

`foob_browser_new` now installs the SIGINT and SIGTERM sources before the display and engine
start (`src/browser.c`). The audit binary (`f1c00899…`) and the new one (`ab59b029…`, source
SHA256 `5f6b8159…`, tree clean) were measured alternating, four launches per workload and
build, same method, all 24 runs passed (raw records: `runs/m1-signal-a4b8e38/`, local):

| Workload | Audit binary (MiB) | New binary (MiB) |
|---|---|---|
| Blank | 189.3 (187.3-191.0) | 189.9 (187.0-192.5) |
| Local fixture | 213.5 (213.3-213.8) | 215.5 (215.4-215.6) |
| Wikipedia | 349.8 (347.2-350.9) | 351.2 (349.4-353.7) |

Same packages as in October 14; the whole session reads about 19 MiB higher than that one
(blank 189 against 170), for both builds, so compare only within the session. The new build
is 0.6 (blank), 2.0 (fixture, ranges do not overlap) and 1.4 MiB (Wikipedia) higher: at most
0.9%, and no code path touched by the change runs on a page. Not explained further.

## Why these numbers differ from the first baseline

The first baseline (174.3, 202.4, 333.7 MiB) is higher because of session drift (system
state, caches, shared library mappings), not because of the audit: the same unmodified control
read 166.2, 194.5 and 327.4 MiB in one earlier session and 169.8, 194.8 and 329.4 in this one.
Do not cite the difference as an improvement; compare builds only within one session. The
[blank-memory diagnosis](blank-memory.md) saw 168-171 MiB for the original binary.

## Not covered

Scholar and Semantic Scholar loads, multi-monitor and fractional scaling. PSS does not count
all GPU memory ([blank-memory.md](blank-memory.md)), and the per-process reads are not atomic:
short-lived helpers can disappear between scans, and shares of memory mapped by unrelated
processes can drift.

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
