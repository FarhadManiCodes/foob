# Guided checks and task records for M2

One section per task (D-33). When a task's code is done, its guided rows go here and the
maintainer runs them on that task's commit; a task is done only when its rows pass. A row
is guided only when it needs a real key, a real click or a real account; everything else
is automated (unit tests, `tests/check_cli.py`, the nested niri suite). Each section also
records the task's binary size and its growth (D-35) and the evidence that is not a
guided row.

## Step 1: F-77 and M2-02's command line

Order: D-30 and D-38; choices: D-39. **Guided rows: none.** Every check in step 1 is
automated; the maintainer reviews the commits and this section.

### F-77: core dumps off outside diagnostics

Without `FOOB_DIAGNOSTICS=1`, `main.c` sets the core-dump limit to 0 (soft and hard)
before WebKit starts any process, and makes foob itself non-dumpable
(`PR_SET_DUMPABLE`); if either call fails, foob refuses to start (exit 1). A diagnostics
run keeps core dumps (D-26).

**Automated (nested niri).**
- `core-dumps-off` (diagnostics off, a loopback page so that the network process runs):
  - All eight processes of the tree have the core limit 0/0: foob, `WPEWebProcess`,
    `WPENetworkProcess`, `xdg-dbus-proxy` and four `bwrap`.
  - foob's `/proc` files belong to root, which is how the kernel shows a non-dumpable
    process.
- `core-dumps-diagnostics`: the same processes keep the runner's limit, and foob stays
  dumpable.
- The harness raises its own soft limit to the hard limit first, so the test cannot pass
  just because the runner already had core dumps off.
- Negative control: on the M1 binary (`a3ba0ca2…`), `core-dumps-off` fails: every process
  is unlimited and foob is dumpable.

**One-time crash checks** (2026-10-19, allowed by the maintainer in D-39).
- Setup: nested niri, a scratch script that sends `SIGABRT` to one process of foob.
- Why not `kill -SEGV` (the signal F-77 proposed): the web process kept running. Its
  JavaScript engine has its own `SIGSEGV` handler, which absorbs a signal sent from
  outside, so that signal is not a crash test for WebKit.
- Results:

| # | Build | Process aborted | Result |
|---|---|---|---|
| 1 | limit only, default mode | `WPEWebProcess` (about:blank) | foob exits 4. `coredumpctl` shows `COREFILE none` ("terminated abnormally without generating a coredump"). The journal entry still holds metadata: command line `WPEWebProcess 4 15`, environment, open descriptors, memory maps, working directory |
| 2 | limit only, default mode | foob, on a scratch page named as a marker | killed by signal 6, no core. **The entry's command line holds the address** (`build/foob file:///…/foob-f77-marker.html`), so foob also became non-dumpable |
| 2b | final | foob, same marker page | no entry at all; the marker appears nowhere in the journal |
| 1b | final | `WPEWebProcess`, on the marker page | foob exits 4, `COREFILE none`; the marker appears nowhere in the journal |
| 3 | final, `FOOB_DIAGNOSTICS=1` (negative control) | `WPEWebProcess` (about:blank) | `COREFILE present`: one core of 3.2 MB stored under `/var/lib/systemd/coredump` (allowed in D-39; systemd removes it later) |

**What remains.**
- A crashed WebKit helper still leaves a journal entry. It has metadata but no memory,
  and the helper's command line holds no address (row 1b).
- A crashed foob leaves nothing.
- In default mode, a non-dumpable foob cannot be attached with gdb, and other processes
  cannot read its `/proc` files. Debugging uses `FOOB_DIAGNOSTICS=1`.

**Checks on this commit.**
- Unit tests 3/3; nested suite 10/10.
- ASan+UBSan: 3/3 and 10/10, with no report.
  - LeakSanitizer suppressions as in [audit-m1.md](audit-m1.md).
  - The run uses `ASAN_OPTIONS=disable_coredump=0`. Otherwise ASan sets the soft limit to
    1 on its own, and `core-dumps-diagnostics` fails on ASan's limit, not foob's.
- `gcc -fanalyzer`: clean.

**Size (D-35).** The stripped binary stays at 22,520 B: the growth fits in the existing
page padding. `.text` grows from 13,473 to 13,799 B (+326 B, +2.4%) and `.data` by 16 B.
`main.c.o` grows from 499 to 705 B, from the two system calls, their error message and
their PLT entries.

| Object | `.text` M1 (B) | `.text` now (B) |
|---|---|---|
| `src_browser.c.o` | 2,253 | 2,253 |
| `src_diagnostics.c.o` | 2,165 | 2,165 |
| `src_main.c.o` | 499 | 705 |
| `src_target.c.o` | 1,143 | 1,143 |
| `src_window_title.c.o` | 1,498 | 1,498 |
