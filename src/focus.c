#include "axiom.h"
#include "focus.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void axiom_focus_manager_init(struct axiom_focus_manager *manager) {
    if (!manager) return;
    
    wl_list_init(&manager->focus_stack);
    wl_list_init(&manager->urgent_windows);
    manager->focused_window = NULL;
    manager->last_focused_window = NULL;
    manager->is_cycling = false;
    manager->cycle_current = NULL;
    wl_list_init(&manager->cycle_list);
    manager->urgency_notifications_enabled = true;
    manager->urgency_timeout_ms = 5000; // Default 5 seconds
    manager->urgency_timer = NULL;
    manager->max_history_size = 10;
    manager->current_history_size = 0;
}

void axiom_focus_manager_cleanup(struct axiom_focus_manager *manager) {
    if (!manager) return;
    
    manager->focused_window = NULL;
    manager->last_focused_window = NULL;
    manager->is_cycling = false;
    manager->urgency_timer = NULL;
    manager->current_history_size = 0;
}

void axiom_focus_window(struct axiom_server *server, struct axiom_window *window) {
    if (!server || !window) return;
    
    struct axiom_focus_manager *manager = server->focus_manager;
    
    if (manager->focused_window == window) {
        return;
    }
    
    manager->last_focused_window = manager->focused_window;
    manager->focused_window = window;
    
    // Bring window to front in focus stack
    axiom_focus_stack_promote(manager, window);
    
    // Update visual focus indicators
    if (window->xdg_toplevel) {
        wlr_xdg_toplevel_set_activated(window->xdg_toplevel, true);
    }
    
    // Log focus change
    AXIOM_LOG_INFO("Focused window: %p", (void *)window);
}

void axiom_focus_stack_promote(struct axiom_focus_manager *manager, struct axiom_window *window) {
    if (!manager || !window) return;
    
    struct axiom_focus_entry *entry;
    wl_list_for_each(entry, &manager->focus_stack, link) {
        if (entry->window == window) {
            wl_list_remove(&entry->link);
            wl_list_insert(&manager->focus_stack, &entry->link);
            
            // Maintain focus history size
            if (manager->current_history_size < manager->max_history_size) {
                manager->current_history_size++;
            } else {
                struct axiom_focus_entry *last;
                last = wl_container_of(manager->focus_stack.prev, last, link);
                wl_list_remove(&last->link);
                free(last);
            }

            return;
        }
    }
    
    entry = calloc(1, sizeof(struct axiom_focus_entry));
    if (!entry) return;
    
    entry->window = window;
    entry->last_focus_time = time(NULL);
    wl_list_insert(&manager->focus_stack, &entry->link);
}

// Legacy function signature for compatibility with existing code
void axiom_focus_window_legacy(struct axiom_server *server, struct axiom_window *window, 
                               struct wlr_surface *surface) {
    if (!server) return;
    
    if (server->focused_window == window) {
        return;
    }
    
    if (server->focused_window) {
        server->focused_window->is_focused = false;
        if (server->focused_window->xdg_toplevel) {
            wlr_xdg_toplevel_set_activated(server->focused_window->xdg_toplevel, false);
        }
    }
    
    server->focused_window = window;
    
    if (window) {
        window->is_focused = true;
        if (window->xdg_toplevel) {
            wlr_xdg_toplevel_set_activated(window->xdg_toplevel, true);
        }
        
        wl_list_remove(&window->link);
        wl_list_insert(&server->windows, &window->link);
        
        if (surface && server->seat && server->seat->keyboard_state.keyboard) {
            wlr_seat_keyboard_notify_enter(server->seat, surface,
                server->seat->keyboard_state.keyboard->keycodes,
                server->seat->keyboard_state.keyboard->num_keycodes,
                &server->seat->keyboard_state.keyboard->modifiers);
        }
    } else {
        if (server->seat) {
            wlr_seat_keyboard_clear_focus(server->seat);
        }
    }
}

// Implement other focus and stacking management functions as needed...

// Urgency system functions
void axiom_window_set_urgent(struct axiom_server *server, struct axiom_window *window, bool urgent) {
    if (!server || !window) return;

    struct axiom_focus_manager *manager = server->focus_manager;

    if (urgent && !window->window_tags->is_urgent) {
        window->window_tags->is_urgent = true;
        axiom_focus_stack_promote(manager, window);
    } else if (!urgent && window->window_tags->is_urgent) {
        window->window_tags->is_urgent = false;
    }
}

void axiom_window_clear_urgency(struct axiom_server *server, struct axiom_window *window) {
    axiom_window_set_urgent(server, window, false);
}

bool axiom_window_is_urgent(struct axiom_window *window) {
    return window ? window->window_tags->is_urgent : false;
}

void axiom_focus_urgent_window(struct axiom_server *server) {
    struct axiom_focus_manager *manager = server->focus_manager;
    struct axiom_focus_entry *entry;

    wl_list_for_each(entry, &manager->focus_stack, link) {
        if (entry->window && entry->window->window_tags->is_urgent) {
            axiom_focus_window(server, entry->window);
            break;
        }
    }
}

int axiom_focus_count_urgent_windows(struct axiom_server *server) {
    if (!server) return 0;

    int count = 0;
    struct axiom_focus_entry *entry;
    wl_list_for_each(entry, &server->focus_manager->focus_stack, link) {
        if (entry->window->window_tags->is_urgent) {
            count++;
        }
    }
    return count;
} 
