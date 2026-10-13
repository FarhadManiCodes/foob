#ifndef FOOB_WINDOW_TITLE_H
#define FOOB_WINDOW_TITLE_H

/*
 * Compositor-visible window title for a page title: "<page title> — foob", or
 * plain "foob" when the page has no usable title.
 *
 * Page titles from remote pages are untrusted: invalid UTF-8 is repaired,
 * control characters (C0, DEL, C1) become spaces, invisible format characters
 * (Unicode bidi overrides, which could visually reorder the trailing "foob",
 * zero-width and tag characters and other default-ignorable code points; not the
 * joiners U+200C/U+200D or variation selectors 1-16) are removed, whitespace is
 * collapsed and the title is cut at 120 characters. A remote title that may hold
 * right-to-left text starts with U+200E LEFT-TO-RIGHT MARK, so the line stays
 * left to right and "foob" stays at its end. Titles of
 * local content (trusted = true, see foob_uri_is_local()) keep their text apart from what the Wayland protocol
 * requires: valid UTF-8, no control characters, at most 240 characters.
 * Returns a newly allocated string; free with g_free(). Depends only on GLib.
 */
char *foob_window_title(const char *page_title, bool trusted);

#endif
