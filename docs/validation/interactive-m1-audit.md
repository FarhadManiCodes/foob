# M1 interactive desktop checks (audit binary)

All ten steps of [desktop-checklist.md](desktop-checklist.md) **pass** on the audit binary,
reported step by step as "all pass" by the maintainer. They replace the first run on the
pre-audit binary, whose record was removed (git keeps it).

- Binary: `build/foob`, sha256 `f1c00899…`, the audit binary (no C change since).
- Fixture: `tests/fixtures/viewport.html`, SHA256 `c6487daeb9c85879…`, launched with
  `FOOB_DIAGNOSTICS=1`.
- Steps passed: initial page, resize wide to narrow to wide, click position, focus away and
  back, typing, JavaScript counter, local link and return, scrolling, maximize and
  fullscreen, close with the compositor binding.
- xdg maximized state: tiling's maximize-column keeps `maximized=0`; niri's
  `maximize-window-to-edges` gives `maximized=1` at 1920×1200 and restores to 950×1188
  ([m1.md](m1.md#environment)).
- Not recorded: no raw process record exists for this run (the monitor script's output
  directory was not found afterwards), so "no surviving process" rests on the nested suite
  and the ten probe runs, not on this session.
- Not covered: multi-monitor and fractional scale (unavailable, M6-07).
- Scholar: the exploratory observations are in [m1.md](m1.md#observations-later-milestones-start-from);
  the missing-glyph screenshot is `images/scholar-language-glyphs.png` (roadmap M3-10).
