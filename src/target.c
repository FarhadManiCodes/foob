#include <stdint.h>
#include <string.h>
#include <glib.h>
#include "target.h"

static bool is_local_file_uri(const char *target)
{
    g_autofree char *hostname = nullptr;
    g_autofree char *filename = g_filename_from_uri(target, &hostname, nullptr);
    return filename && g_path_is_absolute(filename) &&
        (!hostname || !*hostname || g_ascii_strcasecmp(hostname, "localhost") == 0);
}

/* GLib's parser tolerates raw spaces and controls; a valid URI has neither. */
static bool has_space_or_control(const char *target)
{
    for (const char *c = target; *c; c++) {
        if ((unsigned char)*c <= ' ' || *c == 0x7f)
            return true;
    }
    return false;
}

bool foob_target_is_valid(const char *target)
{
    if (strcmp(target, "about:blank") == 0)
        return true;
    if (has_space_or_control(target))
        return false;
    g_autoptr(GUri) uri = g_uri_parse(target, G_URI_FLAGS_NONE, nullptr);
    if (!uri)
        return false;
    const char *scheme = g_uri_get_scheme(uri);
    if (g_ascii_strcasecmp(scheme, "http") == 0 ||
        g_ascii_strcasecmp(scheme, "https") == 0) {
        const char *host = g_uri_get_host(uri);
        return host && *host && !g_uri_get_userinfo(uri) && g_uri_get_port(uri) <= UINT16_MAX;
    }
    return g_ascii_strcasecmp(scheme, "file") == 0 && is_local_file_uri(target);
}
