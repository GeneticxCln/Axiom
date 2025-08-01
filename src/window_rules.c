#include "window_rules.h"
#include "axiom.h"
#include "effects_realtime.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fnmatch.h>
#include <wlr/types/wlr_xdg_shell.h>

// Window rules manager functions
bool axiom_window_rules_manager_init(struct axiom_window_rules_manager *manager) {
    if (!manager) return false;
    
    wl_list_init(&manager->rules);
    manager->rules_file_path = NULL;
    manager->rules_enabled = true;
    manager->rules_count = 0;
    manager->rules_applied = 0;
    manager->rules_matched = 0;
    manager->rules_failed = 0;
    
    printf("Window rules manager initialized\n");
    return true;
}

void axiom_window_rules_manager_destroy(struct axiom_window_rules_manager *manager) {
    if (!manager) return;
    
    // Clean up all rules
    struct axiom_window_rule *rule, *tmp;
    wl_list_for_each_safe(rule, tmp, &manager->rules, link) {
        wl_list_remove(&rule->link);
        axiom_window_rule_destroy(rule);
    }
    
    free(manager->rules_file_path);
    manager->rules_file_path = NULL;
    manager->rules_count = 0;
    
    printf("Window rules manager destroyed\n");
}

bool axiom_window_rules_load_config(struct axiom_window_rules_manager *manager, 
                                   const char *config_path) {
    if (!manager || !config_path) return false;
    
    // Store config path for reloading
    free(manager->rules_file_path);
    manager->rules_file_path = strdup(config_path);
    
    // Try to parse actual config file if it exists
    FILE *file = fopen(config_path, "r");
    if (file) {
        if (axiom_window_rules_parse_ini_file(manager, file)) {
            printf("Loaded %u window rules from config file: %s\n", manager->rules_count, config_path);
        } else {
            printf("Failed to parse config file, loading defaults\n");
            axiom_window_rules_load_defaults(manager);
        }
        fclose(file);
    } else {
        printf("Config file not found, loading default window rules\n");
        axiom_window_rules_load_defaults(manager);
    }
    
    printf("Loaded %u window rules from config\n", manager->rules_count);
    return true;
}

void axiom_window_rules_reload_config(struct axiom_window_rules_manager *manager) {
    if (!manager || !manager->rules_file_path) return;
    
    // Clear existing rules
    struct axiom_window_rule *rule, *tmp;
    wl_list_for_each_safe(rule, tmp, &manager->rules, link) {
        wl_list_remove(&rule->link);
        axiom_window_rule_destroy(rule);
    }
    manager->rules_count = 0;
    
    // Reload from file
    axiom_window_rules_load_config(manager, manager->rules_file_path);
}

// Rule management
struct axiom_window_rule *axiom_window_rule_create(void) {
    struct axiom_window_rule *rule = calloc(1, sizeof(struct axiom_window_rule));
    if (!rule) return NULL;
    
    wl_list_init(&rule->link);
    rule->workspace = -1;  // No workspace change by default
    rule->position = AXIOM_RULE_POS_NONE;
    rule->size = AXIOM_RULE_SIZE_NONE;
    rule->floating = AXIOM_RULE_FLOATING_UNSET;
    rule->opacity = AXIOM_RULE_OPACITY_UNSET;
    rule->custom_opacity = 1.0f;
    rule->priority = 0;
    rule->enabled = true;
    
    return rule;
}

void axiom_window_rule_destroy(struct axiom_window_rule *rule) {
    if (!rule) return;
    
    free(rule->app_id);
    free(rule->class);
    free(rule->title);
    free(rule->instance);
    free(rule->rule_name);
    free(rule);
}

bool axiom_window_rule_set_app_id(struct axiom_window_rule *rule, const char *app_id) {
    if (!rule) return false;
    
    free(rule->app_id);
    rule->app_id = app_id ? strdup(app_id) : NULL;
    return true;
}

bool axiom_window_rule_set_class(struct axiom_window_rule *rule, const char *class) {
    if (!rule) return false;
    
    free(rule->class);
    rule->class = class ? strdup(class) : NULL;
    return true;
}

bool axiom_window_rule_set_title(struct axiom_window_rule *rule, const char *title) {
    if (!rule) return false;
    
    free(rule->title);
    rule->title = title ? strdup(title) : NULL;
    return true;
}

