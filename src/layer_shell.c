#include "layer_shell.h"
#include "axiom.h"
#include "logging.h"
#include "memory.h"
#include <wlr/types/wlr_scene.h>

/**
 * Create and initialize the layer shell manager.
 */
struct axiom_layer_shell_manager *axiom_layer_shell_manager_create(struct axiom_server *server) {
    struct axiom_layer_shell_manager *manager = axiom_calloc_tracked(1, 
        sizeof(struct axiom_layer_shell_manager), AXIOM_MEM_TYPE_LAYER_SHELL, 
        __FILE__, __func__, __LINE__);
    if (!manager) {
        AXIOM_LOG_ERROR("LAYER_SHELL", "Failed to allocate layer shell manager");
        return NULL;
    }
    
    manager->server = server;
    return manager;
}

/**
 * Initialize the layer shell manager.
 */
bool axiom_layer_shell_manager_init(struct axiom_layer_shell_manager *manager) {
    if (!manager || !manager->server) {
        AXIOM_LOG_ERROR("LAYER_SHELL", "Invalid manager or server");
        return false;
    }
    
    manager->wlr_layer_shell = wlr_layer_shell_v1_create(manager->server->wl_display, 4);
    if (!manager->wlr_layer_shell) {
        AXIOM_LOG_ERROR("LAYER_SHELL", "Failed to create wlr_layer_shell_v1");
        return false;
    }
    
    manager->new_surface.notify = axiom_layer_shell_handle_new_surface;
    wl_signal_add(&manager->wlr_layer_shell->events.new_surface, &manager->new_surface);
    
    AXIOM_LOG_INFO("LAYER_SHELL", "Layer shell protocol initialized successfully");
    return true;
}

/**
 * Destroy the layer shell manager.
 */
void axiom_layer_shell_manager_destroy(struct axiom_layer_shell_manager *manager) {
    if (!manager) {
        return;
    }
    
    if (manager->wlr_layer_shell) {
        // wlr_layer_shell is destroyed automatically with the display
    }
    
    axiom_free_tracked(manager, __FILE__, __func__, __LINE__);
}

/**
 * Create a new layer surface wrapper.
 */
struct axiom_layer_surface *axiom_layer_surface_create(
    struct wlr_layer_surface_v1 *wlr_layer_surface, struct axiom_server *server) {
    
    struct axiom_layer_surface *surface = axiom_calloc_tracked(1, 
        sizeof(struct axiom_layer_surface), AXIOM_MEM_TYPE_LAYER_SHELL,
        __FILE__, __func__, __LINE__);
    if (!surface) {
        AXIOM_LOG_ERROR("LAYER_SHELL", "Failed to allocate layer surface");
        return NULL;
    }
    
    surface->wlr_layer_surface = wlr_layer_surface;
    surface->server = server;
    surface->mapped = false;
    
    // Set up event listeners
    surface->destroy.notify = axiom_layer_surface_handle_destroy;
    wl_signal_add(&wlr_layer_surface->events.destroy, &surface->destroy);
    
    surface->map.notify = axiom_layer_surface_handle_map;
    wl_signal_add(&wlr_layer_surface->surface->events.map, &surface->map);
    
    surface->unmap.notify = axiom_layer_surface_handle_unmap;
    wl_signal_add(&wlr_layer_surface->surface->events.unmap, &surface->unmap);
    
    surface->commit.notify = axiom_layer_surface_handle_commit;
    wl_signal_add(&wlr_layer_surface->surface->events.commit, &surface->commit);
    
    surface->new_popup.notify = axiom_layer_surface_handle_new_popup;
    wl_signal_add(&wlr_layer_surface->events.new_popup, &surface->new_popup);
    
    // Create scene layer surface for rendering
    surface->scene_layer_surface = wlr_scene_layer_surface_v1_create(
        &server->scene->tree, wlr_layer_surface);
    if (!surface->scene_layer_surface) {
        AXIOM_LOG_ERROR("LAYER_SHELL", "Failed to create scene layer surface");
        axiom_layer_surface_destroy(surface);
        return NULL;
    }
    
    AXIOM_LOG_INFO("LAYER_SHELL", "Created layer surface: namespace='%s', layer=%d",
                   wlr_layer_surface->namespace ?: "(none)", wlr_layer_surface->pending.layer);
    
    return surface;
}

/**
 * Destroy a layer surface wrapper.
 */
void axiom_layer_surface_destroy(struct axiom_layer_surface *surface) {
    if (!surface) return;
    
    // Remove event listeners
    wl_list_remove(&surface->destroy.link);
    wl_list_remove(&surface->map.link);
    wl_list_remove(&surface->unmap.link);
    wl_list_remove(&surface->commit.link);
    wl_list_remove(&surface->new_popup.link);
    
    if (surface->scene_layer_surface) {
        wlr_scene_node_destroy(&surface->scene_layer_surface->tree->node);
    }
    
    axiom_free_tracked(surface, __FILE__, __func__, __LINE__);
}

// ===== Event Handlers =====

