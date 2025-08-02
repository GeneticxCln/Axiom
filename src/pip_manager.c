#include "pip_manager.h"
#include "axiom.h"
#include <stdlib.h>
#include <string.h>

// Create and initialize the PiP manager
struct axiom_pip_manager *axiom_pip_manager_create(struct axiom_server *server) {
    struct axiom_pip_manager *manager = calloc(1, sizeof(*manager));
    if (!manager) {
        axiom_log_error("Failed to allocate PiP manager");
        return NULL;
    }
    
    manager->server = server;
    wl_list_init(&manager->pip_windows);
    manager->config.enabled = true;
    
    // Load default settings
    axiom_pip_load_defaults(manager);
    
    return manager;
}

// Destroy the PiP manager
void axiom_pip_manager_destroy(struct axiom_pip_manager *manager) {
    if (!manager) return;
    
    // Cleanup PiP windows
    struct axiom_pip_window *pip_window, *tmp;
    wl_list_for_each_safe(pip_window, tmp, &manager->pip_windows, link) {
        wl_list_remove(&pip_window->link);
        free(pip_window);
    }
    
    free(manager);
    axiom_log_debug("PiP manager destroyed");
}

// Initialize the PiP manager with configuration
bool axiom_pip_manager_init(struct axiom_pip_manager *manager, struct axiom_pip_config *config) {
    if (!manager || !config) return false;
    
    manager->config = *config;
    axiom_log_info("PiP manager initialized with default configuration");
    return true;
}

// Enable PiP mode for a window
bool axiom_pip_enable_for_window(struct axiom_pip_manager *manager,
                                 struct axiom_window *window) {
    if (!manager || !manager->config.enabled || !window) return false;

    struct axiom_pip_window *pip_window = calloc(1, sizeof(*pip_window));
    if (!pip_window) return false;

    pip_window->window = window;
    pip_window->is_pip = true;

    // Set default PiP properties (convert from string config)
    pip_window->corner = axiom_pip_corner_from_string(manager->config.default_corner);
    pip_window->size_preset = axiom_pip_size_preset_from_string(manager->config.default_size);
    pip_window->pip_opacity = manager->config.pip_opacity;
    pip_window->focused_opacity = manager->config.focused_opacity;
    pip_window->unfocused_opacity = manager->config.unfocused_opacity;

    // Add to the PiP windows list
    wl_list_insert(&manager->pip_windows, &pip_window->link);

    axiom_log_info("Enabled PiP for window");
    return true;
}

// Disable PiP mode for a window
bool axiom_pip_disable_for_window(struct axiom_pip_manager *manager,
                                  struct axiom_window *window) {
    if (!manager || !window) return false;

    struct axiom_pip_window *pip_window;
    wl_list_for_each(pip_window, &manager->pip_windows, link) {
        if (pip_window->window == window) {
            wl_list_remove(&pip_window->link);
            free(pip_window);
            axiom_log_info("Disabled PiP for window");
            return true;
        }
    }

    return false;
}

// Toggle PiP mode for a window
bool axiom_pip_toggle_for_window(struct axiom_pip_manager *manager,
                                 struct axiom_window *window) {
    if (axiom_pip_is_window_pip(manager, window)) {
        return axiom_pip_disable_for_window(manager, window);
    } else {
        return axiom_pip_enable_for_window(manager, window);
    }
}

// Check if a window is in PiP mode
bool axiom_pip_is_window_pip(struct axiom_pip_manager *manager,
                             struct axiom_window *window) {
    if (!manager || !window) return false;

    struct axiom_pip_window *pip_window;
    wl_list_for_each(pip_window, &manager->pip_windows, link) {
        if (pip_window->window == window) {
            return pip_window->is_pip;
        }
    }

    return false;
}

// Set PiP corner position
bool axiom_pip_set_corner(struct axiom_pip_manager *manager,
                          struct axiom_window *window,
                          enum axiom_pip_corner corner) {
    if (!manager || !window) return false;

    struct axiom_pip_window *pip_window;
    wl_list_for_each(pip_window, &manager->pip_windows, link) {
        if (pip_window->window == window) {
            pip_window->corner = corner;
            axiom_log_debug("Set PiP corner to %s", axiom_pip_corner_to_string(corner));
            return true;
        }
    }

    return false;
}

// Set custom position for PiP window
bool axiom_pip_set_custom_position(struct axiom_pip_manager *manager,
                                   struct axiom_window *window,
                                   int x, int y) {
    if (!manager || !window) return false;

    struct axiom_pip_window *pip_window;
    wl_list_for_each(pip_window, &manager->pip_windows, link) {
        if (pip_window->window == window) {
            pip_window->corner = AXIOM_PIP_CUSTOM;
            pip_window->custom_x = x;
            pip_window->custom_y = y;
            axiom_log_debug("Set PiP custom position to %d,%d", x, y);
            return true;
        }
    }

    return false;
}

