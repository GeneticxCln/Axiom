#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <wlr/util/box.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_scene.h>
#include "window_manager.h"
#include "axiom.h"
#include "logging.h"
// Fallback memory functions if memory.h is not available
#ifndef axiom_calloc_tracked
#define axiom_calloc_tracked(count, size, type, file, func, line) calloc(count, size)
#define axiom_free_tracked(ptr, file, func, line) free(ptr)
#define AXIOM_MEM_TYPE_WINDOW_MANAGER 0
#define AXIOM_MEM_TYPE_WINDOW_STATE 0
#define AXIOM_MEM_TYPE_WINDOW_GEOMETRY 0
#define AXIOM_MEM_TYPE_WINDOW_LAYOUT 0
#define AXIOM_MEM_TYPE_FOCUS 0
#endif

// Forward declare missing functions
void axiom_window_manager_arrange_all(struct axiom_window_manager *manager);
void axiom_window_manager_add_window(struct axiom_window_manager *manager, struct axiom_window *window);
void axiom_window_manager_remove_window(struct axiom_window_manager *manager, struct axiom_window *window);
#include "constants.h"
#include "animation.h"
#include "focus.h"
#include "config.h"

void axiom_update_window_decorations(struct axiom_window *window) {
    if (!window || !window->scene_tree) {
        return;
    }

    // Create decoration tree if it doesn't exist
    if (!window->decoration_tree) {
        window->decoration_tree = wlr_scene_tree_create(window->scene_tree);
        if (!window->decoration_tree) {
            AXIOM_LOG_ERROR("Failed to create decoration tree");
            return;
        }
    }

    // Remove existing decorations
    if (window->title_bar) {
        wlr_scene_node_destroy(&window->title_bar->node);
        window->title_bar = NULL;
    }
    if (window->border_top) {
        wlr_scene_node_destroy(&window->border_top->node);
        window->border_top = NULL;
    }
    if (window->border_bottom) {
        wlr_scene_node_destroy(&window->border_bottom->node);
        window->border_bottom = NULL;
    }
    if (window->border_left) {
        wlr_scene_node_destroy(&window->border_left->node);
        window->border_left = NULL;
    }
    if (window->border_right) {
        wlr_scene_node_destroy(&window->border_right->node);
        window->border_right = NULL;
    }

    // Title bar colors
    const float title_bar_focused[4] = {0.2f, 0.2f, 0.2f, 0.9f};
    const float title_bar_unfocused[4] = {0.1f, 0.1f, 0.1f, 0.9f};
    const float border_focused[4] = {0.4f, 0.6f, 1.0f, 1.0f};
    const float border_unfocused[4] = {0.3f, 0.3f, 0.3f, 1.0f};

    // Choose colors based on focus state
    const float *title_color = window->is_focused ? title_bar_focused : title_bar_unfocused;
    const float *border_color = window->is_focused ? border_focused : border_unfocused;

    // Create title bar
    const int title_height = AXIOM_TITLE_BAR_HEIGHT;
    const int border_width = AXIOM_BORDER_WIDTH;
    window->title_bar = wlr_scene_rect_create(window->decoration_tree, 
                                              window->width, title_height, title_color);
    if (window->title_bar) {
        wlr_scene_node_set_position(&window->title_bar->node, 0, -title_height);
    }

// Enhanced Window Decorations
void axiom_set_window_decorations(struct axiom_window *window, bool focused) {
    if (!window || !window->scene_tree) return;

    // Colors and dimensions
    const float title_bar_focused[] = {0.2f, 0.3f, 0.4f, 0.9f};
    const float title_bar_unfocused[] = {0.1f, 0.1f, 0.1f, 0.7f};
    const float *title_color = focused ? title_bar_focused : title_bar_unfocused;

    const int title_height = AXIOM_TITLE_BAR_HEIGHT;
    const int border_width = AXIOM_BORDER_WIDTH;

    // Remove existing decorations
    if (window->title_bar) {
        wlr_scene_node_destroy(&window->title_bar->node);
        window->title_bar = NULL;
    }

    // Create focused or unfocused decorations
    window->title_bar = wlr_scene_rect_create(window->scene_tree, window->width, title_height, title_color);
    if (window->title_bar) {
        wlr_scene_node_set_position(&window->title_bar->node, 0, -title_height);
    }

    // Callbacks for button actions (close/minimize/maximize)
    // Buttons will use event handlers for interaction
    axiom_create_title_bar_buttons(window);
    
    // Ensure focus state is reflected
    axiom_update_title_bar_buttons(window);
}

// Enhan
    // Top border
    window->border_top = wlr_scene_rect_create(window->decoration_tree,
                                               window->width, border_width, border_color);
    if (window->border_top) {
        wlr_scene_node_set_position(&window->border_top->node, 0, -border_width);
    }

    // Bottom border
    window->border_bottom = wlr_scene_rect_create(window->decoration_tree,
                                                   window->width, border_width, border_color);
    if (window->border_bottom) {
        wlr_scene_node_set_position(&window->border_bottom->node, 0, window->height);
    }

    // Left border
    window->border_left = wlr_scene_rect_create(window->decoration_tree,
                                                border_width, window->height, border_color);
    if (window->border_left) {
        wlr_scene_node_set_position(&window->border_left->node, -border_width, 0);
    }

    // Right border
    window->border_right = wlr_scene_rect_create(window->decoration_tree,
                                                 border_width, window->height, border_color);
    if (window->border_right) {
        wlr_scene_node_set_position(&window->border_right->node, window->width, 0);
    }

    // Create title bar buttons
    axiom_create_title_bar_buttons(window);
    axiom_update_title_bar_buttons(window);
}


// Tiling layout functionality (merged from tiling.c)
static enum axiom_layout_type current_layout = AXIOM_LAYOUT_MASTER_STACK;
static float master_ratio = AXIOM_MASTER_RATIO_DEFAULT;

void axiom_set_layout(enum axiom_layout_type layout) {
    current_layout = layout;
    AXIOM_LOG_INFO("Layout changed to: %d", layout);
}

enum axiom_layout_type axiom_get_layout(void) {
    return current_layout;
}

void axiom_adjust_master_ratio(float delta) {
    master_ratio += delta;
    if (master_ratio < AXIOM_MASTER_RATIO_MIN) master_ratio = AXIOM_MASTER_RATIO_MIN;
    if (master_ratio > AXIOM_MASTER_RATIO_MAX) master_ratio = AXIOM_MASTER_RATIO_MAX;
    AXIOM_LOG_INFO("Master ratio adjusted to: %.2f", master_ratio);
}

const char* axiom_get_layout_name(void) {
    switch (current_layout) {
        case AXIOM_LAYOUT_GRID:
            return "Grid";
        case AXIOM_LAYOUT_MASTER_STACK:
            return "Master-Stack";
        case AXIOM_LAYOUT_SPIRAL:
            return "Spiral";
        case AXIOM_LAYOUT_FLOATING:
            return "Floating";
        default:
            return "Unknown";
    }
}

