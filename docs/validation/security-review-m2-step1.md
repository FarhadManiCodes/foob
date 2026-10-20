# Security review report (DeepSeek, 2026-10-20)

Purpose: a complete, reproducible security check of this repository for a later agent or
the maintainer. It records what was checked, what passed, what remains, and what not to
redo. It is evidence, not a decision: `AGENTS.md`, `docs/validation/handoff.md` and
`docs/validation/findings.md` remain the source of truth. No code was changed for this
report.

This file deliberately omits PIDs, home paths, hostnames and account names so it can be
committed if the maintainer wants to keep it. Re-run the commands locally for exact
figures. Some UTC timestamps in the evidence are 2026-10-19 because the runs happened
around local midnight.

Contents: TL;DR; 1 environment; 2 scope; 3 verdict; 4 detailed verification (4.1–4.13);
5 known/carried items and merge-review follow-ups; 6 recommendations and recording;
7 reproduction commands; 8 bottom line.

---

## TL;DR for another agent

- **No high- or critical-severity vulnerability was found in foob's own code.**
- The startup allowlist and local/remote trust decision survived ~1.1 million generated
  inputs this session (300 k random + ~785 k structured), plus the project's earlier ~2 M
  record, with no crash, no trust flip and no WebKit rewrite.
- The C code is clean under ASan+UBSan, GCC `-fanalyzer`, clang `scan-build` and a 40 k
  random-title fuzz. ELF hardening is present (PIE, RELRO, BIND_NOW, NX stack, stack
  protector, FORTIFY=3, CET, no RPATH, no setuid).
- Security behavior tests pass: download cancellation (five different triggers, no file),
  new-window suppression, remote→`file:` redirect blocked, core-dump limits in four
  nested scenarios, diagnostics contain no URL/host/title, page console output is not
  printed.
- Residual risk is **engine + deliberate M1 design**, not a foob memory-safety bug:
  - the stock **network process is not sandboxed** (upstream WebKit design),
  - the **web-process sandbox** has the whole `~/.cache` rw and exposes `/dev/dri`,
    `/dev/v4l`+`/dev/video*`, `/dev/snd` and the Pulse socket (upstream),
  - `permission-request` is unhandled, and WebKit's documented default **allows pointer
    lock** (and missing-media-plugins) silently; everything else is denied by default,
  - no visible origin/TLS indicator (design), known S-01…S-04, D-08, F-77 residuals and
    D-23 child argv/env issues remain.
- **For the intended trusted-pages use: low risk. For arbitrary untrusted browsing:
  medium residual until M2-03, M3-05 and M4.** This is not a formal WebKit audit.

---

## 1. Environment and revision

| Item | Value |
|---|---|
| Branch / HEAD | `m2`, `3e08a5a` (`M2-02: relative paths against the kernel's current directory (D-44); review follow-ups for step 1`) |
| `main` at review time | `134ec65` (Gate A approval) |
| Working tree | only `docs/validation/handoff.md` modified before this file; no source changes |
| Engine | `wpewebkit 2.54.1-2` (Arch, built 2026-10-06), WPEPlatform 2.54.1, GLib 2.90.1 |
| Compiler / build | GCC 16.2.1, Meson 1.12.1, Ninja 1.13.2, C23, `debugoptimized` (`-O2 -g`) |
| Kernel / session | Arch Linux, kernel 7.2.x, niri/Wayland session; `niri`, `prlimit`, `unshare` available |
| Build dir | `build/` (existing); fresh builds were made under a scratch directory outside the repo |
| Tracked files | 68; reachable commits 174 |

The binary used for runtime checks was `build/foob` from `3e08a5a`, SHA256
`e7a3bafc09eb86e85bc9bd5a1ddf0de5363017296b5a6402a1cba5eabefb517c` (recorded by
`tests/benchmark/run.py`). The source tree was dirty only because of the handoff edit,
which does not affect the binary.

---

## 2. Scope and limits

In scope:

- all of `src/`, `meson.build`, `tests/` (unit, CLI, nested lifecycle, benchmark scripts,
  fixtures), tracked docs, `.gitignore`, repository history and local Git state;
- the browser's trust boundaries, policy defaults, process sandbox as observed at
  runtime, build hardening, dependency/engine version, and repo hygiene.

Out of scope / not done:

- a source audit of WebKit/WPEPlatform (millions of lines). Engine CVEs are handled by
  upstream and the distribution.
- real mouse/keyboard input (never synthesized, per `AGENTS.md`). Pointer lock and other
  click-gated behavior were not runtime-tested; their defaults are taken from the
  installed API documentation and the M1 probes.
