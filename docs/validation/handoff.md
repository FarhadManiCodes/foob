# M0/M1 resumable handoff

**M0/M1 P0 complete; Gate A ready for maintainer review, not approved.** Stop before M2.

## Current state (living section: update it with every change)

Updated 2026-10-16. **Branch `m0-m1`, in the main checkout.** The
M1 audit (`audit-m1.md`) was merged into `m0-m1`; the reviews after it (Codex, two
security reviews, a final cleanup review) were handled here, and the evidence was
re-recorded after each C change, last on `147ae45`. The second security review led to
D-04: foob ignores every new-window request, so a `target=_blank` link or form does
nothing until M2-03. Gate A is **not approved**; no M2 work is authorized.

| | |
|---|---|
| Evidence revision | `147ae452…` (full hash in the records): source SHA256 `a7c70db4…`, audit binary `f1c00899…`, control `3369eead…` ([measurements-m1-audit.md](measurements-m1-audit.md)). Later commits change only tests, fixtures and documents, except two comments (2026-10-15, `src/target.h` and `src/browser.c`, line count kept) and the license metadata in `meson.build`: rebuilt in one directory on 2026-10-16, today's tree and `147ae45` give a byte-identical stripped binary, but the source SHA256 is now `ed80ae36…`. Any other C change invalidates these |
| Verified on `147ae45` | Unit tests 3/3, nested suite 12/12, ASan+UBSan 3/3 and 12/12 with no report, analyzer clean, a clean archive build 3/3 with no warning, binary 22,520 B stripped (text 13,061 B). Interleaved memory (25 launches, all passed): blank 170.3 vs 170.0 MiB, fixture 194.0 vs 194.7, Wikipedia 329.8 vs 336.8 (one control launch at 357 MiB): no regression; the under-100-MiB stretch target is not met |
| Test trim (after `147ae45`, binary unchanged) | Reviewed test by test with mutation checks; redundant cases cut. `check_cli.py` 23 launches to 4 (exit codes 1 and 2; the validity rules are unit-tested), `/target/navigation` 17 cases to 8, nested suite 12 scenarios to 8. Removed scenarios: `policy-fixture-remote` and `policy-fixture-local` (only checked that pages load), `sigterm` (four scenarios end with SIGTERM), `scripted-new-window-requests-do-not-navigate` (WebKit's popup blocker stops scripted requests before foob's handler, so it could not fail from foob code; guided rows 5, 6 and 17 cover it). `title-follows-page` became `title-local-unsanitized`. Accepted: the `window_title.c` character table is sampled, not pinned entry by entry. **Run 2026-10-16 on `4f2163a` (binary `f1c00899…`, built from the clean tree): trimmed suite 8/8 pass; geometry probe `final --resize` pass (viewport 950×1188, 700×1188, fullscreen 1920×1200, restored 700×1188, exit 0, no survivors after 10 s).** An earlier probe run in the same session printed a process dump with a zombie `WPEWebProcess` and its result was not read; the rerun passed. Five more probe runs the same day: 2 passed, 3 failed with "exactly one browser window required", a race in the probe (fixed the same day), not a foob fault. The probe now waits up to 10 s for the window; with that fix 10 of 10 runs passed (identical geometry 950×1188, 700×1188, fullscreen 1920×1200, restored 700×1188; exit 0; no survivors; binary `f1c00899…`, run from the working tree with the fixed script). Xdg maximize observed once with `maximize-window-to-edges` (`maximized=1`, 1920×1200, restored, exit 0) |
| Maintainer checks done | All 19 guided rows: 18 clicked by the maintainer (rows 1, 3, 9, 15, 16 and 19 again after the D-04 update on `4e9b6c2`; row 4 recorded only its page log), row 18 read by Claude, the `pointer.html` click check (within 10 CSS px) and the geometry probe on `d9ea90d` ([guided-checks-m1-audit.md](guided-checks-m1-audit.md)). Typing, scrolling, focus and maximize: all ten checklist steps pass on the audit binary ([interactive-m1-audit.md](interactive-m1-audit.md)). The probe attempt on `147ae45` failed (the browser did not close within 10 s of `close-window`) and failed the same way on the previous binary; the screen was probably locked, as in an earlier failure, but no log shows the cause |
| Not verified | Multi-monitor and fractional scaling (unavailable, M6-07). The negative controls for compositor loss, downloads, app-id and web-process death were run during the audit and are recorded in prose, not as logs. Page `window.close()` is untested (WebKit honours it only for script-opened windows) and shares its handler with the compositor close |
| Known consequences of the interim defaults | Cancelled downloads (D-02), ignored new windows (D-04) and inline PDFs (D-12) are deliberate, but the Scholar and PDF flows of M2-08 and M3 will meet them: OAuth popups and `_blank` result links do nothing, and no PDF can be saved |
| Documentation pass (2026-10-15/16; docs, two comments, license metadata) | Register: decisions D-01 to D-15 (old numbers in [findings.md](findings.md#decision-numbering)), findings empty (next F-76), ideas I-01 and I-02; agent rules and the three interim defaults in `AGENTS.md`. M1 records cut to what later work needs: `c23-review.md` and `plans/m0-m1.md` deleted, `m0.md` folded into `m1.md`, `measurements.md` into `measurements-m1-audit.md`, `font-follow-up.md` replaced by roadmap M3-10; this file's pre-audit sections replaced by the evidence index. Architecture and roadmap: decisions cited where they postpone a target, status and history removed, each topic kept in one place (the architecture holds the design, the roadmap the tasks and acceptance tests), roadmap sections unnumbered; machine model and home paths removed before publishing. Git keeps the removed text |
| License (2026-10-15) | BSD-2-Clause (D-15); M0 is complete. Per-file SPDX lines come with the next C change |
| MiniBrowser comparison (2026-10-14, docs only) | foob `src/` compared with upstream MiniBrowser `main.cpp` (D-14); new roadmap tasks M2-13 and M3-09 repeat it. Local copy in `reference-minibrowser/` (git-ignored, may be absent; provenance in [minibrowser-comparison.md](minibrowser-comparison.md)) |
| Raw runs | `docs/validation/runs/m1-audit-147ae45/` (this evidence); the other `m1-audit-*` sets are superseded; the pre-audit M1 runs sit beside them. All ignored by git and kept only on this disk |

**Where to look:** [findings.md](findings.md) (decisions, findings, ideas), the evidence
index below, the roadmap's triage rule (under *Scope control*).

**Open right now**
1. All done 2026-10-16 on `4f2163a` (binary `f1c00899…`): geometry probe, trimmed nested suite, the real session (typing, scrolling, focus, maximize, close), the remote-page guided rows and the `pointer.html` click check: the maintainer confirms all pass.
2. Done 2026-10-16: published at https://github.com/FarhadManiCodes/foob (public, `main` default, `m0-m1` not merged). History was rewritten first to remove the machine details and the raw runs; every commit hash changed, and the documents and the local raw records cite the new ones. The old history is kept privately in a bundle; `docs/validation/runs/` stays local and backed up, since git ignores it.
3. Maintainer: decide Gate A.

**Rules for every agent (Claude, Codex, others)**
- Read this file and [findings.md](findings.md) before anything else; do not repeat checks recorded as done and do not re-argue a decided item.
- Work in the main checkout on the branch the maintainer names. No worktrees, clones or extra folders without asking. Check `git status` and `git branch --show-current`; another agent may be active.
- Never merge or approve Gate A; push only when the maintainer asks.
- Record every new finding, decision and idea in findings.md in the same commit as the change. findings.md says when a finding or idea row is removed; decisions are retired only on the maintainer's request.
- After any C change, re-record the interleaved measurements and update the hashes (about 6 minutes).
- Put a negative control behind each safety fix (a test that fails on the old behaviour) and say plainly what a test does not prove.
- Keep scratch work in the session scratchpad, not in the repository tree.

## Evidence index

- [m1.md](m1.md): M0/M1 acceptance (M0 `ed52f71`, M1 `a92fbc3`), toolchain and
  environment, sandbox and Scholar observations.
- [audit-m1.md](audit-m1.md): defects and their tests, static checks, size and memory
  baselines, WebKit defaults.
- [measurements-m1-audit.md](measurements-m1-audit.md): PSS method and results on the
  evidence revision; compact records in [results-m1-audit.json](results-m1-audit.json)
  (the pre-audit ones in [results.json](results.json)); raw `runs/` stay local.
- [guided-checks-m1-audit.md](guided-checks-m1-audit.md): the 19 guided rows and the
  pointer and geometry checks on the audit binaries.
- [desktop-checklist.md](desktop-checklist.md) and
  [interactive-m1-audit.md](interactive-m1-audit.md): typing, scrolling, focus and
  maximize on the audit binary.
- [blank-memory.md](blank-memory.md): memory and GPU baseline for M5 (why blank exceeds
  100 MiB, where it goes, GPU evidence).
- [minibrowser-comparison.md](minibrowser-comparison.md) (D-14).

Limits to keep visible at Gate A (with the rows above): blank is about 170 MiB against the under-100-MiB stretch
target; GPU use is confirmed for blank, but the GL renderer and per-feature acceleration are
unmeasured; multi-monitor and fractional scaling are untested; missing glyphs are
deferred to M3-10; the Scholar observations are exploratory, not lasting compatibility.
