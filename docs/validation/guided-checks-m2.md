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

*Superseded:* this subsection describes the first design (commit `b372cc9`). The current
guard is a core limit of 1 ([Rework, D-41](#rework-after-the-independent-reviews-d-41)),
with limit 0 and a non-dumpable foob only where 1 cannot work
([Fallback, D-42](#fallback-where-a-limit-of-1-cannot-work-d-42)).

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
- *Corrected after the third reviews:* this record first called the socket branch
  untestable here. It is tested since the follow-up below.
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
  - resolve dot segments, `%2e` included.

  It also decodes escaped unreserved characters and upper-cases the other escapes, which
  WebKit does not. *Corrected after the reviews (D-43); this record first said WebKit
  does this too.* foob adds the rest: it lower-cases the host, checks it, drops default
  ports, and encodes the characters WebKit would encode.
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

#### Corrections after the second independent reviews (D-43)

**Findings.**
- **Code review:** foob encoded `{`, `}`, `|`, `^` and `` ` `` in queries and fragments,
  which WebKit keeps. GLib's escape normalization was described as WebKit's.
- **Security review:** a fuzzer found `file:/.//etc/passwd`, whose canonical form WebKit
  rewrites once more. It is the same file, with the same trust.
- **Both fuzzers** (about a million inputs each) found no scheme, host or trust mismatch.

**What changed.**
- **Queries and fragments** now keep what WebKit keeps (query: `` ` ``, `^`, `{`, `|`,
  `}`; fragment: `^`, `{`, `|`, `}`). A path still encodes `|`, because WebKit reads
  `file:///a|` as the drive letter `file:///a:`.
- **A `file:` path that resolves to `//…` is rejected.** GLib writes it `/.//…`, which
  covers `file:/.//tmp/a.html`, `file:////tmp/a.html` and `FILE:/tmp/..//a`.
- **GLib's escape normalization is accepted, and the comment says so.** M2-03 rechecks it
  for links that D-23 passes on.

**Checks.**
- **New unit pair:** `a|b?q={x}|y^z`w#{x}|y^z`w` gives `a%7Cb?q={x}|y^z`w#{x}|y^z%60w`.
  Three new rejected cases.
- **New nested scenario `canonical-address-loaded`:** `/x|y.html` reaches the loopback
  server as `/x%7Cy.html`, so foob loads its canonical form, not the argument. The M1
  binary sends `/x|y.html` and fails.
  - A first try with `%7e` passed on M1 too, because WebKit's network layer also
    normalizes `%7e`, so it was replaced.
- **Results:** unit tests 3/3; nested 13/13; ASan+UBSan 3/3 and 13/13, with no report;
  `gcc -fanalyzer` clean.

**Size.** The stripped binary is still 22,520 B. `.text` shrinks from 16,494 to 16,462 B
(−32 B), and `target.c.o` from 2,782 to 2,750 B.

#### Follow-up after the third reviews and a Codex review

**Outcomes.**
- Third reviews (code and security): `f63bfd9` and `a6c7188` both pass on correctness.
- The security fuzzer checked 2.3 M inputs under sanitizers with 0 mismatches. The
  commit before had 34.
- The code review confirmed that the encoding matches WebKit per character and per
  component, with `|` in a path the decided exception.

**Follow-ups the maintainer chose.**
- **Socket-collector test** (new nested scenario `core-dumps-off-socket-collector`):
  - `unshare -rm` gives foob an unprivileged user and mount namespace. In it, a fake
    `core_pattern` (`@@/run/systemd/coredump.socket`) is bind-mounted, with no root and
    no system change.
  - Every process then shows 0/0. On the `ff1ded2` binary every process shows 1/1, so
    the scenario fails there.
  - Inside the namespace, `/proc` ownership cannot show dumpability; the hard-zero
    scenario covers that.
- **Residual risks recorded in D-42, the README and AGENTS.md.** Codex and both
  reviewers raised them:
  - In the fallback, the WebKit helpers become dumpable again at exec. With a pipe
    collector a crashed helper leaves metadata without the address; the security
    reviewer checked this with a marker page. With a socket collector, the helper's
    memory reaches the collector, which must honour the limit of 0
    (`systemd-coredumpd` does).
  - `core_pattern` is read once at start.
- **A test comment corrected:** `test_target.c` no longer calls GLib's escape
  normalization WebKit's.

**Not taken:** a per-character encoding regression test (the maintainer's choice).

**Disclosure.** The security reviewer's crash checks ran under a hard core limit of 0. No
core was stored; the helpers left metadata-only journal entries.

**Checks.** Nested suite 14/14. Unit tests 3/3. Only test files changed.

### M2-02: path argument and `--version`

**What changed.**
- **Path argument (D-32).** A command-line argument that starts with `/`, `./` or `../`
  is a path. `foob_target_from_argument()` makes it absolute with
  `g_canonicalize_filename()`, which does not resolve symbolic links, turns it into a
  `file:` URI with `g_filename_to_uri()`, and passes it through
  `foob_target_canonical()`. Anything else must still be a URI, so `foob example.com`
  and `foob notes.html` stay rejected. Only `main.c` calls it: an address from a page
  is never a path.
- **`--version`** prints `foob 0.2.0-dev` and `WPE WebKit 2.54.1` (the loaded library's
  `webkit_get_*_version()`) and exits 0, before any display. The version is set once in
  `meson.build` (D-39).
- The usage line lists both.

**Checks.**
- **Unit test `/target/from-argument`:**
  - `/tmp/a b#?.html` gives `file:///tmp/a%20b%23%3F.html`.
  - Dot segments are resolved, and a UTF-8 name is encoded.
  - `./x.html` and `../x.html` resolve against the current directory.
  - A URI argument becomes its canonical form (`https://example.org` gives
    `https://example.org/`).
  - A name that is not UTF-8, `example.com`, `x.html`, `~/x.html` and an empty argument
    are rejected.
- **`check_cli.py`:**
  - `--version` with an unreachable display: exit 0, nothing on stderr, and stdout
    `foob <version>`, then `WPE WebKit <x.y.z>`.
  - `--version extra`, `example.com` and a non-UTF-8 path give exit 2.
  - A relative and an absolute path reach the display check.
  - On the M1 binary, `--version` and both paths exit 2.
- **New nested scenario `path-argument-local`** replaces `title-local-escaped-name`.
  `./café #1?.html`, started from the scratch directory, loads, and its title keeps the
  ZWSP, so WebKit's committed `file:` address counts as local. On the M1 binary it fails,
  because the argument is rejected.
- **Results:** unit tests 3/3; nested 14/14; ASan+UBSan 3/3 and 14/14, with no report;
  `gcc -fanalyzer` clean.

**Size (D-35).**
- This commit: the stripped binary is still 22,520 B. `.text` grows from 16,462 to
  17,443 B (+981 B). `main.c.o` grows from 929 to 1,105 B (the version output and the
  longer usage text), and `target.c.o` from 2,750 to 3,029 B (the path conversion). The
  rest is new imports and read-only strings.
- **Step 1 in total:** `.text` grows from 13,473 to 17,443 B (+3,970 B, +29.5%), with the
  stripped size unchanged. The next growth will probably cross the page padding.

| Object | `.text` M1 (B) | `.text` after step 1 (B) |
|---|---|---|
| `src_browser.c.o` | 2,253 | 2,253 |
| `src_diagnostics.c.o` | 2,165 | 2,165 |
| `src_main.c.o` | 499 | 1,105 |
| `src_target.c.o` | 1,143 | 3,029 |
| `src_window_title.c.o` | 1,498 | 1,498 |

#### Follow-up after the reviews of commit 3 and `132cc76`

All three reviews passed (code and security of `68905d1`; a quick code review of
`132cc76`). The security fuzzer checked 900,000 path arguments under sanitizers with 0
violations. Follow-ups, chosen by the maintainer:

- **Relative paths now use the kernel's current directory (D-44).** Commit 3 used
  `g_get_current_dir()`, which prefers `$PWD` and falls back to `/`.
  - The reviews reproduced three surprises: a symlinked directory resolved `..` to the
    link's parent, not the real one; a deleted directory made `./x.html` into `/x.html`;
    and `cd //tmp` made every relative path fail.
  - Now `getcwd()` is the base, and a missing directory rejects the argument.
  - **New unit test `/target/from-argument-cwd`:**
    - `cd` into a symlinked directory with `$PWD` set as a shell sets it: `../x.html`
      names the real parent, as `realpath ..` does.
    - The same directory after it is deleted: `./x.html` is rejected.
    - On commit 3's `target.c` it fails: `file:///tmp/foob-target-…/x.html` instead of
      `…/real/x.html`.
- **Two regression cases in `/target/from-argument`:** `//etc/passwd` is rejected, and
  `/tmp/%2e%2e/x` gives `file:///tmp/%252e%252e/x` (a literal `%` in a name).
- **Small fixes:**
  - The usage line lists `../path`.
  - The socket-collector scenario quotes its scratch path in the shell command, and its
    comment says it is the kernel that does not abort.
  - AGENTS.md's guard sentence is reworded.
  - The roadmap's core-dump line names the D-42 fallback.
  - M2-03 gets the D-23 acceptance constraints from the security review: the parent
    passes only canonical http(s), the child checks a URI only, `--` comes before the
    address, and `--version` is valid only alone.
- **Wording in this record.**
  - The `.text` figures in step 1 are `size`'s text column, which also counts read-only
    data and symbol tables. The `.text` section alone grew from 7,206 to 7,526 B in
    commit 3.
  - "A URI argument passes through unchanged" was corrected above.
- **Size (D-35).**
  - The stripped binary grows from 22,520 to 26,616 B. That is one page (4,096 B): the
    content crossed the padding of the last page, as predicted after commit 3.
  - The content itself grew far less: `size` text column 17,443 → 17,670 B (+227 B), and
    the `.text` section 7,526 → 7,590 B (+64 B), from `getcwd()` and the longer usage
    text.
  - **Step 1 in total:** stripped 22,520 → 26,616 B (+18.2%, one page);
    `size` text column 13,473 → 17,670 B (+31%). The growth comes from the canonical
    form's rules (`target.c.o`), the core-dump guard with its fallback, and `--version`
    (`main.c.o`).
- **Not changed:** `--version` still exits 0 when stdout cannot be written. It is a
  cosmetic issue, and `exit_status.h` has no code for it.

## F-78: every permission request denied

Found by the branch security review (`security-review-m2-step1.md`). foob did not handle
WebKit's `permission-request`, and unanswered WebKit *allows* pointer lock. Its API
documentation says the other request types, notifications among them, are denied; the
review read installing missing media plugins as allowed too. `browser.c` now denies every
request and, with `FOOB_DIAGNOSTICS=1`, logs `permission-denied type=<request type>`
(the type only, never an address). M3-05 decides which permission a page may get.

**Why a guided row.** In a nested probe of both binaries, a page that calls
`requestPointerLock()` without a click gets an exception, and geolocation is refused
before foob is asked. No request reaches foob without a real click, so a nested
scenario could not fail on the old binary, and none was added.

**Guided row** (maintainer at the keyboard, real clicks):

```sh
FOOB_DIAGNOSTICS=1 ./build/foob ./tests/fixtures/permissions.html
```

| # | Click on the page | Expected (read the page's log panel and the terminal) | Result |
|---|---|---|---|
| 1 | "Lock the pointer" | the pointer stays free; log: `pointer lock refused`; terminal: `permission-denied type=WebKitPointerLockPermissionRequest`. **Negative control:** the M1 binary (`main`) locks the pointer and the log says `pointer locked` (`Escape` releases it) | pending |
| 2 | "Ask to show notifications" | log: `notifications: denied` | pending |

**Checks on this change.**
- Unit tests 3/3; nested suite 14/14.
- ASan+UBSan: 3/3 and 14/14, with no report.
- `gcc -fanalyzer`: clean.

**Size (D-35).**
- `size` text column: 17,670 → 17,943 B (+273 B). `browser.c.o` grows from 2,253 to
  2,450 B: the handler, the diagnostics line and the signal connection.
- The stripped binary grows from 26,616 to 30,712 B. That is one page: the file pads each
  segment to a page boundary, and another segment just crossed one.
- The M2 reduction pass (D-35) should check whether the build options can pack the
  segments tighter.

