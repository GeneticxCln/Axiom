#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>
#include "axiom.h"
#include "config.h"
#include "animation.h"
#include "pip_manager.h"

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
        axiom_focus_window(server, next_window, next_window->xdg_toplevel->base->surface);
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
            // Decrease master ratio
            axiom_adjust_master_ratio(-0.05f);
            axiom_arrange_windows(server);
            return true;
        case XKB_KEY_j:
            // Increase master ratio  
            axiom_adjust_master_ratio(0.05f);
            axiom_arrange_windows(server);
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
        axiom_focus_window(server, next_window, next_window->xdg_toplevel->base->surface);
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

void axiom_focus_window(struct axiom_server *server, struct axiom_window *window,
        struct wlr_surface *surface) {
    if (server->focused_window == window) {
        return;
    }
    
    if (server->focused_window) {
        server->focused_window->is_focused = false;
        wlr_xdg_toplevel_set_activated(server->focused_window->xdg_toplevel, false);
    }
    
    server->focused_window = window;
    
    if (window) {
        window->is_focused = true;
        wlr_xdg_toplevel_set_activated(window->xdg_toplevel, true);
        
        wl_list_remove(&window->link);
        wl_list_insert(&server->windows, &window->link);
        
        // Trigger focus ring animation
        if (server->animation_manager) {
            axiom_animate_focus_ring(server, window);
        }
        
        wlr_seat_keyboard_notify_enter(server->seat, surface,
            server->seat->keyboard_state.keyboard->keycodes,
            server->seat->keyboard_state.keyboard->num_keycodes,
            &server->seat->keyboard_state.keyboard->modifiers);
    } else {
        wlr_seat_keyboard_clear_focus(server->seat);
    }
}

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
