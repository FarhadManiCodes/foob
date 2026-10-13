#ifndef FOOB_DIAGNOSTICS_H
#define FOOB_DIAGNOSTICS_H

#include <glib.h>
#include <wpe/webkit.h>
#include <wpe/wpe-platform.h>

/*
 * Optional redacted runtime logging (FOOB_DIAGNOSTICS=1). Everything printed
 * here is application state only: never URLs, error text, cookies or input
 * contents. A disabled instance is nullptr and every function accepts it.
 */
typedef struct FoobDiagnostics FoobDiagnostics;

/* Borrows both objects; they must outlive foob_diagnostics_free(). */
FoobDiagnostics *foob_diagnostics_new(WebKitWebView *web_view, WPEView *view);

/* Disconnects every handler and cancels the pending idle source. */
void foob_diagnostics_free(FoobDiagnostics *diagnostics);

void foob_diagnostics_log(const FoobDiagnostics *diagnostics, const char *format, ...)
    G_GNUC_PRINTF(2, 3);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(FoobDiagnostics, foob_diagnostics_free)

#endif
