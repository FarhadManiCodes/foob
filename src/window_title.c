#include <string.h>
#include <glib.h>
#include "window_title.h"

/* Keeps one Wayland string argument far below the protocol's message size limit. */
static constexpr glong untrusted_max_chars = 120;
static constexpr glong trusted_max_chars = 240;
/* A page controls its title and may set one of many megabytes on every change; only this
 * much of it is read, so the work per change stays bounded (F-76). */
static constexpr size_t max_input_bytes = 16 * 1024;

/* Inclusive ranges of code points; each list is one rule, one entry per line. */
struct range {
    gunichar first, last;
};

static bool in_ranges(gunichar c, const struct range *ranges, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        if (c >= ranges[i].first && c <= ranges[i].last)
            return true;
    }
    return false;
}

/* Format characters (Unicode category Cf) that render as visible glyphs, or that emoji
 * sequences and Persian or Indic text need. Every other Cf character is invisible. */
static const struct range visible_formats[] = {
    { 0x0600, 0x0605 },    /* Arabic number signs and marks */
    { 0x06dd, 0x06dd },    /* Arabic end of ayah */
    { 0x070f, 0x070f },    /* Syriac abbreviation mark */
    { 0x0890, 0x0891 },    /* Arabic pound and piastre marks above */
    { 0x08e2, 0x08e2 },    /* Arabic disputed end of ayah */
    { 0x200c, 0x200d },    /* zero width non-joiner and joiner */
    { 0x110bd, 0x110bd },  /* Kaithi number sign */
    { 0x110cd, 0x110cd },  /* Kaithi number sign above */
};

/* The rest of Unicode's Default_Ignorable_Code_Point set, which renderers draw as
 * nothing, and two characters that are not default-ignorable but look blank. Variation
 * selectors 1-16 (U+FE00-FE0F) stay: emoji presentation needs U+FE0F. */
static const struct range invisible_others[] = {
    { 0x034f, 0x034f },    /* combining grapheme joiner */
    { 0x115f, 0x1160 },    /* Hangul choseong and jungseong fillers */
    { 0x17b4, 0x17b5 },    /* Khmer inherent vowels */
    { 0x180b, 0x180f },    /* Mongolian free variation selectors */
    { 0x2065, 0x2065 },    /* unassigned, default-ignorable */
    { 0x2800, 0x2800 },    /* Braille pattern blank (not default-ignorable) */
    { 0x3164, 0x3164 },    /* Hangul filler */
    { 0xffa0, 0xffa0 },    /* halfwidth Hangul filler */
    { 0xfff0, 0xfff8 },    /* unassigned, default-ignorable */
    { 0xfffc, 0xfffc },    /* object replacement character (not default-ignorable) */
    { 0xe0000, 0xe0fff },  /* tags and the variation selector supplement */
};

/* Code points whose bidi class is, or by default would be, right-to-left (R or AL).
 * Unassigned code points count too: bidi algorithms give them their block's default. */
static const struct range right_to_left[] = {
    { 0x0590, 0x08ff },    /* Hebrew to Arabic Extended-A */
    { 0xfb1d, 0xfdff },    /* Hebrew and Arabic presentation forms A */
    { 0xfe70, 0xfeff },    /* Arabic presentation forms B */
    { 0x10800, 0x10fff },  /* right-to-left supplementary scripts */
    { 0x1e800, 0x1efff },  /* right-to-left supplementary scripts */
};

/* Invisible characters that can hide or reorder text, or pad it with blank-looking runs:
 * every format character (bidi controls, zero width characters, soft hyphen, tag
 * characters, ...) but the visible ones, and the others above. */
static bool is_invisible_format(gunichar c)
{
    if (g_unichar_type(c) == G_UNICODE_FORMAT)
        return !in_ranges(c, visible_formats, G_N_ELEMENTS(visible_formats));
    return in_ranges(c, invisible_others, G_N_ELEMENTS(invisible_others));
}

/* Bidi controls such as U+200F are format characters and already removed from untrusted
 * titles. */
static bool may_be_right_to_left(gunichar c)
{
    return in_ranges(c, right_to_left, G_N_ELEMENTS(right_to_left));
}

/* Builds a valid-UTF-8 copy with control characters (C0, DEL and C1) as spaces;
 * untrusted input also loses invisible characters (see is_invisible_format) and has
 * whitespace runs collapsed. */
static char *normalize(const char *page_title, bool trusted)
{
    const char *end = memchr(page_title, '\0', max_input_bytes);
    size_t length = end ? (size_t)(end - page_title) : max_input_bytes;
    /* A cut inside a multi-byte character would become a visible U+FFFD; stop before it.
     * A character has at most three continuation bytes, so back up at most three. */
    for (int back = 0; !end && back < 3 && ((unsigned char)page_title[length] & 0xc0) == 0x80; back++)
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