static void window_manager_schedule_layout_update(struct axiom_window_manager *manager);
static int window_manager_layout_timer_handler(void *data);
static void window_manager_calculate_tiled_layout(struct axiom_window_manager *manager);
static void window_manager_apply_constraints(struct axiom_window_constraints *constraints, 
                                            int *width, int *height);
static bool window_manager_validate_geometry(const struct axiom_window_geometry *geometry);

void axiom_calculate_window_layout(struct axiom_server *server, int index, 
                                   int *x, int *y, int *width, int *height) {
    if (!server || !x || !y || !width || !height) return;

    struct axiom_window *window;
    wl_list_for_each(window, &server->windows, link) {
        if (index-- == 0) {
            *x = window->x;
            *y = window->y;
            *width = window->width;
            *height = window->height;
            return;
        }
    }
}

void axiom_calculate_window_layout_advanced(struct axiom_server *server, int index, 
                                           int *x, int *y, int *width, int *height) {
    if (!server || !x || !y || !width || !height) return;

    // In advanced layout, we might offset or transform window sizes
    axiom_calculate_window_layout(server, index, x, y, width, height);
    *x += AXIOM_DEFAULT_GAP_SIZE; // Example offset
    *y += AXIOM_DEFAULT_GAP_SIZE; // Example offset
    *width -= 2 * AXIOM_DEFAULT_GAP_SIZE; // Example size change
    *height -= 2 * AXIOM_DEFAULT_GAP_SIZE; // Example size change
}
static void window_manager_schedule_layout_update(struct axiom_window_manager *manager);
static int window_manager_layout_timer_handler(void *data);
static void window_manager_calculate_tiled_layout(struct axiom_window_manager *manager);
static void window_manager_apply_constraints(struct axiom_window_constraints *constraints, 
                                            int *width, int *height);
static bool window_manager_validate_geometry(const struct axiom_window_geometry *geometry);

// Window manager creation and destruction

struct axiom_window_manager *axiom_window_manager_create(struct axiom_server *server) {
    if (!server || !server->wl_event_loop) {
        AXIOM_LOG_ERROR("WINDOW_MGR", "Invalid server or event loop");
        return NULL;
    }

    struct axiom_window_manager *manager = axiom_calloc_tracked(1, 
        sizeof(struct axiom_window_manager), AXIOM_MEM_TYPE_WINDOW_MANAGER, 
        __FILE__, __func__, __LINE__);
    
    if (!manager) {
        AXIOM_LOG_ERROR("WINDOW_MGR", "Failed to allocate window manager");
        return NULL;
    }

    manager->server = server;
    
    // Initialize window lists
    wl_list_init(&manager->all_windows);
    wl_list_init(&manager->mapped_windows);
    wl_list_init(&manager->floating_windows);
    wl_list_init(&manager->tiled_windows);
    wl_list_init(&manager->urgent_windows);
    wl_list_init(&manager->focus_history);

    // Initialize focus state
    manager->focused_window = NULL;
    manager->last_focused_window = NULL;

    // Set default layout parameters
    manager->workspace_width = AXIOM_DEFAULT_WORKSPACE_WIDTH;   // Will be updated from actual output
    manager->workspace_height = AXIOM_DEFAULT_WORKSPACE_HEIGHT;
    manager->title_bar_height = AXIOM_TITLE_BAR_HEIGHT;
    manager->border_width = AXIOM_BORDER_WIDTH;
    manager->gap_size = AXIOM_DEFAULT_GAP_SIZE;

    // Set default configuration
    manager->auto_focus_new_windows = true;
    manager->focus_follows_mouse = false;
    manager->click_to_focus = true;
    manager->max_focus_history = AXIOM_MAX_FOCUS_HISTORY;

    // Initialize performance optimization flags
    manager->layout_calculation_pending = false;
    manager->geometry_update_pending = false;

    // Create layout update timer
    manager->layout_timer = wl_event_loop_add_timer(server->wl_event_loop,
        window_manager_layout_timer_handler, manager);
    
    if (!manager->layout_timer) {
        AXIOM_LOG_WARN("WINDOW_MGR", "Failed to create layout timer");
    }

    AXIOM_LOG_INFO("WINDOW_MGR", "Window manager initialized successfully");
    return manager;
}

void axiom_window_manager_destroy(struct axiom_window_manager *manager) {
    if (!manager) return;

    // Clean up timer
    if (manager->layout_timer) {
        wl_event_source_remove(manager->layout_timer);
        manager->layout_timer = NULL;
    }

    // Clear focus history
    struct axiom_focus_entry *entry, *tmp_entry;
    wl_list_for_each_safe(entry, tmp_entry, &manager->focus_history, link) {
        wl_list_remove(&entry->link);
        axiom_free_tracked(entry, __FILE__, __func__, __LINE__);
    }

    // Note: We don't free windows here as they should be cleaned up
    // by their respective destructors

    axiom_free_tracked(manager, __FILE__, __func__, __LINE__);
    AXIOM_LOG_INFO("WINDOW_MGR", "Window manager destroyed");
}

// Window lifecycle management

void axiom_window_manager_add_window(struct axiom_window_manager *manager, struct axiom_window *window) {
    if (!manager || !window) {
        AXIOM_LOG_ERROR("WINDOW_MGR", "Invalid parameters for add_window");
        return;
    }

    // Add to all windows list
    wl_list_insert(&manager->all_windows, &window->link);

    // Initialize window state and geometry if not already done
    if (!window->state) {
        window->state = axiom_calloc_tracked(1, sizeof(struct axiom_window_state),
            AXIOM_MEM_TYPE_WINDOW_STATE, __FILE__, __func__, __LINE__);
        if (window->state) {
            axiom_window_state_init(window->state);
        }
    }

    if (!window->geometry) {
        window->geometry = axiom_calloc_tracked(1, sizeof(struct axiom_window_geometry),
            AXIOM_MEM_TYPE_WINDOW_GEOMETRY, __FILE__, __func__, __LINE__);
        if (window->geometry) {
            axiom_window_geometry_init(window->geometry, 0, 0, 
                AXIOM_DEFAULT_WINDOW_WIDTH, AXIOM_DEFAULT_WINDOW_HEIGHT);
        }
    }

    if (!window->layout) {
        window->layout = axiom_calloc_tracked(1, sizeof(struct axiom_window_layout),
            AXIOM_MEM_TYPE_WINDOW_LAYOUT, __FILE__, __func__, __LINE__);
        if (window->layout) {
            axiom_window_layout_init(window->layout);
        }
    }

    AXIOM_LOG_DEBUG("WINDOW_MGR", "Added window %p to window manager", (void *)window);
}

