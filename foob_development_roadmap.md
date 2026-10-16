# foob — Staged Development Roadmap

**Document type:** Execution plan / agile-style milestones  
**Status:** Project state: [handoff](docs/validation/handoff.md); stages after Gate A are provisional.  
**Created:** 2026-10-12  
**Working project name:** `foob`  
**Companion document:** [`foob_architecture_decision.md`](./foob_architecture_decision.md) — technical decisions, API investigation, security boundaries and research notes.

## Guiding decision

**Develop the browser against Arch Linux's packaged WPE WebKit first, then build a custom reduced-feature WPE separately and compare them using the same browser source revision, tests and workloads.** The executable may require rebuilding/relinking against each engine installation; do not assume a custom prefix can be swapped safely at runtime.

- Engine: Arch `wpewebkit` 2.54.1-2 with the WPE WebKit and WPEPlatform 2.0 APIs, native Wayland; not the deprecated libwpe backend ([versions](foob_architecture_decision.md#versions)).
- Application: **C23**, Meson, GLib/GObject; one web view/window; no GTK/Qt or permanent browser chrome.
- Machine: x86_64 laptop, integrated AMD GPU (`amdgpu`); Arch Wayland with **niri**.
- Custom engine: use **unmodified, pinned upstream WPE source** with a maintained feature configuration/Arch packaging; **no WebKit fork or git submodule** just for build options.
- Servo: **out of scope** for this development cycle.
- Primary principle: **make it run → make it correct → measure it → customize it → decide based on results**.

This is an execution roadmap, not a commitment that the custom build will outperform the distribution build in runtime memory. The under-100-MiB **total PSS** goal for a lightweight page is a stretch target to measure, not a release-blocking promise.

## Workflow and milestone rules

Work in small, reviewable increments, not calendar sprints. The roadmap checkboxes, [findings.md](docs/validation/findings.md) and the [handoff](docs/validation/handoff.md) are the board.

Each issue should have:

- **ID and outcome**, not just an activity (e.g. `M1-04 — niri tile resize changes CSS viewport`).
- **Priority:** P0 blocks the **milestone to which that issue is assigned**, not necessarily M1; P1 is a desirable follow-up within or after that milestone; P2 is deferred. A P0 dialog task assigned to M3 does **not** block Gate A after M1.
- **Acceptance test:** a repeatable observation, command or fixture.
- **Evidence:** commit reference, terminal output, screenshot if genuinely useful, and measured results.
- **Dependencies:** only when another issue must finish first.

**Definition of done:** builds from a clean checkout, passes its acceptance test on the target hardware, leaves no known regression in the established smoke-test set, and updates minimal setup/usage notes. Prefer automated tests for stable behaviors; document manual niri/display tests explicitly.

**Review policy:** stop for a real architecture/performance review at **Gate A (after Milestone 1)** and **Gate B (after Milestone 5)**. Also resolve the **Scholar compatibility decision after M2**, once persistent profiles are implemented, before starting M3. Small implementation fixes inside an accepted milestone do **not** require a new design discussion. Security or API incompatibility discovered earlier may trigger an immediate exception review.

### Agent execution and handoff rules

Read `AGENTS.md` and both root planning documents before implementation. This roadmap controls task order, milestone acceptance and review stops; the architecture controls technical boundaries. Historical `docs/research/qvu/` material supplies evidence, not additional scope. If these documents still conflict materially, report the conflict before implementing the affected behavior.

Work only in the stage the maintainer authorizes, and stop at every gate. A technical pass makes a gate ready for review; it does not authorize the agent to approve its own progression. Continue routine fixes within the authorized stage. Do not change engine, language, security boundaries or accepted scope without a maintainer decision.

| Boundary | Required evidence / action | Continuation |
|---|---|---|
| M0 → M1 | All M0 P0 checks pass, including a clean C23 build/link; record actual versions | Continue within authorized M0/M1 work |
| Gate A, after M1 | All M1 P0 checks pass; submit runtime and measurement evidence | Stop before M2; maintainer reviews and authorizes continuation |
| Scholar decision, after M2 | All M2 P0 checks pass; persistent-profile tests and challenge behavior recorded | Stop before M3; maintainer accepts viability and any clearance requirement |
| M3 → M4 | All M3 P0 checks pass; challenge-frequency decision and observed reliability limits recorded | Continue only within the assigned scope |
| M4 → M5 | All M4 P0 checks pass; local-resource isolation, offline rendering, navigation and lifecycle evidence recorded | Begin the custom-engine experiment only if included in the assigned scope |
| Gate B, after M5 | All M5 P0 checks pass or failed experiment outcomes are documented; comparison and maintenance assessment submitted | Stop; maintainer chooses stock/custom deployment and subsequent M6 scope |

For each acceptance check, record **pass, fail, or not tested**, reproduction steps and evidence under `docs/validation/`, linked from the task. Mark a task complete only when its required checks pass. An investigation may complete with a documented unsupported capability, but must not be recorded as implemented functionality. Unavailable hardware, manual interaction or diagnostics mean not tested, not pass; a required untested check leaves the milestone incomplete. Record P1 deferrals explicitly; P2 work requires promotion into the assigned scope. Keep a handoff record with current milestone, completed tasks, failures, pending decisions and the next action. Do not publish credentials or private profile contents in evidence.

Before each task, turn its acceptance text into a short checklist with fixture/input, expected result and observation method. For measurements, specify viewport/scale, idle interval, repetitions and cold/warm state before collecting results. Routine implementation choices within accepted boundaries may be selected and documented by the agent; unresolved product tradeoffs such as acceptable challenge frequency, deployment choice or a proposed architecture change belong to the maintainer. Report failed/untestable checks and continue independent work within scope, but do not cross a gate or claim an incomplete milestone is done.

### Open decisions and when to resolve them

These are intentional investigations, not permission to improvise beyond scope or reasons to delay unrelated work.

| Decision | Timing / owner | Required outcome |
|---|---|---|
| Local-file trust | M4, maintainer | **Not closed.** Today every local `file:` page and `about:blank` is trusted (`foob_uri_is_local()`, the single decision point) and the design assumes that. Revisit in M4 for files that arrived from the web (M3 downloads), for converted documents, for a remote iframe inside a local page (trust is judged by the main frame today), and for `about:blank`, whose document may inherit an origin from the page that opened it; no task until then |
| Shortcut bindings and find-query interaction | M2-01/M2-09, implementing agent | Document tested bindings and input/focus behavior; retain no persistent chrome |
| Navigation defaults and persistent-profile concurrency | M2-03/M2-04, implementing agent | Record allowed/denied schemes, window behavior and safe handling of concurrent launches; do not assume shared-profile locking works |
| Scholar clearance tolerance and challenge frequency | Post-M2 decision and M3-08, maintainer | Accept or reject the observed workflow; record the tested time span and limits |
| Dialog/file-picker mechanism and PDF handoff | M3, implementing agent | Select and test a toolkit-free mechanism, cancellation/error behavior and safe destinations |
| Markdown converter/dialect, document authorization and scheme origins | M4, implementing agent | Record the selected route and security policy; pass the required fixtures before closing M4 |
| Typst export feasibility | M4-06, implementing agent | Report supported/unsupported output and fidelity; live tinymist integration remains deferred |
| Custom-engine deployment value | Gate B, maintainer | Choose stock/custom from measured results and maintenance costs |

## Milestone overview

| Stage | Name | Deliverable | Exit / review point |
|---|---|---|---|
| **M0** | Environment and skeleton | Reproducible build, stock WPE version, minimal repo | Toolchain and WPE pkg-config checks pass |
| **M1** | First native window | Single-page Wayland browser running under niri | **Gate A: inspect actual behavior and measurements** |
| **M2** | Controllable browser shell | Safe navigation, focus/keyboard, profile, basic diagnostics | Core workflows pass; Scholar compatibility decision |
| **M3** | Web compatibility and integration | Dialogs, session persistence, uploads, PDF downloads and external handoff | Practical browsing flows pass |
| **M4** | Local research and math | Secure serverless local previews, MathML/KaTeX tests | Offline math fixtures pass; security boundaries checked |
| **M5** | Custom WPE and A/B comparison | Independently packaged, reduced-feature engine; comparative report | **Gate B: keep custom, adjust it, or stay with stock** |
| **M6** | Harden and package | Repeatable releases, regression tests, maintenance procedure | Candidate for routine personal use |

M1 is intentionally small. M2–M4 form the **usable browser MVP**, while M5 is an **optimization experiment**. Feature additions need not wait for M5, and custom-WPE work must not prevent timely security updates.

---

## M0 — Development environment and repository skeleton

**Goal:** confirm the distribution API works on this machine before writing substantial browser code.

### Backlog

- [x] **M0-01 / P0:** Record package and compiler versions (`pacman -Q wpewebkit`; Meson, Ninja, C compiler).
- [x] **M0-02 / P0:** Install Arch build tools and WPE using pacman **only if missing**. First inspect what is already installed and the package dependency changes.
- [x] **M0-03 / P0:** Confirm `pkg-config --modversion wpe-webkit-2.0` and `pkg-config --modversion wpe-platform-2.0`; optionally check `wpe-platform-wayland-2.0` where exposed.
- [x] **M0-04 / P0:** Initialize one small Meson/C23 project; verify the compiler supports `c_std=c23` and WPE symbols link successfully.
- [x] **M0-05 / P1:** Add `README.md`, `.gitignore`, basic license decision (BSD-2-Clause, D-15) and concise `docs/` links to the architecture and this roadmap.

Done 2026-10-12 ([versions](docs/validation/m1.md#environment)); license chosen 2026-10-15 (D-15).

**Exit:** all M0 P0 checks pass: a clean Meson/C23 build links to packaged WPE and WPEPlatform; toolchain and package versions are recorded. If C23 is unsupported, investigate the toolchain and report options; changing the accepted language standard requires a maintainer decision.

## M1 — Minimal native browser: first empirical checkpoint

**Goal:** establish that a toolkit-free WPE view works correctly on this machine's integrated AMD GPU under niri and collect exploratory Scholar observations. No custom WPE, no file chooser and no browser UI yet. The Scholar compatibility decision follows M2's persistent-profile implementation.

### Backlog

- [x] **M1-01 / P0:** Create a `WebKitWebView` through the stable WPEPlatform API, using `g_object_new(WEBKIT_TYPE_WEB_VIEW, ...)`; run a GLib main loop.
- [x] **M1-02 / P0:** Use the **Wayland** backend (foob builds `WPEDisplayWayland` directly, so `WPE_PLATFORM` cannot select another) and load a **trusted** CLI URI or harmless default HTTPS page. Full validation/policy for arbitrary input and unsupported schemes belongs to M2.
- [x] **M1-03 / P0:** Quit on the WPE view's `closed` signal; release owned GObjects; confirm no leftover application subprocesses after normal exit.
- [x] **M1-04 / P0:** Verify niri tiling, resize, maximize/fullscreen where applicable, focus and mouse scrolling; use the WPE-managed toplevel rather than writing another xdg-shell implementation.
- [x] **M1-05 / P0:** Verify simple keyboard input in an HTML text box and navigation/link clicking; observe whether rendering is GPU accelerated, or record inability to verify.
- [x] **M1-06 / P0:** Measure **total browser process-tree PSS** at fixed viewport/idle intervals for `about:blank`, a known local HTML fixture, and Wikipedia; record cold/warm starts separately.
- [x] **M1-07 / P1:** Test one simple local **MathML** fixture and an uncomplicated JS page; record pass/fail without implementing the preview pipeline yet.
- [x] **M1-08 / P1:** Capture runtime warnings, WPE/GLib errors, compositor-related observations and reproducible failure cases.
- [x] **M1-09 / P1:** Explore Google Scholar on stock WPE with default browser identity: submit a search, open a result and scroll. Record the session/profile configuration, immediate access, challenges, any attempted manual clearance and its outcome. Unresolved blocking prompts investigation, not automatic engine rejection; do not assess restart reliability before persistent profiles exist. Do not import Qt cookies, spoof headers, or weaken security to obtain a baseline. See the [qvu navigation and Dooble observations](docs/research/qvu/findings/scholar-navigation-and-dooble.md) for the unresolved failure that motivated this check. This task does not require M2's profile management or M3's download/dialog integration.

### Gate A — stop, review and decide next implementation tasks

Evidence: [M0/M1 acceptance record](docs/validation/m1.md), [audit](docs/validation/audit-m1.md),
[PSS measurements](docs/validation/measurements-m1-audit.md),
[guided checks](docs/validation/guided-checks-m1-audit.md),
[memory and GPU baseline](docs/validation/blank-memory.md) and the
[handoff](docs/validation/handoff.md) (open items and limits). Gate A is ready for review,
**not approved**.

Evidence to bring to the review:

1. Exact installed `wpewebkit` version, compiler version and Meson configuration.
2. Successful startup/close (or a minimal reproduction of the failure).
3. niri resize/focus/scroll observations and graphics backend evidence.
4. Total PSS, page/viewport dimensions and timing protocol for all three M1-06 workloads (`about:blank`, local fixture and Wikipedia), with cold/warm states recorded.
5. Basic text input results; MathML observations if tested, or an explicit P1 deferral; notable log warnings.
6. Exploratory Scholar search/result/scroll outcomes, session/profile conditions, and challenge/manual-clearance behavior, if tested. These observations do not establish persistence or long-term compatibility; the decision check follows M2.

**Ready for review:** all M1 P0 checks pass. Demonstrate a correctly sized initial page, viewport/input alignment after tile resizing, usable typing/clicking/scrolling, supported fullscreen/maximize transitions, and clean shutdown without remaining browser child processes; attach M1-06 measurements. Mark unavailable display/scale cases and unverified acceleration explicitly. Carry unresolved Scholar observations into the persistent-profile test. Stop for the maintainer's Gate A decision before M2.  
**Fix first:** backend initialization, visible rendering, process cleanup or viewport sizing fails. Exploratory Scholar blocking prompts investigation but does not by itself block M2.  
**Reconsider engine choice only if:** a blocking incompatibility appears that cannot reasonably be resolved through public WPE APIs and documented configuration. Defer the Scholar compatibility decision until M2's persistent profile is tested. Do not begin stripping features merely to conceal a stock-engine integration defect.

## M2 — Minimal controllable shell

**Goal:** make the one-window launcher useful without adding a conventional browser chrome.

- [ ] **M2-01 / P0:** Implement explicit, minimal keyboard shortcut policy through `WPEView::event`; forward unrelated input to the webpage. *MiniBrowser comparison (M2-13):* upstream consumes keys by returning TRUE from a `WPEView::event` handler that tests `WPE_EVENT_KEYBOARD_KEY_DOWN`, modifiers and keyval; probe that mechanism first (see M2-09).
- [ ] **M2-02 / P0:** Support command-line URL opening; back, forward, reload, stop, and **copy current URL** (without a permanent address bar). *MiniBrowser comparison (M2-13):* upstream accepts a plain path as the argument (`g_file_new_for_commandline_arg()`) and has `--version`; decide whether foob's strict `file:///absolute/path` argument should accept a path (local files are already trusted; the allowlist and its tests must stay the only gate) and whether `--version` is worth its bytes (binary size, `AGENTS.md`).
- [ ] **M2-03 / P0:** Define ordinary HTTP(S), `file:`, downloads, unknown schemes, and `target=_blank` behavior; reject unsafe external-command interpolation.

  **Carried from the M1 audit into M2-03:** (1) ordinary in-window navigation relies on WebKit's own checks; since the second security review (D-04) foob loads nothing on a page's behalf, so `foob_navigation_allowed()` is no longer called; define one policy (any load foob makes on a page's behalf must call that check first, with a test that fails when a load bypasses it; today only the unit test of `foob_navigation_allowed()` exists, as no such load exists) and move the policy handlers out of `browser.c` into their own module. Judge the source by the committed URI: `webkit_web_view_get_uri()` returns the pending one, so during a pending local navigation a remote page would look local. (2) The window-title sanitizer is not a security boundary: it removes C0/C1 controls, every Unicode format character (category Cf, except the joiners and visible number signs) and the rest of the default-ignorable set (except variation selectors 1-16), and starts a title that may hold right-to-left text with a left-to-right mark, but it does not remove look-alike (homoglyph) characters, and a page can still end its title with `— foob`. A long title is cut by code point, not by grapheme, so a combining or ZWJ sequence can be split (cosmetic, accepted; a fix needs Pango segmentation). (3) Title trust is read from the current URI when the title changes; decide trust once per committed load instead. (4) A `target=_blank` POST form does nothing now (D-04); an earlier version kept its method and body (found in the Codex review). Unsupported or unsafe submissions need an explicit M2-03 policy. (5) URLs with stray percent escapes in the path are rejected by the strict `GUri` parse and silently do nothing. (6) Trust is judged by the main frame's URI, not the frame that asked, so a remote iframe inside a local page can open a `file:` link (see the local-file trust row). (7) Scripted requests (`window.open`, a programmatic click) are stopped by WebKit's popup blocker before the handler; a real click on a `_blank` link or form reaches `decide-policy` and is ignored (D-04); a real click is still a guided check. (8) Define what `target=_blank`, named targets (`_new`), `form.submit()` and links in shadow trees do (D-04: nothing today); if foob ever loads a request itself it must keep the asking frame's sandbox, Fetch Metadata and the remote-to-local rule, which an application-initiated load drops (D-04).

  **Starting point (D-04):** every new-window request is ignored and foob loads nothing on a page's behalf; `window.open()` returns `null`. Popups: M2-11 and M3-01; feedback for an ignored click: M2-12.
- [ ] **M2-04 / P0:** Add dedicated XDG profile/data/cache directories using `webkit_network_session_new(data_dir, cache_dir)` and the web view's `network-session` property; verify persistence after restart. The browser's `WebKitWebContext` and session have distinct responsibilities. *MiniBrowser comparison (M2-13):* once a persistent window exists beside the ephemeral one, show which is which (upstream prefixes the title with `[Private]`).
- [ ] **M2-05 / P1:** Verify text selection, clipboard copy/paste, Unicode/compose input and focus in website forms.
- [ ] **M2-06 / P1:** Log navigation failures, abnormal process exits, and useful troubleshooting details. Consider `enable-developer-extras` (the web inspector) only when diagnostics are on; weigh its extra process and attack surface, and never enable it in a default launch.

  Track the user-requested URL separately from the current committed URL; update it on deliberate navigation, not redirects. Test that a Scholar challenge redirect preserves the original search for explicit recovery or future handoff. Redact sensitive queries in routine logs; this does not add browsing history or external-browser integration.

  In bounded diagnostics, record navigation/redirects, response status where available, resource failures and subsequent JavaScript errors in sequence. Distinguish initial site rejection, challenge-page loading/interaction failure, and loss of clearance after successful manual completion. Verify available WPE diagnostic hooks; disclose missing observations and do not infer causation from console errors alone. Keep sensitive URLs, cookies and request bodies out of routine logs. Motivated by the qvu [read-only investigation](docs/research/qvu/findings/scholar-readonly-investigation.md) and [navigation findings](docs/research/qvu/findings/scholar-navigation-and-dooble.md), where rejection came before the challenge page's errors.

- [ ] **M2-07 / P2:** Evaluate a transient URL launcher/prompt; **do not** choose an overlay, toolbar, tabs or IPC before an actual use case requires them.

  This defers URL-entry UI, not M2-09's required find-query interaction. Implement only the transient interaction needed for accepted reading controls; do not expand it into a general browser interface.
- [ ] **M2-08 / P0:** After M2-04, test Scholar with a fresh dedicated persistent profile and default browser identity: submit a search, open a result, scroll, complete any available challenge manually, then close and restart with the same profile and repeat. Record immediate versus clearance-dependent access, recurring challenges, and whether access survives restart. If blocked, compare the same search in fresh Firefox private mode on the same machine/network near the same time, without assuming identical routing. Confirm profile persistence separately with a controlled fixture so storage defects are not mistaken for site rejection. *Load rule (`AGENTS.md`):* every Scholar load is a deliberate manual step, at most once or twice a minute; never put Scholar in an automated test, script, benchmark or retry loop, so Google does not block this machine's IP.

- [ ] **M2-09 / P0:** Investigate and implement basic reading controls from the initial browser stage: keyboard page zoom in/out/reset, find-in-page query entry, next/previous match and dismiss. Before choosing the interaction, probe WPEView's event signal, event consumption/propagation, focused forms/contenteditable, modifier handling and IME composition, and verify the supported public WebKit find/zoom APIs. Choose the simplest toolkit-free query-entry mechanism that works with that input model; the transient interaction remains to be decided, with no persistent chrome. Do not assume a conventional toolkit shortcut/prompt design is necessary or that signal-based handling automatically improves performance. Consume only reserved shortcuts and preserve ordinary page input. Test Unicode queries, no matches, repeated matches, search dismissal/focus restoration, zoom reset and math legibility; reuse these controls in M4 previews. Measure any claimed performance benefit separately.

- [ ] **M2-10 / P1:** Decide HTTP authentication (401/407 Basic/Digest) without persistent chrome. **Known M1 behavior, deliberately left unchanged (D-03):** foob does not answer WebKit's `authenticate` request, so such a page stays loading and unauthenticated; this fails visibly and is a useful signal until a prompt exists. Credentials must never be logged or persisted unintentionally. Test with a local server returning 401.
- [ ] **M2-11 / P2:** Decide whether and how a second window is opened (for example Shift-click on a link). Define which resources windows share (browser process, network session, cache, cookies, WebKit processes) and the memory/security consequences before implementing; the current design is one view per process. *MiniBrowser comparison (M2-13):* upstream opens a second view with `related-view` and the first view's settings and user-content manager, which shares one web process.
- [ ] **M2-12 / P1:** Feedback when foob blocks an action (cancelled download, blocked popup or script window, rejected link, scheme refused) without persistent chrome. Today nothing is shown; the window title is the candidate channel, to be decided with M2-07 (D-09). *MiniBrowser comparison (M2-13):* also check whether the window flashes white before the first paint and whether `webkit_web_view_set_background_color()` is warranted.
- [ ] **M2-13 / P1:** Compare the finished M2 shell with upstream MiniBrowser (`Tools/MiniBrowser/wpe/main.cpp`, WPE WebKit 2.54.1 or the then-current release; baseline and method in [minibrowser-comparison.md](docs/validation/minibrowser-comparison.md), decision D-14). For every feature foob adds that MiniBrowser also has (keys, navigation, reload, title, fullscreen, permissions, downloads, new windows, profile), record both approaches, foob's line count for it and the evidence (test or guided row). foob passes when it needs no more code for the same behaviour (or says why more is justified), has a test where MiniBrowser has none, and grants nothing without a decision; a miss is recorded as a finding, not hidden. Also work through the six items the baseline lists as "what MiniBrowser does that foob does not": adopt, defer with an owner, or reject with a reason.

### Scholar compatibility decision — before M3

**Ready for review:** demonstrate search, result navigation and scrolling in at least two independent launches using the same persistent profile, including after a clean shutdown; record any manual-clearance requirement and confirm storage persistence with the separate fixture. Report the number of attempts and observation period; two launches are a minimum continuity check, not proof of long-term reliability. Stop before M3 for the maintainer to accept viability and any clearance requirement.  
**Investigate:** access remains blocked, clearance is lost, or results are inconsistent. Check profile persistence and supported configuration before attributing failure to the engine. Pause progression to M3 until the decision is resolved.  
**Reconsider engine choice:** repeatable blocking comparable to the Qt experience remains after profile verification, while the Firefox comparison works and no reasonable public-API/configuration remedy is established. A single challenge alone is not a reason. Do not import Qt cookies, spoof headers or weaken security to force a passing result.

**Exit:** all M2 P0 checks pass: CLI navigation and back/forward/reload/stop/current-URL copy work; unrelated typing is preserved; the profile survives restart; documented scheme/new-window policy tests pass; find/zoom controls pass their fixtures; the maintainer accepts the Scholar evidence; the MiniBrowser comparison (M2-13) is recorded. No persistent on-screen controls are introduced.

## M3 — Compatibility, files and browser responsibility

**Goal:** make essential websites and document workflows work without accidentally widening privileges.

- [ ] **M3-01 / P0:** Explicit policy for JavaScript alerts/confirms/prompts, HTML selects, `window.open` and popup authentication; test each with local fixtures. *M1 guided check:* a right-click shows no context menu today.

  **Carried from the M1 audit:** `window.open()` is unsupported today (returns `null`; WebKit's `create` signal is unhandled) and `target=_blank` links do nothing (D-04), so popup flows that need `window.opener` (OAuth sign-in popups) do not work; decide the popup policy with that in mind.
- [ ] **M3-02 / P0:** File-upload chooser integration (evaluate XDG portal or another truly toolkit-free solution on niri); test single and multiple file uploads. *M1 guided check:* a file input does nothing today (no picker opens).
- [ ] **M3-03 / P0:** WebKit-managed downloads with validated destinations, file-name collisions, cancellation/failure reporting and authenticated PDF links.

  **No silent downloads (requirement from the M1 audit, 2026-10-13).** No download may start, and no file may be created, without an explicit user decision. M1 probing showed that WebKit's default saved an attachment response into `~/Downloads` with no prompt. Until this task is complete foob cancels every download (`WebKitNetworkSession::download-started` -> `webkit_download_cancel`) because there is no UI to ask yet. M3-03 replaces that interim default with an explicit per-download decision on a validated destination. Acceptance fixtures (local server, download directory redirected to a temporary directory through `XDG_CONFIG_HOME/user-dirs.dirs`): `Content-Disposition: attachment`, a link with the `download` attribute, a `blob:` download, a download started by a script without a user gesture, and a PDF link. For each, **before** the user accepts, no file, partial file or temporary file exists; after acceptance the file lands only in the validated destination, never overwrites silently, and leaves nothing behind when cancelled. Today the single `download-started` cancel is the only guard; also decide whether the response policy should ignore non-displayable responses before a download object exists. Replace the `download-cancelled` scenario in `tests/nested_lifecycle.py` with accept/cancel/never-silent scenarios and keep a negative control that fails on a build with WebKit's default behavior.

  **Trust note:** a file saved by a download is afterwards a local file, so `foob_uri_is_local()` would trust it. Validate destinations with that in mind; M4 revisits the local-file trust rule.
- [ ] **M3-04 / P0:** Open downloaded PDFs through a carefully controlled external reader/desktop mechanism; avoid launching raw page-supplied commands. *Found in the M1 audit guided check:* the stock engine renders a PDF URL inline with PDF.js (toolbar with save, print and annotate buttons), which differs from the "external viewer, no embedded PDF renderer" decision. **Maintainer decision (D-12): keep the inline viewer as is for now.** Remaining requirement: the viewer's save button must not bypass the M3-03 download rule; revisit if the custom build sets `ENABLE_PDFJS=OFF` (architecture table).
- [ ] **M3-05 / P0:** Verify TLS error handling, permission defaults and **effective runtime** sandbox isolation. The older `webkit_web_context_{get,set}_sandbox_enabled` accessors are **not available in WPE 2.0**; use current build/runtime evidence rather than coding against those symbols. **Do not disable sandbox/TLS checks to make a site work**. *From the M1 guided check:* `getUserMedia` on this machine fails with `OverconstrainedError` ("no device was found amongst 0 devices") before any permission decision, so the real prompt/deny default is untested. An audio-only request (2026-10-16, real microphones present in PipeWire) fails the same way and `enumerateDevices()` returns an empty list, so the missing `v4l2src` (`gst-plugins-good`) is not the only cause: the stock web process sees no capture device at all (sandbox, build feature or device-monitor cause not yet found); test it with a present device or WebKit's mock capture devices (`enable-mock-capture-devices` setting, scratch build) and record the default.

  **Carried from the M1 audit:** probing showed permission requests are denied and invalid certificates refused by WebKit's defaults, not by foob code; camera and microphone prompts have not been observed at the keyboard. Record the observed defaults before relying on them. **Also decide** (baseline in [minibrowser-comparison.md](docs/validation/minibrowser-comparison.md)): foob runs with engine-default settings, where media stream and site-specific quirks are on and ITP is off, whereas MiniBrowser sets its feature switches explicitly. For each, state whether foob keeps the default, with the reason.
- [ ] **M3-06 / P1:** Test Wikipedia, Scholar and Semantic Scholar search/forms/links and representative ChatGPT/Claude/Gemini editing/login/upload flows, documenting incompatibilities.
- [ ] **M3-07 / P1:** Basic crash recovery (interim since the M1 audit, D-06: a dead web process makes foob exit with code 4, `FOOB_EXIT_ENGINE_LOST`; recovery, such as one reload before exiting, replaces this and must keep a test that kills the web process), safe shutdown with downloads active and sensible user-visible errors without conventional chrome.
- [ ] **M3-08 / P0:** Extend the accepted M2 Scholar baseline with M3's integrations: repeat search/result/scroll checks across restarts and separate sessions, record recurring challenges or loss of clearance, and verify paper/PDF links, authenticated downloads where applicable, and external PDF handoff. Bring renewed blocking to review; the M2 result establishes initial viability, not proof of long-term reliability.

  **Reliability checks for M3-08:** Retest in later sessions, including after any observed clearance expires naturally; do not extend expiry or generate background requests to preserve access. Record elapsed time since clearance, challenge recurrence, and whether manual completion restores usable search/result navigation. Review and agree on an acceptable challenge frequency before declaring the workflow reliable. If no expiry or recurrence was observed during testing, report the observation period and leave behavior beyond it unverified. The [qvu post-expiry comparison](docs/research/qvu/findings/scholar-post-expiry.md) showed that a successful restart could precede renewed blocking even with a complete profile; it did not prove expiry was the sole cause. Validate the ordinary dedicated persistent session first. Any later selective-cookie/storage minimization requires a separate comparison against that working baseline.
- [ ] **M3-09 / P1:** Repeat the MiniBrowser comparison (M2-13, [minibrowser-comparison.md](docs/validation/minibrowser-comparison.md), D-14) for the M3 features: permissions, downloads, popups and `window.open`, dialogs, crash handling, file input. Upstream grants every permission and handles no download, new window or crash; foob must show a tested, explicit decision for each, and report its code size for them next to upstream's as a signal (`AGENTS.md`), not as a target.
- [ ] **M3-10 / P1:** Text in the maintainer's languages renders correctly.

  **Story:** As the maintainer, I read English, Farsi, German, Italian, Portuguese and Arabic on any site (Wikipedia, Google Scholar, others) and see every character in a readable font, and I type English, German, Italian and Portuguese.

  **Observed (M1, 2026-10-13):** Scholar's language menu showed missing-glyph boxes ([screenshot](docs/validation/images/scholar-language-glyphs.png)); the maintainer sees the same on Wikipedia and other sites. Cause unknown (missing fonts, fallback, site styling or the engine). foob sets no fonts, so WebKit's generic `sans-serif` goes to fontconfig: Noto Sans for the four Latin-script languages, while Farsi and Arabic fall back to Noto Kufi Arabic, a display style; Noto Sans Arabic and Noto Naskh Arabic are installed but not picked first. No installed font covers both Latin and Arabic script, and no Chinese, Japanese or Korean font is installed.

  **Constraints:** fonts installed on the system, none bundled; one general default (WebKit's default font families set as constants in foob, or one fontconfig rule), not a font per language; no settings file (D-14). The maintainer picks the font from screenshots.

  **Acceptance criteria:**
  1. A local fixture with a heading, a paragraph, a form field and a list in each of the six languages shows no missing-glyph boxes, at scale 1 and with page zoom (M2-09).
  2. Farsi and Arabic: letters joined, right-to-left order correct, mixed Latin and number runs in the right order, a readable text style (not a decorative one); the Farsi letters پ چ ژ گ ک ی and Persian digits render.
  3. The same holds on live pages: a Wikipedia article in each language and Scholar's language menu (Scholar opened on purpose, `AGENTS.md`).
  4. Typing English, German, Italian and Portuguese, including ä ö ü ß à è ì ò ù ã õ ç, into a form field with the real keyboard shows the right characters (guided check).
  5. Text that rendered before (Latin pages, local MathML) looks the same (screenshot comparison).
  6. Languages outside the six may show boxes; they are recorded, not fixed.
  7. Evidence: the font actually used per language (not only the CSS declaration), the screenshots and the maintainer's choice.

**Exit:** all M3 P0 checks pass with recorded fixtures: dialogs/selects and allowed/blocked popup behavior, single/multiple uploads and cancellation, completed/failed/cancelled downloads **with no file created before explicit user action**, filename collisions, PDF handoff, invalid-certificate rejection and runtime sandbox evidence. The M3-09 comparison is recorded; M3-08 includes the maintainer's challenge-frequency decision and the observation period; unobserved future expiry behavior is a documented limitation, not a claim of reliability.

## M4 — Local documents, math and offline research

**Goal:** make the browser a useful local mathematical/research viewer **without a localhost web server**.

- [ ] **M4-01 / P0:** Open explicitly authorized `.html` using a correctly escaped `file://` URI; test relative assets and filesystem boundaries. Confirm the canonical `file://` URL and MIME detection; minimal installations may need `shared-mime-info`.
- [ ] **M4-02 / P0:** Introduce a limited `foob:` URI-scheme handler for **managed document resources**, with per-document allowlists and correct MIME types. Before choosing a scheme classification, test CORS, relative URLs, cookies/storage, history, origin and CSP on custom-scheme pages. Compare memory and speed of `file://`, load-bytes and streaming scheme handlers, and confirm the sandboxed subprocesses keep working.

  Managed previews use a separate ephemeral network session; ordinary browsing retains M2's persistent session. Investigate the required WPE session/context boundaries before implementation. User-activated web links open in the ordinary browsing session without preview privileges. This preview isolation does not introduce a user-facing private-browsing mode.

- [ ] **M4-03 / P0:** Threat-test traversal (`../`, `%2e%2e`), encoded and malformed paths, absolute paths, symlinks, cross-document access, a remote page navigating to a crafted `foob:` path (must be denied), unexpected remote requests and untrusted content. Do not grant universal file access.

  Verify preview storage is separate from browsing cookies/logins, preview site state does not persist after its session ends, and preview cleanup cannot erase browsing data. Test filesystem/network restrictions independently; ephemeral storage alone does not enforce those boundaries.

  **Fresh-process controls for M4-03:** Apply the intended scheme classifications, allowlists and remote-resource restrictions before first navigation. Start each cold policy case in a fresh browser process with isolated empty test data/cache directories; do not reuse the normal browsing profile or a process previously run with relaxed restrictions. Use controlled allowed/denied resources to distinguish correct policy enforcement from general load failure. Record process/session isolation, cache state, active policy, request attempts and outcomes. Test warm reload separately; changing settings in a running process and reloading is not a substitute for the cold control.

- [ ] **M4-04 / P0:** Keep separate fixture groups for hand-written MathML controls and converter-generated MathML: fractions, radicals, scripts, limits, matrices, aligned rows, cases, delimiters, fonts, selection and zoom. Also check font fallback and scaling, copying math into other applications, and a math-heavy Wikipedia page (equations, font fallback, zoom and resize). Inspect generated markup, visible layout and row/delimiter geometry as well as error counts. Record the converter version, selected math font and actual font used where measurable; a CSS family declaration alone does not prove that font rendered. Use a known-font control when diagnosing malformed delimiters. Compare failing generated cases with equivalent hand-written controls before attributing defects to WPE. See the [qvu renderer comparison](docs/research/qvu/findings/math-renderer-comparison.md) and [font-selection findings](docs/research/qvu/findings/temml-katex-comparison.md); these motivate tests, not verified WPE outcomes.
- [ ] **M4-05 / P0:** Offline Markdown + bundled KaTeX CSS/JS/fonts, or equivalent prerendering; verify **zero required network requests**.

  **Acceptance checks for M4-05:** Recreate or explicitly import the six formula cases from the [qvu local KaTeX experiment](docs/research/qvu/findings/katex-local-assets.md): nested fraction, matrix, aligned equations, cases, summation and fluid equation. Include local images and a filename with special characters. The fixture-generation code remains in qvu; it was not transferred into foob. Pin and record the KaTeX asset version, package JavaScript/CSS/fonts together, and preserve the font paths expected by the stylesheet. On stock WPE, verify visible equation correctness, loaded font faces and absence of math errors through `file://`, then repeat through the managed `foob:` transport when available. With networking unavailable, test both a fresh load and reload and confirm no HTTP requests are attempted. Test missing stylesheet/script/font assets and invalid TeX, ensuring failures produce useful diagnostics rather than silently appearing successful. For prerendering, verify the generated math and required CSS/fonts; browser-side KaTeX JavaScript need not be present. An empty resource-timing log alone is not proof of successful asset loading or offline operation. Keep security restrictions intact, and include the same fixtures in M5's custom-engine regression checks.

  **Cold offline control for M4-05:** Make networking unavailable before launching a fresh browser process with isolated empty test data/cache directories, with restrictions applied before first navigation. Load the local fixture without first visiting a CDN or enabling remote access. Verify visible math, required assets and fonts, and no attempted HTTP(S) requests using appropriate request diagnostics; report how networking was made unavailable and any limits of the diagnostics. Record this cold result separately from a subsequent warm reload under the same offline restrictions. Repeat for `file://` and managed `foob:`. Run missing-asset cases with fresh process/cache state too, so cached copies cannot mask the failure. The [qvu asset-access investigation](docs/research/qvu/findings/katex-investigation.md) found that reload after restoring restrictions could pass while a fresh process failed; its cache/retained-state mechanism was not isolated.

  **Mixed Markdown fixture for M4-05:** Define and document the supported Markdown dialect and math-delimiter rules. Test one document containing a table, checked/unchecked task items, inline code and fenced code with dollar signs, ordinary text with literal currency amounts, and intended inline/display equations. Verify table/task formatting and literal code/text survive conversion, while only intended math is rendered. Use this fixture to evaluate the chosen converter and include it in stock/custom WPE regression checks. The [historical Markdown adapter plan](docs/research/qvu/design/qt6_minimalist_research_viewer_blueprint.md) motivates these cases; it does not require reuse of Neovim's renderer.

- [ ] **M4-06 / P1:** Investigate the actual Typst export route supported by installed Typst; compare output quality, retain external PDF viewer for faithful layout.

  **Evidence boundary for M4-06:** The [qvu rendering experiment](docs/research/qvu/findings/rendering-findings.md) demonstrated paginated output and editor synchronization through tinymist's live preview frontend under Qt; it did not validate serverless Typst HTML/MathML export or WPE. The [historical adapter notes](docs/research/qvu/design/qt6_minimalist_research_viewer_blueprint.md) mention a prior WebKit canvas failure without establishing current WPE behavior. Keep export investigation separate from live preview. If live tinymist integration is requested later, investigate pagination, incremental updates, fonts and bidirectional editor synchronization independently; no live integration task is introduced here.

- [ ] **M4-07 / P1:** Benchmark local MathML versus KaTeX, and prerendering versus client-side rendering, under reproducible inputs. Require correct mathematical structure and visible layout before treating a rendering path as a viable alternative; report converter failures separately from engine/font failures. Use equivalent formulas, consistent viewport/zoom and recorded fonts, and repeated measurements rather than inferring rankings from qvu's single Qt snapshots. Keep KaTeX as the preview candidate unless validated results justify a change.

  Report browser process-tree PSS after rendering, conversion time, converter memory and total source-to-usable-preview time separately; state whether helpers exit or remain running and disclose measurement limitations. Browser-only savings must not hide work moved into converters. The under-100-MiB stretch target remains browser PSS, with workflow costs reported alongside it.

- [ ] **M4-08 / P0:** Implement source-aware linked-Markdown navigation after M4-02/M4-03: preserve each preview's original source location and raw link target, resolve relative `.md` links against that source rather than the generated HTML/cache location, check authorization, and convert the target before displaying it. Preserve fragments such as `details.md#proof`; define deterministic heading IDs in the renderer and test explicit anchors, generated heading anchors and duplicate headings. Test sibling/parent-directory links, spaces and encoded filenames, missing targets, and Back navigation between converted documents. Links outside approved roots must be denied or require explicit authorization; navigation must not silently expand file access. Recreate or explicitly import the [qvu navigation fixtures](docs/research/qvu/findings/markdown-local-navigation.md), whose probe code remains in qvu.

  **Web-link routing for M4-08:** User-activated HTTP(S) links from a managed preview open in a separate foob browsing window, using M2/M3's navigation and window policies, while preserving the preview document, scroll position and zoom. Same-document fragments stay in the preview; linked Markdown follows the authorized conversion route above. Automatic remote resource requests remain blocked by default; this rule does not authorize script-driven popups or redirects to create browsing windows. Test ordinary and `target=_blank` web links, preview-state preservation, and that the browsing window uses the ordinary browsing session without inheriting the preview's document-resource allowlist or local-file privileges: its attempts to fetch unapproved local or managed resources fail. Keep one web view per window; no tab system is required.

- [ ] **M4-09 / P0:** Define and implement preview output lifecycle alongside M4-02/M4-05/M4-08: assign ownership of generated HTML and required assets, publish complete output atomically, and retain each output generation while reload, linked-document navigation or Back navigation can still reference it. Define cleanup on preview close, conversion failure and application restart after a crash; delete only application-owned generated data, never source documents or externally owned assets. Track shared-asset references so closing one preview cannot break another. Test reload after the converter exits, failed publication, navigation back to an earlier document, two previews sharing assets, and crash-recovery cleanup. Live updates and multi-editor ownership remain deferred until their integration is introduced.

**Exit:** all M4 P0 checks pass for the documented Markdown dialect and local-resource policy: cold offline and warm-reload math/mixed-Markdown fixtures, separate MathML controls, preview/browsing storage isolation, denied unapproved file/remote access, linked-document/fragments/Back navigation, and output-lifecycle checks. The `foob:` transport must be tested before closing M4, even if earlier checks used `file://`. Unsupported converter constructs and P1 Typst/export limitations are recorded explicitly.

## M5 — Custom WPE build and controlled comparison

**Goal:** test whether removing unneeded media and other features produces worthwhile improvements **without changing the browser application**.

### Two workstreams

**M5-A: engine build and packaging**

- [ ] **M5-01 / P0:** Pin an upstream WPE release and its checksum; keep a reproducible build recipe (Arch `PKGBUILD` or equivalent) **separate from the application's Meson project**. Decide the engine's compiler flags in this recipe: frame pointers (`-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer`, as Arch's `makepkg.conf` sets them) so `perf` can follow call stacks through the engine for the memory and performance work, and the `-O3`, LTO, PGO and `-march=native` options the M1 audit postponed ([audit-m1.md](docs/validation/audit-m1.md#compiler-flags)). Frame pointers in foob's own binary were declined by the maintainer on 2026-10-17, before the Gate A decision (+24 B of text; `perf --call-graph dwarf` already works with its `-g`); add them only if profiling needs them.
- [ ] **M5-02 / P0:** Inspect the selected release's CMake options, option visibility (**public vs private**), dependencies and feature-summary output. For 2.54.1 the supported entry points include `ENABLE_WPE_PLATFORM_*`, `ENABLE_WPE_LEGACY_API`, `USE_GSTREAMER`, `ENABLE_VIDEO`, `ENABLE_WEB_AUDIO`, `ENABLE_SPEECH_SYNTHESIS`, and `ENABLE_PDFJS`; reject unverified all-flags-off commands.
- [ ] **M5-03 / P0:** Build an **unmodified-config reference** from the selected upstream WPE source where feasible, then test a reduced-feature candidate using **public/supported** switches (`USE_GSTREAMER=OFF`, `ENABLE_VIDEO=OFF`, `ENABLE_WEB_AUDIO=OFF`, `ENABLE_SPEECH_SYNTHESIS=OFF`, `ENABLE_PDFJS=OFF`), with dependency-resolution checks. Retain JS, WASM, MathML, SVG, CSS, fonts, networking, Wayland GPU rendering and security protections. Do not turn private toggles into default supported tuning options.
- [ ] **M5-04 / P0:** Preserve an easy fallback to Arch's stock WPE. Do not silently replace or overwrite the normal system libraries during exploratory builds.
- [ ] **M5-05 / P0:** Record resulting binary/package versions, enabled CMake features, runtime libraries, dependency closure, security settings and test failures.
- [ ] **M5-12 / P0:** Distribute the custom engine with the source and build recipe it was built from and the license texts of WebKit and its bundled components (D-15).

**M5-B: reproducible A/B measurements**

- [ ] **M5-06 / P0:** Run the **same browser commit and test fixtures** against both engine builds (separate binaries/prefixes are acceptable when required). Align WPE upstream source version first, where practicable; record Arch patch/build differences, and match profile/cache state, viewport, scaling, build mode, warm-up and idle time.
- [ ] **M5-07 / P0:** Compare **total process-tree PSS**, cold/warm startup, CPU/scroll behavior, installed size, dependencies and visible rendering differences.
- [ ] **M5-08 / P0:** Repeat web compatibility, local math, clipboard, uploads/downloads, JS execution and sandbox tests with the custom build.

  Repeat M4-03's fresh-process resource-policy controls and M4-05's cold offline, warm-reload and missing-asset cases on stock and custom WPE with equivalent isolated test data/cache state. Record each result separately; a warm/cached success cannot replace a cold pass. Keep Scholar persistence tests on their explicitly retained test profiles rather than applying empty-profile controls to restart-continuity checks.

- [ ] **M5-09 / P1:** Test feature options incrementally when savings appear measurable; don't drop WebGL, spellchecking, accessibility or other capabilities without a targeted compatibility test. `webkit_settings_set_feature_enabled()` and `webkit://gpu` allow these comparisons at runtime, without a rebuild. The engine defaults foob runs with today are listed in [minibrowser-comparison.md](docs/validation/minibrowser-comparison.md).
- [ ] **M5-10 / P1:** Carried from the M1 audit: revisit the JIT-off opt-in, for example only for untrusted sites (measured on stock WPE: saves 11-32 MiB, 2.3x slower on a CPU-heavy page; not adopted, D-05), measure the exact GL renderer and per-feature acceleration, and keep the under-100-MiB result with its attribution (engine mappings and heap, not foob's ~9 KB of code). `-O3`, LTO and PGO affect only foob's own tiny code.
- [ ] **M5-11 / P1:** Maintainer request (M1 guided checks, 2026-10-13): evaluate removing the accessibility feature from the custom engine build (`USE_ATK` off) to drop the `org.a11y.Bus` / `Can't connect to a11y bus` warnings every launch prints on this machine (the user service is masked) and the dependency. This reverses M5-09's rule against dropping accessibility without a test, so it needs a decision (D-13), a targeted compatibility test (keyboard-only access, screen reader, MathML exposure) and the measured savings; until then the warnings stay, `NO_AT_BRIDGE=1` does not silence them.

### Gate B — explicit optimization decision

| Outcome | Action |
|---|---|
| Meaningful savings, stable compatibility, maintainable security-update workflow | **Adopt custom WPE** for personal deployment; keep distro engine as baseline/fallback |
| Minor disk-only savings, negligible PSS savings, substantially more maintenance | **Prefer stock WPE** until there is a stronger reason to own the custom build |
| Custom engine breaks important pages or local math | Restore necessary options or return to stock; document regressions |
| Under-100-MiB target not met | Record evidence and identify real memory contributors; **do not disable sandboxing** to force the number |

No fixed percentage improvement is presumed in advance. Record repeated measurements and their spread, compatibility failures, package/dependency differences and the security-update procedure before stopping for Gate B. The maintainer decides whether the measured benefit justifies maintenance; the agent must not interpret “meaningful savings” as an automatic deployment threshold. A failed custom experiment may conclude with a documented stock-engine decision; it must not be marked as a passing custom build.

## M6 — Hardening, maintenance and packaging

- [ ] **M6-01 / P0:** Establish reproducible clean builds and smoke tests for supported Arch versions.
- [ ] **M6-02 / P0:** Establish WebKit security-update response: monitor releases, update engine pins/checksums promptly, rebuild and rerun tests. Fall back to stock WPE when the custom update lags.
- [ ] **M6-03 / P0:** Test crash handling, blocked permissions, profile isolation, URI schemes and shutdown with outstanding operations.
- [ ] **M6-04 / P1:** Package `foob` independently from the engine, with documented runtime dependencies and license notices.
- [ ] **M6-05 / P1:** Document keyboard shortcuts, PDF handoff, local-file trust model and performance methodology.
- [ ] **M6-06 / P2:** Consider advanced window management, single-instance IPC, richer research integration and further memory tuning *only after* core operation is stable. Basic separate windows required by M3 popup policy and M4 preview-link routing are already in scope; this task does not defer them.
- [ ] **M6-07 / P1:** Carried from the M1 audit: test fractional scaling and multi-monitor behavior when hardware allows (unavailable so far); keep the nested lifecycle suite (`tests/nested_lifecycle.py`) and the stripped-size and hardening checks as release gates.

**Exit:** all M6 P0 checks pass: documented clean builds/smoke tests on the current supported Arch environment, security-update/fallback procedure, and crash/security/shutdown regression evidence. List the tested environment, known limitations and P1 deferrals; additional platforms are not implied. The maintainer decides readiness for regular personal use.

---

## Test and measurement record template

Create one record per fixture/engine build, ideally committed as a small Markdown or machine-readable log:

```text
Date:
Commit:
Kernel / niri / Mesa:
WPE version + source (Arch stock | custom release/build ID):
Compiler / Meson build mode:
Page URL or local fixture + checksum:
Profile (fresh | warm) / network condition:
Viewport logical size / output scale:
Timing protocol (startup, idle interval, repetitions):
Process IDs / process tree:
Total PSS (MiB; repeated measurements):
CPU / startup / installed size (when relevant):
Resize / input / rendering / MathML results:
Warnings / crashes / regressions:
Notes / reproducibility commands:
```

For PSS, account for **all WPE child processes**, not just the top-level `foob` process; avoid double-counting shared pages. Record the collection method and use the same procedure in stock/custom comparisons. The 100 MiB stretch target concerns lightweight single-page scenarios, not rich AI apps or an arbitrary worst-case memory ceiling.

## Scope control and change rules

- **Promote tasks only to solve observed problems** or unlock an explicit accepted goal; record why.
- **Triage of findings (decided 2026-10-13).** A problem is not automatically a bug for the current milestone.
  - *Not implemented yet*, where a later task already owns the capability or will replace the current behavior: record it in the milestone that owns the area; do not patch it.
  - *A defect in behavior that already exists* is fixed in the current milestone, with a test that fails on the old build, when it would (a) propagate, because later work builds on it, or (b) stay wrong whatever later milestones deliver: a hang, crash, spin, data written without consent, or a security problem.
  - When the proper feature belongs to a later milestone but today's default is unsafe, apply the smallest safe interim default now and record the replacement requirement in the owning milestone (examples: cancelled downloads until M3-03, exit on web-process death until M3-07).
  - Every carried-forward finding gets a roadmap line that names its source.
- **Do not fork WPE** unless actual upstream changes become necessary; compilation flags are not a reason to fork.
- **Do not lock in custom WPE** until Gate B shows it has sufficient benefit.
- **Security is non-negotiable:** process isolation/sandbox, TLS validation, and local-file boundaries remain enabled/tested.
- **No hard dates or velocity estimates:** progress is driven by evidence and finished vertical slices.
- **Update this roadmap** when a milestone is closed; keep the longer architecture document for deeper rationale.
- **Keep the root planning files authoritative:** link to them from `docs/` rather than creating independently edited copies.

## Authoritative references (checked 2026-10-12)

- Arch package: https://archlinux.org/packages/extra/x86_64/wpewebkit/ — `wpewebkit` 2.54.1-2 on 2026-10-12 (**upstream 2.54.1**, **Arch revision -2**; verify locally).
- WPEPlatform native integration guide: https://wpewebkit.org/blog/2026-10-06-wpe-platform.html
- WPE FAQ and minimal launcher: https://wpewebkit.org/about/faq.html
- WPE 2.54.1 release notes: https://wpewebkit.org/release/wpewebkit-2.54.1.html
- WPEPlatform 2.54 stabilization: https://wpewebkit.org/blog/2026-09-16-wpewebkit-2.54.html
- WPE 2.54.1 supported build switches: https://raw.githubusercontent.com/WebKit/WebKit/wpewebkit-2.54.1/Source/cmake/OptionsWPE.cmake and https://raw.githubusercontent.com/WebKit/WebKit/wpewebkit-2.54.1/Source/cmake/GStreamerDefinitions.cmake
- Deep design rationale, API and security notes: `foob_architecture_decision.md` (companion document).
