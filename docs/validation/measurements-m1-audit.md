# M1 audit: re-recorded process-tree PSS evidence

**The audit binary shows no memory or timing regression against the unmodified
`m0-m1` build, measured interleaved in the same session. The under-100-MiB stretch
target is still not met.** This file supplements [measurements.md](measurements.md);
it does not replace it. That file describes the original `m0-m1` binary, whose
source and binary hashes no longer match this branch.

Collected 2026-10-13 (UTC 14:38-14:43, run start times) after the review fixes. WPE
Arch `2.54.1-2`; GCC `16.2.1+r23+gd564253eb6c8-1`; Meson 1.12.1 `debugoptimized`,
`-std=c23 -O2 -g` plus the hardening flags in [audit-m1.md](audit-m1.md); kernel
`7.2.9-arch1-1`; niri `26.04-1`; Mesa `1:26.2.4-1`. Machine-readable records:
[results-m1-audit.json](results-m1-audit.json); raw `runs/*.json` and `.log` files
stay ignored and local.

## Inputs

| | |
|---|---|
| Revision | `9c8151513e4e3b79428f625e1b8a5202a5346c9a` (`m1-audit`), tree clean (`source_dirty: false` in every retained record) |
| Source SHA256 (`src/*.c`, `src/*.h`) | `d748f8e3706075c2c87ac142626322ad1e3ac7a693e00dcbaf0ad525de5302b6` |
| Audit binary SHA256 | `53eda7b8f682765bf861775ca6688817acf527ef3d673f6c8a4ff35047b79c34` |
| Control binary SHA256 | `3369eeadb352e812bfc7c3f7b1fd009f491ac663062fa3afccf3a424c9245bcb` |
| Fixtures | unchanged from `m0-m1`: `viewport.html` `c6487daeb9c8…`, `linked.html` `0cc8e2b52c19…`, `mathml.html` `bafc38fe176d…` |

The first measurement of this branch (revision `2dfe12d`, binary `38d0d86c…`) was
superseded: the review changed the sanitizer, exit-status handling and the
`_FORTIFY_SOURCE` logic, so those hashes no longer describe the code. Later commits
change only tests, fixtures and documents.

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
OS, filesystem and network caches are uncontrolled; Wikipedia is live content.

**Interleaved design:** four launches per workload for each binary, alternating audit
and control (audit blank, control blank, audit fixture, control fixture, audit
Wikipedia, control Wikipedia, repeated four times), plus one warm-reload control for
the audit binary, so that session drift hits both builds equally. This differs from
the original `suite.py` (three launches, one binary at a time).

## Results: audit binary

Values are the three samples per launch in MiB.

| Run | Sample 1 | Sample 2 | Sample 3 | Navigation ms |
|---|---:|---:|---:|---:|
| `blank-1` | 168.443 | 168.457 | 168.457 | 0.308 |
| `blank-2` | 170.931 | 170.944 | 170.944 | 0.224 |
| `blank-3` | 168.442 | 168.456 | 168.456 | 0.263 |
| `blank-4` | 168.322 | 168.335 | 168.335 | 0.315 |
| `fixture-1` | 192.703 | 192.708 | 192.708 | 15.589 |
| `fixture-2` | 192.838 | 192.843 | 192.843 | 16.911 |
| `fixture-3` | 192.943 | 192.947 | 192.947 | 17.349 |
| `fixture-4` | 192.798 | 192.802 | 192.802 | 17.222 |
| `wikipedia-1` | 328.153 | 326.407 | 326.396 | 501.804 |
| `wikipedia-2` | 330.245 | 328.624 | 328.612 | 552.136 |
| `wikipedia-3` | 328.488 | 326.867 | 326.855 | 488.419 |
| `wikipedia-4` | 328.109 | 326.488 | 326.477 | 611.932 |
| `fixture-warm-reload` | 194.957 | 194.958 | 195.063 | 4.490 reload; 16.012 initial |

