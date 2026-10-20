#include <string.h>
#include <unistd.h>
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

/* The lower-cased host of an http(s) URI, else nullptr: a name of ASCII letters,
 * digits, '-' and '_' in non-empty labels (one trailing dot allowed; an internationalized
 * name must be given in punycode, D-39), or an IPv6 address in its shortest form, as
 * inet_ntop() and WebKit write it. WebKit reads a name ending in a number (1.2.3,
 * 0x7f.1) as an IPv4 address, or refuses it with its error page if it is not a valid
 * one (256.1.1.1); either way it stays remote (D-45). */
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

/* The address from its checked parts, each component encoded as WebKit would. */
static char *join_escaped(GUri *parsed, const char *host, int port, const char *path)
{
    g_autofree char *escaped_path = escape(path, path_kept);
    g_autofree char *query = escape(g_uri_get_query(parsed), query_kept);
    g_autofree char *fragment = escape(g_uri_get_fragment(parsed), fragment_kept);
    return g_uri_join(G_URI_FLAGS_ENCODED, g_uri_get_scheme(parsed), nullptr, host, port,
                      escaped_path, query, fragment);
}

/* The local-file rule, shared by the startup allowlist and the trust decision so they
 * cannot drift apart (D-56): a file: URI with no userinfo or port, an absolute path,
 * and no host or "localhost". */
static bool is_local_file(GUri *parsed)
{
    const char *host = g_uri_get_host(parsed);
    return strcmp(g_uri_get_scheme(parsed), "file") == 0 && !g_uri_get_userinfo(parsed) &&
        g_uri_get_port(parsed) == -1 && g_uri_get_path(parsed)[0] == '/' &&
        (!host || !*host || g_ascii_strcasecmp(host, "localhost") == 0);
}

/* file: a local file, so every file: address foob loads is trusted, written
 * "file:///path" as WebKit writes it. A path that resolves to "//…" (GLib then writes
 * "/.//…") is one WebKit rewrites again; no one needs it. */
static char *canonical_file(GUri *parsed)
{
    const char *path = g_uri_get_path(parsed);
    if (!is_local_file(parsed) || g_str_has_prefix(path, "//") || g_str_has_prefix(path, "/.//"))
        return nullptr;
    g_autofree char *canonical = join_escaped(parsed, "", -1, path);
    return names_a_file(canonical) ? g_steal_pointer(&canonical) : nullptr;
}

/* http and https: a checked host, no default port, and "/" for an empty path. */
static char *canonical_web(GUri *parsed, int default_port)
{
    g_autofree char *host = canonical_host(g_uri_get_host(parsed));
    if (!host)
        return nullptr;
    int port = g_uri_get_port(parsed);
    const char *path = g_uri_get_path(parsed);
    return join_escaped(parsed, host, port == default_port ? -1 : port, *path ? path : "/");
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
    /* One function per scheme: a new one (foob:, M4-02) adds a function and a line (D-50). */
    const char *scheme = g_uri_get_scheme(parsed);
    if (strcmp(scheme, "file") == 0)
        return canonical_file(parsed);
    if (strcmp(scheme, "http") == 0)
        return canonical_web(parsed, 80);
    if (strcmp(scheme, "https") == 0)
        return canonical_web(parsed, 443);
    return nullptr;
}

char *foob_target_from_argument(const char *argument)
{
    bool absolute = g_str_has_prefix(argument, "/");
    if (!absolute && !g_str_has_prefix(argument, "./") && !g_str_has_prefix(argument, "../"))
        return foob_target_canonical(argument);
    /* The kernel's current directory, as ls and tab completion see it (D-44); GLib's
     * g_get_current_dir() would prefer $PWD and fall back to "/" when it is gone. */
    g_autofree char *cwd = absolute ? nullptr : getcwd(nullptr, 0);
    if (!absolute && !cwd)
        return nullptr;
    g_autofree char *path = g_canonicalize_filename(argument, cwd);
    /* GLib keeps exactly two leading slashes (POSIX) and collapses more; the kernel reads "/". */
    const char *single = path[0] == '/' && path[1] == '/' ? path + 1 : path;
    g_autofree char *uri = g_filename_to_uri(single, nullptr, nullptr);
    return foob_target_canonical(uri);
}

/* Trust needs only the scheme and the host, so this does not apply the startup rules:
 * WebKit reports addresses they would refuse, such as a local page's own fragment with
 * a backslash or "%zz" (D-45). The query and fragment never name another file. */
bool foob_uri_is_local(const char *uri)
{
    if (!uri)
        return false;
    if (strcmp(uri, "about:blank") == 0)
        return true;
    g_autofree char *resource = g_strndup(uri, strcspn(uri, "?#"));
    if (has_unsafe_character(resource))  /* WebKit reads file:/\\h/x as host h. */
        return false;
    g_autoptr(GUri) parsed = g_uri_parse(resource, G_URI_FLAGS_ENCODED | G_URI_FLAGS_NON_DNS, nullptr);
    return parsed && is_local_file(parsed);
}

bool foob_navigation_allowed(const char *from_uri, const char *to_uri)
{
    g_autofree char *destination = foob_target_canonical(to_uri);
    return destination && (foob_uri_is_local(from_uri) || !foob_uri_is_local(destination));
}