bool axiom_window_rule_set_instance(struct axiom_window_rule *rule, const char *instance) {
    if (!rule) return false;
    
    free(rule->instance);
    rule->instance = instance ? strdup(instance) : NULL;
    return true;
}

// Rule application
bool axiom_window_rules_apply_to_window(struct axiom_window_rules_manager *manager,
                                       struct axiom_window *window) {
    if (!manager || !window || !manager->rules_enabled) return false;
    
    struct axiom_window_rule *matching_rule = axiom_window_rules_find_matching_rule(manager, window);
    if (!matching_rule) return false;
    
    printf("Applying rule '%s' to window\n", 
           matching_rule->rule_name ? matching_rule->rule_name : "unnamed");
    
    bool success = true;
    manager->rules_matched++;
    
    // Apply workspace rule
    if (matching_rule->workspace >= 0) {
        if (!axiom_window_rule_apply_workspace(matching_rule, window)) {
            success = false;
        }
    }
    
    // Apply floating rule
    if (matching_rule->floating != AXIOM_RULE_FLOATING_UNSET) {
        if (!axiom_window_rule_apply_floating(matching_rule, window)) {
            success = false;
        }
    }
    
    // Apply size rule
    if (matching_rule->size != AXIOM_RULE_SIZE_NONE) {
        if (!axiom_window_rule_apply_size(matching_rule, window)) {
            success = false;
        }
    }
    
    // Apply position rule
    if (matching_rule->position != AXIOM_RULE_POS_NONE) {
        if (!axiom_window_rule_apply_position(matching_rule, window)) {
            success = false;
        }
    }
    
    // Apply opacity rule
    if (matching_rule->opacity != AXIOM_RULE_OPACITY_UNSET) {
        if (!axiom_window_rule_apply_opacity(matching_rule, window)) {
            success = false;
        }
    }
    
    // Apply effects rules
    if (!axiom_window_rule_apply_effects(matching_rule, window)) {
        success = false;
    }
    
    if (success) {
        manager->rules_applied++;
    } else {
        manager->rules_failed++;
    }
    
    return success;
}

struct axiom_window_rule *axiom_window_rules_find_matching_rule(
    struct axiom_window_rules_manager *manager,
    struct axiom_window *window) {
    
    if (!manager || !window) return NULL;
    
    // Find highest priority matching rule
    struct axiom_window_rule *best_rule = NULL;
    int best_priority = -1;
    
    struct axiom_window_rule *rule;
    wl_list_for_each(rule, &manager->rules, link) {
        if (!rule->enabled) continue;
        
        if (axiom_window_rule_matches_window(rule, window)) {
            if (rule->priority > best_priority) {
                best_rule = rule;
                best_priority = rule->priority;
            }
        }
    }
    
    return best_rule;
}

// Rule matching functions
bool axiom_window_rule_matches_window(struct axiom_window_rule *rule, 
                                     struct axiom_window *window) {
    if (!rule || !window) return false;
    
    // Get window properties
    const char *app_id = axiom_window_get_app_id(window);
    const char *class = axiom_window_get_class(window);
    const char *title = axiom_window_get_title(window);
    const char *instance = axiom_window_get_instance(window);
    
    // Check app_id match
    if (rule->app_id && app_id) {
        if (!axiom_window_rules_pattern_match(rule->app_id, app_id)) {
            return false;
        }
    }
    
    // Check class match
    if (rule->class && class) {
        if (!axiom_window_rules_pattern_match(rule->class, class)) {
            return false;
        }
    }
    
    // Check title match
    if (rule->title && title) {
        if (!axiom_window_rules_pattern_match(rule->title, title)) {
            return false;
        }
    }
    
    // Check instance match
    if (rule->instance && instance) {
        if (!axiom_window_rules_pattern_match(rule->instance, instance)) {
            return false;
        }
    }
    
    // If we have criteria but no match, fail
    if (!rule->app_id && !rule->class && !rule->title && !rule->instance) {
        return false;  // Rule must have at least one criterion
    }
    
    return true;
}

