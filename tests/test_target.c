/* Unit test for the startup-URI allowlist; needs no display. The module needs only
 * GLib; this test also links WebKit to check that WebKit's own parser agrees. */
#include <glib.h>
#include <wpe/webkit.h>
#include "target.h"

static const struct { const char *uri; const char *canonical; } accepted[] = {
    { "about:blank", "about:blank" },
    { "https://example.org/", "https://example.org/" },
    { "http://example.org", "http://example.org/" },
    { "HTTPS://EXAMPLE.org/Path?q=1#frag", "https://example.org/Path?q=1#frag" },
    { "https://example.org:8443/", "https://example.org:8443/" },
    { "https://example.org:443/", "https://example.org/" },
    { "http://example.org:080/", "http://example.org/" },
    { "https://ex%41mple.org./", "https://example.org./" },
    { "http://127.0.0.1:8080/", "http://127.0.0.1:8080/" },
    { "http://[::1]/", "http://[::1]/" },
    { "http://[2001:db8::1]/", "http://[2001:db8::1]/" },
    { "file:///tmp/a.html", "file:///tmp/a.html" },
    { "file:///tmp/a%20b.html", "file:///tmp/a%20b.html" },
    { "file://localhost/tmp/a.html", "file:///tmp/a.html" },
    { "FILE:/tmp/a.html", "file:///tmp/a.html" },
    { "file:///tmp/caf%C3%A9%20%231%3F.html", "file:///tmp/caf%C3%A9%20%231%3F.html" },
    { "file:///tmp/a.html?x#sec", "file:///tmp/a.html?x#sec" },
    /* Dot segments resolved as WebKit does; escapes normalized by GLib, an equivalent form (D-43). */
    { "https://example.org/a/../b/./c", "https://example.org/b/c" },
    { "https://example.org/%2e%2E/a%41%e9", "https://example.org/aA%E9" },
    /* Non-ASCII and the characters WebKit encodes in each component (D-39, D-43). */
    { "https://example.org/\xe2\x80\xae\xc2\x85", "https://example.org/%E2%80%AE%C2%85" },  /* U+202E, U+0085 */
    { "https://fa.wikipedia.org/wiki/\xd9\x81?q=\xc3\xa9#\xc3\xa9",
      "https://fa.wikipedia.org/wiki/%D9%81?q=%C3%A9#%C3%A9" },
    { "https://example.org/\"<>`{|}^?'\"#\"'", "https://example.org/%22%3C%3E%60%7B%7C%7D%5E?%27%22#%22'" },
    { "https://example.org/p'q/[x];a=b@c:d?[y]#[z]#", "https://example.org/p'q/[x];a=b@c:d?[y]#[z]#" },
    { "https://example.org/a|b?q={x}|y^z`w#{x}|y^z`w", "https://example.org/a%7Cb?q={x}|y^z`w#{x}|y^z%60w" },
};

static const char *const rejected[] = {
    "", "example.org", "about:config", "about:blank#x",
    "javascript:alert(1)", "data:text/html,test", "ftp://example.org",
    "blob:https://example.org/uuid", "view-source:https://example.org/",
    "http:", "https:///", "https://", "https://:80/",
    "https://@example.org/", "https://user@example.org/", "https://user:secret@example.org/",
    "file:relative.html", "file://remote-host/tmp/a.html", "file://127.0.0.1/tmp/a.html",
    "file://localhost:80/tmp/a.html", "file:///tmp/%00", "file:///tmp/a%2Fb",
    "file:///tmp/%E9",  /* a valid Linux name that WebKit cannot open: not UTF-8 (D-40) */
    "file:/.//tmp/a.html", "file:////tmp/a.html", "FILE:/tmp/..//a",
    "https://example.org/%xx", "https://example.org:99999/",
    "https://exa mple.org/", "https://example.org/\n", "https://example.org/\xe9",
    "http://good.com\\.evil.com/", "file:/\\evil.com/x",
    /* Hosts WebKit refuses or reads differently (pre-Gate-A review). */
    "http://%20/", "https://host%2540/", "https://stra\xc3\x9f" "e.de/", "https://a..b/", "https://.a/",
    "https://1.2.3/", "https://0x7f.0.0.1/", "https://1.2.3.04/", "https://2130706433/",
    "https://1.2.3.4./", "https://256.1.1.1/", "https://example.0x1f/",
    "http://[0:0::1]/", "http://[::ffff:1.2.3.4]/", "https://[fe80::1%25eth0]/",
};

