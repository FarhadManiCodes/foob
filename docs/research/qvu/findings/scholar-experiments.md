> Historical qvu material imported on 2026-10-12. Qt-specific behavior and earlier decisions require WPE validation. Foob’s architecture decision and roadmap take precedence. Reproduction commands refer to the original qvu checkout.

# Scholar request experiments — 2026-10-12

Scope: functional exploration of Scholar access with small checkpoints. No
production implementation, optimization, memory claims, challenge automation or
changes to live Firefox/Neovim configuration. Persistent profiles have not been
enabled. The scrolling-shake report remains a separate unresolved problem.

## Diagnostics and environment

`probes/scholar_network.mjs` is a 32-line loopback DevTools observer added before
the first checkpoint. It records navigation, document/script response status,
loading failures and JavaScript exceptions; query strings and fragments are omitted.
It neither modifies site content nor records cookies or request bodies.

`probes/request_headers.mjs` is an 11-line local HTTP receiver that records only
Accept-Language, User-Agent and the basic brand/version client hint. Four C++
report lines expose profile headers and Chromium versions. Subsequent experiments
add two lines each for optional `--accept-language` and `--user-agent` controls.
These are diagnostic switches; defaults are unchanged. Each batch was below
50 changed code lines, with findings reported between experiments. Builds passed.

Runtime: Qt 6.12.0, QtWebEngine 6.11.2, Chromium 140.0.7339.225, security patches
through 151.0.7922.71. Firefox package: 157.0.1-1. Chromium's feature baseline
and security-patch level are distinct; the lower user-agent version does not mean
that only that version's security patches are present.

## Controlled observations

One Scholar homepage navigation per fresh Qt process, with off-the-record profiles,
default sandboxing/graphics and a loopback observer attached before navigation.
The browser's actual headers were independently checked on a local receiver.

| Case | Request configuration | Homepage result |
|---|---|---|
| Qt baseline | Default UA; Accept-Language absent | 302 to Google challenge, then HTTP 429 |
| Firefox private control | Existing Firefox private-window launch | User reported that Firefox seemed OK; no automated or visual verification of results |
| Qt language test | Default UA; `en-US,en;q=0.9` | Same redirect and HTTP 429 |
| Qt UA test, initial visit | Same explicit language; remove only Qt token while retaining Linux and Chromium 140 | Same redirect and HTTP 429 |
| Same Qt UA-test session, after manual CAPTCHA | User completed challenge in the off-the-record profile | Redirect back to Scholar homepage, HTTP 200 |

The baseline local receiver observed no Accept-Language header, and the profile
getter returned an empty string. JavaScript reported `en-POSIX`.
Setting the header produced the intended value on the receiver and JavaScript
languages `["en-US","en"]`. This establishes a configuration gap, not that the
gap caused Google's challenge; correcting it did not solve this visit.

The UA comparison used:

```text
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/140.0.0.0 Safari/537.36
```

The local receiver and `navigator.userAgent` confirmed the override. The client
hint still identified Chromium 140. DevTools `Browser.getVersion` continued to
report the browser-wide default UA with the Qt token; that field alone therefore
cannot validate a per-profile UA override.

All three Qt visits logged storage-access denial and `solveSimpleChallenge` being
undefined. Observed reCAPTCHA JavaScript and its frames returned 200; the trace
contained no `Network.loadingFailed` events. This rules out a simple download
failure for those observed scripts, not every possible challenge incompatibility.

## Challenge usability is not established by the console error

The current page defines `submitCallback`, independently of the undefined
`solveSimpleChallenge`. Its reCAPTCHA container and checkbox iframe have nonzero
dimensions (400 × 78 and 304 × 78). Geometry and CSS checks do not establish
that a human can finish the challenge. The user initially reported inability to
complete it, then corrected that report: manual completion succeeded. The trace
confirmed the redirect back to Scholar and HTTP 200 for its homepage. This establishes
that those console errors did not prevent completion in this tested configuration.
No automated challenge interaction was performed, and the baseline/language-only
profiles were not separately tested for manual completion.

