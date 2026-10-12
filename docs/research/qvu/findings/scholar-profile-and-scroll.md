> Historical qvu material imported on 2026-10-12. Qt-specific behavior and earlier decisions require WPE validation. Foob’s architecture decision and roadmap take precedence. Reproduction commands refer to the original qvu checkout.

# Scholar profile retention and scrolling — 2026-10-12

Conclusion: English Scholar results worked after manual CAPTCHA completion in an
isolated named profile. Reopening the same profile reached results without a new
challenge observed. The user reported stable scrolling with no shaking. No Google
login was needed for the observed result.

This is functional exploration, not production implementation or optimization.
The memory snapshot below was separately requested out of curiosity.

## Profile experiment

The earlier off-the-record session could complete its challenge, but its state
was lost when the process ended. A fresh off-the-record search again challenged
the user. The user explicitly rejected repeating the challenge on each opening.

Added `--profile-dir` using `QWebEngineProfileBuilder`. Storage and cache paths are
set before profile creation; the profile is owned until all pages are destroyed.
The probe now requires Qt 6.9 for this API. Without the option it still creates
an off-the-record profile. This does not install a persistent production profile.
[Qt profile builder](https://doc.qt.io/qt-6.11/qwebengineprofilebuilder.html)

Test directory: `/tmp/qvu-scholar-retained-profile`, created user-only (mode 0700).
Diagnostics confirmed `offTheRecord=false`, `AllowPersistentCookies` and `DiskHttpCache`.
Cookie values and profile contents were not read, exported or committed.
The same language and UA settings from the preceding test were retained:

- Accept-Language: `en-US,en;q=0.9`.
- Linux/Chromium 140 UA without the Qt token.
- Scholar search for `fluid structure interaction`, with explicit `hl=en`.

The first named-profile visit still challenged the user. The user completed it
manually and confirmed English results. A probe report showed the Scholar search
path, a search-results title and nonzero result markup. This establishes search
results beyond the earlier homepage-only check.

The process exited normally through `:quit`. A second process reused the exact
profile directory and search request. It loaded Scholar results successfully,
with `hl=en`, nonzero result markup and no new challenge observed in startup logs.
Only one restart was checked; future challenges and the clearance lifetime remain
unknown. This supports retention as a useful workflow feature, not a guarantee
that Google never challenges the browser again.

Keeping the language/UA overrides here does not establish that either is necessary
for retention. They were held constant to isolate profile reuse. Their earlier
individual tests did not avoid the initial challenge.

## Scrolling and navigation

The user scrolled the loaded Scholar results and reported no shaking. Reported
viewport width remained 950 pixels before and after; window width was also 950.
No continuous geometry trace or screen recording was taken, so the cause of the
original shaking is still unknown. The older constrained status label had already
passed a layout check. Treat this as a successful interactive retest for the
current setup, not proof that every page or rendering configuration is stable.

The user followed a link and needed Back. The old probe had no Back shortcut;
`:js history.back()` returned it to Scholar. Four lines add `Alt+Left`/`Alt+Right`
through the existing QWebEngineView Back/Forward APIs for subsequent launches.
Those shortcuts are active in the restarted binary, but a physical keyboard
check has not been independently recorded. Planned modal `H`/`L` controls remain
future implementation.

Google's challenge page uses a URL as its title. A display-title fallback now
shows `Web page` for empty or URL-like titles in the status strip/window title,
including the URL-copy confirmation. Actual URLs remain available for copying
and diagnostics; no Google page content was hidden or rewritten.

The profile/title batch changed 35 code lines including the CMake version change;
findings were reported before the four-line navigation-shortcut batch. Builds
passed and diffs passed whitespace checks. No memory/performance C++ was added.

## Requested current-state memory comparison

Near-simultaneous process-family snapshots using the existing `probes/memory.sh`:

| Browser | Aggregate PSS | Processes |
|---|---:|---:|
| qvu probe + Chromium descendants | 375.8 MiB | 6 |
| Firefox + descendants | 761.7 MiB | 11 |

Niri showed one window for each, both titled for the same Scholar query. Tab
counts were not independently verified. qvu used about half Firefox's PSS in
these loaded states. These are single observations, not matched cold launches,
averages or production-footprint evidence. Profiles, visited pages, caches,
browser UI, extensions and process models differ; there is no attribution of
the difference to any one factor. PSS prorates shared mappings. Summed RSS was
not used for comparison because it double-counts sharing.

The qvu process was the reopened named-profile probe, after previous navigation
within this test. No cookie/asset optimization or media-removal experiment was
performed. No extra measurement driver, installation or benchmark was added.

## State left for further testing

The restarted qvu window was left open for the user, with Back/Forward available.
Its profile remains under `/tmp` for this experiment and may not survive reboot
or temporary-file cleanup. Production profile location, reset/cleanup behavior
and accepted persistence scope belong in the implementation plan. Login is
optional website behavior, not required to retain this tested anonymous session.

Subsequent design discussion rejected browsing history and research-tab restoration.
The retention target is minimal site state; the full-profile test above does not
establish that saving cookies alone is enough. Cookie-only retention, memory-only
HTTP cache and internal visited-link persistence remain untested. Document preview
opening/refresh is independent of research browsing. These planning decisions do
not change the existing experimental profile or imply that its data was minimized.
