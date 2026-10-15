# M0/M1 resumable handoff

**M0/M1 P0 complete; Gate A ready for maintainer review, not approved.** Stop before
M2. The sections below "Revision and reproducibility" are the original M0/M1
record and describe the unmodified `m0-m1` binary.

## Current state (living section: update it with every change)

Updated 2026-10-15. **Branch `m0-m1`, in the main checkout.**
The maintainer authorized merging `m1-audit` and deleting the local audit branch.
The audit was fast-forwarded into `m0-m1` through `ced9c6c`; the local audit branch
was deleted. The final review found no actionable regressions; all three registered
Meson tests pass. After the merge, further Codex findings (F-35 to F-40) and a security
review (`/code-review high`, F-41 to F-50) were handled on `m0-m1`; three of the latter
changed C code (F-43 sanitizer, F-47 allowlist), so the evidence was re-recorded on `89c1ecf`; it was re-recorded again on `0af4314` after the D-04 update and on `679fb9a` after the last review fixes and on `147ae45` after the final cleanup review.
A second security review (Review-4, F-51 to F-57) found that foob's own reload of a page's
`target=_blank` request escaped iframe sandboxes (F-51) and reported `Sec-Fetch-Site: none`
(F-52). An injected script that let the page navigate itself fixed both, and two follow-up
reviews found problems in the script. The maintainer then decided the script was
not an M1 issue (D-04): it was removed, and foob ignores every new-window request, so a
`target=_blank` link or form does nothing until M2-03. The low findings F-53 to F-57 (title
sanitizer, benchmark tools) and F-61 (`summarize.py` guard) stay fixed.
Gate A is **not approved**; no M2 work is authorized.

