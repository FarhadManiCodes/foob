> Historical qvu material imported on 2026-10-12. Qt-specific behavior and earlier decisions require WPE validation. Foob’s architecture decision and roadmap take precedence. Reproduction commands refer to the original qvu checkout.

# qvu: Qt6 Research Browser and Document Previewer

Working blueprint, revised through discussion on 2026-10-12. Proposed architecture;
initial rendering findings are recorded in [docs/rendering-findings.md](../findings/rendering-findings.md).
Implementation is stopped pending review of [structure and engineering rules](../../../../../qvu/docs/implementation-plan.md).

## 1. Agreed workflow

First version combines Markdown preview, Typst preview, and research browsing.

- Previews open in dedicated windows beside Neovim.
- One preview window per document; opening the same document again reuses it.
- Research pages open as tabs in a separate research window.
- Neovim remains an independent editor. Preview updates never replace research tabs.
- Niri controls window placement; qvu controls preview identity and research tabs.
- Sioyek remains the PDF reader, papis the library, and tinymist the Typst compiler.
- LLM chat stays in Firefox. Copy/paste selections into existing chat sessions;
  embedded chat, provider APIs and automatic prompt submission are outside initial scope.
- No dedicated login/account-management UI or in-app audio/video playback.
  Minimize retained research site state while allowing valid anonymous sessions
  to survive restarts. A Scholar cookie-only startup test passed; selective
  persistence through the Qt API still needs a functional check. The agreed
  candidate retains four Google/Scholar cookies, with others temporary.
  Sites needing those workflows are handed to Firefox.
- Basic reading controls include find in page, zoom, selection/copy and URL copying.
  PDFs open in Sioyek; Markdown and Typst use their renderers.

One application instance can own several windows. QtWebEngine also starts Chromium
helper processes: single-instance does not mean single-process.

## 2. Existing system and installation requirements

Read-only inventory on 2026-10-11:

| Component | Current state | Proposed use |
|---|---|---|
| Sioyek | Custom Qt6/C++20 source at `~/Installs/sioyek`; installed under `~/.local/share/sioyek` | Keep PDF rendering, selection, hints and SyncTeX |
| Sioyek searches | `ss`/`sw` use `sioyek-search`, percent-encoding selections before `open-detached` | Add optional qvu destination and `sg` for general Google search |
| Sioyek papis actions | Alt+n notes, Alt+p citation/library handling | Reuse helpers |
| Neovim Markdown | Local `config/md_preview.lua`: cmark-gfm, KaTeX, GFM tables/tasks, image rewriting, theme CSS, path-hashed URLs, atomic publication | Keep renderer; add qvu opener and targeted refresh |
| Markdown server | Loopback port 7654; save updates HTML, browser reload is manual | Automatic refresh; improve server ownership |
| Neovim Typst | typst-preview.nvim 1.x, system tinymist, root-document preview, cursor sync both ways | Consume existing preview URL |
| Tridactyl | Numeric hints, search aliases, papis capture, yts playback | Familiar browser interaction |
| Niri | Separate PDF windows; half-width papis editor; Firefox previews share Firefox app-id | Dedicated qvu identity and preview placement |
| Qt Core/Widgets/Network | Installed; pkg-config reports 6.12.0 | UI and IPC |
| qt6-declarative | Installed, 6.12.0-1 | Existing system dependency |
| CMake/Ninja | Installed, 4.4.4-1 / 1.13.2-3 | Build qvu |
| GCC/Clang/pkgconf | Installed | Compiler and discovery |
| cmark-gfm | Installed, 0.29.0.gfm.13-2 | Existing Markdown conversion |
| tinymist | Installed, 0.15.8-1 | Existing Typst compiler/server |
| papis/yts | Existing local installations | Library and playback |
| Firefox/fuzzel/mpv/jq/wl-clipboard | Installed | Existing integrations/fallback |
| qt6-webengine | Missing | Main additional package |
| qt6-webchannel/qt6-positioning | Missing | Pulled as WebEngine dependencies |
| qt6-tools | Missing | Not required for this Widgets build |

`../sioyel` was absent; the Sioyek checkout was located through dotfiles documentation.

