/* Fuzz target for the startup-URI allowlist and the trust decision (D-46). Each
 * check is a property that must hold for every input; WebKit's own parser is the
 * reference for what an address means. */
#include <stdint.h>
#include <string.h>
#include <glib.h>
#include <wpe/webkit.h>
#include "target.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

/* WebKit's serialization (raw bytes read as Latin-1, so only ASCII input is compared). */
static char *webkit_form(const char *uri)
{
    g_autoptr(WebKitURIRequest) request = webkit_uri_request_new(uri);
    const char *serialized = webkit_uri_request_get_uri(request);
    g_assert(serialized);
    return g_strdup(serialized);
}

/* WebKit's API returns text it cannot parse unchanged, and a parsed URL with its scheme
 * in lower case; so the text with an upper-case scheme comes back changed only if WebKit
 * parsed it. */
[[gnu::nonnull]] static bool webkit_parses(const char *uri)
{
    g_autofree char *upper = g_strdup(uri);
    for (char *c = upper; *c && *c != ':'; c++)
        *c = g_ascii_toupper(*c);
    g_autofree char *webkit = webkit_form(upper);
    return strcmp(webkit, upper) != 0;
}

/* Trusted only where WebKit itself reads a host-less file: URL (or about:blank). */
static bool webkit_reads_local(const char *serialized)
{
    return strcmp(serialized, "about:blank") == 0 || g_str_has_prefix(serialized, "file:///");
}

/* A host whose last label is a decimal or 0x number, which WebKit reads as an IPv4
 * address (0x7f.1 becomes 127.0.0.1): the one canonical form WebKit may rewrite (D-45). */
static bool host_ends_in_number(const char *uri)
{
    g_autoptr(GUri) parsed = g_uri_parse(uri, G_URI_FLAGS_ENCODED, nullptr);
    const char *parsed_host = parsed ? g_uri_get_host(parsed) : nullptr;
    g_autofree char *host = g_strdup(parsed_host ? parsed_host : "");
    size_t length = strlen(host);
    if (length && host[length - 1] == '.')
        host[length - 1] = '\0';
    const char *label = strrchr(host, '.') ? strrchr(host, '.') + 1 : host;
    bool hex = label[0] == '0' && (label[1] == 'x' || label[1] == 'X');
    const char *digit = hex ? label + 2 : label;
    if (!hex && !*digit)
        return false;
    for (; *digit; digit++) {
        if (hex ? !g_ascii_isxdigit(*digit) : !g_ascii_isdigit(*digit))
            return false;
    }
    return true;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    g_autofree char *input = g_strndup((const char *)data, size);

    /* The form foob loads: ASCII with no space or control, a fixed point, kept unchanged
     * by WebKit's parser (but for a numeric host, which stays remote) and local exactly
     * when it is file: or about:blank (M2-02). */
    g_autofree char *canonical = foob_target_canonical(input);
    if (canonical) {
        for (const char *c = canonical; *c; c++)
            g_assert_true((unsigned char)*c > ' ' && (unsigned char)*c < 0x7f);
        g_autofree char *again = foob_target_canonical(canonical);
        g_assert_true(again && strcmp(again, canonical) == 0);
        g_autofree char *webkit = webkit_form(canonical);
        g_assert_true(strcmp(webkit, canonical) == 0 ||
                      (host_ends_in_number(canonical) && !foob_uri_is_local(webkit)));
        g_assert_true(foob_uri_is_local(canonical) ==
                      (g_str_has_prefix(canonical, "file:") || strcmp(canonical, "about:blank") == 0));
    }

    /* Any text WebKit may report or a page may supply (D-45): foob never trusts what
     * WebKit reads as remote, whether foob sees the text or WebKit's form of it. Text
     * WebKit cannot parse (file://localhost:/a) is no address, so either answer is safe. */
    if (g_str_is_ascii(input) && webkit_parses(input)) {
        g_autofree char *webkit = webkit_form(input);
        g_assert_true(!foob_uri_is_local(input) || webkit_reads_local(webkit));
        g_assert_true(!foob_uri_is_local(webkit) || webkit_reads_local(webkit));
    }

    /* A path argument (D-32) is a local file: address or nothing. */
    if (input[0] == '/') {
        g_autofree char *path = foob_target_from_argument(input);
        g_assert_true(!path || (g_str_has_prefix(path, "file:///") && foob_uri_is_local(path)));
    }
    return 0;
}
