# Findings, decisions and ideas register

The single place for what was found, what was decided and what is parked. Every
agent and the maintainer read it with [handoff.md](handoff.md) before starting work,
and update it **in the same commit** as the change it describes. Add a row; do not
delete one. Change its status and link the commit. A row is retired only on the
maintainer's request and then listed under [Retired entries](#retired-entries).

- **Status:** `open` (needs work), `fixed` (commit given), `carried` (belongs to a later
  milestone; the roadmap line is named), `decided` (maintainer choice, do not
  re-argue), `deferred` (maintainer postponed), `wontfix` (reason given).
- **Source:** `Maintainer`, `Claude` (the implementing agent), `Codex`, `Review-1`
  (Opus 5.5 merge review), `Review-2` (Sonnet 5.5 merge review), `Review-3` (`/code-review high` security pass before merging to main), `Review-4` (second `/code-review high` security pass, 2026-10-13).
- Findings that are not for the current milestone get a roadmap line naming their
  source (triage rule, roadmap section 7).

## Decisions log (maintainer, 2026-10-13 unless noted)

| ID | Decision |
|---|---|
| D-01 | Window title = page title + ` — foob`. Remote titles are untrusted and sanitized; local content is trusted. |
| D-02 | Downloads are cancelled until M3; "no silent downloads" is a requirement in roadmap M3-03 and the M3 exit. |
| D-03 | HTTP 401 authentication is left as is (page stays loading); decided in M2-10. |
| D-05 | `window.open()` is unsupported (returns `null`); documented, not implemented. Popups: M2-11 / M3-01. |
| D-06 | JIT-off is not adopted (2.3x slower on a CPU-heavy page, saves 11-32 MiB); revisit at M5-10. |
| D-07 | A dead web process makes foob exit with status 4 (option A); recovery such as one reload is M3-07. |
| D-08 | App-id is `dev.foob.Browser`. |
| D-09 | All local files (`file:`, `about:blank`) are trusted. **Not closed**: revisit in M4 (roadmap open-decisions table). |
| D-10 | Feedback for blocked actions (download, popup, rejected link) is an M2 task (M2-12). |
| D-11 | Triage rule: defects in existing behaviour are fixed now, with a test that fails on the old build, when they would propagate or stay wrong whatever later milestones deliver; missing features go to the owning milestone; an unsafe default gets the smallest safe interim default now plus a requirement later (roadmap section 7). |
| D-12 | No gesture-forcing hook inside the product just to test the policy handler (skip). |
| D-16 | Never disable TLS or sandbox checks. Diagnostics never print URLs, titles, error text, cookies or input contents. No input-automation tools. Never load Google Scholar (use Wikipedia, Semantic Scholar, local fixtures). |
| D-17 | Deferred by the maintainer: the font-language-menu glyph issue and the project license (M0-05). |
| D-18 | Binary size is tracked as a quality signal: record stripped size and `size` per object each milestone; explain growth over about 10%. |
| D-19 | PDFs open inline in the stock engine's PDF.js viewer; keep it as is for now (2026-10-13). The viewer's save button must still obey the M3-03 download rule. |
| D-20 | Removing accessibility (`USE_ATK` off) from the custom engine is a requested evaluation, tracked as roadmap M5-11; the a11y-bus warnings stay until then (2026-10-13). |
| D-21 | Maintainer, 2026-10-13, after the second security review: foob ignores **every** new-window request (`target=_blank` links and forms) and ships no injected script. A `_blank` link does nothing until M2-03 defines the behaviour and M2-11 the second window; feedback for the ignored click is M2-12. Supersedes D-04. Reason: it was not an M1 issue. The script re-implemented WebKit's target resolution (three follow-up findings in one review round, now retired as moot), cannot be tested without real clicks, and M2 would replace it. Safe default: nothing loads on a page's behalf, so the sandbox, Fetch Metadata and remote-to-local rules stay WebKit's |
| D-22 | Maintainer, 2026-10-14, after comparing `src/` with upstream MiniBrowser's `main.cpp`: M2 and M3 each get a task that repeats the comparison (M2-13, M3-09) so foob stays more concise, accurate and safer than the engine team's own minimal browser, and the items it does that foob does not (key handling, settings key file, mode indicator, path argument, background colour, `--version`) each get an owner. Baseline and method: [minibrowser-comparison.md](minibrowser-comparison.md). Cog is not a comparison target (a kiosk shell with its own Wayland layer, about 8,000 code lines). No code changed |

