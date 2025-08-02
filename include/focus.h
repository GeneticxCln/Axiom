#ifndef AXIOM_FOCUS_H
#define AXIOM_FOCUS_H

#include <stdbool.h>
#include <wayland-server-core.h>

struct axiom_server;
struct axiom_window;

// Focus stack entry
struct axiom_focus_entry {
    struct wl_list link;
    struct axiom_window *window;
    uint32_t last_focus_time;
    bool is_urgent;
};

// Focus and stacking manager
struct axiom_focus_manager {
    struct wl_list focus_stack;     // Most recently focused windows (head = most recent)
    struct wl_list urgent_windows;  // Windows that need attention
    struct axiom_window *focused_window;
    struct axiom_window *last_focused_window;
    
    // Focus cycling state
    bool is_cycling;
    struct axiom_focus_entry *cycle_current;
    struct wl_list cycle_list;      // Temporary list for Alt+Tab cycling
    
    // Urgency notification
    bool urgency_notifications_enabled;
    uint32_t urgency_timeout_ms;
    struct wl_event_source *urgency_timer;
    
    // Focus history
    int max_history_size;
    int current_history_size;
};

// Focus management functions
void axiom_focus_manager_init(struct axiom_focus_manager *manager);
void axiom_focus_manager_cleanup(struct axiom_focus_manager *manager);

// Window focus operations
void axiom_focus_window(struct axiom_server *server, struct axiom_window *window);
void axiom_focus_next_window(struct axiom_server *server);
void axiom_focus_prev_window(struct axiom_server *server);
void axiom_focus_most_recent(struct axiom_server *server);
void axiom_focus_last_window(struct axiom_server *server);

// Focus cycling (Alt+Tab style)
void axiom_focus_cycle_start(struct axiom_server *server, bool reverse);
void axiom_focus_cycle_next(struct axiom_server *server);
void axiom_focus_cycle_prev(struct axiom_server *server);
void axiom_focus_cycle_end(struct axiom_server *server, bool confirm);

// Focus stack management
void axiom_focus_stack_add(struct axiom_focus_manager *manager, struct axiom_window *window);
void axiom_focus_stack_remove(struct axiom_focus_manager *manager, struct axiom_window *window);
void axiom_focus_stack_promote(struct axiom_focus_manager *manager, struct axiom_window *window);
struct axiom_window *axiom_focus_stack_get_next(struct axiom_focus_manager *manager, 
                                                struct axiom_window *current, bool reverse);

// Urgency system
void axiom_window_set_urgent(struct axiom_server *server, struct axiom_window *window, bool urgent);
void axiom_window_clear_urgency(struct axiom_server *server, struct axiom_window *window);
bool axiom_window_is_urgent(struct axiom_window *window);
void axiom_focus_urgent_window(struct axiom_server *server);
int axiom_focus_count_urgent_windows(struct axiom_server *server);

// Focus history
struct axiom_window *axiom_focus_get_nth_recent(struct axiom_focus_manager *manager, int n);
void axiom_focus_print_stack(struct axiom_focus_manager *manager);

// Focus predicates and utilities
bool axiom_window_can_focus(struct axiom_window *window);
bool axiom_window_should_auto_focus(struct axiom_window *window);
struct axiom_window *axiom_focus_find_focusable_window(struct axiom_server *server);

// Focus direction for directional focus
enum axiom_focus_direction {
    AXIOM_FOCUS_LEFT,
    AXIOM_FOCUS_RIGHT,
    AXIOM_FOCUS_UP,
    AXIOM_FOCUS_DOWN
};

void axiom_focus_direction(struct axiom_server *server, enum axiom_focus_direction direction);
struct axiom_window *axiom_focus_find_window_in_direction(struct axiom_server *server,
                                                         struct axiom_window *from_window,
                                                         enum axiom_focus_direction direction);

// Event handling
void axiom_focus_handle_window_map(struct axiom_server *server, struct axiom_window *window);
void axiom_focus_handle_window_unmap(struct axiom_server *server, struct axiom_window *window);
void axiom_focus_handle_window_destroy(struct axiom_server *server, struct axiom_window *window);

// Configuration
void axiom_focus_set_urgency_timeout(struct axiom_focus_manager *manager, uint32_t timeout_ms);
void axiom_focus_set_max_history(struct axiom_focus_manager *manager, int max_size);
void axiom_focus_enable_urgency_notifications(struct axiom_focus_manager *manager, bool enabled);

#endif /* AXIOM_FOCUS_H */