## Results: audit versus control, interleaved

| Workload | Audit mean (MiB) | Audit range | Control mean (MiB) | Control range |
|---|---:|---|---:|---|
| Blank | 169.044 (12 samples) | 168.322-170.944 | 169.418 (12) | 168.160-170.794 |
| Local fixture | 192.823 (12) | 192.703-192.947 | 193.859 (12) | 192.873-194.955 |
| Wikipedia | 327.644 (12) | 326.396-330.245 | 328.165 (12) | 324.397-331.258 |

Means differ by 0.4, 1.0 and 0.5 MiB, inside the spread of each build, and the audit
binary is not the higher one in any workload. Navigation times agree (blank 0.22-0.32
ms against 0.28-0.41 ms; fixture 15.6-17.3 ms against 15.3-17.0 ms; Wikipedia
488-612 ms against 500-583 ms). Twelve samples are repeated observations of four
launches, not twelve independent launches.

Both builds show the same occasional launch about 2 MiB higher (the second blank
launch reads 170.9 for the audit binary and 170.8 for the control; the control's third
read 170.3), so that step is session state, not a property of either binary.

The first attempt at this re-measurement (three launches, audit then control, with
documentation files still being edited, so records flagged `source_dirty`) contained
single-launch outliers for the audit binary only: blank 170.2, fixture 196.6 and
Wikipedia 351.1 MiB. They did not reproduce in the interleaved run on the clean tree,
which is why the interleaved run is the evidence kept here. The earlier raw files are
retained locally outside the repository. Treat single-launch spikes as a known
property of this measurement, not as a result.

## Why these numbers are lower than `measurements.md`

`measurements.md` reports blank 174.3, fixture 202.4 and Wikipedia 333.7 MiB. In this
session the **control** (the old code) reads 169.4, 193.9 and 328.2 MiB, so the
~5-9 MiB drop is session drift (system state, caches, library mapping sharing), not
an improvement from this branch. Do not cite it as one. The earlier
[blank-memory diagnosis](blank-memory.md) saw the same 168-171 MiB range for the
original binary. Compare builds only within one session, as above.

## Other evidence recorded with this run

- `meson test -C build`: 3 of 3 pass (`target` and `window_title` unit tests,
  startup URI contract). A clean `git archive` export builds without warnings.
- `python3 tests/nested_lifecycle.py build/foob` in a disposable nested niri: 11 of
  11 scenarios pass, each with exact-PID survivor checks and an app-id assertion:
  `title-follows-page`, `title-remote-sanitized`, `download-cancelled` (300 MiB
  stream aborted after under 16 MiB, nothing on disk),
  `scripted-new-window-requests-do-not-navigate` (re-checked after a 3 s wait),
  `policy-fixture-remote`, `policy-fixture-local`, `compositor-close`, `sigterm`,
  `sigint`, `compositor-lost` (exit 3), `web-process-crash` (exit 4). The pre-fix
  binary fails the compositor-loss, download, app-id and web-process scenarios; those
  negative controls were run during the audit and are recorded in prose, not as
  committed logs.
- **Limit of the new-window scenarios:** scripted requests never reach foob's own
  policy handler (WebKit stops them first), so these scenarios prove that scripted
  and no-gesture requests do not navigate, not that foob's gesture check works.
  The remote-to-`file:` rule is covered only by the `foob_navigation_allowed` unit test.
  Removing that check changes nothing in them (checked with a scratch build); see
  [audit-m1.md](audit-m1.md) defect 5.
- Analyzer and ASan+UBSan (engine-owned leaks suppressed) were repeated on this code
  with no finding. Startup and exit latency results: [audit-m1.md](audit-m1.md).

## Not covered

Not repeated here: the guided keyboard checks in the
[desktop checklist](desktop-checklist.md) and
[guided-checks-m1-audit.md](guided-checks-m1-audit.md); multi-monitor and fractional
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
