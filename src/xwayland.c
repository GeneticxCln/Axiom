#include "xwayland.h"
#include "axiom.h"
#include "animation.h"
#include "window_rules.h"

/**
 * Create and initialize the XWayland manager.
 * @param server Axiom server instance.
 * @return Pointer to the initialized manager.
 */
struct axiom_xwayland_manager *axiom_xwayland_manager_create(struct axiom_server *server) {
    struct axiom_xwayland_manager *manager = calloc(1, sizeof(struct axiom_xwayland_manager));
    if (!manager) {
        AXIOM_LOG_ERROR("Failed to allocate XWayland manager");
        return NULL;
    }
    manager->server = server;
    return manager;
}

/**
 * Initialize the XWayland manager.
 * @param manager The XWayland manager instance.
 * @return True if successfully initialized, false otherwise.
 */
bool axiom_xwayland_manager_init(struct axiom_xwayland_manager *manager) {
    if (!manager || !manager->server) {
        AXIOM_LOG_ERROR("Invalid manager or server");
        return false;
    }

    manager->wlr_xwayland = wlr_xwayland_create(manager->server->wl_display, manager->server->compositor, true);
    if (!manager->wlr_xwayland) {
        AXIOM_LOG_ERROR("Failed to create wlr_xwayland");
        return false;
    }

    manager->new_surface.notify = axiom_xwayland_handle_new_surface;
    wl_signal_add(&manager->wlr_xwayland->events.new_surface, &manager->new_surface);

    manager->ready.notify = axiom_xwayland_handle_ready;
    wl_signal_add(&manager->wlr_xwayland->events.ready, &manager->ready);

    AXIOM_LOG_INFO("XWayland setup complete");
    return true;
}

/**
 * Destroy the XWayland manager.
 * @param manager The XWayland manager instance.
 */
void axiom_xwayland_manager_destroy(struct axiom_xwayland_manager *manager) {
    if (!manager) {
        return;
    }

    if (manager->wlr_xwayland) {
        wlr_xwayland_destroy(manager->wlr_xwayland);
    }
    free(manager);
}

/**
 * Create a new XWayland surface wrapper
 */
struct axiom_xwayland_surface *axiom_xwayland_surface_create(
    struct wlr_xwayland_surface *wlr_surface, struct axiom_server *server) {
    struct axiom_xwayland_surface *surface = calloc(1, sizeof(struct axiom_xwayland_surface));
    if (!surface) {
        AXIOM_LOG_ERROR("Failed to allocate XWayland surface");
        return NULL;
    }
    
    surface->wlr_xwayland_surface = wlr_surface;
    surface->override_redirect = wlr_surface->override_redirect;
    surface->modal = wlr_surface->modal;
    surface->server = server;  // Store server reference
    
    // Set up event listeners
    surface->destroy.notify = axiom_xwayland_surface_handle_destroy;
    wl_signal_add(&wlr_surface->events.destroy, &surface->destroy);
    
    surface->map.notify = axiom_xwayland_surface_handle_map;
    wl_signal_add(&wlr_surface->surface->events.map, &surface->map);
    
    surface->unmap.notify = axiom_xwayland_surface_handle_unmap;
    wl_signal_add(&wlr_surface->surface->events.unmap, &surface->unmap);
    
    surface->request_configure.notify = axiom_xwayland_surface_handle_request_configure;
    wl_signal_add(&wlr_surface->events.request_configure, &surface->request_configure);
    
    surface->request_move.notify = axiom_xwayland_surface_handle_request_move;
    wl_signal_add(&wlr_surface->events.request_move, &surface->request_move);
    
    surface->request_resize.notify = axiom_xwayland_surface_handle_request_resize;
    wl_signal_add(&wlr_surface->events.request_resize, &surface->request_resize);
    
    surface->request_minimize.notify = axiom_xwayland_surface_handle_request_minimize;
    wl_signal_add(&wlr_surface->events.request_minimize, &surface->request_minimize);
    
    surface->request_maximize.notify = axiom_xwayland_surface_handle_request_maximize;
    wl_signal_add(&wlr_surface->events.request_maximize, &surface->request_maximize);
    
    surface->request_fullscreen.notify = axiom_xwayland_surface_handle_request_fullscreen;
    wl_signal_add(&wlr_surface->events.request_fullscreen, &surface->request_fullscreen);
    
    surface->request_activate.notify = axiom_xwayland_surface_handle_request_activate;
    wl_signal_add(&wlr_surface->events.request_activate, &surface->request_activate);
    
    surface->set_title.notify = axiom_xwayland_surface_handle_set_title;
    wl_signal_add(&wlr_surface->events.set_title, &surface->set_title);
    
    surface->set_class.notify = axiom_xwayland_surface_handle_set_class;
    wl_signal_add(&wlr_surface->events.set_class, &surface->set_class);
    
    surface->associate.notify = axiom_xwayland_surface_handle_associate;
    wl_signal_add(&wlr_surface->events.associate, &surface->associate);
    
    surface->dissociate.notify = axiom_xwayland_surface_handle_dissociate;
    wl_signal_add(&wlr_surface->events.dissociate, &surface->dissociate);
    
    AXIOM_LOG_INFO("Created XWayland surface: %s (%s)", 
                   wlr_surface->title ?: "(no title)",
                   wlr_surface->class ?: "(no class)");
    
    return surface;
}

