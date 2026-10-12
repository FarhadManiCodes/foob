> Historical qvu material imported on 2026-10-12. Qt-specific behavior and earlier decisions require WPE validation. Foob’s architecture decision and roadmap take precedence. Reproduction commands refer to the original qvu checkout.

# Scholar cookie minimization — 2026-10-12

Conclusion: a fresh named profile seeded only with the `.google.com` cookie
`GOOGLE_ABUSE_EXEMPTION` reached Scholar search results. An empty-profile control
was challenged; a fresh profile seeded with all seven saved cookies also worked.
One cookie was sufficient as initial restored state in these observations. This
does not guarantee future clearance or establish a universal Google policy.

## Method and observations

The starting profile had previously passed a manually completed CAPTCHA and
survived a clean restart. It was closed normally before copying its cookie database
with SQLite's backup operation. Its original data was preserved. No CAPTCHA was
solved in the following trials.

`probes/cookie_profiles.sh` prepared private directories under `/tmp`. Fresh
profiles received only a cookie database, with no copied history, HTTP cache,
visited links, local storage or other profile files. The single-cookie copy was
filtered by both name and domain; its initial row count was exactly one. Values
were kept opaque, were not printed, and were not committed. Expiry and other
attributes were not extended or fabricated.

Every trial used the same Scholar query (`fluid structure interaction`), `hl=en`,
Accept-Language `en-US,en;q=0.9`, and Linux/Chromium 140 UA without the Qt token.
Qt 6.12.0 / QtWebEngine 6.11.2 and the network environment were held constant.
Keeping these settings does not prove either override is required.

| Initial restored state | Observed result |
|---|---|
| No cookies or other profile data | Google `/sorry/index`; zero Scholar result entries |
| Seven saved cookies; no other copied state | Scholar `/scholar`; 12 `.gs_r` entries |
| Only `GOOGLE_ABUSE_EXEMPTION`; no other copied state | Loaded Scholar results title and completed page |
| Fresh confirmation copy, again initially one cookie | Scholar `/scholar`; 12 `.gs_r` entries |

The first one-cookie process ended before a custom DOM-count check could be
sent; its automatic report recorded a successful completed Scholar results page.
The fresh confirmation window remained inspectable and provided the result count.
The confirmation window was left open for the user.

All seven source cookies were persistent: `GOOGLE_ABUSE_EXEMPTION`,
`SEARCH_SAMESITE`, `__Secure-ENID`, `GSP`, `OTZ`, `__Host-GAPS` and `_GRECAPTCHA`.
The latter six were unnecessary as initial restored state for the successful
one-cookie trials. This does not imply that Google never needs them elsewhere.

The candidate's stored expiry was **2026-10-12 04:20:58 UTC**, or **06:20:58 Berlin
time**. The tests finished around 01:51 UTC. Clearance is temporary; Google's
server may also challenge a still-unexpired cookie. No login was used.

## Limits and next functional question

These were named profiles. Qt and the website could create fresh cookies, cache
and other storage during each visit. The evidence concerns the minimum state
needed at startup, not a browser that prohibits every other cookie or disk write.
No memory or performance conclusion follows from this experiment.

The next bounded API experiment would use an off-the-record research profile,
restore the selected cookies below through `QWebEngineCookieStore::setCookie`,
and save only their current valid values and attributes for the next run. That could leave ordinary
site cookies and cache temporary while preserving selected clearance. This API
path, import timing and clean export are **not tested yet**. Respect expiry and
define a reset action; keep preview storage separate.

