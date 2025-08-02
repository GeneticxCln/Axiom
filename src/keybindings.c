#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wlr/types/wlr_keyboard.h>
#include "axiom.h"
#include "keybindings.h"
#include "tagging.h"
#include "focus.h"

void axiom_keybinding_manager_init(struct axiom_keybinding_manager *manager) {
    if (!manager) return;
    
    memset(manager, 0, sizeof(struct axiom_keybinding_manager));
    manager->binding_count = 0;
    manager->capture_mode = false;
    manager->pending_binding = NULL;
    
    // Load default key bindings
    axiom_keybinding_load_defaults(manager);
    
    AXIOM_LOG_INFO("Key binding manager initialized with %d default bindings", 
                   manager->binding_count);
}

void axiom_keybinding_manager_cleanup(struct axiom_keybinding_manager *manager) {
    if (!manager) return;
    
    // No dynamic memory to clean up currently
    manager->binding_count = 0;
}

bool axiom_keybinding_add(struct axiom_keybinding_manager *manager,
                         uint32_t modifiers, xkb_keysym_t keysym,
                         enum axiom_action_type action, int parameter,
                         const char *command, const char *description) {
    if (!manager || manager->binding_count >= AXIOM_MAX_KEYBINDINGS) {
        return false;
    }
    
    // Check for existing binding
    if (axiom_keybinding_find(manager, modifiers, keysym)) {
        AXIOM_LOG_INFO("Key binding already exists, updating...");
        return axiom_keybinding_update(manager, modifiers, keysym, action, parameter, command);
    }
    
    struct axiom_keybinding *binding = &manager->bindings[manager->binding_count];
    binding->modifiers = modifiers;
    binding->keysym = keysym;
    binding->action = action;
    binding->parameter = parameter;
    binding->enabled = true;
    binding->is_macro = false;
    binding->macro_step_count = 0;
    
    if (command) {
        strncpy(binding->command, command, AXIOM_MAX_COMMAND_LENGTH - 1);
        binding->command[AXIOM_MAX_COMMAND_LENGTH - 1] = '\0';
    } else {
        binding->command[0] = '\0';
    }
    
    if (description) {
        strncpy(binding->description, description, sizeof(binding->description) - 1);
        binding->description[sizeof(binding->description) - 1] = '\0';
    } else {
        snprintf(binding->description, sizeof(binding->description), 
                "Action: %s", axiom_keybinding_action_to_string(action));
    }
    
    manager->binding_count++;
    
    AXIOM_LOG_INFO("Added key binding: %s+%s -> %s", 
                   axiom_keybinding_modifiers_to_string(modifiers),
                   axiom_keybinding_keysym_to_string(keysym),
                   binding->description);
    
    return true;
}

bool axiom_keybinding_add_macro(struct axiom_keybinding_manager *manager,
                               uint32_t modifiers, xkb_keysym_t keysym,
                               const struct axiom_macro_step *steps, int step_count,
                               const char *description) {
    if (!manager || manager->binding_count >= AXIOM_MAX_KEYBINDINGS || !steps || 
        step_count <= 0 || step_count > AXIOM_MAX_MACRO_STEPS) {
        return false;
    }
    
    struct axiom_keybinding *binding = &manager->bindings[manager->binding_count];
    binding->modifiers = modifiers;
    binding->keysym = keysym;
    binding->action = AXIOM_ACTION_MACRO;
    binding->parameter = 0;
    binding->enabled = true;
    binding->is_macro = true;
    binding->macro_step_count = step_count;
    binding->command[0] = '\0';
    
    // Copy macro steps
    memcpy(binding->macro_steps, steps, sizeof(struct axiom_macro_step) * step_count);
    
    if (description) {
        strncpy(binding->description, description, sizeof(binding->description) - 1);
        binding->description[sizeof(binding->description) - 1] = '\0';
    } else {
        snprintf(binding->description, sizeof(binding->description), 
                "Macro with %d steps", step_count);
    }
    
    manager->binding_count++;
    
    AXIOM_LOG_INFO("Added macro binding: %s+%s -> %s", 
                   axiom_keybinding_modifiers_to_string(modifiers),
                   axiom_keybinding_keysym_to_string(keysym),
                   binding->description);
    
    return true;
}