/**
 * Destroy XWayland surface wrapper
 */
void axiom_xwayland_surface_destroy(struct axiom_xwayland_surface *surface) {
    if (!surface) return;
    
    // Remove all event listeners
    wl_list_remove(&surface->destroy.link);
    wl_list_remove(&surface->map.link);
    wl_list_remove(&surface->unmap.link);
    wl_list_remove(&surface->request_configure.link);
    wl_list_remove(&surface->request_move.link);
    wl_list_remove(&surface->request_resize.link);
    wl_list_remove(&surface->request_minimize.link);
    wl_list_remove(&surface->request_maximize.link);
    wl_list_remove(&surface->request_fullscreen.link);
    wl_list_remove(&surface->request_activate.link);
    wl_list_remove(&surface->set_title.link);
    wl_list_remove(&surface->set_class.link);
    wl_list_remove(&surface->associate.link);
    wl_list_remove(&surface->dissociate.link);
    
    free(surface);
}

/**
 * Create an Axiom window from XWayland surface
 */
static struct axiom_window *create_xwayland_window(struct axiom_server *server,
                                                  struct axiom_xwayland_surface *xwayland_surface) {
    struct axiom_window *window = calloc(1, sizeof(struct axiom_window));
    if (!window) {
        AXIOM_LOG_ERROR("Failed to allocate XWayland window");
        return NULL;
    }
    
    window->type = AXIOM_WINDOW_XWAYLAND;
    window->xwayland_surface = xwayland_surface;
    window->xdg_toplevel = NULL; // XWayland windows don't have XDG toplevels
    window->server = server;
    
    // Link the XWayland surface to this window
    xwayland_surface->window = window;
    
    // Create scene tree for the XWayland surface
    window->scene_tree = wlr_scene_subsurface_tree_create(&server->scene->tree, 
                                                          xwayland_surface->wlr_xwayland_surface->surface);
    if (!window->scene_tree) {
        AXIOM_LOG_ERROR("Failed to create scene tree for XWayland surface");
        free(window);
        return NULL;
    }
    
    window->scene_tree->node.data = window;
    
    // Set initial geometry from XWayland surface
    struct wlr_xwayland_surface *wlr_surface = xwayland_surface->wlr_xwayland_surface;
    window->x = wlr_surface->x;
    window->y = wlr_surface->y;
    window->width = wlr_surface->width;
    window->height = wlr_surface->height;
    
    // Store surface reference for effects
    window->surface = wlr_surface->surface;
    
    // Set up tiling properties (most X11 apps should be tiled by default)
    if (!xwayland_surface->override_redirect && !axiom_xwayland_surface_is_unmanaged(xwayland_surface)) {
        window->is_tiled = server->tiling_enabled;
        if (window->is_tiled) {
            server->window_count++;
        }
    } else {
        // Override-redirect windows (tooltips, menus, etc.) should float
        window->is_tiled = false;
    }
    
    AXIOM_LOG_INFO("Created XWayland window: %s (%s) - %s", 
                   wlr_surface->title ?: "(no title)",
                   wlr_surface->class ?: "(no class)",
                   window->is_tiled ? "tiled" : "floating");
    
    return window;
}

/**
 * Callback when a new XWayland surface is created.
 */
