# M1 audit: re-recorded process-tree PSS evidence

**The audit binary shows no memory or timing regression against the unmodified
`m0-m1` build, measured interleaved in the same session. The under-100-MiB stretch
target is still not met.** This file supplements [measurements.md](measurements.md);
it does not replace it. That file describes the original `m0-m1` binary, whose
source and binary hashes no longer match this branch.

Collected 2026-10-13 after the Codex fixes and both security reviews (F-30 to F-61, with the
same-window script removed again, D-21); run start times are in the raw records. WPE
Arch `2.54.1-2`; GCC `16.2.1+r23+gd564253eb6c8-1`; Meson 1.12.1 `debugoptimized`,
`-std=c23 -O2 -g` plus the hardening flags in [audit-m1.md](audit-m1.md); kernel
`7.2.9-arch1-1`; niri `26.04-1`; Mesa `1:26.2.4-1`. Machine-readable records:
[results-m1-audit.json](results-m1-audit.json); raw `runs/*.json` and `.log` files
stay ignored and local (`docs/validation/runs/m1-audit-0af4314/`).

## Inputs

| | |
|---|---|
| Revision | `0af4314ddbb85cc637222f459da9bb2acbdaacf0` (`m0-m1`, the D-21 commit), tree clean (`source_dirty: false` in every record) |
| Source SHA256 (`src/*.c`, `src/*.h`) | `498cc823f10d7fb5fbbbd6e5cf4b64121596ad12837593062df6d0b8469338e9` |
| Audit binary SHA256 | `6e650bf4c639189970805f45dabef613e1966328330765b520f35baef4bebb06` |
| Control binary SHA256 | `3369eeadb352e812bfc7c3f7b1fd009f491ac663062fa3afccf3a424c9245bcb` (unchanged) |
| Fixtures | `viewport.html` unchanged from `m0-m1`; `linked.html` gained a back link and the policy fixtures changed after the first audit measurement, none of them used by these workloads except `viewport.html` |

Earlier measurements of this branch (revisions `2dfe12d`, `9c81515`, `1fae570`,
`9abb6fe` and `89c1ecf`, source hashes `7b3129c2…` for `1fae570`, `0c7dc10f…` for `9abb6fe` and `d306ff47…` for `89c1ecf`) are
superseded: each change to C source (sanitizer, exit-status handling, the
`_FORTIFY_SOURCE` check, the new-window handler that loads the action's request, the
title sanitizer, the URI allowlist, and now the removal of the same-window script) invalidates the hashes. They showed the
same picture (audit and control within a few MiB). Later commits change only tests,
fixtures and documents.

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
previous run on `9abb6fe` had one failed fixture launch for both builds, explained in
F-36 of [findings.md](findings.md): the fixture reloaded itself, probably from input
to the new window.)

## Results: audit binary

Values are the three samples per launch in MiB.

| Run | Sample 1 | Sample 2 | Sample 3 | Navigation ms |
|---|---:|---:|---:|---:|
| `blank-1` | 170.473 | 170.479 | 170.450 | 0.945 |
| `blank-2` | 170.600 | 170.611 | 170.611 | 0.699 |
| `blank-3` | 170.507 | 170.518 | 170.518 | 0.862 |
| `blank-4` | 171.012 | 171.059 | 171.059 | 0.657 |
| `fixture-1` | 197.307 | 197.310 | 197.310 | 37.515 |
| `fixture-2` | 195.076 | 195.081 | 195.081 | 39.206 |
| `fixture-3` | 195.406 | 195.409 | 195.409 | 36.460 |
| `fixture-4` | 195.431 | 195.436 | 195.436 | 36.655 |
| `wikipedia-1` | 330.619 | 330.619 | 330.604 | 1679.067 |
| `wikipedia-2` | 329.993 | 329.993 | 329.971 | 1665.163 |
| `wikipedia-3` | 356.430 | 356.000 | 356.023 | 1703.266 |
| `wikipedia-4` | 328.493 | 328.493 | 328.470 | 1668.119 |
| `fixture-warm-reload` | 197.986 | 197.982 | 197.982 | 8.714 reload; 36.986 initial |

## Results: audit versus control, interleaved

| Workload | Audit mean (MiB) | Audit range | Control mean (MiB) | Control range |
|---|---:|---|---:|---|
| Blank | 170.658 (12 samples) | 170.450-171.059 | 170.705 (12) | 170.373-171.648 |
| Local fixture | 195.808 (12) | 195.076-197.310 | 196.160 (12) | 194.963-197.281 |
| Wikipedia | 336.309 (12) | 328.470-356.430 | 329.730 (12) | 328.407-331.052 |

