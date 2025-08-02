#ifndef AXIOM_XWAYLAND_H
#define AXIOM_XWAYLAND_H

#include <stdbool.h>
#include <wayland-server-core.h>
#include <wlr/xwayland.h>
#include <xcb/xcb.h>
#include <xcb/composite.h>
#include <xcb/xfixes.h>

struct axiom_server;
struct axiom_window;

/**
 * XWayland manager structure
 */
struct axiom_xwayland_manager {
    struct wlr_xwayland *wlr_xwayland;
    struct axiom_server *server;
    
    // Event listeners
    struct wl_listener new_surface;
    struct wl_listener ready;
    
    // XWayland specific settings
    bool enabled;
    bool lazy;  // Start XWayland on demand
    bool force_zero_scaling;  // Force scaling to 1.0 for X11 apps
};

/**
 * XWayland surface wrapper
 */
struct axiom_xwayland_surface {
    struct wlr_xwayland_surface *wlr_xwayland_surface;
    struct axiom_window *window;  // Associated window
    struct axiom_server *server;  // Server reference
    
    // Event listeners
    struct wl_listener destroy;
    struct wl_listener request_configure;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_minimize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;
    struct wl_listener request_activate;
    struct wl_listener set_title;
    struct wl_listener set_class;
    struct wl_listener set_role;
    struct wl_listener set_parent;
    struct wl_listener set_pid;
    struct wl_listener set_window_type;
    struct wl_listener set_hints;
    struct wl_listener set_decorations;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener associate;
    struct wl_listener dissociate;
    
    // X11 specific properties
    bool override_redirect;
    bool modal;
    uint32_t window_type;
};


// XWayland manager functions
struct axiom_xwayland_manager *axiom_xwayland_manager_create(struct axiom_server *server);
void axiom_xwayland_manager_destroy(struct axiom_xwayland_manager *manager);
bool axiom_xwayland_manager_init(struct axiom_xwayland_manager *manager);
void axiom_xwayland_manager_finish(struct axiom_xwayland_manager *manager);

// XWayland surface functions
struct axiom_xwayland_surface *axiom_xwayland_surface_create(
    struct wlr_xwayland_surface *wlr_surface, struct axiom_server *server);
void axiom_xwayland_surface_destroy(struct axiom_xwayland_surface *surface);
void axiom_xwayland_surface_configure(struct axiom_xwayland_surface *surface,
    int16_t x, int16_t y, uint16_t width, uint16_t height);
void axiom_xwayland_surface_close(struct axiom_xwayland_surface *surface);
void axiom_xwayland_surface_set_minimized(struct axiom_xwayland_surface *surface, bool minimized);
void axiom_xwayland_surface_set_maximized(struct axiom_xwayland_surface *surface, bool maximized);
void axiom_xwayland_surface_set_fullscreen(struct axiom_xwayland_surface *surface, bool fullscreen);
void axiom_xwayland_surface_activate(struct axiom_xwayland_surface *surface, bool activated);

// Window integration functions
bool axiom_window_is_xwayland(struct axiom_window *window);
struct axiom_xwayland_surface *axiom_window_get_xwayland_surface(struct axiom_window *window);

// Event handlers
void axiom_xwayland_handle_new_surface(struct wl_listener *listener, void *data);
void axiom_xwayland_handle_ready(struct wl_listener *listener, void *data);

// Surface event handlers
void axiom_xwayland_surface_handle_destroy(struct wl_listener *listener, void *data);
void axiom_xwayland_surface_handle_request_configure(struct wl_listener *listener, void *data);
void axiom_xwayland_surface_handle_request_move(struct wl_listener *listener, void *data);
void axiom_xwayland_surface_handle_request_resize(struct wl_listener *listener, void *data);
void axiom_xwayland_surface_handle_request_minimize(struct wl_listener *listener, void *data);
void axiom_xwayland_surface_handle_request_maximize(struct wl_listener *listener, void *data);
void axiom_xwayland_surface_handle_request_fullscreen(struct wl_listener *listener, void *data);
void axiom_xwayland_surface_handle_request_activate(struct wl_listener *listener, void *data);
void axiom_xwayland_surface_handle_set_title(struct wl_listener *listener, void *data);
void axiom_xwayland_surface_handle_set_class(struct wl_listener *listener, void *data);
void axiom_xwayland_surface_handle_map(struct wl_listener *listener, void *data);
void axiom_xwayland_surface_handle_unmap(struct wl_listener *listener, void *data);
void axiom_xwayland_surface_handle_associate(struct wl_listener *listener, void *data);
void axiom_xwayland_surface_handle_dissociate(struct wl_listener *listener, void *data);

// Utility functions
bool axiom_xwayland_surface_wants_focus(struct axiom_xwayland_surface *surface);
bool axiom_xwayland_surface_is_unmanaged(struct axiom_xwayland_surface *surface);

#endif /* AXIOM_XWAYLAND_H */