void axiom_xwayland_handle_new_surface(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_manager *manager = wl_container_of(listener, manager, new_surface);
    struct wlr_xwayland_surface *wlr_surface = data;
    
    AXIOM_LOG_INFO("New XWayland surface: %s (%s)", 
                   wlr_surface->title ?: "(no title)",
                   wlr_surface->class ?: "(no class)");
    
    // Create XWayland surface wrapper
    struct axiom_xwayland_surface *surface = axiom_xwayland_surface_create(wlr_surface, manager->server);
    if (!surface) {
        AXIOM_LOG_ERROR("Failed to create XWayland surface wrapper");
        return;
    }
    
    // Store reference in wlr_surface for later retrieval
    wlr_surface->data = surface;
    
    // Don't create the window immediately - wait for associate event
    // This ensures the surface has proper Wayland surface backing
}

/**
 * Callback when XWayland is ready.
 */
void axiom_xwayland_handle_ready(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_manager *manager = wl_container_of(listener, manager, ready);
    (void)data; // Suppress unused parameter warning
    
    AXIOM_LOG_INFO("XWayland server is ready and can accept X11 connections");
    
    // Set DISPLAY environment variable for X11 applications
    if (manager->wlr_xwayland->display_name) {
        setenv("DISPLAY", manager->wlr_xwayland->display_name, true);
        AXIOM_LOG_INFO("X11 applications can connect to DISPLAY=%s", 
                       manager->wlr_xwayland->display_name);
    }
}

// ===== XWayland Surface Event Handlers =====

void axiom_xwayland_surface_handle_destroy(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_surface *surface = wl_container_of(listener, surface, destroy);
    (void)data;
    
    AXIOM_LOG_INFO("XWayland surface destroyed");
    
    // If we have an associated window, clean it up
    if (surface->window) {
        struct axiom_window *window = surface->window;
        struct axiom_server *server = window->server;
        
        // Update window count if it was tiled
        if (window->is_tiled && server->window_count > 0) {
            server->window_count--;
            AXIOM_LOG_INFO("XWayland tiled window destroyed, remaining: %d", server->window_count);
            
            // Rearrange remaining windows
            if (server->tiling_enabled) {
                axiom_arrange_windows(server);
            }
        }
        
        // Clear focus if this was the focused window
        if (server->focused_window == window) {
            server->focused_window = NULL;
        }
        
        // Clear grabbed window if this was the grabbed window
        if (server->grabbed_window == window) {
            server->grabbed_window = NULL;
            server->cursor_mode = AXIOM_CURSOR_PASSTHROUGH;
        }
        
        // Remove from window list
        wl_list_remove(&window->link);
        free(window);
    }
    
    // Clean up the surface wrapper
    axiom_xwayland_surface_destroy(surface);
}

void axiom_xwayland_surface_handle_map(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_surface *surface = wl_container_of(listener, surface, map);
    (void)data;
    
    struct wlr_xwayland_surface *wlr_surface = surface->wlr_xwayland_surface;
    AXIOM_LOG_INFO("XWayland surface mapped: %s (%s)", 
                   wlr_surface->title ?: "(no title)",
                   wlr_surface->class ?: "(no class)");
    
    if (!surface->window) {
        AXIOM_LOG_ERROR("XWayland surface mapped but no window exists");
        return;
    }
    
    struct axiom_window *window = surface->window;
    struct axiom_server *server = window->server;
    
    // Update window geometry from XWayland surface
    window->x = wlr_surface->x;
    window->y = wlr_surface->y;
    window->width = wlr_surface->width;
    window->height = wlr_surface->height;
    
    // Position the scene tree
    wlr_scene_node_set_position(&window->scene_tree->node, window->x, window->y);
    
    // Apply window rules if rules system is enabled
    if (server->window_rules_manager) {
        axiom_window_rules_apply_to_window(server->window_rules_manager, window);
    }
    
    // Trigger window appear animation
    if (server->animation_manager) {
        axiom_animate_window_appear(server, window);
    }
    
    // Arrange windows if tiling is enabled
    if (server->tiling_enabled && window->is_tiled) {
        axiom_arrange_windows(server);
    }
}

