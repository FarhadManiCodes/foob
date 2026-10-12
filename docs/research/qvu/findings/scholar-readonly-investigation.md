> Historical qvu material imported on 2026-10-12. Qt-specific behavior and earlier decisions require WPE validation. Foob’s architecture decision and roadmap take precedence. Reproduction commands refer to the original qvu checkout.

# Scholar compatibility investigation — 2026-10-12

The user requested a separate read-only agent. It reviewed the probe and primary
documentation/source, changed no files, launched no browsers and made no Scholar
requests. This report preserves its findings; no Scholar fix was implemented.

## Evidence and limits

Earlier logs contained a storage-access denial and `solveSimpleChallenge` being
undefined. Treat why Google challenged the request and why the challenge failed
as separate questions. Neither cause is established by these logs alone.

In the exact QtWebEngine v6.11.2 source, Blink's `STORAGE_ACCESS_GRANT` and
`TOP_LEVEL_STORAGE_ACCESS` map to Qt's `Unsupported` permission. Unsupported
requests return `DENIED`. A generic Qt permission handler therefore cannot enable
those permission types. This plausibly explains the storage-access denial; it
does not establish a causal link to the missing challenge function.
[Qt permission implementation](https://github.com/qt/qtwebengine/blob/v6.11.2/src/core/permission_manager_qt.cpp)

The probe constructs an off-the-record profile. This permits cookies and cache
in memory; it does not mean cookies are disabled. The probe installs no cookie
filter or request interceptor and does not explicitly disable JavaScript.
Persistent login support is not justified as a fix by the evidence.
[Qt profile documentation](https://doc.qt.io/qt-6.11/qwebengineprofile.html)

Google documents unusual-traffic challenges involving network activity, including
shared or VPN networks. The trigger for this request is unknown.
[Google Search guidance](https://support.google.com/websearch/answer/86640?hl=en)
Its listed reCAPTCHA desktop browsers are recent Chrome, Firefox, Safari and
Chromium Edge versions. QtWebEngine is not listed; Chromium ancestry alone does
not establish supported challenge behavior.
[reCAPTCHA browser requirements](https://support.google.com/recaptcha/answer/6223828?hl=en)

Keep transient site storage when evaluating feature removal. Qt's cookie filter
also affects DOM storage, IndexedDB and service workers, so removing cookies can
break more than account logins.
[Qt cookie-store documentation](https://doc.qt.io/qt-6.11/qwebenginecookiestore.html)

## Proposed checkpoints, not performed

1. Manually try one identical Scholar query in Firefox normal mode, Firefox
   private mode and qvu, on the same network around the same time. If private
   Firefox works while qvu fails, persistent login is unnecessary for that case;
   focus on engine/application compatibility. If all fail, network/site behavior
   remains plausible. Complete any challenge manually; avoid repeated automation.
2. Record actual user agent, Chromium version, JavaScript/storage settings,
   permission signals and failed script requests in a small diagnostic batch.
   Do not assume spoofing, blanket permission grants or disabling security is a fix.
3. Test storage-access permissions with an appropriate local cross-site iframe
   fixture separately from Google. This isolates the API boundary but cannot
   establish that Google's particular challenge depends on it.

The scrolling shake remains a separate unresolved issue. The existing label
layout check is not evidence that Scholar scrolling is fixed.

## Firefox handoff implication

Preserve the attempted Scholar/search URL as well as the current page URL. After
a challenge redirect, the current URL may be Google's challenge page; handing
off the original search is more useful. Explicit Firefox launch with separate
`QProcess` arguments can select Firefox, whereas `QDesktopServices::openUrl()`
selects the system default browser. No cookie transfer or account integration
is necessary to pass the URL.
[Qt desktop services](https://doc.qt.io/qt-6.11/qdesktopservices.html)