- a new deliberate crash for the F-77 journal/core checks. The project's recorded
  one-time crash evidence and the repeatable `/proc` limit tests are used instead.
- camera/microphone permission with a real device (`getUserMedia` has no device here;
  M3-05 owns it).
- a full TLS/PKI test; the M1 audit observed an expired certificate refused, and no code
  overrides the default policy.
- TLS/proxy/DNS behavior outside WebKit's defaults.

---

## 3. Verdict

**No exploitable memory-safety or trust-boundary defect was found in foob's code.** The
application is small and its design is conservative: one view, ephemeral session, no
application-initiated load, no shell/exec, downloads cancelled, new windows ignored,
diagnostics redacted, core dumps off by default.

The residual risk is dominated by the engine and by choices owned by later tasks:

1. upstream WebKit's network process is not sandboxed;
2. the upstream web-process sandbox exposes cache and media devices beyond the minimum;
3. engine-default permissions silently allow pointer lock;
4. no visible origin/TLS indicator;
5. known S-01…S-04, D-08, F-77 and D-23 items.

For the M1 "trusted pages" use, the residual risk is **low**. For arbitrary untrusted web
content, treat it as **medium** until M2-03 (navigation/scheme policy), M3-05
(permissions/TLS/sandbox audit) and M4 (local-file trust) are done.

---

## 4. What was verified, with results

### 4.1 Build and automated tests

Commands (from the repo root; use a fresh build directory to avoid touching the existing
one):

```sh
meson setup /tmp/foob-security-build . --wipe
meson compile -C /tmp/foob-security-build
meson test -C /tmp/foob-security-build --print-errorlogs
```

Results:

- configure/build clean, no compiler warnings at the project's warning level;
- `meson test` **3/3**: `window_title`, `target`, `startup URI contract`;
- a separate ASan+UBSan build (`-Db_sanitize=address,undefined -Db_lundef=false`) also
  passes **3/3** with no sanitizer report.

### 4.2 Static analysis

- GCC `-fanalyzer` (added through `-Dc_args=-fanalyzer`, built in a scratch build dir):
  **no warnings**.
- Clang `scan-build --status-bugs` on a clean build: **no bugs found**.
- `clang-tidy -p build --checks='-*,clang-analyzer-*'` on all `src/*.c`: only
  `clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling` notes from GLib
  headers (`memcpy`/`memmove`), none in foob code.
- Manual source review found no dangerous APIs in production code: no `system`,
  `popen`, `exec*`, `fork`, `posix_spawn`, `g_spawn`, `g_shell`, `strcpy`, `sprintf`,
  `mktemp`, or shell use. The only child processes are WebKit's own.

### 4.3 URL allowlist and trust boundary

Files: `src/target.c`, `src/target.h`, `tests/test_target.c`, `tests/check_cli.py`.

Invariants checked for every accepted input:

1. the canonical form is ASCII;
2. `foob_target_canonical(canonical)` is idempotent;
3. `webkit_uri_request_new(canonical)` returns the canonical form unchanged
   (WPE WebKit 2.54.1);
4. `foob_uri_is_local(input)` agrees with the canonical form's local/remote verdict;
5. a `file:` canonical form decodes to valid UTF-8 and has no `%2F`/`%00`;
6. `foob_target_from_argument()` only turns command-line path forms into `file:` URIs;
   an address from a page is never treated as a path.

Inputs:

- **300,000 random byte strings** (length 0–89) from a grammar mixing prefixes
  (`http://`, `https://`, `file:`, `about:`, `//`, `/`, `./`, `../`), punctuation and
  control bytes, under ASan+UBSan: **2,746 accepted, 0 crashes, 0 invariant failures**.
- **~785,000 structured inputs** this session (file paths built from `.`/`..`/`%2e`/
  `%2f`/`%00`/`%25`/`%5c` segments; host labels with IPv4/hex/percent/UTF-8/empty-label
  cases; IPv6 forms; each path/query/fragment special character): **353,390 accepted, 0
  mismatches**.
- The project's earlier record (handoff, second security review) reports about 2 M fuzz
  inputs with the same result; this review did not repeat that volume.

Specific checks that pass:

- `http://%20/`, `https://host%2540/`, Unicode hosts, `user:pass@`, backslashes, raw
  controls, `about:blank#x`, `data:`/`javascript:`/`blob:`/`ftp:`/`ws:` are rejected;
- `1.2.3`, `0x7f.0.0.1`, `1.2.3.04`, `2130706433`, `1.2.3.4.` are rejected because WebKit
  would rewrite them as a different IPv4 form;
- `file://remote/tmp`, `file://127.0.0.1/tmp`, `file://localhost:80/tmp`, `%2F`, `%00`,
  non-UTF-8 names, `//…` and `/.//…` paths are rejected;
