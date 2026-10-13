#include <signal.h>
#include <glib-unix.h>
#include <wpe/wayland/wpe-wayland.h>
#include "browser.h"
#include "diagnostics.h"

struct FoobBrowser {
    GMainLoop *loop;
    WPEDisplay *display;
    WebKitNetworkSession *session;
    WebKitWebView *web_view;       /* Owns the borrowed WPEView. */
    FoobDiagnostics *diagnostics;  /* nullptr unless diagnostics are enabled. */
};

static void view_closed([[maybe_unused]] WPEView *view, gpointer user_data)
{
    FoobBrowser *browser = user_data;
    g_main_loop_quit(browser->loop);
}

static gboolean unix_signal(gpointer user_data)
{
    FoobBrowser *browser = user_data;
    foob_diagnostics_log(browser->diagnostics, "shutdown signal");
    g_main_loop_quit(browser->loop);
    /* Keep IDs valid until foob_browser_run() removes both signal sources. */
    return G_SOURCE_CONTINUE;
}

FoobBrowser *foob_browser_new(bool diagnostics)
{
    g_autoptr(WPEDisplay) display = wpe_display_wayland_new();
    if (!wpe_display_connect(display, nullptr))
        return nullptr;
    g_autoptr(WebKitNetworkSession) session = webkit_network_session_new_ephemeral();
    FoobBrowser *browser = g_new0(FoobBrowser, 1);
    browser->loop = g_main_loop_new(nullptr, FALSE);
    browser->web_view = g_object_new(WEBKIT_TYPE_WEB_VIEW,
                                     "display", display,
                                     "network-session", session, nullptr);
    browser->display = g_steal_pointer(&display);
    browser->session = g_steal_pointer(&session);

    WPEView *view = webkit_web_view_get_wpe_view(browser->web_view);
    wpe_toplevel_set_title(wpe_view_get_toplevel(view), "foob");
    g_signal_connect(view, "closed", G_CALLBACK(view_closed), browser);
    if (diagnostics) {
        browser->diagnostics = foob_diagnostics_new(browser->web_view, view);
        foob_diagnostics_log(browser->diagnostics, "backend=%s session=ephemeral",
                             G_OBJECT_TYPE_NAME(browser->display));
    }
    return browser;
}

void foob_browser_run(FoobBrowser *browser, const char *target)
{
    guint sources[] = {
        g_unix_signal_add(SIGINT, unix_signal, browser),
        g_unix_signal_add(SIGTERM, unix_signal, browser),
    };
    webkit_web_view_load_uri(browser->web_view, target);
    g_main_loop_run(browser->loop);
    for (size_t i = 0; i < G_N_ELEMENTS(sources); i++)
        g_source_remove(sources[i]);
}

void foob_browser_free(FoobBrowser *browser)
{
    if (!browser)
        return;
    foob_diagnostics_free(browser->diagnostics);
    g_signal_handlers_disconnect_by_data(webkit_web_view_get_wpe_view(browser->web_view),
                                         browser);
    webkit_web_view_stop_loading(browser->web_view);
    g_object_unref(browser->web_view);
    g_main_loop_unref(browser->loop);
    /* Session and display outlive the view; no async request survives it. */
    g_object_unref(browser->session);
    g_object_unref(browser->display);
    g_free(browser);
}