// Window property extraction
const char *axiom_window_get_app_id(struct axiom_window *window) {
    if (!window || !window->xdg_toplevel) return NULL;
    return window->xdg_toplevel->app_id;
}

const char *axiom_window_get_class(struct axiom_window *window) {
    // For Wayland, class is typically the same as app_id
    // For X11 compatibility, we might need additional logic
    return axiom_window_get_app_id(window);
}

const char *axiom_window_get_title(struct axiom_window *window) {
    if (!window || !window->xdg_toplevel) return NULL;
    return window->xdg_toplevel->title;
}

const char *axiom_window_get_instance(struct axiom_window *window) {
    // Instance is typically derived from app_id for Wayland
    return axiom_window_get_app_id(window);
}

// Rule execution functions
bool axiom_window_rule_apply_workspace(struct axiom_window_rule *rule,
                                      struct axiom_window *window) {
    if (!rule || !window || !window->server) return false;
    
    struct axiom_server *server = window->server;
    
    // Validate workspace index
    if (rule->workspace < 0 || rule->workspace >= server->max_workspaces) {
        printf("Invalid workspace %d in rule (max: %d)\n", rule->workspace, server->max_workspaces - 1);
        return false;
    }
    
    // Move window to the specified workspace
    printf("Moving window to workspace %d\n", rule->workspace);
    axiom_move_window_to_workspace(server, window, rule->workspace);
    
    // If the window was moved to the current workspace, update window count
    if (rule->workspace == server->current_workspace && window->is_tiled) {
        server->window_count++;
        
        // Re-arrange windows to accommodate the new window
        if (server->tiling_enabled) {
            axiom_arrange_windows(server);
        }
    }
    
    printf("Applied workspace rule: moved window to workspace %d\n", rule->workspace);
    return true;
}

bool axiom_window_rule_apply_position(struct axiom_window_rule *rule,
                                     struct axiom_window *window) {
    if (!rule || !window || !window->server) return false;
    
    struct axiom_server *server = window->server;
    int new_x = window->x;
    int new_y = window->y;
    
    switch (rule->position) {
        case AXIOM_RULE_POS_CENTER:
            new_x = (server->workspace_width - window->width) / 2;
            new_y = (server->workspace_height - window->height) / 2;
            break;
            
        case AXIOM_RULE_POS_TOP_LEFT:
            new_x = 50;
            new_y = 50;
            break;
            
        case AXIOM_RULE_POS_TOP_RIGHT:
            new_x = server->workspace_width - window->width - 50;
            new_y = 50;
            break;
            
        case AXIOM_RULE_POS_BOTTOM_LEFT:
            new_x = 50;
            new_y = server->workspace_height - window->height - 50;
            break;
            
        case AXIOM_RULE_POS_BOTTOM_RIGHT:
            new_x = server->workspace_width - window->width - 50;
            new_y = server->workspace_height - window->height - 50;
            break;
            
        case AXIOM_RULE_POS_MAXIMIZED:
            // Implement proper maximization
            new_x = 0;
            new_y = 0;
            window->width = server->workspace_width;
            window->height = server->workspace_height;
            
            // Save current state for potential restoration
            window->saved_x = window->x;
            window->saved_y = window->y;
            window->saved_width = window->width;
            window->saved_height = window->height;
            
            // Set maximized state
            window->is_maximized = true;
            
            // Request client to maximize
            if (window->xdg_toplevel) {
                wlr_xdg_toplevel_set_maximized(window->xdg_toplevel, true);
            }
            break;
            
        case AXIOM_RULE_POS_CUSTOM:
            new_x = rule->custom_x;
            new_y = rule->custom_y;
            break;
            
        default:
            return true;  // No position change
    }
    
    // Apply position
    window->x = new_x;
    window->y = new_y;
    
    // Update scene node position
    if (window->scene_tree) {
        wlr_scene_node_set_position(&window->scene_tree->node, new_x, new_y);
    }
    
    printf("Applied position rule: moved window to %d,%d\n", new_x, new_y);
    return true;
}