Use packaged QtWebEngine rather than building Chromium. Local repository metadata
reports `qt6-webengine` 6.11.2-2: 94.54 MiB download, 282.86 MiB installed, excluding
additional dependencies. Let the package manager resolve dependencies. Review the
transaction against the existing system update policy when installation is requested.
QtWebEngine and installed Qt base have different versions; confirm CMake discovery
and runtime behavior rather than inferring compatibility from version numbers alone.

`qt6-wayland` is absent, but `qt6-base` already owns
`/usr/lib/qt6/plugins/platforms/libqwayland.so` and QtWaylandClient here. Do not add
qt6-wayland solely to get a platform plugin that is already present.

Build proposal: C++20, CMake, Ninja, Qt Widgets, with CMake components `Core`,
`Widgets`, `Network`, `WebEngineWidgets`. No QML frontend, Python bindings, Node
service, or new Markdown compiler required. WebChannel need not become a browser
command bridge merely because it is a package dependency.

No packages have been installed and no live configuration has been changed.

## 3. Window and document model

Keep a preview registry separate from research tabs.

Do not save browsing history or restore research tabs, their order, or the active
tab across launches. A research window starts from the explicit request that opens it.
Opening or refreshing a document preview does not open, restore, activate, or change
the research window. Following a web link is a separate explicit research action.

- Research requests normally create a tab in the research window.
- Sioyek `ss` (Scholar), `sw` (Wikipedia), and planned `sg` (Google) searches open
  a new research tab and activate research. Repeating
  the same search (same engine and query) selects its existing tab if still open.
- Ordinary research links navigate in the current tab. `F` opens a hinted link in
  a new background research tab, keeping the current page visible.
- Preview requests contain document ID, editor/owner ID, kind and current URL.
- A Markdown document is identified by its canonical source path. A Typst document
  is identified by its root source path, so included files belong to that preview.
- Reopening a preview reuses its window by document ID.
- Neovim can render again, open the document's preview, or replace its rendered
  content in the existing window. Different documents retain separate preview windows.
- If another Neovim instance explicitly requests that document's preview, transfer
  update ownership to it while reusing the same window. Background saves from the
  previous owner cannot reclaim ownership or refresh the transferred preview.
- Accept refreshes only from the current owner; validate the owner ID supplied
  through the preview/refresh protocol. An old owner's disconnect cannot invalidate
  the new owner's preview. Ownership affects preview updates, not source-file editing.
- Keep rendered output and server URLs specific to the owner so an old editor cannot
  overwrite the active preview's HTML through a shared output path. Transfer switches
  to the new owner's content source; editor-triggered close requests also check ownership.
- Markdown saves refresh that document without activating or raising its window,
  switching workspaces, or taking keyboard focus from Neovim. Preserve the preview's
  reading position and zoom. An explicit open-preview request may activate its window.
- Typst edits update through the existing server, without full-page reloads.
- Closing a preview unregisters it; later saves do not reopen it unexpectedly.
- Editor exit marks its previews disconnected without terminating other owners.

Use a web view plus one-line status/command area. Status height follows the font,
not a fixed window percentage. Temporary URL/command input remains available.

### Agreed visual direction (2026-10-12)

Content fills the window above one thin bottom strip. No permanent toolbar, top tab
row, extra application heading, or duplicate document title. Use a readable font,
subtle separator and restrained highlighting. Keep the strip's appearance consistent
across research and preview windows; websites retain their own appearance and
Markdown previews follow the editor theme.

- Research strip: numbered tmux-style tab labels at the left, active tab highlighted,
  NORMAL/INSERT mode at the right. Shorten long titles; show nearby tabs and an
  overflow indicator when they no longer fit.
- Preview strip: filename, MD/TYP kind and update/connection state at the left;
  zoom percentage and mode at the right. No preview tabs.
- Markdown: continuous document layout, with headings, links, tables and math.
- Typst: paginated document layout supplied by the existing preview renderer.
  Preserve typeset page dimensions, margins, page breaks and numbering rather than
  reflowing the manuscript into Markdown-style HTML. Distinguish page surfaces from
  the surrounding viewport; retain multi-page scrolling and the renderer's zoom/sync.
- Show page/document titles and tab labels, not passive or hovered URLs. Provide
  an action to copy the current URL. Explicit URL/command/search input temporarily
  replaces the strip and restores it on completion.
