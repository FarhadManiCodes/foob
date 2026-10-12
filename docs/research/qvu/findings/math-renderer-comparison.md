> Historical qvu material imported on 2026-10-12. Qt-specific behavior and earlier decisions require WPE validation. Foob’s architecture decision and roadmap take precedence. Reproduction commands refer to the original qvu checkout.

# Math renderer comparison — 2026-10-12

Scope: functional and visual comparison, plus memory snapshots explicitly requested
by the user. No C++ changes or optimization work. The public fixture generator is
`probes/math_compare.py`, run with mathunicode's existing project environment.
It was introduced as a 40-line batch followed by a checkpoint; four additional
lines preserve the direct MathML control. No live renderer/library was changed.

## Inputs and environments

Six identical expressions: a nested fraction, a two-by-two matrix, aligned equations,
piecewise cases, a summation with a fraction, and the fluid equation used earlier.
Each generated page has the same body CSS and was opened through loopback HTTP
in a fresh probe process with an off-the-record profile. Viewports were 950 pixels
wide. Runtime: Qt 6.12.0, WebEngine 6.11.2, latex2mathml 3.81.1, KaTeX 0.17.0.

## Functional results

| Path | Observed result |
|---|---|
| mathunicode | All six outputs contain zero newlines. Matrix/cases/aligned rows are flattened deliberately. User excluded this from further preview consideration. |
| latex2mathml output rendered natively | Nested fractions, matrix rows and cases render as structured math. `aligned` fails: literal ampersands appear and the equations stay on one line. |
| Hand-written MathML control | A proper `mtable` renders two rows, with row positions approximately 25 pixels apart. The aligned failure is in the generated markup, not an inability to render a table. |
| KaTeX | All six expressions render without reported errors. Nested fractions, matrices and cases were inspected visually; the aligned output contains two table rows, with visible HTML row positions approximately 29 pixels apart. |

The installed converter emits `mspace linebreak="newline"` and literal `&` tokens
for `aligned`, rather than an `mtable`. Correcting that conversion would be additional
work; it is not a qvu or mathunicode change made during this test.

An additional mathunicode issue was observed: `\frac{a+b}{\frac{c}{d}}` becomes
`(a + b)/c/d`. It loses denominator grouping under ordinary slash interpretation.
This is recorded for separate investigation, not fixed or used as the default preview.

These are checks of a small set of expressions, not a general LaTeX accuracy score.
Some window captures were affected by compositor capture behavior; DOM structure
and element positions were checked as well. An error count of zero alone does not
prove correct mathematical layout, as the native aligned case demonstrates.

## Requested memory observations

One snapshot per fresh browser process, before its window capture:

| Path | Aggregate process-family PSS | Processes |
|---|---:|---:|
| Native MathML | 173.0 MiB | 5 |
| KaTeX | 188.4 MiB | 5 |

The earlier Unicode sample was 165.3 MiB but that layout is excluded by the user.
These figures include qvu/Chromium, exclude the HTTP server and converter, and are
not repeats or a controlled benchmark. PSS sharing depends on other desktop
processes. They do not establish a stable ranking or an optimized qvu footprint.

The separate Python conversion process used approximately 22.8 MiB PSS after
imports/conversion, versus roughly 12 MiB before converter imports. That snapshot
included both converter imports, not an isolated per-library cost. The process
exited after generating HTML; it was not retained in the browser or a warm server.

## Recommendation and next proposed check

Use KaTeX for the current preview plan. Its tested output preserves the required
multiline layout without adapting the converter. Native MathML remains possible,
but the current conversion path is not a drop-in equivalent for these expressions.
The memory observations do not justify choosing the failing conversion path.

Next, test the same pinned KaTeX JavaScript, CSS and fonts served locally with
remote access disabled. That would establish CDN-independent rendering. No renderer
migration or new C++ work is authorized by this recommendation alone.