void axiom_xwayland_surface_handle_unmap(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_surface *surface = wl_container_of(listener, surface, unmap);
    (void)data;
    
    AXIOM_LOG_INFO("XWayland surface unmapped");
    
    if (surface->window && surface->window->server->animation_manager) {
        axiom_animate_window_disappear(surface->window->server, surface->window);
    }
}

void axiom_xwayland_surface_handle_request_configure(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_surface *surface = wl_container_of(listener, surface, request_configure);
    struct wlr_xwayland_surface_configure_event *event = data;
    
    AXIOM_LOG_DEBUG("XWayland configure request: %dx%d at (%d,%d)", 
                    event->width, event->height, event->x, event->y);
    
    // For now, just acknowledge the configure request
    // In a full implementation, you might want to apply tiling constraints here
    wlr_xwayland_surface_configure(surface->wlr_xwayland_surface, 
                                   event->x, event->y, event->width, event->height);
    
    // Update our window if it exists
    if (surface->window) {
        struct axiom_window *window = surface->window;
        window->x = event->x;
        window->y = event->y;
        window->width = event->width;
        window->height = event->height;
        
        // Update scene tree position
        wlr_scene_node_set_position(&window->scene_tree->node, window->x, window->y);
    }
}

void axiom_xwayland_surface_handle_request_move(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_surface *surface = wl_container_of(listener, surface, request_move);
    (void)data;
    
    AXIOM_LOG_DEBUG("XWayland move request");
    
    if (surface->window) {
        // Begin interactive move
        axiom_begin_interactive(surface->window, AXIOM_CURSOR_MOVE, 0);
    }
}

void axiom_xwayland_surface_handle_request_resize(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_surface *surface = wl_container_of(listener, surface, request_resize);
    struct wlr_xwayland_resize_event *event = data;
    
    AXIOM_LOG_DEBUG("XWayland resize request with edges: %d", event->edges);
    
    if (surface->window) {
        // Begin interactive resize
        axiom_begin_interactive(surface->window, AXIOM_CURSOR_RESIZE, event->edges);
    }
}

void axiom_xwayland_surface_handle_request_minimize(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_surface *surface = wl_container_of(listener, surface, request_minimize);
    struct wlr_xwayland_minimize_event *event = data;
    
    AXIOM_LOG_DEBUG("XWayland minimize request: %s", event->minimize ? "minimize" : "unminimize");
    
    // For now, just acknowledge the request
    // In a full implementation, you might hide/show the window
    if (surface->window) {
        wlr_scene_node_set_enabled(&surface->window->scene_tree->node, !event->minimize);
    }
}

void axiom_xwayland_surface_handle_request_maximize(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_surface *surface = wl_container_of(listener, surface, request_maximize);
    (void)data;
    
    AXIOM_LOG_DEBUG("XWayland maximize request");
    
    if (surface->window) {
        struct axiom_window *window = surface->window;
        struct axiom_server *server = window->server;
        
        if (!window->is_maximized) {
            // Save current geometry
            window->saved_x = window->x;
            window->saved_y = window->y;
            window->saved_width = window->width;
            window->saved_height = window->height;
            
            // Maximize to workspace dimensions
            window->x = 0;
            window->y = 0;
            window->width = server->workspace_width;
            window->height = server->workspace_height;
            window->is_maximized = true;
            
            // Configure the XWayland surface
            wlr_xwayland_surface_configure(surface->wlr_xwayland_surface,
                                           window->x, window->y, window->width, window->height);
            
            // Update scene tree position
            wlr_scene_node_set_position(&window->scene_tree->node, window->x, window->y);
        }
    }
}

void axiom_xwayland_surface_handle_request_fullscreen(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_surface *surface = wl_container_of(listener, surface, request_fullscreen);
    (void)data;
    
    AXIOM_LOG_DEBUG("XWayland fullscreen request");
    
    if (surface->window) {
        struct axiom_window *window = surface->window;
        struct axiom_server *server = window->server;
        
        if (!window->is_fullscreen) {
            // Save current geometry
            window->saved_x = window->x;
            window->saved_y = window->y;
            window->saved_width = window->width;
            window->saved_height = window->height;
            
            // Fullscreen to workspace dimensions
            window->x = 0;
            window->y = 0;
            window->width = server->workspace_width;
            window->height = server->workspace_height;
            window->is_fullscreen = true;
            
            // Configure the XWayland surface
            wlr_xwayland_surface_configure(surface->wlr_xwayland_surface,
                                           window->x, window->y, window->width, window->height);
            
            // Update scene tree position
            wlr_scene_node_set_position(&window->scene_tree->node, window->x, window->y);
        }
    }
}

