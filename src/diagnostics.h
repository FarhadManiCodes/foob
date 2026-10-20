#ifndef FOOB_DIAGNOSTICS_H
#define FOOB_DIAGNOSTICS_H

#include <glib.h>
#include <wpe/webkit.h>
#include <wpe/wpe-platform.h>

/*
 * Optional redacted runtime logging (FOOB_DIAGNOSTICS=1). Everything printed
 * here is application state only: never URLs, hosts, titles, error text, cookies
 * or input contents. So this interface takes no text at all, only foob's own
 * events and type names (D-61): page text cannot reach it without changing this
 * header, and tests/check_logging.py checks the lines diagnostics.c prints. A
 * disabled instance is nullptr and every function accepts it.
 */
typedef struct FoobDiagnostics FoobDiagnostics;

/* Borrows both objects; they must outlive foob_diagnostics_free(). */
FoobDiagnostics *foob_diagnostics_new(WebKitWebView *web_view, WPEView *view);

/* Disconnects every handler and cancels the pending idle source. */
void foob_diagnostics_free(FoobDiagnostics *diagnostics);

typedef enum {
    FOOB_EVENT_DOWNLOAD_CANCELLED,
    FOOB_EVENT_DISPLAY_DISCONNECTED,
    FOOB_EVENT_SHUTDOWN_SIGNAL,
} FoobEvent;

void foob_diagnostics_event(const FoobDiagnostics *diagnostics, FoobEvent event);
void foob_diagnostics_permission_denied(const FoobDiagnostics *diagnostics, GType request);
void foob_diagnostics_backend(const FoobDiagnostics *diagnostics, GType display);

#endif
