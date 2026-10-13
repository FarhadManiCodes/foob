#ifndef FOOB_EXIT_STATUS_H
#define FOOB_EXIT_STATUS_H

/* Process exit codes; tests/check_cli.py depends on the first two. */
enum {
    FOOB_EXIT_START_FAILED = 1,       /* Display or engine could not start. */
    FOOB_EXIT_REJECTED_ARGUMENT = 2,
    FOOB_EXIT_DISPLAY_LOST = 3,       /* Compositor connection dropped while running. */
    FOOB_EXIT_ENGINE_LOST = 4,        /* The web process crashed or was killed. */
};

#endif