- `file://localhost/tmp` becomes `file:///tmp`; default ports are dropped; an empty path
  becomes `/`; empty query/fragment are preserved as WebKit preserves them;
- `foob_target_from_argument()` accepts `/…`, `./…`, `../…`; a deleted cwd rejects the
  relative form; `..` is removed as text (D-44); `//etc/passwd` is rejected;
- `foob_navigation_allowed()` allows local→remote, remote→remote, local→local, and
  blocks remote→local; it is currently unused, as documented.

Known acceptable directions:

- the canonical form may differ from the input where GLib decodes unreserved escapes or
  resolves dot segments (`%7e` → `~`, `%41` → `A`, `%2e%2e`); D-43 accepts this as an
  equivalent address;
- several forms are over-rejected rather than accepted (IPv4-mapped IPv6 such as
  `[::ffff:1.2.3.4]`, equal-length zero-run IPv6 forms, Unicode/IDN hosts, non-canonical
  IPv4). This is the safe direction;
- `foob_target_canonical()` accepts no remote form that `foob_uri_is_local()` would call
  local, and no local form that would be treated as remote, on all generated inputs.

Test-quality note (not a vulnerability): `tests/test_target.c:74-81` checks that
`webkit_uri_request_new()` returns the canonical form unchanged. Although
`webkit_uri_request_new` returns an unparseable URL unchanged (`http://[::1` or
`http://a b/`), every entry in `accepted[]` has already passed `foob_target_canonical()`
(which rejects spaces/unsafe characters, validates IPv6 via `inet_pton`/`inet_ntop`,
validates host labels, and parses/joins via `GUri`), and the nested
`canonical-address-loaded` test loads the HTTP path for real, so the fixed-point check in
`test_webkit_agrees` is sound as written.

### 4.4 Title sanitizer

Files: `src/window_title.c`, `tests/test_window_title.c`.

- Unit tests 3/3 under the normal build and under ASan+UBSan; the existing tests cover
  C0/C1 controls, bidi controls, zero-width/default-ignorable characters, joiners,
  variation selectors, RTL detection/prefix, long titles, 16 KiB cut, invalid UTF-8 and
  character-boundary cuts.
- **40,000 random byte strings up to 20 KB**, both trust levels, under ASan+UBSan:
  0 sanitizer reports, 0 invalid UTF-8 outputs, 0 over-4096-byte outputs. (An initial
  run reported 72 "failures" only because the harness expected the ` — foob` suffix on
  the legitimate empty-title result `foob`; correcting the expectation gave 0.)
- The title is page-controlled and is only sent to the compositor, never printed to the
  terminal. Remote titles strip bidi overrides and invisible format characters; local
  titles are deliberately less restricted (D-08).
- Known limits: homoglyphs and RTL text are not a security boundary; a page can still end
  its title with `— foob`; a combining/ZWJ sequence can be split by the 120/240-character
  cut. Documented in the roadmap.
- Known trust issue S-03/S-04: trust is read from `webkit_web_view_get_uri()` at title
  change, which can be the pending address; a local page moving to a remote page with the
  same title (S-03) or a remote page pending `about:blank` (S-04) could keep an
  unsanitized title. Low impact (visual/title only; the title is not an address bar),
  owned by M2-03.

### 4.5 Diagnostics and terminal safety

Files: `src/diagnostics.c`, `src/browser.c`.

- Live output (`FOOB_DIAGNOSTICS=1`) contains only fixed strings and numbers:
  `load-started`, `load-finished status=… elapsed_ms=…`, `load-failed phase=… domain=…
  code=…`, `download-cancelled`, `web-process-terminated reason=…`,
  `display-disconnected`, `shutdown signal`, `closed`, `backend=… session=ephemeral`,
  and geometry/state numbers.
- `load_failed()` ignores its `uri` parameter (`src/diagnostics.c:94-103`); no URL, host,
  path, query, title or error text is logged.
- A local page with `console.log("MARKER-CONSOLE")`, `console.error("MARKER-ERROR")` and
  an uncaught `throw new Error("MARKER-THROW")` produced **no markers** on stdout or
  stderr; WebKit's console-to-stdout setting is not enabled.
- Page-controlled terminal escapes are therefore not printed by default. WebKit engine
  warnings still appear on stderr (on this machine, a fixed accessibility-bus warning);
  the README documents that engine warnings can appear. They contained no page data in
  the checks.
- The page title is set on the compositor toplevel, not printed; a same-user process can
  read it via the compositor IPC (e.g. `niri msg windows`), which is a same-user privacy
  consideration, not a remote boundary.
