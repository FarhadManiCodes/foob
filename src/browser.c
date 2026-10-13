#include <signal.h>
#include <stddef.h>
#include <unistd.h>
#include <glib-unix.h>
#include <wpe/wayland/wpe-wayland.h>
#include "browser.h"
#include "diagnostics.h"
#include "exit_status.h"

/*
 * The WebKitWebView owns its WPEView, WPEDisplay and WebKitNetworkSession, so
 * the browser keeps only the view; this matches upstream's MiniBrowser, which
 * releases its own session and display references once the view exists.
 */
struct FoobBrowser {
    GMainLoop *loop;
    WebKitWebView *web_view;
    FoobDiagnostics *diagnostics;  /* nullptr unless diagnostics are enabled. */
};

static void stop_loop(FoobBrowser *browser)
{
    g_main_loop_quit(browser->loop);
}

/* Compositor-initiated close (window close request). */
static void view_closed([[maybe_unused]] WPEView *view, gpointer user_data)
{
    stop_loop(user_data);
}

/* Page-initiated close, e.g. window.close(); the only view means the whole app. */
static void web_view_close([[maybe_unused]] WebKitWebView *web_view, gpointer user_data)
{
    stop_loop(user_data);
}

/*
 * The compositor connection is gone (compositor exited or crashed). Without
 * this handler the process keeps spinning on the dead socket. An orderly
 * teardown would talk to the lost display, so exit immediately as upstream's
 * MiniBrowser does; the engine's helper processes exit when their IPC peer
 * disappears.
 */
static void display_disconnected([[maybe_unused]] WPEDisplay *display,
                                 [[maybe_unused]] GError *error, gpointer user_data)
{
    FoobBrowser *browser = user_data;
    foob_diagnostics_log(browser->diagnostics, "display-disconnected");
    _exit(FOOB_EXIT_DISPLAY_LOST);
}

static gboolean unix_signal(gpointer user_data)
{
    FoobBrowser *browser = user_data;
    foob_diagnostics_log(browser->diagnostics, "shutdown signal");
    stop_loop(browser);
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

    WPEView *view = webkit_web_view_get_wpe_view(browser->web_view);
    wpe_toplevel_set_title(wpe_view_get_toplevel(view), "foob");
    g_signal_connect(view, "closed", G_CALLBACK(view_closed), browser);
    g_signal_connect(browser->web_view, "close", G_CALLBACK(web_view_close), browser);
    g_signal_connect(display, "disconnected", G_CALLBACK(display_disconnected), browser);
    if (diagnostics) {
        browser->diagnostics = foob_diagnostics_new(browser->web_view, view);
        foob_diagnostics_log(browser->diagnostics, "backend=%s session=ephemeral",
                             G_OBJECT_TYPE_NAME(display));
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
    g_signal_handlers_disconnect_by_data(webkit_web_view_get_wpe_view(browser->web_view), browser);
    g_signal_handlers_disconnect_by_data(webkit_web_view_get_display(browser->web_view), browser);
    g_signal_handlers_disconnect_by_data(browser->web_view, browser);
    webkit_web_view_stop_loading(browser->web_view);
    g_object_unref(browser->web_view);
    g_main_loop_unref(browser->loop);
    g_free(browser);
}
