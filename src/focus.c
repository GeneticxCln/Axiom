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

// Focus cycling implementation (Alt+Tab style)
void axiom_focus_cycle_start(struct axiom_server *server, bool reverse) {
    if (!server || !server->focus_manager) return;
    
    struct axiom_focus_manager *manager = server->focus_manager;
    
    // Already cycling, just change direction
    if (manager->is_cycling) {
        if (reverse) {
            axiom_focus_cycle_prev(server);
        } else {
            axiom_focus_cycle_next(server);
        }
        return;
    }
    
    // Start new cycling session
    manager->is_cycling = true;
    
    // Build cycling list from current windows
    wl_list_init(&manager->cycle_list);
    
    struct axiom_window *window;
    wl_list_for_each(window, &server->windows, link) {
        if (window->scene_tree && window->scene_tree->node.enabled) {
            struct axiom_focus_entry *entry = calloc(1, sizeof(struct axiom_focus_entry));
            if (entry) {
                entry->window = window;
                entry->last_focus_time = time(NULL);
                wl_list_insert(&manager->cycle_list, &entry->link);
            }
        }
    }
    
    // Start with current focused window or first in list
    if (manager->focused_window) {
        struct axiom_focus_entry *entry;
        wl_list_for_each(entry, &manager->cycle_list, link) {
            if (entry->window == manager->focused_window) {
                manager->cycle_current = entry;
                break;
            }
        }
    }
    
    if (!manager->cycle_current && !wl_list_empty(&manager->cycle_list)) {
        manager->cycle_current = wl_container_of(manager->cycle_list.next, 
                                                manager->cycle_current, link);
    }
    
    // Move to next/prev window
    if (reverse) {
        axiom_focus_cycle_prev(server);
    } else {
        axiom_focus_cycle_next(server);
    }
    
    AXIOM_LOG_INFO("Started focus cycling with %d windows", 
                   wl_list_length(&manager->cycle_list));
}

void axiom_focus_cycle_next(struct axiom_server *server) {
    if (!server || !server->focus_manager || !server->focus_manager->is_cycling) return;
    
    struct axiom_focus_manager *manager = server->focus_manager;
    
    if (wl_list_empty(&manager->cycle_list)) {
        return;
    }
    
    // Move to next window
    if (manager->cycle_current) {
        struct wl_list *next = manager->cycle_current->link.next;
        if (next == &manager->cycle_list) {
            next = manager->cycle_list.next;
        }
        manager->cycle_current = wl_container_of(next, manager->cycle_current, link);
    } else {
        manager->cycle_current = wl_container_of(manager->cycle_list.next, 
                                               manager->cycle_current, link);
    }
    
    // Focus the current window
    if (manager->cycle_current && manager->cycle_current->window) {
        axiom_focus_window(server, manager->cycle_current->window);
        AXIOM_LOG_DEBUG("FOCUS", "Cycled to next window: %p", 
                       (void *)manager->cycle_current->window);
    }
    
    // Move to next window in cycle list
    if (manager->cycle_current) {
        struct wl_list *next_link = manager->cycle_current->link.next;
        if (next_link == &manager->cycle_list) {
            // Wrap around to first
            next_link = manager->cycle_list.next;
        }
        manager->cycle_current = wl_container_of(next_link, manager->cycle_current, link);
    } else if (!wl_list_empty(&manager->cycle_list)) {
        manager->cycle_current = wl_container_of(manager->cycle_list.next, 
                                                manager->cycle_current, link);
    }
    
    if (manager->cycle_current && manager->cycle_current->window) {
        // Temporarily focus the window (visual only)
        struct axiom_window *window = manager->cycle_current->window;
        
        // Update visual indicators without changing full focus state
        if (window->xdg_toplevel) {
            wlr_xdg_toplevel_set_activated(window->xdg_toplevel, true);
        }
        
        // Bring window to front visually
        if (window->scene_tree) {
            wlr_scene_node_raise_to_top(&window->scene_tree->node);
        }
        
        AXIOM_LOG_DEBUG("Cycling to next window: %s", 
                        window->xdg_toplevel ? (window->xdg_toplevel->title ?: "(no title)") : "(no toplevel)");
    }
}

