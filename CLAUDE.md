# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

@AGENTS.md

`AGENTS.md` is the source of truth for structure, build and test commands, style, exit codes, the trust model and security boundaries. The notes below only add what it does not cover.

## Project state

- M0/M1 are implemented and audited (`docs/validation/audit-m1.md`). **Gate A awaits an explicit maintainer decision. Do not start M2 work** (controls, profiles, IPC, converters, custom engine) unless the maintainer authorizes it. A question about Gate A is not approval.
- Before resuming, read `docs/validation/handoff.md`, then `foob_architecture_decision.md` and `foob_development_roadmap.md`. Do not repeat checks the handoff records as complete.
- New findings follow the roadmap's triage rule (section 7): fix defects in existing behaviour now, record missing features in the milestone that owns them.
- Deferred by the maintainer: the font-language-menu glyph issue (`docs/validation/font-follow-up.md`) and the project license (M0-05). Do not attempt a fix or pick a license unprompted.

## Repo hygiene

- `build/`, `build-m0/`, `build-final/` are ignored local build directories. `design-references/qvu/`, `.agents/`, `.codex/` and `.aws/` are also ignored.
- `docs/research/qvu/` is historical Qt-era research, not a requirement.
- Another agent may share the checkout; check `git status` first and use a `git worktree` for work that needs a different branch.
- The global rule against `Co-Authored-By` or AI-attribution trailers in commits and PR bodies applies here.
