#include <stdio.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <glib.h>
#include "browser.h"
#include "exit_status.h"
#include "target.h"

/* The kernel aborts a dump at a core limit of 1 only for a pipe or file pattern,
 * not for a socket collector ("@" or "@@"); an unreadable pattern counts as one. */
static bool core_pattern_is_socket(void)
{
    g_autofree char *pattern = nullptr;
    return !g_file_get_contents("/proc/sys/kernel/core_pattern", &pattern, nullptr, nullptr) ||
        pattern[0] == '@';
}

/*
 * A crash must not leave a process's memory (pages, addresses, cookies) on disk
 * (F-77, D-26). A core limit of exactly 1 makes the kernel abort a dump before any
 * handler runs, so systemd-coredump stores no core and journals nothing, not even
 * the command line that holds foob's address (a limit of 0 still journals it). The
 * WebKit processes inherit the limit, so it is set before any of them starts, hard
 * limit included so none can raise it again (D-41). Where 1 cannot work (a socket
 * collector, or a hard limit already 0), the limit is 0 and foob is not dumpable,
 * so no handler runs for it; desktop portals then refuse foob (D-42).
 */
static bool disable_core_dumps(void)
{
    if (!core_pattern_is_socket() &&
        setrlimit(RLIMIT_CORE, &(struct rlimit){ .rlim_cur = 1, .rlim_max = 1 }) == 0)
        return true;
    return setrlimit(RLIMIT_CORE, &(struct rlimit){}) == 0 &&
        prctl(PR_SET_DUMPABLE, 0UL, 0UL, 0UL, 0UL) == 0;
}

int main(int argc, char *argv[])
{
    bool diagnostics = g_strcmp0(g_getenv("FOOB_DIAGNOSTICS"), "1") == 0;
    /* First, before any parsing of the argument; a diagnostics run keeps core dumps. */
    if (!diagnostics && !disable_core_dumps()) {
        fputs("foob: could not turn off core dumps\n", stderr);
        return FOOB_EXIT_START_FAILED;
    }
    /* Load the canonical form, so that what was checked is what WebKit loads. */
    g_autofree char *target = foob_target_canonical(argc == 2 ? argv[1] : "about:blank");
    if (argc > 2 || !target) {
        fputs("Usage: foob [http(s)://host/... | file:///absolute/path | about:blank]\n",
              stderr);
        return FOOB_EXIT_REJECTED_ARGUMENT;
    }
    g_autoptr(FoobBrowser) browser = foob_browser_new(diagnostics);
    if (!browser) {
        fputs("foob: could not connect to native Wayland\n", stderr);
        return FOOB_EXIT_START_FAILED;
    }
    return foob_browser_run(browser, target);
}
