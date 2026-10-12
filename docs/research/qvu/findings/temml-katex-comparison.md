> Historical qvu material imported on 2026-10-12. Qt-specific behavior and earlier decisions require WPE validation. Foob’s architecture decision and roadmap take precedence. Reproduction commands refer to the original qvu checkout.

# Temml versus KaTeX — 2026-10-12

Decision: keep KaTeX as the preview candidate. The user prefers its clearer
appearance. Temml is viable for the six tested formulas after selecting a proper
math font, but this experiment does not show a compelling reason to migrate.

## Scope and setup

Extended `probes/math_compare.py` by seven additions and three removals, with a
checkpoint after the initial eight-line change and before the font control.
No C++, live Markdown renderer, dotfiles, or production architecture changed.
No packages installed. Generated assets and captures stayed in `/tmp`.

Compared the same nested fraction, matrix, aligned equations, piecewise cases,
summation and fluid equation as the earlier comparison. Runtime: Qt 6.12.0,
QtWebEngine 6.11.2, Wayland; viewport width 950 pixels, zoom 100%.
Temml 0.14.0 used local browser assets over loopback HTTP. KaTeX 0.17.0 used the
existing CDN fixture over loopback HTTP. Both retained the default disabled
`LocalContentCanAccessRemoteUrls` setting and an off-the-record profile.

## Functional findings

| Check | Temml | KaTeX |
|---|---|---|
| Six formulas | Six math elements, no observed parse/fallback errors | Six rendered expressions, no observed errors |
| Nested fraction | Two nested `mfrac` elements; denominator grouping visible | Grouping preserved visually |
| Matrix, aligned, cases | Two distinct rows in each | Multiline structure preserved |
| Aligned row separation | Approximately 29.4 pixels | Two rows; earlier geometry check approximately 29 pixels |
| Delimiter sizing | Failed with original local font configuration; fixed by explicit installed font family | Stretched parentheses and brace visible with bundled fonts |
| Appearance | Sans-serif math with installed Noto Sans Math | User prefers this output |

The original `Temml-Local.css` calls `NotoSansMath-Regular`, while this machine's
installed font family is `Noto Sans Math`. The original screenshot had matrix
parentheses 19 pixels high and a cases brace 18 pixels high, despite `stretchy=true`.
An explicit `math{font-family:"Noto Sans Math",math}` control increased them to
51 and 55 pixels respectively. A fresh page reproduced the corrected sizing.
All six expressions were inspected in captures, including the fluid equation
after scrolling. An immediate capture following CSS injection was stale; the
fresh-page capture and subsequent settled capture are the visual evidence.

This demonstrates a font-selection dependency, not a general Temml parsing
failure. The generator preserves both `temml.html` and `temml-noto.html`.
Temml's tiny `Temml.woff2` supplement is not a complete math font. Its
[administration documentation](https://temml.org/docs/en/administration)
describes the font choices and recommends Latin Modern for rendering quality.
Latin Modern was not tested; rejecting Temml universally would exceed the evidence.
The six formulas are not a general LaTeX correctness suite.

## Requested memory observations

One snapshot per fresh process, after load and before its window capture;
the Temml process exited before the KaTeX process launched:

| Page | Process-family PSS | Process count |
|---|---:|---:|
| Temml with explicit installed Noto Sans Math | 296.9 MiB | 5 |
| KaTeX | 316.5 MiB | 5 |

Sampled using `bash probes/memory.sh PID`. Difference: 19.6 MiB, roughly 6% of
the KaTeX sample. These totals include qvu and Chromium descendants, exclude
the HTTP server and completed fixture generator, and prorate shared mappings.
They are single observations, not stable rankings or production footprint claims.
Asset sourcing and fonts differ; desktop activity and shared mappings also affect
PSS. These totals differ substantially from the earlier session's snapshots;
do not compare absolute values across sessions or combine them into an average.
A 328.2 MiB sample from a reused window that had loaded both libraries was discarded.

## Reproduction and next proposed check

Download the pinned npm tarball without installing npm dependencies:

```sh
curl --fail --location https://registry.npmjs.org/temml/-/temml-0.14.0.tgz --output /tmp/qvu-temml-0.14.0.tgz
mkdir -p /tmp/qvu-render-runtime/nvim_md_preview/math-comparison/temml-assets
tar -xzf /tmp/qvu-temml-0.14.0.tgz --strip-components=2 -C /tmp/qvu-render-runtime/nvim_md_preview/math-comparison/temml-assets package/dist/temml.min.js package/dist/Temml-Local.css package/dist/Temml.woff2
MATHUNICODE_NO_SERVER=1 ~/projects/mathunicode/.venv/bin/python probes/math_compare.py /tmp/qvu-render-runtime/nvim_md_preview/math-comparison
```

The downloaded tarball's SHA-512 matched its npm metadata:
`sha512-tz1LW/5IZpThK8c7ZpdTSH/n56eAj+ThirLieErtKxYC3EKCIiB4sKdRUehAgH+fQmvIKqMfP3gqTLBdLpy9zw==`.
The three browser assets total 186,277 bytes on disk; this is not RAM usage.
The initially tried `temml.org/temml.min.js` and adjacent root URLs returned HTML
with status 200. Those files were replaced with verified tarball assets before testing.

Serve the generated directory through the existing loopback fixture server and
open `math-comparison/temml-noto.html` or `math-comparison/katex.html` with the
unchanged rendering probe. The Noto control requires the installed font family.
The probe's built-in `mathErrors` report only counts KaTeX selectors; Temml was
checked separately with JavaScript for `merror`, `.temml-error`, `.temml-failed`,
math count, table-row geometry and console messages.

Next proposed experiment remains pinned KaTeX JavaScript, CSS and fonts served
locally, to establish CDN-independent rendering. No migration is implemented.
