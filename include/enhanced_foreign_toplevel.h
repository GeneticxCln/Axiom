#ifndef AXIOM_FOREIGN_TOPLEVEL_MANAGER_H
#define AXIOM_FOREIGN_TOPLEVEL_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include <wlr/types/wlr_scene.h>

struct axiom_server;
struct axiom_window;

// Toplevel export modes
enum axiom_toplevel_export_mode {
    AXIOM_TOPLEVEL_EXPORT_NONE = 0,
    AXIOM_TOPLEVEL_EXPORT_TITLEBAR = 1,
    AXIOM_TOPLEVEL_EXPORT_FULLSCREEN = 2
};

// Foreign toplevel surface tracking
struct axiom_foreign_toplevel_surface {
    struct wlr_foreign_toplevel_handle_v1 *wlr_toplevel_handle;
    struct axiom_window *window;  // Associated with Axiom window
    struct axiom_server *server;
    
    // Export properties
    enum axiom_toplevel_export_mode export_mode;
    uint32_t export_flags;  // Reserved for future export flags

    // Attributes
    char *app_id;
    char *title;

    // Scene integration
    struct wlr_scene_tree *scene_tree;

    // Event listeners
    struct wl_listener maximize;
    struct wl_listener minimize;
    struct wl_listener activate;
    struct wl_listener fullscreen;
    struct wl_listener unfullscreen;
    struct wl_listener close;

    // Link in manager
    struct wl_list link;
};

// Foreign toplevel manager
struct axiom_foreign_toplevel_manager {
    struct axiom_server *server;
    struct wlr_foreign_toplevel_manager_v1 *wlr_foreign_toplevel_manager;

    // Toplevel tracking
    struct wl_list toplevels;  // axiom_foreign_toplevel_surface::link

    // Default export properties
    enum axiom_toplevel_export_mode default_export_mode;

    // Event listeners
    struct wl_listener new_toplevel;

    // Configuration
    struct {
        bool auto_export_titlebars;
        bool auto_export_fullscreen;
    } config;

    // Diagnostics
    bool debug_toplevel_export;
};

// Manager functions
struct axiom_foreign_toplevel_manager *axiom_foreign_toplevel_manager_create(struct axiom_server *server);
void axiom_foreign_toplevel_manager_destroy(struct axiom_foreign_toplevel_manager *manager);

// Toplevel functions
struct axiom_foreign_toplevel_surface *axiom_foreign_toplevel_surface_create(
    struct axiom_foreign_toplevel_manager *manager,
    struct wlr_foreign_toplevel_handle_v1 *handle);
void axiom_foreign_toplevel_surface_destroy(struct axiom_foreign_toplevel_surface *surface);
void axiom_foreign_toplevel_surface_export(struct axiom_foreign_toplevel_surface *surface, 
                                          enum axiom_toplevel_export_mode mode);

// Utility functions
struct axiom_foreign_toplevel_surface *axiom_foreign_toplevel_surface_from_wlr_handle(
    struct axiom_foreign_toplevel_manager *manager,
    struct wlr_foreign_toplevel_handle_v1 *handle);

// Event handling
void axiom_foreign_toplevel_manager_handle_new(struct axiom_foreign_toplevel_manager *manager,
                                              struct wlr_foreign_toplevel_handle_v1 *handle);

// Debugging and diagnostics
void axiom_foreign_toplevel_surface_debug_info(struct axiom_foreign_toplevel_surface *surface);
void axiom_foreign_toplevel_manager_debug_state(struct axiom_foreign_toplevel_manager *manager);

#endif /* AXIOM_FOREIGN_TOPLEVEL_MANAGER_H */

