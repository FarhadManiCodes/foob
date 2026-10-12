> Historical qvu material imported on 2026-10-12. Qt-specific behavior and earlier decisions require WPE validation. Foob’s architecture decision and roadmap take precedence. Reproduction commands refer to the original qvu checkout.

# Scholar after clearance expiry — 2026-10-12

Conclusion: three Qt profile configurations were challenged after the saved
clearance expired, while the user reported that a fresh Firefox private window
worked perfectly. The current Qt Scholar experience does not meet the user's
usability expectation. Selected-cookie retention has not resolved this browser
compatibility gap. Its cause remains unknown.

## Comparison

The saved `GOOGLE_ABUSE_EXEMPTION` expired at 04:20:58 UTC (06:20:58 Berlin time).
Testing started at 08:45:53 UTC, roughly four hours after expiry. The source profile
still contained the expired cookie and six other cookies with future expirations.

Fresh private test directories received snapshots from the original saved profile.
The original was left intact. Four-cookie selection matched the agreed
name/domain/path combinations; its initial cookie database contained exactly four
rows, including the expired clearance cookie. Cookie values were not printed or
committed. Nothing extended expiry or solved a CAPTCHA during the comparison.

| Configuration | Result |
|---|---|
| Fresh Qt profile seeded with the agreed four cookies | Google `/sorry/index`; complete DOM, zero `.gs_r` entries |
| Fresh Qt profile seeded with all seven cookies | Google `/sorry/index`; complete DOM, zero `.gs_r` entries |
| Qt profile copied from the complete saved directory, including cache and other state | Google `/sorry/index`; complete DOM, zero `.gs_r` entries |
| Firefox private window using a fresh isolated profile | User reported it worked perfectly; warning was only in qvu |

The Qt trials used the same query, `fluid structure interaction`, with `hl=en`,
Accept-Language `en-US,en;q=0.9`, and Linux/Chromium 140 UA without the Qt token.
Qt 6.12.0 / QtWebEngine 6.11.2 were unchanged. The earlier manual clearance had
allowed these settings to reach results before expiry.

Firefox was opened with `--no-remote`, a fresh profile under the private test
directory, and `--private-window` for the same search. Its result is user-observed;
no Firefox DOM inspection or network trace was collected. This was sequential
testing on the same machine/network, not simultaneous identical HTTP requests.

Qt logged the previously observed `requestStorageAccess: Permission denied` and
`solveSimpleChallenge is not defined` errors. They also occurred in earlier
sessions where manual CAPTCHA completion succeeded; their presence does not
establish the cause of the challenge or a fatal rendering failure.

## What this establishes and leaves open

- Discarding the three excluded cookies is not sufficient to explain this failure:
  the seven-cookie and complete-profile trials failed too.
- No benefit from keeping those extra cookies or other copied profile state was
  observed in this comparison. This does not prove they can never help later.
- Firefox's success makes a block affecting every browser on this network an
  inadequate explanation of the observed difference. Google's assessment could
  still combine browser behavior, network signals, timing and other factors.
- The challenge occurred after expiry, but this does not prove expiry was its sole
  cause. Several test navigations may themselves affect Google's assessment.
- The earlier one-cookie result established immediate startup access while the
  clearance was valid; it did not establish long-term reliability or renewal.

Google's warning lists possible sources of unusual traffic. It is not a diagnosis
of this machine or evidence of malware, a plugin, or automation outside these
known tests.
[Google Search help](https://support.google.com/websearch/answer/86640?hl=en)

## Next direction proposed

Stop repeated search requests and defer selective-cookie API work until the
compatibility question is understood. Retain the agreed four-cookie policy as a
candidate, rather than treating it as a demonstrated Scholar reliability solution.
Reliable Scholar browsing should be reviewed before proceeding with production
research-browser implementation.

Start with read-only comparison of Qt's actual request/runtime configuration and
Firefox's behavior. Choose one bounded experiment from evidence; do not promise
that another UA/header override will solve the problem. If Qt cannot provide
acceptable reliability, discuss explicit Scholar routing to Firefox as a product
tradeoff with the user. Firefox handoff is already an agreed feature.

The final Qt and Firefox comparison windows were left open. Further test requests
were stopped. No exploratory or production code changed, and no memory/performance
measurements were made. Private profiles and raw reports remain outside Git.
