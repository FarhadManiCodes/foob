# Findings, decisions and ideas register

The single place for what was found, what was decided and what is parked. Every
agent and the maintainer read it with [handoff.md](handoff.md) before starting work,
and update it **in the same commit** as the change it describes. Add a row; do not
delete one. Change its status and link the commit.

- **Status:** `open` (needs work), `fixed` (commit given), `carried` (belongs to a later
  milestone; the roadmap line is named), `decided` (maintainer choice, do not
  re-argue), `deferred` (maintainer postponed), `wontfix` (reason given).
- **Source:** `Maintainer`, `Claude` (the implementing agent), `Codex`, `Review-1`
  (Opus 5.5 merge review), `Review-2` (Sonnet 5.5 merge review).
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
| F-30 | POST `target=_blank` forms lose their body. Preserve method and payload (use the action's request) or reject such submissions explicitly; never convert silently | Codex (P2) | fixed: `webkit_web_view_load_request` with the action's request. Scratch experiment with a forced user gesture: before `Echo GET 0 bytes`, after `Echo POST 15 bytes`. No automated test (needs a gesture; D-12); guided check row 9; evidence needs re-recording once F-31 is done | `src/browser.c` decide_policy, audit-m1.md defect 5 |
| F-31 | `scripted-new-window-requests-do-not-navigate` passed vacuously: the script ran in the head before `document.body` existed, so it threw before `a.click()` and `window.open()` | Codex (P2) | fixed: the script runs at the end of `<body>` and reports `click-ok,open-null` to the test server (`/probe`); the scenario asserts that report, then the title and one load. Negative control with scratch builds: popup blocker off + gesture check removed makes the scenario fail; blocker off + check kept passes | `tests/nested_lifecycle.py` |
| F-32 | The guided checks were hard to follow: the log was off-screen at the bottom of the page, clicks that do nothing could not be told from clicks that did not register, row 9 gave no clear verdict | Maintainer | fixed: fixed log panel, click registration lines, item numbers match the checklist, Echo page shows PASS or FAIL; fixtures only, no C change | `tests/fixtures/policy-*.html`, `tests/policy_server.py`, guided-checks-m1-audit.md |

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
| I-07 | Run `desktop_probe.py` on the final binary | blocked while the host session is locked |