## Findings register

### Defects found by the M1 audit (details: [audit-m1.md](audit-m1.md))

| ID | Finding | Source | Status | Where |
|---|---|---|---|---|
| F-01 | Compositor loss left the browser spinning at about 23% CPU | Claude | fixed (exit 3, test `compositor-lost`) | audit-m1.md defect 1 |
| F-02 | Startup allowlist accepted raw spaces and control characters | Claude | fixed (unit test) | defect 2 |
| F-03 | Page `window.close()` was ignored | Claude | fixed; no automated test (WebKit honours it only for script-opened windows) | defect 3 |
| F-04 | Silent downloads written to `~/Downloads` | Claude | fixed as interim (cancel on `download-started`, test `download-cancelled`); real design: M3-03 | defect 4, D-02 |
| F-05 | `target=_blank` links were dead | Claude | fixed for links and forms; `window.open` stays unsupported (D-05). **Reversed by D-21:** `_blank` links and forms do nothing until M2-03 | defect 5 |
| F-06 | Dead web process left a blank or frozen window | Claude | fixed as interim (exit 4, test `web-process-crash`); recovery M3-07 | defect 6, D-07 |
| F-07 | Window app-id was a hash of the binary path | Claude | fixed (`dev.foob.Browser`) | defect 7, D-08 |

### Review findings