void axiom_focus_cycle_prev(struct axiom_server *server) {
    if (!server || !server->focus_manager || !server->focus_manager->is_cycling) return;
    
    struct axiom_focus_manager *manager = server->focus_manager;
    
    if (wl_list_empty(&manager->cycle_list)) {
        return;
    }
    
    // Move to previous window in cycle list
    if (manager->cycle_current) {
        struct wl_list *prev_link = manager->cycle_current->link.prev;
        if (prev_link == &manager->cycle_list) {
            // Wrap around to last
            prev_link = manager->cycle_list.prev;
        }
        manager->cycle_current = wl_container_of(prev_link, manager->cycle_current, link);
    } else if (!wl_list_empty(&manager->cycle_list)) {
        manager->cycle_current = wl_container_of(manager->cycle_list.prev, 
                                                manager->cycle_current, link);
    }
    
    if (manager->cycle_current && manager->cycle_current->window) {
        // Temporarily focus the window (visual only)
        struct axiom_window *window = manager->cycle_current->window;
        
        // Update visual indicators without changing full focus state
        if (window->xdg_toplevel) {
            wlr_xdg_toplevel_set_activated(window->xdg_toplevel, true);
        }
        
        // Bring window to front visually
        if (window->scene_tree) {
            wlr_scene_node_raise_to_top(&window->scene_tree->node);
        }
        
        AXIOM_LOG_DEBUG("Cycling to previous window: %s", 
                        window->xdg_toplevel ? (window->xdg_toplevel->title ?: "(no title)") : "(no toplevel)");
    }
}

void axiom_focus_cycle_end(struct axiom_server *server, bool confirm) {
    if (!server || !server->focus_manager || !server->focus_manager->is_cycling) return;
    
    struct axiom_focus_manager *manager = server->focus_manager;
    
    if (confirm && manager->cycle_current && manager->cycle_current->window) {
        // Confirm the focus change
        axiom_focus_window(server, manager->cycle_current->window);
        AXIOM_LOG_INFO("Focus cycling completed - focused: %s", 
                       manager->cycle_current->window->xdg_toplevel ? 
                       (manager->cycle_current->window->xdg_toplevel->title ?: "(no title)") : "(no toplevel)");
    } else {
        // Cancel - return to original focus
        if (manager->last_focused_window) {
            axiom_focus_window(server, manager->last_focused_window);
            AXIOM_LOG_INFO("Focus cycling cancelled - returned to original window");
        }
    }
    
    // Clean up cycling state
    struct axiom_focus_entry *entry, *tmp;
    wl_list_for_each_safe(entry, tmp, &manager->cycle_list, link) {
        wl_list_remove(&entry->link);
        free(entry);
    }
    
    manager->is_cycling = false;
    manager->cycle_current = NULL;
    wl_list_init(&manager->cycle_list);
}

// Additional focus stack management
void axiom_focus_stack_add(struct axiom_focus_manager *manager, struct axiom_window *window) {
    if (!manager || !window) return;
    
    // Check if already in stack
    struct axiom_focus_entry *entry;
    wl_list_for_each(entry, &manager->focus_stack, link) {
        if (entry->window == window) {
            return; // Already in stack
        }
    }
    
    // Add new entry
    entry = calloc(1, sizeof(struct axiom_focus_entry));
    if (!entry) return;
    
    entry->window = window;
    entry->last_focus_time = time(NULL);
    wl_list_insert(&manager->focus_stack, &entry->link);
    
    // Maintain size limit
    if (manager->current_history_size < manager->max_history_size) {
        manager->current_history_size++;
    } else {
        struct axiom_focus_entry *last = wl_container_of(manager->focus_stack.prev, last, link);
        wl_list_remove(&last->link);
        free(last);
    }
}