void axiom_window_manager_remove_window(struct axiom_window_manager *manager, struct axiom_window *window) {
    if (!manager || !window) return;

    // Remove from all lists
    wl_list_remove(&window->link);

    // Remove from focus history
    struct axiom_focus_entry *entry, *tmp_entry;
    wl_list_for_each_safe(entry, tmp_entry, &manager->focus_history, link) {
        if (entry->window == window) {
            wl_list_remove(&entry->link);
            axiom_free_tracked(entry, __FILE__, __func__, __LINE__);
        }
    }

    // Update focused window if necessary
    if (manager->focused_window == window) {
        // Try to focus the most recent window
        if (!wl_list_empty(&manager->focus_history)) {
            entry = wl_container_of(manager->focus_history.next, entry, link);
            axiom_window_manager_focus_window(manager, entry->window);
        } else {
            manager->focused_window = NULL;
        }
    }

    if (manager->last_focused_window == window) {
        manager->last_focused_window = NULL;
    }

    // Schedule layout update
    window_manager_schedule_layout_update(manager);

    AXIOM_LOG_DEBUG("WINDOW_MGR", "Removed window %p from window manager", (void *)window);
}

void axiom_window_manager_map_window(struct axiom_window_manager *manager, struct axiom_window *window) {
    if (!manager || !window) return;

    // Add to mapped windows list
    wl_list_insert(&manager->mapped_windows, &window->mapped_link);

    // Set mapped state
    if (window->state) {
        axiom_window_set_state(window, AXIOM_WINDOW_STATE_MAPPED, true);
    }

    // Determine if window should be tiled or floating
    bool should_tile = axiom_window_manager_can_tile_window(manager, window);
    
    if (should_tile) {
        wl_list_insert(&manager->tiled_windows, &window->tiled_link);
        if (window->layout) {
            window->layout->mode = AXIOM_WM_LAYOUT_TILED;
        }
        if (window->state) {
            axiom_window_set_state(window, AXIOM_WINDOW_STATE_TILED, true);
            axiom_window_set_state(window, AXIOM_WINDOW_STATE_FLOATING, false);
        }
    } else {
        wl_list_insert(&manager->floating_windows, &window->floating_link);
        if (window->layout) {
            window->layout->mode = AXIOM_WM_LAYOUT_FLOATING;
        }
        if (window->state) {
            axiom_window_set_state(window, AXIOM_WINDOW_STATE_FLOATING, true);
            axiom_window_set_state(window, AXIOM_WINDOW_STATE_TILED, false);
        }
    }

    // Calculate initial geometry
    if (window->geometry) {
        axiom_window_calculate_geometry(manager, window, window->geometry);
        axiom_window_apply_geometry(window, window->geometry);
    }

    // Auto-focus if enabled
    if (manager->auto_focus_new_windows) {
        axiom_window_manager_focus_window(manager, window);
    }

    // Schedule layout update
    window_manager_schedule_layout_update(manager);

    AXIOM_LOG_INFO("WINDOW_MGR", "Mapped window %p (%s)", (void *)window, 
                   should_tile ? "tiled" : "floating");
}

void axiom_window_manager_unmap_window(struct axiom_window_manager *manager, struct axiom_window *window) {
    if (!manager || !window) return;

    // Remove from mapped lists
    wl_list_remove(&window->mapped_link);
    wl_list_remove(&window->tiled_link);
    wl_list_remove(&window->floating_link);

    // Clear mapped state
    if (window->state) {
        axiom_window_set_state(window, AXIOM_WINDOW_STATE_MAPPED, false);
    }

    // Update focus if this window was focused
    if (manager->focused_window == window) {
        axiom_window_manager_focus_next(manager);
    }

    // Schedule layout update
    window_manager_schedule_layout_update(manager);

    AXIOM_LOG_DEBUG("WINDOW_MGR", "Unmapped window %p", (void *)window);
}

// Window positioning and geometry

bool axiom_window_calculate_geometry(struct axiom_window_manager *manager, 
                                    struct axiom_window *window,
                                    struct axiom_window_geometry *geometry) {
    if (!manager || !window || !geometry || !window->layout) {
        return false;
    }

    int x, y, width, height;

    switch (window->layout->mode) {
        case AXIOM_WM_LAYOUT_TILED:
            axiom_window_layout_calculate_tiled_position(manager, window, window->layout,
                                                        &x, &y, &width, &height);
            break;
        
        case AXIOM_WM_LAYOUT_FLOATING:
            axiom_window_layout_calculate_floating_position(manager, window, window->layout,
                                                           &x, &y, &width, &height);
            break;
        
        case AXIOM_WM_LAYOUT_MAXIMIZED:
            x = 0;
            y = manager->title_bar_height;
            width = manager->workspace_width;
            height = manager->workspace_height - manager->title_bar_height;
            break;
        
        case AXIOM_WM_LAYOUT_FULLSCREEN:
            x = y = 0;
            width = manager->workspace_width;
            height = manager->workspace_height;
            break;
        
        default:
            AXIOM_LOG_WARN("WINDOW_MGR", "Unknown layout mode %d", window->layout->mode);
            return false;
    }

    // Update geometry
    geometry->x = x;
    geometry->y = y;
    geometry->width = width;
    geometry->height = height;

    // Apply constraints
    if (geometry->constraints.has_min_size || geometry->constraints.has_max_size) {
        window_manager_apply_constraints(&geometry->constraints, 
                                       &geometry->width, &geometry->height);
    }

    // Update the various boxes
    axiom_window_geometry_update_boxes(geometry, manager->title_bar_height, 
                                      manager->border_width);

    // Validate the final geometry
    if (!window_manager_validate_geometry(geometry)) {
        AXIOM_LOG_WARN("WINDOW_MGR", "Invalid geometry calculated for window %p", (void *)window);
        return false;
    }

    return true;
}

void axiom_window_apply_geometry(struct axiom_window *window, 
                                const struct axiom_window_geometry *geometry) {
    if (!window || !geometry) return;

    // Update window position in scene graph
    if (window->scene_tree) {
        wlr_scene_node_set_position(&window->scene_tree->node, geometry->x, geometry->y);
    }

    // Send configure to client
    if (window->xdg_toplevel) {
        uint32_t serial = wlr_xdg_toplevel_set_size(window->xdg_toplevel, 
                                                    geometry->width, geometry->height);
        
        // Store configure serial for tracking
        if (window->geometry) {
            window->geometry->pending_configure_serial = serial;
        }
        
        AXIOM_LOG_DEBUG("WINDOW_MGR", "Configured window %p: %dx%d+%d+%d (serial=%u)",
                       (void *)window, geometry->width, geometry->height, 
                       geometry->x, geometry->y, serial);
    }

    // Update convenience fields
    window->x = geometry->x;
    window->y = geometry->y;
    window->width = geometry->width;
    window->height = geometry->height;
}

