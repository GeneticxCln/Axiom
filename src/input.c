#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_touch.h>
#include <wlr/types/wlr_tablet_tool.h>
#include <wlr/types/wlr_tablet_pad.h>
#include <wlr/types/wlr_switch.h>
#include <wlr/types/wlr_text_input_v3.h>
#include <wlr/types/wlr_input_method_v2.h>
#include <wlr/types/wlr_pointer_gestures_v1.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>
#include <linux/input-event-codes.h>
#include <math.h>
#include "axiom.h"
#include "input.h"
#include "keybindings.h"
#include "config.h"
#include "animation.h"
#include "pip_manager.h"
#include "logging.h"
#include "advanced_tiling.h"

// Forward declare window manager functions
void axiom_window_manager_arrange_all(struct axiom_window_manager *manager);
void axiom_window_manager_add_window(struct axiom_window_manager *manager, struct axiom_window *window);
void axiom_window_manager_remove_window(struct axiom_window_manager *manager, struct axiom_window *window);

void axiom_cycle_layout(struct axiom_server *server) {
    if (!server || !server->window_manager) {
        AXIOM_LOG_ERROR("Failed to cycle layout, server or window_manager is NULL");
        return;
    }

    // Use the advanced tiling engine for layout cycling
    struct axiom_advanced_tiling_engine *tiling_engine = axiom_window_manager_get_tiling_engine();
    if (tiling_engine) {
        axiom_advanced_tiling_cycle_mode(tiling_engine);
        AXIOM_LOG_INFO("Advanced tiling mode cycled to: %s", 
                       axiom_tiling_mode_name(tiling_engine->current_mode));
        
        // Apply the new tiling layout
        axiom_window_manager_apply_tiling(server->window_manager);
    } else {
        // Fallback to old layout system if advanced tiling not available
        enum axiom_layout_type current = axiom_get_layout();
        enum axiom_layout_type next;
        switch (current) {
            case AXIOM_LAYOUT_MASTER_STACK:
                next = AXIOM_LAYOUT_GRID;
                break;
            case AXIOM_LAYOUT_GRID:
                next = AXIOM_LAYOUT_SPIRAL;
                break;
            case AXIOM_LAYOUT_SPIRAL:
                next = AXIOM_LAYOUT_FLOATING;
                break;
            default:
                next = AXIOM_LAYOUT_MASTER_STACK;
                break;
        }
        
        axiom_set_layout(next);
        AXIOM_LOG_INFO("Layout cycled to: %s", axiom_get_layout_name());
        axiom_window_manager_arrange_all(server->window_manager);
    }
}

void axiom_toggle_window_floating(struct axiom_server *server, struct axiom_window *window) {
    if (!server || !window) {
        AXIOM_LOG_ERROR("Cannot toggle floating state, server or window is NULL");
        return;
    }

    // Switch between floating and tiled layouts
    if (window->is_tiled) {
        window->is_tiled = false;
        axiom_window_manager_remove_window(server->window_manager, window);
        axiom_window_manager_add_window(server->window_manager, window);
        axiom_update_window_decorations(window); // Remove decorations for floating
        AXIOM_LOG_INFO("Window set to floating: %s", window->xdg_toplevel ? window->xdg_toplevel->title : "(no title)");
    } else {
        window->is_tiled = true;
        axiom_window_manager_remove_window(server->window_manager, window);
        axiom_window_manager_add_window(server->window_manager, window);
        axiom_update_window_decorations(window); // Add decorations for tiled
        AXIOM_LOG_INFO("Window set to tiled: %s", window->xdg_toplevel ? window->xdg_toplevel->title : "(no title)");
    }
    
    // Rearrange windows after toggling
    axiom_window_manager_arrange_all(server->window_manager);
}


// Enhanced input handling with gesture support

