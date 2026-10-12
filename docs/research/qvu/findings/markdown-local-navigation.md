> Historical qvu material imported on 2026-10-12. Qt-specific behavior and earlier decisions require WPE validation. Foob’s architecture decision and roadmap take precedence. Reproduction commands refer to the original qvu checkout.

# Local Markdown preview navigation — 2026-10-12

Scope: check local HTML transport, relative images and links using the existing
Neovim renderer. No C++ or live Neovim changes, no new HTTP server, no optimization
or memory measurements. Public fixtures and a machine-local driver are preserved
under `probes/`. A checkpoint followed image loading and initial link inspection.

## Setup

`probes/local_navigation.lua` loads the existing `md_preview.lua` module in an
isolated `nvim --headless -u NONE -i NONE` process. It calls only the exposed
compile seam, generates both public Markdown fixtures, and changes CDN asset
references only in the generated test HTML. The Neovim opener and server lifecycle
are not called. Compilation uses a private, task-specific `XDG_RUNTIME_DIR`.
Local KaTeX 0.17.0 assets are copied beside the generated pages.

The unchanged Qt probe opens the generated page through `file://` with
`LocalContentCanAccessRemoteUrls=false` and an off-the-record profile.
Runtime: Qt 6.12.0, QtWebEngine 6.11.2, Wayland, viewport width 950 pixels.

## Observations

| Case | Result |
|---|---|
| `./local%20figure.svg` | Loaded and visible, natural size 480 × 120 |
| `../fixtures/local%20figure.svg?example=1#diagram` | Loaded and visible, same natural size; query, cache-busting parameter and fragment retained |
| Local KaTeX inline formula | Rendered, zero reported math errors |
| Explicit `#destination` target | Stayed in the same document and scrolled to y=633 |
| `#destination-heading` | Fragment changed but scrolling remained y=0; the generated heading has no ID |
| `./linked-note.md` | Resolved to the generated HTML directory; clicking produced a file-access error |
| `../fixtures/linked-note.md` | Resolved to another runtime-directory path; that file does not exist |
| Resolve both `.md` links against original source directory | Both correctly identify the existing `linked-note.md` source |

The renderer rewrites relative image URLs through hashed directory symlinks.
Those symlinks and encoded filenames work for these SVG images through local-file
transport. The screenshot showed both images and the formula. Original source
documents and live dotfiles were not changed.

The relative Markdown failure is expected without an adapter: the actual source
exists, and its separately compiled HTML exists under a different hashed filename.
The browser cannot infer that mapping from the clicked URL. Resolving raw Markdown
relative links against a generated HTML directory is also wrong with the current
HTTP layout; changing transport alone does not supply source context or conversion.
No HTTP comparison was run during this experiment.

Missing heading IDs are a renderer concern, not evidence of a local-file anchor
failure. Existing explicit IDs work. Automatic heading-anchor generation should
be considered in the Neovim renderer work; it was not implemented here.

## Agreed routing and transport implications

The user agreed that fragments stay in the preview, web links open in research
browsing, and linked Markdown documents go to the renderer for conversion.
Those routes are not implemented by the experimental probe.

Local HTML is the preferred qvu Markdown transport candidate, subject to validating
source-aware linked-document navigation. The current probe does not pass that test.
Preserve original source
path context and the raw link target, or have the renderer supply resolved source
paths. Do not reconstruct source paths from hashed HTML-cache URLs. Keep math
assets and image aliases available for the preview's lifetime. Output ownership
isolation still applies even though this path needs no Markdown HTTP server.

This is not a claim that all local HTML is portable as a single copied file.
The tested document depends on adjacent KaTeX assets and live directory symlinks.
Remote images, other image formats, arbitrary relative HTML links, Firefox behavior,
preview access restrictions, refresh/focus preservation and multiple owners were
not exercised. Typst's live server is unaffected by this transport choice.

## Reproduction

```sh
mkdir -p /tmp/qvu-local-nav-runtime
chmod 700 /tmp/qvu-local-nav-runtime
XDG_RUNTIME_DIR=/tmp/qvu-local-nav-runtime nvim --headless -u NONE -i NONE -l probes/local_navigation.lua
cp -R /tmp/qvu-render-runtime/nvim_md_preview/math-comparison/katex-assets /tmp/qvu-local-nav-runtime/nvim_md_preview/katex-assets
```

The asset directory comes from the pinned download described in
[the local KaTeX report](katex-local-assets.md). The driver records the generated
main page and linked page paths in `/tmp/qvu-local-navigation-paths.txt`.
Open the main page with the unchanged probe. Do not start the renderer's opener
or server merely to run this local-file experiment.
