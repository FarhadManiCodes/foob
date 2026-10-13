# Guided checks for the M1 audit binary (maintainer at the keyboard)

**Status: all 18 rows were done on 2026-10-13 (maintainer at the keyboard; row 18 read by Claude). D-21 then changed what `target=_blank` does (nothing now, the old rows passed with "opens in this window"), so rows 1, 3, 9, 15 and 16 and the new row 19 need a real click again: see [Re-check after D-21](#re-check-after-d-21) (pending).** These check behavior changed or added by the M1 audit, which
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
should do nothing; if the window changes to an "Echo FAIL" page, foob submitted the form.

| # | Item on the page | Expected | Result |
|---|---|---|---|
| 1 | New-window link to a remote page | nothing happens (D-21). Earlier expectation, now void: opens "Second page" in the same window | Old behaviour **PASS** (maintainer, real click, 2026-10-13); re-check pending |
| 2 | Ordinary link to a remote page | opens in the same window | **PASS** (maintainer, 2026-10-13) |
| 3 | New-window link to a local file | nothing happens | **PASS** (maintainer, 2026-10-13); passed on the old behaviour, re-check pending |
| 4 | Ordinary link to a local file | nothing happens | Page log: "click registered on item 4" (maintainer, 2026-10-13) |
| 5 | `window.open()` from a click | nothing happens; the page log says `returned null` (unsupported) | **PASS** (maintainer, 2026-10-13) |
| 6 | `window.open()` 5 s after a click | nothing happens; log says `returned null` | **PASS** (maintainer, 2026-10-13) |
| 7 | Attachment link | nothing happens; no file in `~/Downloads`; server prints `GET /attachment.bin` | **PASS** (maintainer, 2026-10-13): server printed `GET /attachment.bin 200`; no file in `~/Downloads` (listing checked) |
| 8 | Link with `download` attribute | nothing happens; no file saved | **PASS** (maintainer, 2026-10-13): server printed `GET /plain.bin 200`; no file saved (listing checked) |
| 9 | Submit a POST form (`target=_blank`) | nothing happens; the server prints no `POST /echo` (D-21). A red FAIL page means foob submitted it. Earlier expectation, now void: green PASS (POST, 15 bytes) | Old behaviour **PASS** (maintainer, 2026-10-13, real click): "PASS: the form body arrived (POST, 15 bytes)"; re-check pending |
| 10 | File input (record only, M3-02) | what happens? | Recorded (maintainer, 2026-10-13): **no file picker opens**; the input does nothing. Carried to M3-02 |
| 11 | Camera and microphone request (record only, M3-05) | prompt? the page log shows `camera/microphone: <error name>` | Recorded (maintainer, 2026-10-13): no prompt; page log `camera/microphone: OverconstrainedError`; terminal: "Audio capture was requested but no device was found amongst 0 devices". The engine sees no capture device, so the **permission default is untested** (carried to M3-05) |
| 12 | PDF link (record only, M3-04) | what is shown? | Recorded (maintainer, 2026-10-13): the stock engine shows the PDF **inline** in a PDF.js viewer (toolbar with search, zoom, annotate, print, save). Decided to keep as is (D-19) |
| 13 | Right-click on the page (record only, M3-01) | context menu shown? | Recorded (maintainer, 2026-10-13): **no context menu**. Carried to M3-01 |
| 14 | Window title in `niri msg windows` | `Policy fixture (remote) — foob`; app-id `dev.foob.Browser` | Reported as expected (maintainer, 2026-10-13, "all other act as expected"); also asserted by the nested suite |

## Local page (trusted)

```sh
python3 tests/benchmark/interactive.py --binary build/foob \
    --uri file://$PWD/tests/fixtures/policy-local.html --output /tmp/foob-guided-local.json
```

| # | Item | Expected | Result |
|---|---|---|---|
| 15 | New-window link to another local file | nothing happens (D-21). Earlier expectation, now void: opens "Local link navigation PASS" in the same window | Old behaviour **PASS** (maintainer, 2026-10-13); re-check pending |
| 16 | New-window link to a remote page | nothing happens (D-21). Earlier expectation, now void: opens in the same window | Old behaviour **PASS** (maintainer, 2026-10-13); re-check pending |
| 17 | `window.open()` to https://example.org/ from a click | nothing happens; log says `returned null` (unsupported) | **PASS** (maintainer, 2026-10-13) |
| 18 | Window title | `Policy fixture (local) — foob` | **PASS** (verified by Claude, 2026-10-13: window title `Policy fixture (local) — foob`, app-id `dev.foob.Browser` read from `niri msg -j windows`; the maintainer was unsure; also asserted by the nested suite) |

## Re-check after D-21

The second security review (F-51, F-52) showed that loading a page's `target=_blank`
request from foob drops the asking frame's sandbox and Fetch Metadata. An injected script
that avoided this was tried and then removed (D-21): foob now ignores every new-window
request, so a `target=_blank` link or form does nothing until M2-03. Scripted requests
cannot exercise a real click (the nested scenario `blank-form-does-not-navigate` covers
trusted script submits), so these rows need real clicks on the new binary. Use the
commands above with a new `--output` path; the server terminal now prints the method
(`GET`, `POST`) of each request.

| # | Item | Expected | Result |
|---|---|---|---|
| 1 | New-window link to a remote page | nothing happens; no `GET /second.html` on the server | pending |
| 3 | New-window link to a local file | nothing happens | pending |
| 9 | Submit a POST form (`target=_blank`) | nothing happens; no `POST /echo` on the server | pending |
| 15 | Local page: new-window link to another local file | nothing happens | pending |
| 16 | Local page: new-window link to a remote page | nothing happens | pending |
| 19 | Remote page: link inside the sandboxed frame | the frame says `click registered`; nothing else happens (the window stays on the fixture) | pending |

## Optional repeat of the click-position check

`tests/fixtures/pointer.html` compares where you click with the expected centers of
four crosshairs and shows the deltas. It passed on the old binary (within 10 CSS px
after resize and scroll). Nothing in the audit touches input or geometry, so this is
a quick confirmation rather than a requirement. **Result (maintainer, 2026-10-13, real clicks): all deltas within 10 CSS px, as on the old binary.**

```sh
python3 tests/benchmark/interactive.py --binary build/foob \
    --uri file://$PWD/tests/fixtures/pointer.html --output /tmp/foob-guided-pointer.json
```

## Geometry probe (no input needed)

`python3 tests/benchmark/desktop_probe.py --binary build/foob --label <new label>
--uri <viewport fixture URI> --resize` passed on revision `2dfe12d` and again on
**`d9ea90d`** (2026-10-13, session unlocked): viewport 950x1188 at scale 1.0, narrowed
to 700x1188 (one column), fullscreen 1920x1200 (fullscreen=1, two columns), restored to
700x1188; window closed with exit 0 and no surviving processes (the one remaining entry
is a zombie web process that was already reaped). The narrow and fullscreen
screenshots were inspected (layout correct, title bar shows the CSS size) and deleted.
The earlier failure on `9c81515` was swaylock being active on the host. The probe
ran twice on `d9ea90d`; the second run's result is the one recorded. Since F-57 the probe writes its log, record and screenshots into a new private directory (printed at start; `--output-dir` names one that must not exist yet) instead of fixed `/tmp/foob-<label>*` paths.