void axiom_window_validate_geometry(struct axiom_window *window, 
                                   struct axiom_window_geometry *geometry) {
    if (!window || !geometry) return;

    // Ensure minimum dimensions
    if (geometry->width < AXIOM_MIN_WINDOW_WIDTH) {
        geometry->width = AXIOM_MIN_WINDOW_WIDTH;
    }
    if (geometry->height < AXIOM_MIN_WINDOW_HEIGHT) {
        geometry->height = AXIOM_MIN_WINDOW_HEIGHT;
    }

    // Apply client constraints if available
    if (window->xdg_toplevel) {
        struct wlr_xdg_toplevel_state *state = &window->xdg_toplevel->current;
        
        if (state->min_width > 0 && geometry->width < state->min_width) {
            geometry->width = state->min_width;
        }
        if (state->min_height > 0 && geometry->height < state->min_height) {
            geometry->height = state->min_height;
        }
        if (state->max_width > 0 && geometry->width > state->max_width) {
            geometry->width = state->max_width;
        }
        if (state->max_height > 0 && geometry->height > state->max_height) {
            geometry->height = state->max_height;
        }
    }
}

bool axiom_window_geometry_changed(const struct axiom_window_geometry *old_geo,
                                  const struct axiom_window_geometry *new_geo) {
    if (!old_geo || !new_geo) return true;

    return (old_geo->x != new_geo->x ||
            old_geo->y != new_geo->y ||
            old_geo->width != new_geo->width ||
            old_geo->height != new_geo->height);
}

// Window state management

void axiom_window_set_state(struct axiom_window *window, enum axiom_window_state_flags state, bool enabled) {
    if (!window || !window->state) return;

    uint32_t old_flags = window->state->state_flags;

    if (enabled) {
        window->state->state_flags |= state;
    } else {
        window->state->state_flags &= ~state;
    }

    // Handle state-specific logic
    switch (state) {
        case AXIOM_WINDOW_STATE_FOCUSED:
            window->state->is_focused = enabled;
            if (enabled) {
                window->state->last_focus_time = time(NULL);
            }
            break;
            
        case AXIOM_WINDOW_STATE_MAPPED:
            window->state->is_visible = enabled;
            break;
            
        case AXIOM_WINDOW_STATE_MAXIMIZED:
            if (enabled && window->geometry && !window->geometry->has_saved_geometry) {
                axiom_window_geometry_save(window->geometry);
            }
            break;
            
        case AXIOM_WINDOW_STATE_FULLSCREEN:
            if (enabled && window->geometry && !window->geometry->has_saved_geometry) {
                axiom_window_geometry_save(window->geometry);
            }
            break;
            
        default:
            break;
    }

    if (old_flags != window->state->state_flags) {
        AXIOM_LOG_DEBUG("WINDOW_MGR", "Window %p state changed: 0x%x -> 0x%x", 
                       (void *)window, old_flags, window->state->state_flags);
    }
}

bool axiom_window_has_state(struct axiom_window *window, enum axiom_window_state_flags state) {
    if (!window || !window->state) return false;
    return (window->state->state_flags & state) != 0;
}

void axiom_window_save_state(struct axiom_window *window) {
    if (!window || !window->state) return;
    
    window->state->previous_state = window->state->state_flags;
}

void axiom_window_restore_state(struct axiom_window *window) {
    if (!window || !window->state) return;
    
    window->state->state_flags = window->state->previous_state;
}

void axiom_window_update_state(struct axiom_window *window) {
    if (!window || !window->state) return;

    // Update convenience flags
    window->is_focused = axiom_window_has_state(window, AXIOM_WINDOW_STATE_FOCUSED);
    window->is_fullscreen = axiom_window_has_state(window, AXIOM_WINDOW_STATE_FULLSCREEN);
    window->is_maximized = axiom_window_has_state(window, AXIOM_WINDOW_STATE_MAXIMIZED);
    window->is_tiled = axiom_window_has_state(window, AXIOM_WINDOW_STATE_TILED);
}

// Focus management functions

void axiom_window_manager_focus_window(struct axiom_window_manager *manager, struct axiom_window *window) {
    if (!manager) return;

    // Unfocus current window
    if (manager->focused_window && manager->focused_window != window) {
        axiom_window_set_state(manager->focused_window, AXIOM_WINDOW_STATE_FOCUSED, false);
        if (manager->focused_window->xdg_toplevel) {
            wlr_xdg_toplevel_set_activated(manager->focused_window->xdg_toplevel, false);
        }
        // Update decorations for unfocused window
        axiom_update_window_decorations(manager->focused_window);
        // Note: Opacity control would need custom implementation in wlroots 0.19
        // wlr_scene_node_set_opacity(&manager->focused_window->scene_tree->node, 0.8);
    }

    // Update focus pointers
    manager->last_focused_window = manager->focused_window;
    manager->focused_window = window;

    if (window) {
        // Focus new window
        axiom_window_set_state(window, AXIOM_WINDOW_STATE_FOCUSED, true);
        if (window->xdg_toplevel) {
            wlr_xdg_toplevel_set_activated(window->xdg_toplevel, true);
        }

        // Update focus history
        axiom_window_manager_update_focus_history(manager, window);

        // Update decorations for focused window
        axiom_update_window_decorations(window);
        // Note: Opacity control would need custom implementation in wlroots 0.19
        // wlr_scene_node_set_opacity(&window->scene_tree->node, 1.0);

        // Update keyboard focus
        if (manager->server && manager->server->seat && window->surface) {
            wlr_seat_keyboard_notify_enter(manager->server->seat, window->surface,
                manager->server->seat->keyboard_state.keyboard->keycodes,
                manager->server->seat->keyboard_state.keyboard->num_keycodes,
                &manager->server->seat->keyboard_state.keyboard->modifiers);
        }

        AXIOM_LOG_DEBUG("WINDOW_MGR", "Focused window %p", (void *)window);
    } else {
        // Clear keyboard focus
        if (manager->server && manager->server->seat) {
            wlr_seat_keyboard_clear_focus(manager->server->seat);
        }
        AXIOM_LOG_DEBUG("WINDOW_MGR", "Cleared window focus");
    }
}

void axiom_window_manager_focus_next(struct axiom_window_manager *manager) {
    if (!manager) return;

    struct axiom_window *next_window = NULL;

    if (manager->focused_window) {
        // Find next window in mapped list
        struct wl_list *next = manager->focused_window->mapped_link.next;
        if (next == &manager->mapped_windows) {
            next = manager->mapped_windows.next;
        }
        
        if (next != &manager->mapped_windows) {
            next_window = wl_container_of(next, next_window, mapped_link);
        }
    } else if (!wl_list_empty(&manager->mapped_windows)) {
        // Focus first mapped window
        next_window = wl_container_of(manager->mapped_windows.next, next_window, mapped_link);
    }

    if (next_window && next_window != manager->focused_window) {
        axiom_window_manager_focus_window(manager, next_window);
    }
}

