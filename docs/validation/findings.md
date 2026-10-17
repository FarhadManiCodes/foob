# Findings, decisions and ideas register

The single place for what was decided, what is open and what is parked. Every agent and
the maintainer read it with [handoff.md](handoff.md) before starting work, and update it
**in the same commit** as the change it describes.

## Decisions log (maintainer, 2026-10-13 unless noted)

| ID | Decision |
|---|---|
| D-01 | Window title = page title + ` — foob`. Remote titles are untrusted and sanitized; local content is trusted. |
| D-02 | Downloads are cancelled until M3; "no silent downloads" is a requirement in roadmap M3-03 and the M3 exit. |
| D-03 | HTTP 401 authentication is left as is (the page stays loading, no prompt), a visible failure; the prompt is designed in M2-10. |
| D-04 | New windows. `window.open()` is unsupported and returns `null` (WebKit's `create` signal is not handled). **Updated 2026-10-13** after the second security review: foob ignores **every** new-window request (`target=_blank` links and forms), ships no injected script and loads nothing on a page's behalf, so a `_blank` link does nothing until M2-03 defines the behaviour. Popups and the second window: M2-11 / M3-01; feedback for the ignored click: M2-12 (D-09). Supersedes the retired same-window decision (old D-04). Reason for the update: a load foob makes on a page's behalf drops the asking frame's context. When foob loaded a page's `target=_blank` request itself, a sandboxed iframe (`allow-popups`, no `allow-top-navigation`) replaced the top page at its real origin, unsandboxed, and WebKit sent `Sec-Fetch-Site: none` instead of `cross-site` (second security review). A script that let the page navigate itself avoided this but re-implemented WebKit's target resolution, could not be tested without real clicks, and M2 would replace it, so it was removed. Safe default: the sandbox, Fetch Metadata and remote-to-local rules stay WebKit's; any future load on a page's behalf must keep them (roadmap M2-03 note 8). Scenario `blank-form-does-not-navigate` fails on the script build `f40e4b9`; real clicks are guided rows 1, 3, 9, 15, 16 and 19 (pass on `4e9b6c2`). (Numbered D-05 until 2026-10-15, with the update as a separate D-21; see [Decision numbering](#decision-numbering).) |
| D-05 | JIT-off is not adopted (2.3x slower on a CPU-heavy page, saves 11-32 MiB); revisit at M5-10. |
| D-06 | A dead web process makes foob exit with status 4 (option A); recovery such as one reload is M3-07. |
| D-07 | App-id is `dev.foob.Browser`. |
| D-08 | All local files (`file:`, `about:blank`) are trusted. **Not closed**: revisit in M4 (roadmap open-decisions table). |
| D-09 | Feedback for blocked actions (download, popup, rejected link) is an M2 task (M2-12). |
| D-10 | No test hook in product code: foob gets no switch that forces a user gesture (or anything else) only so a test can reach the policy handler; such checks use a scratch build or a guided check. |
| D-11 | Deferred by the maintainer: missing glyphs in page text (seen in Scholar's language menu) wait for roadmap M3-10. (Until 2026-10-15 it also deferred the project license, now D-15.) |
| D-12 | PDFs open inline in the stock engine's PDF.js viewer; keep it as is for now (2026-10-13). The viewer's save button must still obey the M3-03 download rule. |
| D-13 | Removing accessibility (`USE_ATK` off) from the custom engine is a requested evaluation, tracked as roadmap M5-11; the a11y-bus warnings stay until then (2026-10-13). |
| D-14 | Maintainer, 2026-10-14, after comparing `src/` with upstream MiniBrowser's `main.cpp`: M2 and M3 each get a task that repeats the comparison (M2-13, M3-09) so foob stays more concise, accurate and safer than the engine team's own minimal browser, and the items it does that foob does not (key handling, mode indicator, path argument, background colour, `--version`) each get an owner. **No settings file:** upstream can read settings from a key file (`webkit_settings_apply_from_key_file()`, `wpe_settings_load_from_keyfile()`), but a single-user browser does not need one. Revisit only if a value must differ per user or machine, cannot be a constant, argument or environment variable, and must be read before the first page loads; then use that API rather than writing a parser. Baseline and method: [minibrowser-comparison.md](minibrowser-comparison.md). Cog is not a comparison target (a kiosk shell with its own Wayland layer, about 8,000 code lines). No code changed. |
| D-15 | Maintainer, 2026-10-15: foob is licensed under **BSD-2-Clause** (`LICENSE`), the license of WebKit's new files and of MiniBrowser. The `LICENSE` file and `meson.build` carry the license; per-file `SPDX-License-Identifier` lines are not used (maintainer, 2026-10-16: not legally required, and adding them would be a C change that needs the measurements re-recorded). Closes M0-05. |

## Findings

A finding stays here only while it is open. When it is fixed, the row is removed (the
commit and the git history keep the record); when it belongs to a later milestone, it moves
into that roadmap task (the roadmap's triage rule, under *Scope control*); when it becomes a maintainer
choice, it becomes a decision. Record who found it (`Maintainer`, `Claude`, `Codex` or the
review) and how it was checked.

Next number: **F-77**. F-01 to F-74 were used until 2026-10-15, when the maintainer
reviewed every finding: what was still needed moved to the roadmap, the code, D-04 and
`AGENTS.md`, and the rows were removed. Older commits and notes that cite those numbers
refer to that earlier register (see its git history).

None open. A new finding is a row in this table:

| ID | Finding | Found by | How checked | Status |
|---|---|---|---|---|

## Suspected issues (not reproduced)

A bug that is possible by reading the code but unlikely, which a test could not reproduce.
It stays here so that a later crash or odd behaviour can be checked against it. An entry
leaves when someone reproduces it (it becomes a finding) or a change makes it impossible.
Next number: **S-02**.

| ID | Suspicion | Raised by | How tested | If it shows up |
|---|---|---|---|---|
| S-01 | A SIGTERM or SIGINT that arrives after the signal sources exist but before the main loop runs (during display and view creation, about 10 to 15 ms) is handled only once the loop starts, and `foob_browser_run()` calls `webkit_web_view_load_uri()` first. In theory a page request could start for a user who already asked to quit | Code review before Gate A, 2026-10-17 | 36 launches against a local server, SIGTERM at 5 to 100 ms after spawn: no request reached the server. Up to 30 ms the signal killed foob in the dynamic loader, before its handler existed. Not conclusive: the window is a few milliseconds and the test cannot aim at it | Dispatch the pending sources before the load, or load from an idle callback that is cancelled on the teardown path |

## Ideas parking lot (not tasks yet)

An idea stays here only while no roadmap task or decision holds it; once one does, the row
is removed.

| ID | Idea | Notes |
|---|---|---|
| I-01 | Give the memory benchmark its own copy of `tests/fixtures/viewport.html` without the "Reload this fixture" button. The benchmark measures that page in an open window, and a stray click on the button reloads it, so the launch is thrown away (this happened once in the M1 audit measurements). | Only if it happens again: each measurement since M1 records that page's hash, so measuring a different page breaks the comparison with all earlier runs |
| I-02 | A headless display (`wpe_display_headless_new()`, in the installed headers) renders pages without a window. Two possible uses: (1) run the tests that need no compositor (load, download, crash) without a desktop, while the Wayland checks (resize, focus, compositor close, app-id) stay on niri; (2) render a document to a file, for example a Markdown and KaTeX preview to PDF or an image, for the M4 workflow. | Either use needs foob to choose its display at startup, with Wayland the only interactive one; a switch only for tests would be the hook D-10 rules out. The maintainer leans towards a headless requirement in M4; after the M1 audit it becomes a decision with acceptance criteria |

Next number: **I-03**. Until 2026-10-15 the list ran to I-17; the maintainer reviewed it,
the points still needed moved into the roadmap and D-14, and two ideas stayed: I-01 was
I-08, I-02 was I-17. Older commits and notes that cite idea numbers refer to that earlier
list (see its git history).

## Decision numbering

On 2026-10-15 the maintainer merged the two new-window decisions, moved agent rules to `AGENTS.md` and
closed every gap. Commits up to `7da4b4a` use the first numbering; `0806db2` uses an intermediate one.

| Up to `7da4b4a` | In `0806db2` | Now |
|---|---|---|
| D-01, D-02, D-03 | same | same |
| D-04 same-window `_blank` (already retired) | retired | retired: superseded by the new-window decision (D-04) |
| D-05 `window.open` | D-05 (merged with old D-21) | D-04 |
| D-06 to D-10 | same | D-05 to D-09 |
| D-11 triage rule | D-11 | retired: the roadmap's triage rule |
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