void axiom_input_handle_gesture(struct axiom_server *server, struct axiom_gesture_event *event) {
    if (!server) return;

    switch (event->type) {
        case AXIOM_GESTURE_PINCH_IN:
            AXIOM_LOG_INFO("Gesture: Pinch In detected at (%.1f, %.1f)", event->x, event->y);
            // Implement pinch in action (e.g., zoom out)
            break;
        case AXIOM_GESTURE_PINCH_OUT:
            AXIOM_LOG_INFO("Gesture: Pinch Out detected at (%.1f, %.1f)", event->x, event->y);
            // Implement pinch out action (e.g., zoom in)
            break;
        case AXIOM_GESTURE_ROTATE_CW:
            AXIOM_LOG_INFO("Gesture: Rotate Clockwise at (%.1f, %.1f)", event->x, event->y);
            // Implement rotate clockwise action
            break;
        case AXIOM_GESTURE_ROTATE_CCW:
            AXIOM_LOG_INFO("Gesture: Rotate Counter-Clockwise at (%.1f, %.1f)", event->x, event->y);
            // Implement rotate counter-clockwise action
            break;
        default:
            AXIOM_LOG_INFO("Unhandled gesture type: %d", event->type);
            break;
    }
}

// Input configuration dynamically loadable and savable
void axiom_input_load_dynamic_config(struct axiom_input_manager *manager, const char *path) {
    if (!manager || !path) return;

    // Simulate loading configuration from a file
    AXIOM_LOG_INFO("Loading input configuration from %s", path);

    // Example of setting new defaults
    manager->natural_scroll_default = true;
    manager->tap_to_click_default = false;
    manager->pointer_accel_default = 0.5;
}

void axiom_input_save_dynamic_config(struct axiom_input_manager *manager, const char *path) {
    if (!manager || !path) return;

    // Simulate saving configuration to a file
    AXIOM_LOG_INFO("Saving input configuration to %s", path);

    // Example of saving current settings
    // This would typically involve writing to a file
}
struct axiom_input_manager *axiom_input_manager_create(struct axiom_server *server) {
    struct axiom_input_manager *manager = calloc(1, sizeof(struct axiom_input_manager));
    if (!manager) {
        AXIOM_LOG_ERROR("Failed to allocate input manager");
        return NULL;
    }
    manager->server = server;
    wl_list_init(&manager->keyboards);
    wl_list_init(&manager->pointers);
    wl_list_init(&manager->touch_devices);
    wl_list_init(&manager->tablet_tools);
    wl_list_init(&manager->tablet_pads);
    wl_list_init(&manager->switches);
    manager->natural_scroll_default = false;
    manager->tap_to_click_default = true;
    manager->pointer_accel_default = 0.0;
    AXIOM_LOG_INFO("Enhanced input manager created");
    return manager;
}

void axiom_input_manager_destroy(struct axiom_input_manager *manager) {
    if (!manager) return;
    free(manager);
    AXIOM_LOG_INFO("Input manager destroyed");
}

static void keyboard_handle_modifiers(struct wl_listener *listener, void *data) {
    (void)data; // Suppress unused parameter warning
    struct axiom_input_device *kbd = wl_container_of(listener, kbd, keyboard.modifiers);
    struct wlr_keyboard *keyboard = wlr_keyboard_from_input_device(kbd->wlr_device);
    wlr_seat_set_keyboard(kbd->server->seat, keyboard);
    wlr_seat_keyboard_notify_modifiers(kbd->server->seat,
        &keyboard->modifiers);
}

static void cycle_windows(struct axiom_server *server) {
    if (wl_list_empty(&server->windows)) {
        return;
    }
    
    // Find next window in the list
    struct axiom_window *next_window = NULL;
    if (server->focused_window) {
        // Get the next window after the focused one
        struct wl_list *next_link = server->focused_window->link.next;
        if (next_link == &server->windows) {
            // Wrap around to the first window
            next_link = server->windows.next;
        }
        next_window = wl_container_of(next_link, next_window, link);
    } else {
        // No focused window, focus the first one
        next_window = wl_container_of(server->windows.next, next_window, link);
    }
    
    if (next_window) {
        axiom_focus_window_legacy(server, next_window, next_window->xdg_toplevel->base->surface);
        AXIOM_LOG_INFO("Switched to window: %s", next_window->xdg_toplevel->title ?: "(no title)");
    }
}

