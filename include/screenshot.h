#ifndef AXIOM_SCREENSHOT_H
#define AXIOM_SCREENSHOT_H

#include <stdbool.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_screencopy_v1.h>
#include <wlr/types/wlr_export_dmabuf_v1.h>

struct axiom_server;

/**
 * Screenshot manager for screen capture functionality
 */
struct axiom_screenshot_manager {
    struct wlr_screencopy_manager_v1 *screencopy_manager;
    struct wlr_export_dmabuf_manager_v1 *export_dmabuf_manager;
    struct axiom_server *server;
    
    // Event listeners
    struct wl_listener new_screencopy_frame;
    struct wl_listener new_dmabuf_frame;
};

/**
 * Screenshot frame state
 */
struct axiom_screenshot_frame {
    struct wlr_screencopy_frame_v1 *wlr_frame;
    struct axiom_server *server;
    
    // Event listeners
    struct wl_listener destroy;
    struct wl_listener ready;
    struct wl_listener failed;
};

// Screenshot manager functions
struct axiom_screenshot_manager *axiom_screenshot_manager_create(struct axiom_server *server);
void axiom_screenshot_manager_destroy(struct axiom_screenshot_manager *manager);
bool axiom_screenshot_manager_init(struct axiom_screenshot_manager *manager);

// Screenshot frame functions
struct axiom_screenshot_frame *axiom_screenshot_frame_create(
    struct wlr_screencopy_frame_v1 *wlr_frame, struct axiom_server *server);
void axiom_screenshot_frame_destroy(struct axiom_screenshot_frame *frame);

// Event handlers
void axiom_screenshot_handle_new_screencopy_frame(struct wl_listener *listener, void *data);
void axiom_screenshot_handle_new_dmabuf_frame(struct wl_listener *listener, void *data);
void axiom_screenshot_frame_handle_destroy(struct wl_listener *listener, void *data);
void axiom_screenshot_frame_handle_ready(struct wl_listener *listener, void *data);
void axiom_screenshot_frame_handle_failed(struct wl_listener *listener, void *data);

// Utility functions
bool axiom_take_screenshot(struct axiom_server *server, const char *output_name, const char *filename);
bool axiom_take_region_screenshot(struct axiom_server *server, int x, int y, int width, int height, const char *filename);

#endif /* AXIOM_SCREENSHOT_H */
