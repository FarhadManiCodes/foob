#include <string.h>
#include <arpa/inet.h>
#include <glib.h>
#include "target.h"

/* GLib's parser tolerates raw spaces and controls; a valid URI has neither. A backslash
 * is not valid in a URI either, and WebKit's parser (WHATWG) reads it as a slash in
 * http(s) and file URLs while GLib's does not, so the two could disagree about the host
 * or path; refuse it instead of guessing. */
static bool has_unsafe_character(const char *target)
{
    for (const char *c = target; *c; c++) {
        if ((unsigned char)*c <= ' ' || *c == 0x7f || *c == '\\')
            return true;
    }
    return false;
}

/* Four decimal parts of 0 to 255 without leading zeros, the only IPv4 form WebKit
 * writes back unchanged. */
static bool is_dotted_quad(const char *host)
{
    int parts = 0;
    for (const char *c = host;; c++) {
        const char *start = c;
        int value = 0;
        while (g_ascii_isdigit(*c) && c - start < 3)
            value = value * 10 + (*c++ - '0');
        if (c == start || (*start == '0' && c - start > 1) || value > 255)
            return false;
        parts++;
        if (!*c)
            return parts == 4;
        if (*c != '.')
            return false;
    }
}

/* WebKit reads a host whose last label is a number (decimal, or hex after "0x") as an
 * IPv4 address: 1.2.3 becomes 1.2.0.3, 0x7f.0.0.1 becomes 127.0.0.1. The host is
 * lower-case here. */
static bool ends_in_number(const char *host)
{
    size_t end = strlen(host);
    if (end && host[end - 1] == '.')
        end--;  /* WebKit skips one trailing dot here. */
    size_t start = end;
    while (start && host[start - 1] != '.')
        start--;
    bool hex = end - start >= 2 && host[start] == '0' && host[start + 1] == 'x';
    for (size_t i = start + (hex ? 2 : 0); i < end; i++) {
        if (!(hex ? g_ascii_isxdigit(host[i]) : g_ascii_isdigit(host[i])))
            return false;
    }
    return end > start;
}

/* The lower-cased host of an http(s) URI if WebKit reads it as written, else nullptr:
 * a name of ASCII letters, digits, '-' and '_' in non-empty labels (one trailing dot
 * allowed; an internationalized name must be given in punycode), a dotted-quad IPv4
 * address, or an IPv6 address in its shortest form, as inet_ntop() writes it. */
static char *canonical_host(const char *host)
{
    if (!host || !*host)
        return nullptr;
    g_autofree char *lower = g_ascii_strdown(host, -1);
    if (strchr(lower, ':')) {  /* IPv6; GLib has removed the brackets. */
        unsigned char address[16];
        char shortest[INET6_ADDRSTRLEN];
        bool same = inet_pton(AF_INET6, lower, address) == 1 &&
            inet_ntop(AF_INET6, address, shortest, sizeof shortest) &&
            strcmp(shortest, lower) == 0 && !strchr(lower, '.');
        return same ? g_steal_pointer(&lower) : nullptr;
    }
    for (const char *c = lower; *c; c++) {
        bool label_start = c == lower || c[-1] == '.';
        if (*c == '.' ? label_start : !g_ascii_isalnum(*c) && *c != '-' && *c != '_')
            return nullptr;
    }
    if (ends_in_number(lower) && !is_dotted_quad(lower))
        return nullptr;
    return g_steal_pointer(&lower);
}

/* g_filename_from_uri() refuses an escaped NUL or '/', which would change the path.
 * WebKit opens only names that are valid UTF-8 (file:///tmp/%E9 fails as "not
 * supported"), so foob accepts no other (D-40). */
static bool names_a_file(const char *uri)
{
    g_autofree char *filename = g_filename_from_uri(uri, nullptr, nullptr);
    return filename && g_utf8_validate(filename, -1, nullptr);
}

/* Characters each component may keep as written; g_uri_escape_string() encodes every
 * other one except the unreserved. GLib has already encoded each non-ASCII byte; the
 * rest is what WebKit encodes in that component: ", <, >, `, ^, {, } in a path (and |,
 * which WebKit keeps but reads as a drive letter in file:///a|), ", <, >, ' in a
 * query, and ", <, >, ` in a fragment. */