// Cycle through corner positions
bool axiom_pip_cycle_corners(struct axiom_pip_manager *manager,
                             struct axiom_window *window) {
    if (!manager || !window) return false;

    struct axiom_pip_window *pip_window;
    wl_list_for_each(pip_window, &manager->pip_windows, link) {
        if (pip_window->window == window) {
            enum axiom_pip_corner next_corner;
            switch (pip_window->corner) {
                case AXIOM_PIP_TOP_LEFT:
                    next_corner = AXIOM_PIP_TOP_RIGHT;
                    break;
                case AXIOM_PIP_TOP_RIGHT:
                    next_corner = AXIOM_PIP_BOTTOM_RIGHT;
                    break;
                case AXIOM_PIP_BOTTOM_RIGHT:
                    next_corner = AXIOM_PIP_BOTTOM_LEFT;
                    break;
                default:
                    next_corner = AXIOM_PIP_TOP_LEFT;
                    break;
            }
            pip_window->corner = next_corner;
            axiom_log_debug("Cycled PiP corner to %s", axiom_pip_corner_to_string(next_corner));
            return true;
        }
    }

    return false;
}

// Set size preset for PiP window
bool axiom_pip_set_size_preset(struct axiom_pip_manager *manager,
                               struct axiom_window *window,
                               enum axiom_pip_size_preset preset) {
    if (!manager || !window) return false;

    struct axiom_pip_window *pip_window;
    wl_list_for_each(pip_window, &manager->pip_windows, link) {
        if (pip_window->window == window) {
            pip_window->size_preset = preset;
            axiom_log_debug("Set PiP size preset to %s", axiom_pip_size_preset_to_string(preset));
            return true;
        }
    }

    return false;
}

// Set custom size for PiP window
bool axiom_pip_set_custom_size(struct axiom_pip_manager *manager,
                               struct axiom_window *window,
                               int width, int height) {
    if (!manager || !window) return false;

    struct axiom_pip_window *pip_window;
    wl_list_for_each(pip_window, &manager->pip_windows, link) {
        if (pip_window->window == window) {
            pip_window->size_preset = AXIOM_PIP_SIZE_CUSTOM;
            pip_window->custom_width = width;
            pip_window->custom_height = height;
            axiom_log_debug("Set PiP custom size to %dx%d", width, height);
            return true;
        }
    }

    return false;
}

// Resize PiP window by scale factor
bool axiom_pip_resize_relative(struct axiom_pip_manager *manager,
                               struct axiom_window *window,
                               float scale_factor) {
    if (!manager || !window || scale_factor <= 0.0f) return false;

    struct axiom_pip_window *pip_window;
    wl_list_for_each(pip_window, &manager->pip_windows, link) {
        if (pip_window->window == window) {
            int current_width = window->width;
            int current_height = window->height;
            int new_width = (int)(current_width * scale_factor);
            int new_height = (int)(current_height * scale_factor);
            
            return axiom_pip_set_custom_size(manager, window, new_width, new_height);
        }
    }

    return false;
}

// Load default configuration
void axiom_pip_load_defaults(struct axiom_pip_manager *manager) {
    if (!manager) return;

    manager->config.enabled = true;
    manager->config.default_corner = strdup("bottom-right");
    manager->config.default_size = strdup("small");
    manager->config.default_margin_x = 20;
    manager->config.default_margin_y = 20;

    manager->config.always_on_top = true;
    manager->config.auto_hide_unfocused = false;
    manager->config.maintain_aspect_ratio = true;
    manager->config.animate_transitions = true;

    manager->config.pip_opacity = 0.9f;
    manager->config.focused_opacity = 1.0f;
    manager->config.unfocused_opacity = 0.7f;

    manager->config.animation_duration = 250;
    manager->config.auto_hide_delay = 3000;
    manager->config.auto_hide_on_fullscreen = true;

    manager->config.min_width = 160;
    manager->config.min_height = 120;
    manager->config.max_width = 1280;
    manager->config.max_height = 720;

    axiom_log_debug("Loaded default PiP configuration");
}

// Get statistics
struct axiom_pip_stats axiom_pip_get_stats(struct axiom_pip_manager *manager) {
    struct axiom_pip_stats stats = {0};
    if (!manager) return stats;

    // Count active PiP windows
    struct axiom_pip_window *pip_window;
    wl_list_for_each(pip_window, &manager->pip_windows, link) {
        if (pip_window->is_pip) {
            stats.active_pip_windows++;
        }
    }

