> Historical qvu material imported on 2026-10-12. Qt-specific behavior and earlier decisions require WPE validation. Foob’s architecture decision and roadmap take precedence. Reproduction commands refer to the original qvu checkout.

# KaTeX asset-access investigation — 2026-10-12

Scope: explain the earlier Markdown math failure. No renderer changes, production
implementation, memory measurements or optimization. Added nine C++ lines for a
diagnostic setting toggle and report fields, followed by a user-facing checkpoint.

## Experiment

Use the HTML already produced by Neovim's renderer from
`probes/fixtures/rendering.md`. Its KaTeX script and stylesheet are pinned to 0.17.0
on jsDelivr. A temporary copy adds a resource-error listener before those assets.
The same copy is opened as a local file and through the existing loopback server.
Neither the source Markdown nor the live Neovim renderer is modified.

The probe defaults to Qt's `LocalContentCanAccessRemoteUrls = false`. Diagnostic
stdin commands `:local-remote on` / `:local-remote off` change only that page setting;
`:reload` reloads the current document. They are experiment controls, not a proposed
application-wide permission policy.

## Results

| Case | Setting | KaTeX loaded | Rendered equations | Math errors |
|---|---|---|---:|---:|
| Fresh process, `file://` | Off | No | 0 | 2 |
| Same file, allow remote assets and reload | On | Yes, 0.17.0 | 2 | 0 |
| Turn setting off after assets loaded, reload | Off | Yes | 2 | 0 |
| Second fresh process, `file://` | Off | No | 0 | 2 |
| Second process, loopback HTTP; setting never enabled | Off | Yes, 0.17.0 | 2 | 0 |

Both fresh local-file cases logged load errors for the CDN JavaScript and CSS.
The attempted render had `ReferenceError: katex is not defined`. Enabling the
local-to-remote setting allowed the same document to render both equations.
The final HTTP case was visually inspected and showed correctly typeset math.

The successful off-after-on case could reflect cached assets or retained state.
That mechanism was not isolated. It shows why a reload after changing settings
cannot replace a fresh-process control.

## What we learned

The reproduced failure is local-file access to external assets under Qt's default
policy, rather than a failure to support the tested KaTeX expressions. Qt documents
this setting as controlling local-origin access to normally blocked remote resources.
The CDN version exists and worked; no library upgrade was needed.

Loopback HTTP supports the current renderer without enabling that local-file
permission. Keep the default policy; this experiment does not justify a blanket
permission change.

The renderer catches JavaScript exceptions and marks spans `katex-failed`, but does
not report the exception. Future preview behavior should distinguish asset-load
failure from invalid TeX instead of silently leaving source text without an explanation.
That behavior has not been implemented.

## Proposed next experiment, awaiting user direction

Serve the same pinned KaTeX JavaScript, CSS and fonts locally, leave remote access
disabled, and test the fixture without CDN access. This would check whether local
assets make Markdown math independent of network availability. Broader TeX coverage,
bad expressions and automatic refresh remain separate functional checks.

References: [Qt settings](https://doc.qt.io/qt-6.11/qwebenginesettings.html),
[KaTeX browser setup](https://katex.org/docs/browser.html).