bool axiom_keybinding_remove(struct axiom_keybinding_manager *manager,
                            uint32_t modifiers, xkb_keysym_t keysym) {
    if (!manager) return false;
    
    for (int i = 0; i < manager->binding_count; i++) {
        if (manager->bindings[i].modifiers == modifiers && 
            manager->bindings[i].keysym == keysym) {
            
            // Shift remaining bindings down
            for (int j = i; j < manager->binding_count - 1; j++) {
                manager->bindings[j] = manager->bindings[j + 1];
            }
            
            manager->binding_count--;
            
            AXIOM_LOG_INFO("Removed key binding: %s+%s", 
                          axiom_keybinding_modifiers_to_string(modifiers),
                          axiom_keybinding_keysym_to_string(keysym));
            
            return true;
        }
    }
    
    return false;
}

bool axiom_keybinding_enable(struct axiom_keybinding_manager *manager,
                            uint32_t modifiers, xkb_keysym_t keysym, bool enabled) {
    struct axiom_keybinding *binding = axiom_keybinding_find(manager, modifiers, keysym);
    if (!binding) return false;
    
    binding->enabled = enabled;
    
    AXIOM_LOG_INFO("%s key binding: %s+%s", 
                   enabled ? "Enabled" : "Disabled",
                   axiom_keybinding_modifiers_to_string(modifiers),
                   axiom_keybinding_keysym_to_string(keysym));
    
    return true;
}

bool axiom_keybinding_update(struct axiom_keybinding_manager *manager,
                            uint32_t modifiers, xkb_keysym_t keysym,
                            enum axiom_action_type new_action, int new_parameter,
                            const char *new_command) {
    struct axiom_keybinding *binding = axiom_keybinding_find(manager, modifiers, keysym);
    if (!binding) return false;
    
    binding->action = new_action;
    binding->parameter = new_parameter;
    
    if (new_command) {
        strncpy(binding->command, new_command, AXIOM_MAX_COMMAND_LENGTH - 1);
        binding->command[AXIOM_MAX_COMMAND_LENGTH - 1] = '\0';
    }
    
    AXIOM_LOG_INFO("Updated key binding: %s+%s", 
                   axiom_keybinding_modifiers_to_string(modifiers),
                   axiom_keybinding_keysym_to_string(keysym));
    
    return true;
}

bool axiom_keybinding_handle_key(struct axiom_server *server,
                                uint32_t modifiers, xkb_keysym_t keysym) {
    if (!server || !server->keybinding_manager) return false;
    
    // Convert wlroots modifiers to our format
    uint32_t our_modifiers = 0;
    if (modifiers & WLR_MODIFIER_SHIFT) our_modifiers |= AXIOM_MOD_SHIFT;
    if (modifiers & WLR_MODIFIER_CTRL) our_modifiers |= AXIOM_MOD_CTRL;
    if (modifiers & WLR_MODIFIER_ALT) our_modifiers |= AXIOM_MOD_ALT;
    if (modifiers & WLR_MODIFIER_LOGO) our_modifiers |= AXIOM_MOD_SUPER;
    
    struct axiom_keybinding *binding = axiom_keybinding_find(server->keybinding_manager, 
                                                            our_modifiers, keysym);
    
    if (!binding || !binding->enabled) {
        return false;
    }
    
    // Execute the binding
    if (binding->is_macro) {
        axiom_keybinding_execute_macro(server, binding->macro_steps, binding->macro_step_count);
    } else {
        axiom_keybinding_execute_action(server, binding->action, binding->parameter, binding->command);
    }
    
    return true;
}

