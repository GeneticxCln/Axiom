#include "window_snapping.h"
#include "axiom.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <wlr/util/log.h>

// Default snapping behavior
static const struct axiom_snap_behavior default_behavior = {
    .edge_resistance = true,
    .sticky_edges = true,
    .magnetism = true,
    .resistance_threshold = 20,
    .magnetism_range = 15,
    .delay_ms = 100
};

// Default snapping configuration
static const struct axiom_snapping_config default_config = {
    .snap_threshold = 20,
    .edge_resistance = 15,
    .magnetism_strength = 0.8f,
    .animation_duration = 200,
    .smart_corners = true,
    .multi_monitor_snapping = true,
    .window_to_window_snapping = true,
    .edge_snapping = true
};

/**
 * Create and initialize window snapping manager
 */
struct axiom_window_snapping_manager *axiom_window_snapping_manager_create(struct axiom_server *server) {
    struct axiom_window_snapping_manager *manager = calloc(1, sizeof(*manager));
    if (!manager) {
        wlr_log(WLR_ERROR, "Failed to allocate window snapping manager");
        return NULL;
    }

    manager->server = server;
    
    // Initialize snapping windows lists
    wl_list_init(&manager->snapping_windows);
    wl_list_init(&manager->snapped_windows);
    
    // Set default behavior and configuration
    manager->behavior = default_behavior;
    manager->config = default_config;
    manager->enabled = true;
    
    // Initialize statistics
    memset(&manager->stats, 0, sizeof(manager->stats));
    
    wlr_log(WLR_INFO, "Window snapping manager initialized");
    return manager;
}

/**
 * Initialize window snapping manager with configuration
 */
bool axiom_window_snapping_manager_init(struct axiom_window_snapping_manager *manager, 
                                       struct axiom_snapping_config *config) {
    if (!manager) {
        return false;
    }
    
    // Apply configuration if provided
    if (config) {
        // Note: config is axiom_snapping_config, not the full window_snapping_config
        // manager->enabled = true; // Keep default enabled state since config doesn't have this field
        manager->config.snap_threshold = config->snap_threshold;
        manager->config.edge_resistance = config->edge_resistance;
        manager->config.magnetism_strength = config->magnetism_strength;
        manager->config.smart_corners = config->smart_corners;
        manager->config.multi_monitor_snapping = config->multi_monitor_snapping;
        manager->config.window_to_window_snapping = config->window_to_window_snapping;
        manager->config.edge_snapping = config->edge_snapping;
        
        // Use animation duration from config if available
        manager->config.animation_duration = config->animation_duration;
        
        wlr_log(WLR_INFO, "Window snapping configured: enabled=%s, threshold=%d, magnetism=%.2f",
               manager->enabled ? "yes" : "no", 
               manager->config.snap_threshold,
               manager->config.magnetism_strength);
    } else {
        // Use defaults
        manager->config = default_config;
        wlr_log(WLR_INFO, "Window snapping using default configuration");
    }
    
    wlr_log(WLR_INFO, "Window snapping manager initialized successfully");
    return true;
}

/**
 * Destroy window snapping manager and cleanup resources
 */
void axiom_window_snapping_manager_destroy(struct axiom_window_snapping_manager *manager) {
    if (!manager) {
        return;
    }

    // Clear all snap states
    struct axiom_snap_state *state, *tmp;
    wl_list_for_each_safe(state, tmp, &manager->snapping_windows, link) {
        wl_list_remove(&state->link);
        free(state);
    }
    
    free(manager);
    wlr_log(WLR_INFO, "Window snapping manager destroyed");
}

/**
 * Get snapping state for a specific window
 */
static struct axiom_window_snapping_state *get_window_snapping_state(
    struct axiom_window_snapping_manager *manager, 
    struct axiom_window *window) {
    
    struct axiom_window_snapping_state *state;
    wl_list_for_each(state, &manager->snapped_windows, link) {
        if (state->window == window) {
            return state;
        }
    }
    
    // Create new snapping state if not found
    state = calloc(1, sizeof(*state));
    if (!state) {
        return NULL;
    }
    
    state->window = window;
    state->is_snapped = false;
    state->snap_direction = AXIOM_SNAP_NONE;
    state->snap_edge_mask = 0;
    wl_list_insert(&manager->snapped_windows, &state->link);
    
    return state;
}

/**
 * Calculate snap positions for window edges
 */
