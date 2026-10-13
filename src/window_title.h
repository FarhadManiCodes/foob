#ifndef FOOB_WINDOW_TITLE_H
#define FOOB_WINDOW_TITLE_H

/*
 * Compositor-visible window title for a page title: "<page title> — foob", or
 * plain "foob" when the page has no usable title.
 *
 * Page titles from remote pages are untrusted: invalid UTF-8 is repaired,
 * control characters and Unicode bidi overrides (which could visually reorder
 * the trailing "foob") are removed, whitespace is collapsed and the title is
 * cut at 120 characters. Titles of local content (trusted = true, see
 * foob_uri_is_local()) keep their text apart from what the Wayland protocol
 * requires: valid UTF-8, no control characters, at most 240 characters.
 * Returns a newly allocated string; free with g_free(). Depends only on GLib.
 */
char *foob_window_title(const char *page_title, bool trusted);

#endif