void axiom_keybinding_execute_action(struct axiom_server *server,
                                   enum axiom_action_type action, int parameter,
                                   const char *command) {
    if (!server) return;
    
    switch (action) {
        case AXIOM_ACTION_COMMAND:
            if (command && command[0]) {
                // Execute shell command (simplified - in real implementation use process spawning)
                system(command);
                AXIOM_LOG_INFO("Executed command: %s", command);
            }
            break;
            
        case AXIOM_ACTION_WINDOW_CLOSE:
            if (server->focused_window && server->focused_window->xdg_toplevel) {
                wlr_xdg_toplevel_send_close(server->focused_window->xdg_toplevel);
            }
            break;
            
        case AXIOM_ACTION_WINDOW_KILL:
            if (server->focused_window) {
                // Force kill the window's client
                if (server->focused_window->xdg_toplevel) {
                    struct wl_client *client = wl_resource_get_client(server->focused_window->xdg_toplevel->base->resource);
                    wl_client_destroy(client);
                    AXIOM_LOG_INFO("Force killed window client");
                }
            }
            break;
            
        case AXIOM_ACTION_WINDOW_FULLSCREEN:
            if (server->focused_window) {
                bool is_fullscreen = server->focused_window->is_fullscreen;
                wlr_xdg_toplevel_set_fullscreen(server->focused_window->xdg_toplevel, !is_fullscreen);
                server->focused_window->is_fullscreen = !is_fullscreen;
            }
            break;
            
        case AXIOM_ACTION_WINDOW_MAXIMIZE:
            if (server->focused_window) {
                bool is_maximized = server->focused_window->is_maximized;
                wlr_xdg_toplevel_set_maximized(server->focused_window->xdg_toplevel, !is_maximized);
                server->focused_window->is_maximized = !is_maximized;
            }
            break;
            
        case AXIOM_ACTION_WINDOW_FLOATING:
            axiom_toggle_window_floating(server, server->focused_window);
            break;
            
        case AXIOM_ACTION_WINDOW_STICKY:
            if (server->focused_window && server->tag_manager) {
                axiom_window_toggle_sticky(server, server->focused_window);
            }
            break;
            
        case AXIOM_ACTION_LAYOUT_CYCLE:
            axiom_cycle_layout(server);
            break;
            
        case AXIOM_ACTION_LAYOUT_SET:
            if (parameter >= 0 && parameter < 4) {
                axiom_set_layout((enum axiom_layout_type)parameter);
                axiom_arrange_windows(server);
            }
            break;
            
        case AXIOM_ACTION_MASTER_RATIO_INC:
            axiom_adjust_master_ratio(0.05f);
            axiom_arrange_windows(server);
            break;
            
        case AXIOM_ACTION_MASTER_RATIO_DEC:
            axiom_adjust_master_ratio(-0.05f);
            axiom_arrange_windows(server);
            break;
            
        case AXIOM_ACTION_TAG_VIEW:
            if (server->tag_manager && parameter >= 1 && parameter <= AXIOM_TAGS_MAX) {
                uint32_t tag = axiom_tag_mask_from_number(parameter);
                axiom_tag_view(server, tag);
            }
            break;
            
        case AXIOM_ACTION_TAG_TOGGLE_VIEW:
            if (server->tag_manager && parameter >= 1 && parameter <= AXIOM_TAGS_MAX) {
                uint32_t tag = axiom_tag_mask_from_number(parameter);
                axiom_tag_toggle_view(server, tag);
            }
            break;
            
        case AXIOM_ACTION_TAG_VIEW_ALL:
            if (server->tag_manager) {
                axiom_tag_view_all(server);
            }
            break;
            
        case AXIOM_ACTION_TAG_VIEW_PREVIOUS:
            if (server->tag_manager) {
                axiom_tag_view_previous(server);
            }
            break;
            
        case AXIOM_ACTION_WINDOW_TAG:
            if (server->focused_window && server->tag_manager && parameter >= 1 && parameter <= AXIOM_TAGS_MAX) {
                uint32_t tag = axiom_tag_mask_from_number(parameter);
                axiom_window_move_to_tag(server, server->focused_window, tag);
            }
            break;
            
        case AXIOM_ACTION_WINDOW_TAG_TOGGLE:
            if (server->focused_window && server->tag_manager && parameter >= 1 && parameter <= AXIOM_TAGS_MAX) {
                uint32_t tag = axiom_tag_mask_from_number(parameter);
                axiom_window_toggle_tag(server, server->focused_window, tag);
            }
            break;
            
        case AXIOM_ACTION_QUIT:
            wl_display_terminate(server->wl_display);
            break;
            
        case AXIOM_ACTION_RELOAD_CONFIG:
            axiom_reload_configuration(server);
            break;
            
        case AXIOM_ACTION_FOCUS_NEXT:
            axiom_focus_next_window(server);
            break;
            
        case AXIOM_ACTION_FOCUS_PREV:
            axiom_focus_prev_window(server);
            break;
            
        case AXIOM_ACTION_FOCUS_URGENT:
            axiom_focus_urgent_window(server);
            break;
            
        default:
            AXIOM_LOG_INFO("Action not implemented: %s", axiom_keybinding_action_to_string(action));
            break;
    }
}

void axiom_keybinding_execute_macro(struct axiom_server *server,
                                  const struct axiom_macro_step *steps, int step_count) {
    if (!server || !steps || step_count <= 0) return;
    
    AXIOM_LOG_INFO("Executing macro with %d steps", step_count);
    
    for (int i = 0; i < step_count; i++) {
        axiom_keybinding_execute_action(server, steps[i].action, steps[i].parameter, steps[i].command);
        
        // Small delay between macro steps (could be configurable)
        usleep(50000); // 50ms
    }
}

