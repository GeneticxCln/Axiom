#include <stdbool.h>
#include <stddef.h>
#include "axiom.h"
#include "focus.h"
#include "window_snapping.h"
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/util/log.h>
#include <linux/input-event-codes.h>
#include "logging.h"



static void process_motion(struct axiom_server *server, uint32_t time) {
    if (server->cursor_mode != AXIOM_CURSOR_PASSTHROUGH) {
        axiom_process_cursor_motion(server, time);
        return;
    }
    
    struct wlr_surface *surface = NULL;
    double sx, sy;
    struct axiom_window *window = axiom_window_at(server,
        server->cursor->x, server->cursor->y,
        &surface, &sx, &sy);
    
    // Set cursor on first motion - load theme if needed
    static bool cursor_theme_loaded = false;
    static bool cursor_set = false;
    
    if (!cursor_theme_loaded && server->cursor_mgr) {
        // Try to load cursor theme now that we're in motion and output should be ready
        struct axiom_output *output;
        wl_list_for_each(output, &server->outputs, link) {
            if (wlr_xcursor_manager_load(server->cursor_mgr, output->wlr_output->scale)) {
                axiom_log_debug("Cursor theme loaded successfully during motion");
                cursor_theme_loaded = true;
                break;
            }
        }
        if (!cursor_theme_loaded) {
            axiom_log_debug("Failed to load cursor theme during motion, trying default scale");
            if (wlr_xcursor_manager_load(server->cursor_mgr, 1.0)) {
                cursor_theme_loaded = true;
                axiom_log_debug("Cursor theme loaded with default scale");
            }
        }
    }
    
    if (!cursor_set && cursor_theme_loaded && server->cursor_mgr) {
        struct wlr_xcursor *xcursor = wlr_xcursor_manager_get_xcursor(server->cursor_mgr, "default", 1.0);
        if (xcursor && xcursor->image_count > 0) {
            // This should be safe now that output is fully initialized and theme is loaded
            wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
            cursor_set = true;
            axiom_log_debug("Cursor set successfully on first motion");
        }
    } else if (!cursor_set && !server->cursor_mgr) {
        axiom_log_debug("Skipping cursor setting (cursor_mgr is NULL)");
        cursor_set = true; // Mark as set to avoid repeated messages
    }
    
    if (!window && cursor_set) {
        // Only set cursor if we have cursor manager and it's already been set once
        if (server->cursor_mgr) {
            struct wlr_xcursor *cursor = wlr_xcursor_manager_get_xcursor(server->cursor_mgr, "default", 1.0);
            if (cursor && cursor->image_count > 0) {
                wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
            }
        }
    }
    
    // Handle title bar button hover states
    if (window) {
        double relative_x = server->cursor->x - window->x;
        double relative_y = server->cursor->y - (window->y - 30); // Account for title bar above window
        
        // Check if cursor is over any title bar button and update hover states
        axiom_update_button_hover_states(window, relative_x, relative_y);
    }
    
    if (surface) {
        wlr_seat_pointer_notify_enter(server->seat, surface, sx, sy);
        wlr_seat_pointer_notify_motion(server->seat, time, sx, sy);
    } else {
        wlr_seat_pointer_clear_focus(server->seat);
    }
}

void axiom_cursor_motion(struct wl_listener *listener, void *data) {
    struct axiom_server *server = wl_container_of(listener, server, cursor_motion);
    struct wlr_pointer_motion_event *event = data;
    wlr_cursor_move(server->cursor, &event->pointer->base,
        event->delta_x, event->delta_y);
    process_motion(server, event->time_msec);
}

void axiom_cursor_motion_absolute(struct wl_listener *listener, void *data) {
    struct axiom_server *server = wl_container_of(listener, server, cursor_motion_absolute);
    struct wlr_pointer_motion_absolute_event *event = data;
    wlr_cursor_warp_absolute(server->cursor, &event->pointer->base,
        event->x, event->y);
    process_motion(server, event->time_msec);
}

void axiom_cursor_button(struct wl_listener *listener, void *data) {
    struct axiom_server *server = wl_container_of(listener, server, cursor_button);
    struct wlr_pointer_button_event *event = data;
    wlr_seat_pointer_notify_button(server->seat, event->time_msec,
        event->button, event->state);

    if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
        server->cursor_mode = AXIOM_CURSOR_PASSTHROUGH;
        server->grabbed_window = NULL;
        return;
    }
    
    // Handle left mouse button press
    if (event->button == BTN_LEFT && event->state == WL_POINTER_BUTTON_STATE_PRESSED) {
        struct wlr_surface *surface = NULL;
        double sx, sy;
        struct axiom_window *window = axiom_window_at(server,
            server->cursor->x, server->cursor->y, &surface, &sx, &sy);
        
        if (window) {
            // Check if click is on title bar buttons first
            // Get relative coordinates within the window decorations
            double relative_x = server->cursor->x - window->x;
            double relative_y = server->cursor->y - (window->y - 30); // Account for title bar above window
            
            // Try to handle title bar button click
            if (axiom_handle_title_bar_click(window, relative_x, relative_y)) {
                // Title bar button was clicked, don't proceed with focus or other actions
                return;
            }
            
            // Modern focus system with proper window focusing
            if (server->focus_manager) {
                axiom_focus_window(server, window);
            } else {
                axiom_focus_window_legacy(server, window, surface);
            }
            
            AXIOM_LOG_DEBUG("FOCUS", "Click-to-focus: Focused window %s", 
                           window->xdg_toplevel->title ?: "(no title)");
            
            // Check if modifiers are held down for move/resize
            if (server->seat->keyboard_state.keyboard) {
                uint32_t modifiers = wlr_keyboard_get_modifiers(server->seat->keyboard_state.keyboard);
                if (modifiers & WLR_MODIFIER_LOGO) {
                    // Logo + Left Click = Move window
                    axiom_begin_interactive(window, AXIOM_CURSOR_MOVE, 0);
                    AXIOM_LOG_DEBUG("INTERACTION", "Started interactive move for window");
                }
            }
        } else {
            // Click on empty space - unfocus current window
            if (server->focus_manager) {
                // Clear focus using modern focus system
                if (server->focused_window) {
                    server->focused_window->is_focused = false;
                    if (server->focused_window->xdg_toplevel) {
                        wlr_xdg_toplevel_set_activated(server->focused_window->xdg_toplevel, false);
                    }
                }
                server->focused_window = NULL;
                if (server->seat) {
                    wlr_seat_keyboard_clear_focus(server->seat);
                }
                AXIOM_LOG_DEBUG("FOCUS", "Unfocused window due to empty space click");
            } else {
                axiom_focus_window_legacy(server, NULL, NULL);
            }
        }
    }
}

