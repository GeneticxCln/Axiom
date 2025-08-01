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
    
    // For now, create some default rules programmatically
    // TODO: Implement actual config file parsing
    
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
    
    // TODO: Implement workspace switching
    // For now, just print what we would do
    printf("Would move window to workspace %d\n", rule->workspace);
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
            // TODO: Implement proper maximization
            new_x = 0;
            new_y = 0;
            window->width = server->workspace_width;
            window->height = server->workspace_height;
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
    
    // TODO: Implement effects overrides
    if (rule->disable_shadows) {
        printf("Would disable shadows for this window\n");
    }
    
    if (rule->disable_blur) {
        printf("Would disable blur for this window\n");
    }
    
    if (rule->disable_animations) {
        printf("Would disable animations for this window\n");
    }
    
    if (rule->enable_pip) {
        printf("Would enable picture-in-picture for this window\n");
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
