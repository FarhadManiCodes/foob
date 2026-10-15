# Findings, decisions and ideas register

The single place for what was decided, what is open and what is parked. Every agent and
the maintainer read it with [handoff.md](handoff.md) before starting work, and update it
**in the same commit** as the change it describes.

## Decisions log (maintainer, 2026-10-13 unless noted)

| ID | Decision |
|---|---|
| D-01 | Window title = page title + ` — foob`. Remote titles are untrusted and sanitized; local content is trusted. |
| D-02 | Downloads are cancelled until M3; "no silent downloads" is a requirement in roadmap M3-03 and the M3 exit. |
| D-03 | HTTP 401 authentication is left as is (page stays loading); decided in M2-10. |
| D-04 | New windows. `window.open()` is unsupported and returns `null` (WebKit's `create` signal is not handled). **Updated 2026-10-13** after the second security review: foob ignores **every** new-window request (`target=_blank` links and forms), ships no injected script and loads nothing on a page's behalf, so a `_blank` link does nothing until M2-03 defines the behaviour. Popups and the second window: M2-11 / M3-01; feedback for the ignored click: M2-12 (D-09). Supersedes the retired same-window decision (old D-04). Reason for the update: a load foob makes on a page's behalf drops the asking frame's context. When foob loaded a page's `target=_blank` request itself, a sandboxed iframe (`allow-popups`, no `allow-top-navigation`) replaced the top page at its real origin, unsandboxed, and WebKit sent `Sec-Fetch-Site: none` instead of `cross-site` (second security review). A script that let the page navigate itself avoided this but re-implemented WebKit's target resolution, could not be tested without real clicks, and M2 would replace it, so it was removed. Safe default: the sandbox, Fetch Metadata and remote-to-local rules stay WebKit's; any future load on a page's behalf must keep them (roadmap M2-03 note 8). Scenario `blank-form-does-not-navigate` fails on the script build `f40e4b9`; real clicks are guided rows 1, 3, 9, 15, 16 and 19 (pass on `4e9b6c2`). (Numbered D-05 until 2026-10-15, with the update as a separate D-21; see [Decision numbering](#decision-numbering).) |
| D-05 | JIT-off is not adopted (2.3x slower on a CPU-heavy page, saves 11-32 MiB); revisit at M5-10. |
| D-06 | A dead web process makes foob exit with status 4 (option A); recovery such as one reload is M3-07. |
| D-07 | App-id is `dev.foob.Browser`. |
| D-08 | All local files (`file:`, `about:blank`) are trusted. **Not closed**: revisit in M4 (roadmap open-decisions table). |
| D-09 | Feedback for blocked actions (download, popup, rejected link) is an M2 task (M2-12). |
| D-10 | No gesture-forcing hook inside the product just to test the policy handler (skip). |
| D-11 | Deferred by the maintainer: the font-language-menu glyph issue and the project license (M0-05). |
| D-12 | PDFs open inline in the stock engine's PDF.js viewer; keep it as is for now (2026-10-13). The viewer's save button must still obey the M3-03 download rule. |
| D-13 | Removing accessibility (`USE_ATK` off) from the custom engine is a requested evaluation, tracked as roadmap M5-11; the a11y-bus warnings stay until then (2026-10-13). |
| D-14 | Maintainer, 2026-10-14, after comparing `src/` with upstream MiniBrowser's `main.cpp`: M2 and M3 each get a task that repeats the comparison (M2-13, M3-09) so foob stays more concise, accurate and safer than the engine team's own minimal browser, and the items it does that foob does not (key handling, mode indicator, path argument, background colour, `--version`) each get an owner; the settings key file was rejected for a single-user browser (I-10). Baseline and method: [minibrowser-comparison.md](minibrowser-comparison.md). Cog is not a comparison target (a kiosk shell with its own Wayland layer, about 8,000 code lines). No code changed |

## Findings

A finding stays here only while it is open. When it is fixed, the row is removed (the
commit and the git history keep the record); when it belongs to a later milestone, it moves
into that roadmap task (triage rule, roadmap section 7); when it becomes a maintainer
choice, it becomes a decision. Record who found it (`Maintainer`, `Claude`, `Codex` or the
review) and how it was checked.

Next number: **F-75**. F-01 to F-74 were used until 2026-10-15, when the maintainer
reviewed every finding: what was still needed moved to the roadmap, the code, D-04 and
`AGENTS.md`, and the rows were removed. Older commits and notes that cite those numbers
refer to that earlier register (see its git history).

None open.

## Ideas parking lot (not tasks yet)

| ID | Idea | Notes |
|---|---|---|
| I-01 | Use the window title as a status channel (`Download blocked — foob`) | Chrome-free; belongs to M2-12 |
| I-04 | `create` handler so `window.open` can navigate the same window | M2-11 / M3-01; decide with popup policy |
| I-05 | Untrusted-site mode with JIT off | M5-10 |
| I-06 | Hardening-vs-feature split of the +3.5 KB binary growth | measure by building without hardening flags |
| I-08 | A benchmark-only fixture without the "Reload this fixture" button would stop a stray click from invalidating a run (one fixture launch failed that way, `measurements-m1-audit.md`) | changes the fixture hash recorded since M1, so only if it recurs |
| I-09 | Key handling through `WPEView::event` returning TRUE, as MiniBrowser does | M2-01, M2-09; probe event consumption first |
| I-10 | `webkit_settings_apply_from_key_file()` / `wpe_settings_load_from_keyfile()` as a configuration path (both in the installed headers, not tried) | **No owner; not planned (maintainer, 2026-10-14: a single-user browser needs no settings file).** Revisit only if a value must differ per user or machine, cannot be a constant, argument or environment variable, and must be read before the first page loads; then use this API before writing a parser |
| I-11 | Show persistent versus ephemeral in the window title (`[Private]` upstream) | M2-04, M2-12 |
| I-12 | Accept a plain path as the startup argument (`g_file_new_for_commandline_arg()` upstream) | M2-02; the allowlist stays the only gate |
| I-13 | Check for a white flash before first paint; `webkit_web_view_set_background_color()` | M2-12 |
| I-14 | `--version` (a rejected URI today, exit 2) | M2-02; weigh against binary size (`AGENTS.md`) |
| I-15 | MiniBrowser opens a second view with `related-view` plus the first view's settings and user-content manager, which shares the web process | M2-11: decide sharing with the memory and isolation consequences first |
| I-16 | `enable-developer-extras` (inspector) only when diagnostics are on, to debug compatibility problems and the M2-06 JavaScript-error log | M2-06; weigh the extra process and attack surface, never in a default launch |
| I-17 | `wpe_display_headless_new()` (headers installed) would let tests that do not need a compositor (load, download, crash) run without a nested niri; and `webkit_settings_set_feature_enabled()` plus `webkit://gpu` allow runtime feature and GPU comparisons without a rebuild | M6 / M5-09; a headless switch in product code conflicts with D-10 and with building `WPEDisplayWayland` directly, so it needs a decision first |
| I-07 | Run `desktop_probe.py` on the final binary | done on `d9ea90d` (pass, 2026-10-13); repeat after any C change |

I-02 and I-03 were retired (already roadmap M2-03: the policy module, and trust decided once per committed load).

## Decision numbering

On 2026-10-15 the maintainer merged the two new-window decisions, moved agent rules to `AGENTS.md` and
closed every gap. Commits up to `7da4b4a` use the first numbering; `0806db2` uses an intermediate one.

| Up to `7da4b4a` | In `0806db2` | Now |
|---|---|---|
| D-01, D-02, D-03 | same | same |
| D-04 same-window `_blank` (already retired) | retired | retired: superseded by the new-window decision (D-04) |
| D-05 `window.open` | D-05 (merged with old D-21) | D-04 |
| D-06 to D-10 | same | D-05 to D-09 |
| D-11 triage rule | D-11 | retired: roadmap section 7 |
| D-12 no gesture hook | D-12 | D-10 |
| D-13, D-14, D-15 working rules (already retired) | retired | retired: `AGENTS.md`, `CLAUDE.md`, the handoff rules |
| D-16 TLS, diagnostics, input, Scholar | D-16 | retired: `AGENTS.md` (Architecture & Security Boundaries, Testing Guidelines) |
| D-17 deferred items | D-17 | D-11 |
| D-18 binary size | D-18 | retired: `AGENTS.md` Testing Guidelines |
| D-19 PDF inline | D-19 | D-12 |
| D-20 accessibility | D-20 | D-13 |
| first D-21 merge authorization (already retired) | retired | retired: done, `m1-audit` was merged into `m0-m1` |
| D-21 new windows | merged into D-05 | D-04 |
| D-22 MiniBrowser comparison | D-21 | D-14 |