bool axiom_window_rule_apply_size(struct axiom_window_rule *rule,
                                 struct axiom_window *window) {
    if (!rule || !window) return false;
    
    int new_width = window->width;
    int new_height = window->height;
    
    switch (rule->size) {
        case AXIOM_RULE_SIZE_SMALL:
            new_width = 400;
            new_height = 300;
            break;
            
        case AXIOM_RULE_SIZE_MEDIUM:
            new_width = 800;
            new_height = 600;
            break;
            
        case AXIOM_RULE_SIZE_LARGE:
            new_width = 1200;
            new_height = 800;
            break;
            
        case AXIOM_RULE_SIZE_CUSTOM:
            new_width = rule->custom_width;
            new_height = rule->custom_height;
            break;
            
        default:
            return true;  // No size change
    }
    
    // Apply size
    window->width = new_width;
    window->height = new_height;
    
    // Request resize from the client
    if (window->xdg_toplevel) {
        wlr_xdg_toplevel_set_size(window->xdg_toplevel, new_width, new_height);
    }
    
    printf("Applied size rule: resized window to %dx%d\n", new_width, new_height);
    return true;
}

bool axiom_window_rule_apply_floating(struct axiom_window_rule *rule,
                                     struct axiom_window *window) {
    if (!rule || !window) return false;
    
    switch (rule->floating) {
        case AXIOM_RULE_FLOATING_FORCE_TILED:
            window->is_tiled = true;
            printf("Applied floating rule: forced window to tiled\n");
            break;
            
        case AXIOM_RULE_FLOATING_FORCE_FLOATING:
            window->is_tiled = false;
            printf("Applied floating rule: forced window to floating\n");
            break;
            
        default:
            return true;  // No floating change
    }
    
    return true;
}

bool axiom_window_rule_apply_opacity(struct axiom_window_rule *rule,
                                    struct axiom_window *window) {
    if (!rule || !window) return false;
    
    float opacity = 1.0f;
    
    switch (rule->opacity) {
        case AXIOM_RULE_OPACITY_OPAQUE:
            opacity = 1.0f;
            break;
            
        case AXIOM_RULE_OPACITY_TRANSPARENT:
            opacity = 0.8f;
            break;
            
        case AXIOM_RULE_OPACITY_CUSTOM:
            opacity = rule->custom_opacity;
            break;
            
        default:
            return true;  // No opacity change
    }
    
    // Apply opacity through effects system
    if (window->effects) {
        axiom_effects_set_window_opacity(window, opacity);
        printf("Applied opacity rule: set window opacity to %.2f\n", opacity);
    }
    
    return true;
}

bool axiom_window_rule_apply_effects(struct axiom_window_rule *rule,
                                    struct axiom_window *window) {
    if (!rule || !window) return false;
    
    struct axiom_server *server = window->server;
    bool effects_changed = false;
    
    // Initialize window effects if not already done
    if (!window->effects && server && server->effects_manager) {
        axiom_window_effects_init(window);
    }
    
    // Apply shadow disable/enable
    if (rule->disable_shadows && window->effects) {
        // Create a local shadow config with shadows disabled
        struct axiom_shadow_config shadow_config = server->effects_manager->shadow;
        shadow_config.enabled = false;
        axiom_effects_update_shadow_config(window, &shadow_config);
        printf("Disabled shadows for window per rule\n");
        effects_changed = true;
    }
    
    // Apply blur disable/enable
    if (rule->disable_blur && window->effects) {
        // Create a local blur config with blur disabled
        struct axiom_blur_config blur_config = server->effects_manager->blur;
        blur_config.enabled = false;
        axiom_effects_update_blur_config(window, &blur_config);
        printf("Disabled blur for window per rule\n");
        effects_changed = true;
    }
    
    // Apply transparency override
    if (rule->custom_opacity != 1.0f && window->effects) {
        axiom_effects_set_window_opacity(window, rule->custom_opacity);
        printf("Set custom opacity %.2f for window per rule\n", rule->custom_opacity);
        effects_changed = true;
    }
    
    // Picture-in-picture mode implementation
    if (rule->enable_pip) {
        // PiP typically involves keeping window on top and making it smaller
        window->is_tiled = false; // Force floating for PiP
        
        // Set a smaller size if not already set by size rule
        if (rule->size == AXIOM_RULE_SIZE_NONE) {
            window->width = 320;
            window->height = 240;
            if (window->xdg_toplevel) {
                wlr_xdg_toplevel_set_size(window->xdg_toplevel, window->width, window->height);
            }
        }
        
        // Position in corner if not already set by position rule
        if (rule->position == AXIOM_RULE_POS_NONE && server) {
            window->x = server->workspace_width - window->width - 20;
            window->y = server->workspace_height - window->height - 20;
            if (window->scene_tree) {
                wlr_scene_node_set_position(&window->scene_tree->node, window->x, window->y);
            }
        }
        
        printf("Enabled picture-in-picture mode for window\n");
        effects_changed = true;
    }
    
    // Mark effects for update if changes were made
    if (effects_changed && window->effects) {
        axiom_effects_mark_dirty(window);
    }
    
    return true;
}

