#include <signal.h>
#include <stddef.h>
#include <unistd.h>
#include <glib-unix.h>
#include <wpe/wayland/wpe-wayland.h>
#include "browser.h"
#include "diagnostics.h"
#include "exit_status.h"
#include "target.h"
#include "window_title.h"

/*
 * The WebKitWebView owns its WPEView, WPEDisplay and WebKitNetworkSession, so
 * the browser keeps only the view; this matches upstream's MiniBrowser, which
 * releases its own session and display references once the view exists.
 */
struct FoobBrowser {
    GMainLoop *loop;
    WebKitWebView *web_view;
    FoobDiagnostics *diagnostics;  /* nullptr unless diagnostics are enabled. */
    int exit_status;               /* Result of foob_browser_run(). */
    bool stopping;                 /* A shutdown was requested; keep its exit status. */
};

/* Mirrors the page title into the compositor-visible window title. */
static void update_window_title(FoobBrowser *browser)
{
    /* Only content from this machine is trusted; any other origin is not. */
    bool local = foob_uri_is_local(webkit_web_view_get_uri(browser->web_view));
    g_autofree char *title = foob_window_title(webkit_web_view_get_title(browser->web_view), local);
    WPEToplevel *toplevel = wpe_view_get_toplevel(webkit_web_view_get_wpe_view(browser->web_view));
    if (toplevel)  /* Nullable: the view may not have a toplevel yet. */
        wpe_toplevel_set_title(toplevel, title);
}

static void title_changed([[maybe_unused]] GObject *object,
                          [[maybe_unused]] GParamSpec *property, gpointer user_data)
{
    update_window_title(user_data);
}

static void stop_loop(FoobBrowser *browser)
{
    browser->stopping = true;
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
 * Interim safety default until M3 designs download policy: no download starts
 * without an explicit user decision, and there is no UI to ask yet, so every
 * download is cancelled before a destination is chosen.
 */
static void download_started([[maybe_unused]] WebKitNetworkSession *session,
                             WebKitDownload *download, gpointer user_data)
{
    foob_diagnostics_log(((FoobBrowser *)user_data)->diagnostics, "download-cancelled");
    webkit_download_cancel(download);
}

/*
 * Links that ask for a new window (target=_blank, user-triggered window.open)
 * navigate this window instead: one view per window. Requests without a user
 * gesture stay blocked, as do destinations that fail foob_navigation_allowed().
 * Opening a separate window is a later decision (roadmap M2).
 */
static gboolean decide_policy(WebKitWebView *web_view, WebKitPolicyDecision *decision,
                              WebKitPolicyDecisionType type, [[maybe_unused]] gpointer user_data)
{
    if (type != WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION)
        return FALSE;  /* Default handling. */
    WebKitNavigationAction *action =
        webkit_navigation_policy_decision_get_navigation_action(WEBKIT_NAVIGATION_POLICY_DECISION(decision));
    const char *destination = webkit_uri_request_get_uri(webkit_navigation_action_get_request(action));
    webkit_policy_decision_ignore(decision);
    if (webkit_navigation_action_is_user_gesture(action) &&
        foob_navigation_allowed(webkit_web_view_get_uri(web_view), destination))
        webkit_web_view_load_uri(web_view, destination);
    return TRUE;
}

/*
 * The web process crashed or was killed; the page is gone and the window would
 * stay blank or frozen with no signal. Exit with a distinct status so the
 * failure is visible and the launcher can react. Reloading once is roadmap M3-07.
 */
static void web_process_terminated([[maybe_unused]] WebKitWebView *web_view,
                                   [[maybe_unused]] WebKitWebProcessTerminationReason reason,
                                   gpointer user_data)
{
    FoobBrowser *browser = user_data;
    if (!browser->stopping)  /* A requested shutdown keeps its own status. */
        browser->exit_status = FOOB_EXIT_ENGINE_LOST;
    stop_loop(browser);
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

/*
 * The engine derives the Wayland app-id from the GLib program name, but only
 * when it is a valid application ID; otherwise it uses org.webkit.app- plus a
 * hash of the executable path, which changes whenever the binary moves. A fixed
 * id lets compositor window rules match foob. Must be set before the view exists.
 */
static constexpr char app_id[] = "dev.foob.Browser";

FoobBrowser *foob_browser_new(bool diagnostics)
{
    g_set_prgname(app_id);
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
    update_window_title(browser);
    g_signal_connect(browser->web_view, "notify::title", G_CALLBACK(title_changed), browser);
    g_signal_connect(view, "closed", G_CALLBACK(view_closed), browser);
    g_signal_connect(browser->web_view, "close", G_CALLBACK(web_view_close), browser);
    g_signal_connect(display, "disconnected", G_CALLBACK(display_disconnected), browser);
    g_signal_connect(session, "download-started", G_CALLBACK(download_started), browser);
    g_signal_connect(browser->web_view, "decide-policy", G_CALLBACK(decide_policy), browser);
    g_signal_connect(browser->web_view, "web-process-terminated",
                     G_CALLBACK(web_process_terminated), browser);
    if (diagnostics) {
        browser->diagnostics = foob_diagnostics_new(browser->web_view, view);
        foob_diagnostics_log(browser->diagnostics, "backend=%s session=ephemeral",
                             G_OBJECT_TYPE_NAME(display));
    }
    return browser;
}

int foob_browser_run(FoobBrowser *browser, const char *target)
{
    guint sources[] = {
        g_unix_signal_add(SIGINT, unix_signal, browser),
        g_unix_signal_add(SIGTERM, unix_signal, browser),
    };
    webkit_web_view_load_uri(browser->web_view, target);
    g_main_loop_run(browser->loop);
    for (size_t i = 0; i < G_N_ELEMENTS(sources); i++)
        g_source_remove(sources[i]);
    return browser->exit_status;
}

void foob_browser_free(FoobBrowser *browser)
{
    if (!browser)
        return;
    foob_diagnostics_free(browser->diagnostics);
    g_signal_handlers_disconnect_by_data(webkit_web_view_get_wpe_view(browser->web_view), browser);
    g_signal_handlers_disconnect_by_data(webkit_web_view_get_display(browser->web_view), browser);
    g_signal_handlers_disconnect_by_data(webkit_web_view_get_network_session(browser->web_view), browser);
    g_signal_handlers_disconnect_by_data(browser->web_view, browser);
    webkit_web_view_stop_loading(browser->web_view);
    g_object_unref(browser->web_view);
    g_main_loop_unref(browser->loop);
    g_free(browser);
}