static bool handle_keybinding(struct axiom_server *server, xkb_keysym_t sym, uint32_t modifiers) {
    // Handle Super + Shift key bindings
    if ((modifiers & WLR_MODIFIER_LOGO) && (modifiers & WLR_MODIFIER_SHIFT)) {
        switch (sym) {
        case XKB_KEY_1:
        case XKB_KEY_2:
        case XKB_KEY_3:
        case XKB_KEY_4:
        case XKB_KEY_5:
        case XKB_KEY_6:
        case XKB_KEY_7:
        case XKB_KEY_8:
        case XKB_KEY_9:
            {
                int workspace_num = sym - XKB_KEY_1 + 1;
                axiom_move_focused_window_to_workspace(server, workspace_num);
                return true;
            }
        case XKB_KEY_h:
            // Move window left in tiling layout (Super+Shift+H)
            if (server->focused_window && server->window_manager) {
                // TODO: Implement window movement in tiling layout
                AXIOM_LOG_INFO("Moving window left: %s", 
                               server->focused_window->xdg_toplevel ? server->focused_window->xdg_toplevel->title : "(no title)");
                return true;
            }
            break;
        case XKB_KEY_j:
            // Move window down in tiling layout (Super+Shift+J)
            if (server->focused_window && server->window_manager) {
                // TODO: Implement window movement in tiling layout
                AXIOM_LOG_INFO("Moving window down: %s", 
                               server->focused_window->xdg_toplevel ? server->focused_window->xdg_toplevel->title : "(no title)");
                return true;
            }
            break;
        case XKB_KEY_k:
            // Move window up in tiling layout (Super+Shift+K)
            if (server->focused_window && server->window_manager) {
                // TODO: Implement window movement in tiling layout
                AXIOM_LOG_INFO("Moving window up: %s", 
                               server->focused_window->xdg_toplevel ? server->focused_window->xdg_toplevel->title : "(no title)");
                return true;
            }
            break;
        case XKB_KEY_l:
            // Move window right in tiling layout (Super+Shift+L)
            if (server->focused_window && server->window_manager) {
                // TODO: Implement window movement in tiling layout
                AXIOM_LOG_INFO("Moving window right: %s", 
                               server->focused_window->xdg_toplevel ? server->focused_window->xdg_toplevel->title : "(no title)");
                return true;
            }
            break;
        case XKB_KEY_p:
            // Cycle PiP corner position (Super+Shift+P)
            if (server->focused_window && server->pip_manager &&
                axiom_pip_is_window_pip(server->pip_manager, server->focused_window)) {
                axiom_pip_cycle_corners(server->pip_manager, server->focused_window);
                AXIOM_LOG_INFO("Cycled PiP corner for window: %s", 
                               server->focused_window->xdg_toplevel->title ?: "(no title)");
                return true;
            }
            break;
        }
    }
    
    // Handle Super key bindings
    if (modifiers & WLR_MODIFIER_LOGO) {
        switch (sym) {
        case XKB_KEY_Escape:
            if (server->cursor_mode != AXIOM_CURSOR_PASSTHROUGH) {
                server->cursor_mode = AXIOM_CURSOR_PASSTHROUGH;
                wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
                return true;
            }
            break;
        case XKB_KEY_q:
            wl_display_terminate(server->wl_display);
            return true;
        case XKB_KEY_t:
            server->tiling_enabled = !server->tiling_enabled;
            AXIOM_LOG_INFO("Tiling %s", server->tiling_enabled ? "enabled" : "disabled");
            if (server->tiling_enabled) {
                axiom_arrange_windows(server);
            }
            return true;
        case XKB_KEY_Return:
            axiom_spawn_terminal();
            return true;
        case XKB_KEY_d:
            axiom_spawn_rofi();
            return true;
        case XKB_KEY_b:
            if (axiom_process_exists("waybar")) {
                axiom_kill_waybar();
            } else {
                axiom_spawn_waybar(server);
            }
            return true;
        case XKB_KEY_w:
            // Close focused window
            if (server->focused_window && server->focused_window->xdg_toplevel) {
                wlr_xdg_toplevel_send_close(server->focused_window->xdg_toplevel);
                return true;
            }
            break;
        case XKB_KEY_f:
            // Toggle fullscreen
            if (server->focused_window) {
                bool is_fullscreen = server->focused_window->is_fullscreen;
                wlr_xdg_toplevel_set_fullscreen(server->focused_window->xdg_toplevel, !is_fullscreen);
                server->focused_window->is_fullscreen = !is_fullscreen;
                return true;
            }
            break;
        case XKB_KEY_l:
            // Cycle through tiling layouts
            axiom_cycle_layout(server);
            return true;
        case XKB_KEY_space:
            // Toggle focused window floating/tiled
            axiom_toggle_window_floating(server, server->focused_window);
            return true;
        case XKB_KEY_h:
            // Decrease master ratio using advanced tiling engine
            {
                struct axiom_advanced_tiling_engine *tiling_engine = axiom_window_manager_get_tiling_engine();
                if (tiling_engine) {
                    axiom_advanced_tiling_adjust_master_ratio(tiling_engine, -0.05f);
                    axiom_window_manager_apply_tiling(server->window_manager);
                } else {
                    axiom_adjust_master_ratio(-0.05f);
                    axiom_arrange_windows(server);
                }
            }
            return true;
        case XKB_KEY_j:
            // Increase master ratio using advanced tiling engine
            {
                struct axiom_advanced_tiling_engine *tiling_engine = axiom_window_manager_get_tiling_engine();
                if (tiling_engine) {
                    axiom_advanced_tiling_adjust_master_ratio(tiling_engine, 0.05f);
                    axiom_window_manager_apply_tiling(server->window_manager);
                } else {
                    axiom_adjust_master_ratio(0.05f);
                    axiom_arrange_windows(server);
                }
            }
            return true;
        case XKB_KEY_i:
            // Increase master count using advanced tiling engine
            {
                struct axiom_advanced_tiling_engine *tiling_engine = axiom_window_manager_get_tiling_engine();
                if (tiling_engine) {
                    axiom_advanced_tiling_adjust_master_count(tiling_engine, 1);
                    axiom_window_manager_apply_tiling(server->window_manager);
                    AXIOM_LOG_INFO("Increased master count");
                }
            }
            return true;
        case XKB_KEY_u:
            // Decrease master count using advanced tiling engine
            {
                struct axiom_advanced_tiling_engine *tiling_engine = axiom_window_manager_get_tiling_engine();
                if (tiling_engine) {
                    axiom_advanced_tiling_adjust_master_count(tiling_engine, -1);
                    axiom_window_manager_apply_tiling(server->window_manager);
                    AXIOM_LOG_INFO("Decreased master count");
                }
            }
            return true;
        case XKB_KEY_r:
            // Reload configuration (Super+R)
            axiom_reload_configuration(server);
            return true;
        case XKB_KEY_p:
            // Toggle Picture-in-Picture mode (Super+P)
            if (server->focused_window && server->pip_manager) {
                if (axiom_pip_is_window_pip(server->pip_manager, server->focused_window)) {
                    axiom_pip_disable_for_window(server->pip_manager, server->focused_window);
                    AXIOM_LOG_INFO("Disabled PiP for window: %s", 
                                   server->focused_window->xdg_toplevel->title ?: "(no title)");
                } else {
                    axiom_pip_enable_for_window(server->pip_manager, server->focused_window);
                    AXIOM_LOG_INFO("Enabled PiP for window: %s", 
                                   server->focused_window->xdg_toplevel->title ?: "(no title)");
                }
                return true;
            }
            break;
        case XKB_KEY_m:
            // Super+M to maximize/restore window (Hyprland-style)
            if (server->focused_window) {
                bool is_maximized = server->focused_window->is_maximized;
                wlr_xdg_toplevel_set_maximized(server->focused_window->xdg_toplevel, !is_maximized);
                server->focused_window->is_maximized = !is_maximized;
                AXIOM_LOG_INFO("%s window: %s", 
                               is_maximized ? "Restored" : "Maximized",
                               server->focused_window->xdg_toplevel->title ?: "(no title)");
                return true;
            }
            break;
        case XKB_KEY_k:
            // Super+K to kill focused window (Hyprland-style)
            if (server->focused_window && server->focused_window->xdg_toplevel) {
                wlr_xdg_toplevel_send_close(server->focused_window->xdg_toplevel);
                AXIOM_LOG_INFO("Killed window: %s", 
                               server->focused_window->xdg_toplevel->title ?: "(no title)");
                return true;
            }
            break;
        // Phase 2: Workspace number key switching (Super + 1-9)
        case XKB_KEY_1:
        case XKB_KEY_2:
        case XKB_KEY_3:
        case XKB_KEY_4:
        case XKB_KEY_5:
        case XKB_KEY_6:
        case XKB_KEY_7:
        case XKB_KEY_8:
        case XKB_KEY_9:
            {
                int workspace_num = sym - XKB_KEY_1 + 1;
                axiom_switch_to_workspace_by_number(server, workspace_num);
                return true;
            }
        }
    }
    
    // Handle Alt key bindings
    if (modifiers & WLR_MODIFIER_ALT) {
        switch (sym) {
        case XKB_KEY_Tab:
            cycle_windows(server);
            return true;
        case XKB_KEY_F4:
            // Alt+F4 to close window
            if (server->focused_window && server->focused_window->xdg_toplevel) {
                wlr_xdg_toplevel_send_close(server->focused_window->xdg_toplevel);
                return true;
            }
            break;
        }
    }
    
    // Legacy keybindings (remove these eventually)
    switch (sym) {
    case XKB_KEY_F1: {
        if (wl_list_length(&server->windows) < 2) {
            break;
        }
        struct wl_list *next_link = server->windows.next;
        struct axiom_window *next_window;
        next_window = wl_container_of(next_link, next_window, link);
        axiom_focus_window_legacy(server, next_window, next_window->xdg_toplevel->base->surface);
        return true;
    }
    case XKB_KEY_q:
        wl_display_terminate(server->wl_display);
        return true;
    case XKB_KEY_t:
        server->tiling_enabled = !server->tiling_enabled;
        AXIOM_LOG_INFO("Tiling %s", server->tiling_enabled ? "enabled" : "disabled");
        if (server->tiling_enabled) {
            axiom_arrange_windows(server);
        }
        return true;
    case XKB_KEY_Return:
        axiom_spawn_terminal();
        return true;
    case XKB_KEY_d:
        axiom_spawn_rofi();
        return true;
    case XKB_KEY_b:
        if (axiom_process_exists("waybar")) {
            axiom_kill_waybar();
        } else {
            axiom_spawn_waybar(server);
        }
        return true;
    case XKB_KEY_w:
        // Close focused window
        if (server->focused_window && server->focused_window->xdg_toplevel) {
            wlr_xdg_toplevel_send_close(server->focused_window->xdg_toplevel);
            return true;
        }
        break;
    }
    return false;
}

