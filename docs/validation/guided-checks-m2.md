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

#### Rework after the independent reviews (D-41)

The two independent reviews of the F-77 commit (code and security) found no defect in
what it does. They raised three points:

- **Portals.** The code review measured that a non-dumpable process is refused by every
  desktop portal ("Unable to open /proc/PID/root"). Portals are planned for the M3 file
  chooser.
- **Guard after parsing.** The guard ran after GLib had parsed the argument. That argument
  will carry page-supplied text once D-23 spawns `foob --private URL`.
- **Harness.** The nested harness raised the core limit for every scenario.

**What changed.**
- foob now sets the core limit of itself and every helper to 1 (soft and hard). It no
  longer calls `PR_SET_DUMPABLE`.
- The guard runs first in `main()`.
- The harness raises the runner's limit only while it starts the two core-dump scenarios.
  `core-dumps-off` now expects 1/1 in all eight processes. On the M1 binary it still fails
  (every process unlimited).

**Crash check** (once, allowed by the maintainer; `SIGABRT`, nested niri, the marker page).
Aborting foob, and then the web process, left:
- no coredump entry (`coredumpctl`: "No coredumps found");
- no mention of the marker in the journal;
- only one kernel line per crash, with the PID and process name:
  `coredump: 27352(foob): RLIMIT_CORE is set to 1, aborting core`.

