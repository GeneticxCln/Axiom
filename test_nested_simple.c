#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-server.h>
#include <wlr/backend/wayland.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_output.h>

// Simple test to create a nested wayland window
int main() {
    printf("Testing nested Wayland window creation...\n");
    
    // Create event loop
    struct wl_event_loop *loop = wl_event_loop_create();
    if (!loop) {
        printf("Failed to create event loop\n");
        return 1;
    }
    
    // Create wayland backend
    struct wlr_backend *backend = wlr_wl_backend_create(loop, NULL);
    if (!backend) {
        printf("Failed to create wayland backend\n");
        wl_event_loop_destroy(loop);
        return 1;
    }
    
    printf("Created wayland backend successfully\n");
    
    // Start backend FIRST
    if (!wlr_backend_start(backend)) {
        printf("Failed to start backend\n");
        wl_event_loop_destroy(loop);
        return 1;
    }
    
    printf("Started backend successfully\n");
    
    // Create output (this should create a window)
    struct wlr_output *output = wlr_wl_output_create(backend);
    if (!output) {
        printf("Failed to create wayland output\n");
        wl_event_loop_destroy(loop);
        return 1;
    }
    
    printf("Created wayland output successfully\n");
    
    printf("Started backend - window should now be visible!\n");
    printf("Press Ctrl+C to exit...\n");
    
    // Run for 10 seconds
    for (int i = 0; i < 100; i++) {
        wl_event_loop_dispatch(loop, 100);
    }
    
    printf("Test completed\n");
    wl_event_loop_destroy(loop);
    return 0;
}
