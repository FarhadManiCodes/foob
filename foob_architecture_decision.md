# Architecture Decision: Minimal Wayland Browser

**Status:** Architecture direction accepted. Project state: [handoff](docs/validation/handoff.md).
**Date:** 2026-10-12  
**Working name:** `foob` (not final)  
**Decision owner:** Project maintainer

## Decision summary

Build a **minimal, keyboard-driven, chrome-free Wayland web browser** in **C23** using **WPE WebKit with WPEPlatform**. For now, **do not pursue Servo**. The browser renders web content but provides no persistent address bar, toolbar, tab strip, or other browser chrome. Browser control comes from keyboard shortcuts and, where useful, a command-line/IPC interface.

The application and the web engine have **separate build and release lifecycles**. Develop the application against WPE's supported public APIs. Start with the distribution build to establish a working baseline, then **evaluate** a custom-built WPE package as an optional optimized deployment. Adopt it only if the benchmark, compatibility and security-maintenance results justify it. Do **not** fork WebKit, check WebKit into this repository, or add it as a Git submodule solely to modify build flags.

A custom build **does not require a source fork**: obtain a pinned upstream release, verify the source, configure supported CMake switches, build, and package it independently. Keep the stock build available for comparison and troubleshooting.

**Execution authority:** the root [development roadmap](foob_development_roadmap.md) defines task order, required acceptance evidence and maintainer review stops; this document defines technical boundaries and rationale. Its broader API/feature investigations apply when their milestone is reached, not as prerequisites to the M0/M1 skeleton. Gate A, the post-M2 Scholar decision and Gate B require maintainer decisions before progression. Technical success does not let an agent approve its own gate. Historical qvu documents do not add implementation scope.

**Decisions since this document:** the maintainer's decisions (trust model, window title, app-id, exit codes, the interim defaults for downloads, new windows and PDFs, the license) are logged in [findings.md](docs/validation/findings.md), and `AGENTS.md` states the ones agents must apply. Where this document describes a target that a decision postpones, the decision holds until its roadmap task.

## Versions

| Term | Meaning |
|---|---|
| WPE WebKit **2.54.0** | Upstream engine release that made WPEPlatform stable and the default (2026-09-16) |
| WPE WebKit **2.54.1** | Upstream stable bug-fix release used as the baseline (2026-10-06) |
| Arch **`wpewebkit 2.54.1-2`** | Arch package of 2.54.1; `-2` is Arch's packaging revision, not an upstream release |
| **`wpe-webkit-2.0`** | WPE WebKit API and pkg-config name; `2.0` is the API namespace, not the engine release |
| **`wpe-platform-2.0`** | WPEPlatform API and pkg-config name, same namespace |
| **`wpe-platform-wayland-2.0`** | Optional Wayland-specific pkg-config module |

Keep the three apart in code, build instructions and evidence.

What M1 verified on this machine: [m1.md](docs/validation/m1.md); memory and GPU: [blank-memory.md](docs/validation/blank-memory.md).

## Target machine and language/runtime decision

- **Hardware:** x86_64 laptop with an integrated AMD GPU (`amdgpu` driver).
- **Operating environment:** Arch Linux, native Wayland session under **niri**.
- **Application standard:** **C23** (`c_std=c23` in Meson). Prefer standard C23 language features where practical; GLib convenience macros such as `g_autoptr` themselves depend on compiler-specific cleanup attributes, so do not imply a strictly extension-free ISO C build. C23 is an application compilation choice, independent of WPE's C ABI.
- **Event loop and object integration:** **GLib/GObject** because WPE's supported C API exposes GObject types, signals and GLib asynchronous/event-loop integration. GLib is **not** GTK and does not add a visible GUI toolkit. Prefer ordinary C for application-owned data and business logic, using GLib/GObject at the WPE integration boundary.

## Goals

- Native Wayland operation, with no Qt or GTK browser UI framework.
- Modern JavaScript, HTML/CSS, DOM, HTTPS, cookies/storage, WebAssembly, SVG, MathML, and adequate font/text rendering.
- Primary destinations: Wikipedia, Google Scholar, Semantic Scholar.
- Local mathematical HTML and Markdown rendered with KaTeX (bundled locally), plus suitable Typst HTML/MathML exports; use a separate PDF/document viewer for exact Typst layout and papers.
- Open paper links, download PDFs, and hand them to the separate document viewer.
- Optionally support ChatGPT, Claude, and Gemini if compatibility tests succeed.
- Small dependency footprint, predictable resources, maintainable packaging, and automated performance measurements.
- Stretch target: **under 100 MiB total PSS** for one lightweight page, including all browser subprocesses; this is *not* a measured result or a guaranteed ceiling.

## Non-goals (initially)

- Servo integration or interchangeable engine adapters.
- A full graphical browser interface or extension ecosystem.
- An embedded PDF renderer as the target (interim: the engine's PDF.js viewer, D-12), video/audio playback, WebRTC calls, microphone/camera, DRM streaming, gamepad support.
- A source-level fork of WebKit or modifications to upstream engine internals.
- Compiling the entire browser engine every time the application is rebuilt.
- Guaranteeing full compatibility with all complex web applications.

## Engine choice and rationale

**Selected:** WPE WebKit **2.54.1** as the upstream baseline, with the WPEPlatform Wayland backend (stable by default since 2.54.0).

WPE is an official WebKit port suited to embedded and toolkit-independent deployments. It preserves a mature browser engine while avoiding the need to base the application on a conventional GTK/Qt frontend. "Embedded" does **not** mean the engine is fundamentally simpler than WebKit: the modern JS, DOM, CSS, networking and multiprocess core remains substantial. Its platform focus nonetheless aligns with this browser's constraints.

**Deferred:** Servo. Its component-oriented architecture is interesting but its embedding API and modern-site compatibility are less mature. Reconsider only if WPE fails material compatibility/memory criteria or Servo's maturity changes significantly.

**Not selected:** Forcing a fork of Cog (legacy integration and unnecessary downstream maintenance); WebKitGTK/webview wrapper (adds GTK dependency); CEF (Chromium-scale engine); litehtml/QTextBrowser (insufficient for modern interactive sites). The comparison reference is upstream MiniBrowser (D-14).

## System architecture

```text
CLI / keyboard / optional IPC
             |
   Browser application (C23, GLib/GObject)
   - navigation and link policies
   - persistent profile / sessions
   - downloads and external PDF opener
   - keyboard bindings / window-state controls and sizing preferences
             |
    WPE WebKit public API
             |
    WPEPlatform Wayland
             |
       Wayland compositor
```

The **webpage occupies the whole browser window**. First implement one web view per window. Tabs, composited overlays, and multi-view management are deferred until actual requirements justify them; do not assume multi-view composition is trivial without another UI layer.

foob is Wayland-only for interactive use: it builds `WPEDisplayWayland` directly, so the `WPE_PLATFORM` environment variable has no effect. A headless display (idea I-02) needs a maintainer decision.

Use one persistent browser profile for ordinary browsing storage and logins. M4 managed previews use a separate ephemeral network session; a user-facing private-browsing mode remains optional future work. Handle downloads via WebKit's download/session APIs where needed so authenticated links remain functional. The target is an external PDF viewer rather than a built-in renderer (interim: the engine's PDF.js viewer, D-12; roadmap M3-04).

## Window lifecycle, resizing, and scaling (required from first prototype)

**Decision:** Native Wayland window behavior is a first-class acceptance criterion, especially under the **niri** tiling compositor. Do not assume the web viewport automatically follows window sizes without verifying the WPEPlatform integration.

**Responsibility split:**

- **niri / Wayland compositor:** chooses and announces the configured toplevel size/state (including tiling, fullscreen, maximize, and output changes). The client must respect compositor configurations rather than impose fixed dimensions.
- **WPEPlatform's built-in Wayland backend:** manages its native Wayland surface/toplevel and the low-level Wayland configure and rendering integration. Use this backend through its public `WPEView` / `WPEToplevel` APIs; **do not implement a second xdg-shell backend or manually forward Wayland configure events** when the built-in backend already handles them.
- **Browser shell:** owns startup sizing preferences, optional user resize/maximize/fullscreen shortcuts, tracks toplevel state, and tests that the view resizes correctly. Use `wpe_view_get_toplevel()` and supported `wpe_toplevel_*` methods/signals; use `toplevel-state-changed` if the UI/application needs state notifications. Avoid manually resizing the `WPEView` unless public API documentation requires it for the selected integration.

