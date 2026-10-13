# M1 guided desktop validation — 2026-10-13

Required M1 desktop checks **pass**, confirmed by the maintainer. Scholar
interaction reports no blocker; a small font issue is explicitly deferred.
Gate A is ready for review, not approved.
Gate A is not approved. Source and measured binary are unchanged from review.

Starting revision: `39f0e12cf1f3ab62492f2b0a75946209aa054b5d`.
Fixture: `tests/fixtures/viewport.html`, SHA256
`c6487daeb9c858798aa83e44fef15e82e31629a728d8239ce04a7d4c699d02b9`.
Launch: `FOOB_DIAGNOSTICS=1 ./build/foob file://$PWD/tests/fixtures/viewport.html`.
The standard-library interactive monitor records only application diagnostics
and process identities; it does not record input contents or inject input.

Environment rechecked: kernel `7.2.9-arch1-1`, WPE `2.54.1-2`, niri `26.04-1`,
Mesa `1:26.2.4-1`; one built-in output, 1920×1200 physical/logical, scale 1.0.
Raw session: `runs/user-viewport-20261013.json` and paired `.log`.

Diagnostic observations so far: native `WPEDisplayWayland`, ephemeral session,
successful initial load in 52.807 ms; viewport widths 631/950/1269/1908 with
height 1188, fullscreen 1920×1200/fullscreen=1 and return to 950×1188/fullscreen=0.
Scale stayed 1.0. These establish compositor/view responses, not user input
alignment or visible acceptance. Existing a11y-bus warnings persist.

| Check | User result |
|---|---|
| Initial CSS viewport and complete borders | Pass — maintainer confirms 950×1188/DPR 1 and complete borders |
| Responsive resize narrow/wide, correct orange box | Pass — maintainer confirms one/two-column switching and box follows resize |
| Typing/caret/input-event counter | Pass — maintainer confirms characters at caret and increasing input count |
| Pointer alignment after resize | Pass — maintainer confirms center hits within ±10 CSS px on both axes at narrow/wide sizes and after scrolling |
| Focus other app then return | Pass — maintainer reports pass |
| JavaScript counter click | Pass — maintainer confirms exactly one increment per click, including after state changes |
| Local link and return | Pass — supplied screenshot shows correct linked page; maintainer confirms return works |
| Mouse/touchpad scroll to footer | Pass — maintainer reports pass |
| Maximize/fullscreen visible geometry/input | Pass — maintainer confirms page fits, no black strips/stale stretch, typing/clicking work and tiled restoration works |
| User compositor close and tracked subprocess cleanup | Pass — user closes through niri; `closed` signal, exit 0, no tracked survivors |
| Scholar form/search/result/scroll exploration | Pass for reported exploratory flow — user reports no blocker; font-menu glyph issue deferred |

Maintainer asked how to verify pointer alignment more conclusively. Added a
separate `pointer.html` target fixture: expected client coordinates from DOM
layout and actual pointer-event coordinates, displayed deltas, center-hit counts,
resize/scroll controls. This leaves the benchmark fixture/checksum unchanged.
The supplied linked-page screenshot was inspected in the conversation, without
copying it into the repository; the pale green background matches `#eff8ee`.

The user also changed tile height (observed 391/591/790/1188) with width changes
and focus transitions. Scale remains 1.0. niri maximize-column expands to 1908
pixels while WPE reports maximized=0; this is tested compositor tiling behavior,
not proof of the xdg maximized state. Fullscreen=1 appears at 1920×1200.

Precise pointer session: `runs/user-pointer-20261013.json` and paired `.log`.
Pointer fixture SHA256:
`d68dc515e0265baa3a70a93182a77c1db2448acf46fa5e9beb0179656a266dad`.
Expected positions come from target DOM rectangles; actual positions are
`clientX/clientY` from user pointer events. A deliberately aimed center click
within ±10 CSS px on both axes is labelled CENTER HIT. This is a bounded manual
alignment check, not a renderer-acceleration measurement. The monitor records
only engine diagnostics and process identities, not click coordinates/input text.

Pointer window also closed through niri: exit 0, `closed` signal and no tracked
browser survivors. No user input contents are retained. The original benchmark
fixture and C source are unchanged, so prior measurements/review remain applicable.

Scholar session: `runs/user-scholar-20261013.json` and paired `.log`; native
Wayland, explicit ephemeral session/default identity, initial home load succeeds
in 512.316 ms. The user reports no blocker for the guided search/result/scroll
flow, then requests reopening for an additional check. Window closed normally,
exit 0, no tracked survivors. No challenge or manual clearance was reported.
This is the observed exploratory flow, not persistence or long-term reliability.

Requested repeat: `runs/user-scholar-20261013-repeat.json` and paired `.log`,
a separate fresh ephemeral launch. The agent does not automate any searches.
The maintainer reports “seems ok” and identifies a small font issue to document
for later work. The supplied screenshot shows square missing-glyph placeholders
in the language list; [font-follow-up.md](font-follow-up.md) preserves the image,
observed limits and a deferred investigation. No challenge or other blocker was
reported. Repeat window closed through niri: exit 0 and no tracked survivors.
All four guided launches have clean lifecycle records. No C source changes,
font changes, input automation or M2 implementation were made during this session.