static void keyboard_handle_key(struct wl_listener *listener, void *data) {
    struct axiom_input_device *kbd = wl_container_of(listener, kbd, keyboard.key);
    struct axiom_server *server = kbd->server;
    struct wlr_keyboard_key_event *event = data;
    struct wlr_seat *seat = server->seat;
    struct wlr_keyboard *keyboard = wlr_keyboard_from_input_device(kbd->wlr_device);
    
    uint32_t keycode = event->keycode + 8;
    const xkb_keysym_t *syms;
    int nsyms = xkb_state_key_get_syms(keyboard->xkb_state, keycode, &syms);
    
    bool handled = false;
    uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard);
    
    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        for (int i = 0; i < nsyms; i++) {
            // Try new keybinding system first
            if (server->keybinding_manager) {
                handled = axiom_keybinding_handle_key(server, modifiers, syms[i]);
                if (handled) break;
            }
            
            // Fall back to legacy keybinding handler
            handled = handle_keybinding(server, syms[i], modifiers);
            if (handled) break;
        }
    }
    
    if (!handled) {
        wlr_seat_set_keyboard(seat, keyboard);
        wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
    }
}

static void keyboard_handle_destroy(struct wl_listener *listener, void *data) {
    (void)data; // Suppress unused parameter warning
    struct axiom_input_device *kbd = wl_container_of(listener, kbd, destroy);
    wl_list_remove(&kbd->keyboard.modifiers.link);
    wl_list_remove(&kbd->keyboard.key.link);
    wl_list_remove(&kbd->link);
    free(kbd);
}