- Normal mode handles navigation; focus in an editable field enters insert mode,
  where typing reaches the webpage. Esc returns to normal mode; validate IME behavior.

Local comparison references are stored in the Git-ignored `design-references/` folder:
`research-window.png`, `markdown-preview.png`, and `typst-preview.png`.
These are illustrative layout references, not promises of website styling or
replacements for testing real Markdown/Typst rendering. They are intentionally
absent from Git history and will not appear in a fresh clone.

Verify the desktop app-id Qt emits. If distinct research/preview Wayland app-ids
require separate application processes, compare that with Niri title/role rules.
Do not assume one QApplication supports arbitrary per-window app-ids.

## 4. CLI and IPC

Proposed commands, not yet installed:

```text
qvu search scholar QUERY
qvu search wikipedia QUERY
qvu search google QUERY
qvu open URL
qvu preview --kind markdown --id DOC_ID --owner OWNER_ID URL
qvu preview --kind typst --id DOC_ID --owner OWNER_ID URL
qvu refresh --id DOC_ID --owner OWNER_ID
qvu close --id DOC_ID --owner OWNER_ID
```

QLocalServer/QLocalSocket use a private directory under `$XDG_RUNTIME_DIR`, with
user-only access, startup locking, bounded timeouts and stale-endpoint recovery.
A connection failure alone does not establish that no server exists.

Use framed JSON with protocol version, request ID, action and arguments, plus
acknowledgements and useful errors. Handle forwarding before initializing WebEngine.
Consider a separate Core/Network client only if measured launch cost justifies it.
Remove the original "0ms footprint" claim. Exit after the last window closes
initially; an always-running daemon is a separate resource tradeoff.

## 5. Markdown adapter

Keep Neovim's existing HTML renderer. Preserve tables, task lists, code-dollar
handling, KaTeX error recovery, local images with encoded names, independent document
URLs, atomic publication and theme changes.

Preferred Markdown transport candidate is local rendered HTML with locally bundled
math assets. Adoption still requires a working source-aware linked-document adapter;
the current probe's relative Markdown link test fails without that adapter.
The existing renderer's relative image aliases work through `file://`, including
encoded spaces and query/fragment suffixes; see
[the local navigation findings](../findings/markdown-local-navigation.md).
Retain its referenced assets and image aliases for the preview's lifetime.
No Markdown HTTP server is required by this proposed qvu path. The renderer's Firefox
transport and Typst's live server remain separate integration concerns.

After successful publication, notify qvu by document ID. Preserve reading position
on reload; start with scroll position and consider block/heading anchors later.
Do not block editor saves. Failed conversion keeps the last good preview.

Renderer output needs ownership isolation for multiple editors regardless of transport.
Any retained HTTP transport's fixed-port server and broad shutdown logic need lifecycle
improvements in the Neovim project. A listening port is not proof of server identity.

Relative Markdown links need original source-path context and a conversion request
to the owning renderer. Generic HTML URLs cannot recover this reliably. Implement
basic preview first, then linked Markdown documents. Link routing is agreed:
same-document fragments stay in the preview, web links open in research browsing,
and linked Markdown documents go to the renderer for conversion. Preserve the original
link target or a renderer-resolved source path instead of resolving against the HTML
cache directory. Heading-anchor generation belongs to the renderer; qvu follows
existing target IDs. Preserve ordinary images.
The renderer owns the pinned KaTeX version and rendering calls; transport serves
the bundled files. Local assets worked in the experiment; installing them for the
live renderer is tracked in the Neovim issue.

## 6. Typst adapter

Keep tinymist and typst-preview.nvim responsible for compilation and incremental
updates. Initially replace only the browser-opening adapter after verification.
Preserve root-document selection, included files, papis bibliography sync and the
existing Neovim commands.

Compatibility gate: multiple pages, incremental edits, fonts, editor-to-preview
cursor movement and preview-to-editor navigation. Existing config records a WebKit
canvas failure; QtWebEngine behavior needs a real document test. Keep Firefox usable
until this passes. Exclude active Typst previews/WebSockets from tab suspension.

## 7. Keyboard and research interaction

