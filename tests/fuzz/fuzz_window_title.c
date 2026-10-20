/* Fuzz target for the window title (D-46). The first byte picks the trust level; the
 * rest is the page title. */
#include <stdint.h>
#include <string.h>
#include <glib.h>
#include "window_title.h"

#define LRM "\xe2\x80\x8e"  /* U+200E, written as bytes: no hidden bidi character here */

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

/* Characters that reorder text: the bidi embeddings, overrides, isolates and marks. */
static bool is_bidi_control(gunichar c)
{
    return c == 0x061c || c == 0x200e || c == 0x200f || (c >= 0x202a && c <= 0x202e) ||
        (c >= 0x2066 && c <= 0x2069);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0)
        return 0;
    bool trusted = data[0] & 1;
    g_autofree char *page_title = g_strndup((const char *)data + 1, size - 1);
    g_autofree char *title = foob_window_title(page_title, trusted);

    /* What the Wayland protocol needs, and the suffix that names foob. */
    g_assert_true(g_utf8_validate(title, -1, nullptr));
    g_assert_true(strcmp(title, "foob") == 0 || g_str_has_suffix(title, " \xe2\x80\x94 foob"));
    g_assert_true(g_utf8_strlen(title, -1) <= (trusted ? 240 : 1 + 120) + 8);
    const char *start = title;
    if (!trusted && g_str_has_prefix(title, LRM))  /* the one mark foob adds */
        start = g_utf8_next_char(title);
    for (const char *p = start; *p; p = g_utf8_next_char(p)) {
        gunichar c = g_utf8_get_char(p);
        g_assert_true(!g_unichar_iscntrl(c));
        /* An untrusted title cannot reorder the suffix or pad itself with blank runs. */
        g_assert_true(trusted || !is_bidi_control(c));
        g_assert_true(trusted || !(c == ' ' && p[1] == ' '));
    }
    g_autofree char *again = foob_window_title(page_title, trusted);
    g_assert_true(strcmp(again, title) == 0);
    return 0;
}
