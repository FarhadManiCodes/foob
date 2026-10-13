#include <stdarg.h>
#include "diagnostics.h"

struct FoobDiagnostics {
    WebKitWebView *web_view;     /* Borrowed. */
    WPEView *view;               /* Borrowed; owned by web_view. */
    gint64 navigation_started;
    guint geometry_source;
    bool load_failed;
    bool closed;
};

static const char *const geometry_properties[] = {
    "notify::width", "notify::height", "notify::scale", "notify::has-focus",
};

void foob_diagnostics_log(const FoobDiagnostics *diagnostics, const char *format, ...)
{
    if (!diagnostics)
        return;
    va_list arguments;
    va_start(arguments, format);
    g_autofree char *message = g_strdup_vprintf(format, arguments);
    va_end(arguments);
    g_printerr("foob: %s\n", message);
}

static void report_geometry(const FoobDiagnostics *diagnostics)
{
    WPEView *view = diagnostics->view;
    WPEToplevelState state = wpe_view_get_toplevel_state(view);
    foob_diagnostics_log(diagnostics,
                         "viewport width=%d height=%d scale=%.3f "
                         "active=%d fullscreen=%d maximized=%d focus=%d",
                         wpe_view_get_width(view), wpe_view_get_height(view),
                         wpe_view_get_scale(view),
                         !!(state & WPE_TOPLEVEL_STATE_ACTIVE),
                         !!(state & WPE_TOPLEVEL_STATE_FULLSCREEN),
                         !!(state & WPE_TOPLEVEL_STATE_MAXIMIZED),
                         wpe_view_get_has_focus(view));
}

static gboolean geometry_idle(gpointer user_data)
{
    FoobDiagnostics *diagnostics = user_data;
    diagnostics->geometry_source = 0;
    report_geometry(diagnostics);
    return G_SOURCE_REMOVE;
}

/* Coalesces bursts of resize/state notifications into one report. */
static void schedule_geometry_report(FoobDiagnostics *diagnostics)
{
    if (!diagnostics->closed && !diagnostics->geometry_source)
        diagnostics->geometry_source = g_idle_add(geometry_idle, diagnostics);
}

static void geometry_changed([[maybe_unused]] GObject *object,
                             [[maybe_unused]] GParamSpec *property, gpointer user_data)
{
    schedule_geometry_report(user_data);
}

static void state_changed([[maybe_unused]] WPEView *view,
                          [[maybe_unused]] WPEToplevelState previous, gpointer user_data)
{
    schedule_geometry_report(user_data);
}

static void view_closed([[maybe_unused]] WPEView *view, gpointer user_data)
{
    FoobDiagnostics *diagnostics = user_data;
    diagnostics->closed = true;
    foob_diagnostics_log(diagnostics, "closed");
}

static void load_changed([[maybe_unused]] WebKitWebView *web_view, WebKitLoadEvent event,
                         gpointer user_data)
{
    FoobDiagnostics *diagnostics = user_data;
    if (event == WEBKIT_LOAD_STARTED) {
        diagnostics->navigation_started = g_get_monotonic_time();
        diagnostics->load_failed = false;
        foob_diagnostics_log(diagnostics, "load-started");
    } else if (event == WEBKIT_LOAD_FINISHED) {
        double elapsed_ms = (double)(g_get_monotonic_time() - diagnostics->navigation_started) /
            (double)G_TIME_SPAN_MILLISECOND;
        foob_diagnostics_log(diagnostics, "load-finished status=%s elapsed_ms=%.3f",
                             diagnostics->load_failed ? "failed" : "ok", elapsed_ms);
        report_geometry(diagnostics);
    }
}

static gboolean load_failed([[maybe_unused]] WebKitWebView *web_view, WebKitLoadEvent event,
                            [[maybe_unused]] const char *uri, /* May hold a private query. */
                            GError *error, gpointer user_data)
{
    FoobDiagnostics *diagnostics = user_data;
    diagnostics->load_failed = true;
    foob_diagnostics_log(diagnostics, "load-failed phase=%d domain=%s code=%d",
                         event, g_quark_to_string(error->domain), error->code);
    return FALSE; /* Preserve default error handling and TLS policy. */
}

static void process_terminated([[maybe_unused]] WebKitWebView *web_view,
                               WebKitWebProcessTerminationReason reason, gpointer user_data)
{
    foob_diagnostics_log(user_data, "web-process-terminated reason=%d", reason);
}

FoobDiagnostics *foob_diagnostics_new(WebKitWebView *web_view, WPEView *view)
{
    FoobDiagnostics *diagnostics = g_new0(FoobDiagnostics, 1);
    diagnostics->web_view = web_view;
    diagnostics->view = view;
    for (size_t i = 0; i < G_N_ELEMENTS(geometry_properties); i++)
        g_signal_connect(view, geometry_properties[i], G_CALLBACK(geometry_changed), diagnostics);
    g_signal_connect(view, "toplevel-state-changed", G_CALLBACK(state_changed), diagnostics);
    g_signal_connect(view, "closed", G_CALLBACK(view_closed), diagnostics);
    g_signal_connect(web_view, "load-changed", G_CALLBACK(load_changed), diagnostics);
    g_signal_connect(web_view, "load-failed", G_CALLBACK(load_failed), diagnostics);
    g_signal_connect(web_view, "web-process-terminated",
                     G_CALLBACK(process_terminated), diagnostics);
    return diagnostics;
}

void foob_diagnostics_free(FoobDiagnostics *diagnostics)
{
    if (!diagnostics)
        return;
    if (diagnostics->geometry_source)
        g_source_remove(diagnostics->geometry_source);
    g_signal_handlers_disconnect_by_data(diagnostics->view, diagnostics);
    g_signal_handlers_disconnect_by_data(diagnostics->web_view, diagnostics);
    g_free(diagnostics);
}