void axiom_new_keyboard(struct axiom_server *server, struct wlr_input_device *device) {
    struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(device);
    
    struct axiom_input_device *keyboard = calloc(1, sizeof(struct axiom_input_device));
    if (!keyboard) {
        AXIOM_LOG_ERROR("Failed to allocate keyboard device");
        return;
    }
    
    keyboard->server = server;
    keyboard->wlr_device = device;
    
    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!context) {
        AXIOM_LOG_ERROR("Failed to create XKB context");
        free(keyboard);
        return;
    }
    
    struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap) {
        AXIOM_LOG_ERROR("Failed to create XKB keymap");
        xkb_context_unref(context);
        free(keyboard);
        return;
    }
    
    wlr_keyboard_set_keymap(wlr_keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
    
    wlr_keyboard_set_repeat_info(wlr_keyboard, server->config->repeat_rate, server->config->repeat_delay);
    
    keyboard->keyboard.modifiers.notify = keyboard_handle_modifiers;
    wl_signal_add(&wlr_keyboard->events.modifiers, &keyboard->keyboard.modifiers);
    keyboard->keyboard.key.notify = keyboard_handle_key;
    wl_signal_add(&wlr_keyboard->events.key, &keyboard->keyboard.key);
    keyboard->destroy.notify = keyboard_handle_destroy;
    wl_signal_add(&device->events.destroy, &keyboard->destroy);
    
    wlr_seat_set_keyboard(server->seat, wlr_keyboard);
    wl_list_insert(&server->input_devices, &keyboard->link);
    
    AXIOM_LOG_INFO("New keyboard: %s", device->name);
}