void axiom_window_manager_focus_prev(struct axiom_window_manager *manager) {
    if (!manager) return;

    struct axiom_window *prev_window = NULL;

    if (manager->focused_window) {
        // Find previous window in mapped list
        struct wl_list *prev = manager->focused_window->mapped_link.prev;
        if (prev == &manager->mapped_windows) {
            prev = manager->mapped_windows.prev;
        }
        
        if (prev != &manager->mapped_windows) {
            prev_window = wl_container_of(prev, prev_window, mapped_link);
        }
    } else if (!wl_list_empty(&manager->mapped_windows)) {
        // Focus last mapped window
        prev_window = wl_container_of(manager->mapped_windows.prev, prev_window, mapped_link);
    }

    if (prev_window && prev_window != manager->focused_window) {
        axiom_window_manager_focus_window(manager, prev_window);
    }
}

struct axiom_window *axiom_window_manager_find_focusable_window(struct axiom_window_manager *manager) {
    if (!manager) return NULL;

    struct axiom_window *window;
    wl_list_for_each(window, &manager->mapped_windows, mapped_link) {
        if (window->state && axiom_window_has_state(window, AXIOM_WINDOW_STATE_MAPPED) &&
            !axiom_window_has_state(window, AXIOM_WINDOW_STATE_MINIMIZED)) {
            return window;
        }
    }

    return NULL;
}

void axiom_window_manager_update_focus_history(struct axiom_window_manager *manager, struct axiom_window *window) {
    if (!manager || !window) return;

    // Remove existing entry for this window
    struct axiom_focus_entry *entry, *tmp;
    wl_list_for_each_safe(entry, tmp, &manager->focus_history, link) {
        if (entry->window == window) {
            wl_list_remove(&entry->link);
            axiom_free_tracked(entry, __FILE__, __func__, __LINE__);
        }
    }

    // Add new entry at head
    entry = axiom_calloc_tracked(1, sizeof(struct axiom_focus_entry), 
                                AXIOM_MEM_TYPE_FOCUS, __FILE__, __func__, __LINE__);
    if (entry) {
        entry->window = window;
        entry->last_focus_time = time(NULL);
        wl_list_insert(&manager->focus_history, &entry->link);

        // Limit history size
        int count = 0;
        wl_list_for_each(entry, &manager->focus_history, link) {
            count++;
            if (count > manager->max_focus_history) {
                wl_list_remove(&entry->link);
                axiom_free_tracked(entry, __FILE__, __func__, __LINE__);
                break;
            }
        }
    }
}

// Layout and arrangement

void axiom_window_manager_arrange_all(struct axiom_window_manager *manager) {
    if (!manager) return;

    // Calculate layout for all tiled windows
    window_manager_calculate_tiled_layout(manager);

    // Apply geometry to all windows
    struct axiom_window *window;
    wl_list_for_each(window, &manager->mapped_windows, mapped_link) {
        if (window->geometry) {
            if (axiom_window_calculate_geometry(manager, window, window->geometry)) {
                axiom_window_apply_geometry(window, window->geometry);
            }
        }
    }

    manager->layout_calculation_pending = false;
    AXIOM_LOG_DEBUG("WINDOW_MGR", "Arranged all windows");
}

void axiom_window_manager_arrange_workspace(struct axiom_window_manager *manager, int workspace) {
    if (!manager) return;
    
    (void)workspace; // Mark as intentionally unused

    // For now, arrange all windows as we don't have workspace filtering yet
    axiom_window_manager_arrange_all(manager);
}

void axiom_window_manager_arrange_tiled_windows(struct axiom_window_manager *manager) {
    if (!manager) return;

    window_manager_calculate_tiled_layout(manager);

    struct axiom_window *window;
    wl_list_for_each(window, &manager->tiled_windows, tiled_link) {
        if (window->geometry) {
            if (axiom_window_calculate_geometry(manager, window, window->geometry)) {
                axiom_window_apply_geometry(window, window->geometry);
            }
        }
    }
}

void axiom_window_manager_update_window_layout(struct axiom_window_manager *manager, struct axiom_window *window) {
    if (!manager || !window || !window->geometry) return;

    if (axiom_window_calculate_geometry(manager, window, window->geometry)) {
        axiom_window_apply_geometry(window, window->geometry);
    }
}

// Utility functions

bool axiom_window_manager_can_tile_window(struct axiom_window_manager *manager, struct axiom_window *window) {
    if (!manager || !window) return false;

    // Don't tile windows that request floating
    if (window->state && axiom_window_has_state(window, AXIOM_WINDOW_STATE_FLOATING)) {
        return false;
    }

    // Check XDG toplevel properties
    if (window->xdg_toplevel) {
        struct wlr_xdg_toplevel_state *state = &window->xdg_toplevel->current;
        
        // Don't tile modal dialogs (modal field not available in wlroots 0.19)
        // In wlroots 0.19, modal state would need to be checked differently
        
        // Don't tile windows with specific size constraints that suggest floating
        if (state->min_width > 0 && state->max_width > 0 && 
            state->min_width == state->max_width &&
            state->min_height > 0 && state->max_height > 0 &&
            state->min_height == state->max_height) {
            return false;
        }
    }

    return true;
}

void axiom_window_manager_set_workspace_geometry(struct axiom_window_manager *manager, 
                                                 int width, int height) {
    if (!manager) return;

    bool changed = (manager->workspace_width != width || manager->workspace_height != height);
    
    manager->workspace_width = width;
    manager->workspace_height = height;

    if (changed) {
        window_manager_schedule_layout_update(manager);
        AXIOM_LOG_INFO("WINDOW_MGR", "Workspace geometry updated to %dx%d", width, height);
    }
}

// Helper functions for geometry management

void axiom_window_geometry_init(struct axiom_window_geometry *geometry, int x, int y, int width, int height) {
    if (!geometry) return;

    memset(geometry, 0, sizeof(struct axiom_window_geometry));
    
    geometry->x = x;
    geometry->y = y;
    geometry->width = width;
    geometry->height = height;
    
    geometry->has_saved_geometry = false;
    geometry->configure_serial = 0;
    geometry->pending_configure_serial = 0;
    
    axiom_window_constraints_init(&geometry->constraints);
}

void axiom_window_geometry_copy(struct axiom_window_geometry *dst, const struct axiom_window_geometry *src) {
    if (!dst || !src) return;
    
    memcpy(dst, src, sizeof(struct axiom_window_geometry));
}

bool axiom_window_geometry_equals(const struct axiom_window_geometry *a, const struct axiom_window_geometry *b) {
    if (!a || !b) return false;
    
    return (a->x == b->x && a->y == b->y && 
            a->width == b->width && a->height == b->height);
}