void axiom_keybinding_load_defaults(struct axiom_keybinding_manager *manager) {
    if (!manager) return;
    
    // Application launchers
    axiom_keybinding_add(manager, AXIOM_MOD_SUPER, XKB_KEY_Return, 
                        AXIOM_ACTION_COMMAND, 0, "foot", "Launch terminal");
    axiom_keybinding_add(manager, AXIOM_MOD_SUPER, XKB_KEY_d, 
                        AXIOM_ACTION_COMMAND, 0, "rofi -show drun", "Launch application launcher");
    
    // Window management
    axiom_keybinding_add(manager, AXIOM_MOD_SUPER, XKB_KEY_w, 
                        AXIOM_ACTION_WINDOW_CLOSE, 0, NULL, "Close window");
    axiom_keybinding_add(manager, AXIOM_MOD_SUPER, XKB_KEY_k, 
                        AXIOM_ACTION_WINDOW_KILL, 0, NULL, "Kill window");
    axiom_keybinding_add(manager, AXIOM_MOD_SUPER, XKB_KEY_f, 
                        AXIOM_ACTION_WINDOW_FULLSCREEN, 0, NULL, "Toggle fullscreen");
    axiom_keybinding_add(manager, AXIOM_MOD_SUPER, XKB_KEY_m, 
                        AXIOM_ACTION_WINDOW_MAXIMIZE, 0, NULL, "Toggle maximize");
    axiom_keybinding_add(manager, AXIOM_MOD_SUPER, XKB_KEY_space, 
                        AXIOM_ACTION_WINDOW_FLOATING, 0, NULL, "Toggle floating");
    axiom_keybinding_add(manager, AXIOM_MOD_SUPER, XKB_KEY_s, 
                        AXIOM_ACTION_WINDOW_STICKY, 0, NULL, "Toggle sticky");
    
    // Layout management
    axiom_keybinding_add(manager, AXIOM_MOD_SUPER, XKB_KEY_l, 
                        AXIOM_ACTION_LAYOUT_CYCLE, 0, NULL, "Cycle layouts");
    axiom_keybinding_add(manager, AXIOM_MOD_SUPER, XKB_KEY_h, 
                        AXIOM_ACTION_MASTER_RATIO_DEC, 0, NULL, "Decrease master ratio");
    axiom_keybinding_add(manager, AXIOM_MOD_SUPER, XKB_KEY_j, 
                        AXIOM_ACTION_MASTER_RATIO_INC, 0, NULL, "Increase master ratio");
    
    // Tag management (view tags)
    for (int i = 1; i <= AXIOM_TAGS_MAX; i++) {
        xkb_keysym_t key = XKB_KEY_1 + (i - 1);
        axiom_keybinding_add(manager, AXIOM_MOD_SUPER, key, 
                            AXIOM_ACTION_TAG_VIEW, i, NULL, "View tag");
        axiom_keybinding_add(manager, AXIOM_MOD_SUPER | AXIOM_MOD_SHIFT, key, 
                            AXIOM_ACTION_WINDOW_TAG, i, NULL, "Move window to tag");
        axiom_keybinding_add(manager, AXIOM_MOD_SUPER | AXIOM_MOD_CTRL, key, 
                            AXIOM_ACTION_TAG_TOGGLE_VIEW, i, NULL, "Toggle view tag");
        axiom_keybinding_add(manager, AXIOM_MOD_SUPER | AXIOM_MOD_CTRL | AXIOM_MOD_SHIFT, key, 
                            AXIOM_ACTION_WINDOW_TAG_TOGGLE, i, NULL, "Toggle window tag");
    }
    
    // Special tag operations
    axiom_keybinding_add(manager, AXIOM_MOD_SUPER, XKB_KEY_0, 
                        AXIOM_ACTION_TAG_VIEW_ALL, 0, NULL, "View all tags");
    axiom_keybinding_add(manager, AXIOM_MOD_SUPER, XKB_KEY_Tab, 
                        AXIOM_ACTION_TAG_VIEW_PREVIOUS, 0, NULL, "View previous tags");
    
    // System control
    axiom_keybinding_add(manager, AXIOM_MOD_SUPER, XKB_KEY_q, 
                        AXIOM_ACTION_QUIT, 0, NULL, "Quit compositor");
    axiom_keybinding_add(manager, AXIOM_MOD_SUPER, XKB_KEY_r, 
                        AXIOM_ACTION_RELOAD_CONFIG, 0, NULL, "Reload configuration");
    
    AXIOM_LOG_INFO("Loaded %d default key bindings", manager->binding_count);
}

