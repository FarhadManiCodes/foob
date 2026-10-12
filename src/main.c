#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <glib-unix.h>
#include <wpe/webkit.h>
#include <wpe/wpe-platform.h>
#include <wpe/wayland/wpe-wayland.h>

#if __STDC_VERSION__ < 202311L
#error foob requires C23
#endif

typedef struct {
    GMainLoop *loop;             /* Owned; callbacks run in this context. */
    WebKitWebView *web_view;     /* Owned; owns the borrowed WPE view. */
    WPEView *view;               /* Borrowed until web_view is released. */
    bool diagnostics;
    bool load_failed;
    bool stopping;
    gint64 navigation_started;
    guint geometry_source;
} Browser;

/* Startup validation for trusted M1 pages, not an M2 navigation policy. */
static bool valid_target(const char *target)
{
    if (strcmp(target, "about:blank") == 0)
        return true;
    g_autoptr(GUri) uri = g_uri_parse(target, G_URI_FLAGS_NONE, NULL);
    if (!uri)
        return false;
    const char *scheme = g_uri_get_scheme(uri);
    if (!scheme)
        return false;
    if (g_ascii_strcasecmp(scheme, "http") == 0 ||
        g_ascii_strcasecmp(scheme, "https") == 0) {
        const char *host = g_uri_get_host(uri);
        return host && *host && !g_uri_get_userinfo(uri);
    }
    if (g_ascii_strcasecmp(scheme, "file") != 0)
        return false;
    g_autofree char *hostname = NULL;
    g_autofree char *filename = g_filename_from_uri(target, &hostname, NULL);
    return filename && g_path_is_absolute(filename) &&
        (!hostname || !*hostname || g_ascii_strcasecmp(hostname, "localhost") == 0);
}

static void report_geometry(Browser *browser)
{
    if (!browser->diagnostics)
        return;
    WPEToplevelState state = wpe_view_get_toplevel_state(browser->view);
    g_printerr("foob: viewport width=%d height=%d scale=%.3f "
               "active=%d fullscreen=%d maximized=%d focus=%d\n",
               wpe_view_get_width(browser->view),
               wpe_view_get_height(browser->view),
               wpe_view_get_scale(browser->view),
               !!(state & WPE_TOPLEVEL_STATE_ACTIVE),
               !!(state & WPE_TOPLEVEL_STATE_FULLSCREEN),
               !!(state & WPE_TOPLEVEL_STATE_MAXIMIZED),
               wpe_view_get_has_focus(browser->view));
}

static gboolean geometry_idle(gpointer user_data)
{
    Browser *browser = user_data;
    browser->geometry_source = 0;
    report_geometry(browser);
    return G_SOURCE_REMOVE;
}

static void geometry_changed(GObject *object, GParamSpec *property, gpointer user_data)
{
    (void)object;
    (void)property;
    Browser *browser = user_data;
    if (!browser->stopping && !browser->geometry_source)
        browser->geometry_source = g_idle_add(geometry_idle, browser);
}

static void state_changed(WPEView *view, WPEToplevelState previous, gpointer user_data)
{
    (void)view;
    (void)previous;
    Browser *browser = user_data;
    if (!browser->stopping && !browser->geometry_source)
        browser->geometry_source = g_idle_add(geometry_idle, browser);
}

static void stop_browser(Browser *browser)
{
    browser->stopping = true;
    g_main_loop_quit(browser->loop);
}

static void view_closed(WPEView *view, gpointer user_data)
{
    (void)view;
    Browser *browser = user_data;
    if (browser->diagnostics)
        g_printerr("foob: closed\n");
    stop_browser(browser);
}

static gboolean unix_signal(gpointer user_data)
{
    Browser *browser = user_data;
    if (browser->diagnostics)
        g_printerr("foob: shutdown signal\n");
    stop_browser(browser);
    /* Keep IDs valid until main removes both signal sources. */
    return G_SOURCE_CONTINUE;
}