**Required behavior:**

1. At startup, show a correctly sized page using compositor-approved dimensions. An initial requested size is merely a preference, not a guarantee.
2. When a niri tile is resized by keyboard or pointer, the HTML viewport and pointer/keyboard coordinates must follow without cropping, black strips, or stretching a stale frame.
3. Fullscreen/maximize/unmaximize transitions and moving a window between differently scaled displays must render at the right logical dimensions and pixel scale; include fractional scaling where available.
4. Check CSS viewport-dependent layout (`vw`, `vh`, responsive breakpoints), `window.resize` notifications, scrolling, selection, and input positioning.
5. Avoid resize loops, repeated unsolicited size requests, and unnecessary recreation of the WebKit web view on every change.
6. Record the actual **logical viewport size and output scale** alongside every memory/graphics benchmark. Fixed dimensions are necessary for fair stock-vs-custom engine comparisons.

**API evidence:** WPEPlatform 2.54 documentation says the Wayland `WPEToplevel` supports resizing, maximize, fullscreen, and state signals; its built-in Wayland view source already observes toplevel-size changes and forwards them to the view. The browser must exercise and validate these behaviors rather than reimplementing the backend. See https://wpewebkit.org/blog/2026-10-06-wpe-platform.html and https://github.com/WebKit/WebKit/blob/main/Source/WebKit/WPEPlatform/wpe/wayland/WPEViewWayland.cpp .

## Repository and packaging strategy

**Application repository:** only the application's source, tests, fixtures, configuration, and packaging/build *recipes*. No vendored WebKit source.

The current modules are listed in `AGENTS.md`. The custom WPE build recipe lives in `packaging/arch/wpe-minimal`.

If the custom WPE packaging becomes complex, move `packaging/arch/wpe-minimal` to a **separate repository**. This is a build recipe, *not* a WebKit submodule or source fork.

Do not duplicate the root architecture or roadmap as independently edited documents under `docs/`.

**Development workflow:**

1. Build the browser against the normal, maintained Arch WPE package for a compatibility and performance baseline.
2. Construct a separate Arch-style package from a pinned, checksummed upstream WPE release and selected supported CMake options.
3. Verify the custom build in an isolated environment/prefix first; avoid accidentally replacing libraries relied upon by other applications.
4. Build exactly the same browser commit against each engine variant; benchmark and compare.
5. Use the custom package for the preferred deployment **only if** it passes functionality tests and provides meaningful benefits.
6. Track security updates; rebuild and retest the custom package promptly. Fall back to the stock engine if a custom configuration breaks or lags on security updates.

**Important:** A custom engine build is the intended *optimization path*, not an assumption that every developer must build WebKit before building the small browser application.

## Custom WPE configuration: first candidate

Keep configuration changes small and declarative. Verify names, compatibility, and their resolved values against the **exact pinned WPE release**; upstream CMake options can change. In the verified 2.54.1 source, `OptionsWPE.cmake` explicitly separates **PUBLIC supported configuration options** from **PRIVATE options whose modification is unsupported**. Do not use a private option as a production tuning knob merely because CMake accepts `-D` for it.

| Area | Verified 2.54.1 status / intended setting | Reason |
|---|---|---|
| WPE port | `-DPORT=WPE` for the upstream WebKit CMake tree | Select WPE port |
| WPEPlatform | `ENABLE_WPE_PLATFORM=ON` (**public**, default ON) | Stable native API |
| Wayland backend | `ENABLE_WPE_PLATFORM_WAYLAND=ON` (**public**, default ON) | Native niri integration |
| DRM/KMS backend | `ENABLE_WPE_PLATFORM_DRM=OFF` (**public**) | Not needed on this machine |
| Headless backend | `ENABLE_WPE_PLATFORM_HEADLESS=ON` initially (**public**) | Useful for automated tests; disable later only if justified |
| Legacy API | `ENABLE_WPE_LEGACY_API=OFF` after validation (**public**) | Remove legacy libwpe integration |
| GStreamer | `USE_GSTREAMER=OFF` (**public** in `GStreamerDefinitions.cmake`) | Test multimedia-free dependencies |
| HTML video / Web Audio | `ENABLE_VIDEO=OFF`, `ENABLE_WEB_AUDIO=OFF` (**public** in GStreamer definitions) | Primary supported media removals; verify their dependencies |
| Speech synthesis | `ENABLE_SPEECH_SYNTHESIS=OFF` (**public** for WPE) | Avoid media/speech dependencies |
| Embedded PDF.js | `ENABLE_PDFJS=OFF` (**public** for WPE) | Target: the external viewer; the stock viewer stays for now (D-12, M3-04) |
| WebDriver | `ENABLE_WEBDRIVER` (**public** for WPE): decide based on test automation | Do not disable test support prematurely |
| Documentation / introspection | `ENABLE_DOCUMENTATION`, `ENABLE_INTROSPECTION` (**public** for WPE): possible packaging choices | Verify development tooling does not require them |
| Media stream / WebCodecs / WebRTC / media capture | **Private** engine features; prefer automatic dependency resolution and verify resulting feature manifest rather than setting unsupported private flags | Preserve upstream-supported configuration |
| WebGL, MathML, JS/JIT, WebAssembly, GPU process | Treat as engine functionality to **preserve/test**, not a blanket list of public CMake toggles; many are private | Web and math compatibility; graphics and security |
| Bubblewrap process sandbox | `ENABLE_BUBBLEWRAP_SANDBOX=ON` (**public** on Linux WPE; default ON) | Security must remain enabled and verified |
| Accessibility and spellcheck | Keep spellcheck (`ENABLE_SPELLCHECK`); removing accessibility (`USE_ATK`, both public WPE options) is under evaluation (D-13, roadmap M5-11) | Preserve desktop usability |