- The on-demand dump (D-26/M2-06) is not built. When built it must be `0600`, in the
  profile's `0700` directory, and hold hosts only, never paths, queries, fragments or
  private-mode data.
- `FOOB_DIAGNOSTICS=1` intentionally keeps core dumps on (D-26). A crash in diagnostics
  mode can write page/UI memory to disk; this is intended for debugging.

### 4.6 Downloads

File: `src/browser.c:60-65` (`download-started` → `webkit_download_cancel`).

- The nested `download-cancelled` scenario passes: a 300 MiB attachment is aborted, no
  file and no payload in a redirected download/temp/cache tree, no survivor.
- Additional manual loopback test with **five trigger types** on a remote page:
  1. `<a download>` clicked by script,
  2. `data:` URL with `download`,
  3. `blob:` URL from `fetch()` with `download`,
  4. top-level navigation to an attachment response,
  5. iframe loading an attachment.

  Result: five `download-cancelled` lines, **zero payload files**. Only engine caches
  (Mesa shader cache, GStreamer registry) appeared under the redirected
  `XDG_CACHE_HOME`; those are not download files and are documented as a private-mode
  limitation (D-19).
- PDF.js saves use the same download path and are therefore cancelled by the same
  handler; a real click on the PDF save button remains a guided check (M3-04).
- The D-02 interim rule ("no download may start and no file be created without an
  explicit user decision") holds in the automated checks.

### 4.7 New windows and navigation

File: `src/browser.c:67-85`.

- Every `WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION` is ignored; other decisions
  return `FALSE` (WebKit default).
- `window.open()` is unhandled through the `create` signal, so it returns `null`;
  `target=_blank` links/forms are ignored. The nested
  `blank-form-does-not-navigate` scenario passes: no `/echo` request, one load, title
  unchanged.
- No application-initiated load exists: `grep` shows only
  `webkit_web_view_load_uri(browser->web_view, target)` in `browser.c:184`, with the
  canonical startup target. `foob_navigation_allowed()` is still uncalled.
- Manual HTTP `302 Location: file:///…/local.html` test: the redirect is **blocked**
  with `WebKitPolicyError` code 103; the window title stays `foob`, the marker local
  content never appears; the second `load-started` is WebKit's error page. This supports
  D-04's reliance on WebKit's remote-to-local check.
- The `about:blank` origin caveat remains: `foob_uri_is_local()` treats `about:blank` as
  local, and a remote-initiated `about:blank` document can inherit a remote origin. No
  code loads on a page's behalf today, but a future `foob_navigation_allowed()` caller
  must use the committed document origin, not only
  `webkit_web_view_get_uri()` (S-04/M2-03). Test remote → `about:blank` → `file:` before
  adding such a caller.
- `target=_blank` POST forms are not replayed; unsupported/unsafe submissions are M2-03.

### 4.8 Core-dump guard (F-77)

Files: `src/main.c:12-47`, `tests/nested_lifecycle.py`.

- Default launch (`FOOB_DIAGNOSTICS` unset): core limit **1/1** (soft/hard) for foob and
  every process in the tree; the kernel aborts a dump for pipe/file `core_pattern`
  before the collector runs. This machine's `core_pattern` is the systemd pipe.
- `FOOB_DIAGNOSTICS=1`: no guard; the runner's core limit is inherited (the nested test
  checks this).
- Hard limit already 0 (`prlimit --core=0:0`): the 1/1 `setrlimit` fails, foob sets 0/0
  and `PR_SET_DUMPABLE=0`, so no handler runs for foob; portals later refuse a
  non-dumpable foob (accepted in D-42).
- Fake socket `core_pattern` (`@@…`, bind-mounted in an unprivileged user/mount
  namespace): foob takes the 0/0 + non-dumpable fallback; test passes.
- Nested scenarios pass: `core-dumps-off`, `core-dumps-diagnostics`,
  `core-dumps-off-hard-zero`, `core-dumps-off-socket-collector`; no helper survives.
- The guard runs before the argument is parsed and before `--version`, so a crash during
  startup cannot journal a page-supplied argv. A crash before `main` (dynamic loader or a
  pre-main constructor) is outside the guard, but is not page-triggered.
- Residual risks already recorded in D-42 and not reopened here:
  - in the fallback, WebKit helpers become dumpable again at exec; with a pipe collector
    a helper-crash leaves metadata-only journal lines (no address), and with a socket
    collector the collector must honour the limit of 0;
  - `core_pattern` is read once at start; enabling a socket collector while foob runs
    would meet foob's already-set limit 1;
  - a diagnostics run intentionally permits cores, and a future private child must not
    inherit `FOOB_DIAGNOSTICS=1` (M2-03/M2-04);
  - the URL is still visible in `/proc/<pid>/cmdline` to same-user processes, and in
    `/proc/<pid>/environ` for the UI/network processes; hiding it is a D-23 design
    question, not part of F-77.

