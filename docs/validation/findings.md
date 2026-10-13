# Findings, decisions and ideas register

The single place for what was found, what was decided and what is parked. Every
agent and the maintainer read it with [handoff.md](handoff.md) before starting work,
and update it **in the same commit** as the change it describes. Add a row; do not
delete one. Change its status and link the commit.

- **Status:** `open` (needs work), `fixed` (commit given), `carried` (belongs to a later
  milestone; the roadmap line is named), `decided` (maintainer choice, do not
  re-argue), `deferred` (maintainer postponed), `wontfix` (reason given).
- **Source:** `Maintainer`, `Claude` (the implementing agent), `Codex`, `Review-1`
  (Opus 5.5 merge review), `Review-2` (Sonnet 5.5 merge review), `Review-3` (`/code-review high` security pass before merging to main).
- Findings that are not for the current milestone get a roadmap line naming their
  source (triage rule, roadmap section 7).

## Decisions log (maintainer, 2026-10-13 unless noted)

| ID | Decision |
|---|---|
| D-01 | Window title = page title + ` — foob`. Remote titles are untrusted and sanitized; local content is trusted. |
| D-02 | Downloads are cancelled until M3; "no silent downloads" is a requirement in roadmap M3-03 and the M3 exit. |
| D-03 | HTTP 401 authentication is left as is (page stays loading); decided in M2-10. |
| D-04 | `target=_blank` links navigate the same window. Opening a second window (Shift-click) is future work that must define shared resources (M2-11). |
| D-05 | `window.open()` is unsupported (returns `null`); documented, not implemented. Popups: M2-11 / M3-01. |
| D-06 | JIT-off is not adopted (2.3x slower on a CPU-heavy page, saves 11-32 MiB); revisit at M5-10. |
| D-07 | A dead web process makes foob exit with status 4 (option A); recovery such as one reload is M3-07. |
| D-08 | App-id is `dev.foob.Browser`. |
| D-09 | All local files (`file:`, `about:blank`) are trusted. **Not closed**: revisit in M4 (roadmap open-decisions table). |
| D-10 | Feedback for blocked actions (download, popup, rejected link) is an M2 task (M2-12). |
| D-11 | Triage rule: defects in existing behaviour are fixed now, with a test that fails on the old build, when they would propagate or stay wrong whatever later milestones deliver; missing features go to the owning milestone; an unsafe default gets the smallest safe interim default now plus a requirement later (roadmap section 7). |
| D-12 | No gesture-forcing hook inside the product just to test the policy handler (skip). |
| D-13 | Agents never merge; the maintainer reviews and merges. No M2 work and no Gate A approval without an explicit decision. |
| D-14 | Work in the main checkout only. Do not create worktrees, clones or extra folders without asking. |
| D-15 | No `Co-Authored-By` or AI-attribution trailers in commits or PR bodies. |
| D-16 | Never disable TLS or sandbox checks. Diagnostics never print URLs, titles, error text, cookies or input contents. No input-automation tools. Never load Google Scholar (use Wikipedia, Semantic Scholar, local fixtures). |
| D-17 | Deferred by the maintainer: the font-language-menu glyph issue and the project license (M0-05). |
| D-18 | Binary size is tracked as a quality signal: record stripped size and `size` per object each milestone; explain growth over about 10%. |
| D-19 | PDFs open inline in the stock engine's PDF.js viewer; keep it as is for now (2026-10-13). The viewer's save button must still obey the M3-03 download rule. |
| D-20 | Removing accessibility (`USE_ATK` off) from the custom engine is a requested evaluation, tracked as roadmap M5-11; the a11y-bus warnings stay until then (2026-10-13). |
| D-21 | Maintainer explicitly authorized merging `m1-audit` into `m0-m1` and deleting the local audit branch. Fast-forward through `ced9c6c`; final review found no actionable regressions and registered tests pass 3/3. Gate A remains unapproved; no M2 authorization (2026-10-13). |

## Findings register

### Defects found by the M1 audit (details: [audit-m1.md](audit-m1.md))