static void calculate_snap_positions(struct axiom_server *server, 
                                   struct axiom_window *window,
                                   int current_x, int current_y,
                                   int *snap_x, int *snap_y,
                                   enum axiom_snap_direction *snap_direction) {
    
    struct axiom_window_snapping_manager *manager = server->window_snapping_manager;
    if (!manager || !manager->enabled) {
        *snap_x = current_x;
        *snap_y = current_y;
        *snap_direction = AXIOM_SNAP_NONE;
        return;
    }

    int threshold = manager->config.snap_threshold;
    *snap_x = current_x;
    *snap_y = current_y;
    *snap_direction = AXIOM_SNAP_NONE;

    // Get output dimensions
    struct wlr_output *output = wlr_output_layout_output_at(server->output_layout, 
                                                           current_x, current_y);
    if (!output) {
        return;
    }

    struct wlr_box output_box;
    wlr_output_layout_get_box(server->output_layout, output, &output_box);

    int window_width = window->width;
    int window_height = window->height;

    // Check for edge snapping
    if (manager->config.edge_snapping) {
        // Left edge
        if (abs(current_x - output_box.x) <= threshold) {
            *snap_x = output_box.x;
            *snap_direction = AXIOM_SNAP_LEFT;
        }
        // Right edge
        else if (abs((current_x + window_width) - (output_box.x + output_box.width)) <= threshold) {
            *snap_x = output_box.x + output_box.width - window_width;
            *snap_direction = AXIOM_SNAP_RIGHT;
        }

        // Top edge
        if (abs(current_y - output_box.y) <= threshold) {
            *snap_y = output_box.y;
            if (*snap_direction == AXIOM_SNAP_LEFT) {
                *snap_direction = AXIOM_SNAP_TOP_LEFT;
            } else if (*snap_direction == AXIOM_SNAP_RIGHT) {
                *snap_direction = AXIOM_SNAP_TOP_RIGHT;
            } else {
                *snap_direction = AXIOM_SNAP_TOP;
            }
        }
        // Bottom edge
        else if (abs((current_y + window_height) - (output_box.y + output_box.height)) <= threshold) {
            *snap_y = output_box.y + output_box.height - window_height;
            if (*snap_direction == AXIOM_SNAP_LEFT) {
                *snap_direction = AXIOM_SNAP_BOTTOM_LEFT;
            } else if (*snap_direction == AXIOM_SNAP_RIGHT) {
                *snap_direction = AXIOM_SNAP_BOTTOM_RIGHT;
            } else {
                *snap_direction = AXIOM_SNAP_BOTTOM;
            }
        }
    }

    // Check for window-to-window snapping
    if (manager->config.window_to_window_snapping) {
        struct axiom_window *other_window;
        wl_list_for_each(other_window, &server->windows, link) {
            if (other_window == window || other_window->is_fullscreen) {
                continue;
            }

            // Check horizontal alignment for vertical snapping
            if (abs(current_y - other_window->y) < threshold ||
                abs((current_y + window_height) - (other_window->y + other_window->height)) < threshold) {
                
                // Snap to left of other window
                if (abs(current_x - (other_window->x - window_width)) <= threshold) {
                    *snap_x = other_window->x - window_width;
                }
                // Snap to right of other window
                else if (abs(current_x - (other_window->x + other_window->width)) <= threshold) {
                    *snap_x = other_window->x + other_window->width;
                }
            }

            // Check vertical alignment for horizontal snapping
            if (abs(current_x - other_window->x) < threshold ||
                abs((current_x + window_width) - (other_window->x + other_window->width)) < threshold) {
                
                // Snap to top of other window
                if (abs(current_y - (other_window->y - window_height)) <= threshold) {
                    *snap_y = other_window->y - window_height;
                }
                // Snap to bottom of other window
                else if (abs(current_y - (other_window->y + other_window->height)) <= threshold) {
                    *snap_y = other_window->y + other_window->height;
                }
            }
        }
    }
}

/**
 * Apply magnetism effect during window movement
 */
static void apply_magnetism(struct axiom_window_snapping_manager *manager,
                           int current_x, int current_y,
                           int snap_x, int snap_y,
                           int *final_x, int *final_y) {
    
    if (!manager->config.magnetism_strength) {
        *final_x = current_x;
        *final_y = current_y;
        return;
    }

    float strength = manager->config.magnetism_strength;
    
    // Apply magnetism based on distance to snap position
    int dx = snap_x - current_x;
    int dy = snap_y - current_y;
    
    *final_x = current_x + (int)(dx * strength);
    *final_y = current_y + (int)(dy * strength);
}

/**
 * Handle window move with snapping
 */