The nested checks read `/proc/<pid>/limits` and `/proc/<pid>/limits` ownership, not the
kernel's actual dump decision; the one-time real crash checks (no core with the guard,
one core with diagnostics) are recorded in `docs/validation/guided-checks-m2.md`. This
review did not repeat a crash.

### 4.9 Runtime sandbox (new detail for M3-05)

Method: launch foob on a local page, inspect `/proc/<pid>/status`, `/proc/<pid>/ns/*`
and `/proc/<pid>/mountinfo` for each process; `tests/benchmark/run.py --lifecycle-only`
also records namespaces, seccomp and `NoNewPrivs` per process.

Observed tree (names as `/proc` truncates them):

| Process | NoNewPrivs | Seccomp | Namespaces | Notes |
|---|---:|---:|---|---|
| `foob` (UI) | 0 | 0 | UI mnt/pid/user/net | unsandboxed UI, normal |
| `WPENetworkProce` | 0 | 0 | same as UI | **not sandboxed** |
| `bwrap` parents | 1 | 0 | UI namespaces | setup helpers |
| `bwrap` children | 1 | 2 | new mnt/pid/user/net | sandbox |
| `xdg-dbus-proxy` | 1 | 2 | inside web sandbox | filtered bus |
| `WPEWebProcess` | 1 | 2 | own mnt/pid/user/net | web content sandbox |

This matches WebKit's own design. In `BubblewrapLauncher.cpp` the network process
returns before sandbox setup: "For now we are just considering the network process
trusted as it requires a lot of access but doesn't execute arbitrary code like the
WebProcess where our focus lies." The web process gets `--unshare-net`, `--unshare-pid`,
a seccomp filter and `NoNewPrivs`.

Web-process mount namespace (observed; paths are upstream choices, not foob code):

- `~/.cache` **read-write** (Mesa shader cache, GStreamer registry, fontconfig caches; the
  whole cache directory appeared in mountinfo). After a renderer compromise this is a
  persistence and cross-application cache-integrity path, and it exposes other
  applications' cached data.
- `/dev/dri` (GPU), `/dev/v4l`, `/dev/video0-3`, `/dev/media0-1` (v4l2, needed for video
  decoding), `/dev/snd`, the PulseAudio socket (`/run/user/<uid>/pulse`) and
  `~/.config/pulse` read-only. After a renderer compromise this permits direct
  camera/audio-device access outside WebKit's permission model; upstream WebKit binds
  these for decode/audio (its source has a FIXME: "Pulse doesn't restrict clients").
- `/dev/fb0` on WPE, `/dev/udmabuf`, selected `/sys` subtrees.
- No general `/home` mount; `/tmp` is a private tmpfs; `/proc` is in the web process's own
  PID namespace; `/etc`, `/usr`, `/lib` read-only.
- The network process, being unsandboxed, has the UI process's full filesystem/mount
  namespace and environment. A network-layer engine bug is therefore outside bwrap, and
  a compromise exposes the user's files and environment. This is upstream behavior, not
  a foob defect.
- `CapEff` is 0 for the network process, but `NoNewPrivs` is 0 and the bounding set is
  full, so no-new-privs hardening is absent there. Normal for WebKit's trusted network
  process, but relevant to M3-05.

Implication: `AGENTS.md`'s "preserve WebKit process sandbox and site isolation as
offered" is met; "the web process is sandboxed" is true. The *scope* is wider than a
minimal sandbox and should be recorded. No foob-only fix exists without changing the
engine/packaging policy.

### 4.10 Permissions (engine defaults)

`browser.c` does not connect `permission-request`, `script-dialog`, `authenticate` or
`run-file-chooser`.

Observed/recorded defaults:

- Geolocation and Notification: denied (M1 probe).
- `getUserMedia` (camera/mic): fails with no device; the real permission decision is
  untested (M3-05).
- Website data access, media keys, XR, device info: documented deny-by-default.
- **Pointer lock: documented allow-by-default when unhandled** (WebKitGTK/WPE
  `WebKitPointerLockPermissionRequest`). Not click-tested here; the API docs are
  explicit. Impact: after a user gesture, a remote page can hide the cursor and capture
  relative motion until Escape; no file/device access. This is a UI capability, not a
  data leak, but it contradicts the architecture's "never silently grant" P0.
