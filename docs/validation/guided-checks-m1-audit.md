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

Choose a new `--output` path for every run. The page lists each item with its
expected result. In a third terminal, `niri msg windows` shows the window title and
app-id while the page is open.

| # | Item on the page | Expected | Result |
|---|---|---|---|
| 1 | New-window link to a remote page | opens in the same window ("Second page"). If nothing happens, say so: foob's handler may not be reached by real clicks (scripted requests never reach it) | |
| 2 | Ordinary link to a remote page | opens in the same window | |
| 3 | New-window link to a local file | nothing happens | |
| 4 | Ordinary link to a local file | nothing happens | |
| 5 | `window.open()` from a click | opens "Second page" in the same window (same caveat as row 1) | |
| 6 | `window.open()` 5 s after a click | nothing happens; log says `returned null` | |
| 7 | Attachment link | nothing happens; no file in `~/Downloads`; server prints `GET /attachment.bin` | |
| 8 | Link with `download` attribute | nothing happens; no file saved | |
| 9 | File input (record only, M3-02) | what happens? | |
| 10 | Camera and microphone request (record only, M3-05) | prompt? log shows a permission error | |
| 11 | PDF link (record only, M3-04) | what is shown? | |
| 12 | Right-click on the page (record only, M3-01) | context menu shown? | |
| 13 | Window title in `niri msg windows` | `Policy fixture (remote) — foob`; app-id `dev.foob.Browser` | |

## Local page (trusted)

```sh
python3 tests/benchmark/interactive.py --binary build/foob \
    --uri file://$PWD/tests/fixtures/policy-local.html --output /tmp/foob-guided-local.json
```

| # | Item | Expected | Result |
|---|---|---|---|
| 14 | New-window link to another local file | opens in the same window; its link returns to the viewport fixture | |
| 15 | New-window link to a remote page | opens in the same window | |
| 16 | `window.open()` to https://example.org/ from a click | opens in the same window | |
| 17 | Window title | `Policy fixture (local) — foob` | |

## Optional repeat of the click-position check

`tests/fixtures/pointer.html` compares where you click with the expected centers of
four crosshairs and shows the deltas. It passed on the old binary (within 10 CSS px
after resize and scroll). Nothing in the audit touches input or geometry, so this is
a quick confirmation rather than a requirement:

```sh
python3 tests/benchmark/interactive.py --binary build/foob \
    --uri file://$PWD/tests/fixtures/pointer.html --output /tmp/foob-guided-pointer.json
```

## Already repeated on the audit binary (no input needed)

`python3 tests/benchmark/desktop_probe.py --binary build/foob --label audit-probe
--uri <viewport fixture URI> --resize` on revision `2dfe12d`: pass. Viewport
950x1188 at scale 1.0, narrowed to 700x1188, fullscreen 1920x1200 (fullscreen=1),
restored to 700x1188 (the narrowed column width), window closed with exit 0 and no
surviving processes. Screenshots were inspected and deleted.
