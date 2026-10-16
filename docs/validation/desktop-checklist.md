# M1 interactive desktop checklist — passed

The maintainer completed the required checks on 2026-10-13 (local time), after
the initial deferral. Rerun on the audit binary, all steps pass: [interactive-m1-audit.md](interactive-m1-audit.md). M1 P0 checks pass; Gate A is ready for
review and has not been approved. No input-automation tools were installed.

```sh
FOOB_DIAGNOSTICS=1 ./build/foob file://$PWD/tests/fixtures/viewport.html
```

Expected: one chrome-free test window; header shows current CSS viewport, DPR,
resize count and scroll position. Log reports the WPE logical viewport/scale.

| Step | Expected observation | Current status |
|---|---|---|
| Initial page | Header/edges complete, no black strips, stale stretch or cropping; CSS viewport equals WPE logical dimensions | Pass — user confirms initial 950×1188/DPR 1 |
| Resize tile wide → narrow → wide | Orange 25vh box and full-width border follow window; breakpoint at 800px; resize count increments | Pass — user confirmation and diagnostic geometry |
| Click orange box after resize | Visible click location agrees with reported `clientX/clientY`; no coordinate offset | Pass — strengthened with separate crosshair target fixture, narrow/wide/scroll hits within ±10 CSS px |
| Focus another app then return | Caret/focus restores, input responds normally | Pass — user confirmation and focus diagnostics |
| Type disposable text | Characters appear at clicked caret, input-event count increases; no input contents in diagnostics | Pass — user confirmation |
| JavaScript counter button | Each click increments counter exactly once, including after resize | Pass — user confirms exact increments after state changes |
| Local navigation link | Separate green page says “Local link navigation PASS”; return link works | Pass — user screenshot and return confirmation |
| Mouse/touchpad scrolling | Ruler moves smoothly, sticky header updates scrollY; orange END footer reachable; scroll offset correct after resize | Pass — user confirmation |
| Supported maximize/fullscreen transitions | Page fits each state and returns to tiled geometry; no stale frame/input mismatch | Pass — niri maximize-column/fullscreen and return; xdg maximized state not asserted |
| Close via compositor binding | Window disappears and diagnostics say `closed`; browser/descendants disappear | Pass — two user closes, exit 0/no tracked survivors |

An automated niri geometry probe supplements these checks but cannot prove
typing, clicking, scrolling or user-perceived responsiveness. No browser shortcut
policy exists yet; use the compositor's existing controls. niri's maximize-column
is a tiling operation, not necessarily the xdg maximized state. Record that
distinction instead of equating the two.

Only the built-in 1920×1200 display at scale 1 was available. Multi-monitor
movement and fractional scale are **not tested**; leave display settings intact.

Optional P1 Scholar continuation: launch `./build/foob https://scholar.google.com/`.
Use the default identity and ephemeral session. Submit one deliberate search,
open a result and scroll. Record immediate access and any challenge; complete a
challenge manually only if available. Do not repeatedly automate searches,
import cookies or change security settings. No persistence claim is possible
until M2. See the M1 record for the initial exploration outcome.

Guided Scholar check on 2026-10-13: user reports the search/result/scroll flow
seems OK with no blocker. Requested manual rerun exposed a small language-menu
glyph issue, deferred to roadmap M3-10. No challenge was reported;
no persistence or long-term reliability claim. See guided session evidence.