static void pointer_handle_destroy(struct wl_listener *listener, void *data) {
    (void)data; // Suppress unused parameter warning
    struct axiom_input_device *pointer = wl_container_of(listener, pointer, destroy);
    wl_list_remove(&pointer->link);
    free(pointer);
}

void axiom_new_pointer(struct axiom_server *server, struct wlr_input_device *device) {
    struct axiom_input_device *pointer = calloc(1, sizeof(struct axiom_input_device));
    if (!pointer) {
        AXIOM_LOG_ERROR("Failed to allocate pointer device");
        return;
    }
    
    pointer->server = server;
    pointer->wlr_device = device;
    
    pointer->destroy.notify = pointer_handle_destroy;
    wl_signal_add(&device->events.destroy, &pointer->destroy);
    
    wlr_cursor_attach_input_device(server->cursor, device);
    wl_list_insert(&server->input_devices, &pointer->link);
    
    AXIOM_LOG_INFO("New pointer: %s", device->name);
}

void axiom_new_input(struct wl_listener *listener, void *data) {
    struct axiom_server *server = wl_container_of(listener, server, new_input);
    struct wlr_input_device *device = data;
    
    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
        axiom_new_keyboard(server, device);
        break;
    case WLR_INPUT_DEVICE_POINTER:
        axiom_new_pointer(server, device);
        break;
    default:
        AXIOM_LOG_INFO("Unsupported input device type: %d", device->type);
        break;
    }
    
    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    if (!wl_list_empty(&server->input_devices)) {
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    wlr_seat_set_capabilities(server->seat, caps);
}

