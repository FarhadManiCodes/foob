#include <string.h>
#include <glib.h>
#include "window_title.h"

/* Keeps one Wayland string argument far below the protocol's message size limit. */
static constexpr glong untrusted_max_chars = 120;
static constexpr glong trusted_max_chars = 240;

static bool is_bidi_control(gunichar c)
{
    return c == 0x061c || c == 0x200e || c == 0x200f ||
        (c >= 0x202a && c <= 0x202e) || (c >= 0x2066 && c <= 0x2069);
}

/* Builds a valid-UTF-8 copy with control characters as spaces; untrusted input
 * also loses bidi controls and has whitespace runs collapsed. */
static char *normalize(const char *page_title, bool trusted)
{
    g_autofree char *valid = g_utf8_make_valid(page_title, -1);
    GString *out = g_string_sized_new(strlen(valid));
    bool previous_space = false;
    for (const char *p = valid; *p; p = g_utf8_next_char(p)) {
        gunichar c = g_utf8_get_char(p);
        if (!trusted && is_bidi_control(c))
            continue;
        bool space = c < ' ' || c == 0x7f || (!trusted && g_unichar_isspace(c));
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
