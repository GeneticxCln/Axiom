#ifndef AXIOM_LAYER_SHELL_H
#define AXIOM_LAYER_SHELL_H

#include <stdbool.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_layer_shell_v1.h>

struct axiom_server;

/**
 * Layer shell manager for panels, status bars, notifications, etc.
 */
struct axiom_layer_shell_manager {
    struct wlr_layer_shell_v1 *wlr_layer_shell;
    struct axiom_server *server;
    
    // Event listeners
    struct wl_listener new_surface;
};

/**
 * Layer surface wrapper
 */
struct axiom_layer_surface {
    struct wlr_layer_surface_v1 *wlr_layer_surface;
    struct axiom_server *server;
    struct wlr_scene_layer_surface_v1 *scene_layer_surface;
    
    // Surface state
    bool mapped;
    
    // Event listeners
    struct wl_listener destroy;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener new_popup;
    struct wl_listener commit;
};

// Layer shell manager functions
struct axiom_layer_shell_manager *axiom_layer_shell_manager_create(struct axiom_server *server);
void axiom_layer_shell_manager_destroy(struct axiom_layer_shell_manager *manager);
bool axiom_layer_shell_manager_init(struct axiom_layer_shell_manager *manager);

// Layer surface functions
struct axiom_layer_surface *axiom_layer_surface_create(
    struct wlr_layer_surface_v1 *wlr_layer_surface, struct axiom_server *server);
void axiom_layer_surface_destroy(struct axiom_layer_surface *surface);

// Event handlers
void axiom_layer_shell_handle_new_surface(struct wl_listener *listener, void *data);
void axiom_layer_surface_handle_destroy(struct wl_listener *listener, void *data);
void axiom_layer_surface_handle_map(struct wl_listener *listener, void *data);
void axiom_layer_surface_handle_unmap(struct wl_listener *listener, void *data);
void axiom_layer_surface_handle_commit(struct wl_listener *listener, void *data);
void axiom_layer_surface_handle_new_popup(struct wl_listener *listener, void *data);

// Utility functions
void axiom_arrange_layer(struct axiom_server *server, struct wl_list *surfaces);
void axiom_arrange_layers(struct axiom_server *server);

#endif /* AXIOM_LAYER_SHELL_H */