| ID | Finding | Source | Status | Where |
|---|---|---|---|---|
| F-01 | Compositor loss left the browser spinning at about 23% CPU | Claude | fixed (exit 3, test `compositor-lost`) | audit-m1.md defect 1 |
| F-02 | Startup allowlist accepted raw spaces and control characters | Claude | fixed (unit test) | defect 2 |
| F-03 | Page `window.close()` was ignored | Claude | fixed; no automated test (WebKit honours it only for script-opened windows) | defect 3 |
| F-04 | Silent downloads written to `~/Downloads` | Claude | fixed as interim (cancel on `download-started`, test `download-cancelled`); real design: M3-03 | defect 4, D-02 |
| F-05 | `target=_blank` links were dead | Claude | fixed for links and forms; `window.open` stays unsupported (D-05) | defect 5 |
| F-06 | Dead web process left a blank or frozen window | Claude | fixed as interim (exit 4, test `web-process-crash`); recovery M3-07 | defect 6, D-07 |
| F-07 | Window app-id was a hash of the binary path | Claude | fixed (`dev.foob.Browser`) | defect 7, D-08 |

### Review findings

| ID | Finding | Source | Status | Where |
|---|---|---|---|---|
| F-10 | Blocked-request scenarios asserted nothing after waiting | Review-1 | fixed (re-check title and load count); see F-31 | commit 31debfd, `tests/nested_lifecycle.py` |
| F-11 | Stale "Decision needed" rows in audit-m1.md | Review-1 | fixed | audit-m1.md |
| F-12 | Trust judged by the main frame, not the frame that asked (remote iframe in a local page) | Review-1 | carried (M4 trust row) | roadmap open-decisions table |
| F-13 | `webkit_web_view_load_uri` drops method, body and referrer: a POST `target=_blank` form becomes a GET | Review-1, Codex (P2) | fixed with F-30 | `src/browser.c`, roadmap M2-03 note 4 |
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
| F-24 | Defect 2's port-99999 claim is false on GLib 2.90.1 | Review-2 | fixed (doc corrected; extra guard kept as defense in depth) | audit-m1.md |
| F-25 | Remote-to-`file:` new-window rule covered only by a unit test | Review-2 | documented; real click is a guided check | measurements-m1-audit.md |
| F-26 | Trust is read from the current URI only on title change (small race) | Review-2 | carried (M2-03 note 3) | roadmap |
| F-27 | `display_disconnected` calls `_exit(3)` even after a requested shutdown | Review-2 | wontfix: deliberate, matches upstream | `src/browser.c` |
| F-28 | Diagnostics pointer left dangling after free | Review-2 | fixed (set to null) | `src/browser.c` |
| F-29 | `target.h` said "not an M2 policy" while exporting `foob_navigation_allowed()` | Review-2 | fixed (wording) | `src/target.h` |
| F-30 | POST `target=_blank` forms lose their body. Preserve method and payload (use the action's request) or reject such submissions explicitly; never convert silently | Codex (P2) | fixed: `webkit_web_view_load_request` with the action's request. Scratch experiment with a forced user gesture: before `Echo GET 0 bytes`, after `Echo POST 15 bytes`. No automated test (needs a gesture; D-12); guided check row 9 (real click: PASS, POST 15 bytes); evidence re-recorded on `9abb6fe` | `src/browser.c` decide_policy, audit-m1.md defect 5 |
| F-31 | `scripted-new-window-requests-do-not-navigate` passed vacuously: the script ran in the head before `document.body` existed, so it threw before `a.click()` and `window.open()` | Codex (P2) | fixed: the script runs at the end of `<body>` and reports `click-ok,open-null` to the test server (`/probe`); the scenario asserts that report, then the title and one load. Negative control with scratch builds: popup blocker off + gesture check removed makes the scenario fail; blocker off + check kept passes | `tests/nested_lifecycle.py` |
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
| F-42 | Review-3: new-window trust uses the main frame, so a remote iframe in a local page inherits local trust | Review-3 | carried: same as F-12 (D-09, M4); not reachable with today's inputs (local pages are the user's own files, none embeds frames) | roadmap M4 trust row |
| F-43 | Review-3: sanitizer misses invisible and blank characters (U+00AD, U+2800, U+17B4/17B5, U+FFF9-FFFC, U+1D173-1D17A) | Review-3 | **open**: proposed fix now, same class as F-15: strip all Unicode format characters (category Cf, keeping U+200C/D) plus the listed fillers, with unit tests | `src/window_title.c` |
| F-44 | Review-3: local and remote titles share the ` — foob` suffix, so a remote page can imitate a local title | Review-3 | carried: D-01 design plus M2-03 note 2; a trust cue is an M2-12 idea (I-01) | roadmap M2-03 |
| F-45 | Review-3: title trust is recomputed only on title change, not on URI change | Review-3 | carried: F-26, M2-03 note 3, I-03 | roadmap M2-03 |
| F-46 | Review-3: `about:blank` is always trusted, though a document there may inherit an origin | Review-3 | carried: D-09, M4; not exploitable today (navigating a remote page to `about:blank` ends its scripts and no popup exists) | roadmap M4 trust row |
| F-47 | Review-3: `g_uri_get_port(uri) <= UINT16_MAX` is always true (unset port is -1); GLib and WebKit parse URIs differently | Review-3 | **open**: remove the dead check and pin the behaviour with unit tests. Probe: backslash and odd-port URIs differ between parsers only for remote URLs (`http://good.com\.evil.com/`), never in the local/remote classification of a `file:` URL, so no exploitable difference found | `src/target.c` |
| F-48 | Review-3: downloads rely on one cancel handler (no RESPONSE-level ignore); permission defaults are unobserved; nested test scans only for payload bytes | Review-3 | carried: M3-03 (add: ignore non-displayable responses before a download object exists; test for any new file), M3-05 (F-34) | roadmap M3-03, M3-05 |
| F-49 | Review-3: `display_disconnected` calls `_exit(3)` without tearing down helpers | Review-3 | wontfix: same as F-27; the `compositor-lost` scenario checks that every helper exits | `src/browser.c` |
| F-50 | Review-3: `policy_server.py` trusts `Content-Length` (non-numeric or negative values raise or read to EOF) | Review-3 | **open**: test tool on loopback only; clamp and validate | `tests/policy_server.py` |

