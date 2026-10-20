/* Runs each file in the given directories through a fuzz target once, so the stored
 * inputs are a regression test in every build, without libFuzzer (D-46). */
#include <stdint.h>
#include <stdio.h>
#include <glib.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int main(int argc, char *argv[])
{
    unsigned count = 0;
    for (int i = 1; i < argc; i++) {
        g_autoptr(GDir) dir = g_dir_open(argv[i], 0, nullptr);
        if (!dir) {
            fprintf(stderr, "cannot read %s\n", argv[i]);
            return 1;
        }
        for (const char *name; (name = g_dir_read_name(dir)); count++) {
            g_autofree char *path = g_build_filename(argv[i], name, nullptr);
            g_autofree char *contents = nullptr;
            gsize length = 0;
            if (!g_file_get_contents(path, &contents, &length, nullptr)) {
                fprintf(stderr, "cannot read %s\n", path);
                return 1;
            }
            LLVMFuzzerTestOneInput((const uint8_t *)contents, length);
        }
    }
    printf("%u inputs replayed\n", count);
    return count ? 0 : 1;
}