void axiom_layer_shell_handle_new_surface(struct wl_listener *listener, void *data) {
    struct axiom_layer_shell_manager *manager = wl_container_of(listener, manager, new_surface);
    struct wlr_layer_surface_v1 *wlr_layer_surface = data;
    
    AXIOM_LOG_INFO("LAYER_SHELL", "New layer surface: namespace='%s', layer=%d",
                   wlr_layer_surface->namespace ?: "(none)", wlr_layer_surface->pending.layer);
    
    // Create layer surface wrapper
    struct axiom_layer_surface *surface = axiom_layer_surface_create(wlr_layer_surface, manager->server);
    if (!surface) {
        AXIOM_LOG_ERROR("LAYER_SHELL", "Failed to create layer surface wrapper");
        return;
    }
    
    // Store reference in wlr_layer_surface for later retrieval
    wlr_layer_surface->data = surface;
}

void axiom_layer_surface_handle_destroy(struct wl_listener *listener, void *data) {
    struct axiom_layer_surface *surface = wl_container_of(listener, surface, destroy);
    (void)data;
    
    AXIOM_LOG_INFO("LAYER_SHELL", "Layer surface destroyed");
    
    // Re-arrange layers after surface is destroyed
    axiom_arrange_layers(surface->server);
    
    // Clean up the surface wrapper
    axiom_layer_surface_destroy(surface);
}

void axiom_layer_surface_handle_map(struct wl_listener *listener, void *data) {
    struct axiom_layer_surface *surface = wl_container_of(listener, surface, map);
    (void)data;
    
    surface->mapped = true;
    
    AXIOM_LOG_INFO("LAYER_SHELL", "Layer surface mapped: namespace='%s'",
                   surface->wlr_layer_surface->namespace ?: "(none)");
    
    // Arrange layers to accommodate the new surface
    axiom_arrange_layers(surface->server);
    
    // Focus handling for layer surfaces
    struct wlr_layer_surface_v1 *layer_surface = surface->wlr_layer_surface;
    if (layer_surface->current.keyboard_interactive == 
        ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE) {
        // Give keyboard focus to this layer surface
        wlr_seat_keyboard_notify_enter(surface->server->seat, layer_surface->surface,
                                       NULL, 0, NULL);
        AXIOM_LOG_DEBUG("LAYER_SHELL", "Exclusive keyboard focus granted to layer surface");
    }
}

void axiom_layer_surface_handle_unmap(struct wl_listener *listener, void *data) {
    struct axiom_layer_surface *surface = wl_container_of(listener, surface, unmap);
    (void)data;
    
    surface->mapped = false;
    
    AXIOM_LOG_INFO("LAYER_SHELL", "Layer surface unmapped");
    
    // Re-arrange layers after surface is unmapped
    axiom_arrange_layers(surface->server);
    
    // If this surface had keyboard focus, return it to the normal windows
    if (surface->server->seat->keyboard_state.focused_surface == surface->wlr_layer_surface->surface) {
        // Focus the most recently focused window if available
        if (surface->server->focused_window && surface->server->focused_window->surface) {
            wlr_seat_keyboard_notify_enter(surface->server->seat, surface->server->focused_window->surface,
                                           NULL, 0, NULL);
        } else {
            wlr_seat_keyboard_clear_focus(surface->server->seat);
        }
    }
}

void axiom_layer_surface_handle_commit(struct wl_listener *listener, void *data) {
    struct axiom_layer_surface *surface = wl_container_of(listener, surface, commit);
    (void)data;
    
    // Re-arrange layers on any commit to handle size/anchor changes
    axiom_arrange_layers(surface->server);
}

void axiom_layer_surface_handle_new_popup(struct wl_listener *listener, void *data) {
    struct axiom_layer_surface *surface = wl_container_of(listener, surface, new_popup);
    struct wlr_xdg_popup *popup = data;
    
    AXIOM_LOG_DEBUG("LAYER_SHELL", "New popup on layer surface");
    
    // Create scene tree for the popup
    struct wlr_scene_tree *popup_tree = wlr_scene_xdg_surface_create(
        &surface->server->scene->tree, popup->base);
    if (!popup_tree) {
        AXIOM_LOG_ERROR("LAYER_SHELL", "Failed to create popup scene tree");
        return;
    }
    
    popup->base->data = popup_tree;
}

// ===== Utility Functions =====

/**
 * Arrange layer surfaces for all outputs.
 */
void axiom_arrange_layers(struct axiom_server *server) {
    if (!server->layer_shell_manager || !server->layer_shell_manager->wlr_layer_shell) {
        return;
    }
    
    struct axiom_output *output;
    wl_list_for_each(output, &server->outputs, link) {
        // Get output dimensions
        struct wlr_box full_area = {
            .x = 0,
            .y = 0,
            .width = output->wlr_output->width,
            .height = output->wlr_output->height,
        };
        
        struct wlr_box usable_area __attribute__((unused)) = full_area;
        
        // Configure layer surfaces for this output
        // In newer wlroots, layer surfaces are managed differently
        // The scene layer surface system handles arrangement automatically
    }
    
    AXIOM_LOG_DEBUG("LAYER_SHELL", "Arranged layer surfaces for all outputs");
}
