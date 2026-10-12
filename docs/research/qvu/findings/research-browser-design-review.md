> Historical qvu material imported on 2026-10-12. Qt-specific behavior and earlier decisions require WPE validation. Foob’s architecture decision and roadmap take precedence. Reproduction commands refer to the original qvu checkout.

# Research browser design review — 2026-10-12

The user requested a return to the design after Firefox private mode worked while
Qt was challenged after clearance expiry. This is a read-only review of the probe,
blueprint, implementation plan and primary Qt/Google documentation. No browser was
launched, no Scholar request was made, and no code or installed package changed.

## Principle and investigation mistakes

The design should provide a minimal application interface while preserving the
web behavior needed by supported research sites. Omitting account-management or
media UI does not establish that browser storage or web APIs can be removed.
Research and local previews already have separate proposed profiles; retain that
separation so preview-specific restrictions do not become research-wide policy.

The investigation moved toward cookie minimization before establishing reliable
Scholar access without temporary clearance. Immediate restart success was useful
evidence of restoration, but not of compatibility over time. The assistant's
early recommendation of a one-cookie policy was premature as a reliability
solution. The four-cookie set remains an agreed candidate to test later, not a
proven solution to the present challenge.

A reference browser configuration should be kept unchanged throughout experiments.
Earlier default-Qt tests failed too, but we have not run a contemporary independent
bare-view or upstream Qt browser control against the current probe. Retaining
diagnostic language/UA overrides across later experiments does not establish that
they belong in the eventual product.

## What the current code actually does

`probes/render.cpp` leaves JavaScript enabled and installs no request interceptor,
cookie filter, injected navigator overrides, custom cookie persistence, or engine
feature-removal build. It uses the packaged QtWebEngine and chooses either an
off-the-record profile or a named profile through a diagnostic option.

Explicit settings disable spellchecking and the internal PDF viewer, mute audio,
require a gesture for playback and disable screen capture. Language/UA overrides
are optional CLI settings used in the recent Scholar trials. Diagnostics read DOM
state after loading; the probe does not contain a search polling or retry loop.
The current tool shell had no WebEngine flag/sandbox/debug/path overrides set;
`QT_QPA_PLATFORM` was `wayland;xcb`. This is a shell observation, not an audit of
every previous launch environment.

Most proposed engine cuts have not been implemented. The evidence therefore does
not establish that feature stripping caused the challenge. The title-only status
strip, separate preview windows and ordinary UI layout have no identified causal
link to it either. Those interface choices remain reasonable.

## Engine assumptions that need validation

QtWebEngine embeds Chromium but has its own integration and release schedule.
The installed runtime reports Chromium feature version `140.0.7339.225` with
security patches through `151.0.7922.71`. Qt documents cherry-picking security
patches; that does not upgrade the entire feature baseline or make the embedded
engine identical to Chrome.
[Qt architecture and version policy](https://doc.qt.io/qt-6.11/qtwebengine-overview.html)

The exact Qt 6.11.2 source maps storage-access permission types to Unsupported.
This is a real API difference and is consistent with the logged storage-access
denial. Its role in Google's decision remains unknown; earlier manual CAPTCHA
completion succeeded despite that error.
[Qt permission implementation](https://raw.githubusercontent.com/qt/qtwebengine/v6.11.2/src/core/permission_manager_qt.cpp)

HTTP UA strings, UA client hints and JavaScript-visible capabilities are distinct
surfaces. Replacing one header does not establish full browser equivalence. Qt
provides a separate client-hints API; its defaults were not modified by the probe.
[Qt client hints](https://doc.qt.io/qt-6.11/qwebengineclienthints.html)

Google lists the two most recent major versions of named desktop browsers as
supported for reCAPTCHA; QtWebEngine is not listed. This is a compatibility risk,
not proof that an older engine or a particular API caused our challenge.
[Google browser requirements](https://support.google.com/recaptcha/answer/6223828?hl=en)

Firefox is the successful user-observed control. Stock Chromium is not installed,
and an upstream Qt browser example has not been tested here. We should not claim
every standard browser has already passed this experiment.

## Proposed return to a baseline

1. Use a tiny independent Qt WebEngine view or upstream Qt example with a fresh
   isolated profile and default web settings, without qvu-specific restrictions or
   identity overrides. Make one deliberate Scholar visit and compare with Firefox.
   Normal cookies and temporary storage remain available during the visit.
2. If that control works, compare the current probe's differences one at a time.
   If both Qt controls fail, focus on shared Qt integration, engine capabilities,
   actual request behavior and launch environment. Failure would not alone prove
   an upstream bug or identify Google's private decision rules.
3. Consider a stock Chromium comparison if needed to narrow the browser-family
   question. It would require installation here, so it is not part of this review.
4. Establish usable target-site behavior before selective persistence or engine
   cuts. Add restrictions one at a time against the retained reference, with the
   user-facing checkpoints required by AGENTS.md.

Reliable Scholar browsing should be reviewed as a requirement for the research
browser. The Markdown/Typst preview architecture remains independently useful;
changing the research engine or workflow should be an explicit design discussion
if Qt cannot provide acceptable behavior. Firefox handoff remains an agreed action.

All steps above are proposals. This review does not authorize production work,
an engine fork, installation, broad request logging or any CAPTCHA automation.
