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
stay ignored and local (`docs/validation/runs/m1-audit-679fb9a/`).

## Inputs

| | |
|---|---|
| Revision | `679fb9ad…` (full hash in every record; `m0-m1`, after the final review fixes F-66 to F-68), tree clean (`source_dirty: false` in every record) |
| Source SHA256 (`src/*.c`, `src/*.h`) | `667746fc73624bf710b9d039dfca8c54ae964f10f12951f22fa7b2aa4ee40158` |
| Audit binary SHA256 | `e0b89b8f40589f3c58a182576060fe336c342bedbc3311b20d789a9530777459` |
| Control binary SHA256 | `3369eeadb352e812bfc7c3f7b1fd009f491ac663062fa3afccf3a424c9245bcb` (unchanged) |
| Fixtures | `viewport.html` unchanged from `m0-m1`; `linked.html` gained a back link and the policy fixtures changed after the first audit measurement, none of them used by these workloads except `viewport.html` |

Earlier measurements of this branch (revisions `2dfe12d`, `9c81515`, `1fae570`,
`9abb6fe`, `89c1ecf` and `0af4314`, source hashes `7b3129c2…` for `1fae570`, `0c7dc10f…` for `9abb6fe`, `d306ff47…` for `89c1ecf` and `498cc823…` for `0af4314`) are
superseded: each change to C source (sanitizer, exit-status handling, the
`_FORTIFY_SOURCE` check, the new-window handler that loads the action's request, the
title sanitizer, the URI allowlist, the removal of the same-window script, and now the trimmed cut title) invalidates the hashes. They showed the
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
| `blank-1` | 171.858 | 171.872 | 171.872 | 0.319 |
| `blank-2` | 169.710 | 169.724 | 169.724 | 0.247 |
| `blank-3` | 170.001 | 170.015 | 170.015 | 0.574 |
| `blank-4` | 169.780 | 169.794 | 169.794 | 0.427 |
| `fixture-1` | 196.229 | 196.233 | 196.233 | 16.059 |
| `fixture-2` | 194.102 | 194.105 | 194.105 | 16.085 |
| `fixture-3` | 194.267 | 194.271 | 194.271 | 15.586 |
| `fixture-4` | 194.122 | 194.725 | 194.725 | 15.988 |
| `wikipedia-1` | 332.481 | 330.860 | 330.849 | 493.384 |
| `wikipedia-2` | 331.474 | 329.853 | 329.841 | 525.160 |
| `wikipedia-3` | 328.751 | 327.130 | 327.118 | 518.733 |
| `wikipedia-4` | 335.229 | 333.608 | 333.597 | 535.914 |
| `fixture-warm-reload` | 196.535 | 196.540 | 196.540 | 5.246 reload; 16.744 initial |

## Results: audit versus control, interleaved

| Workload | Audit mean (MiB) | Audit range | Control mean (MiB) | Control range |
|---|---:|---|---:|---|
| Blank | 170.347 (12 samples) | 169.710-171.872 | 169.796 (12) | 169.697-169.904 |
| Local fixture | 194.782 (12) | 194.102-196.233 | 194.841 (12) | 194.028-196.580 |
| Wikipedia | 330.899 (12) | 327.118-335.229 | 329.412 (12) | 325.886-332.880 |

Means differ by 0.55 MiB (audit higher), 0.06 MiB (audit lower) and 1.5 MiB (audit higher),
all inside the spread of the launches of each build. Navigation times agree: blank 0.25-0.57 ms
against 0.26-0.41 ms, fixture 15.6-16.1 against 15.6-17.5, Wikipedia 493-536 against 517-544 ms.
This session was about three times faster than the previous one for **both** builds, and the
audit build's higher Wikipedia navigation time seen in `89c1ecf` and `0af4314` (F-62) did not
appear: it was most likely the busy sessions and live network loads, not the binary, though the
earlier sessions cannot be re-run to prove that. Twelve samples are repeated observations of
four launches, not twelve independent launches.

Single-launch spikes occur in both builds and in every attempt: blank launches 5-12 MiB
away from the others (here none; the `wikipedia-3` audit launch at 356 MiB in the previous session, `0af4314`, was one)
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
  12 scenarios pass (re-run on `679fb9a`), each with exact-PID survivor checks and an app-id assertion:
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
- Repeated on revision `679fb9a`: `gcc -fanalyzer` with `-Wall -Wextra -Wconversion
  -Wshadow -Wformat=2 -Wnull-dereference` on every translation unit (exit 0, no
  warning); ASan+UBSan build (engine-owned leaks suppressed by library) passes the 3
  unit/CLI tests and all 12 nested scenarios, and the run with logs kept shows no
  `runtime error` and no sanitizer report in any scenario. Binary 22,520 B stripped
  (unchanged; text 13,125 B, 13,324 B at `89c1ecf`); a clean `git archive`
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
