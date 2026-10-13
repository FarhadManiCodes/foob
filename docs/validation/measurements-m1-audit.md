# M1 audit: re-recorded process-tree PSS evidence

**The audit binary shows no memory or timing regression against the unmodified
`m0-m1` build, measured interleaved in the same session. The under-100-MiB stretch
target is still not met.** This file supplements [measurements.md](measurements.md);
it does not replace it. That file describes the original `m0-m1` binary, whose
source and binary hashes no longer match this branch.

Collected 2026-10-13 (UTC 15:26-15:31, run start times) after both review rounds. WPE
Arch `2.54.1-2`; GCC `16.2.1+r23+gd564253eb6c8-1`; Meson 1.12.1 `debugoptimized`,
`-std=c23 -O2 -g` plus the hardening flags in [audit-m1.md](audit-m1.md); kernel
`7.2.9-arch1-1`; niri `26.04-1`; Mesa `1:26.2.4-1`. Machine-readable records:
[results-m1-audit.json](results-m1-audit.json); raw `runs/*.json` and `.log` files
stay ignored and local.

## Inputs

| | |
|---|---|
| Revision | `1fae5706b2bb528a0bc8bc30c1e637a1d06a8b19` (`m1-audit`), tree clean (`source_dirty: false` in every retained record) |
| Source SHA256 (`src/*.c`, `src/*.h`) | `7b3129c2152a918762bc25696c018ef932d2a7cd9ff80ca3af7b028272a565b5` |
| Audit binary SHA256 | `916f7b039f8e10fce6082e97f2fea1c2a540a8eb111649cd547f54449a2c8c4f` |
| Control binary SHA256 | `3369eeadb352e812bfc7c3f7b1fd009f491ac663062fa3afccf3a424c9245bcb` |
| Fixtures | unchanged from `m0-m1`: `viewport.html` `c6487daeb9c8…`, `linked.html` `0cc8e2b52c19…`, `mathml.html` `bafc38fe176d…` |

Two earlier measurements of this branch (revisions `2dfe12d`, binary `38d0d86c…`, and
`9c81515`, binary `53eda7b8…`) were superseded: each review round changed C source
(sanitizer, exit-status handling, the `_FORTIFY_SOURCE` check, comments), so their
hashes no longer describe the code. They showed the same picture as this run (audit
and control within about 1 MiB). Later commits change only tests, fixtures and
documents.

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
| `blank-1` | 168.427 | 168.439 | 168.439 | 0.280 |
| `blank-2` | 168.283 | 168.296 | 168.296 | 0.294 |
| `blank-3` | 170.455 | 170.469 | 170.469 | 0.227 |
| `blank-4` | 168.466 | 168.479 | 168.479 | 0.266 |
| `fixture-1` | 194.980 | 194.984 | 194.984 | 15.901 |
| `fixture-2` | 192.554 | 192.559 | 192.559 | 17.564 |
| `fixture-3` | 192.812 | 192.816 | 192.816 | 16.721 |
| `fixture-4` | 192.728 | 192.732 | 192.732 | 15.406 |
| `wikipedia-1` | 331.111 | 329.635 | 329.623 | 596.664 |
| `wikipedia-2` | 328.898 | 327.281 | 327.270 | 566.034 |
| `wikipedia-3` | 333.974 | 332.353 | 332.341 | 548.772 |
| `wikipedia-4` | 332.821 | 331.200 | 331.247 | 557.474 |
| `fixture-warm-reload` | 194.551 | 194.551 | 194.551 | 5.571 reload; 16.296 initial |

## Results: audit versus control, interleaved

| Workload | Audit mean (MiB) | Audit range | Control mean (MiB) | Control range |
|---|---:|---|---:|---|
| Blank | 168.917 (12 samples) | 168.283-170.469 | 169.024 (12) | 168.294-170.429 |
| Local fixture | 193.271 (12) | 192.554-194.984 | 192.844 (12) | 192.644-193.064 |
| Wikipedia | 330.646 (12) | 327.270-333.974 | 335.458 (12) | 325.876-358.340 |

Means differ by 0.1, 0.4 and 4.8 MiB (audit lower for blank and Wikipedia, higher by
0.4 for the fixture), inside the spread of each build: one control Wikipedia launch
read 356 MiB, which alone accounts for the Wikipedia gap (the other three control
launches average 328.5 MiB against 330.6 for the audit binary). Navigation times agree
(blank 0.23-0.29 ms against 0.30-0.47 ms; fixture 15.4-17.6 ms against 15.6-16.9 ms;
Wikipedia 549-597 ms against 508-601 ms). Twelve samples are repeated observations of
four launches, not twelve independent launches.

Single-launch spikes occur in both builds and in every attempt: blank launches about
2 MiB high (audit launch 3, control launch 1 here) and one Wikipedia launch 20-30 MiB
high (the audit binary in the first attempt, the control in this one). They are
session state, not a property of either binary, and they are why launches are
interleaved and repeated. The superseded attempts' raw files are retained locally
outside the repository.

## Why these numbers are lower than `measurements.md`

`measurements.md` reports blank 174.3, fixture 202.4 and Wikipedia 333.7 MiB. In this
session the **control** (the old code) reads 169.0, 192.8 and 335.5 MiB (328.5 for
Wikipedia without the one spike), so the ~5-9 MiB drop is session drift (system state, caches, library mapping sharing), not
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