Sources inspected: [`OptionsWPE.cmake`](https://raw.githubusercontent.com/WebKit/WebKit/wpewebkit-2.54.1/Source/cmake/OptionsWPE.cmake), [`GStreamerDefinitions.cmake`](https://raw.githubusercontent.com/WebKit/WebKit/wpewebkit-2.54.1/Source/cmake/GStreamerDefinitions.cmake), [`GStreamerDependencies.cmake`](https://raw.githubusercontent.com/WebKit/WebKit/wpewebkit-2.54.1/Source/cmake/GStreamerDependencies.cmake) and [`WebKitFeatures.cmake`](https://raw.githubusercontent.com/WebKit/WebKit/wpewebkit-2.54.1/Source/cmake/WebKitFeatures.cmake). `ENABLE_VIDEO`, `ENABLE_WEB_AUDIO`, `ENABLE_WEB_CODECS` and speech synthesis depend on `USE_GSTREAMER` according to the dependency rules; dependent features can be disabled automatically. **Do not blindly apply a copied CMake invocation:** inspect configuration output, CMake cache, link dependencies and real feature behavior. The exact candidate has **not** been built.

Disabling audio/video can reduce installed size and transitive dependency requirements, but may have only a modest effect on idle PSS for Wikipedia. GPU compositing and video decoding are separate concerns: keep graphics acceleration unless measurements justify otherwise.

## Benchmark and acceptance plan

**Google Scholar** is a primary destination and the main compatibility risk (blocking was seen in the earlier Qt work). WPE is judged on Scholar only after M2's persistent profile exists, with the default identity and no spoofing, imported cookies or weakened security. A single challenge does not reject the engine; repeated blocking that a contemporaneous Firefox does not see calls for an engine decision. Method and acceptance: roadmap M2-08, the Scholar decision before M3, and M3-08.

For **both stock and custom WPE**, use the same browser **source revision** and behavior, window size, sites, cache state, and measurement method. The executable may need rebuilding or relinking per engine prefix. For a controlled feature comparison, initially target the **same upstream WPE source version** and as similar build settings as practical; record Arch downstream patches and toolchain differences rather than attributing them to stripping.

| Workload | Functional check | Performance check |
|---|---|---|
| Blank window | Startup, close, keyboard input | Startup time; total PSS |
| Tiled/resized window (niri) | Initial configure, keyboard resize, maximize/fullscreen, responsive viewport | Correct viewport and scale; no resize loops |
| Wikipedia | Navigation, links, scrolling, fonts | Idle PSS; scrolling CPU |
| Google Scholar | Search form, result links, PDFs, any anti-bot behavior | Idle PSS |
| Semantic Scholar | Page layout, dynamic JS, links | Idle PSS |
| Local Markdown + KaTeX | Equations, fonts, selection/copy | Idle PSS |
| Typst HTML + MathML | Equation correctness | Idle PSS |
| PDF download | Download, filenames, progress, handoff to viewer | Peak memory; success |
| ChatGPT/Claude/Gemini (optional) | Login, streaming, math, uploads | Comparative PSS; no fixed 100 MiB limit |

Measure **total proportional set size (PSS)** of the full application process tree after a defined idle period. Also record startup time, CPU, installed dependency closure, compressed/uncompressed package sizes, and any GPU allocations PSS doesn't capture. The **100 MiB total-PSS target applies to lightweight single-page workloads only**; rich web applications will generally exceed it.

**Preview benchmark scope (M4-07):** report browser process-tree PSS after rendering separately from conversion time, converter memory and total source-to-usable-preview time. Identify helpers that exit versus remain running and disclose measurement limitations. The [qvu math comparison](docs/research/qvu/findings/math-renderer-comparison.md) excluded conversion from browser totals; do not interpret browser savings alone as reduced workflow cost. The under-100-MiB stretch target remains browser PSS, with conversion costs reported alongside it.

## Maintenance and security policy

- Follow upstream WPE/WebKit releases; treat browser-engine security updates as urgent.
- Maintain a small set of public build-option overrides, not a rebased WebKit patch stack.
- Pin source versions and checksums for reproducible packages, but update pins promptly.
- Keep the public WPE API boundary clean; don't depend on private engine internals.
- Preserve a stock-engine fallback when custom builds fail or delay security updates.
- Don't weaken sandboxing, TLS validation or process isolation to meet a RAM number.
- Periodically reevaluate whether the custom build yields enough savings to justify ownership of engine packaging.

## Browser-shell responsibilities and design audit (2026-10-12)

The browser is deliberately chrome-free and does not use a conventional GUI toolkit. **That does not remove browser-shell responsibilities.** Below, **P0** denotes core browser capabilities to complete by their **assigned milestone**, **P1** means follow-up quality/functionality, and **P2** means deferred. The roadmap's specific task priorities and exit checklists control completion; this overview does not downgrade a roadmap P0 task. **P0 here does not mean every item blocks the M1 first-window smoke test**; M1 has its own smaller acceptance gate. Recommendations are provisional until exercised against the chosen WPE release. For each item, prefer upstream WPEPlatform/WebKit API before introducing custom Wayland protocols, GLib wrappers, or a graphical toolkit.

| Priority | Subsystem / question | Proposed policy | Acceptance test / research needed |
|---|---|---|---|
| P0 | **Keyboard shortcuts vs website input** | Intercept only explicitly reserved shortcuts via WPEPlatform input events; forward all other input. Avoid collisions with Ctrl+C/V, editing, web apps, and compositor shortcuts. | URL prompt, navigation, focus on an input, copy/paste, modifier behavior; test in niri. |
| P0 | **Focus and keyboard/text input** | Rely on WPEPlatform's view/input-method integration where possible; don't assume key events alone are sufficient for text composition. | Compose/dead keys, Unicode, keyboard repeat, caret/selection, focused forms; investigate input-method protocols and IME behavior if needed. |
| P0 | **Wayland clipboard and selection** | Use platform/WebKit clipboard mechanisms; support plain text and common content types, copy links and page selections. | Copy rendered text/MathML, paste into HTML text field, exchange with foot and other apps; verify MIME-type and image paste behavior. |
| P0 | **Dialogs, dropdowns, and new windows** | Use WPE's built-in option popup implementation where available. Explicitly handle JS alert/confirm/prompt, `window.open`, target=_blank, context menus, and failed popup requests. Target: no silent loss of website actions (interim: every new-window request is ignored, D-04; feedback is M2-12). | HTML test fixture with each dialog and select control; confirm placement, keyboard focus and dismissal. |
| P0 | **Profile and storage identity** | M2 persistent browsing session in XDG directories; M4 separate ephemeral managed-preview session. User-facing private mode remains deferred. Define ownership of cookies, cache, IndexedDB, localStorage, service workers and permissions. | Restart retains browsing state; preview cleanup preserves browsing data and leaves no persistent preview site state; test windows sharing the intended session. |
| P0 | **TLS, permissions, sandbox, origin restrictions** | Fail closed on certificate errors; preserve WebKit process sandbox and site isolation as offered; default deny or explicitly prompt sensitive permissions. No broad local-file or localhost permissions by default. | Certificate error handling, cross-origin isolation tests, sandbox status, permission signals and logging. |
| P0 | **Downloads and file uploads** | Use WebKit download/chooser APIs and XDG desktop portals (or a deliberately chosen alternative) without GTK. Enforce safe destination/name handling; authenticate downloads through WebKit session. | PDF download, download cancellation, collision handling, input type=file upload, multiple files, drag/drop if desired. |
| P0 | **Window state / resize / scale** | Keep the existing dedicated window lifecycle policy. Do not duplicate Wayland configure handling inside WPEPlatform. | Tiled resize, maximized, fullscreen, move between displays, fractional scaling, CSS viewport correctness. |
| P1 | **Navigation surface without chrome** | Decide between `foob https://…`, external launcher (e.g. fuzzel), stdin/IPC, and a transient in-page/overlay prompt. Treat shell input as **URL/search**, with proper escaping and untrusted-input boundaries. | Open URL from terminal, search query, history back/forward/reload, display current URI, copy current URI using a shortcut. |
| P1 | **Navigation policy and URI schemes** | HTTP(S) internally; PDFs/downloads to external reader; decide explicit handlers for mailto, tel, file, blob, data, and custom schemes. Guard against untrusted sites launching arbitrary commands. | PDF links and redirects, target=_blank, mailto, local documents and failed schemes. |
| P1 | **Local research documents and math** | Define trusted local content origin and resource resolution (KaTeX fonts/scripts, MathML and Typst HTML); avoid casually disabling web security for local HTML. | Local KaTeX without network; local links/images; no unintended local-file exposure to remote webpages. |
| P1 | **Site compatibility and browser identity** | Test default user agent, feature detection, CSP/service workers, sign-in redirects, popup authentication, WebAuthn/passkeys where relevant. Do not change user-agent globally before observing failures. | Wikipedia, Scholar, Semantic Scholar, ChatGPT, Claude, Gemini representative flows. |
| P1 | **Appearance, zoom, font and text rendering** | Distinguish CSS/device scale, page zoom, reader magnification and browser UI absence; support keyboard zoom and default font preferences. Follow system dark-mode/contrast/reduced-motion settings where supported. | Small math labels, ligatures, non-Latin text, system theme changes, 125–200% scale, resize after zoom. |
| P0 | **Basic reading controls (M2-09)** | Provide keyboard page zoom in/out/reset and find-in-page with query entry, next/previous match and dismiss. Investigate WPE event dispatch and public find/zoom APIs before selecting a transient query interaction; reuse controls for local previews in M4. | Unicode/no-match/repeated-match searches, focus restoration, form/IME input preservation, zoom reset and math legibility. |
| P1 | **Webprocess crashes and application recovery** | Target: log crashed subprocesses, show or load a minimal recovery page and support reload, so a page crash does not end the application (interim: foob exits with code 4, D-06; roadmap M3-07). | Deliberately terminate a web process; check browser remains controllable and recoverable. |
| P1 | **Single-instance behavior and IPC security** | Initially permit multiple independent windows/processes; choose later whether URL commands go to an existing instance. IPC endpoint, if implemented, should be per-user, local, permission-restricted. | Launch second URL; check no unexpected shared locks/data corruption; reject other-user IPC. |
| P1 | **Startup, termination and subprocess cleanup** | Handle SIGTERM/interrupt cleanly, orderly WebKit shutdown and download cancellation/continuation policy; no leaked child processes. | Quit during page load and download, inspect process tree and persistent state. |
| P1 | **Desktop integration without toolkit** | Determine whether an XDG portal or external command supplies file chooser, open-with and notifications. Do not assume portals are always present under niri. | File upload and external PDF opening in actual niri session, including missing-portal fallback. |
| P1 | **Resource control beyond media flags** | Track memory per web/network/GPU process, caching and background pages; establish GPU memory observability and process limits without weakening security. | Blank/Wikipedia/math benchmarks, cold/warm startup, idle after navigation, leaks after repeated opens/closes. |
| P2 | **Multiple tabs/web views** | Remain one web view per window first; revisit shared profile, process model and keyboard window switching after prototype. | Validate whether tab semantics are needed beyond niri's own windows. |
| P2 | **Accessibility and automation** | Removing accessibility from the custom engine is under evaluation (D-13, M5-11); use WebDriver or headless builds for regression tests if justified. | Keyboard-only access, accessibility-tree exposure, repeatable tests. |
| P2 | **Browser conveniences** | History/bookmarks, printing, saved pages, devtools, reader mode, ad/tracker blocking, password-manager integration, extensions. | Prioritize based on real workflows; do not treat these as included by default. |

### Implementation implications

- Add a **small `input.c` / `shortcuts.c` boundary** before mixing application shortcuts with content text input. Input events can be inspected on WPEPlatform `WPEView`'s `event` signal and consumed explicitly.
- Add **`policies.c`** for navigation, new-window, permission and external URI decisions. Keep *navigation policy* separate from *download handling* and *launching external programs*.
- Add **`dialogs.c`** only for behavior not already supplied by WPE (notably website JavaScript prompts and file-upload chooser integration). Recent WPE already has built-in select/option popup support; verify it instead of writing a duplicate.
- Add **`session.c` / `profile.c`** for WebKit website data manager and per-profile directories. Reuse WebKit's cookie, proxy and storage APIs rather than reimplementing them.
- Add a **small test-fixture suite** (HTML pages for forms, popup menus, file uploads, JS dialogs, resize, clipboard, MathML/KaTeX, navigation and resource cleanup). This is the cheapest way to expose shell integration gaps early.
- For GUI-like system functions on niri, **research XDG Desktop Portal support and availability** separately rather than assuming GTK is necessary or introducing GTK for a file chooser.

### Decisions to research before the corresponding feature implementation

Schedule these checks by the roadmap: native view/input at M1/M2, persistent browsing at M2, dialogs/portals at M3, local origins at M4 and custom-engine security at M5. Do not delay M0/M1 to resolve later-stage choices.

1. **WPEPlatform 2.54 exact Wayland coverage:** clipboard MIME types, IME/text-input, drag-and-drop, popup placement, file chooser and compositor-supported scaling. The existence of a WPEPlatform class does not prove that a particular Wayland backend implements every operation.
2. **WebKit 2.0 API and callback contracts:** navigation/new-window policy, JS dialog signals, file chooser, permission prompts, download destination selection, website data persistence, web-process termination. Decide which default behaviors exist and which require application code.
3. **niri / portal environment:** active `xdg-desktop-portal` backends and whether GTK-free file picking is feasible without unwanted heavyweight runtime dependencies. Portal support may require a backend installed separately.
4. **Security profile:** WebKit sandbox requirements and supported permissions on the target Arch build, plus how custom WPE packaging changes process permissions and upgrade obligations.
5. **Baseline site behavior:** sign-in flows and new-window/redirect policies on the actual research and AI websites. Avoid designing from assumptions about compatibility.

### Verified upstream observations and sources

- WPEPlatform 2.54 stable API includes `WPEView` input events and platform facilities for input methods, keymaps, clipboard, gestures and accessibility. WPE's example shows applications consuming keyboard events at the `WPEView` `event` signal: https://wpewebkit.org/blog/2026-10-06-wpe-platform.html .
- WPE 2.53.90 added a built-in HTML select/option popup implementation when applications don't handle the corresponding `show-option-menu` signal: https://wpewebkit.org/release/wpewebkit-2.53.90.html .
- WPE 2.54.1 contains Wayland compatibility, paste-from-clipboard and IME fixes; target at least that maintenance level for initial tests: https://wpewebkit.org/release/wpewebkit-2.54.1.html .
- Upstream WPE MiniBrowser is useful API-reference code for profile storage, cookie policy and proxy configuration; the comparison baseline is [minibrowser-comparison.md](docs/validation/minibrowser-comparison.md): https://github.com/WebKit/WebKit/blob/main/Tools/MiniBrowser/wpe/main.cpp .

## P0 API investigation — WPE WebKit 2.54.1 / WPEPlatform 2.0 (2026-10-12)

**Evidence level:** The inventory comes from the upstream **2.54.1** headers, CMake definitions and release notes; each API still needs its milestone's test. Prefer the new `g_object_new(WEBKIT_TYPE_WEB_VIEW, ...)` construction path, not the legacy `webkit_web_view_new()` or libwpe backend dispatch.

### P0-A. Keyboard, focus and browser shortcuts

- **Upstream handles:** Wayland keyboard events and dispatch to the `WPEView` and web page; usual web editing/focus behavior.
- **Public hook:** `g_signal_connect(wpe_view, "event", ...)`, retrieving the view via `webkit_web_view_get_wpe_view(web_view)`; inspect `wpe_event_get_event_type()`, `wpe_event_get_modifiers()` and `wpe_event_keyboard_get_keyval()`. `WPE_EVENT_KEYBOARD_KEY_DOWN` + `WPE_MODIFIER_KEYBOARD_CONTROL` and `WPE_KEY_q` are used in the upstream example.
- **Application policy:** Consume only explicitly reserved shortcuts by returning `TRUE`; return `FALSE` for ordinary typing and default web handling. Define an intentional URL-entry mechanism (e.g. optional temporary prompt, CLI or IPC) without persistent chrome. Avoid stealing familiar editor shortcuts from ChatGPT/contenteditable; niri's compositor shortcuts take precedence and are not available to the client.
- **Acceptance:** Type Unicode into input/textarea/contenteditable; Ctrl+C/V, page shortcuts, back/forward, Esc, modifier release and focus changes. Verify shortcut behavior when a form has focus.
- **Reference:** https://wpewebkit.org/blog/2026-10-06-wpe-platform.html

### P0-B. Clipboard and text input

**Reading-control investigation (M2-09):** use the WPEView event-signal integration as the starting point for shortcut handling, then verify consumption/propagation, modifier state, focused forms/contenteditable and IME composition in a small probe. Confirm supported public WebKit find and page-zoom APIs separately from keyboard dispatch: detecting a shortcut does not provide a query-entry interface. Select the simplest toolkit-free transient interaction after these checks, without committing to a conventional toolkit prompt or a persistent control. Preserve ordinary webpage input and restore focus after search dismissal. Signal-based handling may enable a simpler implementation, but does not itself establish lower latency or resource use; measure any optimization claim. Find-in-page is an initial M2 reading feature, reused in M4, rather than a deferred browser convenience.

- **Upstream handles:** WPEPlatform Wayland clipboard integration and web selection/paste; WebKit 2.54.1 specifically fixes pasting in-memory image data and several input-method issues.
- **Application policy:** Do not write a second clipboard subsystem for ordinary page copy/paste. Use the platform implementation; add an explicit browser command to copy the current URL only if needed. Clipboard transfers (plain text, HTML, image) and primary selection are distinct features—verify what works instead of assuming equivalence.
- **Acceptance:** Copy/paste with native Wayland apps; text, Unicode/compose, image paste into supporting web apps; focus changes between input and webpage; verify primary selection separately. Test niri/Wayland clipboard persistence after source application exits.
- **References:** https://wpewebkit.org/release/wpewebkit-2.54.1.html and https://wpewebkit.org/reference/stable/wpe-platform-2.0/

### P0-C. Popups, JavaScript dialogs and permissions

**Requested versus current URL (M2-06):** retain the user-requested navigation target separately from the committed URL so redirects cannot erase the intended search. Update it on deliberate navigation, use it for explicit recovery or future handoff, and redact sensitive queries in routine diagnostics. The [qvu investigation](docs/research/qvu/findings/scholar-readonly-investigation.md) motivates this distinction; it does not introduce browsing history or external-browser integration.

**Diagnostic sequence (M2-06):** distinguish initial site rejection, challenge loading/interaction failure and later loss of clearance. In bounded diagnostics, record navigation/redirects, response status where available, resource failures and subsequent JavaScript errors in order, using verified WPE hooks and disclosing observation gaps. The [qvu navigation findings](docs/research/qvu/findings/scholar-navigation-and-dooble.md) recorded rejection before challenge-page errors; error presence alone does not establish cause. Exclude sensitive URLs, cookies and request bodies from routine logs.

- **WebKitWebView signals:** `script-dialog`, `create` (new-window request), `close`, `ready-to-show`, `show-option-menu`, `permission-request`, `authenticate`, `decide-policy`, and potentially `context-menu` / `run-as-modal`.
- **Upstream handles:** `<select>` option menus have a built-in default implementation since the 2.53.90 cycle. This does **not** establish that JS `alert()`/`confirm()`/`prompt()`, `window.open()`, permission UI or HTTP authentication are handled satisfactorily with no shell UI.
- **Application policy:** Treat JS dialogs, HTTP-auth challenges and user-permission requests as distinct types. Provide a minimal asynchronous prompt/response policy, initially deny unrequested sensitive permissions; never silently grant. Decide whether popups open a separate window or are blocked with an explicit way to allow them. Do not conflate `WebKitWebView::create` with `WebKitWebView::script-dialog`.
- **Acceptance:** Test JS alert/confirm/prompt (including cancel), `<select>`, popup requests, HTTP authentication, site permission requests, modal-focus and accidental repeated prompts. Verify default signal behavior experimentally before relying on it.
- **References:** https://wpewebkit.org/reference/stable/wpe-webkit-2.0/class.WebView.html and https://wpewebkit.org/release/wpewebkit-2.53.90.html

### P0-D. Persistent authentication, cookies and isolation

- **Public API:** `webkit_network_session_new(data_directory, cache_directory)` creates a persistent session; `webkit_network_session_new_ephemeral()` creates an in-memory/nonpersistent session. Provide the chosen `WebKitNetworkSession` via the `WebKitWebView` `network-session` construct property. Use `webkit_network_session_get_cookie_manager()` and `webkit_network_session_get_website_data_manager()`; `WebKitWebsiteDataManager` can inspect/remove site data. Network session also exposes `webkit_network_session_set_persistent_credential_storage_enabled()` and intelligent tracking prevention settings.
- **Application policy:** Dedicated XDG paths (`$XDG_DATA_HOME/foob`, `$XDG_CACHE_HOME/foob`) rather than a shared default profile. Restrict file permissions. Keep a single normal profile initially and do not accidentally initialize ephemeral sessions for login-required sites. Browser history/session restoration (`webkit_web_view_get_session_state` / `webkit_web_view_restore_session_state`) is **separate** from cookie persistence and optional.
- **Acceptance:** In M2, confirm persistence with a controlled storage fixture and Scholar restart tests; representative login behavior is tested in M3 where applicable. Verify storage isolation from other WebKit apps. In M4, confirm the ephemeral preview session leaves no persistent site state and preview cleanup preserves browsing data. User-facing private-mode/data-management UI remains deferred.
- **References:** https://wpewebkit.org/reference/stable/wpe-webkit-2.0/class.NetworkSession.html and https://wpewebkit.org/reference/stable/wpe-webkit-2.0/class.WebsiteDataManager.html

### P0-E. Security and TLS

- **Public API:** `WebKitNetworkSession` provides `webkit_network_session_get_tls_errors_policy()` and `webkit_network_session_set_tls_errors_policy()`, plus a narrowly scoped certificate exception API. `WebKitWebView` emits `load-failed-with-tls-errors`. `WebKitWebContext` and `WebKitSecurityManager` expose further site policy configuration. **Important version distinction:** `webkit_web_context_get_sandbox_enabled()` / `set_sandbox_enabled()` are guarded out of the WPE **2.0 / 2022 GLib API**; do not instruct M3 to call these legacy accessors. Check sandbox operation through the build/runtime instead.
- **Application policy:** Keep strict TLS checking; do not globally ignore certificate errors or disable sandboxing to simplify the custom build. Preserve normal origin/security boundaries and process isolation. Block/confirm potentially unsafe URI schemes and `file://` access; choose explicit policy for local docs. Pin and update upstream security fixes promptly; verify actual web-process sandboxing and runtime permissions on Arch rather than assuming it is active.
- **Acceptance:** Invalid TLS certificates fail, HTTPS works, isolated storage, unexpected file:// links cannot read arbitrary local files through browser shell, and runtime subprocess confinement is inspected. Security build flags require independent verification against the pinned upstream source.
- **References:** https://wpewebkit.org/reference/stable/wpe-webkit-2.0/class.NetworkSession.html and https://wpewebkit.org/security/WSA-2026-0006.html

### P0-F. Upload, downloads and safe file paths

- **Upload API:** `WebKitWebView::run-file-chooser` with `WebKitFileChooserRequest`; inspect `webkit_file_chooser_request_get_mime_types()` and `_get_select_multiple()`, fulfill via `webkit_file_chooser_request_select_files()`, or cancel via `_cancel()`.
- **Upload UI choice still open:** An XDG desktop portal file chooser is a candidate for a toolkit-free shell, but must be integrated and tested (including the response, URI-to-path handling, cancellation, multiple selections and portal availability under niri). **Do not assume** the published reference's GTK default file-chooser description applies to a toolkit-free WPE application; an upstream doc section appears shared with WebKitGTK. Fallback may be an explicit keyboard/terminal picker but must be safe and nonblocking.
- **Download API:** `WebKitNetworkSession::download-started`, `WebKitDownload::decide-destination`, `webkit_download_set_destination()`, `WebKitDownload::received-data`, `failed`, and `finished`. Preserve request cookies via WebKit, avoid reimplementing authenticated downloads with cURL. Sanitize suggested filenames, avoid silent overwrites (`allow-overwrite` should remain false), and open PDFs externally only after successful completion and explicit policy.
- **Acceptance:** Upload one/multiple files to a test form, cancel a chooser, download a PDF from an authenticated page, interrupted download cleanup, malicious filename, collisions, and long filenames. Prefer downloading into a controlled directory rather than automatically opening arbitrary downloaded executables.
- **References:** https://wpewebkit.org/reference/stable/wpe-webkit-2.0/class.FileChooserRequest.html , https://wpewebkit.org/reference/stable/wpe-webkit-2.0/class.NetworkSession.html , https://wpewebkit.org/reference/stable/wpe-webkit-2.0/class.Download.html

### P0 findings and implementation order

1. **Use WPEPlatform's native Wayland input and clipboard**, not manually injected libwpe events; intercept shortcuts on `WPEView::event`.
2. **In M2, create the explicit persistent browsing `WebKitNetworkSession` before constructing browsing views**; use its download signal when adding M3 downloads.
3. **In M3, prototype `<select>`, JS dialogs and file upload before expanding compatibility work**: those are the likely toolkit-free UX integration gaps.
4. **Do not claim default dialogs or XDG portal integration work without a niri runtime test.** The API hooks are confirmed; the shell UX implementation is not.
5. **Verify sandbox, TLS and subprocess behavior on the actual custom build** before trusting it with logged-in accounts.

**Reference versions:** WPE WebKit 2.54.1 (released 2026-10-06); WPEPlatform 2.0 stable since 2.54. Upstream reference index: https://wpewebkit.org/reference/stable/wpe-webkit-2.0/ .


## P0/G. Local files and document-preview origins (decision, 2026-10-12)

**Goal:** render local HTML and generated Markdown/Typst previews with no localhost HTTP server, while preserving browser-origin boundaries and keeping the browser toolkit-free.

### Three distinct loading paths

| Path | Public API / mechanism | Use | Cautions |
| --- | --- | --- | --- |
| Regular local files | Convert a user-authorized absolute path to a properly escaped `file://` URI using `g_filename_to_uri()`; navigate with `webkit_web_view_load_uri()` | Open an existing local HTML page with normal relative resources | Must define who authorizes file access and test the WebKit file-origin and sandbox policy; do not globally enable universal file access |
| In-memory HTML | `webkit_web_view_load_html(view, html, base_uri)` or `webkit_web_view_load_bytes(view, bytes, mime, encoding, base_uri)` | Generated one-off report, error page, self-contained preview | The base URI affects relative resources and potentially security context; `about:blank` is preferable for a genuinely self-contained page; avoid broad `file://` base roots |
| App-provided resources | `webkit_web_context_register_uri_scheme(context, "foob", callback, ...)`; `WebKitURISchemeRequest` and `webkit_uri_scheme_request_finish()` / `finish_with_response()` | Managed Markdown/Typst preview, internal UI, bundled KaTeX/fonts/CSS, allowed assets | Design scheme origins/CORS and an allowlisted resource resolver; this must not provide a general arbitrary-file-reading API |

**Preferred architecture:** no local web server. Initially open explicitly requested `.html` via `file://`; serve converted/document-generated HTML through a private `foob:` scheme. This is a browser-private *URI scheme*, not a registered internet host or an HTTP service. Using `foob://document/<opaque-id>/...` is a proposed application convention, **not** a built-in WPE API or a security guarantee. Consider whether separate documents need separate hosts/origins to isolate mutually untrusted content, and test the engine's treatment of custom-scheme origins before relying on it.

### Relevant public WPE/WebKit APIs

- `webkit_web_context_register_uri_scheme()` registers a callback; the callback receives a `WebKitURISchemeRequest`. Obtain the URI using `webkit_uri_scheme_request_get_uri()`; provide a `GInputStream` with correct length and MIME type using `webkit_uri_scheme_request_finish()` or an explicit `WebKitURISchemeResponse` via `webkit_uri_scheme_request_finish_with_response()`.
- `WebKitSecurityManager` can classify a custom scheme (notably `webkit_security_manager_register_uri_scheme_as_local()`, `...as_secure()`, `...as_cors_enabled()`, `...as_no_access()` and `...as_display_isolated()`). These classifications have **different security semantics**: do not enable all of them automatically. In particular, marking a scheme CORS-enabled is not equivalent to restricting it, and marking it secure does not validate content.
- Register the scheme and set any intentional security classifications **before navigating the first view**. The handler and security-manager APIs were confirmed in the pinned WPE WebKit 2.54.1 upstream headers. The upstream 2023 introductory article's browser skeleton uses an old Cog/libwpe backend; take only its scheme-related design, not that outdated platform boilerplate.
- `webkit_web_view_load_html()` / `load_bytes()` are useful for isolated in-memory pages, but less convenient than scheme handlers for multiple linked CSS/JS/image/font assets. Do not confuse `load_alternate_html()` (error/replacement content) with a general-purpose file server.

### Strict resource policy

1. Treat remote pages as untrusted; local user documents are trusted today (D-08) and M4 revisits that. A remote website must never be able to use `foob:` as a bridge to arbitrary filesystem paths or browser-native operations.
2. Maintain a per-document allowlist (e.g. document ID -> approved root/assets) and resolve requested assets against it. Canonicalize paths; reject `..` traversal, percent-encoded traversal, disallowed symlinks and access outside the approved roots. Prefer pre-opened handles where feasible. Do not concatenate raw URI path strings into filesystem paths.
3. Use explicit MIME types and charset, especially `text/html; charset=utf-8`, `text/css`, `text/javascript`, `image/svg+xml`, font types and PDF downloads. Consider CSP restricting script and network access for local previews, with an explicit policy if KaTeX loads local JS.
4. Avoid permissive global security switches: no blanket `allow-universal-access-from-file-urls`, no disabled CORS, no general 'local page can fetch every file', no `http://localhost` fallback unless separately justified.
5. Decide whether local documents may load remote resources. **Default for managed research previews: block remote resource fetching unless explicitly enabled** (avoids privacy leaks from third-party images/scripts). Remote CSS/JS embeds should not inherit native file privileges.
6. If invoking Markdown/Typst converters, treat filenames and contents as untrusted; use argv-based subprocess APIs (no shell string interpolation), enforce resource/time limits for untrusted documents where feasible, and avoid granting native API access to JavaScript in previews.
7. Be explicit that a custom scheme doesn't necessarily give `https:`-equivalent features such as service workers, secure-context APIs or the same storage semantics. Test whichever Web APIs a local viewer actually needs.

### Planned preview flow

```
CLI / keyboard selection of ~/papers/notes.md
    -> authorize this file and allowed adjacent assets
    -> convert Markdown to HTML (converter TBD)
    -> supply page + KaTeX/CSS/fonts through foob: scheme handler
    -> WebKit renders in native Wayland view
    -> changing/reloading the document updates the preview
```

Document-filename display placement is undecided; do not infer a status strip or other persistent chrome from historical qvu layouts. This flow does not require a DNS hostname or socket listener. Manual reload is sufficient initially; automatic watching/editor-driven updates are deferred. If later added, use GIO monitoring or another lightweight filesystem watcher outside the web-process privilege boundary.

### Source-aware Markdown navigation

Keep each managed preview's original source location alongside its document ID and authorized resources. A relative Markdown link belongs to that source location, not to the generated HTML/cache directory or its hashed filename. For example, `./details.md#proof` in `/papers/notes.md` identifies `/papers/details.md` and the `proof` fragment. Preserve the raw target, resolve and validate its source path under the existing filesystem policy, then convert the authorized target and map it to a managed preview URI with the fragment retained. Never reconstruct source paths from cache filenames or silently expand approved roots; deny out-of-root links or require explicit authorization.

Same-document fragments stay in the preview. The renderer must supply deterministic heading IDs, including a documented duplicate-heading rule, for generated heading links to work. Test explicit anchors separately from generated heading anchors, and verify Back navigation between converted documents. The [qvu local-navigation experiment](docs/research/qvu/findings/markdown-local-navigation.md) demonstrated broken source-relative Markdown links and missing heading IDs in its renderer; these are reusable test cases, not established WPE behavior. Its probe code remains in qvu and must be recreated or explicitly imported. Switching transport alone does not supply source context or conversion.

### Web links from managed previews

**Preview storage isolation (M4-02/M4-03):** managed previews use a separate ephemeral network session, while ordinary browsing uses M2's persistent session. Investigate WPE's network-session/context boundaries and scheme-handler ownership before implementation; verify preview restrictions cannot become browsing-wide policy. Test that preview site state disappears when its session ends and that preview cleanup leaves browsing cookies/logins intact. Ephemeral storage does not itself enforce filesystem/network access or isolate documents sharing a session; retain the separate resource/origin checks. This follows the [qvu design review](docs/research/qvu/findings/research-browser-design-review.md) without adopting its Qt profile implementation or adding a user-facing private mode.

User-activated HTTP(S) links from managed previews open in a separate foob browsing window, preserving the preview document, scroll position and zoom. This uses one web view per window and M2/M3's navigation/new-window policies; it does not introduce tabs or reuse the old qvu research-window design. Same-document fragments stay in the preview, and linked Markdown follows source-aware authorization and conversion. Automatic remote resource requests remain blocked by default; script-driven popups and redirects must follow explicit policy and cannot use this link-routing rule as blanket authorization.

The destination window uses the ordinary browsing session and must not inherit the preview's document-resource allowlist or local-file privileges. Enforce this boundary even if URI handlers are shared internally; opening a browsing window does not authorize access to managed document resources. M4-08 tests ordinary and `target=_blank` web links, preservation of preview state, and rejection of attempts by the destination page to fetch unapproved local/managed resources. The [qvu navigation findings](docs/research/qvu/findings/markdown-local-navigation.md) motivate preserving the preview, while foob's current window model determines the destination.

### Preview output ownership and lifetime

Assign an explicit owner to each generated preview output and identify its required HTML, CSS, scripts, fonts and image resources. Publish a complete output generation atomically; failed conversion/publication must not replace a working generation with partial content. Keep referenced generations and assets available while reload, linked-document navigation or Back navigation can still use them. Track shared-asset references so closing one preview cannot invalidate another. External renderer handoffs must establish who retains and releases the output; do not assume a temporary path survives the converter's exit.

Define cleanup for preview close, failed conversion and restart after an application crash. Cleanup is limited to identifiable application-owned generated data and must preserve source documents, external assets and outputs still used by active previews. Validate this lifecycle in M4-09 with converter-exit/reload, failed-publication, Back-navigation, shared-asset and crash-recovery tests. The [historical blueprint](docs/research/qvu/design/qt6_minimalist_research_viewer_blueprint.md) supplies the ownership concern, not a required IPC design. Live updates and multi-editor ownership remain deferred; if introduced, define current-owner identity and stale-update rejection before accepting editor-driven refreshes.

### Test matrix before calling this production-ready

**Fresh-process policy and offline controls:** configure resource restrictions and scheme classifications before first navigation. Run cold resource-policy cases in fresh browser processes with isolated empty test data/cache directories, preserving the normal browsing profile. For offline math, make networking unavailable before launch and load only local assets; verify visible output/font loading and record attempted HTTP(S) requests, the network-isolation method and diagnostic limitations. Test warm reload separately under unchanged restrictions. Missing-asset tests also need fresh process/cache state so cached resources cannot hide failures. Use allowed/denied resource controls to distinguish policy enforcement from general loading defects. The [qvu asset-access investigation](docs/research/qvu/findings/katex-investigation.md) observed success after restoring restrictions and reloading but failure in a fresh process; the mechanism was not isolated. Repeat these controls on stock/custom engines in M5. These empty-state controls apply to local-resource tests, not Scholar restart-continuity tests that intentionally retain a profile.

- Open stand-alone local `.html` with relative CSS/images; confirm its canonical `file://` URL and MIME detection (`shared-mime-info` may be required on minimal installations).
- Render generated Markdown math, KaTeX fonts/scripts, MathML, local images, and special-character filenames without an HTTP server.
- Follow source-relative sibling/parent Markdown links with fragments; test spaces, encoded filenames, missing targets, duplicate headings and Back navigation. Authorized targets must be converted, while links outside approved roots must not silently grant access.
- Follow user-activated HTTP(S) links, including `target=_blank`, into separate browsing windows; verify preview document/scroll/zoom preservation and that remote destinations cannot inherit document-resource access. Automatic remote requests remain blocked by default.
- Verify generated-output lifetime and cleanup: reload after converter exit, retain a working generation after failed publication, preserve Back-navigation resources, close one of two previews sharing assets, and recover owned temporary data after a crash without deleting source/external files.
- Navigate from a remote page to a crafted `foob:` path and try to read an unapproved path; must be denied. Test `../`, `%2e%2e`, symlink traversal, absolute paths and malformed encodings.
- Open two unrelated local documents and verify they cannot fetch each other's unapproved resources; verify remote requests are blocked by default.
- Test behavior of CORS, relative URLs, cookies/storage, history/back, origin and CSP on custom-scheme pages before adopting any scheme classification.
- Confirm memory/performance differences between `file://`, load-bytes and streaming scheme handlers, and test sandboxed sub-processes remain functional.

**Status:** WPE's public custom URI integration mechanism is verified in upstream documentation. The particular `foob:` scheme design, origin isolation rules and production-safe file resolver are **design proposals requiring implementation and security tests**.

### References

- Igalia / WPE: [Integrating WPE: URI Scheme Handlers and Script Messages](https://wpewebkit.org/blog/06-integrating-wpe.html) (2023; note platform example predates WPEPlatform).
- WPE: [WPEPlatform: the new WPE API](https://wpewebkit.org/blog/2026-10-06-wpe-platform.html) (2026).
- WPE: [FAQ on local MIME type detection](https://wpewebkit.org/about/faq.html).
- WebKit upstream public headers: [WebKitWebView.h.in](https://github.com/WebKit/WebKit/blob/main/Source/WebKit/UIProcess/API/glib/WebKitWebView.h.in).

## Mathematical local-document rendering — provisional investigation plan (2026-10-12)

**Decision:** Keep mathematical document rendering as a core goal, but **do not assume compatibility until tested first on the stock WPE build and then on the pinned custom WPE build**. No localhost HTTP server is required for application-supplied resources; prefer a constrained `foob:` URI scheme for managed previews. The precise scheme-origin/security behavior remains subject to tests (see the local-files section above).

### Three rendering paths to distinguish

1. **Native MathML:** Test MathML Core in ordinary HTML and the kinds of MathML emitted by converters. Preserve MathML functionality in the custom build; this is not an instruction to set a private `ENABLE_MATHML` tuning option. Check fractions, roots, matrices, stretchy delimiters, scripts, display/inline layout, line breaking, copy/selection, accessibility, and fonts. Do not assume every LaTeX package or construct maps cleanly to MathML.
2. **KaTeX-rendered Markdown/LaTeX:** Convert Markdown to HTML, render inline/display TeX with locally bundled KaTeX JavaScript/CSS/fonts (or prerender HTML outside the browser). **Offline operation** must work with no CDN and no local HTTP service. Check glyphs/font loading and mathematical accuracy. Compare client-side KaTeX with preprocessing: prerendering may save JS startup work, but its size/runtime benefit must be measured.
3. **Typst:** The browser is not a native Typst layout engine. Investigate supported export/conversion formats for the **specific Typst version** used. Render an HTML/MathML result only if a supported export path actually produces it with acceptable fidelity. For authoritative pagination/visual fidelity, use the existing external PDF viewer. Avoid writing "Typst HTML export works" as a verified capability before testing it.

### API and security checks

**Typst evidence provenance:** the [qvu rendering findings](docs/research/qvu/findings/rendering-findings.md) demonstrated pagination and editor synchronization using tinymist's live preview frontend under Qt, not exported HTML/MathML. They do not validate M4-06's serverless export route or current WPE compatibility. A prior WebKit canvas failure mentioned in the [historical adapter notes](docs/research/qvu/design/qt6_minimalist_research_viewer_blueprint.md) is a lead for a future live-preview investigation, not proof that current WPE or exported HTML fails. Keep M4-06 focused on supported export and external PDF viewing. Any later live tinymist integration needs a separate scope/compatibility investigation covering pagination, incremental updates, fonts and bidirectional editor synchronization.

- Test `file://`, `webkit_web_view_load_html()`, and a registered `foob:` handler for HTML + relative stylesheet, scripts, WOFF/WOFF2 fonts and images. A custom URI scheme is a resource-serving mechanism, **not** evidence of correct math rendering or safe same-origin semantics.
- Establish whether locally bundled KaTeX assets and relative links load under the chosen custom-scheme classifications. Retain same-origin protections; never enable unrestricted filesystem access or broad cross-origin privileges to get fonts working.
- Prefer document-specific identifiers and an allowlist for adjacent figures/fonts. Test content containing untrusted HTML, TeX macros, SVG and links; define an HTML sanitization/CSP policy before allowing arbitrary downloaded Markdown or generated HTML.
- Evaluate mathematical font fallback and HarfBuzz/FreeType/WOFF2 behavior on this machine, including scaling and selection/copy into other applications.
- Preserve `MathML`, JavaScript, CSS, SVG, Canvas and web-font support in the custom WPE configuration unless a controlled test proves a feature unnecessary. Disabling video/audio must not disable these text/math requirements.

### Acceptance fixtures

Maintain separate hand-written MathML controls and converter-generated fixtures. The [qvu renderer comparison](docs/research/qvu/findings/math-renderer-comparison.md) found that generated `aligned` markup failed while a hand-written `mtable` rendered correctly; zero reported errors did not establish correct layout. Inspect markup, visible output and row/delimiter geometry, and compare equivalent controls before blaming WPE. Record converter versions and the selected/actual math font where measurable. The [Temml comparison](docs/research/qvu/findings/temml-katex-comparison.md) showed that font selection affected delimiter sizing; a declared CSS family alone is insufficient evidence of the font used. Keep these historical findings distinct from WPE results. Only mathematically correct paths qualify as alternatives in performance comparisons; KaTeX remains the preview candidate pending validated evidence.

| Fixture | Pass criterion |
|---|---|
| Offline Markdown with inline/display KaTeX | Six qvu formula cases render correctly; required local assets/fonts load; fresh load and reload work with networking unavailable; zero HTTP requests attempted; missing assets and invalid TeX produce useful diagnostics |
| Mixed Markdown and math | Tables and checked/unchecked tasks retain formatting; dollar signs in inline/fenced code and literal currency remain text; only intended inline/display equations render under documented delimiter rules |
| Hand-written MathML controls | Fractions, radicals, matrices, aligned rows, cases, limits and stretched operators render properly with recorded fonts |
| Converter-generated MathML | Intended structure and visible layout match equivalent controls; converter defects are distinguished from engine/font failures |
| Math-heavy Wikipedia page | Equations, font fallback, zoom and resize render correctly |
| Typst sample and external PDF | Determine supported export route; compare visual fidelity; PDF handoff works |
| Hostile local content | Cannot read unapproved files or silently fetch remote content |
| Stock vs custom WPE | Same visual results, with measured load time and **total process-tree PSS** |

**Status:** Planned validation, not completed. The MathML/KaTeX/Typst results and exact custom-scheme origin policy must be confirmed by prototype tests; do not make production-compatibility claims beforehand.

Document the supported Markdown dialect and math-delimiter rules when selecting the converter. M4-05 includes a mixed Markdown fixture to check ordinary formatting and math together, following the cases identified in the [historical adapter plan](docs/research/qvu/design/qt6_minimalist_research_viewer_blueprint.md). Test tables, checked/unchecked task items, inline/fenced code with dollar signs, literal currency amounts and intended equations; math recognition must respect code boundaries and the documented escaping rules. This fixture evaluates converter suitability without requiring Neovim renderer reuse and belongs in both stock and custom WPE regression checks.

The [qvu local KaTeX experiment](docs/research/qvu/findings/katex-local-assets.md) supplies a candidate six-formula baseline, not WPE validation. Recreate or explicitly import its fixtures; their generator remains in qvu. Pin and record the chosen KaTeX version, bundle JavaScript/CSS/fonts with the stylesheet's relative font paths intact, and validate on stock WPE through `file://` and the managed `foob:` transport when available. Prerendered math may omit browser-side KaTeX JavaScript but still needs its required CSS/fonts. Verify visible output and font loading separately from resource logs: an empty `file://` resource-timing list does not prove successful loading. The historical test kept networking connected and did not test reload, missing assets or invalid TeX; M4 must cover those gaps, and M5 must repeat the fixtures on the custom engine.

## Language and remaining architecture decisions (2026-10-12)

### Language decision: C23, with an explicit revisit criterion

**Accepted for initial implementation:** C23, GLib/GObject, Meson. The supported WPE WebKit 2.54+ and WPEPlatform APIs are C/GObject; writing the embedder in C makes the callbacks and library interfaces direct. `g_autoptr` and GLib cleanup facilities can help with resource ownership, but require clearly documented transfer/borrow semantics. GLib `g_autoptr` relies on compiler-specific cleanup attributes even with `-std=c23`; this is not a fully extension-free application. Avoid undocumented assumptions about callback lifetime. C++20/23 is technically viable and does not inherently cost more runtime memory; its RAII and stronger abstractions may be useful if state management becomes substantially more complex. Do not mix languages without a concrete dependency or complexity benefit. Test the installed compiler's `c_std=c23` support as part of Meson configuration.

### Five architecture decisions to resolve at the relevant milestone

Resolve lifecycle for M1, navigation and persistent-profile concurrency for M2/M3, local trust for M4, and custom-engine reproducibility for M5. IPC remains deferred. Each decision needs a documented policy and repeatable acceptance check before the affected feature handles untrusted input; the full list does not block the first trusted-page prototype.

1. **Object and process lifetime (P0, resolved in M1: `AGENTS.md`, `browser.[ch]`):** Define ownership of `WebKitWebView`, WPE view/toplevel, network session, profile, outstanding asynchronous requests, and GLib signal handlers. Identify shutdown semantics, cancellation, process-crash signals and how to distinguish a page crash from a UI-process failure. Verify clean exit after compositor-initiated close and no persistent orphaned processes.
2. **Navigation and window-creation policy (P0):** Define same-window vs new-window requests, target=`_blank`, popups, external protocols (`mailto:`, etc.), redirects, downloads, and forbidden schemes. Design explicit policy defaults before handling untrusted web content. Do not silently pass untrusted strings to a shell.
3. **Local-document origin and filesystem trust (P0):** Document `file:` vs in-memory HTML vs managed custom scheme, and choose how managed previews identify individual documents. Do not give a generic custom URI handler access to arbitrary host paths. Define symlink/path traversal policy, resource allowlists, per-document isolation and which script/network capabilities previews need. Verify WebKit origin/CORS behavior experimentally; avoid relaxing universal-file-access settings. Bundle KaTeX locally and benchmark native MathML separately.
4. **Single instance, window/process topology and IPC (P1):** Prefer one browser UI process with one view per window initially; let WPE manage its own subprocesses. Decide whether a second `foob URL` invocation opens a new instance or sends a request to an existing instance. A private Unix-domain socket or other authenticated local mechanism may be considered later; do not expose unauthenticated browser control over TCP. Document startup URI and profile locking before supporting concurrent launches.
5. **Reproducible engine builds and security response (P0):** Pin upstream release, checksum/source provenance, CMake feature manifest, enabled graphics backends, build dependencies, binary dependencies and sandbox tests. Keep custom-engine builds independent from the browser's Meson compilation. Define a method to detect and rebuild for security updates; benchmark stock and custom variants on identical fixtures. Never trade away process sandboxing for a small memory saving.

### Additional prototype-level risks to test early

- **Text input and editing:** compose/dead keys, Unicode, selection, clipboard, cursor positioning, focus transitions, text input on complex editor apps, niri keyboard shortcuts and IME behavior.
- **Portal and desktop integration:** file chooser under niri, downloads, opening external PDF viewer, notifications (if needed), and XDG environment; determine which capabilities work without GTK/Qt rather than assuming them.
- **GPU/display correctness:** renderer identity (M5-10), fractional scaling (M6-07), suspend/resume, GPU-process failure and software fallback.
- **Web app compatibility:** login/SSO, third-party cookie policy, `window.open`, file uploads, clipboard, service workers, local storage and heavy single-page apps. Disabling media APIs may have indirect website effects; record failures rather than assuming all sites work.
- **Benchmark reproducibility:** capture warm/cold startup, total PSS by process, idle and post-navigation values, viewport geometry, JS heap behavior where measurable, and cached vs uncached runs.

**Sources:** https://wpewebkit.org/blog/2026-10-06-wpe-platform.html ; https://wpewebkit.org/blog/2026-09-16-wpewebkit-2.54.html ; https://wpewebkit.org/blog/06-integrating-wpe.html .

## Open questions / deferred decisions

- Choose a final project name (`foob` is provisional).
- Decide on URL prompt strategy (keyboard-only built-in prompt versus external launcher such as fuzzel; M2-07).
- Decide whether a single-window, multi-tab model is needed at all (M2-11, M6-06).
- Evaluate the engine's per-site compatibility before relying on it as a Firefox replacement (M3-06).

## References

- WPE project: https://wpewebkit.org/
- WebKit upstream: https://github.com/WebKit/WebKit
- WPE build options (release-specific; verify exact tag): https://raw.githubusercontent.com/WebKit/WebKit/wpewebkit-2.54.1/Source/cmake/OptionsWPE.cmake
- Cog, a historical minimal WPE launcher: https://github.com/Igalia/cog
- Igalia's Yocto integration recipes: https://github.com/Igalia/meta-webkit
- RDK platform: https://rdkcentral.com/

**Decision to revisit:** After the first stock-vs-custom benchmarks and successful navigation/download tests. No Servo work is planned in the initial implementation.
