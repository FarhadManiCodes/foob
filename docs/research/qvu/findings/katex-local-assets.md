> Historical qvu material imported on 2026-10-12. Qt-specific behavior and earlier decisions require WPE validation. Foob’s architecture decision and roadmap take precedence. Reproduction commands refer to the original qvu checkout.

# Local KaTeX assets — 2026-10-12

Conclusion: the six-formula fixture renders with fully local KaTeX 0.17.0 assets
through both `file://` and loopback HTTP. Neither path requires enabling
`LocalContentCanAccessRemoteUrls`. This resolves the asset-access problem for
the experiment; the live Neovim renderer still references the CDN and is unchanged.

## Scope and setup

Added two lines to `probes/math_compare.py` to generate `katex-local.html` by
replacing the CDN prefix with `katex-assets/`. All equations, rendering options
and layout CSS remain identical to the previous KaTeX comparison fixture.
No C++ changes, package installations, memory measurements or production work.

Downloaded the pinned npm package tarball into `/tmp`, verified its SHA-512
against npm's version metadata, and extracted only the built browser JavaScript,
CSS and fonts. Kept `fonts/` alongside the CSS, preserving its relative URLs.
Used the existing mathunicode Python environment to generate fixtures and run
a temporary server bound to `127.0.0.1:8876`.

Runtime: Qt 6.12.0, QtWebEngine 6.11.2, Wayland, viewport width 950 pixels.
Each transport was tested in a fresh probe process with an off-the-record
profile; the file process exited before starting the HTTP process. The
local-to-remote setting remained false throughout, with no toggle or reload
after previously loading CDN assets.

## Observations

| Fresh page | KaTeX version | Expressions | Math errors | Loaded font faces |
|---|---|---:|---:|---:|
| `file://…/katex-local.html` | 0.17.0 | 6 | 0 | 6 |
| `http://127.0.0.1:8876/katex-local.html` | 0.17.0 | 6 | 0 | 6 |

Loaded faces: Main regular/bold, Math italic, Size2, Size3 and Size4.
The file page's captures show the nested fraction, matrix parentheses, aligned
equations, cases brace, summation and fluid equation. The aligned HTML rows
are approximately 29 pixels apart. Matrix, aligned and cases MathML each
contain two rows; the nested fraction contains two nested fraction elements.

The HTTP resource timing log contains the local CSS, JavaScript, six WOFF2
fonts and a favicon request. Every recorded URL has the loopback page's origin.
The server returned 200 for all rendering assets; its favicon 404 did not affect
math. `document.fonts` reports the same six faces loaded. DOM structure and
element geometry were checked separately. The HTTP window capture was black,
so it is not evidence of visual correctness; the file captures provide the
visual evidence for this same generated document.

Resource timing returned an empty list on the file page. That list alone cannot
verify file loading; local asset references, the loaded library version, loaded
font faces and visible output provide the evidence there.

The machine's network was not disconnected. The fixture uses local assets and
the HTTP trace showed no external resources. Qt's local-to-remote setting is a
file-origin restriction, not a global networking switch for HTTP pages.
These are six expression checks, not general TeX coverage or a complete Markdown
integration test. Refresh, missing-asset messages and invalid TeX remain untested.

## Reproduction

```sh
curl --fail --location https://registry.npmjs.org/katex/-/katex-0.17.0.tgz --output /tmp/qvu-katex-0.17.0.tgz
mkdir -p /tmp/qvu-render-runtime/nvim_md_preview/math-comparison/katex-assets
tar -xzf /tmp/qvu-katex-0.17.0.tgz --strip-components=2 -C /tmp/qvu-render-runtime/nvim_md_preview/math-comparison/katex-assets package/dist/katex.min.js package/dist/katex.min.css package/dist/fonts
MATHUNICODE_NO_SERVER=1 ~/projects/mathunicode/.venv/bin/python probes/math_compare.py /tmp/qvu-render-runtime/nvim_md_preview/math-comparison
```

Verified tarball integrity:
`sha512-Vdw0ATsQ9V+LuegM/BTwQqV/6cTl5lbGcIrU+BCgLxyf6bo38ybOr372tuSIxir3CN720flu1meYR6XzNMwQnw==`.

Open the generated local file with the unchanged probe. For HTTP, serve the
`math-comparison` directory with the existing environment's `python -m http.server`
bound to loopback, and open `/katex-local.html`. No Node process or npm install
is needed at rendering time.

## Design implication and next discussion

Keep KaTeX as the preview candidate and plan to package pinned JavaScript, CSS
and fonts together. Loading them locally avoids the reproduced CDN restriction
and makes these fixture equations independent of CDN availability. Preserve the
default remote-access policy. This agrees with
[KaTeX's local hosting instructions](https://katex.org/docs/browser.html).

Before changing the Neovim integration, decide who owns and locates these assets:
the existing renderer or qvu's preview transport. The asset test establishes
feasibility, not that ownership decision or authorization for production changes.
