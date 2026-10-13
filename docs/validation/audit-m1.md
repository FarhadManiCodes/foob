# M1 code, integration and efficiency audit

2026-10-13, branch `m1-audit` (cut from `m0-m1` at `4fb9c9c`). Gate A is still
the maintainer's decision; no M2 task was started. This audit restructures the
code and fixes defects in existing behaviour; two maintainer-decided defaults
overlap M2-03 (same-window `target=_blank` links, interim download cancel) and
are tracked there. So the M1 hashes in `measurements.md` describe
the unmodified `m0-m1` binary, not this branch.

Method: the `foob-milestone-audit` skill (kept local, git-ignored). Everything
below was measured with stock WPE `2.54.1-2`, Mesa `26.2.4`, GCC 16 / C23.

## Defects found and fixed

| # | Finding | Evidence | Fix |
|---|---|---|---|
| 1 | **Compositor loss leaves the browser spinning.** If the compositor exits or crashes, `foob` keeps running at ~23% CPU on the dead socket with its helper processes alive. | Nested niri killed under the baseline binary: still alive 5 s later, state `R`, helpers present. `tests/nested_lifecycle.py` hangs on the baseline and passes on the fix. | Handle `WPEDisplay::disconnected`, exit immediately with new code 3 (`FOOB_EXIT_DISPLAY_LOST`), as upstream MiniBrowser does. Helpers exit when their IPC peer disappears (verified: no survivors). |
| 2 | Startup allowlist accepted URIs with raw spaces and control characters; GLib's parser tolerates both. (An out-of-range port such as `:99999` was also suspected; GLib 2.90.1 already rejects it, so that guard is defense in depth for older GLib and `m0-m1` was not affected on this system.) | New C unit test `tests/test_target.c`, failed on first run for the space and control-character cases. | Reject any byte <= 0x20 or 0x7f before parsing; keep an explicit port check. |
| 3 | `WebKitWebView::close` (page `window.close()`) was ignored. | Upstream handles it; single view means quit. | Connect `close` and stop the loop. Not exercised by a test: WebKit only honours `window.close()` for script-opened windows. |
| 4 | **Silent downloads.** An attachment response was saved into `~/Downloads` with no prompt. | Probe against the baseline: `payload.bin` appeared. `tests/nested_lifecycle.py download-cancelled` streams a 300 MiB attachment from a loopback server and passes only if the server sees the transfer abort after under 16 MiB of the 300 MiB (it sent 0.1 MiB in the one-off run and 1.2 MiB once under load), the stream stays stopped, and no file holding the payload exists in the download, cache, data or temp directories (all redirected below a temp dir). On the baseline the server is still streaming after 10 s and the test fails (negative control); the earlier one-off run saw 81 MiB sent and `.wkdownload` files there versus 0.1 MiB and no files now. | Cancel every download on `WebKitNetworkSession::download-started` until M3-03 designs an explicit per-download decision (requirement and acceptance criteria added to the roadmap). |
| 5 | `target=_blank` links did nothing (dead links); `window.open` returned `null`. | Probe: no window appeared. | Decided: `target=_blank` links and forms navigate the **current** window for user-gesture requests that pass `foob_navigation_allowed()` (startup allowlist; remote never navigates to local); no-gesture requests stay blocked. **`window.open()` is unsupported** and returns `null`: WebKit reports it through the `create` signal, which foob does not handle (maintainer: document only; popups are roadmap M2-11 and M3-01). **Verified:** scripted requests (`window.open`, a programmatic click on a `_blank` link, with or without WebKit's popup blocker) never navigate and never reach foob's policy handler (`tests/nested_lifecycle.py`, re-checked after a 3 s wait); a second review, using a scratch build that forces a user gesture, saw a gesture `a.click()` and a `_blank` form submit reach the handler and navigate the same window, while a gesture `window.open` only fires `create`, and mailto/`javascript:` links reach the handler and are rejected. **POST forms:** after the Codex review, a scratch experiment with a forced gesture showed `webkit_web_view_load_uri` turning a `target=_blank` POST form into a GET with 0 bytes (`Echo GET 0 bytes`); loading the action's own request keeps it (`Echo POST 15 bytes`); the product now does the latter. **Not verified:** a real mouse click (guided check); foob's own gesture check and the remote-to-`file:` rule have no automated end-to-end test (the latter only the `foob_navigation_allowed` unit test, since WebKit stops that request first). Separate window deferred to roadmap M2-11. |
| 6 | **Web process crash left a blank or frozen window** with no signal. | `tests/nested_lifecycle.py web-process-crash` SIGKILLs the exact `WPEWebProcess`: the baseline stays running (negative control), the fix exits with code 4 and no survivors. | Handle `web-process-terminated`: stop the loop and return `FOOB_EXIT_ENGINE_LOST` (4) through `foob_browser_run()`, so the normal teardown runs. Reload-once recovery stays in roadmap M3-07. |
| 7 | **Unstable window app-id.** niri reported `org.webkit.app-<sha256 of the executable's absolute path>`, so compositor window rules could not match foob and broke whenever the binary moved. | Nested niri, `niri msg -j windows`: the hash equals the SHA256 of the absolute binary path. Setting the GLib program name to a valid application ID before the view exists changes it; a name without a dot is ignored and the hash returns. Every `tests/nested_lifecycle.py` scenario now asserts the app-id; the baseline binary fails it. | `g_set_prgname("dev.foob.Browser")` in `foob_browser_new()` (name chosen by the maintainer). M2-04 passes explicit profile directories, so the program name does not decide any storage path. |

## Design changes (behaviour unchanged)

`src/main.c` (one 200-line file) became `main.c` (CLI), `target.[ch]` (pure GLib
allowlist, unit-tested without a display), `diagnostics.[ch]` (all redacted
logging, null-safe when disabled, one geometry scheduler instead of two copies),
`browser.[ch]` (lifecycle), `exit_status.h`. Diagnostics output is
byte-identical to the baseline on SIGTERM and SIGINT. This pure refactor grew `.text` by ~100 bytes; the later behaviour changes account for the rest of the growth in the binary-size section.
`window_title.[ch]` (pure GLib) maps the page title to the compositor-visible
window title, `<page title> — foob`, or plain `foob` without a usable title.
`foob_uri_is_local()` in `target.c` is the single trust decision: `about:blank`
and `file:` URIs without a remote host are local and trusted; every other origin
is untrusted. Untrusted titles are repaired to valid UTF-8, lose control
characters (C0, DEL and C1), lose invisible format characters (Unicode bidi
overrides, which could visually reorder the trailing `foob`, plus zero-width and
tag characters; the joiners U+200C and U+200D stay), have whitespace collapsed
and are cut at 120 characters;
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
- Final re-run on revision `1fae570` (the revision of the final measurements): `gcc -fanalyzer` with `-Wconversion -Wshadow -Wformat=2 -Wnull-dereference` reports nothing; the ASan+UBSan build passes the 3 unit/CLI tests and all 11 nested scenarios with no UBSan runtime error and no leak attributed to foob. LeakSanitizer does report small leaks inside `libWPEWebKit`/`libdrm` (for example a 144-byte `drmGetDeviceFromDevId` allocation reached from `webkit_web_view_load_uri`); these are engine-owned and were suppressed by library (`leak:libWPEWebKit-2.0.so`, `leak:libdrm.so`).
- Hardening (readelf): PIE, full RELRO + `BIND_NOW`, non-exec stack, stack
  protector, stack-clash protection, CET (IBT/SHSTK) markers,
  `_FORTIFY_SOURCE=3` at `-O2` (set at compile time; foob imports no checked
  `*_chk` functions, so it has no effect that `readelf` can show). Before: PIE, partial RELRO (no `BIND_NOW`) and
  the distro-default stack protector, no explicit flags. No measurable
  cost: blank 168.8 vs 168.9 MiB, load times equal (first-pass numbers; the final
  evidence is [measurements-m1-audit.md](measurements-m1-audit.md)).

## Compiler flags

For our ~9.5 KB of code: `-Os` -5% text, `-O3` +1%, nothing measurable at
runtime because every hot path is inside the engine. `-O2` stays the baseline.
`-O3`, LTO, PGO and `-march=native` are deferred to the end and to the
custom-engine work, where they can matter; they cannot reduce the stock engine.
Reading assembly was not worthwhile for this code (runs once at startup); it
becomes relevant for code we own on hot paths in M2+ (converters, IPC parsing).

## Binary size

Size is tracked as a code-quality signal: any growth must be explainable by
features, and our binary is compared with the upstream program that does the same
job (WebKit's MiniBrowser, Arch package, stripped).

| | `m0-m1` | `m1-audit` | MiniBrowser 2.54.1 |
|---|---:|---:|---:|
| File, stripped | 18,992 B | 22,520 B | 26,896 B |
| `size` text / data / bss | 8,632 / 1,000 / 16 | 13,241 / 1,192 / 16 | 20,933 / 2,792 / 304 |
| File with debug info (`-g`) | 43,352 B | 70,584 B | n/a (distro-stripped) |
| Direct libraries | glib, gobject, WPEWebKit, libc | same | adds gio and gstreamer |

Compare stripped files only: the 70 KB figure is mostly `-g` and says nothing
about the shipped size. Like for like, foob is **smaller** than MiniBrowser
(22.5 vs 26.9 KB; text 13.2 vs 20.9 KB) and links fewer libraries, because it has
no command-line settings, automation, GStreamer or cookie options.

Growth since `m0-m1` is +3.5 KB stripped (+18%), +4.6 KB of text. Per object
(text bytes): `browser.c` 2,245, `diagnostics.c` 2,165, `target.c` 1,151,
`window_title.c` 1,020, `main.c` 499. Everything added is a behaviour change from
this audit (titles and their sanitizing, navigation policy, download cancel,
compositor-loss and web-process-crash exits, app-id) plus the hardening flags
(stack protector, CET markers, fortify), whose share was not separated. None of it
is dead code. Runtime effect: none measurable (blank 169.0 vs 169.4 MiB, interleaved,
see [measurements-m1-audit.md](measurements-m1-audit.md)); the engine library is 148 MB on disk.

Rule: record stripped size and `size` per object at each milestone audit and
explain growth of more than about 10%. `-Os` would save ~5% of our text (about
0.5 KB) and is not worth the speed trade.

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

Regression pairing, **superseded for memory by
[measurements-m1-audit.md](measurements-m1-audit.md)** (interleaved, final revision;
the numbers here are from an earlier revision, kept for the Semantic Scholar row and
the latency figures, which were not repeated) (new vs `m0-m1` binary, same tooling): blank 168.8 vs 168.9,
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
| Title | fixed `"foob"` | follows `notify::title` | Done: page title plus ` — foob`, remote titles sanitized (decision 1). |
| Web process crash | logged only | not handled in main either | Done: exit 4 (decision 5); reload-once recovery is roadmap M3-07. |
| Settings / features | WebKit defaults | explicit feature flags for developer use | Defaults are the recommendation for a browser; revisit with the M2 policy. |
| Memory pressure | not set | not set | Tested; no benefit. |

## Behavior of unhandled WebKit signals (probed in a nested compositor)

| Probe | Result | Verdict |
|---|---|---|
| `alert()` / `confirm()` / `prompt()` | auto-dismissed immediately (`false` / `null`), page continues | Safe default; roadmap M3-01 defines real handling. |
| `window.open()` (with or without user gesture) | `null`, no window (`create` signal unhandled) | Safe; unsupported until M2-11 / M3-01. |
| Geolocation / Notification permission | denied (`code 1` / `denied`) | Safe default; M3-05 reviews permission defaults. |
| Expired TLS certificate | load refused (`g-tls-error-quark` code 2) | Correct; TLS never weakened. |
| HTTP 401 Basic | page stays loading, no prompt, no window title | **Known, accepted for M1** (maintainer: it fails visibly and is a useful signal). Prompt design is roadmap M2-10. |
| Attachment download | silently saved | Fixed, see defect 4. |
| PDF URL | `load-finished status=ok`; what is shown was not verified | Open: external PDF handoff is M3-04. |

## Decisions

Decided (maintainer, 2026-10-13):

1. **Window title** follows the page title with a ` — foob` suffix; remote titles
   untrusted, local content trusted. Implemented and tested.
2. **Downloads** are cancelled until M3; the no-silent-download requirement and
   acceptance criteria are in the roadmap (M3-03 and the M3 exit).
3. **HTTP authentication** is left as is (roadmap M2-10).
4. **Links asking for a new window** navigate the current window; a separate
   window (Shift-click) and the resources windows would share are roadmap M2-11.
5. **Web process crash** exits at once with code 4 (option chosen by the
   maintainer); reload-once recovery is roadmap M3-07.
6. **Window app-id** is `dev.foob.Browser` (maintainer's choice), stable across
   binary locations.

Also decided (2026-10-13): blocked-action feedback is an M2 task (M2-12); all
local files stay trusted, explicitly **not closed**, to be revisited in M4 (roadmap
open-decisions table); the triage rule for future findings is in roadmap section 7.
Findings carried into later milestones are written into the roadmap: M2-03 notes,
M2-12, notes under M3-01 to M3-05, M5-10 (including the JIT-off opt-in, not
adopted) and M6-07.

Still open:

7. **Branch and Gate A.** The maintainer reviews and merges `m1-audit` into
   `m0-m1` personally; the M1 evidence was re-recorded for the audit binary
   ([measurements-m1-audit.md](measurements-m1-audit.md)). Gate A stays undecided.
8. **Host compositor closed-lid state** is environment only (the nested test
   replaces the host-dependent benchmark mode); nothing to change.

## Guided checks still needed (real input, maintainer at the keyboard)

I cannot synthesize input (no input tools installed, by the maintainer's rule), so
these are unverified until the maintainer runs them: see
[guided-checks-m1-audit.md](guided-checks-m1-audit.md) for the fixtures
(`tests/fixtures/policy-remote.html`, `policy-local.html`, served by
`tests/policy_server.py`), commands and expected results. The new-window policy
function itself is unit-tested and the fixtures' loads and window titles are covered
by the nested suite.

## Reproduce

```sh
meson setup build && meson compile -C build
meson test -C build --print-errorlogs      # allowlist unit test + CLI contract
python3 tests/nested_lifecycle.py build/foob   # needs a niri desktop session
```
