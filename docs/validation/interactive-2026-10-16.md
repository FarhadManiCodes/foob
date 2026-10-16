# M1 interactive desktop checks — 2026-10-16

All ten steps of [desktop-checklist.md](desktop-checklist.md) **pass** on the current
binary, reported step by step as "all pass" by the maintainer. This repeats
[interactive-2026-10-13.md](interactive-2026-10-13.md), which ran on the pre-audit binary.

- Binary: `build/foob`, sha256 `f1c00899…`, the audit binary (the tree is at `4f2163a` plus
  documents and the probe script; no C change).
- Fixture: `tests/fixtures/viewport.html`, SHA256 `c6487daeb9c85879…`, launched with
  `FOOB_DIAGNOSTICS=1`.
- Steps passed: initial page, resize wide to narrow to wide, click position, focus away and
  back, typing, JavaScript counter, local link and return, scrolling, maximize and
  fullscreen, close with the compositor binding.
- Not recorded: no raw process record exists for this run (the monitor script's output
  directory was not found afterwards), so "no surviving process" rests on the nested suite
  and the ten probe runs of the same day, not on this session.
- Not covered: multi-monitor and fractional scale (unavailable, M6-07).
