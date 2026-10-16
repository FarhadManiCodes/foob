# Guided checks for the M1 audit binary (maintainer at the keyboard)

**All rows were done on 2026-10-13; the rows D-04 changed (1, 3, 9, 15, 16, 19) were
clicked again on `4e9b6c2` and pass.** These check behaviour the audit changed or added
that needs real input, which cannot be synthesized here. The nested suite covers close,
signals, crashes, downloads, titles and app-ids; typing, scrolling and fullscreen were
checked on the pre-audit binary ([desktop-checklist.md](desktop-checklist.md)).

## Remote page (untrusted origin)

```sh
python3 tests/policy_server.py            # prints the URL; keep it running
python3 tests/benchmark/interactive.py --binary build/foob \
    --uri http://127.0.0.1:8765/ --output /tmp/foob-guided-remote.json
```

Choose a new `--output` path for every run (`./build/foob URL` also works but records
nothing). The page numbers its items as below. **The dark panel at the bottom is the page's
own log**: it confirms each click and shows what `window.open` and the camera request
returned. The server terminal prints each request's method and path; `niri msg windows`
shows the title and app-id. There is no reload key: close the window and start again.

| # | Item on the page | Expected | Result (maintainer, real clicks) |
|---|---|---|---|
| 1 | New-window link to a remote page | nothing happens; no `GET /second.html` (D-04) | **PASS** on `4e9b6c2` |
| 2 | Ordinary link to a remote page | opens in the same window | **PASS** |
| 3 | New-window link to a local file | nothing happens | **PASS** on `4e9b6c2` |
| 4 | Ordinary link to a local file | nothing happens | Page log "click registered on item 4" (no result beyond that was recorded) |
| 5 | `window.open()` from a click | nothing happens; log says `returned null` | **PASS** |
| 6 | `window.open()` 5 s after a click | nothing happens; log says `returned null` | **PASS** |
| 7 | Attachment link | nothing happens; no file in `~/Downloads` (D-02) | **PASS**: server printed `GET /attachment.bin 200`, no file (listing checked) |
| 8 | Link with `download` attribute | nothing happens; no file saved | **PASS**: server printed `GET /plain.bin 200`, no file (listing checked) |
| 9 | Submit a POST form (`target=_blank`) | nothing happens; no `POST /echo`; a red "Echo FAIL" page would mean foob submitted it | **PASS** on `4e9b6c2` |
| 10 | File input | record what happens | No file picker opens (M3-02) |
| 11 | Camera and microphone request | record whether a prompt appears | No prompt; log `OverconstrainedError`: no capture device here, so the permission default is untested (M3-05) |
| 12 | PDF link | record what is shown | Shown inline in PDF.js (D-12, M3-04) |
| 13 | Right-click on the page | record whether a menu appears | No context menu (M3-01) |
| 14 | Window title | `Policy fixture (remote) — foob`, app-id `dev.foob.Browser` | **PASS** |
| 19 | Link inside the sandboxed frame | the frame says `click registered`; the window stays on the fixture | **PASS** on `4e9b6c2` |

## Local page (trusted)

```sh
python3 tests/benchmark/interactive.py --binary build/foob \
    --uri file://$PWD/tests/fixtures/policy-local.html --output /tmp/foob-guided-local.json
```

| # | Item | Expected | Result (maintainer, real clicks) |
|---|---|---|---|
| 15 | New-window link to another local file | nothing happens (D-04) | **PASS** on `4e9b6c2` |
| 16 | New-window link to a remote page | nothing happens (D-04) | **PASS** on `4e9b6c2` |
| 17 | `window.open()` to https://example.org/ from a click | nothing happens; log says `returned null` | **PASS** |
| 18 | Window title | `Policy fixture (local) — foob` | **PASS** (read by Claude from `niri msg -j windows`, with app-id `dev.foob.Browser`) |

The `4e9b6c2` run (clean tree, binary `6e650bf4…`): both windows exited 0 with no surviving
process. The server log shows one `GET /second.html` (row 2, the control), no `POST /echo`,
and the requests of rows 7, 8 and 12, clicked again (downloads stayed cancelled). Raw
records stay local.

**Rerun 2026-10-16 on `4f2163a` (same binary `f1c00899…`):** the maintainer confirms the remote page rows and the click-position check pass.

## Click position

`tests/fixtures/pointer.html` shows how far each click lands from four crosshair centres.
**Result (2026-10-13, real clicks): all within 10 CSS px after resize and scroll.**

```sh
python3 tests/benchmark/interactive.py --binary build/foob \
    --uri file://$PWD/tests/fixtures/pointer.html --output /tmp/foob-guided-pointer.json
```

## Geometry probe (no input needed, screen unlocked)

```sh
python3 tests/benchmark/desktop_probe.py --binary build/foob \
    --uri file://$PWD/tests/fixtures/viewport.html --label <new label> --resize
```

It writes its log, record and screenshots into a new private directory, printed at start
(`--output-dir` names one that must not exist yet). **Last passes: `d9ea90d`, 2026-10-13, then on the audit binary 10 of 10 after the probe learned to wait for the window (a fast load ended before niri listed it)**:
950x1188 at scale 1.0, narrowed to 700x1188 (one column), fullscreen 1920x1200 (two
columns), restored to 700x1188; exit 0 and no surviving process; the screenshots were
inspected and deleted. A locked screen makes it fail.