Use shared commands for UI and IPC, with UTF-8 config and explicit parse errors.
Borrow familiar Tridactyl/Sioyek behavior before inventing another key vocabulary.

Essentials: j/k, gg/G, /, n/N, H/L, reload, f/F link hints, URL yank, open/search
prompt, tab next/previous/close and Esc. Tridactyl-style direct bindings are agreed;
do not add a Ctrl+b prefix. Numeric hints match the current Tridactyl preference.
Proposed tab defaults are J/K for next/previous and D to close; verify these against
the user's actual Tridactyl habits before finalizing the complete binding table.

Agreed hint bindings: `f` follows the chosen link in the current research tab;
`F` opens it in a new background tab without switching. Remote links from document
previews still route to the research window, preserving the preview document.

Normal, insert, hint and command modes need explicit handling. Preserve typing in
forms/contenteditable, IME composition and site shortcuts. Top-level keyPressEvent
alone is insufficient when WebEngine's child owns focus; test event filtering.
Start hints with visible links and document iframe/shadow-DOM limitations.
Use Qt's clipboard API rather than requiring wl-copy for every URL copy.
Both previews and research support find in page, zoom, selection/copy and URL
copying. Previews preserve scroll and zoom across refreshes and show
when their editor/server disconnects.

General Google search reuses the selection-to-search flow. Initially use only three
engine-to-URL mappings: Google, Scholar and Wikipedia, with correctly encoded queries.
No engine-management UI, provider discovery or general helper/plugin framework.
LLM conversations remain in Firefox, using normal selection copying and pasting.
This is a workflow/scope choice, not a claim that QtWebEngine cannot render chat sites.

The limited external actions are papis-add-paper for DOI/arXiv capture, Sioyek for
PDFs, Firefox handoff, and the future pask answer adapter. Existing rendering tools
remain responsible for Markdown/Typst content. No yts/mpv playback integration in
the initial version; their presence in the system inventory is not a qvu requirement.
Invoke external tools through QProcess argument lists, never shell interpolation.

Provide simple keyboard actions to open the current page or a hinted link in
Firefox. This passes a URL, not cookies or authenticated session state.
Downloads use completion/failure notification and an action to open completed PDFs
in Sioyek. No progress panel or download-history manager. Handle destination
collisions without overwriting files silently. Institutional logins, eID, uploads
and printing are outside scope; Firefox handles those workflows.

## 8. Profiles and navigation

Design review proposes establishing a working research-site reference before
selective persistence or engine cuts. Keep the interface minimal while preserving
the web behavior needed by target sites; omitting login/media UI does not establish
that storage or web APIs are unnecessary. Current code has not applied most proposed
cuts, so their role in the Scholar challenge is unproven. See the separate
[research-browser design review](../findings/research-browser-design-review.md).

Separate preview and research profiles. Profile storage separation is not a complete
local-file sandbox. acceptNavigationRequest handles navigation, not all scripts,
images, fetches, frames or downloads.

- Preview origins must be registered; do not trust every localhost page.
- Restrict preview access to intended assets and avoid arbitrary application bridges.
- Retain minimal research-site state to avoid forgetting valid anonymous sessions.
  A fresh Scholar profile seeded with one clearance cookie reached results without
  copied cache or other site data. The agreed candidate retains Google's latest
  valid `GOOGLE_ABUSE_EXEMPTION`, `_GRECAPTCHA`, `GSP` and `__Secure-ENID` cookies
  across restarts, respecting expiry and deletion. `SEARCH_SAMESITE`, `OTZ` and
  `__Host-GAPS` remain temporary; allow their use during the research session.
  Capture replacements Google issues; silent renewal has not been established.
  Validate selective cookie import/export with
  temporary storage before choosing production policy; the named-profile test
  still allowed new disk writes during browsing. Do not save browsing history or
  tab/session lists. Login is not required for the tested workflow; future
  challenges remain possible. After clearance expired, the four-cookie,
  seven-cookie and complete Qt profiles were challenged while the user reported
  Firefox private mode worked. Retention has not established reliable Scholar
  access; investigate this compatibility gap before advancing cookie API work.
  Keep preview
  storage separate and define profile location and reset/cleanup behavior. See
  [the retention and scrolling findings](../findings/scholar-profile-and-scroll.md) and
  [cookie-minimization findings](../findings/scholar-cookie-minimization.md) and
  [post-expiry comparison](../findings/scholar-post-expiry.md).