Means differ by 0.05 MiB (audit lower), 0.35 MiB (audit lower) and 6.6 MiB (audit higher).
The Wikipedia gap comes from one launch: `wikipedia-3` read 356.0-356.4 MiB, 26 MiB above the
other three audit launches (328.5-330.6), which agree with the control's four (328.4-331.1).
Without it the audit mean is 329.7 MiB against 329.7 for the control. The same kind of
single launch spike has appeared before in both builds (see below), so it is not read as a
property of the binary, but it is one launch, not proof. Wikipedia navigation times: audit
1665-1703 ms against control 1450-1610 ms in these four pairs, the audit build slower in
all four. Five more interleaved Wikipedia pairs (order alternated, kept in
`runs/m1-audit-0af4314-nav/`) gave audit 1489-2565 ms (median 1611) against control
1547-1611 ms (median 1586) and PSS 328.1-333.1 against 327.1-330.9 MiB, with no consistent
sign (audit faster in two pairs). Over all nine pairs the audit median is 1668 ms against
1586 ms (about 5% slower), inside the spread of live network loads; this is not explained
and not claimed as noise or as a real cost. Blank (0.66-0.95 ms against 0.48-0.94 ms) and
fixture (36.5-39.2 ms against 36.6-38.1 ms) agree. Twelve samples are repeated observations
of four launches, not twelve independent launches.

Single-launch spikes occur in both builds and in every attempt: blank launches 5-12 MiB
away from the others (here `wikipedia-3` of the audit build)
and one Wikipedia launch 15-30 MiB high in earlier runs. They are session state, not a
property of either binary, and they are why launches are interleaved and repeated.

## Why these numbers differ from `measurements.md` and from the earlier runs

`measurements.md` reports blank 174.3, fixture 202.4 and Wikipedia 333.7 MiB. In the
session of the earlier run (`89c1ecf`) the **control** (the old code) read 166.2, 194.5 and 327.4 MiB; in this one it reads 170.7, 196.2 and 329.7 MiB, so the lower
numbers are session drift (system state, caches, library mapping sharing), not an
improvement from this branch. Do not cite it as one. Compare builds only within one
session, as above. The earlier [blank-memory diagnosis](blank-memory.md) saw the same 158-171 MiB
range for the original binary.

## Other evidence recorded with this run

- `meson test -C build`: 3 of 3 pass (`target` and `window_title` unit tests,
  startup URI contract). A clean `git archive` export builds without warnings.
- `python3 tests/nested_lifecycle.py build/foob` in a disposable nested niri: 12 of
  12 scenarios pass (re-run on `0af4314`), each with exact-PID survivor checks and an app-id assertion:
  `title-follows-page`, `title-remote-sanitized`, `download-cancelled` (300 MiB
  stream aborted after under 16 MiB, nothing on disk),
  `scripted-new-window-requests-do-not-navigate` (re-checked after a 3 s wait),
  `blank-form-does-not-navigate` (a trusted `_blank` form submit from the page and from a
  sandboxed frame reaches no `/echo`; the window stays on the page; the script build `f40e4b9`
  fails it),
  `policy-fixture-remote`, `policy-fixture-local`, `compositor-close`, `sigterm`,
  `sigint`, `compositor-lost` (exit 3), `web-process-crash` (exit 4). The pre-fix
  binary fails the compositor-loss, download, app-id and web-process scenarios; those
  negative controls were run during the audit and are recorded in prose, not as
  committed logs.
- **Limit of the new-window scenarios:** with default settings scripted requests never
  reach foob's own policy handler (WebKit's popup blocker stops them first), so the
  scenarios prove that scripted and no-gesture requests do not navigate and that a trusted
  script submit of a `_blank` form does nothing, not what a real click does (guided rows
  1, 3, 9, 15, 16 and 19, pending). The page reports its probe result
  (`click-ok,open-null`, `form-submitted`) to the test server, so a scenario cannot pass
  without the probes having run (Codex F-31). Since D-21 `decide_policy` ignores every
  new-window request; there is no gesture check and no remote-to-`file:` check on this
  path to test (`foob_navigation_allowed` is covered by its unit test only).
- Repeated on revision `0af4314`: `gcc -fanalyzer` with `-Wall -Wextra -Wconversion
  -Wshadow -Wformat=2 -Wnull-dereference` on every translation unit (exit 0, no
  warning); ASan+UBSan build (engine-owned leaks suppressed by library) passes the 3
  unit/CLI tests and all 12 nested scenarios, and the run with logs kept shows no
  `runtime error` and no sanitizer report in any scenario. Binary 22,520 B stripped
  (unchanged; text 13,107 B, 13,324 B at `89c1ecf`); a clean `git archive`
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