- **Install-missing-media-plugins: documented allow-by-default.** Not exercised here;
  record it with the pointer-lock decision (low expected impact in this build).
- `alert`/`confirm`/`prompt`: auto-dismissed (`false`/`null`) in the M1 probe.
- `window.open`: returns `null`; no window.
- HTTP 401: page stays loading, no prompt, no title (M2-10). Not a credential leak, but a
  possible stuck-load annoyance.
- `<input type=file>`: no picker observed; M3-02.
- TLS certificate error: load refused (M1 probe); no policy override exists.

Recommendation for M3-05: connect `permission-request` and deny everything not
explicitly needed (at minimum pointer lock and missing-media-plugins), with a fixture
and a real-click guided row. Keep TLS fail-closed and sandbox on.

### 4.11 Build hardening and dependencies

`meson.build`:

- `b_pie=true`; link `-Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,--as-needed`;
- `-fstack-protector-strong -fstack-clash-protection -fcf-protection=full -fno-plt
  -ftrivial-auto-var-init=zero -D_FORTIFY_SOURCE=3` (fortify only with optimization);
- `warning_level=3`, C23; `-Werror=format-security`, `-Wformat=2`,
  `-Wnull-dereference`, etc.

`readelf`/`file` on `build/foob`:

- `DYN (Position-Independent Executable)`, `BIND_NOW`/`FLAGS_1: NOW PIE`,
  GNU_RELRO present, GNU_STACK `RW` (no execute), `__stack_chk_fail` imported,
  FORTIFY `_chk` wrappers present;
- CET `IBT, SHSTK` properties;
- no `RPATH`/`RUNPATH`;
- mode `0755`, not setuid; 93,992 bytes with debug info in the local build (the packaged
  binary should be stripped; debug info is not a runtime weakness but helps local
  inspection).

Dependencies: system Arch packages; `wpewebkit 2.54.1-2` is current in `extra` and no
update was pending. It depends on `bubblewrap`, `libseccomp`, `xdg-dbus-proxy`, so the
sandbox is available. `wpewebkit` does **not** depend on `libsecret`; the HTTP-auth
credential-storage default is therefore inert today, but D-19/M2-04 still requires
turning it off explicitly. The engine's security advisories are the dominant risk:
WSA-2026-0006 (2026-09-29) lists many CVEs fixed before 2.54.0, including sandbox
escapes and network/GPU issues. Keep the engine updated urgently; do not weaken TLS or
sandbox to make a site work.

### 4.12 Repository and Git hygiene

- 68 tracked files; no profiles, cookie databases, credentials, private keys, AWS/GitHub
  tokens, `.env` files or machine home paths in the working tree.
- Scanned **all 174 reachable commits** (`git log --all -p`) for private-key, AWS,
  GitHub-token and `/home/…`/hostname patterns: none. The only non-hash long strings
  were a public GitHub repository URL and a KaTeX SRI hash; the rest were commit,
  binary and source SHA256 values. The qvu research docs mention cookie **names**
  (`GOOGLE_ABUSE_EXEMPTION`, etc.), not values.
- `git fsck` shows ~20 unreachable objects: dropped-stash WIP/index commits from
  2026-10-17/19. They are local, not pushed. Their trees contain only repository files as
  of those snapshots; no cookie/profile/credential files by name. If the "nothing is
  locally recoverable" claim matters, clear dropped stashes/reflogs and `git gc` after
  confirming nothing is needed. Do not `push --all` or bundle without checking them.
- `.gitignore` covers build dirs, `docs/validation/runs/`, `__pycache__/`,
  `.agents/`, `.codex/`, `.aws/` and the local design references. It does not cover
  `core`/`*.core`; adding those is cheap insurance given F-77's concern about page memory.
  `.claude/` is ignored only through `.git/info/exclude`, not `.gitignore`, so a fresh
  clone may show it as untracked. Browser profiles live outside the repo (XDG dirs), so a
  profile pattern is not strictly needed.
- `.agents/`, `.codex/` and `.aws/` are empty and `0755`; no secrets.
- No core files or leftover test browser/helper processes were present after the checks.

### 4.13 Test harness observations

- `tests/check_cli.py` guards `prlimit` with `shutil.which`; `tests/nested_lifecycle.py`
  uses `prlimit` and `unshare` unconditionally. Without util-linux or user namespaces,
  the nested suite errors instead of skipping those two scenarios. Test portability only.
- `tests/nested_lifecycle.py:139-146` calls `os.killpg(browser.pid, SIGKILL)` in a
  `finally` without a start-time check. After the browser has exited, a PID/PGID-reuse
  race could signal an unrelated group. The window is tiny and this is a test harness,
  not the browser, but a safer form uses the tracked start times or skips when the group
  is empty.
