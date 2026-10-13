#ifndef FOOB_TARGET_H
#define FOOB_TARGET_H

/*
 * Startup-URI allowlist for trusted M1 pages; this is not an M2 navigation
 * policy. Accepts exactly one of: "about:blank", an http(s) URI with a host and
 * no credentials, or a local absolute file: URI (no host or "localhost").
 * Depends only on GLib so it can be unit-tested without a display.
 */
bool foob_target_is_valid(const char *target);

/*
 * True for content that comes from this machine and is assumed safe:
 * "about:blank" and file: URIs with no host or "localhost". Everything else,
 * including a nullptr or unparsable URI, is remote and untrusted. Later local
 * document schemes (converted Markdown/Typst) extend this one function.
 */
bool foob_uri_is_local(const char *uri);

#endif
