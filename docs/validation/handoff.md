# M0/M1 resumable handoff

**M0/M1 P0 complete; Gate A ready for maintainer review, not approved.** Stop before
M2. The sections below "Revision and reproducibility" are the original M0/M1
record and describe the unmodified `m0-m1` binary.

## Current state (living section: update it with every change)

Updated 2026-10-13. **Branch `m0-m1`, in the main checkout.**
The maintainer authorized merging `m1-audit` and deleting the local audit branch.
The audit was fast-forwarded into `m0-m1` through `ced9c6c`; the local audit branch
was deleted. The final review found no actionable regressions; all three registered
Meson tests pass. After the merge, further Codex findings (F-35 to F-40) and a security
review (`/code-review high`, F-41 to F-50) were handled on `m0-m1`; three of the latter
changed C code (F-43 sanitizer, F-47 allowlist), so the evidence was re-recorded on `89c1ecf`.
A second security review (Review-4, F-51 to F-58) found that foob's own reload of a page's
`target=_blank` request escaped iframe sandboxes (F-51) and reported `Sec-Fetch-Site: none`
(F-52); the maintainer asked for the two medium findings to be fixed. The page now makes that
navigation itself (injected script, `_blank` to `_top`) and foob ignores every remaining
new-window request. The low findings F-53 to F-57 were then fixed as well (title sanitizer and benchmark tools). Both change C code, so the evidence below must be re-recorded.
Gate A is **not approved**; no M2 work is authorized.

| | |
|---|---|
| Evidence revision | `89c1ecf280555a00bb3f8f8462a9f1f5cf5174bc`: source SHA256 `d306ff47…`, audit binary `dc22e2c2…`, control `3369eead…` ([measurements-m1-audit.md](measurements-m1-audit.md)); later commits change only tests, fixtures and documents; any C change invalidates these |
| After F-51 to F-57 | Unit tests 3/3 (new title tests for F-54/F-55) and nested suite 12/12 (new scenario `blank-form-navigates-as-page`, which fails on the pre-fix binary) on the fix; scratch-build experiments for both findings in findings.md. Memory, sanitizer and analyzer evidence: **to be re-recorded on the fix commit** |
| What is verified | Unit tests 3/3; nested suite 11/11; ASan+UBSan and `-fanalyzer` clean (engine-owned leaks suppressed); interleaved memory comparison shows no regression (blank 167.3 vs 166.2 MiB, fixture 196.0 vs 194.5, Wikipedia 326.2 vs 327.4; all 25 runs passed); binary 22,520 B stripped (unchanged) |
| Maintainer checks done | All 18 guided rows, the `pointer.html` click-position check (within 10 CSS px) and the geometry probe on `d9ea90d` (2026-10-13, [guided-checks-m1-audit.md](guided-checks-m1-audit.md)) |
| What is not | Multi-monitor and fractional scaling (unavailable here, M6-07); the maintainer's Gate A decision |
| Raw runs | `docs/validation/runs/m1-audit-89c1ecf/` (this evidence); `m1-audit-9abb6fe/` and `m1-audit-final/` are superseded sets; both ignored by git, the original M1 runs sit beside them, untouched |

**Where to look:** [findings.md](findings.md) (decisions D-xx, findings F-xx, ideas
I-xx), [audit-m1.md](audit-m1.md) (what was found and measured),
[guided-checks-m1-audit.md](guided-checks-m1-audit.md) (maintainer's checklist), the
roadmap section 7 (triage rule).

**Open right now**
1. Maintainer: decide Gate A. Review and merge are complete. The 18 guided checks are done (all pass or recorded); `pointer.html` and the geometry probe were checked on `d9ea90d`.
2. Codex review, handled one by one: **F-30** (POST `target=_blank` forms lose their body) and **F-31** (scripted-popup scenario was vacuous) are both fixed; the claims are re-verified and the evidence was re-recorded (now on `89c1ecf`, after the security-review fixes F-43, F-47 and F-50).
3. After any C change: re-record the interleaved measurements and update the hashes (cost about 6 minutes). **Due now** for the F-51/F-52 fix.
4. Maintainer: real clicks on guided rows 1, 3, 9, 15, 16 and the new row 19 ([re-check section](guided-checks-m1-audit.md#re-check-after-f-51f-52)); the new mechanism cannot be driven by script.
5. Review-4 low findings: all fixed. F-53 by the F-51 change; F-54 (left-to-right mark before right-to-left remote titles) and F-55 (rest of the default-ignorable set) in `window_title.c` with unit tests that abort on the old code; F-56 (`summarize.py` curated-file guard) and F-57 (`desktop_probe.py` writes into a new private directory) in the benchmark tools. F-58 (named targets and `form.submit()` no longer navigate) is carried to M2-03.

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
