# M1 code, integration and efficiency audit

2026-10-13. Stock WPE `2.54.1-2`, Mesa `26.2.4`, GCC 16 / C23. The audit fixed defects
in existing behaviour and recorded the baselines below; final measurements and hashes:
[measurements-m1-audit.md](measurements-m1-audit.md); decisions:
[findings.md](findings.md); maintainer checks:
[guided-checks-m1-audit.md](guided-checks-m1-audit.md).

## Defects found and fixed

Each fix has a test that fails on the unfixed build (negative control), except defects 3 and 8.

| # | Defect | Fix | Guarded by |
|---|---|---|---|
| 1 | Compositor loss left foob spinning at ~23% CPU on the dead socket, helpers alive | Handle `WPEDisplay::disconnected`, exit 3 at once (as MiniBrowser); helpers exit with their IPC peer | `tests/nested_lifecycle.py` (hangs on the old build) |
| 2 | The startup allowlist accepted raw spaces and control characters (GLib tolerates them) | Reject any byte <= 0x20, 0x7f and backslash (WebKit reads it as a slash) before parsing | `tests/test_target.c` |
| 3 | Page `window.close()` was ignored | Stop the loop, shared with the compositor close | **No test:** WebKit honours `window.close()` only for script-opened windows, which foob never has |
| 4 | Silent downloads: an attachment was saved into `~/Downloads` with no prompt | Cancel every download on `download-started` until M3-03 (D-02) | `download-cancelled`: a 300 MiB stream must abort under 16 MiB with no file left |
| 5 | `target=_blank` links did nothing, `window.open()` returned `null` | Kept, now decided: every new-window request is ignored (D-04, which records the two earlier attempts and why they were removed) | `blank-form-does-not-navigate`; real clicks: guided rows 1, 3, 9, 15, 16, 19 |
| 6 | A dead web process left a blank or frozen window | Exit 4 through the normal teardown (D-06); recovery is M3-07 | `web-process-crash` (SIGKILLs the exact `WPEWebProcess`) |
| 7 | Unstable app-id: niri saw `org.webkit.app-<SHA256 of the binary's path>`, so window rules broke when the binary moved | `g_set_prgname("dev.foob.Browser")` before the view exists (D-07); a name without a dot is ignored | Every nested scenario asserts the app-id |
| 8 | (Found in the pre-Gate-A code review, 2026-10-17.) SIGINT/SIGTERM were handled only once `foob_browser_run()` started, so a signal during display and engine start-up killed foob by signal instead of exiting 0 | The two signal sources are installed first in `foob_browser_new` and removed in `foob_browser_run()` or `foob_browser_free()`. About 40 ms of the window remain, in the dynamic loader before `main()`: SIGTERM within 5-40 ms of spawn still kills (16 of 39 tries, 21 before the fix) | **No deterministic test:** the window is a few milliseconds, so a timing probe is only a spot check |

The split into modules is described in `AGENTS.md`; source hashes recorded before the
split (a manifest of every `src/*.[ch]` now, `main.c` only before) cannot be compared with later ones.

## Static checks

- Warnings added in `meson.build` are clean; a separate pass with `-Wconversion
  -Wsign-conversion -Wpedantic -Wdouble-promotion -Wlogical-op` is also clean.
- `gcc -fanalyzer` on every translation unit: clean.
- ASan + UBSan: unit tests, CLI test and all nested scenarios pass with no runtime error.
  LeakSanitizer reports only engine-owned allocations (libdrm, libwayland-client, GLib
  `GVariant` calls from the engine); re-runs suppress them with `leak:libWPEWebKit-2.0.so`
  and `leak:libdrm.so`.
- clang-tidy (`bugprone`, `cert`, `clang-analyzer`, `concurrency`, `portability`),
  declined with reasons: `cert-err33-c` on unchecked `fputs` to stderr (a failed write
  cannot be handled usefully), `bugprone-signed-bitwise` on GLib flag-enum masks (GLib
  idiom), brace and "can be static" style checks (conflict with the project style or are
  false positives). The rest are inside GLib headers. Run it with correct include flags
  (an earlier run with broken ones reported nothing useful and was discarded).
