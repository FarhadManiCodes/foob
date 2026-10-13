# M1 code, integration and efficiency audit

2026-10-13, branch `m1-audit` (cut from `m0-m1` at `4fb9c9c`). Gate A is still
the maintainer's decision; no M2 feature was added. This audit changes source
layout and fixes defects only, so the M1 hashes in `measurements.md` describe
the unmodified `m0-m1` binary, not this branch.

Method: the `foob-milestone-audit` skill (kept local, git-ignored). Everything
below was measured with stock WPE `2.54.1-2`, Mesa `26.2.4`, GCC 16 / C23.

## Defects found and fixed

| # | Finding | Evidence | Fix |
|---|---|---|---|
| 1 | **Compositor loss leaves the browser spinning.** If the compositor exits or crashes, `foob` keeps running at ~23% CPU on the dead socket with its helper processes alive. | Nested niri killed under the baseline binary: still alive 5 s later, state `R`, helpers present. `tests/nested_lifecycle.py` hangs on the baseline and passes on the fix. | Handle `WPEDisplay::disconnected`, exit immediately with new code 3 (`FOOB_EXIT_DISPLAY_LOST`), as upstream MiniBrowser does. Helpers exit when their IPC peer disappears (verified: no survivors). |
| 2 | Startup allowlist accepted `https://example.org:99999/` and URIs with raw spaces/control characters; GLib's parser tolerates both. | New C unit test `tests/test_target.c`, failed on first run. | Reject ports above 65535 and any byte <= 0x20 or 0x7f before parsing. |
| 3 | `WebKitWebView::close` (page `window.close()`) was ignored. | Upstream handles it; single view means quit. | Connect `close` and stop the loop. Not exercised by a test: WebKit only honours `window.close()` for script-opened windows. |

## Design changes (behaviour unchanged)

`src/main.c` (one 200-line file) became `main.c` (CLI), `target.[ch]` (pure GLib
allowlist, unit-tested without a display), `diagnostics.[ch]` (all redacted
logging, null-safe when disabled, one geometry scheduler instead of two copies),
`browser.[ch]` (lifecycle), `exit_status.h`. Diagnostics output is
byte-identical to the baseline on SIGTERM and SIGINT. `.text` grew by ~100 bytes.
`window_title.[ch]` (pure GLib) maps the page title to the compositor-visible
window title, `<page title> — foob`, or plain `foob` without a usable title.
`foob_uri_is_local()` in `target.c` is the single trust decision: `about:blank`
and `file:` URIs without a remote host are local and trusted; every other origin
is untrusted. Untrusted titles are repaired to valid UTF-8, lose control
characters and Unicode bidi overrides (which could visually reorder the
trailing `foob`), have whitespace collapsed and are cut at 120 characters;
trusted ones keep their text apart from protocol needs (valid UTF-8, no control
characters, 240 characters). Titles are never logged. Verified end to end in
the nested compositor: a hostile loopback page title
(`  Pay<RLO>evil   <LRI> page<BEL>  `) reaches niri as `Payevil page — foob`.
Later local document schemes extend `foob_uri_is_local()` only.

The browser no longer keeps its own display/session references; the view owns
them (as upstream). Benchmark collectors now record a manifest of every
`src/*.[ch]` digest instead of only `main.c`; hashes are not comparable with
pre-split records.

## Standards and static checks

- C23 idioms: `nullptr`, `[[maybe_unused]]`, `bool` keyword; no `NULL`/`(void)x`.
- Added warnings: `-Wshadow -Wformat=2 -Wnull-dereference -Wstrict-prototypes
  -Wmissing-prototypes -Wundef -Wcast-align -Wwrite-strings`. Clean. A separate
  pass with `-Wconversion -Wsign-conversion -Wpedantic -Wdouble-promotion
  -Wlogical-op` is also clean.
- `gcc -fanalyzer` on every translation unit: clean.
- ASan + UBSan build: unit test, CLI test and nested lifecycle all pass; no
  `runtime error`. LeakSanitizer reports 1.8 KB in 28 allocations, every one
  allocated inside libdrm, libwayland-client or GLib `GVariant` calls made by the
  engine during display/view setup, none by our own allocations.
- clang-tidy bug-finding set (`bugprone`, `cert`, `clang-analyzer`,
  `concurrency`, `portability`), run with correct include flags: `cert-err33-c`
  for unchecked `fputs` on the usage/error path (a failed stderr write cannot be
  handled usefully; not changed); `bugprone-signed-bitwise` on GLib flag-enum
  masks in `diagnostics.c` (GLib idiom; declined) and on `1 << 20` in tests
  (fixed); remaining hits are inside GLib's own headers or GLib assert macros.
  Style checks (braces, "can be static") were declined: they conflict with the
  project's existing style or are single-translation-unit false positives.
  (An earlier pass in this audit used broken include flags under zsh and was
  discarded; this is the valid run.)
- Hardening (readelf): PIE, full RELRO + `BIND_NOW`, non-exec stack, stack
  protector, stack-clash protection, CET (IBT/SHSTK) markers,
  `_FORTIFY_SOURCE=3` at `-O2`. Before: PIE, partial RELRO (no `BIND_NOW`) and
  the distro-default stack protector, no explicit flags. No measurable
  cost: blank 168.8 vs 168.9 MiB, load times equal.

