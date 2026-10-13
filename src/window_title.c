#include <string.h>
#include <glib.h>
#include "window_title.h"

/* Keeps one Wayland string argument far below the protocol's message size limit. */
static constexpr glong untrusted_max_chars = 120;
static constexpr glong trusted_max_chars = 240;

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
 * the ones above, plus the invisible characters outside Cf: combining grapheme joiner,
 * Hangul and halfwidth fillers, Khmer inherent vowels, Braille blank, object
 * replacement character and variation selector supplement. */
static bool is_invisible_format(gunichar c)
{
    if (g_unichar_type(c) == G_UNICODE_FORMAT)
        return !is_visible_or_needed_format(c);
    return c == 0x034f || c == 0x115f || c == 0x1160 || c == 0x17b4 || c == 0x17b5 ||
        c == 0x2800 || c == 0x3164 || c == 0xffa0 || c == 0xfffc || (c >= 0xe0000 && c <= 0xe01ef);
}

/* Builds a valid-UTF-8 copy with control characters (C0, DEL and C1) as spaces;
 * untrusted input also loses invisible characters (see is_invisible_format) and has
 * whitespace runs collapsed. */
static char *normalize(const char *page_title, bool trusted)
{
    g_autofree char *valid = g_utf8_make_valid(page_title, -1);
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
        return g_strconcat(head, "… — foob", nullptr);
    }
    return g_strconcat(clean, " — foob", nullptr);
}