Qt cookie loading is asynchronous. Its cookie filter also controls other storage
APIs, so a broad filter would change more than persistence and should not be
introduced as a shortcut for this experiment.
[Qt cookie-store API](https://doc.qt.io/qt-6.11/qwebenginecookiestore.html)

The experiment added a 13-line shell helper and no C++ changes. `bash -n` and
`shellcheck` passed. Raw profiles and reports remain outside Git.

## Retention and renewal discussion

The user agreed to retain `GOOGLE_ABUSE_EXEMPTION` across restarts. Retention means
keeping Google's latest valid cookie, including its original expiry and attributes;
it does not make the current value permanent.

A subsequent metadata-only check at 01:56 UTC found the same expiry and update
time in the source and confirmation profiles: expiry 04:20:58 UTC, last update
01:22:49 UTC. The successful visits above did not update these stored timestamps.
Ordinary browsing therefore has not demonstrated renewal. No cookie values were
read into diagnostics for this check.

Proposed lifecycle for the selective-persistence experiment:

1. Restore an unexpired saved cookie before the first research navigation.
2. Observe cookie-store changes for the exact name/domain/path tuple:
   `GOOGLE_ABUSE_EXEMPTION`, `.google.com`, `/`.
3. When Google issues a replacement, save its current value and attributes
   privately and atomically. Handle deletion/replacement ordering so removal of
   an old value cannot erase a newer replacement. Restore events themselves do
   not establish that Google renewed the cookie.
4. Respect expiration and server deletion. A later search may work without it;
   if Google presents a challenge, let the user complete it in the research
   window and capture the newly issued cookie, if any.

Qt exposes cookie addition/removal signals and asynchronous import. Replacement
signal ordering and the selective save/restore flow still require an API test.
[Qt cookie-store API](https://doc.qt.io/qt-6.11/qwebenginecookiestore.html)

No documented silent renewal endpoint was established. Do not extend the saved
expiry locally or add timer-driven requests to keep clearance alive. Google's
documented recovery for the unusual-traffic page is manual CAPTCHA completion;
this guidance does not promise a particular cookie or clearance duration.
[Google Search help](https://support.google.com/websearch/answer/86640?hl=en)

## Agreed selected-cookie policy

After discussing continuity beyond the immediate restart test, the user agreed
to a four-cookie retention set. This is the candidate policy to validate, not a
change to the running probe or evidence that these cookies prevent future challenges.
It expands the earlier clearance-only proposal.

| Cookie | Observed domain and path | Retention decision | Evidence and limits |
|---|---|---|---|
| `GOOGLE_ABUSE_EXEMPTION` | `.google.com`, `/` | Across restarts while valid | Alone restored immediate access in our tests; renewal remains unverified. |
| `_GRECAPTCHA` | `www.google.com`, `/recaptcha` | Across restarts while valid | Google documents reCAPTCHA risk analysis; a continuity benefit here is untested. |
| `GSP` | `.scholar.google.com`, `/` | Across restarts while valid | Historical Zotero investigations associate it with Scholar preferences/citation export; current contents and complete purpose are unverified. |
| `__Secure-ENID` | `.google.com`, `/` | Across restarts while valid | Google's ENID documentation describes preferences and Search analytics. This analytics tradeoff was accepted; no CAPTCHA benefit has been shown. |
| `SEARCH_SAMESITE` | `.google.com`, `/` | Temporary only | Exact purpose not established from authoritative documentation; do not infer necessity from its name. |
| `OTZ` | `accounts.google.com`, `/` | Temporary only | Account-domain placement observed; exact purpose and any Scholar need unverified. |
| `__Host-GAPS` | `accounts.google.com`, `/` | Temporary only | Account-domain placement observed; exact purpose unverified. Appeared without Google login. |

Purpose references: [Google reCAPTCHA FAQ](https://developers.google.com/recaptcha/docs/faq),
[Google cookie policy](https://policies.google.com/technologies/cookies?hl=en), and
[Zotero's historical Scholar investigation](https://forums.zotero.org/discussion/48651/says-saving-to-but-nothing-gets-saved).
Google's policy labels the ENID family `_Secure-ENID`; the exact observed cookie
name here is `__Secure-ENID`.

Temporary-only cookies remain usable during the running research session and are
discarded on exit; this is a persistence decision, not a proposal to block cookies.
Match the retained name/domain/path combinations, preserve Google's attributes
and replacements, and honor expiration and deletion. The replacement handling
described above applies to every selected cookie. Keep document-preview state
separate, and retain no browsing history or research-tab/session lists.

The next continuity comparison is a fresh four-cookie profile against the
seven-cookie control after clearance expires naturally, with Firefox private mode
as a browser comparison if needed. Expiry alone does not imply another CAPTCHA.
No background renewal requests, expiry manipulation, automatic CAPTCHA completion,
or code changes were introduced for this decision. Selective Qt API persistence
remains untested. The subsequent [post-expiry comparison](scholar-post-expiry.md)
challenged the four-cookie, seven-cookie and complete Qt profiles while the user
reported Firefox private mode worked. The candidate policy is retained; it has
not established reliable Scholar access. Cookie API work is deferred in favor of
investigating this compatibility gap.
