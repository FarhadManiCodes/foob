# M0/M1 resumable handoff

2026-10-13, branch `m0-m1`. **M0 P0 complete; M1 incomplete; Gate A not approved.**
Maintainer chose to save desktop interaction for later. Stop before M2.

## Revision and reproducibility

- Baseline: `cd93192`.
- M0 implementation/evidence: `ed52f71`.
- M1 implementation/evidence: `a92fbc358f10546f845301be1eb367f131ab29dd`.
  Later checkpoints add paired diagnostic logs, clean-export evidence and a
  collector geometry guard; C source and measured binary are unchanged. Resolve
  current documentation head with `git rev-parse HEAD`.
- Source SHA256: `6bf147f1390eef2881a8180f418236e5056373c336e081f71f20cb880995bce2`.
- Measured binary SHA256: `9a9225d877defb9f6c574ea1929653fdbef5355ad8db5ab5dc99e4ee10283229`.

Read AGENTS.md, root architecture/roadmap, approved plan, and this handoff.
Inspect Git status and existing evidence before resuming; do not repeat completed
checks merely because this is a new context.

## Completed independent work

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
- One Scholar query rendered results without an observed challenge. User
  form/result/scroll interactions and persistence remain untested.
- Fresh read-only reviewer found no source/ownership/API/C23 blocking defect;
  tooling findings fixed, negative control tested, focused follow-up clears them.
- Test-window captures inspected and deleted; no input tools/packages installed
  and no desktop/lid/power/idle settings changed.

## Outstanding acceptance and limitations

- **M1-04 / M1-05 P0:** user typing, pointer coordinate alignment after resize,
  link navigation, mouse scrolling, focus transitions and supported maximize/
  fullscreen interactions. Resume [desktop-checklist.md](desktop-checklist.md).
- Graphics: Wayland and Mesa EGL/render-device evidence available; end-to-end
  acceleration remains unverified. Multi-monitor/fractional scale unavailable.
- **M1-09 P1:** search-form submit/result opening/scrolling/challenge observation
  with user interaction. Current single-query access is not sustained compatibility.
- **M0-05 P1:** license decision deferred to maintainer; no license invented.
- Accessibility warnings persist because the system a11y bus is masked. No
  system configuration change attempted.
- Under-100-MiB stretch target not met: mean PSS blank 174.318 MiB, fixture
  202.412 MiB, Wikipedia 333.708 MiB. No custom-engine work or security weakening.
- Blank/Wikipedia warm reloads untested; local warm reload measured separately.
- Full sandbox/TLS/resource-policy audit remains later-milestone scope. Observed
  stock web process has seccomp/new namespaces; network/UI confinement differs.

No unresolved blocking independent-review finding or known persistent process
leak. M1 remains incomplete because required user interaction is outstanding.

## Next action

Launch the trusted fixture with `FOOB_DIAGNOSTICS=1 ./build/foob
file://$PWD/tests/fixtures/viewport.html`, guide the checklist,
record pass/fail and actual geometry. Fix defects within M1, rerun affected
checks, then submit complete evidence for the maintainer's **Gate A** decision.
Do not add M2 controls, profiles, IPC, converters or custom engine beforehand.

Evidence index: [M0](m0.md), [M1](m1.md), [measurements](measurements.md),
[independent review](c23-review.md), [raw runs](runs/),
[approved implementation plan](../plans/m0-m1.md).
