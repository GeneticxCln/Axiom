#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"

bool axiom_config_validate(struct axiom_config *config) {
    if (config->animation_speed_multiplier < 0.1f || config->animation_speed_multiplier > 5.0f) {
        AXIOM_LOG_ERROR("Animation speed multiplier out of range (0.1 - 5.0): %f", config->animation_speed_multiplier);
        return false;
    }
    return true;
}


struct axiom_config *axiom_config_create(void) {
    struct axiom_config *config = calloc(1, sizeof(struct axiom_config));
    if (!config) {
        return NULL;
    }

    // Set defaults
    config->cursor_theme = strdup("default");
    config->cursor_size = 24;
    config->repeat_rate = 25;
    config->repeat_delay = 600;
    config->tiling_enabled = true;
    config->border_width = 2;
    config->gap_size = 5;
    config->background_color = strdup("#1e1e1e");
    config->border_active = strdup("#ffffff");
    config->border_inactive = strdup("#666666");
    
    // Animation defaults
    config->animations_enabled = true;
    config->window_animations = true;
    config->workspace_animations = true;
    config->focus_animations = true;
    config->layout_animations = true;
    
    config->window_appear_duration = 300;
    config->window_disappear_duration = 200;
    config->window_move_duration = 250;
    config->window_resize_duration = 200;
    config->workspace_switch_duration = 400;
    config->focus_ring_duration = 150;
    config->layout_change_duration = 300;
    
    config->animation_speed_multiplier = 1.0f;
    config->default_easing = strdup("ease_out_cubic");
    config->animation_debug_mode = false;
    
    // Effects defaults
    config->effects.shadows_enabled = true;
    config->effects.blur_enabled = true;
    config->effects.transparency_enabled = true;
    config->effects.shadow_blur_radius = 10;
    config->effects.shadow_offset_x = 5;
    config->effects.shadow_offset_y = 5;
    config->effects.shadow_opacity = 0.5f;
    config->effects.shadow_color = strdup("#000000");
    config->effects.blur_radius = 15;
    config->effects.blur_focus_only = false;
    config->effects.blur_intensity = 0.7f;
    config->effects.focused_opacity = 1.0f;
    config->effects.unfocused_opacity = 0.85f;
    config->effects.inactive_opacity = 0.7f;
    
    // Smart gaps defaults
    config->smart_gaps.enabled = true;
    config->smart_gaps.base_gap = 10;
    config->smart_gaps.min_gap = 0;
    config->smart_gaps.max_gap = 50;
    config->smart_gaps.single_window_gap = 0;
    config->smart_gaps.adaptive_mode = strdup("count");
    
    // Window snapping defaults
    config->window_snapping.enabled = true;
    config->window_snapping.snap_threshold = 20;
    config->window_snapping.edge_resistance = 5;
    config->window_snapping.magnetism_strength = 0.8f;
    config->window_snapping.smart_corners = true;
    config->window_snapping.multi_monitor_snapping = true;
    config->window_snapping.window_to_window_snapping = true;
    config->window_snapping.edge_snapping = true;
    config->window_snapping.show_snap_preview = true;
    
    // Workspaces defaults
    config->workspaces.max_workspaces = 9;
    config->workspaces.names = calloc(9, sizeof(char*));
    if (config->workspaces.names) {
        config->workspaces.names[0] = strdup("Main");
        config->workspaces.names[1] = strdup("Web");
        config->workspaces.names[2] = strdup("Code");
        config->workspaces.names[3] = strdup("Term");
        config->workspaces.names[4] = strdup("Media");
        config->workspaces.names[5] = strdup("Files");
        config->workspaces.names[6] = strdup("Chat");
        config->workspaces.names[7] = strdup("Game");
        config->workspaces.names[8] = strdup("Misc");
        config->workspaces.names_count = 9;
    }
    config->workspaces.persistent_layouts = true;
    
    // XWayland defaults
    config->xwayland.enabled = true;
    config->xwayland.lazy = false;
    config->xwayland.force_zero_scaling = false;

    return config;
}

void axiom_config_destroy(struct axiom_config *config) {
    if (!config) {
        return;
    }

    free(config->cursor_theme);
    free(config->background_color);
    free(config->border_active);
    free(config->border_inactive);
    free(config->default_easing);
    
    // Free effects config
    free(config->effects.shadow_color);
    
    // Free smart gaps config
    free(config->smart_gaps.adaptive_mode);
    
    // Free workspaces config
    if (config->workspaces.names) {
        for (int i = 0; i < config->workspaces.names_count; i++) {
            free(config->workspaces.names[i]);
        }
        free(config->workspaces.names);
    }
    
    free(config);
}

