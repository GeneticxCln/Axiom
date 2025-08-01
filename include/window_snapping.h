#ifndef AXIOM_WINDOW_SNAPPING_H
#define AXIOM_WINDOW_SNAPPING_H

#include <stdbool.h>
#include <wayland-server-core.h>

// Forward declaration
struct axiom_server;
struct axiom_window;

// Direction for snapping
enum axiom_snap_direction {
    AXIOM_SNAP_NONE,
    AXIOM_SNAP_TOP,
    AXIOM_SNAP_BOTTOM,
    AXIOM_SNAP_LEFT,
    AXIOM_SNAP_RIGHT,
    AXIOM_SNAP_TOP_LEFT,
    AXIOM_SNAP_TOP_RIGHT,
    AXIOM_SNAP_BOTTOM_LEFT,
    AXIOM_SNAP_BOTTOM_RIGHT,
    AXIOM_SNAP_CENTER,
};

// Snap behavior options
struct axiom_snap_behavior {
    bool edge_resistance;         // Enable edge snapping resistance
    bool sticky_edges;            // Enable sticky edges for snapping
    bool magnetism;               // Snap windows together like magnets
    int resistance_threshold;     // Pixel distance for resistance activation
    int magnetism_range;          // Pixel range for magnetism influence
    int delay_ms;                 // Delay for snapping activation
};

// Window snapping state for individual windows
struct axiom_window_snapping_state {
    struct wl_list link;
    struct axiom_window *window;
    bool is_snapped;
    enum axiom_snap_direction snap_direction;
    struct {
        int x, y;
    } snap_position;
    int snap_edge_mask;
};

// Snap state for windows
struct axiom_snap_state {
    struct wl_list link;
    struct axiom_window *window;
    enum axiom_snap_direction current_snap;
    enum axiom_snap_direction target_snap;
    struct wl_listener move;
    struct wl_listener resize;
    struct wl_listener focus;
    struct wl_listener fullscreen_toggle;
};

// Snapping configuration structure
struct axiom_snapping_config {
    int snap_threshold;
    int edge_resistance;
    float magnetism_strength;
    int animation_duration;
    bool smart_corners;
    bool multi_monitor_snapping;
    bool window_to_window_snapping;
    bool edge_snapping;
};

// Snapping statistics
struct axiom_snapping_stats {
    unsigned int total_snaps;
    unsigned int edge_snaps;
    unsigned int window_snaps;
    unsigned int corner_snaps;
};

// Window snapping manager
struct axiom_window_snapping_manager {
    struct axiom_server *server;
    struct wl_list snapping_windows; // List of snapped windows
    struct wl_list snapped_windows;  // Alternative list for compatibility
    struct axiom_snap_behavior behavior;
    struct axiom_snapping_config config; // Configuration settings
    bool enabled;
    
    // Statistics and counters
    struct axiom_snapping_stats stats;
};

// Snapping functions
struct axiom_window_snapping_manager *axiom_window_snapping_manager_create(struct axiom_server *server);
void axiom_window_snapping_manager_destroy(struct axiom_window_snapping_manager *manager);
bool axiom_window_snapping_manager_init(struct axiom_window_snapping_manager *manager);

bool axiom_window_snap_to(struct axiom_window *window, enum axiom_snap_direction direction);
bool axiom_window_snap_toggle(struct axiom_window *window);
bool axiom_window_unsnap(struct axiom_window *window);

// Snap handling
void axiom_window_snapping_on_move(struct axiom_window *window);
void axiom_window_snapping_on_resize(struct axiom_window *window);
void axiom_window_snapping_on_focus(struct axiom_window *window);
void axiom_window_snapping_on_fullscreen_toggle(struct axiom_window *window);

// Configuration
bool axiom_window_snapping_load_config(struct axiom_window_snapping_manager *manager, const char *config_path);
bool axiom_window_snapping_save_config(struct axiom_window_snapping_manager *manager, const char *config_path);
void axiom_window_snapping_update_config(struct axiom_window_snapping_manager *manager, const struct axiom_snapping_config *config);

// Statistics
struct axiom_snapping_stats axiom_window_snapping_get_stats(struct axiom_window_snapping_manager *manager);
void axiom_window_snapping_reset_stats(struct axiom_window_snapping_manager *manager);

// Debug/Info
void axiom_window_snapping_print_stats(struct axiom_window_snapping_manager *manager);
void axiom_window_snapping_debug_state(struct axiom_window *window);

#endif // AXIOM_WINDOW_SNAPPING_H