void axiom_focus_stack_remove(struct axiom_focus_manager *manager, struct axiom_window *window) {
    if (!manager || !window) return;
    
    struct axiom_focus_entry *entry, *tmp;
    wl_list_for_each_safe(entry, tmp, &manager->focus_stack, link) {
        if (entry->window == window) {
            wl_list_remove(&entry->link);
            free(entry);
            if (manager->current_history_size > 0) {
                manager->current_history_size--;
            }
            break;
        }
    }
}

struct axiom_window *axiom_focus_stack_get_next(struct axiom_focus_manager *manager, 
                                                struct axiom_window *current, bool reverse) {
    if (!manager || wl_list_empty(&manager->focus_stack)) return NULL;
    
    if (!current) {
        // Return first window in stack
        struct axiom_focus_entry *first = wl_container_of(manager->focus_stack.next, first, link);
        return first->window;
    }
    
    // Find current window in stack
    struct axiom_focus_entry *current_entry = NULL;
    struct axiom_focus_entry *entry;
    wl_list_for_each(entry, &manager->focus_stack, link) {
        if (entry->window == current) {
            current_entry = entry;
            break;
        }
    }
    
    if (!current_entry) return NULL;
    
    // Get next/prev window
    struct wl_list *target_link;
    if (reverse) {
        target_link = current_entry->link.prev;
        if (target_link == &manager->focus_stack) {
            target_link = manager->focus_stack.prev; // Wrap to last
        }
    } else {
        target_link = current_entry->link.next;
        if (target_link == &manager->focus_stack) {
            target_link = manager->focus_stack.next; // Wrap to first
        }
    }
    
    struct axiom_focus_entry *target = wl_container_of(target_link, target, link);
    return target->window;
}

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

void axiom_focus_next_window(struct axiom_server *server) {
    if (!server || !server->focus_manager) return;
    
    struct axiom_focus_manager *manager = server->focus_manager;
    struct axiom_window *current = manager->focused_window;
    
    // If no current window, focus the first available
    if (!current) {
        if (!wl_list_empty(&server->windows)) {
            struct axiom_window *first = wl_container_of(server->windows.next, first, link);
            axiom_focus_window(server, first);
        }
        return;
    }
    
    // Find next window in the server's window list
    struct axiom_window *next = NULL;
    bool found_current = false;
    
    struct axiom_window *window;
    wl_list_for_each(window, &server->windows, link) {
        if (found_current) {
            next = window;
            break;
        }
        if (window == current) {
            found_current = true;
        }
    }
    
    // If we reached the end or no next window, wrap around to first
    if (!next && !wl_list_empty(&server->windows)) {
        next = wl_container_of(server->windows.next, next, link);
    }
    
    if (next && next != current) {
        axiom_focus_window(server, next);
        AXIOM_LOG_INFO("Focused next window");
    }
}

void axiom_focus_prev_window(struct axiom_server *server) {
    if (!server || !server->focus_manager) return;
    
    struct axiom_focus_manager *manager = server->focus_manager;
    struct axiom_window *current = manager->focused_window;
    
    // If no current window, focus the last available
    if (!current) {
        if (!wl_list_empty(&server->windows)) {
            struct axiom_window *last = wl_container_of(server->windows.prev, last, link);
            axiom_focus_window(server, last);
        }
        return;
    }
    
    // Find previous window in the server's window list
    struct axiom_window *prev = NULL;
    
    struct axiom_window *window;
    wl_list_for_each_reverse(window, &server->windows, link) {
        if (window == current) {
            break;
        }
        prev = window;
    }
    
    // If no previous window, wrap around to last
    if (!prev && !wl_list_empty(&server->windows)) {
        prev = wl_container_of(server->windows.prev, prev, link);
    }
    
    if (prev && prev != current) {
        axiom_focus_window(server, prev);
        AXIOM_LOG_INFO("Focused previous window");
    }
}