void axiom_window_geometry_save(struct axiom_window_geometry *geometry) {
    if (!geometry) return;
    
    geometry->saved_x = geometry->x;
    geometry->saved_y = geometry->y;
    geometry->saved_width = geometry->width;
    geometry->saved_height = geometry->height;
    geometry->has_saved_geometry = true;
}

void axiom_window_geometry_restore(struct axiom_window_geometry *geometry) {
    if (!geometry || !geometry->has_saved_geometry) return;
    
    geometry->x = geometry->saved_x;
    geometry->y = geometry->saved_y;
    geometry->width = geometry->saved_width;
    geometry->height = geometry->saved_height;
    geometry->has_saved_geometry = false;
}

void axiom_window_geometry_update_boxes(struct axiom_window_geometry *geometry, 
                                       int title_bar_height, int border_width) {
    if (!geometry) return;

    // Content box (usable client area)
    geometry->content_box.x = geometry->x + border_width;
    geometry->content_box.y = geometry->y + title_bar_height + border_width;
    geometry->content_box.width = geometry->width - (2 * border_width);
    geometry->content_box.height = geometry->height - title_bar_height - (2 * border_width);

    // Decoration box (title bar and borders)
    geometry->decoration_box.x = geometry->x;
    geometry->decoration_box.y = geometry->y;
    geometry->decoration_box.width = geometry->width;
    geometry->decoration_box.height = title_bar_height + (2 * border_width);

    // Full box (entire window including decorations)
    geometry->full_box.x = geometry->x;
    geometry->full_box.y = geometry->y;
    geometry->full_box.width = geometry->width;
    geometry->full_box.height = geometry->height;
}

// Window constraints functions

void axiom_window_constraints_init(struct axiom_window_constraints *constraints) {
    if (!constraints) return;

    memset(constraints, 0, sizeof(struct axiom_window_constraints));
    
    constraints->min_width = AXIOM_MIN_WINDOW_WIDTH;
    constraints->min_height = AXIOM_MIN_WINDOW_HEIGHT;
    constraints->max_width = 0;  // 0 means no limit
    constraints->max_height = 0;
    constraints->has_min_size = true;
    constraints->has_max_size = false;
    constraints->respect_aspect_ratio = false;
    constraints->aspect_ratio = 0.0;
}

void axiom_window_constraints_apply(struct axiom_window_constraints *constraints,
                                   int *width, int *height) {
    if (!constraints || !width || !height) return;

    if (constraints->has_min_size) {
        if (*width < constraints->min_width) {
            *width = constraints->min_width;
        }
        if (*height < constraints->min_height) {
            *height = constraints->min_height;
        }
    }

    if (constraints->has_max_size) {
        if (constraints->max_width > 0 && *width > constraints->max_width) {
            *width = constraints->max_width;
        }
        if (constraints->max_height > 0 && *height > constraints->max_height) {
            *height = constraints->max_height;
        }
    }

    if (constraints->respect_aspect_ratio && constraints->aspect_ratio > 0.0) {
        double current_ratio = (double)*width / (double)*height;
        if (fabs(current_ratio - constraints->aspect_ratio) > 0.01) {
            // Adjust height to maintain aspect ratio
            *height = (int)(*width / constraints->aspect_ratio);
        }
    }
}

bool axiom_window_constraints_validate_size(const struct axiom_window_constraints *constraints,
                                           int width, int height) {
    if (!constraints) return true;

    if (constraints->has_min_size) {
        if (width < constraints->min_width || height < constraints->min_height) {
            return false;
        }
    }

    if (constraints->has_max_size) {
        if ((constraints->max_width > 0 && width > constraints->max_width) ||
            (constraints->max_height > 0 && height > constraints->max_height)) {
            return false;
        }
    }

    return true;
}

// Layout calculation functions

void axiom_window_layout_init(struct axiom_window_layout *layout) {
    if (!layout) return;

    memset(layout, 0, sizeof(struct axiom_window_layout));
    
    layout->mode = AXIOM_WM_LAYOUT_TILED;
    layout->tiling.grid_x = 0;
    layout->tiling.grid_y = 0;
    layout->tiling.grid_cols = 1;
    layout->tiling.grid_rows = 1;
    layout->tiling.is_master = false;
    layout->tiling.split_ratio = 0.5f;
    layout->tiling.tiling_edge = AXIOM_TILING_NONE;
    layout->workspace = 0;
    layout->workspace_sticky = false;
    layout->z_index = 0;
    layout->always_on_top = false;
    layout->always_on_bottom = false;
}

void axiom_window_layout_calculate_tiled_position(struct axiom_window_manager *manager,
                                                 struct axiom_window *window,
                                                 struct axiom_window_layout *layout,
                                                 int *x, int *y, int *width, int *height) {
    if (!manager || !window || !layout || !x || !y || !width || !height) return;

    // Count tiled windows
    int tiled_count = 0;
    int window_index = 0;
    struct axiom_window *w;
    wl_list_for_each(w, &manager->tiled_windows, tiled_link) {
        if (w == window) {
            window_index = tiled_count;
        }
        tiled_count++;
    }

    if (tiled_count == 0) {
        *x = *y = 0;
        *width = manager->workspace_width;
        *height = manager->workspace_height;
        return;
    }

    // Simple grid layout
    int cols = (int)ceil(sqrt(tiled_count));
    int rows = (int)ceil((double)tiled_count / cols);
    
    int col = window_index % cols;
    int row = window_index / cols;
    
    int usable_width = manager->workspace_width - ((cols + 1) * manager->gap_size);
    int usable_height = manager->workspace_height - ((rows + 1) * manager->gap_size) - manager->title_bar_height;
    
    *width = usable_width / cols;
    *height = usable_height / rows;
    
    *x = manager->gap_size + col * (*width + manager->gap_size);
    *y = manager->gap_size + manager->title_bar_height + row * (*height + manager->gap_size);
    
    // Update layout data
    layout->tiling.grid_x = col;
    layout->tiling.grid_y = row;
    layout->tiling.grid_cols = cols;
    layout->tiling.grid_rows = rows;
}

void axiom_window_layout_calculate_floating_position(struct axiom_window_manager *manager,
                                                    struct axiom_window *window,
                                                    struct axiom_window_layout *layout,
                                                    int *x, int *y, int *width, int *height) {
    if (!manager || !window || !layout || !x || !y || !width || !height) return;

    // For floating windows, use current geometry or center on screen
    if (window->geometry && window->geometry->width > 0 && window->geometry->height > 0) {
        *x = window->geometry->x;
        *y = window->geometry->y;
        *width = window->geometry->width;
        *height = window->geometry->height;
    } else {
        // Center new floating window
        *width = AXIOM_DEFAULT_WINDOW_WIDTH;
        *height = AXIOM_DEFAULT_WINDOW_HEIGHT;
        *x = (manager->workspace_width - *width) / 2;
        *y = (manager->workspace_height - *height) / 2;
    }
}