static void trim_whitespace(char *str) {
    if (!str) return;
    
    // Trim leading space
    while (isspace((unsigned char)*str)) {
        size_t len = strlen(str);
        if (len > 0) {
            memmove(str, str + 1, len); // len already includes null terminator space
        }
    }
    
    // All spaces?
    if (*str == 0) return;
    
    // Trim trailing space
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

bool axiom_config_load(struct axiom_config *config, const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        AXIOM_LOG_INFO("Config file not found at %s, using defaults", path);
        return true; // Not an error, just use defaults
    }

    char line[256];
    char section[64] = "";
    
    while (fgets(line, sizeof(line), file)) {
        trim_whitespace(line);
        
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        
        // Check for section headers
        if (line[0] == '[' && line[strlen(line)-1] == ']') {
            size_t section_len = strlen(line) - 2; // Exclude '[' and ']'
            if (section_len >= sizeof(section)) {
                section_len = sizeof(section) - 1;
            }
            // Use safer copy with explicit bounds checking
            if (section_len > 0) {
                memcpy(section, line + 1, section_len);
            }
            section[section_len] = '\0';
            continue;
        }
        
        // Parse key=value pairs
        char *equals = strchr(line, '=');
        if (!equals) {
            continue;
        }
        
        *equals = '\0';
        char *key = line;
        char *value = equals + 1;
        
        trim_whitespace(key);
        trim_whitespace(value);
        
        // Remove quotes from value if present
        if (value[0] == '"' && value[strlen(value)-1] == '"') {
            value[strlen(value)-1] = '\0';
            value++;
        }
        
        // Parse based on section
        if (strcmp(section, "input") == 0) {
            if (strcmp(key, "repeat_rate") == 0) {
                config->repeat_rate = atoi(value);
            } else if (strcmp(key, "repeat_delay") == 0) {
                config->repeat_delay = atoi(value);
            } else if (strcmp(key, "cursor_theme") == 0) {
                free(config->cursor_theme);
                config->cursor_theme = strdup(value);
            } else if (strcmp(key, "cursor_size") == 0) {
                config->cursor_size = atoi(value);
            }
        } else if (strcmp(section, "tiling") == 0) {
            if (strcmp(key, "enabled") == 0) {
                config->tiling_enabled = strcmp(value, "true") == 0;
            } else if (strcmp(key, "border_width") == 0) {
                config->border_width = atoi(value);
            } else if (strcmp(key, "gap_size") == 0) {
                config->gap_size = atoi(value);
            }
        } else if (strcmp(section, "appearance") == 0) {
            if (strcmp(key, "background_color") == 0) {
                free(config->background_color);
                config->background_color = strdup(value);
            } else if (strcmp(key, "border_active") == 0) {
                free(config->border_active);
                config->border_active = strdup(value);
            } else if (strcmp(key, "border_inactive") == 0) {
                free(config->border_inactive);
                config->border_inactive = strdup(value);
            }
        } else if (strcmp(section, "animations") == 0) {
            if (strcmp(key, "enabled") == 0) {
                config->animations_enabled = strcmp(value, "true") == 0;
            } else if (strcmp(key, "window_animations") == 0) {
                config->window_animations = strcmp(value, "true") == 0;
            } else if (strcmp(key, "workspace_animations") == 0) {
                config->workspace_animations = strcmp(value, "true") == 0;
            } else if (strcmp(key, "focus_animations") == 0) {
                config->focus_animations = strcmp(value, "true") == 0;
            } else if (strcmp(key, "layout_animations") == 0) {
                config->layout_animations = strcmp(value, "true") == 0;
            } else if (strcmp(key, "window_appear_duration") == 0) {
                config->window_appear_duration = atoi(value);
            } else if (strcmp(key, "window_disappear_duration") == 0) {
                config->window_disappear_duration = atoi(value);
            } else if (strcmp(key, "window_move_duration") == 0) {
                config->window_move_duration = atoi(value);
            } else if (strcmp(key, "window_resize_duration") == 0) {
                config->window_resize_duration = atoi(value);
            } else if (strcmp(key, "workspace_switch_duration") == 0) {
                config->workspace_switch_duration = atoi(value);
            } else if (strcmp(key, "focus_ring_duration") == 0) {
                config->focus_ring_duration = atoi(value);
            } else if (strcmp(key, "layout_change_duration") == 0) {
                config->layout_change_duration = atoi(value);
            } else if (strcmp(key, "speed_multiplier") == 0) {
                config->animation_speed_multiplier = atof(value);
            } else if (strcmp(key, "default_easing") == 0) {
                free(config->default_easing);
                config->default_easing = strdup(value);
            } else if (strcmp(key, "debug_mode") == 0) {
                config->animation_debug_mode = strcmp(value, "true") == 0;
            }
        } else if (strcmp(section, "effects") == 0) {
            if (strcmp(key, "shadows_enabled") == 0) {
                config->effects.shadows_enabled = strcmp(value, "true") == 0;
            } else if (strcmp(key, "blur_enabled") == 0) {
                config->effects.blur_enabled = strcmp(value, "true") == 0;
            } else if (strcmp(key, "transparency_enabled") == 0) {
                config->effects.transparency_enabled = strcmp(value, "true") == 0;
            } else if (strcmp(key, "shadow_blur_radius") == 0) {
                config->effects.shadow_blur_radius = atoi(value);
            } else if (strcmp(key, "shadow_offset_x") == 0) {
                config->effects.shadow_offset_x = atoi(value);
            } else if (strcmp(key, "shadow_offset_y") == 0) {
                config->effects.shadow_offset_y = atoi(value);
            } else if (strcmp(key, "shadow_opacity") == 0) {
                config->effects.shadow_opacity = atof(value);
            } else if (strcmp(key, "shadow_color") == 0) {
                free(config->effects.shadow_color);
                config->effects.shadow_color = strdup(value);
            } else if (strcmp(key, "blur_radius") == 0) {
                config->effects.blur_radius = atoi(value);
            } else if (strcmp(key, "blur_focus_only") == 0) {
                config->effects.blur_focus_only = strcmp(value, "true") == 0;
            } else if (strcmp(key, "blur_intensity") == 0) {
                config->effects.blur_intensity = atof(value);
            } else if (strcmp(key, "focused_opacity") == 0) {
                config->effects.focused_opacity = atof(value);
            } else if (strcmp(key, "unfocused_opacity") == 0) {
                config->effects.unfocused_opacity = atof(value);
            } else if (strcmp(key, "inactive_opacity") == 0) {
                config->effects.inactive_opacity = atof(value);
            }
        } else if (strcmp(section, "smart_gaps") == 0) {
            if (strcmp(key, "enabled") == 0) {
                config->smart_gaps.enabled = strcmp(value, "true") == 0;
            } else if (strcmp(key, "base_gap") == 0) {
                config->smart_gaps.base_gap = atoi(value);
            } else if (strcmp(key, "min_gap") == 0) {
                config->smart_gaps.min_gap = atoi(value);
            } else if (strcmp(key, "max_gap") == 0) {
                config->smart_gaps.max_gap = atoi(value);
            } else if (strcmp(key, "single_window_gap") == 0) {
                config->smart_gaps.single_window_gap = atoi(value);
            } else if (strcmp(key, "adaptive_mode") == 0) {
                free(config->smart_gaps.adaptive_mode);
                config->smart_gaps.adaptive_mode = strdup(value);
            }
        } else if (strcmp(section, "window_snapping") == 0) {
            if (strcmp(key, "enabled") == 0) {
                config->window_snapping.enabled = strcmp(value, "true") == 0;
            } else if (strcmp(key, "snap_threshold") == 0) {
                config->window_snapping.snap_threshold = atoi(value);
            } else if (strcmp(key, "edge_resistance") == 0) {
                config->window_snapping.edge_resistance = atoi(value);
            } else if (strcmp(key, "magnetism_strength") == 0) {
                config->window_snapping.magnetism_strength = atof(value);
            } else if (strcmp(key, "smart_corners") == 0) {
                config->window_snapping.smart_corners = strcmp(value, "true") == 0;
            } else if (strcmp(key, "multi_monitor_snapping") == 0) {
                config->window_snapping.multi_monitor_snapping = strcmp(value, "true") == 0;
            } else if (strcmp(key, "window_to_window_snapping") == 0) {
                config->window_snapping.window_to_window_snapping = strcmp(value, "true") == 0;
            } else if (strcmp(key, "edge_snapping") == 0) {
                config->window_snapping.edge_snapping = strcmp(value, "true") == 0;
            } else if (strcmp(key, "show_snap_preview") == 0) {
                config->window_snapping.show_snap_preview = strcmp(value, "true") == 0;
            }
        } else if (strcmp(section, "workspaces") == 0) {
            if (strcmp(key, "max_workspaces") == 0) {
                config->workspaces.max_workspaces = atoi(value);
            } else if (strcmp(key, "persistent_layouts") == 0) {
                config->workspaces.persistent_layouts = strcmp(value, "true") == 0;
            }
            // Note: workspace names parsing would require more complex array handling
        } else if (strcmp(section, "xwayland") == 0) {
            if (strcmp(key, "enabled") == 0) {
                config->xwayland.enabled = strcmp(value, "true") == 0;
            } else if (strcmp(key, "lazy") == 0) {
                config->xwayland.lazy = strcmp(value, "true") == 0;
            } else if (strcmp(key, "force_zero_scaling") == 0) {
                config->xwayland.force_zero_scaling = strcmp(value, "true") == 0;
            }
        }
    }
    
    fclose(file);
    AXIOM_LOG_INFO("Configuration loaded from %s", path);
    return true;
}