| ID | Finding | Source | Status | Where |
|---|---|---|---|---|
| F-10 | Blocked-request scenarios asserted nothing after waiting | Review-1 | fixed (re-check title and load count); see F-31 | commit 31debfd, `tests/nested_lifecycle.py` |
| F-11 | Stale "Decision needed" rows in audit-m1.md | Review-1 | fixed | audit-m1.md |
| F-12 | Trust judged by the main frame, not the frame that asked (remote iframe in a local page) (also reported as F-42) | Review-1 | carried (M4 trust row) | roadmap open-decisions table |
| F-14 | `wpe_view_get_toplevel()` can be null | Review-1 | fixed | `src/browser.c` |
| F-15 | Title sanitizer let C1, zero-width, tag and filler characters through | Review-1, Review-2 | fixed (blocklist widened, unit tests); homoglyphs and a page-supplied trailing `— foob` remain (M2-03 note 2) | `src/window_title.c` |
| F-16 | `_FORTIFY_SOURCE` comment wrong; caller-defined levels were redefined | Review-1, Review-2 | fixed (checks Meson `c_args`, six cases tested) | `meson.build` |
| F-17 | Exit status 4 could override a requested shutdown | Review-1 | fixed (`stopping` flag) | `src/browser.c` |
| F-18 | URLs with stray `%xx` in the path are silently rejected | Review-1 | carried (M2-03 note 5) | roadmap |
| F-19 | M2-10/11/12 were numbered out of order in the roadmap | Review-1 | fixed | roadmap |
| F-20 | Download payload scan missed files under 4 KiB | Review-1 | fixed | `tests/nested_lifecycle.py` |
| F-21 | `source_manifest()` hashed an empty list from another directory | Review-1 | fixed (assertion) | `tests/benchmark/process_tree.py` |
| F-22 | `window.open` with a user gesture does nothing, but docs and fixtures said it navigates | Review-2 | fixed in docs (D-05) | audit-m1.md, roadmap, fixtures, guided checks |
| F-23 | audit-m1.md still cited numbers from superseded runs | Review-2 | fixed (marked superseded) | audit-m1.md |
| F-24 | Defect 2's port-99999 claim is false on GLib 2.90.1 | Review-2 | fixed (doc corrected); the redundant port guard was removed later, see F-47 | audit-m1.md |
| F-25 | Remote-to-`file:` new-window rule covered only by a unit test | Review-2 | documented; real click is a guided check | measurements-m1-audit.md |
| F-26 | Trust is read from the current URI only on title change (small race) (also reported as F-45 and F-70) | Review-2 | carried (M2-03 note 3) | roadmap |
| F-27 | `display_disconnected` calls `_exit(3)` even after a requested shutdown (also reported as F-49) | Review-2 | wontfix: deliberate, matches upstream | `src/browser.c` |
| F-28 | Diagnostics pointer left dangling after free | Review-2 | fixed (set to null) | `src/browser.c` |
| F-29 | `target.h` said "not an M2 policy" while exporting `foob_navigation_allowed()` | Review-2 | fixed (wording) | `src/target.h` |
| F-30 | POST `target=_blank` forms lose their body. Preserve method and payload (use the action's request) or reject such submissions explicitly; never convert silently (also reported as F-13) | Codex (P2) | fixed: `webkit_web_view_load_request` with the action's request. Scratch experiment with a forced user gesture: before `Echo GET 0 bytes`, after `Echo POST 15 bytes`. No automated test (needs a gesture; D-12); guided check row 9 (real click: PASS, POST 15 bytes); evidence re-recorded on `9abb6fe`. **Superseded by F-51 / D-21:** foob no longer loads the request at all; a `_blank` form does nothing until M2-03 (scenario `blank-form-does-not-navigate`) | `src/browser.c` decide_policy, audit-m1.md defect 5 |
| F-31 | `scripted-new-window-requests-do-not-navigate` passed vacuously: the script ran in the head before `document.body` existed, so it threw before `a.click()` and `window.open()` | Codex (P2) | fixed: the script runs at the end of `<body>` and reports `click-ok,open-null` to the test server (`/probe`); the scenario asserts that report, then the title and one load. Negative control with scratch builds: popup blocker off + gesture check removed makes the scenario fail; blocker off + check kept passes (The scenario was retired in the test trim: WebKit's popup blocker stops scripted requests before foob's handler, so it could not fail from foob code.) | `tests/nested_lifecycle.py` |
| F-32 | The guided checks were hard to follow: the log was off-screen at the bottom of the page, clicks that do nothing could not be told from clicks that did not register, row 9 gave no clear verdict | Maintainer | fixed: fixed log panel, click registration lines, item numbers match the checklist, Echo page shows PASS or FAIL; fixtures only, no C change | `tests/fixtures/policy-*.html`, `tests/policy_server.py`, guided-checks-m1-audit.md |
| F-33 | A PDF URL opens inline in the stock engine's PDF.js viewer (save, print, annotate buttons), against the "external PDF viewer, no embedded renderer" decision | Maintainer (guided check row 12) | decided D-19: keep as is; save-button requirement carried to M3-03/M3-04 | roadmap M3-04, audit-m1.md |
| F-34 | Camera/microphone request: no prompt, `OverconstrainedError`, "no device found amongst 0 devices"; the permission default could not be observed | Maintainer (guided check row 11) | carried: M3-05 (roadmap note) | roadmap M3-05 |
| F-35 | `tests/nested_lifecycle.py` ran nothing and exited 0 when the requested scenario name was misspelled or obsolete (`web-process-crsh`), so a typo reported success | Codex (P2) | fixed: every requested name must match a scenario, otherwise the runner exits non-zero and lists the known names. Negative control: the old runner returned 0 for `web-process-crsh`, the new one 1; a mistyped flag such as `--keeep` is rejected the same way | `tests/nested_lifecycle.py` |
| F-36 | Fixture launch 1 failed for both builds in the evidence run ("navigation changed after requested completion") | Maintainer asked why | explained, trigger not proven: the page loaded twice because the fixture reloaded itself (its Reload button or `?warm=1`, which was not used); a deliberate reload reproduces the signature; 0 failures in 28 other fixture launches (12 hands-off repeats this session). Most likely input to the new window during the run. The collector's guard worked as designed | measurements-m1-audit.md |
| F-37 | `run.py` truncated and overwrote an existing output JSON and log, and `suite.py` used fixed paths, so rerunning the suite destroyed earlier evidence | Codex (P2) | fixed: `run.py` refuses an existing JSON or log path before launching anything; `suite.py` writes each run into a new `docs/validation/runs/suite-<UTC time>/` folder; `summarize.py <dir>` summarizes such a folder. Negative control: the old `run.py` changed an existing record's hash, the new one exits 1 and leaves it byte-identical. A real `suite.py` run on the fixed tools wrote only to `docs/validation/runs/suite-20261013T192206Z/` (10 of 10 pass, exit 0); the original runs were untouched | `tests/benchmark/` |
| F-38 | `interactive.py` recorded `lifecycle_result: fail` but exited 0 | Codex (P2) | fixed: exits 1 unless the lifecycle passed. Negative control with `--binary /bin/false`: old rc 0, new rc 1 | `tests/benchmark/interactive.py` |
| F-39 | After F-37 the suite writes to `runs/suite-<time>/`, but `summarize.py` read only the top level of `runs/`, so the README command republished old measurements (or empty ones on a fresh checkout), and the explicit-directory mode wrote only into the ignored folder | Codex (P2) | fixed: `summarize.py [dir] [--output FILE] [--replace]` summarizes one chosen run folder; an existing output (and the curated file) needs `--replace`; input with no run records is rejected before anything is written. Default behaviour (top-level runs to `docs/validation/results.json`) is unchanged and reproduced the same content. Checked: suite folder summarized to a new file (10 measurements), existing output and curated file refused, empty folder rejected | `tests/benchmark/summarize.py`, README |
| F-40 | A nested compositor that started but never reported its sockets within 15 s was left running when the scenario failed | Codex (P2) | fixed: `start_compositor()` terminates and reaps the child before re-raising. Negative control with a fake `niri` that sleeps: old runner left 1 process, new runner 0 | `tests/nested_lifecycle.py` |
| F-41 | Security review (Review-3, `/code-review high`): the remote-to-local rule is enforced only for NEW_WINDOW decisions; NAVIGATION_ACTION and RESPONSE fall through to WebKit | Review-3 | carried: M2-03 note 1 (unify navigation policy); WebKit refuses remote-to-`file:` today (audit-m1.md defect 5) | roadmap M2-03 |
| F-43 | Review-3: sanitizer misses invisible and blank characters (U+00AD, U+2800, U+17B4/17B5, U+FFF9-FFFC, U+1D173-1D17A) | Review-3 | fixed: untrusted titles lose every Unicode format character (category Cf) except the joiners U+200C/D and the visible Arabic and Syriac number signs, plus the invisible non-Cf characters (U+034F, 115F/1160, 17B4/17B5, 2800, 3164, FFA0, FFFC, E0000-E01EF); unit tests added. Negative control: the new tests abort against the old `window_title.c` | `src/window_title.c` |
| F-44 | Review-3: local and remote titles share the ` — foob` suffix, so a remote page can imitate a local title | Review-3 | carried: D-01 design plus M2-03 note 2; a trust cue is an M2-12 idea (I-01) | roadmap M2-03 |
| F-46 | Review-3: `about:blank` is always trusted, though a document there may inherit an origin | Review-3 | carried: D-09, M4; not exploitable today (navigating a remote page to `about:blank` ends its scripts and no popup exists) | roadmap M4 trust row |
| F-47 | Review-3: `g_uri_get_port(uri) <= UINT16_MAX` is always true (unset port is -1); GLib and WebKit parse URIs differently | Review-3 | fixed: the dead port check is removed; backslashes are now rejected outright, because WebKit's parser reads them as slashes and GLib's does not. Probe: backslash and odd-port URIs differ between parsers only for remote URLs (`http://good.com\.evil.com/`), never in the local/remote classification of a `file:` URL, so no exploitable difference found. Unit tests added (four backslash cases); they abort against the old `target.c` | `src/target.c` |
| F-48 | Review-3: downloads rely on one cancel handler (no RESPONSE-level ignore); permission defaults are unobserved; nested test scans only for payload bytes | Review-3 | carried: M3-03 (add: ignore non-displayable responses before a download object exists; test for any new file), M3-05 (F-34) | roadmap M3-03, M3-05 |
| F-50 | Review-3: `policy_server.py` trusts `Content-Length` (non-numeric or negative values raise or read to EOF) | Review-3 | fixed: malformed, negative or oversized lengths get HTTP 400. Negative control: old server hung on `-1` and a huge length and crashed its handler on `abc`; new server answers 400 to all three and 200 to a valid POST | `tests/policy_server.py` |
| F-51 | Review-4 (medium): a `target=_blank` request from a sandboxed iframe with `allow-popups` (no `allow-top-navigation`) was loaded by foob into the main view, so the frame's content replaced the top page at its real origin, unsandboxed. WebKit also turns such a frame's refused `_top` navigation into a new-window request, which took the same path | Review-4 | **Final fix (D-21): the injected script was removed.** `decide_policy` ignores every new-window request and foob loads nothing on a page's behalf, so the frame can never be replaced. Committed scenario `blank-form-does-not-navigate` (sandboxed and top forms with `target=_blank` submitted by `requestSubmit()`: no `/echo` request, one load, title unchanged); it fails on the script build `f40e4b9` (the window navigates). Real click (maintainer, `4e9b6c2`): guided rows 19, 1, 3, 9, 15, 16 all pass (nothing happens). **Superseded intermediate fix:** fixed: an injected script (own script world, all frames) turns a user's click on a `_blank` link and a `_blank` form submission into the page's own `_top` navigation, so WebKit applies the frame's sandbox; `decide_policy` ignores every remaining new-window request and foob makes no load on a page's behalf. Scratch builds with a forced gesture (popup blocker off, gesture check or trusted-event check removed): before, the landing page loaded top-level with origin `http://127.0.0.1:…` for both `_blank` and `_top`; after, the top page stays (1 load). Scripted link clicks stay inert (not trusted events). Committed scenario `blank-form-navigates-as-page` (sandboxed frame must not reach `/echo`); it fails on the pre-fix binary (no navigation at all). A real click is guided row 19. Same-window `_blank` (the retired D-04, superseded by D-21) was kept in that intermediate version: link, POST form, local-to-local, local-to-remote and `<base target=_BLANK>` checked in the scratch build; remote-to-`file:` still refused, now by WebKit | `src/browser.c`, `tests/nested_lifecycle.py`, guided checks |
| F-52 | Review-4 (medium, reported as unverified CSRF): the app-initiated reload of a page's `_blank` request dropped its initiator. **Verified:** SameSite cookies were not affected (a cross-site `_blank` POST carried no Strict or Lax cookie, a cross-site `_blank` GET only the Lax one, as in a normal browser; a same-site POST control carried both), but WebKit sent `Sec-Fetch-Site: none` instead of `cross-site` (and `none` instead of `same-origin` for the guided row 9 form), so a server whose Fetch Metadata policy accepts `none` as user-typed would accept a cross-site POST | Review-4 | **Gone with the script (D-21):** no request is made at all, so `Sec-Fetch-Site` cannot be wrong. Intermediate fix: fixed by the F-51 change: after it the cross-site form arrives with `Sec-Fetch-Site: cross-site` and the same-origin form with `same-origin`. Scenario `blank-form-navigates-as-page` asserts `cross-site` (localhost vs 127.0.0.1); the echo page of guided row 9 now shows the header and counts `none` as FAIL | `src/browser.c`, `tests/policy_server.py` |
| F-53 | Review-4 (low): `foob_navigation_allowed()` judged the source by `webkit_web_view_get_uri()`, the pending URL, so during a pending local navigation a remote page could look local | Review-4 | fixed by construction with F-51: foob no longer loads anything on a page's behalf, so the check is no longer on any path (kept, unit-tested, for M2-03). Race not demonstrated before the change | `src/target.h` |
| F-54 | Review-4 (low): a remote title that starts with a strong right-to-left letter (`<title>א Bank</title>`) sets the paragraph direction, so bars that detect direction from the first strong character show ` — foob` at the visual start | Review-4 | fixed: an untrusted title that may contain right-to-left text (the Hebrew-to-Arabic blocks, presentation forms and the right-to-left supplementary ranges, including their unassigned code points) starts with U+200E LEFT-TO-RIGHT MARK, so the first strong character is left to right for both the full bidi algorithm and Pango's first-strong check; LTR-only titles and local titles are unchanged. (An FSI…PDI isolate alone would not do: Pango's base-direction check does not skip isolates.) New unit test `/window-title/direction`; it aborts against the old `window_title.c` | `src/window_title.c` |
| F-55 | Review-4 (low): the F-43 filter misses some default-ignorable code points (U+180B-180D, U+180F, U+2065, U+FFF0-FFF8, U+E01F0-E0FFF) that render invisible | Review-4 | fixed: the filter now also removes U+180B-180F, U+2065, U+FFF0-FFF8 and U+E0000-E0FFF, so with category Cf it covers the whole Default_Ignorable_Code_Point set except the joiners and variation selectors 1-16 (U+FE0F is needed for emoji; tested). Mongolian free variation selectors are lost in remote titles (the letters stay). Unit test added; it aborts against the old `window_title.c` | `src/window_title.c` |
| F-56 | Review-4 (low): `summarize.py docs/validation` targets `docs/validation/results.json`, which skips both F-39 guards and overwrites the curated file without `--replace`; the guards compare unresolved paths, so `./docs/validation/results.json` as `--output` also escapes the curated-file check | Review-4 | fixed: only the default call (no directory, no `--output`) regenerates the curated file without `--replace`; any other path to it, compared resolved, is refused. Negative control in scratch copies: old script `summarize.py docs/validation` exit 0 and the curated file changed; new script exit 1, file unchanged; the default call and `--replace` still work. (An absolute `--output` spelling was already refused by the generic exists check.) | `tests/benchmark/summarize.py` |
| F-57 | Review-4 (low): `desktop_probe.py` writes `/tmp/foob-<label>.log`, `.json` and `-<name>.png` with no existence check (F-37 covered only `run.py` and `interactive.py`), and leaves screenshots in shared `/tmp` with the default umask | Review-4 | fixed: log, record and screenshots go into a new directory with mode 0700 (`--output-dir`, which must not exist, or a fresh `$TMPDIR/foob-probe-<label>-*`); an existing `--output-dir` is refused before anything launches (checked). A real probe run with the new paths is part of the next evidence re-record (I-07) | `tests/benchmark/desktop_probe.py` |
| F-61 | The F-56 fix granted the curated-file exemption by resolved-path equality, so an explicit absolute path to `docs/validation/runs` counted as the default call and overwrote `docs/validation/results.json` without `--replace` (before F-56 it wrote `<directory>/results.json`) | Review (P2), supplied by the maintainer | fixed: only an omitted directory (and no `--output`) is the default call; any explicit directory writes `<directory>/results.json`, so `summarize.py docs/validation/runs` (any spelling) now writes into the ignored runs folder instead of the curated file. Scratch copies: absolute runs directory before: exit 0, curated changed; after: exit 0, curated unchanged. `docs/validation` and an absolute `--output` still refused, the default call and `--replace` still work | `tests/benchmark/summarize.py` |
| F-63 | `summarize.py` accepted a directory holding only a differently named summary file (for example `docs/validation`, which contains `results-m1-audit.json`): the file passed the no-records guard, was treated as a lifecycle run and written out as a bogus entry; with `--replace` it would overwrite curated evidence with that result | Review (P2), supplied by the maintainer | fixed: every input file must be a run record (an object with a run outcome and no `schema_version`); otherwise the script exits before writing anything. Reproduced first (exit 0, entry `summary-other` in `lifecycle_and_controls`); after: exit 1, nothing written, a copy of the curated file unchanged with `--replace`. The original runs (13/8/3 records, as in the committed `results.json`) and the audit runs are still accepted | `tests/benchmark/summarize.py` |
| F-64 | The compact summary dropped `source_dirty` (whitelist), so a run on an uncommitted tree looked clean. The original M1 raw runs carry `source_dirty: true`, disclosed in prose in `measurements.md` but not in `results.json` | Review (P2), supplied by the maintainer | fixed: `source_dirty` is kept (checked with the original dirty record). `results-m1-audit.json` was regenerated from the raw `0af4314` runs (all `false`). The curated original `results.json` was left as committed (legacy evidence; its dirty flag is in `measurements.md`) | `tests/benchmark/summarize.py` |
| F-65 | `desktop_probe.py` records carried no binary hash, source hash, revision or dirty flag (`interactive.py` also lacked the dirty flag), so probes could not be tied to a build | Review (P2), supplied by the maintainer; `interactive.py` found by Claude | fixed: one `provenance()` helper in `process_tree.py` (revision, `source_dirty`, source and binary SHA256, source file list) now serves `run.py`, `interactive.py` and `desktop_probe.py`; the probe takes it before launching. Checked with real runs (`run.py` about:blank pass; probe on `viewport.html` pass with all fields). Earlier probe records (`d9ea90d`) stay without them; the binary was identified by revision in the prose. `about:blank` cannot be used with the probe (the load ends before niri lists the window; use a fixture) | `tests/benchmark/process_tree.py`, `desktop_probe.py`, `interactive.py`, `run.py` |
| F-66 | PR-style review (`/code-review high`, agent a2e77d4): `scenario()` cleanup in `nested_lifecycle.py` could raise `TimeoutExpired` from `compositor.wait(timeout=10)` before `os.killpg(browser)`, leaving a browser tree on the desktop | Review-5 (low, test tooling) | fixed: the browser process group is killed first, then the compositor is terminated, and killed if it ignores the terminate for 10 s. Read, not reproduced (a hung nested niri cannot be provoked on demand); the suite still passes 12/12 | `tests/nested_lifecycle.py` |
| F-67 | Same review: `transfer_stopped_without_files` read every cache file fully (twice) to compare 4 KiB | Review-5 (low, test tooling) | fixed: reads only the first 4096 bytes | `tests/nested_lifecycle.py` |
| F-68 | Same review: a remote title cut after a space ended `abc … — foob` | Review-5 (cosmetic) | fixed: the cut text is right-trimmed before the ellipsis; unit test aborts on the old `window_title.c` (checked). The cut is still by code point, not by grapheme: a combining or ZWJ sequence can be split (cosmetic, accepted; needs Pango segmentation) | `src/window_title.c`, `tests/test_window_title.c` |