    stats.total_pip_activations = manager->stats.total_pip_activations;
    stats.total_pip_deactivations = manager->stats.total_pip_deactivations;
    stats.auto_hide_activations = manager->stats.auto_hide_activations;
    stats.position_changes = manager->stats.position_changes;

    return stats;
}

// Print statistics
void axiom_pip_print_stats(struct axiom_pip_manager *manager) {
    if (!manager) return;

    struct axiom_pip_stats stats = axiom_pip_get_stats(manager);
    axiom_log_info("PiP Statistics:");
    axiom_log_info("  Active PiP windows: %u", stats.active_pip_windows);
    axiom_log_info("  Total activations: %u", stats.total_pip_activations);
    axiom_log_info("  Total deactivations: %u", stats.total_pip_deactivations);
    axiom_log_info("  Auto-hide activations: %u", stats.auto_hide_activations);
    axiom_log_info("  Position changes: %u", stats.position_changes);
}

// Utility functions for string conversion
const char *axiom_pip_corner_to_string(enum axiom_pip_corner corner) {
    switch (corner) {
        case AXIOM_PIP_TOP_LEFT: return "top-left";
        case AXIOM_PIP_TOP_RIGHT: return "top-right";
        case AXIOM_PIP_BOTTOM_LEFT: return "bottom-left";
        case AXIOM_PIP_BOTTOM_RIGHT: return "bottom-right";
        case AXIOM_PIP_CENTER: return "center";
        case AXIOM_PIP_CUSTOM: return "custom";
        default: return "unknown";
    }
}

const char *axiom_pip_size_preset_to_string(enum axiom_pip_size_preset preset) {
    switch (preset) {
        case AXIOM_PIP_SIZE_TINY: return "tiny";
        case AXIOM_PIP_SIZE_SMALL: return "small";
        case AXIOM_PIP_SIZE_MEDIUM: return "medium";
        case AXIOM_PIP_SIZE_LARGE: return "large";
        case AXIOM_PIP_SIZE_CUSTOM: return "custom";
        default: return "unknown";
    }
}

enum axiom_pip_corner axiom_pip_corner_from_string(const char *str) {
    if (!str) return AXIOM_PIP_BOTTOM_RIGHT;
    
    if (strcmp(str, "top-left") == 0) return AXIOM_PIP_TOP_LEFT;
    if (strcmp(str, "top-right") == 0) return AXIOM_PIP_TOP_RIGHT;
    if (strcmp(str, "bottom-left") == 0) return AXIOM_PIP_BOTTOM_LEFT;
    if (strcmp(str, "bottom-right") == 0) return AXIOM_PIP_BOTTOM_RIGHT;
    if (strcmp(str, "center") == 0) return AXIOM_PIP_CENTER;
    if (strcmp(str, "custom") == 0) return AXIOM_PIP_CUSTOM;
    
    return AXIOM_PIP_BOTTOM_RIGHT; // Default
}

enum axiom_pip_size_preset axiom_pip_size_preset_from_string(const char *str) {
    if (!str) return AXIOM_PIP_SIZE_SMALL;
    
    if (strcmp(str, "tiny") == 0) return AXIOM_PIP_SIZE_TINY;
    if (strcmp(str, "small") == 0) return AXIOM_PIP_SIZE_SMALL;
    if (strcmp(str, "medium") == 0) return AXIOM_PIP_SIZE_MEDIUM;
    if (strcmp(str, "large") == 0) return AXIOM_PIP_SIZE_LARGE;
    if (strcmp(str, "custom") == 0) return AXIOM_PIP_SIZE_CUSTOM;
    
    return AXIOM_PIP_SIZE_SMALL; // Default
}

// Server integration functions
bool axiom_server_init_pip_manager(struct axiom_server *server, struct axiom_pip_config *config) {
    if (!server) return false;

    server->pip_manager = axiom_pip_manager_create(server);
    if (!server->pip_manager) {
        axiom_log_error("Failed to create PiP manager");
        return false;
    }

    if (config) {
        if (!axiom_pip_manager_init(server->pip_manager, config)) {
            axiom_log_error("Failed to initialize PiP manager");
            axiom_pip_manager_destroy(server->pip_manager);
            server->pip_manager = NULL;
            return false;
        }
    }

    axiom_log_info("PiP system initialized");
    return true;
}

void axiom_server_destroy_pip_manager(struct axiom_server *server) {
    if (!server || !server->pip_manager) return;

    axiom_pip_manager_destroy(server->pip_manager);
    server->pip_manager = NULL;
    axiom_log_debug("PiP system destroyed");
}
