#include <stdio.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <glib.h>
#include "browser.h"
#include "exit_status.h"
#include "target.h"

/*
 * A crash must not leave a process's memory (pages, addresses, cookies) on disk
 * (F-77, D-26). The WebKit processes inherit the limit, so it is set before any of
 * them starts, hard limit included so none can raise it again. systemd-coredump
 * reads the soft limit and stores no core when it is 0, but it still journals the
 * command line, which holds foob's address (not the helpers'), so foob also makes
 * itself non-dumpable: then no handler runs at all.
 */
static bool disable_core_dumps(void)
{
    return setrlimit(RLIMIT_CORE, &(struct rlimit){}) == 0 &&
        prctl(PR_SET_DUMPABLE, 0UL, 0UL, 0UL, 0UL) == 0;
}

int main(int argc, char *argv[])
{
    const char *target = argc == 2 ? argv[1] : "about:blank";
    if (argc > 2 || !foob_target_is_valid(target)) {
        fputs("Usage: foob [http(s)://host/... | file:///absolute/path | about:blank]\n",
              stderr);
        return FOOB_EXIT_REJECTED_ARGUMENT;
    }
    bool diagnostics = g_strcmp0(g_getenv("FOOB_DIAGNOSTICS"), "1") == 0;
    /* A diagnostics run keeps core dumps for debugging. */
    if (!diagnostics && !disable_core_dumps()) {
        fputs("foob: could not turn off core dumps\n", stderr);
        return FOOB_EXIT_START_FAILED;
    }
    g_autoptr(FoobBrowser) browser = foob_browser_new(diagnostics);
    if (!browser) {
        fputs("foob: could not connect to native Wayland\n", stderr);
        return FOOB_EXIT_START_FAILED;
    }
    return foob_browser_run(browser, target);
}