// Pattern matching utilities
bool axiom_window_rules_pattern_match(const char *pattern, const char *text) {
    if (!pattern || !text) return false;
    
    // Use glob-style pattern matching
    return axiom_window_rules_glob_match(pattern, text);
}

bool axiom_window_rules_glob_match(const char *pattern, const char *text) {
    if (!pattern || !text) return false;
    
    // Use fnmatch for glob pattern matching
    return fnmatch(pattern, text, FNM_CASEFOLD) == 0;
}

// Debug and statistics
void axiom_window_rules_print_statistics(struct axiom_window_rules_manager *manager) {
    if (!manager) return;
    
    printf("Window Rules Statistics:\n");
    printf("  Total rules: %u\n", manager->rules_count);
    printf("  Rules matched: %u\n", manager->rules_matched);
    printf("  Rules applied: %u\n", manager->rules_applied);
    printf("  Rules failed: %u\n", manager->rules_failed);
}

void axiom_window_rules_print_rules(struct axiom_window_rules_manager *manager) {
    if (!manager) return;
    
    printf("Window Rules (%u total):\n", manager->rules_count);
    
    struct axiom_window_rule *rule;
    wl_list_for_each(rule, &manager->rules, link) {
        printf("  Rule: %s (priority: %d, enabled: %s)\n",
               rule->rule_name ? rule->rule_name : "unnamed",
               rule->priority,
               rule->enabled ? "yes" : "no");
        
        if (rule->app_id) printf("    app_id: %s\n", rule->app_id);
        if (rule->class) printf("    class: %s\n", rule->class);
        if (rule->title) printf("    title: %s\n", rule->title);
        if (rule->workspace >= 0) printf("    workspace: %d\n", rule->workspace);
    }
}

void axiom_window_rules_debug_window_properties(struct axiom_window *window) {
    if (!window) return;
    
    printf("Window Properties:\n");
    printf("  app_id: %s\n", axiom_window_get_app_id(window) ?: "(null)");
    printf("  class: %s\n", axiom_window_get_class(window) ?: "(null)");
    printf("  title: %s\n", axiom_window_get_title(window) ?: "(null)");
    printf("  instance: %s\n", axiom_window_get_instance(window) ?: "(null)");
}

// Integration with server
bool axiom_server_init_window_rules(struct axiom_server *server) {
    if (!server) return false;
    
    server->window_rules_manager = calloc(1, sizeof(struct axiom_window_rules_manager));
    if (!server->window_rules_manager) {
        return false;
    }
    
    if (!axiom_window_rules_manager_init(server->window_rules_manager)) {
        free(server->window_rules_manager);
        server->window_rules_manager = NULL;
        return false;
    }
    
    // Try to load rules from config
    const char *config_paths[] = {
        "./rules.conf",
        "./examples/rules.conf",
        "~/.config/axiom/rules.conf",
        "/etc/axiom/rules.conf",
        NULL
    };
    
    for (int i = 0; config_paths[i]; i++) {
        if (axiom_window_rules_load_config(server->window_rules_manager, config_paths[i])) {
            break;
        }
    }
    
    printf("Window rules system initialized\n");
    return true;
}

void axiom_server_destroy_window_rules(struct axiom_server *server) {
    if (!server || !server->window_rules_manager) return;
    
    axiom_window_rules_manager_destroy(server->window_rules_manager);
    free(server->window_rules_manager);
    server->window_rules_manager = NULL;
    
    printf("Window rules system destroyed\n");
}