// Utility functions
const char *axiom_keybinding_action_to_string(enum axiom_action_type action) {
    switch (action) {
        case AXIOM_ACTION_COMMAND: return "command";
        case AXIOM_ACTION_WINDOW_CLOSE: return "window_close";
        case AXIOM_ACTION_WINDOW_KILL: return "window_kill";
        case AXIOM_ACTION_WINDOW_FULLSCREEN: return "window_fullscreen";
        case AXIOM_ACTION_WINDOW_MAXIMIZE: return "window_maximize";
        case AXIOM_ACTION_WINDOW_FLOATING: return "window_floating";
        case AXIOM_ACTION_WINDOW_STICKY: return "window_sticky";
        case AXIOM_ACTION_LAYOUT_CYCLE: return "layout_cycle";
        case AXIOM_ACTION_LAYOUT_SET: return "layout_set";
        case AXIOM_ACTION_MASTER_RATIO_INC: return "master_ratio_inc";
        case AXIOM_ACTION_MASTER_RATIO_DEC: return "master_ratio_dec";
        case AXIOM_ACTION_TAG_VIEW: return "tag_view";
        case AXIOM_ACTION_TAG_TOGGLE_VIEW: return "tag_toggle_view";
        case AXIOM_ACTION_TAG_VIEW_ALL: return "tag_view_all";
        case AXIOM_ACTION_TAG_VIEW_PREVIOUS: return "tag_view_previous";
        case AXIOM_ACTION_WINDOW_TAG: return "window_tag";
        case AXIOM_ACTION_WINDOW_TAG_TOGGLE: return "window_tag_toggle";
        case AXIOM_ACTION_FOCUS_NEXT: return "focus_next";
        case AXIOM_ACTION_FOCUS_PREV: return "focus_prev";
        case AXIOM_ACTION_FOCUS_URGENT: return "focus_urgent";
        case AXIOM_ACTION_QUIT: return "quit";
        case AXIOM_ACTION_RELOAD_CONFIG: return "reload_config";
        case AXIOM_ACTION_MACRO: return "macro";
        default: return "unknown";
    }
}

const char *axiom_keybinding_modifiers_to_string(uint32_t modifiers) {
    static char buffer[64];
    buffer[0] = '\0';
    
    if (modifiers & AXIOM_MOD_SUPER) strcat(buffer, "Super+");
    if (modifiers & AXIOM_MOD_CTRL) strcat(buffer, "Ctrl+");
    if (modifiers & AXIOM_MOD_ALT) strcat(buffer, "Alt+");
    if (modifiers & AXIOM_MOD_SHIFT) strcat(buffer, "Shift+");
    
    // Remove trailing +
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '+') {
        buffer[len - 1] = '\0';
    }
    
    return buffer;
}

const char *axiom_keybinding_keysym_to_string(xkb_keysym_t keysym) {
    static char buffer[64];
    xkb_keysym_get_name(keysym, buffer, sizeof(buffer));
    return buffer;
}

struct axiom_keybinding *axiom_keybinding_find(struct axiom_keybinding_manager *manager,
                                              uint32_t modifiers, xkb_keysym_t keysym) {
    if (!manager) return NULL;
    
    for (int i = 0; i < manager->binding_count; i++) {
        if (manager->bindings[i].modifiers == modifiers && 
            manager->bindings[i].keysym == keysym) {
            return &manager->bindings[i];
        }
    }
    
    return NULL;
}

void axiom_keybinding_print_all(struct axiom_keybinding_manager *manager) {
    if (!manager) return;
    
    AXIOM_LOG_INFO("=== Key Bindings (%d total) ===", manager->binding_count);
    
    for (int i = 0; i < manager->binding_count; i++) {
        struct axiom_keybinding *binding = &manager->bindings[i];
        
        AXIOM_LOG_INFO("%s+%s -> %s %s", 
                       axiom_keybinding_modifiers_to_string(binding->modifiers),
                       axiom_keybinding_keysym_to_string(binding->keysym),
                       binding->description,
                       binding->enabled ? "" : "[DISABLED]");
    }
}