- Remote links from previews open in the research window.
- Omit password-manager UI, account workflows, client-certificate selection and
  dedicated SSO popup support. Preserve ordinary link/new-tab behavior; login-specific
  exceptions are not a compatibility target.
- Keep Chromium sandboxing and certificate verification enabled.

Use Qt's default user agent initially. A Chrome user-agent string does not establish
Scholar compatibility or bypass bot detection. Test real queries and challenge flows.
No special AusweisApp/localhost permission bypasses.

## 9. Resource targets

"Under 200 MB idle" is an aspiration to evaluate, not a promise. Measure aggregate
PSS of qvu and Chromium helpers for blank, Markdown/math/images, Typst, research tabs,
and the combined workflow. Record startup and refresh latency too.

Start with engine defaults; test flags individually before disabling GPU, WebGL,
decoding or caches. Off-the-record profiles still keep data in memory. Minimal UI
chrome does not remove Chromium's rendering costs.

The active research tab is the selected, visible tab and must be loaded. A new
background tab remains an unloaded placeholder until selected. Previously loaded
tabs may retain their page/cache for reuse; retained cache is not a saved tab session.
Freeze eligible background research pages using Qt's recommended lifecycle state.
Discard only when losing/reloading state is acceptable. Protect live previews,
forms, authentication and downloads.

### Minimal feature policy and possible custom engine experiment

Proposed baseline remains the packaged engine with a small application policy:
disable the internal PDF viewer and spellchecker, omit extension management and
printing UI, deny camera/microphone/screen-capture/geolocation/notification requests,
and suppress audio/video playback. Confirm the playback policy against real pages:
muting, blocking autoplay and denying capture do not remove codecs or guarantee that
all media decoding/network activity stops. WebRTC is distinct from WebSockets;
Typst's live preview still needs WebSockets.

Keep JavaScript, HTML/CSS layout, font rendering, SVG, canvas and the graphics path
needed by the actual previews. Investigate WebGL only after Typst compatibility
testing; disabling all GPU acceleration is not synonymous with removing video support.

Future investigation, not an agreed build requirement: compile upstream QtWebEngine
with supported feature switches before considering a source fork. Candidate cuts
include WebRTC, printing/PDF, spellchecking, extensions and proprietary codecs.
Check options/dependencies for the selected release. Disabling proprietary codecs
does not disable every audio/video format; wholesale media removal may require patches.

QtWebEngine does not support static builds. A custom build still ships shared engine
libraries, Chromium helpers and runtime assets; it is not folded into one tiny qvu
executable. Keep it isolated from system Qt and preserve Chromium security updates.
Compare disk size, aggregate PSS, CPU/latency and preview/site compatibility with the
packaged baseline. Smaller binaries alone do not establish lower idle memory.
Only adopt a custom engine when measured benefits justify build/maintenance costs.

## 10. Implementation sequence

1. Dependency/rendering spike: packaged WebEngine when requested; CMake discovery,
   sandboxed Wayland launch, Markdown math and a representative Typst document.
2. Window/IPC skeleton: preview registry, research tabs, request acknowledgements,
   simultaneous startup handling, close behavior and Niri matching.
3. Combined workflow: targeted Markdown refresh/position retention, Typst opener,
   Sioyek searches, typing modes and link hints.
4. Research actions: papis capture, simple downloads/PDF handoff and Firefox shortcuts.
5. Refinement: multi-editor ownership, offline math, linked Markdown, minimal site state
   and measured memory policies.
6. Optional custom-engine evaluation after measuring the packaged baseline; keep
   institutional/login/media workflows in Firefox.

### Future phase: papis-ask answer previews

Agreed future interface (not currently supported):

```sh
pask "your question" --output qvu
```

Implement this in the existing Zsh `pask` wrapper: intercept the qvu output option,
request `--output markdown` from papis-ask, and hand the completed answer to qvu's
Markdown preview workflow. Preserve other arguments, including document/tag scope,
and the wrapper's existing credential-loading behavior. Terminal output remains
the default; existing Markdown and JSON options remain available.

