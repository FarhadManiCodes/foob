# M1 stock-engine process-tree PSS baseline

**Pass for M1-06 measurement protocol; under-100-MiB stretch target not met.**
Collected 2026-10-13 Europe/Berlin (raw timestamps use UTC). WPE Arch
`2.54.1-2`; GCC 16.2.1; Meson 1.12.1 `debugoptimized`, `-std=c23 -O2 -g`;
kernel `7.2.9-arch1-1`, niri `26.04-1`, Mesa `1:26.2.4-1`.

## Workloads and method

- Blank: `about:blank`.
- Local: repository `tests/fixtures/viewport.html`, loaded through `file:`.
- Wikipedia: `https://en.wikipedia.org/wiki/Mathematics`.
- Separate warm control: local fixture `?warm=1`, which performs exactly one
  same-session `location.reload()` three seconds after its initial script runs.

One browser window at a time; three independent fresh processes per workload.
Each uses an explicit ephemeral session; **OS/filesystem/network caches are
uncontrolled**, so “fresh process” does not mean a cold-cache start. Wikipedia
uses live networking and mutable remote content. No login/cookie imports or
custom user agent. No concurrent browser benchmark workload.

Every retained sample: **950×1188 WPE logical viewport, scale 1.0**. Local
fixture screenshot confirms CSS viewport 950×1188 and DPR 1. Physical output
1920×1200. Equal geometry is checked per run and across the suite. The final
collector also rejects any differing geometry diagnostic after final load
completion, so a transient resize cannot be hidden by matching sampled endpoints.
All accepted logs satisfy that stronger condition, including a replacement of
the original third fixture run.

For each launch, wait for successful `WEBKIT_LOAD_FINISHED`, then collect three
samples beginning at least ten seconds after observed completion, one second
apart. Polling can delay observation by up to about 50 ms. Sample timestamps
record actual elapsed time. Fail on subsequent load failure/crash/navigation
change/root exit. This is **navigation-completion timing, not first-visible
frame, process startup time or source-to-usable-page latency**.

Scan `/proc/*/stat` for descendants and session members of the separately
sessioned browser; this includes reparented bubblewrap helpers and D-Bus proxy.
Read `Pss` from each `/proc/PID/smaps_rollup` and sum KiB once per PID. Convert
to MiB by dividing by 1024. Eight measured processes per retained sample:
UI, network, web, four bubblewrap helpers and one xdg-dbus-proxy. Unreadable
PSS fails the run. Process identities use PID plus kernel start time for
survivor checks; every accepted run exits 0 with no tracked survivors.

Limitations: non-atomic per-process snapshots; short-lived helpers can disappear
between process scans (50–100 ms tracking); proportional shares of memory mapped
by unrelated processes can drift. PSS does not account for all GPU allocations.
No benchmark ranking between engines or optimization claim is made.

## Results

Each row is an independent launch; values are the three samples in MiB.
Elapsed times are engine navigation-completion durations in milliseconds.

| Run | Sample 1 | Sample 2 | Sample 3 | Navigation ms |
|---|---:|---:|---:|---:|
| [blank-1](runs/blank-1.json) | 174.262 | 174.274 | 174.274 | 0.941 |
| [blank-2](runs/blank-2.json) | 174.443 | 174.457 | 174.457 | 0.752 |
| [blank-3](runs/blank-3.json) | 174.224 | 174.236 | 174.236 | 0.975 |
| [fixture-1](runs/fixture-1.json) | 202.607 | 202.757 | 202.771 | 38.015 |
| [fixture-2](runs/fixture-2.json) | 199.457 | 199.431 | 199.442 | 37.364 |
| [fixture-3](runs/fixture-3.json) | 204.149 | 205.662 | 205.428 | 37.632 |
| [wikipedia-1](runs/wikipedia-1.json) | 334.151 | 333.577 | 333.556 | 1462.888 |
| [wikipedia-2](runs/wikipedia-2.json) | 335.004 | 333.957 | 333.937 | 1465.389 |
| [wikipedia-3](runs/wikipedia-3.json) | 333.509 | 332.853 | 332.829 | 1538.081 |
| [fixture warm reload](runs/fixture-warm-reload.json) | 202.500 | 202.497 | 202.497 | 8.308 reload; 39.709 initial |

| Fresh-process workload | Mean of 9 samples (MiB) | Observed range (MiB) |
|---|---:|---:|
| Blank | 174.318 | 174.224–174.457 |
| Local fixture | 202.412 | 199.431–205.662 |
| Wikipedia | 333.708 | 332.829–335.004 |

Nine samples are repeated observations across **three** independent launches,
not nine independent launches. Warm fixture is one separate launch with three
post-reload samples, mean 202.498 MiB. It is an exploratory warm control,
not evidence of a general warm-start speedup. Blank/Wikipedia warm reloads
are **not tested**; no reload control was added to the M1 browser for them.

## Inputs, revision and reproduction

Source SHA256: `6bf147f1390eef2881a8180f418236e5056373c336e081f71f20cb880995bce2`.
Measured binary SHA256: `9a9225d877defb9f6c574ea1929653fdbef5355ad8db5ab5dc99e4ee10283229`.
Most raw records contain M0 parent `ed52f71` and `source_dirty: true` because
M1 was measured before its evidence commit. Final replacement fixture-3 records
documentation parent `c44316e` with collector/evidence changes pending.
Exact source/binary hashes remove that ambiguity; no C source change occurred
after measurement or independent review.

Fixture SHA256:

| File | SHA256 |
|---|---|
| viewport.html | `c6487daeb9c858798aa83e44fef15e82e31629a728d8239ce04a7d4c699d02b9` |
| linked.html | `0cc8e2b52c19d629472db2c3954addcdc365c22f5c5d594dedc8f11b01d87d22` |
| mathml.html | `bafc38fe176dcc8475831c9c360e402994b42cf06fada5a5f7d921c80b76693f` |

```sh
meson setup build
meson compile -C build
python3 tests/benchmark/suite.py
```

JSON contains PID identities/per-process PSS/runtime confinement and graphics
metadata; paired `.log` files contain redacted application diagnostics and
engine warnings. Review logs alongside records. `runs/preliminary/` contains
one initial valid sample run and the failed negative-sleep run from the older
collector, both **excluded**. Full suite was rerun with continuous health checks.
Final log audit additionally excluded `fixture-3-transient` (950→1908→950
between completion and shutdown, despite matching sample endpoints). One
replacement was rejected for a second navigation and retained excluded as
`fixture-3-navigation`. The final fixture-3 replacement passes continuous
geometry/navigation checks; all other retained logs were audited with the same
geometry condition. This replacement uses identical source/binary/fixture hashes.
`crash-negative-control` and `missing-local-file` are expected failure controls,
not passing workloads or measurements.
