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
 * Links and forms that ask for a new window (target=_blank) navigate this
 * window instead: one view per window. The page makes that navigation itself:
 * this script, injected into every frame in its own script world, turns a
 * user's click on a _blank link, or a _blank form submission, into the same
 * request with target _top and then restores the attribute. The document that
 * asked stays the initiator, so WebKit applies its usual rules: the frame's
 * sandbox flags, Sec-Fetch-Site and SameSite cookies, and its refusal to let
 * remote content navigate to local content. _top grants nothing a page could
 * not already ask for. The effective target follows WebKit: a link with an
 * empty or missing target uses the document's <base target>, a form only with a
 * missing one (an empty form target or formtarget means this frame). The saved
 * attribute is restored only while it still holds _top, so a target the page's
 * own handlers set meanwhile is kept. Scripted clicks are not trusted events and
 * are left alone, so the popup blocker still stops them; other target names that
 * would open a window are not rewritten.
 */
static constexpr char same_window_script[] =
    "(() => {"
    "  const isBlank = (value) => value !== null && value.toLowerCase() === '_blank';"
    "  const effective = (value, node) => value !== null ? value :"
    "      node.ownerDocument.querySelector('base[target]')?.getAttribute('target') ?? null;"
    "  const toTop = (element, attribute) => {"
    "    const old = element.getAttribute(attribute);"
    "    element.setAttribute(attribute, '_top');"
    "    setTimeout(() => {"
    "      if (element.getAttribute(attribute) === '_top')"
    "        old === null ? element.removeAttribute(attribute) : element.setAttribute(attribute, old);"
    "    });"
    "  };"
    "  addEventListener('click', (event) => {"
    "    if (!event.isTrusted)"
    "      return;"
    "    const link = event.composedPath().find((node) =>"
    "        (node.localName === 'a' || node.localName === 'area') && node.hasAttribute('href'));"
    "    if (link && isBlank(effective(link.getAttribute('target') || null, link)))"
    "      toTop(link, 'target');"
    "  }, true);"
    "  addEventListener('submit', (event) => {"
    "    const form = event.target, submitter = event.submitter;"
    "    if (!event.isTrusted)"
    "      return;"
    "    if (submitter?.hasAttribute('formtarget')) {"
    "      if (isBlank(submitter.getAttribute('formtarget')))"
    "        toTop(submitter, 'formtarget');"
    "    } else if (isBlank(effective(form.getAttribute('target'), form))) {"
    "      toTop(form, 'target');"
    "    }"
    "  }, true);"
    "})();";

static void add_same_window_script(WebKitWebView *web_view)
{
    g_autoptr(WebKitUserScript) script = webkit_user_script_new_for_world(
        same_window_script, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, "foob", nullptr, nullptr);
    webkit_user_content_manager_add_script(webkit_web_view_get_user_content_manager(web_view), script);
}

/*
 * Every request that still asks for a new window is ignored; foob never loads
 * it on the page's behalf. Such a load is application-initiated: it would drop
 * the asking frame's sandbox (WebKit turns a sandboxed frame's refused _top
 * navigation into a new-window request), send Sec-Fetch-Site: none for a
 * cross-site request and bypass WebKit's remote-to-local check. window.open()
 * is unsupported: WebKit reports it through the "create" signal, which is not
 * handled, so it returns null (roadmap M2-11 and M3-01 decide popups). Opening a
 * separate window is likewise a later decision.
 */
static gboolean decide_policy([[maybe_unused]] WebKitWebView *web_view, WebKitPolicyDecision *decision,
                              WebKitPolicyDecisionType type, [[maybe_unused]] gpointer user_data)
{
    if (type != WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION)
        return FALSE;  /* Default handling. */
    webkit_policy_decision_ignore(decision);
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
    add_same_window_script(browser->web_view);
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
    browser->diagnostics = nullptr;
    g_signal_handlers_disconnect_by_data(webkit_web_view_get_wpe_view(browser->web_view), browser);
    g_signal_handlers_disconnect_by_data(webkit_web_view_get_display(browser->web_view), browser);
    g_signal_handlers_disconnect_by_data(webkit_web_view_get_network_session(browser->web_view), browser);
    g_signal_handlers_disconnect_by_data(browser->web_view, browser);
    webkit_web_view_stop_loading(browser->web_view);
    g_object_unref(browser->web_view);
    g_main_loop_unref(browser->loop);
    g_free(browser);
}
