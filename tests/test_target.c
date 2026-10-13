/* Unit test for the startup-URI allowlist; needs only GLib, no display. */
#include <glib.h>
#include "target.h"

static const char *const accepted[] = {
    "about:blank",
    "https://example.org/",
    "http://example.org/",
    "HTTPS://EXAMPLE.org/Path?q=1#frag",
    "https://example.org:8443/",
    "http://[::1]/",
    "http://127.0.0.1:8080/",
    "file:///tmp/a.html",
    "file:///tmp/a%20b.html",
    "file://localhost/tmp/a.html",
    "FILE:///tmp/a.html",
    "file:///",
};

static const char *const rejected[] = {
    "", " ", "example.org", "/tmp/page.html", "about:config", "about:blank#x",
    "about:BLANK", "javascript:alert(1)", "data:text/html,test", "ftp://example.org",
    "blob:https://example.org/uuid", "view-source:https://example.org/",
    "http:", "https:///", "https://", "https://:80/",
    "https://user@example.org/", "https://user:secret@example.org/",
    "https://:secret@example.org/",
    "file:relative.html", "file://remote-host/tmp/a.html", "file://127.0.0.1/tmp/a.html",
    "file:///tmp/%00", "https://example.org/%xx", "https://example.org:99999/",
    "https://exa mple.org/", "https://example.org/\n", "\nhttps://example.org/",
};

static void test_accepted(void)
{
    for (size_t i = 0; i < G_N_ELEMENTS(accepted); i++) {
        g_test_message("accepted[%zu]", i);
        g_assert_true(foob_target_is_valid(accepted[i]));
    }
}

static void test_rejected(void)
{
    for (size_t i = 0; i < G_N_ELEMENTS(rejected); i++) {
        g_test_message("rejected[%zu]", i);
        g_assert_false(foob_target_is_valid(rejected[i]));
    }
}

static void test_local_uri(void)
{
    static const char *const local[] = {
        "about:blank", "file:///tmp/a.html", "FILE:///tmp/a%20b.md",
        "file://localhost/tmp/a.typ", "file:///",
    };
    static const char *const remote[] = {
        "https://example.org/", "http://127.0.0.1/", "file://remote-host/tmp/a.html",
        "file:relative.html", "about:config", "javascript:1", "data:text/html,x",
        "blob:file:///tmp/x", "", "not a uri",
    };
    for (size_t i = 0; i < G_N_ELEMENTS(local); i++)
        g_assert_true(foob_uri_is_local(local[i]));
    for (size_t i = 0; i < G_N_ELEMENTS(remote); i++)
        g_assert_false(foob_uri_is_local(remote[i]));
    g_assert_false(foob_uri_is_local(nullptr));
}

static void test_long_input(void)
{
    g_autofree char *path = g_strnfill(1024 * 1024, 'a');
    g_autofree char *http = g_strconcat("https://example.org/", path, nullptr);
    g_autofree char *garbage = g_strnfill(1024 * 1024, ':');
    g_assert_true(foob_target_is_valid(http));
    g_assert_false(foob_target_is_valid(garbage));
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, nullptr);
    g_test_add_func("/target/accepted", test_accepted);
    g_test_add_func("/target/rejected", test_rejected);
    g_test_add_func("/target/local-uri", test_local_uri);
    g_test_add_func("/target/long-input", test_long_input);
    return g_test_run();
}
