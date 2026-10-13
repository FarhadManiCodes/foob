#ifndef FOOB_BROWSER_H
#define FOOB_BROWSER_H

#include <glib.h>

/*
 * One browser window: a native Wayland display, an ephemeral network session
 * and one WebKitWebView/WPEView. All shutdown paths (window close, SIGINT,
 * SIGTERM) stop the same main loop; foob_browser_free() then tears down in
 * dependency order.
 */
typedef struct FoobBrowser FoobBrowser;

/* Returns nullptr when the native Wayland display cannot be reached. */
FoobBrowser *foob_browser_new(bool diagnostics);

/* Loads the already validated target and blocks until the browser stops. */
void foob_browser_run(FoobBrowser *browser, const char *target);

void foob_browser_free(FoobBrowser *browser);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(FoobBrowser, foob_browser_free)

#endif