- `tests/benchmark/run.py` deliberately sets `FOOB_DIAGNOSTICS=1`, so benchmark crashes
  would keep core dumps; that is the documented diagnostics behavior. Its outputs are
  local evidence, not product data.
- The scratch download/temp test scripts used for this review lived outside the repo and
  were not committed; they used a loopback server and a redirected `XDG_*` tree.
  No automated or manual browser check opened Google Scholar or any external site; only
  `about:blank`, local files and loopback pages were loaded.

---

## 5. Known/suspected items confirmed or carried

These are already tracked; this review did not re-open them, but a complete security
record should name them:

| ID | Item | Status / owner |
|---|---|---|
| S-01 | SIGTERM/SIGINT arriving before the main loop still allows the first load | Suspected, low privacy; M3-07 |
| S-02 | Ctrl-C race could exit 4 instead of 0 | Suspected, UX; M3-07 |
| S-03 | Title trust computed only on title change; local→remote same title | Suspected, low; M2-03 |
| S-04 | Title trust reads pending URI; remote page pending `about:blank` | Suspected, low; M2-03 |
| D-08 | All local `file:` content trusted | Intended for M1; M4 |
| D-19 | HTTP-auth credential storage default on | M2-04 turns it off |
| D-26 | Live diagnostics redacted; on-demand host-only dump unbuilt | M2-06 |
| D-42 | F-77 fallback residual risks (helpers, socket collector, read-once pattern) | Accepted |
| D-23 | Future private child inherits `FOOB_DIAGNOSTICS`; URL in argv visible to same user | M2-03/M2-04 |
| — | `about:blank` origin vs URI for a future navigation policy | M2-03/M4 |
| — | No origin/TLS indicator; page-controlled title; homoglyphs | Deliberate minimal chrome; M2-13 |
| — | Network process unsandboxed (upstream) | M3-05 record |
| — | Web sandbox exposes `~/.cache`, v4l2, ALSA/Pulse (upstream) | M3-05 record |
| — | Pointer lock / missing-media-plugins allowed by default | Candidate F-78 / M3-05 |

### Other merge-review follow-ups (not security-critical)

These were already recorded in the handoff's merge-review row; repeat them here so a
later agent has the complete list:

1. `docs/validation/minibrowser-comparison.md` rows 4 and 6 still say foob requires
   `file:///absolute/path` and rejects `--version`; both were implemented in M2-02.
   Update or mark them done before M2-13 uses the table as its baseline.
2. `foob_navigation_allowed()` canonicalizes its destination twice (once directly, once
   inside `foob_uri_is_local()`). Harmless and unused until M2-03; simplify when the
   first caller is added.
3. `docs/validation/guided-checks-m2.md`'s first F-77 subsection describes the commit-1
   design (limit 0 and a non-dumpable foob) in the present tense; the D-41/D-42
   subsections below supersede it. Add a "superseded by the rework below" pointer.

Test-only observations from 4.13 (nested `prlimit`/`unshare` assumptions and the
`killpg` race) are not security findings.

---

## 6. Recommended decisions for the maintainer

1. **Pointer lock / permissions (M3-05):** record the documented allow-by-default for
   `WebKitPointerLockPermissionRequest` and `WebKitInstallMissingMediaPluginsPermissionRequest`
   (candidate new finding F-78) and decide whether to connect `permission-request` to
   deny everything not explicitly needed. This is a small, testable policy change; keep
   it out of the M1/M2 code until then.
2. **M3-05 sandbox record:** add the runtime evidence: network process unsandboxed
   (upstream), web process `~/.cache` rw and v4l2/ALSA/Pulse access, UI process
   unsandboxed. Decide "accept and document" vs. custom-engine work; do not weaken
   anything for memory. Keep engine updates urgent.
3. **M2-03:** use the committed document origin in any policy caller; add a regression for
   the remote→`file:` redirect (blocked today) and the remote→`about:blank`→`file:` test;
   keep `foob_navigation_allowed()` as the single check; pass only canonical
   `http`/`https` addresses to a future private child, never `from_argument()`, and do
   not inherit `FOOB_DIAGNOSTICS=1`.
4. **M2-04:** turn persistent credential storage off explicitly; profile dirs `0700`;
   private mode creates no profile; decide whether `--private` always keeps core dumps
   off even under diagnostics.
5. **M2-06:** on-demand dump `0600`, in the profile `0700` directory, hosts only, never
   written in private mode or without the key; test that live output contains no host,
   path or query.