## Compiler flags

For our ~9.5 KB of code: `-Os` -5% text, `-O3` +1%, nothing measurable at
runtime because every hot path is inside the engine. `-O2` stays the baseline.
`-O3`, LTO, PGO and `-march=native` are deferred to the end and to the
custom-engine work, where they can matter; they cannot reduce the stock engine.
Reading assembly was not worthwhile for this code (runs once at startup); it
becomes relevant for code we own on hot paths in M2+ (converters, IPC parsing).

## Memory: what is ours and what is not

Blank-page PSS ~169-174 MiB: web process ~107-114, UI process ~33, network ~27,
helpers ~1.6. Our own code is ~9 KB. The UI process (33 MiB) is: ~12.7 MiB
`libWPEWebKit` mappings (only ~3.6 MiB private dirty relocation data; the rest is
clean file pages shared with the other processes; the engine library already
uses `RELR` and `BIND_NOW`), ~11 MiB WebKit allocator heap, 2 MiB JIT region
(JavaScriptCore starts in the UI process too), ~7 MiB other libraries. 113
libraries are mapped because of the engine's link dependencies.

Levers tried on the stock engine (3 launches each):

| Lever | Blank | Wikipedia | Cost | Verdict |
|---|---|---|---|---|
| `JSC_useJIT=0` | -11 MiB | -32 MiB (340→308) | 2.3x slower on a CPU-heavy page (162 vs 70 ms) | Not adopted. Possible opt-in "untrusted site" mode (also removes the RWX JIT region). |
| `JSC_useFTLJIT=0` | ~0 | -11 MiB, within noise | ~5% slower | Not adopted. |
| `MALLOC_ARENA_MAX=1` | 0 | - | none | No effect. |
| `webkit_network_session_set_memory_pressure_settings` limit 150/300 MB (kill threshold off) | - | 324.6 vs 326.8 / 325.5 vs 324.2 | none | No effect. |

Rule for future levers: adopt only if the gain exceeds run-to-run noise, there
is no measurable slowdown on docs/math/scholarly pages, and no security is
weakened. Stock-engine tuning will not close the gap to 100 MiB; that decision
belongs to the custom-engine milestone.

Regression pairing (new vs `m0-m1` binary, same tooling): blank 168.8 vs 168.9,
local fixture 193.1 vs 193.3, Wikipedia 332.7 vs 332.9, Semantic Scholar 441.2
vs 442.5 MiB; all loads `status=ok` with equal timings. Latency, 8 interleaved
launches each: process start to `load-finished` median 107 ms (new) vs 108 ms
(baseline); SIGTERM to exit 7 ms vs 8 ms. (Google Scholar was not
loaded in this audit.)

## Relationship with WebKit/WPE, against upstream MiniBrowser

| Aspect | foob | Upstream | Verdict |
|---|---|---|---|
| Display | `wpe_display_wayland_new()` + connect, passed as `"display"` | default display; headless passes `"display"` explicitly | Valid, deliberate (deterministic native Wayland). |
| Session | `webkit_network_session_new_ephemeral()` | same for private mode | Matches. |
| Web context | implicit default | created only for time-zone/automation | Fine. |
| TLS / sandbox | defaults, never weakened | opt-in `--ignore-tls-errors` only | Matches. |
| Signals / loop | `g_unix_signal_add` INT/TERM + `GMainLoop` | same sources, `GApplication` | Equivalent; no D-Bus app registration needed. |
| Display loss | now handled (was missing) | `_exit(1)` | Fixed. |
| Page close | now handled (was missing) | quit when last view closes | Fixed. |
| Ownership | view owns display/session | releases own refs after view creation | Now matches. |
| Title | fixed `"foob"` | follows `notify::title` | **Decision needed** (below). |
| Web process crash | logged only | not handled in main either | **Decision needed.** |
| Settings / features | WebKit defaults | explicit feature flags for developer use | Defaults are the recommendation for a browser; revisit with the M2 policy. |
| Memory pressure | not set | not set | Tested; no benefit. |

## Open decisions for the maintainer

1. **Window title.** Decided (maintainer): follow the page title with a
   ` — foob` suffix; remote titles untrusted, local content trusted. Implemented
   and tested.
2. **Web process crash.** Today the window goes blank and stays. Options: reload
   once, show an error page, or exit with a distinct code. M2 policy.
3. **Window app-id.** WPE exposes no setter; current app-id comes from the
   program name. Confirm that suits compositor window rules.
4. **JIT-off.** Keep default on. Do you want an opt-in low-memory/untrusted mode
   in M2 profiles?
5. **Host compositor closed-lid state.** With the laptop lid closed the host niri
   ignored `close-window` and showed no focused window; this made the original
   `--stop compositor` benchmark mode time out for both binaries. It is an
   environment state, not an application defect; the nested test replaces it.

## Reproduce

```sh
meson setup build && meson compile -C build
meson test -C build --print-errorlogs      # allowlist unit test + CLI contract
python3 tests/nested_lifecycle.py build/foob   # needs a niri desktop session
```