static void test_accepted(void)
{
    for (size_t i = 0; i < G_N_ELEMENTS(accepted); i++) {
        g_test_message("accepted[%zu]", i);
        g_autofree char *canonical = foob_target_canonical(accepted[i].uri);
        g_assert_cmpstr(canonical, ==, accepted[i].canonical);
        g_autofree char *again = foob_target_canonical(canonical);
        g_assert_cmpstr(again, ==, canonical);
    }
}

/* What foob loads, WebKit's own parser leaves unchanged: no second parse can change
 * the host, the path or the trust decision (M2-02). */
static void test_webkit_agrees(void)
{
    for (size_t i = 0; i < G_N_ELEMENTS(accepted); i++) {
        g_test_message("accepted[%zu]", i);
        g_autoptr(WebKitURIRequest) request = webkit_uri_request_new(accepted[i].canonical);
        g_assert_cmpstr(webkit_uri_request_get_uri(request), ==, accepted[i].canonical);
    }
}

static void test_rejected(void)
{
    for (size_t i = 0; i < G_N_ELEMENTS(rejected); i++) {
        g_test_message("rejected[%zu]", i);
        g_autofree char *canonical = foob_target_canonical(rejected[i]);
        g_assert_null(canonical);
    }
    g_assert_null(foob_target_canonical(nullptr));
}

static void test_from_argument(void)
{
    g_autofree char *cwd = g_get_current_dir();
    g_autofree char *parent = g_path_get_dirname(cwd);
    g_autofree char *here = g_build_filename(cwd, "x.html", nullptr);
    g_autofree char *above = g_build_filename(parent, "x.html", nullptr);
    g_autofree char *here_uri = g_filename_to_uri(here, nullptr, nullptr);
    g_autofree char *above_uri = g_filename_to_uri(above, nullptr, nullptr);
    const struct { const char *argument; const char *canonical; } cases[] = {
        { "/tmp/a b#?.html", "file:///tmp/a%20b%23%3F.html" },
        { "/tmp/./x/../caf\xc3\xa9.html", "file:///tmp/caf%C3%A9.html" },
        { "./x.html", here_uri },
        { "../x.html", above_uri },
        { "https://example.org", "https://example.org/" },  /* not a path: a URI */
        { "/tmp/\xe9.html", nullptr },  /* not UTF-8: WebKit cannot open it (D-40) */
        { "example.com", nullptr }, { "x.html", nullptr }, { "~/x.html", nullptr }, { "", nullptr },
    };
    for (size_t i = 0; i < G_N_ELEMENTS(cases); i++) {
        g_test_message("argument[%zu]", i);
        g_autofree char *canonical = foob_target_from_argument(cases[i].argument);
        g_assert_cmpstr(canonical, ==, cases[i].canonical);
    }
}

static void test_local_uri(void)
{
    static const char *const local[] = {
        "about:blank", "file:///tmp/a.html", "FILE:///tmp/a%20b.md",
        "file://localhost/tmp/a.typ", "file:///tmp/caf%C3%A9.html", "file:///tmp/a.html#x",
    };
    static const char *const remote[] = {
        "https://example.org/", "http://127.0.0.1/", "file://remote-host/tmp/a.html",
        "file:relative.html", "about:config", "javascript:1",
        "blob:file:///tmp/x", "",
    };
    for (size_t i = 0; i < G_N_ELEMENTS(local); i++)
        g_assert_true(foob_uri_is_local(local[i]));
    for (size_t i = 0; i < G_N_ELEMENTS(remote); i++)
        g_assert_false(foob_uri_is_local(remote[i]));
    g_assert_false(foob_uri_is_local(nullptr));
}

static void test_navigation(void)
{
    static const struct { const char *from; const char *to; bool allowed; } cases[] = {
        { "https://a.example/", "https://b.example/x", true },
        { "file:///tmp/a.html", "https://b.example/", true },
        { "file:///tmp/a.html", "file:///tmp/b.html", true },
        { "https://a.example/", "file:///etc/passwd", false },
        { "https://a.example/", "about:blank", false },
        { nullptr, "file:///tmp/b.html", false },
        { "file:///tmp/a.html", "javascript:alert(1)", false },
        { "file:///tmp/a.html", nullptr, false },
    };
    for (size_t i = 0; i < G_N_ELEMENTS(cases); i++) {
        g_test_message("navigation[%zu]", i);
        g_assert_cmpint(foob_navigation_allowed(cases[i].from, cases[i].to), ==, cases[i].allowed);
    }
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, nullptr);
    g_test_add_func("/target/accepted", test_accepted);
    g_test_add_func("/target/webkit-agrees", test_webkit_agrees);
    g_test_add_func("/target/rejected", test_rejected);
    g_test_add_func("/target/from-argument", test_from_argument);
    g_test_add_func("/target/local-uri", test_local_uri);
    g_test_add_func("/target/navigation", test_navigation);
    return g_test_run();
}