// INI file parsing implementation
bool axiom_window_rules_parse_ini_file(struct axiom_window_rules_manager *manager, FILE *file) {
    if (!manager || !file) return false;
    
    char line[512];
    char current_section[128] = "";
    struct axiom_window_rule *current_rule = NULL;
    
    while (fgets(line, sizeof(line), file)) {
        // Remove newline and whitespace
        axiom_window_rules_trim_string(line);
        
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Check for section header
        if (line[0] == '[' && line[strlen(line)-1] == ']') {
            // Finish previous rule
            if (current_rule) {
                wl_list_insert(&manager->rules, &current_rule->link);
                manager->rules_count++;
            }
            
            // Start new section/rule
            strncpy(current_section, line + 1, sizeof(current_section) - 1);
            current_section[strlen(current_section) - 1] = '\0'; // Remove ]
            
            current_rule = axiom_window_rule_create();
            if (current_rule) {
                current_rule->rule_name = strdup(current_section);
            }
            continue;
        }
        
        // Parse key=value pairs
        if (current_rule) {
            axiom_window_rules_parse_ini_line(current_rule, line);
        }
    }
    
    // Finish last rule
    if (current_rule) {
        wl_list_insert(&manager->rules, &current_rule->link);
        manager->rules_count++;
    }
    
    return manager->rules_count > 0;
}

void axiom_window_rules_parse_ini_line(struct axiom_window_rule *rule, const char *line) {
    if (!rule || !line) return;
    
    char key[128], value[256];
    if (sscanf(line, "%127[^=]=%255[^\n]", key, value) != 2) {
        return;
    }
    
    // Trim key and value
    axiom_window_rules_trim_string(key);
    axiom_window_rules_trim_string(value);
    
    // Remove quotes from value if present
    if (value[0] == '"' && value[strlen(value)-1] == '"') {
        value[strlen(value)-1] = '\0';
        memmove(value, value + 1, strlen(value));
    }
    
    // Parse different settings
    if (strcmp(key, "app_id") == 0) {
        axiom_window_rule_set_app_id(rule, value);
    } else if (strcmp(key, "class") == 0) {
        axiom_window_rule_set_class(rule, value);
    } else if (strcmp(key, "title") == 0) {
        axiom_window_rule_set_title(rule, value);
    } else if (strcmp(key, "instance") == 0) {
        axiom_window_rule_set_instance(rule, value);
    } else if (strcmp(key, "workspace") == 0) {
        rule->workspace = atoi(value);
    } else if (strcmp(key, "priority") == 0) {
        rule->priority = atoi(value);
    } else if (strcmp(key, "enabled") == 0) {
        rule->enabled = (strcmp(value, "true") == 0 || strcmp(value, "yes") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "floating") == 0) {
        if (strcmp(value, "force_floating") == 0) {
            rule->floating = AXIOM_RULE_FLOATING_FORCE_FLOATING;
        } else if (strcmp(value, "force_tiled") == 0) {
            rule->floating = AXIOM_RULE_FLOATING_FORCE_TILED;
        }
    } else if (strcmp(key, "position") == 0) {
        if (strcmp(value, "center") == 0) {
            rule->position = AXIOM_RULE_POS_CENTER;
        } else if (strcmp(value, "maximized") == 0) {
            rule->position = AXIOM_RULE_POS_MAXIMIZED;
        } else if (strcmp(value, "top-left") == 0) {
            rule->position = AXIOM_RULE_POS_TOP_LEFT;
        } else if (strcmp(value, "top-right") == 0) {
            rule->position = AXIOM_RULE_POS_TOP_RIGHT;
        } else if (strcmp(value, "bottom-left") == 0) {
            rule->position = AXIOM_RULE_POS_BOTTOM_LEFT;
        } else if (strcmp(value, "bottom-right") == 0) {
            rule->position = AXIOM_RULE_POS_BOTTOM_RIGHT;
        }
    } else if (strcmp(key, "size") == 0) {
        if (strcmp(value, "small") == 0) {
            rule->size = AXIOM_RULE_SIZE_SMALL;
        } else if (strcmp(value, "medium") == 0) {
            rule->size = AXIOM_RULE_SIZE_MEDIUM;
        } else if (strcmp(value, "large") == 0) {
            rule->size = AXIOM_RULE_SIZE_LARGE;
        } else {
            // Try to parse custom size (e.g., "800x600")
            int w, h;
            if (sscanf(value, "%dx%d", &w, &h) == 2) {
                rule->size = AXIOM_RULE_SIZE_CUSTOM;
                rule->custom_width = w;
                rule->custom_height = h;
            }
        }
    } else if (strcmp(key, "disable_shadows") == 0) {
        rule->disable_shadows = (strcmp(value, "true") == 0 || strcmp(value, "yes") == 0);
    } else if (strcmp(key, "disable_blur") == 0) {
        rule->disable_blur = (strcmp(value, "true") == 0 || strcmp(value, "yes") == 0);
    } else if (strcmp(key, "disable_animations") == 0) {
        rule->disable_animations = (strcmp(value, "true") == 0 || strcmp(value, "yes") == 0);
    } else if (strcmp(key, "picture_in_picture") == 0) {
        rule->enable_pip = (strcmp(value, "true") == 0 || strcmp(value, "yes") == 0);
    }
}