This is stronger than the first version, where a crashed helper still left a metadata
entry (rows 1 and 1b above). Portals and gdb (from foob's parent) keep working.

**Accepted.** Other programs of the same user can again read the UI process's `environ`,
`maps` and open-file links. Its memory and ptrace stay blocked by Yama
(`ptrace_scope=1`), and the page data lives in the helpers, which are dumpable after exec
in either design.

**Checks on this commit.**
- Unit tests 3/3; nested suite 11/11.
- ASan+UBSan: 3/3 and 11/11, with no report.
- `gcc -fanalyzer`: clean.

**Size.**
- Stripped: 22,520 B, unchanged.
- `.text`: 16,216 → 16,184 B (−32 B).
- `main.c.o`: 728 → 733 B.

**Later-task risks** (from the security review; recorded in the roadmap's M2-03 and
M2-04):
- A private child started by a diagnostics parent would inherit `FOOB_DIAGNOSTICS=1`, and
  with it, core dumps.
- An address in argv is readable by every local process through `/proc/PID/cmdline`.

#### Fallback where a limit of 1 cannot work (D-42)

The second independent reviews found two gaps in the limit-of-1 design.

- **Code review: foob refused to start.** `setrlimit(RLIMIT_CORE, {1,1})` fails with
  EPERM when the hard limit is already 0, so foob refused to start, with a false message.
  `meson test` failed under bash's `ulimit -c 0`.
- **Security review: socket collectors are not covered.** The kernel's abort at a limit
  of 1 covers only pipe and file patterns. A socket collector would still receive the
  dump and journal foob's command line. Such a collector is `@` or `@@` in `core_pattern`;
  `systemd-coredumpd` is installed here and disabled.

**What changed.** In both cases foob sets the core limit to 0 and makes itself
non-dumpable, so no handler runs for it; an unreadable `core_pattern` counts as a socket.
Portals then refuse foob, but only in those setups.

**Checks.**
- **`check_cli.py`** now runs every case twice: as is, and under `prlimit --core=0:0`.
  For accepted inputs it also requires the display message, so a refused guard can no
  longer pass as "reached the display". On `ff1ded2`'s `main.c` it fails as it should:
  `foob: could not turn off core dumps` under the hard limit of 0.
- **New nested scenario `core-dumps-off-hard-zero`** (under `prlimit --core=0:0`): every
  process shows 0/0 and foob is non-dumpable. On the `ff1ded2` binary it fails (foob does
  not start).
- **Not testable here:** the socket branch, because it needs a change to the system's
  `core_pattern`. It shares the fallback code with the hard-zero case. Only the pattern
  read is untested.
- **Sanitizers and analyzer:** unit tests 3/3, the three core scenarios pass under
  ASan+UBSan with no report, `gcc -fanalyzer` is clean.

**Size.** The stripped binary is still 22,520 B. `.text` grows from 16,184 to 16,494 B
(+310 B), and `main.c.o` from 733 to 929 B, from the `core_pattern` read and the fallback
branch.

### M2-02: load the canonical address

`main.c` used to load the argument text that GLib had checked, and WebKit then parsed it
again with its own parser. Now `foob_target_canonical()` (`src/target.c`) returns the
form foob loads. That form is ASCII, and WebKit's parser leaves it unchanged, so what was
checked is what WebKit loads. `foob_uri_is_local()` and `foob_navigation_allowed()` use
the same function.

**Probe** (a scratch program, without a display, on `wpewebkit 2.54.1` and GLib 2.90.1;
62 addresses).

- **Mismatches today:**
  - `http://%20/` and `https://host%2540/` were accepted. WebKit leaves them as written,
    with `%20` as the host.
  - `1.2.3`, `0x7f.0.0.1`, `1.2.3.04` and `2130706433` are names to GLib. WebKit reads
    them as IPv4 addresses: `1.2.0.3`, `127.0.0.1`, `1.2.3.4`, `127.0.0.1`.
  - `[0:0::1]` becomes `[::1]`.
  - Raw `"<>`{}^` in the path, and `'` in the query, get encoded by WebKit.
  - `file:///tmp/%E9` was rejected, because GLib requires UTF-8 after decoding.
- **Already correct:** `file:///x.html#sec` was accepted and local. The plan had
  suspected otherwise.
- **What GLib already does like WebKit,** when it keeps escapes (`G_URI_FLAGS_ENCODED |
  G_URI_FLAGS_NON_DNS`):
  - percent-encode each raw non-ASCII byte;
  - decode escaped unreserved characters and upper-case the other escapes;
  - resolve dot segments, `%2e` included.

  foob adds the rest: it lower-cases the host, checks it, drops default ports, and encodes
  the characters WebKit would encode.
- **WebKit's request API reads its argument as Latin-1:** `webkit_uri_request_new()`
  double-encodes raw UTF-8. That is harmless here, because canonical forms are ASCII.
  `webkit_web_view_load_uri()` reads UTF-8 correctly: a nested run on the M1 binary loaded
  a raw `café` path correctly, so that run was not a negative control and was dropped.
- **Non-UTF-8 file names:** WebKit cannot open a file whose name is not valid UTF-8
  (`file:///…/%E9.html` fails with GIO error 15, "not supported"). Such names stay
  rejected (D-40).

**Checks.**
- **`tests/test_target.c`:**
  - 23 accepted pairs of input and canonical form; each canonical form is also stable
    when canonicalized again.
  - 45 rejected cases, local-trust cases and navigation cases.
  - `/target/webkit-agrees`: `webkit_uri_request_new()` returns each canonical form
    unchanged.
- **Mutation checks** (in scratch copies):
  - Keeping `'` raw in a query, with my expected string changed to match: only
    `webkit-agrees` fails.
  - Turning off the numeric-host check: the rejected cases fail.
- **`check_cli.py`:** `http://%20/` is rejected. The M1 binary accepts it, which is the
  negative control.
- **Nested suite 11/11.** The new `title-local-escaped-name` scenario loads
  `café #1?.html` through its `file:` address and keeps the title's ZWSP, so the page
  counts as local. It also passes on the M1 binary: it guards the rewritten
  `foob_uri_is_local()`, and in commit 3 it becomes the path-argument test.
- **ASan+UBSan:** 3/3 and 11/11, with no report.
- **`gcc -fanalyzer`:** clean.
- **Two unplanned core files.** The two mutation runs ended in deliberate assertion
  aborts, and each stored a core of the test program (1.1 MB, test strings only) under
  `/var/lib/systemd/coredump`. Deliberately failing runs now use `ulimit -c 0`.

**Accepted limits.**
- An IPv6 address must be written in its shortest form.
- An international host must be given in punycode (D-39).
- A file name must be UTF-8 (D-40).
- An `xn--` label that is not valid punycode is left to WebKit.

**Size and time (D-35).** The stripped binary is still 22,520 B: the growth fits in the
existing page padding. `.text` grows from 13,799 to 16,216 B (+2,417 B, +17.5%).
`target.c.o` grows from 1,143 to 2,782 B and `main.c.o` from 705 to 728 B. That is the
host, port and component rules; the compiler inlines all the helpers into
`foob_target_canonical()`. One `foob_uri_is_local()` call, which runs on each title
change, takes about 0.4 µs instead of about 0.2 µs (200,000 calls each on three typical
addresses, `-O2`). That is negligible next to the title handling itself (about 0.3 ms,
audit-m1.md).