| | |
|---|---|
| Evidence revision | `147ae452…` (full hash in the records): source SHA256 `a7c70db4…`, audit binary `f1c00899…`, control `3369eead…` ([measurements-m1-audit.md](measurements-m1-audit.md)); later commits change only tests, fixtures and documents, except one comment in `src/target.h` (2026-10-15, decision renumbering): the stripped binary built from today's `src/` and from `147ae45` in the same directory is byte-identical, but the source SHA256 of `src/` is now `7f94bd8f…`; any other C change invalidates these |
| After the final cleanup review | Re-recorded on `147ae45` (the review removed duplicate handlers and dead code: text 13,125 to 13,061 B): unit tests 3/3, nested suite 12/12, ASan+UBSan 3/3 and 12/12 with no report, analyzer clean, a clean archive build 3/3 with no warning, binary 22,520 B stripped. Interleaved memory (25 launches, all passed): blank 170.3 vs 170.0 MiB, fixture 194.0 vs 194.7, Wikipedia 329.8 vs 336.8 (one control launch at 357 MiB); one first-launch audit Wikipedia navigation took 1136 ms, the rest 530-545 ms. The geometry probe could not be re-run: it failed the same way (the browser did not close within 10 s after `close-window`) with the previous binary and the previous script, so it is the desktop session state (the earlier failure was a locked screen); repeat it with the session unlocked (I-07) |
| Decision renumbering (docs and one comment) | 2026-10-15, maintainer: the new-window decisions (then D-05 and D-21) were merged into one, the agent rules (then D-11, D-16, D-18) moved to roadmap section 7 and `AGENTS.md`, and every gap was closed, so the decisions now run D-01 to D-14; every citation was updated. Old-to-new table: [findings.md](findings.md#decision-numbering). `AGENTS.md` lists the three interim defaults (downloads D-02, new windows D-04, inline PDFs D-12) in one place |
| Scholar load rule (docs only) | 2026-10-15: Google Scholar may be opened on purpose for a check that needs it (M2-08), at most once or twice a minute, never in automated tests, scripts, benchmarks or loops, so Google does not block the IP. The rule is in `AGENTS.md` (Testing Guidelines). No C change, evidence stays valid |
| MiniBrowser comparison (docs only) | 2026-10-14: foob `src/` compared with upstream MiniBrowser `main.cpp` (D-14): foob is ahead on crash, app-id, title, argument, downloads, new windows and permissions; six upstream items are parked as I-09 to I-14 with owners; engine defaults foob runs with were probed. New roadmap tasks M2-13 and M3-09 repeat it. Local copy in `reference-minibrowser/` (git-ignored via `.git/info/exclude`, may be absent; provenance in its `SOURCE.txt` and in [minibrowser-comparison.md](minibrowser-comparison.md)). No C change, evidence stays valid |
| Test trim (tests only, binary unchanged) | Reviewed test by test (mutation checks of `target.c`, `window_title.c` and `main.c`; no padded test was found, redundant cases were cut). Unit tests: `test_target.c` 112 to 90 lines and `test_window_title.c` 179 to 167 (11 test functions remain; `/target/long-input` removed because a 1 MiB URI cannot reach the validator from `argv`; `/target/navigation` cut from 17 to 8 cases, M2-03 can add more). `tests/check_cli.py`: 23 launches to 4 (exit codes 1 and 2 wiring; the validity rules are unit-tested). Nested suite: 12 to 8 scenarios (`nested_lifecycle.py` 339 to 294 lines; all test files together 1,415 to 1,314). Removed: `policy-fixture-remote` and `policy-fixture-local` (only checked that guided-check pages load), `sigterm` (four other scenarios end with SIGTERM and assert the same), and `scripted-new-window-requests-do-not-navigate` (WebKit's popup blocker stops scripted requests before foob's handler, so it could not fail from foob code; `window.open` returning null stays a guided check, rows 5, 6 and 17). `title-follows-page` became `title-local-unsanitized` (a zero-width space in a local page's title must survive; it pairs with `title-remote-sanitized` and is the only test that foob derives trust from the loaded URI). `download-cancelled` lost a redundant 2 s re-check; `policy_server.py` lost its unused `serve()` helper. The counts above (12/12 scenarios, 23 launches) describe `147ae45`. Known and accepted: the `window_title.c` character table is sampled by the tests, not pinned entry by entry |
| What is verified | See the row above; the PSS comparison shows no regression in memory beyond one single-launch spike; the under-100-MiB stretch target is still not met (blank about 170 MiB) |
| Maintainer checks done | All 18 guided rows (rows 1, 3, 9, 15, 16 and the new row 19 re-clicked after the D-04 update on `4e9b6c2`), the `pointer.html` click-position check (within 10 CSS px) and the geometry probe on `d9ea90d` (2026-10-13, [guided-checks-m1-audit.md](guided-checks-m1-audit.md)) |
| What is not | Multi-monitor and fractional scaling (unavailable here, M6-07); the maintainer's Gate A decision |
| Raw runs | `docs/validation/runs/m1-audit-147ae45/` (this evidence); `m1-audit-679fb9a/`, `m1-audit-0af4314/`, `m1-audit-0af4314-nav/` (five extra Wikipedia pairs), `m1-audit-89c1ecf/`, `m1-audit-93bdf41/`, `m1-audit-9abb6fe/` and `m1-audit-final/` are superseded sets; both ignored by git, the original M1 runs sit beside them, untouched |

**Where to look:** [findings.md](findings.md) (decisions D-xx, findings F-xx, ideas
I-xx), [audit-m1.md](audit-m1.md) (what was found and measured),
[guided-checks-m1-audit.md](guided-checks-m1-audit.md) (maintainer's checklist), the
roadmap section 7 (triage rule).

**Open right now**
1. Maintainer: decide Gate A. Review and merge are complete. The 18 guided checks are done (all pass or recorded); `pointer.html` and the geometry probe were checked on `d9ea90d`.
2. Codex review, handled one by one: **F-30** (POST `target=_blank` forms lose their body) and **F-31** (scripted-popup scenario was vacuous) are both fixed; the claims are re-verified and the evidence was re-recorded (now on `679fb9a`).
3. After any C change: re-record the interleaved measurements and update the hashes (cost about 6 minutes). Done on `147ae45`; due again after the next C change.
4. Done: the real-click re-check after the D-04 update (rows 1, 3, 9, 15, 16, 19) passed on `4e9b6c2`; every `target=_blank` item does nothing and the sandboxed frame stays put ([record](guided-checks-m1-audit.md#re-check-after-the-d-04-update)).
5. Review-4 and later findings: F-51 and F-52 are closed by D-04 (no script, no load on a page's behalf). F-53 holds by construction. F-54 (left-to-right mark before right-to-left remote titles) and F-55 (rest of the default-ignorable set) are in `window_title.c` with unit tests that abort on the old code. F-56, F-61, F-63 and F-64 (`summarize.py`: curated-file guard, run-record validation, dirty flag kept), F-57 and F-65 (`desktop_probe.py` private output directory; every benchmark record carries provenance) are in the benchmark tools. The final PR-style `/code-review high` (F-66 to F-68) led to three small fixes (test cleanup order, partial file read, cut-title trailing space); its other findings were not defects, duplicates or already carried and are retired (see the Retired entries in findings.md). The `target=_blank` policy is M2-03.

**Rules for every agent (Claude, Codex, others)**
- Read this file and [findings.md](findings.md) before anything else; do not repeat checks recorded as done and do not re-argue a decided item.
- Work in the main checkout on the branch the maintainer names. No worktrees, clones or extra folders without asking. Check `git status` and `git branch --show-current`; another agent may be active.
- Never merge, push or approve Gate A. Never add AI-attribution trailers.
- Record every new finding, decision and idea in findings.md in the same commit as the change; add a row, never delete one.
- Put a negative control behind each safety fix (a test that fails on the old behaviour) and say plainly what a test does not prove.
- Keep scratch work in the session scratchpad, not in the repository tree.

## Revision and reproducibility

- Baseline: `cd93192`.
- M0 implementation/evidence: `ed52f71`.
- M1 implementation/evidence: `a92fbc358f10546f845301be1eb367f131ab29dd`.
  Later checkpoints add paired diagnostic logs, clean-export evidence and a
  collector geometry guard; C source and measured binary are unchanged. Resolve
  current documentation head with `git rev-parse HEAD`.
- Source SHA256: `6bf147f1390eef2881a8180f418236e5056373c336e081f71f20cb880995bce2`.
- Measured binary SHA256: `9a9225d877defb9f6c574ea1929653fdbef5355ad8db5ab5dc99e4ee10283229`.
- Guided interaction starts from `39f0e12cf1f3ab62492f2b0a75946209aa054b5d`.
  Completed guided evidence: `6f6a3e95921cc93b70c0b47b4f07a749b49c6911`.
  That commit adds a separate pointer-target fixture and a standard-library
  interactive process monitor. Browser source/binary and benchmark fixture stay
  unchanged; raw records include exact hashes. Resolve latest evidence revision
  with `git log -1 --format='%H' -- docs/validation/interactive-2026-10-13.md`.
- Blank-memory/GPU diagnosis starts from `2626b59`; browser source/binary remain
  unchanged. See [blank-memory.md](blank-memory.md) and [results.json](results.json).
  Generated raw `docs/validation/runs/` artifacts are ignored and kept locally;
  curated results and reproduction scripts are committed.

Read AGENTS.md, root architecture/roadmap, approved plan, and this handoff.
Inspect Git status and existing evidence before resuming; do not repeat completed
checks merely because this is a new context.

## Completed implementation and validation

- C23 Meson skeleton, public symbols linked; installed versions recorded.
- `foob [URI]`: default blank, explicit native Wayland, one view, ephemeral
  session, trusted startup target validation and optional redacted diagnostics.
- Registered startup contract test passes (16 rejected / 7 accepted cases).
- Fresh final configure/build/test passes, no compiler warnings; actual C23 flags
  and independent strict-warning syntax check confirmed.
- Clean Git export of committed M1 `a92fbc3` to `/tmp/foob-m1-clean.EBZFWg`:
  fresh `meson setup build`, compile and registered test all pass with no
  compiler warnings. The export has no ignored local build/profile/agent data.
- Compositor close, SIGTERM, SIGINT: exit 0 and no tracked leftover processes.
- Nine fresh-process PSS launches and one separate local warm-reload control:
  all retained samples 950×1188/scale 1, full browser tree including helpers.
  Final audit replaced one run with transient geometry; the collector rejects
  geometry changes throughout settling, and all retained logs satisfy that check.
- Automatic tile-width/fullscreen geometry and visual fixture inspection; simple
  arithmetic JS and hand-written MathML visually pass.
- Three additional blank attribution launches measure 168.304–171.092 MiB PSS,
  mean 170.143 MiB. Web process dominates (~108.5 MiB), application ~32.7,
  network ~27.5, sandbox helpers ~1.5. Mapping/kernel accounting identifies
  engine/allocator and Mesa/LLVM startup costs; no optimization or source change.
- Hardware GPU participation now confirmed for blank: each web process's amdgpu
  render client has nonzero graphics/compute execution counters. Exact GL renderer
  string and per-feature/every-frame acceleration remain unmeasured.
- Guided user resize/height/focus/typing/click/counter/navigation/scroll/maximize-
  column/fullscreen checks pass. Precise crosshair clicks after resize/scroll
  land within ±10 CSS px of expected centers. Four user closes exit 0 without
  tracked survivors. See [interactive evidence](interactive-2026-10-13.md).
- Scholar guided search/result/scroll exploration reports no blocker or challenge.
  A requested repeat exposed a small language-menu glyph issue, explicitly
  deferred by the maintainer. Persistence/long-term reliability remain untested.
- Fresh read-only reviewer found no source/ownership/API/C23 blocking defect;
  tooling findings fixed, negative control tested, focused follow-up clears them.
- Agent-generated transient test-window captures inspected and deleted. The
  maintainer-supplied narrow language-menu image is retained with the deferred
  font issue; the supplied linked-page image remains conversation evidence.
  No input tools/packages installed or desktop/lid/power/idle settings changed.

## Deferred work, review boundary and limitations

- All required M1 P0 checks pass; [desktop checklist](desktop-checklist.md) is
  complete. Gate A requires an explicit maintainer decision before any M2 work.
- Graphics: hardware GPU participation confirmed for blank; exact GL renderer
  and per-feature/every-frame acceleration not measured. Maintainer confirms no
  second monitor; multi-monitor checks unavailable. Fractional scale untested.
- Font-language menu glyphs: [P1 follow-up](font-follow-up.md) with supplied image;
  no cause established or fix attempted, per the maintainer's request.
- Observed Scholar exploration is not sustained compatibility; persistent-profile
  and restart tests remain M2, and longer-term observations remain later work.
- **M0-05 P1:** license decision deferred to maintainer; no license invented.
- Accessibility warnings persist because the system a11y bus is masked. No
  system configuration change attempted.
- Under-100-MiB stretch target not met: mean PSS blank 174.318 MiB, fixture
  202.412 MiB, Wikipedia 333.708 MiB. No custom-engine work or security weakening.
  Additional blank attribution mean 170.143 MiB is a later diagnostic observation,
  not a replacement baseline or evidence of a memory improvement.
- Blank/Wikipedia warm reloads untested; local warm reload measured separately.
- Full sandbox/TLS/resource-policy audit remains later-milestone scope. Observed
  stock web process has seccomp/new namespaces; network/UI confinement differs.

No unresolved blocking independent-review finding or known persistent process
leak. The deferred font observation does not block Gate A. No M2 work started.

## Next action

Review the completed runtime, measurement, independent-review and interactive
evidence for the maintainer's **Gate A** decision. Keep the font follow-up and
explicit graphics/display/memory limits visible in that decision.
Do not add M2 controls, profiles, IPC, converters or custom engine beforehand.

The decision is whether to accept the measured stock-WPE prototype as a sound
baseline and authorize M2, or require a specific M1 correction first. Evidence
covers C23 build/API review, native rendering/input/window behavior, clean
shutdown, three-workload process-tree PSS and exploratory Scholar behavior.
The >100-MiB measurements, deferred font issue, limits of GPU verification and
untested display-scale cases remain disclosed limitations, not pending repeats
of the desktop checks already passed. A question about Gate A is not approval.

Evidence index: [M0](m0.md), [M1](m1.md), [measurements](measurements.md),
[independent review](c23-review.md), [curated results](results.json),
[blank memory/GPU diagnosis](blank-memory.md), local ignored raw `runs/`,
[guided interaction](interactive-2026-10-13.md), [font follow-up](font-follow-up.md),
[approved implementation plan](../plans/m0-m1.md).