static const char path_kept[] = "!$&'()*+,;=:@/[]%";
static const char query_kept[] = "!$&()*+,;=:@/?[]%`^{|}";
static const char fragment_kept[] = "!$&'()*+,;=:@/?#[]%^{|}";

static char *escape(const char *component, const char *kept)
{
    return component ? g_uri_escape_string(component, kept, FALSE) : nullptr;
}

char *foob_target_canonical(const char *uri)
{
    if (!uri)
        return nullptr;
    if (strcmp(uri, "about:blank") == 0)
        return g_strdup(uri);
    if (!g_utf8_validate(uri, -1, nullptr) || has_unsafe_character(uri))
        return nullptr;
    /* Keeps escapes, encodes each non-ASCII byte and resolves dot segments (%2e too), as
     * WebKit does. Unlike WebKit, GLib also upper-cases escapes and decodes unreserved
     * ones (%7e becomes ~): equivalent addresses, accepted (D-43). The host is kept as
     * written, so the checks below see what WebKit will see. */
    g_autoptr(GUri) parsed = g_uri_parse(uri, G_URI_FLAGS_ENCODED | G_URI_FLAGS_NON_DNS, nullptr);
    if (!parsed || g_uri_get_userinfo(parsed))
        return nullptr;
    const char *scheme = g_uri_get_scheme(parsed);
    const char *path = g_uri_get_path(parsed);
    int port = g_uri_get_port(parsed);
    g_autofree char *host = nullptr;
    bool file = strcmp(scheme, "file") == 0;
    if (file) {
        const char *file_host = g_uri_get_host(parsed);
        if ((file_host && *file_host && g_ascii_strcasecmp(file_host, "localhost") != 0) ||
            port != -1 || path[0] != '/' ||
            /* A path that resolves to "//…" (GLib then writes "/.//…") is one WebKit
             * rewrites again; no one needs it. */
            g_str_has_prefix(path, "//") || g_str_has_prefix(path, "/.//"))
            return nullptr;
        host = g_strdup("");  /* "file:///path", as WebKit writes it. */
    } else if (strcmp(scheme, "http") == 0 || strcmp(scheme, "https") == 0) {
        host = canonical_host(g_uri_get_host(parsed));
        if (!host)
            return nullptr;
        if (port == (strcmp(scheme, "http") == 0 ? 80 : 443))
            port = -1;
        if (!*path)
            path = "/";
    } else {
        return nullptr;
    }
    g_autofree char *escaped_path = escape(path, path_kept);
    g_autofree char *query = escape(g_uri_get_query(parsed), query_kept);
    g_autofree char *fragment = escape(g_uri_get_fragment(parsed), fragment_kept);
    g_autofree char *canonical = g_uri_join(G_URI_FLAGS_ENCODED, scheme, nullptr, host, port,
                                            escaped_path, query, fragment);
    if (file && !names_a_file(canonical))
        return nullptr;
    return g_steal_pointer(&canonical);
}

char *foob_target_from_argument(const char *argument)
{
    if (!g_str_has_prefix(argument, "/") && !g_str_has_prefix(argument, "./") &&
        !g_str_has_prefix(argument, "../"))
        return foob_target_canonical(argument);
    g_autofree char *path = g_canonicalize_filename(argument, nullptr);
    g_autofree char *uri = g_filename_to_uri(path, nullptr, nullptr);
    return foob_target_canonical(uri);
}

bool foob_uri_is_local(const char *uri)
{
    g_autofree char *canonical = foob_target_canonical(uri);
    return canonical &&
        (strcmp(canonical, "about:blank") == 0 || g_str_has_prefix(canonical, "file:"));
}

bool foob_navigation_allowed(const char *from_uri, const char *to_uri)
{
    g_autofree char *destination = foob_target_canonical(to_uri);
    if (!destination)
        return false;
    return foob_uri_is_local(from_uri) || !foob_uri_is_local(destination);
}