6. **Optional hygiene:** add `core`/`*.core` to `.gitignore`; clear or inspect the
   dropped stashes; fix the stale security-record wording in
   `docs/validation/guided-checks-m2.md` (the first F-77 subsection still describes the
   superseded limit-0/non-dumpable design in the present tense) and the stale
   MiniBrowser-comparison rows 4/6; simplify the double canonicalization in
   `foob_navigation_allowed()` when M2-03 builds its caller.
7. **Do not:** disable TLS checks or sandboxing, add `xdg-open`/shell interpolation, load
   a page's `target=_blank` request itself, or trust a `file:` URL from remote content.
   These remain the project's boundaries.

**Recording:** if the maintainer accepts these, add the pointer-lock item as the next
finding (`F-78` in `docs/validation/findings.md`), the sandbox/permission observations to
`docs/validation/guided-checks-m2.md` under M3-05 (or a dedicated security section), and
the decisions to `docs/validation/handoff.md` in the same commit, per `AGENTS.md`. This
report lives at `docs/validation/security-review-m2-step1.md`; it is a review
artifact, not a source of truth. Do not record findings or change policy without the
maintainer's decision.

---

## 7. Reproduction notes for another agent

Read `AGENTS.md`, `docs/validation/handoff.md` and `docs/validation/findings.md` first.
Do not repeat checks recorded here unless the code changed; when re-running, use a fresh
build directory and a disposable output path.

Useful commands (adapt paths; the nested suite opens a window on the desktop):

```sh
# build and tests (use fresh directories; add --wipe to reuse one)
meson setup /tmp/foob-review-build .
meson compile -C /tmp/foob-review-build
meson test -C /tmp/foob-review-build --print-errorlogs

# static analysis
meson setup /tmp/foob-analyzer . -Dc_args=-fanalyzer
meson compile -C /tmp/foob-analyzer
CC=clang meson setup /tmp/foob-clang .
scan-build --status-bugs --use-analyzer=/usr/bin/clang -o /tmp/scan-reports ninja -C /tmp/foob-clang
clang-tidy -p /tmp/foob-clang --checks='-*,clang-analyzer-*' src/*.c

# sanitizers
meson setup /tmp/foob-asan . -Db_sanitize=address,undefined -Db_lundef=false
meson compile -C /tmp/foob-asan
meson test -C /tmp/foob-asan --print-errorlogs

# security-relevant nested scenarios (opens a nested niri window)
python3 tests/nested_lifecycle.py build/foob \
  core-dumps-off core-dumps-diagnostics core-dumps-off-hard-zero \
  core-dumps-off-socket-collector download-cancelled blank-form-does-not-navigate \
  title-remote-sanitized canonical-address-loaded

# one lifecycle-only runtime evidence run (opens one window; writes under /tmp)
mkdir -p /tmp/foob-runtime
python3 tests/benchmark/run.py --binary build/foob --label security-check \
  --uri about:blank --output /tmp/foob-runtime/blank.json --lifecycle-only

# ELF hardening
readelf -hW build/foob
readelf -lW build/foob | grep -E 'GNU_STACK|GNU_RELRO|INTERP'
readelf -dW build/foob | grep -Ei 'BIND_NOW|FLAGS|RPATH|RUNPATH'
readelf -nW build/foob

# repo hygiene (does not dump file contents)
git ls-files | wc -l
git log --all -p | grep -nE 'BEGIN (RSA|OPENSSH|EC|PGP) PRIVATE|AKIA[0-9A-Z]{16}|ghp_[A-Za-z0-9]{20,}|/home/[a-z]+/'
git fsck --unreachable --no-reflogs
```

For the manual sandbox inspection, launch a local page with `build/foob`, then read
`/proc/<pid>/status` (`NoNewPrivs`, `Seccomp`, `NSpid`), `/proc/<pid>/ns/{mnt,pid,user,net}`
and `/proc/<pid>/mountinfo` for each process in the browser's tree; compare UI, network
and web processes. `tests/benchmark/run.py` records the same fields in `runtime_evidence`.
Never synthesize keyboard or mouse input; click-gated checks are guided rows for the
maintainer.

---

## 8. Bottom line

- **foob code: no high/critical finding.**
- **Trust boundary: sound for the tested inputs; over-rejects some valid URLs in the
  safe direction.**
- **Memory safety: clean under the checks above.**
- **Interim safety guards: downloads cancelled, new windows ignored, remote→file blocked,
  core dumps off, diagnostics redacted.**
- **Residual risk: upstream WebKit (network process unsandboxed; CVE volume), wider web
  sandbox than minimal, engine-default pointer lock, no origin/TLS UI, known S/D items.**
- **Use guidance: trusted pages low; arbitrary untrusted browsing medium until M2-03,
  M3-05 and M4.**
- No code was changed.
