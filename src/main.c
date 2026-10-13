#if __STDC_VERSION__ < 202311L
#error foob requires C23
#endif

#include <stdio.h>
#include <glib.h>
#include "browser.h"
#include "target.h"

/* tests/check_cli.py depends on these codes. */
enum {
    EXIT_START_FAILED = 1,  /* Display or engine could not start. */
    EXIT_REJECTED_ARGUMENT = 2,
};

int main(int argc, char *argv[])
{
    const char *target = argc == 2 ? argv[1] : "about:blank";
    if (argc > 2 || !foob_target_is_valid(target)) {
        fputs("Usage: foob [http(s)://host/... | file:///absolute/path | about:blank]\n",
              stderr);
        return EXIT_REJECTED_ARGUMENT;
    }
    bool diagnostics = g_strcmp0(g_getenv("FOOB_DIAGNOSTICS"), "1") == 0;
    g_autoptr(FoobBrowser) browser = foob_browser_new(diagnostics);
    if (!browser) {
        fputs("foob: could not connect to native Wayland\n", stderr);
        return EXIT_START_FAILED;
    }
    foob_browser_run(browser, target);
    return EXIT_SUCCESS;
}
