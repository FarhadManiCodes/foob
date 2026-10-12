# Scholar navigation and Dooble observations — 2026-10-12

These are final qvu-era observations transferred to foob. No WPE runtime test has
been performed by this experiment, and no particular Google detection rule was
identified.

## Independent Qt control

A separate browser used a plain QWebEngineView with default web settings, an
address field and eventually Alt+Left/Alt+Right Back/Forward shortcuts. It did not
apply qvu's media/PDF/spellchecker settings, UA/language overrides, cookie selection
or profile-import logic. A localhost page supplied ordinary links to the Scholar
homepage and the exact previous search.

The user reported that typed URLs reached CAPTCHA. An earlier answer that the
homepage worked was corrected after confusion caused by missing Back navigation;
do not count it as established success. After restarting with navigation shortcuts
and attaching diagnostics before showing the link page, the user explicitly
confirmed CAPTCHA from clicking the exact-search link.

The captured clicked requests to both Scholar `/scholar` and `/` had
`Sec-Fetch-Site: cross-site` and `Sec-Fetch-User: ?1`. Typed requests in the previous
control had `Sec-Fetch-Site: none` and `Sec-Fetch-User: ?1`. Scholar returned HTTP
302 to Google's `/sorry/index`, which returned HTTP 429. No Scholar results page
was established in this fresh control without manual clearance.

This rules out a missing `Sec-Fetch-User` header as the explanation for these
specific failures. Ordinary link navigation did not fix them. The rejection
occurred before delivery of the Scholar page; CAPTCHA-page JavaScript errors
followed the redirect and do not explain the initial redirect by themselves.
Earlier server-side assessments or network/identity factors could still matter.

The control reported default Qt/Chromium 140 identity, `navigator.webdriver=false`,
cookies enabled and language `en-POSIX`. The observer recorded only selected
non-cookie headers, path-only navigation, statuses and errors; no raw diagnostics
were imported here. URL sanitization was tightened for malformed URLs with query
delimiters encoded into the path.

The independent probe built successfully with `-fPIC`; Node syntax checks passed.
Missing Back/Forward was a control-UI defect and was corrected. The code remains
in the qvu repository; it is not a foob implementation base.

## Dooble and Firefox

The user installed Dooble and reported the same Scholar problem immediately with
the unchanged browser. This is user-observed; no Dooble network trace or independent
DOM inspection was collected. Foob should not infer that every Qt configuration or
every site fails from this observation.

The user previously reported that fresh Firefox private mode worked on the same
machine/network while qvu was challenged. Identical routing, remote addresses,
proxy behavior, protocol/TLS details and Google's private assessment were not
verified. The common QtWebEngine stack is a stronger suspect after the bare-view
and Dooble results, but the exact cause remains unresolved.

## Reason for the new direction

The user chose WPE WebKit with no GUI framework. This avoids committing to a
browser fork sharing the tested QtWebEngine stack. WPE compatibility must be proven
on its own baseline: test Scholar's search, result links, challenge behavior,
scrolling and restart behavior before treating a brief success as reliability.
Retest bundled math and local asset resolution under WPE; do not carry Qt-specific
permission or transport conclusions into the new engine as facts.
