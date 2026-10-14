# foob documentation

The current project scope is defined by the existing
[architecture decision](../foob_architecture_decision.md) and
[development roadmap](../foob_development_roadmap.md).

Implementation: [approved M0/M1 plan](plans/m0-m1.md),
[M0 validation](validation/m0.md), and [build instructions](../README.md).

**Start here:** [handoff](validation/handoff.md) (current state and rules for every agent) and
[findings, decisions and ideas](validation/findings.md).

Current [handoff](validation/handoff.md): M0/M1 P0 complete; M1 implemented and
independently reviewed, with [runtime evidence](validation/m1.md),
[measurements](validation/measurements.md) and
[desktop checks passed](validation/desktop-checklist.md). Gate A is ready for
maintainer review, not approved. The [MiniBrowser comparison](validation/minibrowser-comparison.md) (D-22) is the baseline
for roadmap tasks M2-13 and M3-09. A [font issue](validation/font-follow-up.md)
reported during guided Scholar testing is explicitly deferred.

[Blank-memory/GPU diagnosis](validation/blank-memory.md) identifies engine,
allocation and graphics startup costs behind the >100-MiB footprint; actual GPU
work is confirmed for blank. [Compact results](validation/results.json) stay
in Git; generated raw `docs/validation/runs/` outputs are ignored and kept locally.

The [qvu research handoff](research/qvu/README.md) contains earlier design discussion,
math/local-document experiments and Scholar compatibility findings. Those records
are historical evidence, not authorization to implement the old Qt design in foob.

The [latest navigation and Dooble observations](research/qvu/findings/scholar-navigation-and-dooble.md)
explain the immediate context for evaluating WPE.

Reference mockups are in `design-references/qvu/` and are excluded from Git. They
illustrate the earlier layout only; foob's current single-view, toolkit-free scope
supersedes that layout.

The historical transfer added documentation and reference images only. No qvu source code,
build outputs, private browser profiles, captures or credentials were imported.
