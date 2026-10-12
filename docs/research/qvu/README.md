# qvu research handoff to foob

Copied on 2026-10-12 from `~/projects/qvu`, after the user chose
toolkit-free WPE WebKit for foob.

Foob's [architecture decision](../../../foob_architecture_decision.md) and
[roadmap](../../../foob_development_roadmap.md) take precedence. The transfer does
not reopen engine selection, start coding, or introduce Qt/GTK dependencies.

## What remains useful

| Topic | Reusable information | Record |
|---|---|---|
| Offline math | Bundled KaTeX JS/CSS/fonts worked in Qt; network access policy caused the initial CDN failure. Retest in WPE. | [Local assets](findings/katex-local-assets.md), [initial failure](findings/katex-investigation.md) |
| Math alternatives | Temml and KaTeX rendered multiline formulas; mathunicode's single-line model did not meet the matrix requirement. | [Math comparison](findings/math-renderer-comparison.md), [Temml/KaTeX](findings/temml-katex-comparison.md) |
| Markdown resources | Relative images, spaces and anchors worked in the tested setup; linked Markdown needed original source-path context and renderer conversion. | [Local navigation](findings/markdown-local-navigation.md) |
| Scholar | Header changes did not avoid the challenge; valid clearance enabled temporary access; post-expiry retention did not resolve Qt/Firefox differences. | [Header trials](findings/scholar-experiments.md), [post-expiry test](findings/scholar-post-expiry.md), [latest observations](findings/scholar-navigation-and-dooble.md) |
| Profile behavior | Cookies, cache, ordinary storage and saved tab/history lists are distinct decisions. The selected-cookie proposal was not a proven long-term compatibility solution. | [Retention](findings/scholar-profile-and-scroll.md), [cookie tests](findings/scholar-cookie-minimization.md) |
| API boundaries | Qt permission and local-resource behavior were investigated; they do not describe WPE's API or prove Google's detection rule. | [Read-only Qt investigation](findings/scholar-readonly-investigation.md) |
| Experimental method | Preserve a reference configuration, separate compatibility from optimization, and test restrictions against evidence. | [Design review](findings/research-browser-design-review.md), [initial rendering](findings/rendering-findings.md) |
| Prior workflow discussion | Neovim independence, Sioyek/PDF handoff, papis actions and future answer-preview ideas may inform later integration. | [Historical blueprint](design/qt6_minimalist_research_viewer_blueprint.md) |

Qt memory snapshots are observations of those particular processes and workloads.
They predict neither WPE's footprint nor achievement of foob's stretch target.

## Scope differences to preserve

| Earlier qvu discussion | Current foob direction |
|---|---|
| C++20, Qt Widgets/WebEngine, CMake | C23, GLib/GObject, Meson, WPEPlatform Wayland |
| Research tabs and a bottom status strip; NORMAL/INSERT controls | One web view per window, no persistent chrome; tabs and prompt/mode design deferred |
| Selected four-cookie persistence candidate; no saved research tabs | Dedicated persistent network session; private mode and richer session decisions follow foob's roadmap |
| Renderer-owned HTML/assets with local-file and loopback experiments | No localhost server; authorized local HTML first, guarded `foob:` resources later |
| Live tinymist preview, pagination and bidirectional Neovim sync | Investigate Typst HTML/MathML export; faithful page layout remains in an external viewer |
| LLM chat handed to Firefox | Optional AI-site compatibility tests if they fit foob's stages |
| Qt under-200-MiB aspiration | Foob's under-100-MiB lightweight-page stretch target; neither is a verified result |

These differences are deliberate scope boundaries, not discrepancies to silently
merge. The old Qt implementation proposal and all probe code remain in qvu. No
old AGENTS.md or production rules were imported into foob.

## Reference images

Three user-supplied mockups were copied unchanged:

- [Research window](../../../design-references/qvu/research-window.png): historical tabs/status layout.
- [Markdown preview](../../../design-references/qvu/markdown-preview.png): math and document styling reference.
- [Typst preview](../../../design-references/qvu/typst-preview.png): historical paginated appearance.

They are ignored by `/design-references/qvu/` in foob's `.gitignore`. They are local
references and will not appear in a fresh clone unless the user later chooses to
track them.

## Reproduction and provenance

Imported experiment commands refer to the original qvu checkout and its probes.
Run them from `~/projects/qvu`; source code was preserved on branch
`spike/rendering`, including independent navigation probe commit `f8d314d`.
Browser profiles and raw diagnostics under `/tmp` were not transferred. Some
historical files describe an earlier open question that a later record resolves;
read the later Scholar records before interpreting those statements as current.