- Generate the complete answer before publishing it. On failure, report the error
  and clean up partial output rather than opening an incomplete answer.
- Store each answer in a private, uniquely named directory under `$XDG_RUNTIME_DIR`.
  Give it a unique document ID and its own answer preview window.
- Use the Markdown rendering path, including math rendering; merely opening raw
  `.md` content in WebEngine is insufficient. Keep source and rendered assets while
  the window is open so reload, copying and saving continue to work.
- After a successful handoff, qvu owns cleanup of the temporary answer and generated
  preview assets when the window closes. Failed handoffs clean up immediately;
  define recovery for leftovers after crashes without deleting live previews.
- Saving an answer into notes or another durable file is an explicit action.
  Preserve citations and the distinction between paper evidence and personal notes.
- Asking is an on-demand subprocess operation. Do not embed Python/the ask backend,
  retain a loaded library index, start a persistent ask service, or automatically
  run refinement/indexing in qvu. API and transient memory costs belong to the
  requested operation; the integration still adds some command-handling code.

A later alternative is `pask "your question" --output markdown | qvu preview --stdin`.
Stdin support is not part of the current CLI; it would still require qvu to retain
the answer for the preview's lifetime. Prefer temporary files for the first adapter.
Clickable citations that open the referenced PDF/page in Sioyek are a subsequent
enhancement, after validating papis-ask's citation data and page-number conventions.

Validation for this phase: equations/citations, two concurrent answers, ask failure,
qvu handoff failure, close/crash cleanup, explicit save, and unchanged terminal output.

Before changing live openers, check two editors, same-named files in different
folders, preview saves while browsing, close/reopen, conversion failure, server exit,
Unicode/quoted queries and paths, and cursor sync in both directions for Typst.

## 11. Remaining brainstorming choices

- Same-document ownership transfer is agreed. Validate explicit transfer, ignored
  old-owner saves/disconnects, and current-owner disconnect behavior before implementation.
- Search tab reuse is agreed for identical engine/query requests; whether generic
  `qvu open URL` requests also reuse existing tabs remains open.
- Verify agreed explicit-search activation with Wayland/Niri. Background preview
  saves never steal focus.
- Tridactyl-style keys without a Ctrl+b prefix are agreed; finalize the complete
  binding table and visual layout during design review.
- No saved browsing history or research-tab restoration. New background tabs load
  on selection; previously loaded pages may be retained for reuse. The minimum
  research-site state has an agreed four-cookie Scholar candidate, based on an
  immediate one-cookie success. Post-expiry tests challenged Qt with both four
  and seven cookies and a complete profile, while Firefox private mode worked
  according to the user. Resolve Scholar compatibility before advancing selective
  persistence through the Qt API; discuss reliability as an implementation gate.
- Would supported custom-engine feature cuts yield enough measured benefit to maintain?
- Add bidirectional Markdown source navigation later, matching Typst's workflow?

## Evidence and references

Local evidence: Sioyek preferences/keys and README, sioyek-search, Neovim md_preview
and markdown modules, documents.lua, Tridactyl config, Niri config, Sioyek
main_widget.cpp, and read-only pacman/pkg-config inventory.

- [Qt WebEngine architecture](https://doc.qt.io/qt-6.8/qtwebengine-overview.html)
- [Widgets/CMake integration](https://doc.qt.io/QT-6/qtwebenginewidgets-module.html)
- [Profiles and storage](https://doc.qt.io/QT-6/qwebengineprofile.html)
- [Page navigation, windows and lifecycle](https://doc.qt.io/qt-6.11/qwebenginepage.html)
- [Arch qt6-webengine package](https://archlinux.org/packages/extra/x86_64/qt6-webengine/)
- [Qt WebEngine settings](https://doc.qt.io/QT-6/qwebenginesettings.html)
- [Qt WebEngine build requirements](https://doc.qt.io/qt-6.11/qtwebengine-platform-notes.html)
- [Qt WebEngine feature documentation](https://doc.qt.io/qt-6.11/qtwebengine-features.html)
- [Upstream feature-build checks (development branch)](https://github.com/qt/qtwebengine/blob/dev/CMakeLists.txt)