void axiom_cursor_axis(struct wl_listener *listener, void *data) {
    struct axiom_server *server = wl_container_of(listener, server, cursor_axis);
    struct wlr_pointer_axis_event *event = data;
    wlr_seat_pointer_notify_axis(server->seat, event->time_msec,
        event->orientation, event->delta,
        event->delta_discrete, event->source, 0);
}

void axiom_cursor_frame(struct wl_listener *listener, void *data) {
    (void)data; // Suppress unused parameter warning
    struct axiom_server *server = wl_container_of(listener, server, cursor_frame);
    wlr_seat_pointer_notify_frame(server->seat);
}

void axiom_process_cursor_motion(struct axiom_server *server, uint32_t time) {
    if (server->cursor_mode == AXIOM_CURSOR_MOVE) {
        axiom_process_cursor_move(server, time);
    } else if (server->cursor_mode == AXIOM_CURSOR_RESIZE) {
        axiom_process_cursor_resize(server, time);
    }
}

void axiom_process_cursor_move(struct axiom_server *server, uint32_t time) {
    (void)time; // Suppress unused parameter warning
    if (!server->grabbed_window) {
        return;
    }
    
    struct axiom_window *window = server->grabbed_window;
    if (window->is_tiled) {
        // Don't allow moving tiled windows
        return;
    }
    
    int new_x = server->cursor->x - server->grab_x;
    int new_y = server->cursor->y - server->grab_y;
    
    // Apply window snapping during move
    axiom_window_snapping_handle_move(server, window, new_x, new_y);
    
    // The snapping handler updates window->x and window->y with snapped positions
    wlr_scene_node_set_position(&window->scene_tree->node, window->x, window->y);
    AXIOM_LOG_DEBUG("Moving window to %d, %d (with snapping)", window->x, window->y);
}

void axiom_process_cursor_resize(struct axiom_server *server, uint32_t time) {
    (void)time; // Suppress unused parameter warning
    if (!server->grabbed_window) {
        return;
    }
    
    struct axiom_window *window = server->grabbed_window;
    if (window->is_tiled) {
        // Don't allow resizing tiled windows
        return;
    }
    
    double border_x = server->cursor->x - server->grab_x;
    double border_y = server->cursor->y - server->grab_y;
    int new_left = server->grab_geobox.x;
    int new_right = server->grab_geobox.x + server->grab_geobox.width;
    int new_top = server->grab_geobox.y;
    int new_bottom = server->grab_geobox.y + server->grab_geobox.height;
    
    if (server->resize_edges & WLR_EDGE_TOP) {
        new_top = border_y;
        if (new_top >= new_bottom) {
            new_top = new_bottom - 1;
        }
    } else if (server->resize_edges & WLR_EDGE_BOTTOM) {
        new_bottom = border_y;
        if (new_bottom <= new_top) {
            new_bottom = new_top + 1;
        }
    }
    if (server->resize_edges & WLR_EDGE_LEFT) {
        new_left = border_x;
        if (new_left >= new_right) {
            new_left = new_right - 1;
        }
    } else if (server->resize_edges & WLR_EDGE_RIGHT) {
        new_right = border_x;
        if (new_right <= new_left) {
            new_right = new_left + 1;
        }
    }
    
    struct wlr_box geo_box;
    wlr_surface_get_extents(window->xdg_toplevel->base->surface, &geo_box);
    window->x = new_left - geo_box.x;
    window->y = new_top - geo_box.y;
    window->width = new_right - new_left;
    window->height = new_bottom - new_top;
    
    // Apply window snapping constraints during resize
    axiom_window_snapping_handle_resize(server, window, window->width, window->height);
    
    wlr_scene_node_set_position(&window->scene_tree->node, window->x, window->y);
    wlr_xdg_toplevel_set_size(window->xdg_toplevel, window->width, window->height);
    
    AXIOM_LOG_DEBUG("Resizing window to %dx%d at %d,%d (with snapping constraints)", 
                    window->width, window->height, window->x, window->y);
}
