#ifndef FOOB_TARGET_H
#define FOOB_TARGET_H

/*
 * The startup-URI allowlist; foob_navigation_allowed() below reuses it, and the
 * full navigation policy remains roadmap M2-03.
 * Accepts exactly one of: "about:blank", an http(s) URI with a host and no
 * credentials, or a local absolute file: URI (no host or "localhost"), and
 * returns the form foob loads (newly allocated), or nullptr if the URI is not
 * allowed. That form is ASCII and one WebKit's own parser leaves unchanged, so
 * what was checked is what WebKit loads (M2-02).
 * Depends only on GLib so it can be unit-tested without a display.
 */
char *foob_target_canonical(const char *uri);

/*
 * The canonical form for a command-line argument (D-32): one starting with "/",
 * "./" or "../" is a path, made absolute against the kernel's current directory
 * (D-44; ".." inside the argument is removed as text) and turned into a file:
 * URI; anything else must be a URI. Either way the result passes
 * foob_target_canonical(). Only for the command line: an address from a page is
 * never a path.
 */
char *foob_target_from_argument(const char *argument);

/*
 * True for content that comes from this machine and is assumed safe:
 * "about:blank" and file: URIs with no host or "localhost". Everything else,
 * including a nullptr or unparsable URI, is remote and untrusted. Later local
 * document schemes (converted Markdown/Typst) extend this one function.
 */
bool foob_uri_is_local(const char *uri);

/*
 * Whether the application may itself navigate the view from from_uri to
 * to_uri. The destination must pass the same allowlist as a startup target, and
 * remote content must never navigate to local content: an application-initiated
 * load bypasses WebKit's own remote-to-local check, so any such load has to call
 * this first. Nothing calls it today: since D-04 foob makes no load on a
 * page's behalf (new-window requests are ignored); kept for the M2-03 policy.
 */
bool foob_navigation_allowed(const char *from_uri, const char *to_uri);

#endif