static void load_changed(WebKitWebView *web_view, WebKitLoadEvent event,
                         gpointer user_data)
{
    (void)web_view;
    Browser *browser = user_data;
    if (event == WEBKIT_LOAD_STARTED) {
        browser->navigation_started = g_get_monotonic_time();
        browser->load_failed = false;
        g_printerr("foob: load-started\n");
    } else if (event == WEBKIT_LOAD_FINISHED) {
        double elapsed = (g_get_monotonic_time() - browser->navigation_started) / 1000.0;
        g_printerr("foob: load-finished status=%s elapsed_ms=%.3f\n",
                   browser->load_failed ? "failed" : "ok", elapsed);
        report_geometry(browser);
    }
}

static gboolean load_failed(WebKitWebView *web_view, WebKitLoadEvent event,
                            const char *uri, GError *error, gpointer user_data)
{
    (void)web_view;
    (void)uri; /* Error messages/URIs can contain private queries. */
    Browser *browser = user_data;
    browser->load_failed = true;
    g_printerr("foob: load-failed phase=%d domain=%s code=%d\n",
               event, g_quark_to_string(error->domain), error->code);
    return FALSE; /* Preserve default error handling and TLS policy. */
}

static void process_terminated(WebKitWebView *web_view,
                               WebKitWebProcessTerminationReason reason,
                               gpointer user_data)
{
    (void)web_view;
    (void)user_data;
    g_printerr("foob: web-process-terminated reason=%d\n", reason);
}

int main(int argc, char *argv[])
{
    const char *target = argc == 2 ? argv[1] : "about:blank";
    if (argc > 2 || !valid_target(target)) {
        fputs("Usage: foob [http(s)://host/... | file:///absolute/path | about:blank]\n",
              stderr);
        return 2;
    }
    g_autoptr(GError) error = NULL;
    g_autoptr(WPEDisplay) display = wpe_display_wayland_new();
    if (!wpe_display_connect(display, &error)) {
        fputs("foob: could not connect to native Wayland\n", stderr);
        return 1;
    }
    g_autoptr(WebKitNetworkSession) session = webkit_network_session_new_ephemeral();
    Browser browser = {
        .loop = g_main_loop_new(NULL, FALSE),
        .web_view = g_object_new(WEBKIT_TYPE_WEB_VIEW,
                                "display", display,
                                "network-session", session, NULL),
        .diagnostics = g_strcmp0(g_getenv("FOOB_DIAGNOSTICS"), "1") == 0,
    };
    browser.view = webkit_web_view_get_wpe_view(browser.web_view);
    WPEToplevel *toplevel = wpe_view_get_toplevel(browser.view); /* Borrowed. */
    wpe_toplevel_set_title(toplevel, "foob");
    g_signal_connect(browser.view, "closed", G_CALLBACK(view_closed), &browser);
    if (browser.diagnostics) {
        g_printerr("foob: backend=%s session=ephemeral\n", G_OBJECT_TYPE_NAME(display));
        const char *properties[] = { "width", "height", "scale", "has-focus" };
        for (size_t i = 0; i < G_N_ELEMENTS(properties); i++) {
            g_autofree char *name = g_strconcat("notify::", properties[i], NULL);
            g_signal_connect(browser.view, name, G_CALLBACK(geometry_changed), &browser);
        }
        g_signal_connect(browser.view, "toplevel-state-changed", G_CALLBACK(state_changed), &browser);
        g_signal_connect(browser.web_view, "load-changed", G_CALLBACK(load_changed), &browser);
        g_signal_connect(browser.web_view, "load-failed", G_CALLBACK(load_failed), &browser);
        g_signal_connect(browser.web_view, "web-process-terminated",
                         G_CALLBACK(process_terminated), &browser);
    }
    guint interrupt_source = g_unix_signal_add(SIGINT, unix_signal, &browser);
    guint terminate_source = g_unix_signal_add(SIGTERM, unix_signal, &browser);
    webkit_web_view_load_uri(browser.web_view, target);
    g_main_loop_run(browser.loop);

    browser.stopping = true;
    g_source_remove(interrupt_source);
    g_source_remove(terminate_source);
    if (browser.geometry_source)
        g_source_remove(browser.geometry_source);
    g_signal_handlers_disconnect_by_data(browser.view, &browser);
    g_signal_handlers_disconnect_by_data(browser.web_view, &browser);
    webkit_web_view_stop_loading(browser.web_view);
    g_object_unref(browser.web_view);
    g_main_loop_unref(browser.loop);
    /* session/display auto-cleanup follows the view; no async request survives. */
    return 0;
}
