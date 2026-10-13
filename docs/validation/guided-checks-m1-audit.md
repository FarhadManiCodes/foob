# Guided checks for the M1 audit binary (maintainer at the keyboard)

**Status: pending.** These check behavior changed or added by the M1 audit, which
cannot be automated here because input cannot be synthesized. The nested lifecycle
suite already covers close, signals, crashes, downloads, titles and app-ids; the
original [desktop checklist](desktop-checklist.md) covers resize, typing, scrolling
and fullscreen on the old binary, and the geometry probe was repeated on the audit
binary (below). Record each result in the table, then update this file.

## Remote page (untrusted origin)

```sh
python3 tests/policy_server.py            # prints the URL; keep it running
python3 tests/benchmark/interactive.py --binary build/foob \
    --uri http://127.0.0.1:8765/ --output /tmp/foob-guided-remote.json
```

Choose a new `--output` path for every run. (Running `./build/foob http://127.0.0.1:8765/`
directly also works; it just records nothing.) The page numbers its items like the table
below and lists each item's expected result. **The dark panel at the bottom of the page is
the page's own log**: it confirms that each click registered and shows what `window.open`
and the camera request returned. The terminal prints nothing for these checks except the
server's `GET` lines. In a third terminal, `niri msg windows` shows the window title and
app-id. You do not need to fill the table: tell Claude the item number and what you saw.
There is no reload key; to restart a page, close the window and start it again. Item 9
ends on an "Echo page" that says **PASS** or **FAIL** in large text.

| # | Item on the page | Expected | Result |
|---|---|---|---|
| 1 | New-window link to a remote page | opens in the same window ("Second page"). A forced-gesture scratch test saw this work; this checks a real click | |
| 2 | Ordinary link to a remote page | opens in the same window | |
| 3 | New-window link to a local file | nothing happens | |
| 4 | Ordinary link to a local file | nothing happens | |
| 5 | `window.open()` from a click | nothing happens; the page log says `returned null` (unsupported) | |
| 6 | `window.open()` 5 s after a click | nothing happens; log says `returned null` | |
| 7 | Attachment link | nothing happens; no file in `~/Downloads`; server prints `GET /attachment.bin` | |
| 8 | Link with `download` attribute | nothing happens; no file saved | |
| 9 | Submit a POST form (`target=_blank`) | opens "Echo page" in the same window with a green **PASS** (POST, 15 bytes); a red FAIL (GET, 0 bytes) means the body was lost | **PASS** (maintainer, 2026-10-13, real click): "PASS: the form body arrived (POST, 15 bytes)" |
| 10 | File input (record only, M3-02) | what happens? | |
| 11 | Camera and microphone request (record only, M3-05) | prompt? the page log shows `camera/microphone: <error name>` | |
| 12 | PDF link (record only, M3-04) | what is shown? | Recorded (maintainer, 2026-10-13): the stock engine shows the PDF **inline** in a PDF.js viewer (toolbar with search, zoom, annotate, print, save). Carried to M3-04 |
| 13 | Right-click on the page (record only, M3-01) | context menu shown? | |
| 14 | Window title in `niri msg windows` | `Policy fixture (remote) — foob`; app-id `dev.foob.Browser` | |

## Local page (trusted)

```sh
python3 tests/benchmark/interactive.py --binary build/foob \
    --uri file://$PWD/tests/fixtures/policy-local.html --output /tmp/foob-guided-local.json
```

| # | Item | Expected | Result |
|---|---|---|---|
| 15 | New-window link to another local file | opens in the same window; its link returns to the viewport fixture | |
| 16 | New-window link to a remote page | opens in the same window | |
| 17 | `window.open()` to https://example.org/ from a click | nothing happens; log says `returned null` (unsupported) | |
| 18 | Window title | `Policy fixture (local) — foob` | |

## Optional repeat of the click-position check

`tests/fixtures/pointer.html` compares where you click with the expected centers of
four crosshairs and shows the deltas. It passed on the old binary (within 10 CSS px
after resize and scroll). Nothing in the audit touches input or geometry, so this is
a quick confirmation rather than a requirement:

```sh
python3 tests/benchmark/interactive.py --binary build/foob \
    --uri file://$PWD/tests/fixtures/pointer.html --output /tmp/foob-guided-pointer.json
```

## Geometry probe (no input needed)

`python3 tests/benchmark/desktop_probe.py --binary build/foob --label audit-probe
--uri <viewport fixture URI> --resize` passed on revision `2dfe12d`: viewport
950x1188 at scale 1.0, narrowed to 700x1188, fullscreen 1920x1200 (fullscreen=1),
restored to 700x1188 (the narrowed column width), window closed with exit 0 and no
surviving processes. Screenshots were inspected and deleted. A repeat on the final
revision `9c81515` could not run: `swaylock` was active on the host, the compositor
ignored the resize, fullscreen and close actions, and the probe failed with a close
timeout (it cleaned up its own processes). Source changes after `2dfe12d` touch the
title sanitizer, exit-status handling, a null guard and the build flags, not geometry
or input; re-run the probe once the session is unlocked if you want it on the final
binary.
