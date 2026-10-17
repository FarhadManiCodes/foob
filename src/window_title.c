#include <string.h>
#include <glib.h>
#include "window_title.h"

/* Keeps one Wayland string argument far below the protocol's message size limit. */
static constexpr glong untrusted_max_chars = 120;
static constexpr glong trusted_max_chars = 240;
/* A page controls its title and may set one of many megabytes on every change; only this
 * much of it is read, so the work per change stays bounded (F-76). */
static constexpr size_t max_input_bytes = 16 * 1024;

/* Format characters (Unicode category Cf) that render as visible glyphs, mostly Arabic
 * and Syriac prefixed number signs, and the joiners U+200C and U+200D, which emoji
 * sequences and Persian or Indic text need. Every other Cf character is invisible. */
static bool is_visible_or_needed_format(gunichar c)
{
    return c == 0x200c || c == 0x200d || (c >= 0x0600 && c <= 0x0605) || c == 0x06dd ||
        c == 0x070f || c == 0x0890 || c == 0x0891 || c == 0x08e2 || c == 0x110bd || c == 0x110cd;
}

/* Invisible characters that can hide or reorder text, or pad it with blank-looking runs:
 * every category Cf format character (bidi controls, zero width characters, soft
 * hyphen, interlinear annotation, musical format characters, tag characters, ...) except
 * the ones above, plus the rest of Unicode's Default_Ignorable_Code_Point set, which
 * renderers draw as nothing: combining grapheme joiner, Hangul and halfwidth fillers,
 * Khmer inherent vowels, Mongolian free variation selectors, the unassigned U+2065,
 * U+FFF0-FFF8 and U+E0000-E0FFF (with the variation selector supplement). Variation
 * selectors 1-16 (U+FE00-FE0F) stay, like the joiners: emoji presentation needs U+FE0F.
 * Also removed although not default-ignorable: Braille blank and object replacement. */
static bool is_invisible_format(gunichar c)
{
    if (g_unichar_type(c) == G_UNICODE_FORMAT)
        return !is_visible_or_needed_format(c);
    return c == 0x034f || c == 0x115f || c == 0x1160 || c == 0x17b4 || c == 0x17b5 ||
        (c >= 0x180b && c <= 0x180f) || c == 0x2065 || c == 0x2800 || c == 0x3164 ||
        c == 0xffa0 || (c >= 0xfff0 && c <= 0xfff8) || c == 0xfffc || (c >= 0xe0000 && c <= 0xe0fff);
}

/* Code points whose bidi class is, or by default would be, right-to-left (R or AL):
 * the Hebrew to Arabic Extended-A blocks, Hebrew and Arabic presentation forms, and the
 * right-to-left supplementary ranges. Unassigned code points there count too, because
 * bidi algorithms give them their block's default class. Bidi controls such as U+200F
 * are format characters and already removed from untrusted titles. */
static bool may_be_right_to_left(gunichar c)
{
    return (c >= 0x0590 && c <= 0x08ff) || (c >= 0xfb1d && c <= 0xfdff) ||
        (c >= 0xfe70 && c <= 0xfeff) || (c >= 0x10800 && c <= 0x10fff) ||
        (c >= 0x1e800 && c <= 0x1efff);
}

/* Builds a valid-UTF-8 copy with control characters (C0, DEL and C1) as spaces;
 * untrusted input also loses invisible characters (see is_invisible_format) and has
 * whitespace runs collapsed. */
static char *normalize(const char *page_title, bool trusted)
{
    const char *end = memchr(page_title, '\0', max_input_bytes);
    size_t length = end ? (size_t)(end - page_title) : max_input_bytes;
    /* A cut inside a multi-byte character would become a visible U+FFFD; stop before it. */
    while (!end && length > 0 && ((unsigned char)page_title[length] & 0xc0) == 0x80)
        length--;
    g_autofree char *valid = g_utf8_make_valid(page_title, (gssize)length);
    GString *out = g_string_sized_new(strlen(valid));
    bool previous_space = false;
    for (const char *p = valid; *p; p = g_utf8_next_char(p)) {
        gunichar c = g_utf8_get_char(p);
        if (!trusted && is_invisible_format(c))
            continue;
        bool space = g_unichar_iscntrl(c) || (!trusted && g_unichar_isspace(c));
        if (space && !trusted && previous_space)
            continue;
        g_string_append_unichar(out, space ? ' ' : c);
        previous_space = space;
    }
    return g_string_free(out, FALSE);
}

/*
 * A title whose first strong character is right-to-left makes the whole line
 * right-to-left in renderers that pick the direction from the text (Pango's
 * default), which moves " — foob" to the visual start. An untrusted title that
 * may contain such a character therefore starts with U+200E LEFT-TO-RIGHT MARK;
 * the page text itself still displays right to left.
 */
static const char *direction_prefix(const char *clean, bool trusted)
{
    if (trusted)
        return "";
    for (const char *p = clean; *p; p = g_utf8_next_char(p)) {
        if (may_be_right_to_left(g_utf8_get_char(p)))
            return "\u200e";
    }
    return "";
}

char *foob_window_title(const char *page_title, bool trusted)
{
    if (!page_title)
        return g_strdup("foob");
    g_autofree char *clean = normalize(page_title, trusted);
    g_strstrip(clean);
    if (!*clean)
        return g_strdup("foob");
    glong limit = trusted ? trusted_max_chars : untrusted_max_chars;
    if (g_utf8_strlen(clean, -1) > limit) {
        g_autofree char *head = g_utf8_substring(clean, 0, limit);
        g_strchomp(head);  /* A cut after a space must not leave "abc … — foob". */
        return g_strconcat(direction_prefix(head, trusted), head, "… — foob", nullptr);
    }
    return g_strconcat(direction_prefix(clean, trusted), clean, " — foob", nullptr);
}
