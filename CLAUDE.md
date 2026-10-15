# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

@AGENTS.md

`AGENTS.md` is the source of truth for structure, build and test commands, style, exit codes, the trust model and security boundaries. The notes below only add what it does not cover.

## Project state

- M0/M1 are implemented and audited (`docs/validation/audit-m1.md`). **Gate A awaits an explicit maintainer decision. Do not start M2 work** (controls, profiles, IPC, converters, custom engine) unless the maintainer authorizes it. A question about Gate A is not approval.
- Before resuming, read `docs/validation/handoff.md` and `docs/validation/findings.md` (current state, decided items, open findings), then `foob_architecture_decision.md` and `foob_development_roadmap.md`. Do not repeat checks the handoff records as complete or re-argue decided items. Update both files in the same commit as any change, finding or decision.
- New findings follow the roadmap's triage rule (section 7): fix defects in existing behaviour now, record missing features in the milestone that owns them.
- Deferred by the maintainer: the font-language-menu glyph issue (`docs/validation/font-follow-up.md`). Do not attempt a fix unprompted. License: BSD-2-Clause (D-15); new source files carry `SPDX-License-Identifier: BSD-2-Clause`.

## Repo hygiene

- `build/`, `build-m0/`, `build-final/` are ignored local build directories. `design-references/qvu/`, `.agents/`, `.codex/` and `.aws/` are also ignored. `reference-minibrowser/` (upstream MiniBrowser source for the M2-13/M3-09 comparison, not foob source, never built or run) is ignored through `.git/info/exclude` and may be absent; see `docs/validation/minibrowser-comparison.md` to refetch it.
- `docs/research/qvu/` is historical Qt-era research, not a requirement.
- Another agent (Codex) may share the checkout; check `git status` and `git branch --show-current` first. Work in the main checkout on the branch the maintainer names; do not create worktrees, clones or extra project folders without asking, because the maintainer reviews and runs other agents in this one folder.
- The global rule against `Co-Authored-By` or AI-attribution trailers in commits and PR bodies applies here.
