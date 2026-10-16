# Comparison with upstream MiniBrowser (WPE)

The baseline for roadmap tasks M2-13 and M3-09 (D-14): foob next to MiniBrowser
(`Tools/MiniBrowser/wpe/main.cpp`), the engine team's own minimal browser on the same API,
taken at the end of M1 (2026-10-14, WPE WebKit 2.54.1).

**Where the source is.** A local copy is in `reference-minibrowser/` (git-ignored through
`.git/info/exclude`, so it may be absent). Fetch it again from the 2.54.1 release
tarball (`https://wpewebkit.org/releases/wpewebkit-2.54.1.tar.xz`, file
`Tools/MiniBrowser/wpe/main.cpp`, SHA256 of the file
`1cb7d78673bcfe832f8a58d47a57a6b7f09fc5dc0ac91ed1e84a5c1364cd7dbf`), into a new empty
directory, and never build or run it. Two-clause BSD license, copyright Igalia.

## Size (a signal, not a target; `AGENTS.md`)

Non-blank, non-comment-start lines, crude filter. foob `src/`: 432 code lines (C, five
headers included), 22,520 B stripped. MiniBrowser `main.cpp`: 690 code lines (838
raw), which holds two backends (libwpe and WPEPlatform) and needs
`Tools/WPEToolingBackends` for the legacy one (not counted); 26,896 B stripped. The engine dominates memory (about 170 MiB blank
process-tree PSS) for any shell.

## Where foob is ahead at M1

| Area | MiniBrowser | foob |
|---|---|---|
| Web-process death | no handler | exit 4, tested |
| Wayland app-id | none set (`org.webkit.app-<hash>`) | `dev.foob.Browser` |
| Window title | page title unchecked | sanitized for remote pages, tested |
| Startup argument | any string is loaded | allowlist, rejected input never echoed |
| Downloads, new windows | not handled | cancelled / ignored, tested |
| Permissions | **grants every request** and prints it | none handled: WebKit denies |
| Structure | one file, `#if` throughout, not unit-testable | pure logic (`target.c`, `window_title.c`) split from wiring, unit-tested |

## What MiniBrowser does that foob does not (owning task)

| # | Item | What it does | Owner |
|---|---|---|---|
| 1 | Key handling | connects to `WPEView::event`, tests `WPE_EVENT_KEYBOARD_KEY_DOWN`, modifiers and keyval, returns TRUE to consume (Ctrl+Q, Ctrl+R, Alt+Left/Right, F11, Escape) | M2-01, M2-09 |
| 2 | Settings from a key file | `webkit_settings_apply_from_key_file()` (`[websettings]` group) and `wpe_settings_load_from_keyfile()`; both are in the installed headers (not tried) | none: rejected for now (single user, D-14) |
| 3 | Mode indicator | window title prefixed `[Private]` or `[Automation]` | M2-04 (persistent profile next to ephemeral), M2-12 |
| 4 | Plain path as argument | `g_file_new_for_commandline_arg()`: `minibrowser ./page.html` works; foob needs `file:///absolute/path` | M2-02 (local files are already trusted) |
| 5 | Background colour | `webkit_web_view_set_background_color()`; whether foob flashes white before first paint is not checked | M2-12 |
| 6 | `--version` | prints the engine version; foob treats it as a rejected URI (usage, exit 2) | M2-02 |

Also seen: `related-view` for a second window (M2-11), developer extras only in a diagnostics mode (M2-06), runtime feature switches (M5-09) and a headless display for tests (idea I-02).

Not to take: permission auto-grant, `--ignore-tls-errors`, proxy, cookie-file and
content-filter options, automation, XR, headless mode, the legacy libwpe path (`AGENTS.md`).

## Engine defaults foob runs with (measured 2026-10-14)

foob sets no `WebKitSettings`; MiniBrowser sets developer extras, WebGL, media stream,
WebRTC, encrypted media and autoplay explicitly, and passes `enable-itp` as false. A
small program (not committed) created `webkit_settings_new()` and an ephemeral
`WebKitNetworkSession` on WPE WebKit 2.54.1 and read these back:

| Setting | Default |
|---|---|
| WebGL | on |
| media stream | on (a camera request fails here for lack of a device, roadmap M3-05; geolocation and notification requests are denied, audit-m1) |
| WebRTC | off |
| encrypted media | off |
| developer extras | off |
| site-specific quirks | on |
| `javascript-can-open-windows-automatically` | off |
| file-from-file / universal-from-file access | off |
| ITP | off |
| autoplay (`WebKitWebsitePolicies`) | allow without sound (MiniBrowser sets allow) |
| automation | not allowed (MiniBrowser sets it from `--automation`) |
| TLS errors policy | fail (strict) |
| cookie accept policy | no third party |

Owners: media stream, ITP and autoplay are decided under M3-05 (permission defaults), the
feature list under M5-09. Nothing was changed.