struct axiom_window *axiom_window_at(struct axiom_server *server, double lx, double ly,
        struct wlr_surface **surface, double *sx, double *sy) {
    struct wlr_scene_node *node = wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);
    if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER) {
        return NULL;
    }
    
    struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
    struct wlr_scene_surface *scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);
    if (!scene_surface) {
        return NULL;
    }
    
    *surface = scene_surface->surface;
    struct wlr_scene_tree *tree = node->parent;
    while (tree != NULL && tree->node.data == NULL) {
        tree = tree->node.parent;
    }
    
    return tree ? tree->node.data : NULL;
}

// axiom_focus_window is now implemented in focus.c

void axiom_begin_interactive(struct axiom_window *window, enum axiom_cursor_mode mode, uint32_t edges) {
    struct axiom_server *server = window->server;
    struct wlr_surface *focused_surface = server->seat->pointer_state.focused_surface;
    
    if (window->xdg_toplevel->base->surface != wlr_surface_get_root_surface(focused_surface)) {
        return;
    }
    
    server->grabbed_window = window;
    server->cursor_mode = mode;
    
    if (mode == AXIOM_CURSOR_MOVE) {
        server->grab_x = server->cursor->x - window->x;
        server->grab_y = server->cursor->y - window->y;
        wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "grabbing");
    } else {
        struct wlr_box geo_box;
        wlr_surface_get_extents(window->xdg_toplevel->base->surface, &geo_box);
        
        double border_x = (window->x + geo_box.x) + ((edges & WLR_EDGE_RIGHT) ? geo_box.width : 0);
        double border_y = (window->y + geo_box.y) + ((edges & WLR_EDGE_BOTTOM) ? geo_box.height : 0);
        server->grab_x = server->cursor->x - border_x;
        server->grab_y = server->cursor->y - border_y;
        
        server->grab_geobox = geo_box;
        server->grab_geobox.x += window->x;
        server->grab_geobox.y += window->y;
        
        server->resize_edges = edges;
        
        const char *cursor_name;
        if (edges == (WLR_EDGE_TOP | WLR_EDGE_LEFT)) {
            cursor_name = "nw-resize";
        } else if (edges == (WLR_EDGE_TOP | WLR_EDGE_RIGHT)) {
            cursor_name = "ne-resize";
        } else if (edges == (WLR_EDGE_BOTTOM | WLR_EDGE_LEFT)) {
            cursor_name = "sw-resize";
        } else if (edges == (WLR_EDGE_BOTTOM | WLR_EDGE_RIGHT)) {
            cursor_name = "se-resize";
        } else if (edges == WLR_EDGE_TOP) {
            cursor_name = "n-resize";
        } else if (edges == WLR_EDGE_BOTTOM) {
            cursor_name = "s-resize";
        } else if (edges == WLR_EDGE_LEFT) {
            cursor_name = "w-resize";
        } else if (edges == WLR_EDGE_RIGHT) {
            cursor_name = "e-resize";
        } else {
            cursor_name = "default";
        }
        wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, cursor_name);
    }
}

// Cleanup input devices
void axiom_remove_input_devices(struct axiom_server *server) {
    struct axiom_input_device *device, *tmp;
    wl_list_for_each_safe(device, tmp, &server->input_devices, link) {
        if (device->wlr_device) {
            // Remove signal listeners before destroying
            if (device->wlr_device->type == WLR_INPUT_DEVICE_KEYBOARD) {
                wl_list_remove(&device->keyboard.modifiers.link);
                wl_list_remove(&device->keyboard.key.link);
            }
            wl_list_remove(&device->destroy.link);
        }
        wl_list_remove(&device->link);
        free(device);
    }
}
