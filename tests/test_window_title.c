/* Unit test for the page-title to window-title mapping; needs only GLib. */
#include <string.h>
#include <glib.h>
#include "window_title.h"

static void check(const char *page_title, bool trusted, const char *expected)
{
    g_autofree char *title = foob_window_title(page_title, trusted);
    g_assert_cmpstr(title, ==, expected);
}

/* Bidi controls are written as separate UTF-8 byte strings so this file itself
 * holds no hidden bidirectional characters (compilers warn about them). */
#define RLO "\xe2\x80\xae"  /* U+202E right-to-left override */
#define PDF "\xe2\x80\xac"  /* U+202C pop directional formatting */
#define LRI "\xe2\x81\xa6"  /* U+2066 left-to-right isolate */
#define ZWJ "\xe2\x80\x8d"  /* U+200D zero width joiner */
#define RLM "\xe2\x80\x8f"  /* U+200F right-to-left mark */

/* Same expectation for untrusted (remote) and trusted (local) content. */
static void check_both(const char *page_title, const char *expected)
{
    check(page_title, false, expected);
    check(page_title, true, expected);
}

static void test_plain(void)
{
    check_both("Mathematics - Wikipedia", "Mathematics - Wikipedia — foob");
    check_both("attention is all you need | Semantic Scholar",
               "attention is all you need | Semantic Scholar — foob");
    check_both("数学 – Wikipedia", "数学 – Wikipedia — foob");
}

static void test_fallback(void)
{
    check_both(nullptr, "foob");
    check_both("", "foob");
    check_both("   \t\n ", "foob");
    check_both("\x01\x02\x7f", "foob");
}

static void test_protocol_requirements(void)
{
    check_both("  padded  ", "padded — foob");
    check_both("tab\tonly", "tab only — foob");
    check_both("line one\nline two", "line one line two — foob");
    for (int trusted = 0; trusted < 2; trusted++) {
        g_autofree char *repaired = foob_window_title("bad \xff byte", trusted);
        g_assert_true(g_utf8_validate(repaired, -1, nullptr));
        g_assert_true(g_str_has_suffix(repaired, " — foob"));
    }
}

static void test_trust_levels(void)
{
    /* Remote: whitespace runs collapse and bidi overrides cannot reorder the suffix. */
    check("a   b \r\n\t c", false, "a b c — foob");
    check("Pay" RLO "evil" LRI "x" RLM, false, "Payevilx \u2014 foob");
    check(RLO PDF "  ", false, "foob");
    /* Local: text is kept as written apart from protocol requirements. */
    check("a   b", true, "a   b — foob");
    check("Pay" RLO "evil", true, "Pay" RLO "evil \u2014 foob");
    /* Joiners are not bidi controls: emoji sequences stay intact. */
    check("\U0001F468" ZWJ "\U0001F4BB", false, "\U0001F468" ZWJ "\U0001F4BB \u2014 foob");
}

static void test_long_title(void)
{
    g_autofree char *ascii = g_strnfill(1024 * 1024, 'a');
    g_autofree char *remote = foob_window_title(ascii, false);
    g_assert_true(g_utf8_validate(remote, -1, nullptr));
    g_assert_cmpuint(strlen(remote), <, 200);
    g_assert_true(g_str_has_suffix(remote, "… — foob"));
    g_autofree char *local = foob_window_title(ascii, true);
    g_assert_cmpuint(strlen(local), >, strlen(remote));
    g_assert_cmpuint(strlen(local), <, 300);

    /* Multi-byte characters must be cut on a character boundary. */
    GString *wide = g_string_new(nullptr);
    for (int i = 0; i < 5000; i++)
        g_string_append(wide, "\U0001F600");
    for (int trusted = 0; trusted < 2; trusted++) {
        g_autofree char *emoji = foob_window_title(wide->str, trusted);
        g_assert_true(g_utf8_validate(emoji, -1, nullptr));
        g_assert_cmpuint(strlen(emoji), <, 1100);
    }
    g_string_free(wide, TRUE);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, nullptr);
    g_test_add_func("/window-title/plain", test_plain);
    g_test_add_func("/window-title/fallback", test_fallback);
    g_test_add_func("/window-title/protocol-requirements", test_protocol_requirements);
    g_test_add_func("/window-title/trust-levels", test_trust_levels);
    g_test_add_func("/window-title/long-title", test_long_title);
    return g_test_run();
}