**Result of F-31:** the earlier claim "scripted requests never reach foob's policy
handler" was unproven because the scenario and the first debug experiment used the same
broken page. Repeated with a page that provably runs (probe report): with default
settings a scripted `window.open` and a programmatic `a.click()` on a `_blank` link
never reach `decide-policy` or `create` (WebKit's popup blocker), so the claim holds.
With `javascript-can-open-windows-automatically` on (scratch build) both reach the
handler with gesture=0 and foob's gesture check blocks them; with the check also removed
the page navigates and the scenario fails. foob's own gesture check therefore has
evidence only from scratch builds, not from a committed test (D-12).

### Carried into later milestones (roadmap lines hold the detail)

| Milestone | Items |
|---|---|
| M2 | M2-03 notes (unify navigation policy and split `browser.c`, sanitizer limits, trust per committed load, POST handling, `%` URLs, handler reachability), M2-10 HTTP auth, M2-11 second window and `window.open`, M2-12 blocked-action feedback |
| M3 | M3-01 popups and `window.opener`, right-click default; M3-02 file input default; M3-03 no silent downloads and downloaded-file trust; M3-04 PDF URL display; M3-05 permission and certificate defaults; M3-07 crash recovery |
| M4 | Local-file trust rule (D-09), remote iframe in a local page |
| M5 | M5-10 JIT-off opt-in, GL renderer measurement, under-100-MiB attribution |
| M6 | M6-07 fractional scaling, multi-monitor, nested suite and size/hardening checks as release gates |

## Ideas parking lot (not tasks yet)

| ID | Idea | Notes |
|---|---|---|
| I-01 | Use the window title as a status channel (`Download blocked — foob`) | Chrome-free; belongs to M2-12 |
| I-02 | Split policy handlers out of `browser.c` into their own module | M2-03 |
| I-03 | Decide trust once per committed load instead of per title change | M2-03, fixes F-26 |
| I-04 | `create` handler so `window.open` can navigate the same window | M2-11 / M3-01; decide with popup policy |
| I-05 | Untrusted-site mode with JIT off | M5-10 |
| I-06 | Hardening-vs-feature split of the +3.5 KB binary growth | measure by building without hardening flags |
| I-08 | A benchmark-only fixture without the "Reload this fixture" button would stop a stray click from invalidating a run (F-36) | changes the fixture hash recorded since M1, so only if it recurs |
| I-07 | Run `desktop_probe.py` on the final binary | done on `d9ea90d` (pass, 2026-10-13); repeat after any C change |