void axiom_window_rules_trim_string(char *str) {
    if (!str) return;
    
    // Trim leading whitespace
    char *start = str;
    while (*start && (*start == ' ' || *start == '\t')) {
        start++;
    }
    
    // Trim trailing whitespace
    char *end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    
    // Move trimmed string to beginning
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

// Load default rules when no config file is found
bool axiom_window_rules_load_defaults(struct axiom_window_rules_manager *manager) {
    if (!manager) return false;
    
    // Rule for Firefox - workspace 1, maximized
    struct axiom_window_rule *firefox_rule = axiom_window_rule_create();
    if (firefox_rule) {
        axiom_window_rule_set_app_id(firefox_rule, "firefox");
        firefox_rule->workspace = 1;
        firefox_rule->position = AXIOM_RULE_POS_MAXIMIZED;
        firefox_rule->priority = 100;
        firefox_rule->rule_name = strdup("firefox-rule");
        wl_list_insert(&manager->rules, &firefox_rule->link);
        manager->rules_count++;
    }
    
    // Rule for VS Code - workspace 2, large size
    struct axiom_window_rule *code_rule = axiom_window_rule_create();
    if (code_rule) {
        axiom_window_rule_set_app_id(code_rule, "code");
        axiom_window_rule_set_class(code_rule, "Code");
        code_rule->workspace = 2;
        code_rule->size = AXIOM_RULE_SIZE_LARGE;
        code_rule->position = AXIOM_RULE_POS_CENTER;
        code_rule->priority = 100;
        code_rule->rule_name = strdup("vscode-rule");
        wl_list_insert(&manager->rules, &code_rule->link);
        manager->rules_count++;
    }
    
    // Rule for media players - floating, PiP enabled
    struct axiom_window_rule *media_rule = axiom_window_rule_create();
    if (media_rule) {
        axiom_window_rule_set_app_id(media_rule, "mpv");
        media_rule->floating = AXIOM_RULE_FLOATING_FORCE_FLOATING;
        media_rule->size = AXIOM_RULE_SIZE_MEDIUM;
        media_rule->position = AXIOM_RULE_POS_BOTTOM_RIGHT;
        media_rule->enable_pip = true;
        media_rule->priority = 90;
        media_rule->rule_name = strdup("media-player-rule");
        wl_list_insert(&manager->rules, &media_rule->link);
        manager->rules_count++;
    }
    
    // Rule for calculator - small floating window
    struct axiom_window_rule *calc_rule = axiom_window_rule_create();
    if (calc_rule) {
        axiom_window_rule_set_class(calc_rule, "gnome-calculator");
        axiom_window_rule_set_app_id(calc_rule, "org.gnome.Calculator");
        calc_rule->floating = AXIOM_RULE_FLOATING_FORCE_FLOATING;
        calc_rule->size = AXIOM_RULE_SIZE_SMALL;
        calc_rule->position = AXIOM_RULE_POS_TOP_RIGHT;
        calc_rule->priority = 95;
        calc_rule->rule_name = strdup("calculator-rule");
        wl_list_insert(&manager->rules, &calc_rule->link);
        manager->rules_count++;
    }
    
    printf("Loaded %u default window rules\n", manager->rules_count);
    return true;
}
