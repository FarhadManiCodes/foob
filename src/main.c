#include <wpe/webkit.h>
#include <wpe/wpe-platform.h>
#include <wpe/wayland/wpe-wayland.h>

#if __STDC_VERSION__ < 202311L
#error foob requires C23
#endif

/* M0 checks public symbol linkage without opening a display. */
int main(void)
{
    WPEView *(*volatile get_view)(WebKitWebView *) = webkit_web_view_get_wpe_view;
    void (*volatile close_view)(WPEView *) = wpe_view_closed;
    if (!get_view || !close_view)
        return 1;
    GType types[] = {
        webkit_web_view_get_type(),
        wpe_view_get_type(),
        wpe_toplevel_get_type(),
        wpe_display_wayland_get_type(),
        webkit_network_session_get_type(),
    };
    for (size_t i = 0; i < G_N_ELEMENTS(types); i++) {
        if (!types[i])
            return 1;
    }
    return 0;
}