**Result of F-31:** the earlier claim "scripted requests never reach foob's policy
handler" was unproven because the scenario and the first debug experiment used the same
broken page. Repeated with a page that provably runs (probe report): with default
settings a scripted `window.open` and a programmatic `a.click()` on a `_blank` link
never reach `decide-policy` or `create` (WebKit's popup blocker), so the claim holds.
With `javascript-can-open-windows-automatically` on (scratch build) both reach the
handler with gesture=0 and foob's gesture check blocks them; with the check also removed
the page navigates and the scenario fails. foob's own gesture check therefore has
evidence only from scratch builds, not from a committed test (D-12). **Since F-51** the
gesture check is gone: `decide_policy` ignores every new-window request, so scripted and
real requests alike do nothing (D-21; an intermediate version of F-51 retargeted trusted
clicks and submits with an injected script and was removed again).

### Carried into later milestones (roadmap lines hold the detail)

| Milestone | Items |
|---|---|
| M2 | M2-13 MiniBrowser comparison (D-22; items I-09 to I-14), M2-03 notes (unify navigation policy and split `browser.c`, sanitizer limits, trust per committed load, POST handling, `%` URLs, handler reachability, `target=_blank` and named targets, D-21), M2-10 HTTP auth, M2-11 second window and `window.open`, M2-12 blocked-action feedback |
| M3 | M3-09 MiniBrowser comparison (D-22), M3-05 also decides the engine-default settings (media stream, ITP, site quirks), M3-01 popups and `window.opener`, right-click default; M3-02 file input default; M3-03 no silent downloads and downloaded-file trust; M3-04 PDF URL display; M3-05 permission and certificate defaults; M3-07 crash recovery |
| M4 | Local-file trust rule (D-09), remote iframe in a local page |
| M5 | M5-10 JIT-off opt-in, GL renderer measurement, under-100-MiB attribution |
| M6 | M6-07 fractional scaling, multi-monitor, nested suite and size/hardening checks as release gates |

## Retired entries

Removed from the tables above on the maintainer's request (2026-10-13) because they were
not accurate, duplicated another entry, or are done and recorded elsewhere. The numbers
stay unused so older commits and notes still resolve; the git history has the full text.

| ID | Why retired |
|---|---|
| D-04 | Superseded by D-21 (`target=_blank` is ignored). |
| D-13, D-14, D-15 | Working rules (agents never merge, no M2 or Gate A approval without a decision, one folder, no attribution trailers); they live in `AGENTS.md`, `CLAUDE.md` and the handoff rules. |
| first D-21 (merge authorization) | Done: `m1-audit` was merged into `m0-m1` and the audit branch deleted (see the handoff). Its number collided with the new-window decision, which keeps D-21. |
| F-13 | Duplicate of F-30 (POST `target=_blank` forms). |
| F-42 | Duplicate of F-12. |
| F-45, F-70 | Duplicates of F-26 (trust is decided on title change only); the fix is M2-03 note 3. |
| F-49 | Duplicate of F-27 (`_exit(3)` on compositor loss, deliberate). |
| F-58, F-59, F-60 | Moot: they concerned the injected same-window script, which D-21 removed. |
| F-62 | Observation: the audit build's Wikipedia navigation time was higher in two busy sessions. It did not reappear on `679fb9a`, and on `147ae45` only one first-launch outlier (1136 ms, three others 530-545 ms) (493-536 ms against 517-544 ms for the control); the data is in `measurements-m1-audit.md`. |
| F-69 | Not reproduced: a scratch build logging the toplevel at the first `update_window_title()` call showed it set for `about:blank` and a file fixture. The null check stays as defence. |
| F-71 | Not a defect: `SIGHUP` to a running foob (8 tracked processes) ended it by the signal with 0 survivors after 10 s, as `SIGTERM` does. |
| F-72 | Declined by design: the default `summarize.py` call is the documented regeneration of the curated file (F-56, F-61); change to always requiring `--replace` if the maintainer prefers. |
| F-73 | Duplicate of F-53 and `AGENTS.md`: `foob_navigation_allowed()` is unused, kept and unit-tested for M2-03. |
| F-74 | History: commit `da48866` changed a fixture without updating the registers; not rewritten, the final tree is consistent. |
| I-02, I-03 | Already roadmap M2-03 (policy module) and F-26 (trust once per committed load). |

## Ideas parking lot (not tasks yet)

| ID | Idea | Notes |
|---|---|---|
| I-01 | Use the window title as a status channel (`Download blocked — foob`) | Chrome-free; belongs to M2-12 |
| I-04 | `create` handler so `window.open` can navigate the same window | M2-11 / M3-01; decide with popup policy |
| I-05 | Untrusted-site mode with JIT off | M5-10 |
| I-06 | Hardening-vs-feature split of the +3.5 KB binary growth | measure by building without hardening flags |
| I-08 | A benchmark-only fixture without the "Reload this fixture" button would stop a stray click from invalidating a run (F-36) | changes the fixture hash recorded since M1, so only if it recurs |
| I-09 | Key handling through `WPEView::event` returning TRUE, as MiniBrowser does | M2-01, M2-09; probe event consumption first |
| I-10 | `webkit_settings_apply_from_key_file()` / `wpe_settings_load_from_keyfile()` as the configuration path (both in the installed headers, not tried) | M2-04 |
| I-11 | Show persistent versus ephemeral in the window title (`[Private]` upstream) | M2-04, M2-12 |
| I-12 | Accept a plain path as the startup argument (`g_file_new_for_commandline_arg()` upstream) | M2-02; the allowlist stays the only gate |
| I-13 | Check for a white flash before first paint; `webkit_web_view_set_background_color()` | M2-12 |
| I-14 | `--version` (a rejected URI today, exit 2) | M2-02; weigh against binary size (D-18) |
| I-07 | Run `desktop_probe.py` on the final binary | done on `d9ea90d` (pass, 2026-10-13); repeat after any C change |