Later navigation reached Google's German sign-in page with `hl=de`. No login was
performed by the agent. The probe exited before an English Scholar URL (`?hl=en`)
could be checked, so English UI and search-result behavior remain unverified.

A historical report in Google's reCAPTCHA repository shows related missing/helper
markup trouble in Chrome and Firefox, although its error differs from this case.
It is not proof of our cause and does not identify a fix.
[Google reCAPTCHA issue #489](https://github.com/google/recaptcha/issues/489)

qutebrowser's maintainer discussion records similar Google challenges with a
different QtWebEngine release and emphasizes that the triggering rules are
unknown. That supports treating this as a wider compatibility possibility,
not assuming that our probe code or a particular header is responsible.
[qutebrowser discussion #8721](https://github.com/qutebrowser/qutebrowser/discussions/8721)

The earlier read-only investigation identified unsupported storage-access
permission types in QtWebEngine 6.11.2. That boundary still does not establish
that storage denial caused this challenge's JavaScript error or prevented completion.
See [the separate read-only report](scholar-readonly-investigation.md).

## Assessment of the proposed three-setting change

Changing all three settings together would obscure which change matters.
Language and UA were tested individually relative to the preceding configuration;
both were verified locally, and neither resolved Scholar in these samples.

A named profile can retain persistent cookies/site data, potentially preserving a
successfully established session across restarts. It cannot guarantee that Google
accepts that state or make Qt support an unsupported permission. This behavior
changes the agreed off-the-record design, so no permanent profile was enabled.
If a profile experiment becomes useful, first use explicitly isolated temporary
storage and distinguish initial challenge completion from restart persistence.
[Qt profile documentation](https://doc.qt.io/qt-6.11/qwebengineprofile.html)

Anonymous access worked in the tested off-the-record session after manual completion,
so login is not established as necessary. Login could use the existing engine;
it does not require a native qvu account framework. Retaining account state would
require a persistence-policy decision. An active sign-in page brings its own web
content workload; no memory measurements were taken for profile persistence or login.
The working off-the-record session ended, so it cannot preserve its clearance state
for another launch. A named-profile test could explore retention without logging in.

The proposed Windows/Chrome 120 UA would misstate this Linux engine and advertise
an older version than its actual Chromium baseline. The tested UA omitted only
the Qt token; changing identity does not upgrade engine capabilities.

## Remaining work and UI implications

- Manual completion and subsequent Scholar homepage access succeeded in the tested
  off-the-record configuration; preserve this finding instead of treating the console
  errors alone as fatal. Search results and clearance retention remain unverified.
- If justified, isolate profile type and retention in a separate bounded test.
- Reproduce scrolling on an actual Scholar results page. The observed challenge and
  login pages had document height equal to viewport height, so they cannot exercise
  that report.
- Keep ordinary chrome title-only with a copy-URL action. The Google challenge
  sets its page title to the requested URL; a future display-label fallback is
  needed to keep that URL out of qvu's labels too. No site content was hidden.
- Preserve the attempted Scholar URL for Firefox handoff after challenge redirects.

These are sequential, single observations; network/site state can vary between
requests. Firefox working narrows the comparison but does not isolate a cause.
Raw probe reports remain in `/tmp`, outside Git.

## Reproduction of diagnostics

Start `node probes/request_headers.mjs` for the loopback header receiver. Launch
the probe on `http://127.0.0.1:9231/headers`, optionally specifying the diagnostic
language and UA switches. The profile report and receiver verify actual settings.

For one controlled site visit, set `QTWEBENGINE_REMOTE_DEBUGGING=127.0.0.1:9224`
when launching the probe. Attach:

```sh
node probes/scholar_network.mjs http://127.0.0.1:9224 /tmp/qvu-scholar-network.json
```

After observer initialization, initiate the single intended navigation through
the probe. Stop diagnostics when done; they are not a background service or
production browser interface.
[Qt debugging documentation](https://doc.qt.io/qt-6.11/qtwebengine-debugging.html)
