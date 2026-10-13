# M1 audit: re-recorded process-tree PSS evidence

**The audit binary shows no memory or timing regression against the unmodified
`m0-m1` build, measured interleaved in the same session. The under-100-MiB stretch
target is still not met.** This file supplements [measurements.md](measurements.md);
it does not replace it. That file describes the original `m0-m1` binary, whose
source and binary hashes no longer match this branch.

Collected 2026-10-13 (UTC 19:40-19:45, run start times) after the Codex fixes and the
security-review fixes (F-30 to F-50). WPE
Arch `2.54.1-2`; GCC `16.2.1+r23+gd564253eb6c8-1`; Meson 1.12.1 `debugoptimized`,
`-std=c23 -O2 -g` plus the hardening flags in [audit-m1.md](audit-m1.md); kernel
`7.2.9-arch1-1`; niri `26.04-1`; Mesa `1:26.2.4-1`. Machine-readable records:
[results-m1-audit.json](results-m1-audit.json); raw `runs/*.json` and `.log` files
stay ignored and local (`docs/validation/runs/m1-audit-89c1ecf/`).

## Inputs

| | |
|---|---|
| Revision | `89c1ecf280555a00bb3f8f8462a9f1f5cf5174bc` (`m0-m1`, after the audit merge), tree clean (`source_dirty: false` in every record) |
| Source SHA256 (`src/*.c`, `src/*.h`) | `d306ff47eaa616f7fc6ce2652ea443373de5400a6c23e67efd481111775a82e2` |
| Audit binary SHA256 | `dc22e2c285e0b07d65aec28091147836d30a221c06f9af647c0fdb853309d4c4` |
| Control binary SHA256 | `3369eeadb352e812bfc7c3f7b1fd009f491ac663062fa3afccf3a424c9245bcb` (unchanged) |
| Fixtures | `viewport.html` unchanged from `m0-m1`; `linked.html` gained a back link and the policy fixtures changed after the first audit measurement, none of them used by these workloads except `viewport.html` |

Earlier measurements of this branch (revisions `2dfe12d`, `9c81515`, `1fae570` and
`9abb6fe`, source hashes `7b3129c2…` for `1fae570` and `0c7dc10f…` for `9abb6fe`) are
superseded: each change to C source (sanitizer, exit-status handling, the
`_FORTIFY_SOURCE` check, the new-window handler that loads the action's request, and
now the title sanitizer and the URI allowlist) invalidates the hashes. They showed the
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
| `blank-1` | 166.966 | 166.972 | 166.972 | 0.891 |
| `blank-2` | 161.685 | 161.660 | 161.660 | 0.999 |
| `blank-3` | 170.068 | 170.892 | 170.892 | 0.598 |
| `blank-4` | 169.928 | 169.934 | 169.938 | 0.709 |
| `fixture-1` | 194.113 | 194.117 | 194.117 | 37.576 |
| `fixture-2` | 198.275 | 198.278 | 198.274 | 33.532 |
| `fixture-3` | 196.283 | 196.292 | 196.288 | 30.550 |
| `fixture-4` | 195.415 | 195.419 | 195.419 | 38.996 |
| `wikipedia-1` | 328.263 | 324.480 | 324.204 | 1667.048 |
| `wikipedia-2` | 326.600 | 326.588 | 326.567 | 1614.742 |
| `wikipedia-3` | 324.659 | 324.659 | 324.495 | 3022.855 |
| `wikipedia-4` | 327.875 | 327.902 | 327.887 | 1424.828 |
| `fixture-warm-reload` | 198.544 | 198.543 | 198.543 | 8.237 reload; 37.647 initial |

## Results: audit versus control, interleaved

| Workload | Audit mean (MiB) | Audit range | Control mean (MiB) | Control range |
|---|---:|---|---:|---|
| Blank | 167.297 (12 samples) | 161.660-170.892 | 166.179 (12) | 157.967-170.724 |
| Local fixture | 196.024 (12) | 194.113-198.278 | 194.500 (12) | 193.854-196.584 |
| Wikipedia | 326.182 (12) | 324.204-328.263 | 327.393 (12) | 318.771-332.035 |

Means differ by 1.1 MiB, 1.5 MiB (audit higher) and 1.2 MiB (audit lower). Every gap is
inside the spread of the two builds' launches (blank launches alone span 158-171 MiB in
the control), and the sign has changed between sessions (audit lower on blank in the
earlier run), which is what noise looks like. Navigation times agree (blank 0.60-1.00 ms
against 0.68-0.96 ms; fixture 31-39 ms against 37-39 ms; Wikipedia 1425-3023 ms against
1141-1573 ms; the 3023 ms audit launch is a single slow load, 2x the others, with the
same memory as the rest). Twelve samples are repeated observations of four launches, not
twelve independent launches.

Single-launch spikes occur in both builds and in every attempt: blank launches 5-12 MiB
away from the others (audit launch 2 low and launch 3 high; control launch 2 low here)
and one Wikipedia launch 15-30 MiB high in earlier runs. They are session state, not a
property of either binary, and they are why launches are interleaved and repeated.

## Why these numbers differ from `measurements.md` and from the earlier runs

`measurements.md` reports blank 174.3, fixture 202.4 and Wikipedia 333.7 MiB. In this
session the **control** (the old code) reads 166.2, 194.5 and 327.4 MiB, so the lower
numbers are session drift (system state, caches, library mapping sharing), not an
improvement from this branch. Do not cite it as one. Navigation times in this session
(and the previous one) are 2-4 times longer than in the first audit run for **both**
builds, so the machine was busier or on battery; compare builds only within one session,
as above. The earlier [blank-memory diagnosis](blank-memory.md) saw the same 158-171 MiB
range for the original binary.

## Other evidence recorded with this run

- `meson test -C build`: 3 of 3 pass (`target` and `window_title` unit tests,
  startup URI contract). A clean `git archive` export builds without warnings.
- `python3 tests/nested_lifecycle.py build/foob` in a disposable nested niri: 11 of
  11 scenarios pass (re-run on `89c1ecf`), each with exact-PID survivor checks and an app-id assertion:
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
- Repeated on revision `89c1ecf`: `gcc -fanalyzer` with `-Wall -Wextra -Wconversion
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
