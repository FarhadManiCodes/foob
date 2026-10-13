# Independent C23 / public API / lifetime review

**Current status:** all blocking review findings resolved; subsequent guided
desktop checks pass, M1 P0 complete, Gate A ready for maintainer review and not
approved. The review sections below record their state before user validation;
their historical limitations do not mean today's desktop checks are pending.

Raw reports used for review are now ignored and retained locally; committed
[results.json](results.json) and Markdown preserve curated evidence. Subsequent
[blank diagnosis](blank-memory.md) adds hardware GPU execution evidence without
changing the reviewed browser source/binary. The new optional attribution helper
does not imply another independent source review was performed.

Requested fresh-context read-only reviewer: `/root/c23_review`.
Initial review completed 2026-10-13 local time; parent M0 commit `ed52f71`.
Reviewed implementation was uncommitted and identified by exact hashes:

- `src/main.c`: `6bf147f1390eef2881a8180f418236e5056373c336e081f71f20cb880995bce2`
- `build/foob`: `9a9225d877defb9f6c574ea1929653fdbef5355ad8db5ab5dc99e4ee10283229`

Reviewer made no edits, performed no GUI tests and **did not approve Gate A**.

## Initial findings and disposition

| Severity | Finding | Main-agent resolution |
|---|---|---|
| Medium | Benchmark could pass after post-load crash/failure/new navigation; PSS might measure a dead or different page | Fixed: health/navigation checks throughout wait/sampling, before shutdown and after exit; reject changed load count and changed geometry. Deliberate post-load web-process kill is detected as failure with zero samples. Rerun full suite; preliminary runs excluded. |
| Medium | Desktop probe termination timeout could skip cleanup/evidence write and leave process running | Fixed: catch timeout, mark failure, kill/reap disposable root, inspect tracked survivors and write JSON. |
| Low | Desktop probe accepted stale successful load without checking later failure/crash | Fixed: check failure first, throughout settling/captures and after exit. Execution pass remains separate from visual/user acceptance. |
| Low | README still described M0 exit-only executable | Fixed: current launcher/URI/session/diagnostic/shutdown/test instructions documented. |

Main-agent discovery: benchmark deadline subtraction could become negative
between clock reads. Fixed with nonnegative sleep clamp. First suite stopped
on this error; original failed and preliminary artifacts are retained in
`runs/preliminary/` and excluded from accepted results.

## Reviewer conclusions

No high/critical C23, API, ownership or callback-lifetime defect found.

- Actual compile uses `-std=c23 -Wall -Wextra -Wpedantic`. Meson feature/version
  probe and source guard reject older standards; preprocessing reports
  `__STDC_VERSION__=202311L`, `__STRICT_ANSI__=1`; no GNU/C17 fallback.
- Independent `cc -std=c23 -Wall -Wextra -Wpedantic -Werror -fsyntax-only`
  with installed pkg-config include flags passes without diagnostics.
- Application-owned constructs are C23. GLib `g_autoptr`/`g_autofree` use
  documented GCC cleanup attributes; no claim of an extension-free build.
- Installed headers/GIR match public APIs, construction properties and signal
  callback signatures. WebKit view is a GObject (not floating); owned view,
  session and display references are released. WPE view/toplevel are borrowed.
- Stack callback state survives the main loop. Unix signal and idle sources are
  removed and handlers disconnected before owned state is released. Signal
  callback `G_SOURCE_CONTINUE` keeps IDs valid for explicit removal.
- Process tracking includes descendants and session members, preserves
  reparented helpers, checks PID/start-time identity for cleanup, and rejects
  unreadable PSS rather than dropping processes.
- Explicit Wayland/one view/ephemeral session/trusted startup/fixtures remain
  within M0/M1; no private engine APIs, toolkit chrome, persistent profiles,
  custom-engine packaging or security overrides.

Evidence reviewed: all four required project documents, source/Meson/README,
fixtures and validation tools, compile commands/configure/test logs, installed
public headers/GIR, compositor-close/SIGINT records, and SIGTERM smoke record.
Lifecycle evidence matches hashes, exit 0/no tracked survivors, native Wayland
and web-process confinement. Accessibility warnings noted.

Limits: no independent runtime launch, visual/input verification, acceleration
proof or completed memory-suite audit in initial review. Runtime/user evidence
remains separate; at the initial review, M1 was incomplete pending interaction.

## Focused follow-up

Completed by the same fresh-context reviewer on 2026-10-13. All four original
findings confirmed resolved; no remaining blocking review finding.

Reviewer inspected all ten final measurement records: nine fresh launches and
one warm control, three samples each, eight measured processes, equal 950×1188
viewport/scale 1, correct PSS sums, sample times meeting the ten-second minimum,
exit 0 and no survivors. Accepted logs contain no load-failure/abnormal-termination
markers. Crash negative control rejected post-load termination with zero samples.
Source/binary hashes unchanged.

Follow-up was read-only code/saved-evidence inspection. It did not independently
launch GUI tests or inject a hung-browser cleanup test. Visual/input acceptance
remains separate; at this follow-up, M1 was incomplete pending interaction and
Gate A was not approved.

## Final geometry follow-up

Final log audit found transient 950→1908→950 geometry in the original fixture-3,
which sampled-endpoint checks alone missed. The main agent excluded that run,
added continuous checks of every logged geometry report after final completion,
and collected a stable replacement. An intermediate retry with a second
navigation was correctly rejected and retained excluded.

Reviewer confirms the incremental guard catches transient changes through idle,
sampling and shutdown. Reviewed collector SHA256:
`fa9a1e07023100af7501954456da02bc4a93b5a80015f6a1851ebd982278941e`.
Replacement fixture-3 passes at 950×1188/scale 1, eight processes, samples about
10/11/12 seconds, correct PSS sums, exit 0/no survivors. Reviewer retrospectively
checked all ten accepted logs for expected navigation counts, no failure markers
and no differing post-completion geometry. This strengthens the earlier review,
which inspected sampled geometry only. No remaining blocking review finding;
source/binary unchanged, no GUI test or Gate A approval. At that point M1
interaction was pending; the later guided results below supersede that status.

Later maintainer-guided interaction (2026-10-13) now passes the required desktop
checks; see [the separate runtime evidence](interactive-2026-10-13.md). Browser
source/binary hashes remain identical to this review. This updates milestone
acceptance evidence without implying that the reviewer performed GUI tests or
approved Gate A. The font-menu observation is explicitly deferred by the maintainer.