void axiom_xwayland_surface_handle_request_activate(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_surface *surface = wl_container_of(listener, surface, request_activate);
    (void)data;
    
    AXIOM_LOG_DEBUG("XWayland activate request");
    
    if (surface->window) {
        axiom_focus_window_legacy(surface->window->server, surface->window, surface->wlr_xwayland_surface->surface);
    }
}

void axiom_xwayland_surface_handle_set_title(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_surface *surface = wl_container_of(listener, surface, set_title);
    (void)data;
    
    AXIOM_LOG_DEBUG("XWayland title changed: %s", 
                    surface->wlr_xwayland_surface->title ?: "(no title)");
}

void axiom_xwayland_surface_handle_set_class(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_surface *surface = wl_container_of(listener, surface, set_class);
    (void)data;
    
    AXIOM_LOG_DEBUG("XWayland class changed: %s", 
                    surface->wlr_xwayland_surface->class ?: "(no class)");
    
    // Re-apply window rules when class changes
    if (surface->window && surface->window->server->window_rules_manager) {
        axiom_window_rules_apply_to_window(surface->window->server->window_rules_manager, surface->window);
    }
}

void axiom_xwayland_surface_handle_associate(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_surface *surface = wl_container_of(listener, surface, associate);
    (void)data;
    
    AXIOM_LOG_INFO("XWayland surface associated with Wayland surface");
    
    // Now we can create the window since we have a proper Wayland surface
    if (!surface->window) {
        struct wlr_xwayland_surface *wlr_surface = surface->wlr_xwayland_surface;
        
        // Use the stored server reference
        struct axiom_server *server = surface->server;
        if (!server) {
            AXIOM_LOG_ERROR("No server reference in XWayland surface");
            return;
        }
        
        // Create the window
        struct axiom_window *window = create_xwayland_window(server, surface);
        if (!window) {
            AXIOM_LOG_ERROR("Failed to create XWayland window");
            return;
        }
        
        // Add window to server's window list
        wl_list_insert(&server->windows, &window->link);
        
        AXIOM_LOG_INFO("Created and linked XWayland window: %s (%s)",
                       wlr_surface->title ?: "(no title)",
                       wlr_surface->class ?: "(no class)");
    }
}

void axiom_xwayland_surface_handle_dissociate(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_surface *surface = wl_container_of(listener, surface, dissociate);
    (void)data;
    
    AXIOM_LOG_INFO("XWayland surface dissociated from Wayland surface");
}

// ===== Utility Functions =====

bool axiom_xwayland_surface_wants_focus(struct axiom_xwayland_surface *surface) {
    if (!surface || !surface->wlr_xwayland_surface) {
        return false;
    }
    
    struct wlr_xwayland_surface *wlr_surface = surface->wlr_xwayland_surface;
    return wlr_surface->surface && wlr_surface->surface->mapped && !wlr_surface->override_redirect;
}

bool axiom_xwayland_surface_is_unmanaged(struct axiom_xwayland_surface *surface) {
    if (!surface || !surface->wlr_xwayland_surface) {
        return true;
    }
    
    struct wlr_xwayland_surface *wlr_surface = surface->wlr_xwayland_surface;
    
    // Override-redirect windows should not be managed
    if (wlr_surface->override_redirect) {
        return true;
    }
    
    // Check for common unmanaged window types
    for (size_t i = 0; i < wlr_surface->window_type_len; i++) {
        xcb_atom_t type = wlr_surface->window_type[i];
        // These would need to be mapped from atom names, but for now we'll skip this
        // In a full implementation, you'd check for _NET_WM_WINDOW_TYPE_TOOLTIP, etc.
        (void)type;
    }
    
    return false;
}

// Window integration functions
bool axiom_window_is_xwayland(struct axiom_window *window) {
    return window && window->type == AXIOM_WINDOW_XWAYLAND;
}

struct axiom_xwayland_surface *axiom_window_get_xwayland_surface(struct axiom_window *window) {
    if (!axiom_window_is_xwayland(window)) {
        return NULL;
    }
    return window->xwayland_surface;
}
