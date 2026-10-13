# M1 audit: re-recorded process-tree PSS evidence

**The audit binary shows no memory or timing regression against the unmodified
`m0-m1` build, measured interleaved in the same session. The under-100-MiB stretch
target is still not met.** This file supplements [measurements.md](measurements.md);
it does not replace it. That file describes the original `m0-m1` binary, whose
source and binary hashes no longer match this branch.

Collected 2026-10-13 (UTC 18:53-18:59, run start times) after the Codex review
fixes (F-30, F-31) and the maintainer's guided checks. WPE
Arch `2.54.1-2`; GCC `16.2.1+r23+gd564253eb6c8-1`; Meson 1.12.1 `debugoptimized`,
`-std=c23 -O2 -g` plus the hardening flags in [audit-m1.md](audit-m1.md); kernel
`7.2.9-arch1-1`; niri `26.04-1`; Mesa `1:26.2.4-1`. Machine-readable records:
[results-m1-audit.json](results-m1-audit.json); raw `runs/*.json` and `.log` files
stay ignored and local (`docs/validation/runs/m1-audit-9abb6fe/`).

## Inputs

| | |
|---|---|
| Revision | `9abb6fea5b61ec4c71632a4468daef5fe255cc29` (`m1-audit`), tree clean (`source_dirty: false` in every record) |
| Source SHA256 (`src/*.c`, `src/*.h`) | `0c7dc10f07406262f9151b1f3783010bbe91e356e3a91ca91935fb2f12025c26` |
| Audit binary SHA256 | `b77d32718594ea39b75b525f519ac444a8d8e1ee1e52a4f699c2b67e462c5c1b` |
| Control binary SHA256 | `3369eeadb352e812bfc7c3f7b1fd009f491ac663062fa3afccf3a424c9245bcb` (unchanged) |
| Fixtures | `viewport.html` unchanged from `m0-m1`; `linked.html` gained a back link and the policy fixtures changed after the earlier measurement, none of them used by these workloads except `viewport.html` |

