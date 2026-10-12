# M1 interactive desktop checklist — pending

The maintainer chose to save these checks for later on 2026-10-13 (local time).
M1 remains incomplete; Gate A has not been approved. No input-automation tools
were installed. Fill in **pass / fail / not tested** and observations per step.

```sh
FOOB_DIAGNOSTICS=1 ./build/foob file://$PWD/tests/fixtures/viewport.html
```

Expected: one chrome-free test window; header shows current CSS viewport, DPR,
resize count and scroll position. Log reports the WPE logical viewport/scale.

| Step | Expected observation | Current status |
|---|---|---|
| Initial page | Header/edges complete, no black strips, stale stretch or cropping; CSS viewport equals WPE logical dimensions | Not tested by user |
| Resize tile wide → narrow → wide | Orange 25vh box and full-width border follow window; breakpoint at 800px; resize count increments | Not tested by user |
| Click orange box after resize | Visible click location agrees with reported `clientX/clientY`; no coordinate offset | Not tested |
| Focus another app then return | Caret/focus restores, input responds normally | Not tested |
| Type disposable text | Characters appear at clicked caret, input-event count increases; no input contents in diagnostics | Not tested |
| JavaScript counter button | Each click increments counter exactly once, including after resize | Not tested by user |
| Local navigation link | Separate green page says “Local link navigation PASS”; return link works | Not tested |
| Mouse/touchpad scrolling | Ruler moves smoothly, sticky header updates scrollY; orange END footer reachable; scroll offset correct after resize | Not tested |
| Supported maximize/fullscreen transitions | Page fits each state and returns to tiled geometry; no stale frame/input mismatch | Not tested by user |
| Close via compositor binding | Window disappears and diagnostics say `closed`; browser/descendants disappear | Automated close pass; user repeat pending |

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
