# M0/M1 resumable handoff

2026-10-13, branch `m0-m1`. **M0/M1 P0 complete; Gate A ready for maintainer
review, not approved.** Maintainer returned and completed guided interaction.
Stop before M2.

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
- Graphics: Wayland and Mesa EGL/render-device evidence available; end-to-end
  acceleration remains unverified. Multi-monitor/fractional scale unavailable.
- Font-language menu glyphs: [P1 follow-up](font-follow-up.md) with supplied image;
  no cause established or fix attempted, per the maintainer's request.
- Observed Scholar exploration is not sustained compatibility; persistent-profile
  and restart tests remain M2, and longer-term observations remain later work.
- **M0-05 P1:** license decision deferred to maintainer; no license invented.
- Accessibility warnings persist because the system a11y bus is masked. No
  system configuration change attempted.
- Under-100-MiB stretch target not met: mean PSS blank 174.318 MiB, fixture
  202.412 MiB, Wikipedia 333.708 MiB. No custom-engine work or security weakening.
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
The >100-MiB measurements, deferred font issue, unverified acceleration and
untested display-scale cases remain disclosed limitations, not pending repeats
of the desktop checks already passed. A question about Gate A is not approval.

Evidence index: [M0](m0.md), [M1](m1.md), [measurements](measurements.md),
[independent review](c23-review.md), [raw runs](runs/),
[guided interaction](interactive-2026-10-13.md), [font follow-up](font-follow-up.md),
[approved implementation plan](../plans/m0-m1.md).