Earlier measurements of this branch (revisions `2dfe12d`, `9c81515` and `1fae570`,
binaries `38d0d86c…`, `53eda7b8…` and `916f7b03…`) are superseded: each change to C
source (sanitizer, exit-status handling, the `_FORTIFY_SOURCE` check, and now the
new-window handler that loads the action's request, F-30) invalidates the hashes. They showed
the same picture (audit and control within a few MiB). Later commits change only
tests, fixtures and documents.

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
`Pss` summed per PID from `smaps_rollup`. Every retained sample: **950x1188 logical
viewport, scale 1.0**, eight tracked processes, exit code 0, no survivors after ten
seconds. OS, filesystem and network caches are uncontrolled; Wikipedia is live content.

**Interleaved design:** four launches per workload for each binary, alternating audit
and control (audit blank, control blank, audit fixture, control fixture, audit
Wikipedia, control Wikipedia, repeated four times), plus one warm-reload run for the
audit binary, so that session drift hits both builds equally.

**Failed launch, repeated:** fixture launch 1 failed for both builds ("navigation
changed after requested completion; idle interval invalid"; both runs started within a
few seconds of each other, probably an interaction with the desktop during the run).
The failed records are kept in `results-m1-audit.json`; launch 5 repeats the fixture
workload for both builds, so each build has four valid fixture launches (2-5).

## Results: audit binary

Values are the three samples per launch in MiB.

| Run | Sample 1 | Sample 2 | Sample 3 | Navigation ms |
|---|---:|---:|---:|---:|
| `blank-1` | 166.951 | 166.958 | 166.958 | 0.960 |
| `blank-2` | 163.689 | 163.694 | 163.657 | 1.139 |
| `blank-3` | 163.747 | 163.768 | 163.771 | 0.891 |
| `blank-4` | 165.599 | 165.610 | 165.610 | 0.793 |
| `fixture-2` | 194.223 | 196.815 | 196.745 | 39.664 |
| `fixture-3` | 190.336 | 190.351 | 190.351 | 30.104 |
| `fixture-4` | 192.463 | 192.455 | 192.455 | 38.335 |
| `fixture-5` | 192.328 | 192.332 | 192.332 | 36.183 |
| `wikipedia-1` | 329.341 | 329.341 | 329.306 | 1277.169 |
| `wikipedia-2` | 324.903 | 324.903 | 324.868 | 1602.437 |
| `wikipedia-3` | 340.101 | 346.479 | 344.405 | 1575.421 |
| `wikipedia-4` | 325.140 | 325.140 | 325.118 | 1572.097 |
| `fixture-warm-reload` | 193.249 | 193.249 | 193.249 | 8.634 reload; 30.220 initial |

## Results: audit versus control, interleaved

| Workload | Audit mean (MiB) | Audit range | Control mean (MiB) | Control range |
|---|---:|---|---:|---|
| Blank | 165.001 (12 samples) | 163.657-166.958 | 165.732 (12) | 163.663-168.868 |
| Local fixture | 192.766 (12) | 190.336-196.815 | 191.386 (12) | 190.436-193.060 |
| Wikipedia | 330.754 (12) | 324.868-346.479 | 325.242 (12) | 323.197-326.297 |

Means differ by 0.7 MiB (audit lower), 1.4 MiB and 5.5 MiB (audit higher). Every gap is
inside the spread of the two builds' launches, and the sign flipped against the earlier
run (there the audit binary was lower for Wikipedia), which is what noise looks like.
The Wikipedia gap comes from one launch: audit launch 3 read 343.7 MiB on average while
the other three audit launches average 326.4 MiB against 325.2 for the control (+1.2).
Navigation times agree (blank 0.79-1.14 ms against 0.56-0.88 ms; fixture 30-40 ms
against 39-41 ms; Wikipedia 1277-1602 ms against 1376-1573 ms). Twelve samples are
repeated observations of four launches, not twelve independent launches.

Single-launch spikes occur in both builds and in every attempt: blank launches 2-5 MiB
high (control launch 4 here) and one Wikipedia launch 15-30 MiB high (the audit binary
in this run and in the first attempt, the control in the previous one). They are
session state, not a property of either binary, and they are why launches are
interleaved and repeated.

## Why these numbers differ from `measurements.md` and from the earlier run

`measurements.md` reports blank 174.3, fixture 202.4 and Wikipedia 333.7 MiB. In this
session the **control** (the old code) reads 165.7, 191.4 and 325.2 MiB, so the lower
numbers are session drift (system state, caches, library mapping sharing), not an
improvement from this branch. Do not cite it as one. Navigation times in this session
are 2-4 times longer than in the earlier run for **both** builds (control Wikipedia
1376-1573 ms against 508-601 ms), so the machine was busier; compare builds only
within one session, as above. The earlier [blank-memory diagnosis](blank-memory.md)
saw the same 163-171 MiB range for the original binary.

## Other evidence recorded with this run

- `meson test -C build`: 3 of 3 pass (`target` and `window_title` unit tests,
  startup URI contract). A clean `git archive` export builds without warnings.
- `python3 tests/nested_lifecycle.py build/foob` in a disposable nested niri: 11 of
  11 scenarios pass (re-run on `9abb6fe`), each with exact-PID survivor checks and an app-id assertion:
  `title-follows-page`, `title-remote-sanitized`, `download-cancelled` (300 MiB
  stream aborted after under 16 MiB, nothing on disk),
  `scripted-new-window-requests-do-not-navigate` (re-checked after a 3 s wait),
  `policy-fixture-remote`, `policy-fixture-local`, `compositor-close`, `sigterm`,
  `sigint`, `compositor-lost` (exit 3), `web-process-crash` (exit 4). The pre-fix
  binary fails the compositor-loss, download, app-id and web-process scenarios; those
  negative controls were run during the audit and are recorded in prose, not as
  committed logs.
- **Limit of the new-window scenarios:** with default settings scripted requests never
  reach foob's own policy handler (WebKit's popup blocker stops them first), so the
  scenario proves that scripted and no-gesture requests do not navigate, not that foob's
  gesture check works. The page reports its probe result (`click-ok,open-null`) to the
  test server, so the scenario cannot pass without the probes having run (Codex F-31).
  Scratch builds with the popup blocker off showed the requests reaching the handler
  with gesture=0 and the gesture check blocking them; also removing the check made the
  scenario fail (negative control, run once, not a committed log).
  The remote-to-`file:` rule is covered only by the `foob_navigation_allowed` unit test.
  Removing that check changes nothing in them (checked with a scratch build); see
  [audit-m1.md](audit-m1.md) defect 5.
- Repeated on revision `9abb6fe`: `gcc -fanalyzer` with `-Wall -Wextra -Wconversion
  -Wshadow -Wformat=2 -Wnull-dereference` on every translation unit (exit 0, no
  warning); ASan+UBSan build (engine-owned leaks suppressed by library) passes the 3
  unit/CLI tests and all 11 nested scenarios, and a second run with logs kept shows no
  `runtime error` and no sanitizer report in any scenario. Startup and exit latency results: [audit-m1.md](audit-m1.md).

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