// Window state functions

void axiom_window_state_init(struct axiom_window_state *state) {
    if (!state) return;

    memset(state, 0, sizeof(struct axiom_window_state));
    
    state->state_flags = AXIOM_WINDOW_STATE_NORMAL;
    state->previous_state = AXIOM_WINDOW_STATE_NORMAL;
    state->is_focused = false;
    state->can_focus = true;
    state->last_focus_time = 0;
    state->is_being_moved = false;
    state->is_being_resized = false;
    state->resize_edges = 0;
    state->is_visible = false;
    state->decorations_visible = true;
    state->opacity = 1.0f;
    state->is_animating = false;
    state->animation_id = 0;
    state->needs_configure = false;
    state->configure_pending = false;
}

void axiom_window_state_copy(struct axiom_window_state *dst, const struct axiom_window_state *src) {
    if (!dst || !src) return;
    
    memcpy(dst, src, sizeof(struct axiom_window_state));
}

bool axiom_window_state_changed(const struct axiom_window_state *old_state,
                                const struct axiom_window_state *new_state) {
    if (!old_state || !new_state) return true;
    
    return (old_state->state_flags != new_state->state_flags ||
            old_state->is_focused != new_state->is_focused ||
            old_state->is_visible != new_state->is_visible ||
            old_state->opacity != new_state->opacity);
}

// Title bar dimensions and styling
#define TITLE_BAR_HEIGHT 30
#define BUTTON_SIZE 18
#define BUTTON_MARGIN 6
#define BUTTON_SPACING 2

// Button colors
static const float close_button_color[4] = {0.85f, 0.35f, 0.35f, 1.0f};
static const float close_button_hover[4] = {1.0f, 0.4f, 0.4f, 1.0f};
static const float minimize_button_color[4] = {0.95f, 0.75f, 0.3f, 1.0f};
static const float minimize_button_hover[4] = {1.0f, 0.85f, 0.4f, 1.0f};
static const float maximize_button_color[4] = {0.4f, 0.75f, 0.4f, 1.0f};
static const float maximize_button_hover[4] = {0.5f, 0.85f, 0.5f, 1.0f};

void axiom_create_title_bar_buttons(struct axiom_window *window) {
    if (!window || !window->decoration_tree) {
        AXIOM_LOG_ERROR("TITLE_BAR", "Cannot create title bar buttons");
        return;
    }
    int window_width = window->width;
    int close_x = window_width - BUTTON_MARGIN - BUTTON_SIZE;
    int minimize_x = close_x - BUTTON_SIZE - BUTTON_SPACING;
    int maximize_x = minimize_x - BUTTON_SIZE - BUTTON_SPACING;
    int button_y = (TITLE_BAR_HEIGHT - BUTTON_SIZE) / 2;
    window->close_button = wlr_scene_rect_create(window->decoration_tree, BUTTON_SIZE, BUTTON_SIZE, close_button_color);
    if (window->close_button) {
        wlr_scene_node_set_position(&window->close_button->node, close_x, button_y);
    }
    window->minimize_button = wlr_scene_rect_create(window->decoration_tree, BUTTON_SIZE, BUTTON_SIZE, minimize_button_color);
    if (window->minimize_button) {
        wlr_scene_node_set_position(&window->minimize_button->node, minimize_x, button_y);
    }
    window->maximize_button = wlr_scene_rect_create(window->decoration_tree, BUTTON_SIZE, BUTTON_SIZE, maximize_button_color);
    if (window->maximize_button) {
        wlr_scene_node_set_position(&window->maximize_button->node, maximize_x, button_y);
    }
}

void axiom_update_title_bar_buttons(struct axiom_window *window) {
    if (!window) {
        return;
    }
    if (window->close_button) {
        const float *color = window->close_button_hovered ? close_button_hover : close_button_color;
        wlr_scene_rect_set_color(window->close_button, color);
    }
    if (window->minimize_button) {
        const float *color = window->minimize_button_hovered ? minimize_button_hover : minimize_button_color;
        wlr_scene_rect_set_color(window->minimize_button, color);
    }
    if (window->maximize_button) {
        const float *color = window->maximize_button_hovered ? maximize_button_hover : maximize_button_color;
        wlr_scene_rect_set_color(window->maximize_button, color);
    }
}

void axiom_update_button_hover_states(struct axiom_window *window, double x, double y) {
    if (!window) return;
    
    int window_width = window->width;
    int close_x = window_width - BUTTON_MARGIN - BUTTON_SIZE;
    int minimize_x = close_x - BUTTON_SIZE - BUTTON_SPACING;
    int maximize_x = minimize_x - BUTTON_SIZE - BUTTON_SPACING;
    int button_y = (TITLE_BAR_HEIGHT - BUTTON_SIZE) / 2;
    
    // Check close button hover
    window->close_button_hovered = (x >= close_x && x <= close_x + BUTTON_SIZE &&
                                   y >= button_y && y <= button_y + BUTTON_SIZE);
    
    // Check minimize button hover
    window->minimize_button_hovered = (x >= minimize_x && x <= minimize_x + BUTTON_SIZE &&
                                      y >= button_y && y <= button_y + BUTTON_SIZE);
    
    // Check maximize button hover
    window->maximize_button_hovered = (x >= maximize_x && x <= maximize_x + BUTTON_SIZE &&
                                      y >= button_y && y <= button_y + BUTTON_SIZE);
    
    // Update button colors based on new hover states
    axiom_update_title_bar_buttons(window);
}

bool axiom_handle_title_bar_click(struct axiom_window *window, double x, double y) {
    if (!window) return false;
    
    int window_width = window->width;
    int close_x = window_width - BUTTON_MARGIN - BUTTON_SIZE;
    int minimize_x = close_x - BUTTON_SIZE - BUTTON_SPACING;
    int maximize_x = minimize_x - BUTTON_SIZE - BUTTON_SPACING;
    int button_y = (TITLE_BAR_HEIGHT - BUTTON_SIZE) / 2;
    
    // Check if click is on close button
    if (x >= close_x && x <= close_x + BUTTON_SIZE &&
        y >= button_y && y <= button_y + BUTTON_SIZE) {
        axiom_window_close(window);
        return true;
    }
    
    // Check if click is on minimize button
    if (x >= minimize_x && x <= minimize_x + BUTTON_SIZE &&
        y >= button_y && y <= button_y + BUTTON_SIZE) {
        axiom_window_minimize(window);
        return true;
    }
    
    // Check if click is on maximize button
    if (x >= maximize_x && x <= maximize_x + BUTTON_SIZE &&
        y >= button_y && y <= button_y + BUTTON_SIZE) {
        axiom_window_toggle_maximize(window);
        return true;
    }
    
    return false;
}