- Hardening (`meson.build`, checked with `readelf`): PIE, full RELRO with `BIND_NOW`,
  non-executable stack, stack protector, stack-clash protection, CET markers,
  `_FORTIFY_SOURCE=3` (no effect visible, foob imports no `*_chk` function), zero
  initialization of locals (`-ftrivial-auto-var-init=zero`) and `-Werror=format-security`
  (both added after the audit; every section that is loaded is byte-identical, since foob
  initializes all its locals and no format string is non-literal; only the build ID and
  debug info differ).
- Cost of the hardening, measured against a copy of the tree without any hardening flag
  (and without PIE): stripped 22,520 B against 23,272 B, so no growth. Startup, process
  start to `load-finished`, 20 interleaved launches each: median 229.3 ms hardened against
  232.6 ms without, standard deviation 6 to 13 ms, so no difference above the noise. The
  engine is Arch's package and is hardened the same way (`BIND_NOW`, RELRO, stack protector,
  fortify imports).
- All of this was repeated on the evidence revision `147ae45` with the same result.

## Compiler flags

For foob's own code `-Os` saves ~5% of text (about 0.5 KB) and `-O3` adds 1%, with no
runtime effect, since every hot path is inside the engine. `-O2` stays. `-O3`, LTO, PGO
and `-march=native` wait for the custom engine (M5), where they can matter. Reading the
assembly becomes worthwhile for code foob owns on hot paths (M2+ converters, IPC parsing).

## Binary size baseline (M1, `147ae45`)

The next milestone explains growth over about 10% against these figures (`AGENTS.md`).

| | `m0-m1` before the audit | M1 final | MiniBrowser 2.54.1 |
|---|---:|---:|---:|
| File, stripped | 18,992 B | 22,520 B | 26,896 B |
| `size` text / data / bss | 8,632 / 1,000 / 16 | 13,061 / 1,160 / 16 | 20,933 / 2,792 / 304 |
| Direct libraries | glib, gobject, WPEWebKit, libc | same | adds gio and gstreamer |

Text per object: `diagnostics.c` 2,165, `browser.c` 2,061, `window_title.c` 1,402,
`target.c` 1,143, `main.c` 499 bytes. The growth since `m0-m1` is the audit's behaviour
changes (titles, download cancel, the two exits, app-id) plus the hardening flags; none of
it is dead code. Compare stripped files only; `-g` roughly triples the file.

## Memory levers tried

Where the ~170 MiB of a blank page goes: [blank-memory.md](blank-memory.md). Levers tried
on the stock engine (3 launches each):

| Lever | Blank | Wikipedia | Cost | Verdict |
|---|---|---|---|---|
| `JSC_useJIT=0` | -11 MiB | -32 MiB | 2.3x slower on a CPU-heavy page | Not adopted (D-05; M5-10) |
| `JSC_useFTLJIT=0` | ~0 | -11 MiB, within noise | ~5% slower | Not adopted |
| `MALLOC_ARENA_MAX=1` | 0 | - | none | No effect |
| Network-session memory pressure limit 150/300 MB | - | within noise | none | No effect |

A lever is adopted only if its gain exceeds run-to-run noise, it slows no docs, math or
scholarly page measurably and it weakens no security. Stock-engine tuning will not reach
100 MiB; that belongs to the custom engine.

Timing baseline (8 interleaved launches, earlier audit revision, not repeated since): process
start to `load-finished` median 107 ms, SIGTERM to exit 7 ms; Semantic Scholar 441 MiB.

## Defaults of unhandled WebKit signals (probed in a nested compositor)

The starting point for the tasks that will handle them.

| Probe | Observed | Owner |
|---|---|---|
| `alert()` / `confirm()` / `prompt()` | dismissed at once (`false` / `null`), page continues | M3-01 |
| `window.open()` | `null`, no window | M2-11, M3-01 (D-04) |
| Geolocation / Notification | denied | M3-05 |
| Expired TLS certificate | load refused | M3-05 |
| HTTP 401 Basic | page stays loading, no prompt, no title | M2-10 (D-03) |
| PDF URL | shown inline in PDF.js (guided row 12) | M3-04 (D-12) |