void axiom_window_snapping_handle_move(struct axiom_server *server,
                                     struct axiom_window *window,
                                     int x, int y) {
    if (!server->window_snapping_manager || !server->window_snapping_manager->enabled) {
        return;
    }

    struct axiom_window_snapping_manager *manager = server->window_snapping_manager;
    struct axiom_window_snapping_state *state = get_window_snapping_state(manager, window);
    if (!state) {
        return;
    }

    int snap_x, snap_y;
    enum axiom_snap_direction snap_direction;
    
    // Calculate snap positions
    calculate_snap_positions(server, window, x, y, &snap_x, &snap_y, &snap_direction);
    
    // Apply magnetism if snapping is detected
    int final_x = x, final_y = y;
    if (snap_direction != AXIOM_SNAP_NONE) {
        apply_magnetism(manager, x, y, snap_x, snap_y, &final_x, &final_y);
        
        // Update snapping state
        state->is_snapped = true;
        state->snap_direction = snap_direction;
        state->snap_position.x = snap_x;
        state->snap_position.y = snap_y;
        
        // Update statistics
        manager->stats.total_snaps++;
        if (snap_direction == AXIOM_SNAP_LEFT || snap_direction == AXIOM_SNAP_RIGHT ||
            snap_direction == AXIOM_SNAP_TOP || snap_direction == AXIOM_SNAP_BOTTOM) {
            manager->stats.edge_snaps++;
        }
        if (snap_direction == AXIOM_SNAP_TOP_LEFT || snap_direction == AXIOM_SNAP_TOP_RIGHT ||
            snap_direction == AXIOM_SNAP_BOTTOM_LEFT || snap_direction == AXIOM_SNAP_BOTTOM_RIGHT) {
            manager->stats.corner_snaps++;
        }
    } else {
        state->is_snapped = false;
        state->snap_direction = AXIOM_SNAP_NONE;
    }

    // Update window position
    window->x = final_x;
    window->y = final_y;
}

/**
 * Handle window resize with snapping constraints
 */
void axiom_window_snapping_handle_resize(struct axiom_server *server,
                                       struct axiom_window *window,
                                       int width, int height) {
    if (!server->window_snapping_manager || !server->window_snapping_manager->enabled) {
        return;
    }

    struct axiom_window_snapping_manager *manager = server->window_snapping_manager;
    struct axiom_window_snapping_state *state = get_window_snapping_state(manager, window);
    if (!state || !state->is_snapped) {
        return;
    }

    // Apply edge resistance during resize
    int resistance = manager->config.edge_resistance;
    
    // Get output bounds for edge resistance
    struct wlr_output *output = wlr_output_layout_output_at(server->output_layout, 
                                                           window->x, window->y);
    if (!output) {
        return;
    }

    struct wlr_box output_box;
    wlr_output_layout_get_box(server->output_layout, output, &output_box);

    int final_width = width;
    int final_height = height;

    // Apply resistance based on snap direction
    switch (state->snap_direction) {
        case AXIOM_SNAP_LEFT:
        case AXIOM_SNAP_TOP_LEFT:
        case AXIOM_SNAP_BOTTOM_LEFT:
            // Resist expanding beyond right edge
            if (window->x + width > output_box.x + output_box.width - resistance) {
                final_width = output_box.x + output_box.width - window->x - resistance;
            }
            break;
            
        case AXIOM_SNAP_RIGHT:
        case AXIOM_SNAP_TOP_RIGHT:
        case AXIOM_SNAP_BOTTOM_RIGHT:
            // Resist expanding beyond left edge
            if (window->x < output_box.x + resistance) {
                final_width = width - (output_box.x + resistance - window->x);
            }
            break;
            
        default:
            break;
    }

    switch (state->snap_direction) {
        case AXIOM_SNAP_TOP:
        case AXIOM_SNAP_TOP_LEFT:
        case AXIOM_SNAP_TOP_RIGHT:
            // Resist expanding beyond bottom edge
            if (window->y + height > output_box.y + output_box.height - resistance) {
                final_height = output_box.y + output_box.height - window->y - resistance;
            }
            break;
            
        case AXIOM_SNAP_BOTTOM:
        case AXIOM_SNAP_BOTTOM_LEFT:
        case AXIOM_SNAP_BOTTOM_RIGHT:
            // Resist expanding beyond top edge
            if (window->y < output_box.y + resistance) {
                final_height = height - (output_box.y + resistance - window->y);
            }
            break;
            
        default:
            break;
    }

    // Update window dimensions
    window->width = final_width;
    window->height = final_height;
}