void axiom_window_close(struct axiom_window *window) {
    if (!window || !window->xdg_toplevel) return;
    
    AXIOM_LOG_INFO("WINDOW_MGR", "Closing window %p", (void*)window);
    
    // Send close request to client
    wlr_xdg_toplevel_send_close(window->xdg_toplevel);
}

void axiom_window_minimize(struct axiom_window *window) {
    if (!window || !window->state) return;
    
    AXIOM_LOG_INFO("WINDOW_MGR", "Minimizing window %p", (void*)window);
    
    // Set minimized state
    axiom_window_set_state(window, AXIOM_WINDOW_STATE_MINIMIZED, true);
    
    // Hide window in scene graph
    if (window->scene_tree) {
        wlr_scene_node_set_enabled(&window->scene_tree->node, false);
    }
    
    // Hide decorations
    if (window->decoration_tree) {
        wlr_scene_node_set_enabled(&window->decoration_tree->node, false);
    }
    
    // Start minimize animation if available
    if (window->server && window->server->animation_manager) {
        axiom_animate_window_disappear(window->server, window);
    }
}

void axiom_window_toggle_maximize(struct axiom_window *window) {
    if (!window || !window->state || !window->geometry) return;
    
    bool is_maximized = axiom_window_has_state(window, AXIOM_WINDOW_STATE_MAXIMIZED);
    
    if (is_maximized) {
        AXIOM_LOG_INFO("WINDOW_MGR", "Unmaximizing window %p", (void*)window);
        
        // Restore from maximized state
        axiom_window_set_state(window, AXIOM_WINDOW_STATE_MAXIMIZED, false);
        
        // Restore saved geometry
        axiom_window_geometry_restore(window->geometry);
        axiom_window_apply_geometry(window, window->geometry);
        
        // Send unmaximized state to client
        if (window->xdg_toplevel) {
            wlr_xdg_toplevel_set_maximized(window->xdg_toplevel, false);
        }
    } else {
        AXIOM_LOG_INFO("WINDOW_MGR", "Maximizing window %p", (void*)window);
        
        // Save current geometry before maximizing
        axiom_window_geometry_save(window->geometry);
        
        // Set maximized state
        axiom_window_set_state(window, AXIOM_WINDOW_STATE_MAXIMIZED, true);
        
        // Get workspace dimensions
        struct axiom_window_manager *manager = window->manager;
        if (manager) {
            // Calculate maximized geometry
            window->geometry->x = 0;
            window->geometry->y = manager->title_bar_height;
            window->geometry->width = manager->workspace_width;
            window->geometry->height = manager->workspace_height - manager->title_bar_height;
            
            // Apply maximized geometry
            axiom_window_apply_geometry(window, window->geometry);
        }
        
        // Send maximized state to client
        if (window->xdg_toplevel) {
            wlr_xdg_toplevel_set_maximized(window->xdg_toplevel, true);
        }
    }
    
    // Start resize animation if available
    if (window->server && window->server->animation_manager) {
        axiom_animate_window_resize(window->server, window, 
                                   window->geometry->width, window->geometry->height);
    }
    
    // Update decorations
    axiom_update_window_decorations(window);
}

static void window_manager_schedule_layout_update(struct axiom_window_manager *manager) {
    if (!manager || !manager->layout_timer) return;

    if (!manager->layout_calculation_pending) {
        manager->layout_calculation_pending = true;
        wl_event_source_timer_update(manager->layout_timer, 1); // 1ms delay
    }
}

static int window_manager_layout_timer_handler(void *data) {
    struct axiom_window_manager *manager = data;
    if (!manager) return 0;

    if (manager->layout_calculation_pending) {
        axiom_window_manager_arrange_all(manager);
    }

    return 0;
}

static void window_manager_calculate_tiled_layout(struct axiom_window_manager *manager) {
    if (!manager) return;

    // This is where we would implement more sophisticated tiling algorithms
    // For now, the grid layout is calculated per-window in the position calculation
    
    AXIOM_LOG_DEBUG("WINDOW_MGR", "Calculated tiled layout for %d windows",
                   wl_list_length(&manager->tiled_windows));
}

static void window_manager_apply_constraints(struct axiom_window_constraints *constraints, 
                                            int *width, int *height) {
    axiom_window_constraints_apply(constraints, width, height);
}

static bool window_manager_validate_geometry(const struct axiom_window_geometry *geometry) {
    if (!geometry) return false;

    return (geometry->width > 0 && geometry->height > 0 &&
            geometry->width >= AXIOM_MIN_WINDOW_WIDTH &&
            geometry->height >= AXIOM_MIN_WINDOW_HEIGHT);
}

// Implementation of the global arrange windows function
void axiom_arrange_windows(struct axiom_server *server) {
    if (!server || !server->window_manager) {
        AXIOM_LOG_ERROR("WINDOW_MGR", "Cannot arrange windows: invalid server or window manager");
        return;
    }

    axiom_window_manager_arrange_all(server->window_manager);
    AXIOM_LOG_DEBUG("WINDOW_MGR", "Arranged all windows");
}

// Implementation of the global reload configuration function
void axiom_reload_configuration(struct axiom_server *server) {
    if (!server) {
        AXIOM_LOG_ERROR("CONFIG", "Cannot reload configuration: invalid server");
        return;
    }

    AXIOM_LOG_INFO("CONFIG", "Reloading configuration...");

    // Load the configuration from the default path
    const char *config_path = getenv("AXIOM_CONFIG_PATH");
    if (!config_path) {
        config_path = "/home/alex/.config/axiom/config.ini"; // Default path
    }

    // Create a new config and load it
    struct axiom_config *new_config = axiom_config_create();
    if (!new_config) {
        AXIOM_LOG_ERROR("CONFIG", "Failed to create new configuration");
        return;
    }

    if (!axiom_config_load(new_config, config_path)) {
        AXIOM_LOG_ERROR("CONFIG", "Failed to load configuration from %s", config_path);
        axiom_config_destroy(new_config);
        return;
    }

    if (!axiom_config_validate(new_config)) {
        AXIOM_LOG_ERROR("CONFIG", "Configuration validation failed");
        axiom_config_destroy(new_config);
        return;
    }

    // Replace the old config with the new one
    if (server->config) {
        axiom_config_destroy(server->config);
    }
    server->config = new_config;

    // Apply configuration changes to the window manager
    if (server->window_manager) {
        // Update window manager settings from new config
        server->window_manager->border_width = new_config->border_width;
        server->window_manager->gap_size = new_config->gap_size;
        
        // Rearrange windows with new settings
        axiom_window_manager_arrange_all(server->window_manager);
    }

    // Update cursor settings if available
    if (server->cursor_mgr && new_config->cursor_size > 0) {
        // Note: cursor manager update would need implementation
        AXIOM_LOG_DEBUG("CONFIG", "Updated cursor settings");
    }

    AXIOM_LOG_INFO("CONFIG", "Configuration reloaded successfully");
}