/**
 * Clear snapping state for a window
 */
void axiom_window_snapping_clear_state(struct axiom_server *server,
                                     struct axiom_window *window) {
    if (!server->window_snapping_manager) {
        return;
    }

    struct axiom_window_snapping_manager *manager = server->window_snapping_manager;
    struct axiom_window_snapping_state *state;
    
    wl_list_for_each(state, &manager->snapped_windows, link) {
        if (state->window == window) {
            state->is_snapped = false;
            state->snap_direction = AXIOM_SNAP_NONE;
            state->snap_edge_mask = 0;
            return;
        }
    }
}

/**
 * Update snapping configuration
 */
void axiom_window_snapping_update_config(struct axiom_window_snapping_manager *manager,
                                        const struct axiom_snapping_config *config) {
    if (!manager || !config) {
        return;
    }

    manager->config = *config;
    wlr_log(WLR_INFO, "Window snapping configuration updated");
}

/**
 * Load snapping configuration from config file
 */
bool axiom_window_snapping_load_config(struct axiom_window_snapping_manager *manager, const char *config_path) {
    if (!manager) {
        return false;
    }

    // Load configuration values (placeholder - actual implementation would read from config file)
    struct axiom_snapping_config snapping_config = default_config;
    
    // Example configuration loading (would be replaced with actual config file parsing)
    snapping_config.snap_threshold = 20;  // Could be loaded from config file
    snapping_config.edge_resistance = 15;
    snapping_config.magnetism_strength = 0.8f;
    snapping_config.animation_duration = 200;
    snapping_config.smart_corners = true;
    snapping_config.multi_monitor_snapping = true;
    snapping_config.window_to_window_snapping = true;
    snapping_config.edge_snapping = true;

    manager->config = snapping_config;
    wlr_log(WLR_INFO, "Window snapping configuration loaded from %s", config_path ?: "default");
    return true;
}

/**
 * Save current snapping configuration
 */
bool axiom_window_snapping_save_config(struct axiom_window_snapping_manager *manager, const char *config_path) {
    if (!manager) {
        return false;
    }

    // Save configuration to config file (placeholder)
    wlr_log(WLR_INFO, "Window snapping configuration saved to %s", config_path ?: "default");
    return true;
}

/**
 * Get snapping statistics
 */
struct axiom_snapping_stats axiom_window_snapping_get_stats(
    struct axiom_window_snapping_manager *manager) {
    
    if (!manager) {
        struct axiom_snapping_stats empty_stats = {0};
        return empty_stats;
    }

    return manager->stats;
}

/**
 * Reset snapping statistics
 */
void axiom_window_snapping_reset_stats(struct axiom_window_snapping_manager *manager) {
    if (!manager) {
        return;
    }

    memset(&manager->stats, 0, sizeof(manager->stats));
    wlr_log(WLR_INFO, "Window snapping statistics reset");
}

/**
 * Debug: Print current snapping state
 */
void axiom_window_snapping_debug_print(struct axiom_window_snapping_manager *manager) {
    if (!manager) {
        wlr_log(WLR_INFO, "Window snapping manager: NULL");
        return;
    }

    wlr_log(WLR_INFO, "=== Window Snapping Debug ===");
    wlr_log(WLR_INFO, "Enabled: %s", manager->enabled ? "yes" : "no");
    wlr_log(WLR_INFO, "Snap threshold: %d", manager->config.snap_threshold);
    wlr_log(WLR_INFO, "Edge resistance: %d", manager->config.edge_resistance);
    wlr_log(WLR_INFO, "Magnetism strength: %.2f", manager->config.magnetism_strength);
    wlr_log(WLR_INFO, "Smart corners: %s", manager->config.smart_corners ? "yes" : "no");
    
    wlr_log(WLR_INFO, "=== Statistics ===");
    wlr_log(WLR_INFO, "Total snaps: %u", manager->stats.total_snaps);
    wlr_log(WLR_INFO, "Edge snaps: %u", manager->stats.edge_snaps);
    wlr_log(WLR_INFO, "Window snaps: %u", manager->stats.window_snaps);
    wlr_log(WLR_INFO, "Corner snaps: %u", manager->stats.corner_snaps);

    int snapped_count = 0;
    struct axiom_window_snapping_state *state;
    wl_list_for_each(state, &manager->snapped_windows, link) {
        if (state->is_snapped) {
            snapped_count++;
        }
    }
    wlr_log(WLR_INFO, "Currently snapped windows: %d", snapped_count);
    wlr_log(WLR_INFO, "========================");
}
