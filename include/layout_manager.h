#ifndef AXIOM_LAYOUT_MANAGER_H
#define AXIOM_LAYOUT_MANAGER_H

#include <wayland-server-core.h>
#include <stdbool.h>
#include <stdint.h>

// Forward declarations
struct axiom_server;
struct axiom_window;
struct axiom_workspace;
struct axiom_monitor;

// Direction enumeration for window movement and creation
enum axiom_direction {
    AXIOM_DIRECTION_DEFAULT = -1,
    AXIOM_DIRECTION_UP = 0,
    AXIOM_DIRECTION_RIGHT,
    AXIOM_DIRECTION_DOWN,
    AXIOM_DIRECTION_LEFT
};

// Corner enumeration for resizing operations
enum axiom_rect_corner {
    AXIOM_CORNER_NONE = 0,
    AXIOM_CORNER_TOPLEFT = (1 << 0),
    AXIOM_CORNER_TOPRIGHT = (1 << 1),
    AXIOM_CORNER_BOTTOMRIGHT = (1 << 2),
    AXIOM_CORNER_BOTTOMLEFT = (1 << 3),
};

// Snap edge enumeration for window snapping
enum axiom_snap_edge {
    AXIOM_SNAP_INVALID = 0,
    AXIOM_SNAP_UP = (1 << 0),
    AXIOM_SNAP_DOWN = (1 << 1),
    AXIOM_SNAP_LEFT = (1 << 2),
    AXIOM_SNAP_RIGHT = (1 << 3),
};

// Layout types supported by Axiom
enum axiom_layout_type_enhanced {
    AXIOM_LAYOUT_DWINDLE = 0,    // Binary space partitioning (Hyprland-style)
    AXIOM_LAYOUT_MASTER,         // Master-stack layout
    AXIOM_LAYOUT_GRID,           // Grid layout
    AXIOM_LAYOUT_SPIRAL,         // Spiral layout
    AXIOM_LAYOUT_FLOATING,       // Floating windows
    AXIOM_LAYOUT_CUSTOM          // Custom/plugin layouts
};

// Window render layout hints (for special rendering)
struct axiom_window_render_hints {
    bool is_border_gradient;
    void *border_gradient_data;
    bool force_opaque;
    bool disable_rounded_corners;
    bool custom_shadow;
    float alpha_override;
};

// Layout message header for custom layout communication
struct axiom_layout_message_header {
    struct axiom_window *window;
    struct axiom_workspace *workspace;
    struct axiom_monitor *monitor;
    uint32_t flags;
};

// Base layout interface structure (inspired by Hyprland's IHyprLayout)
struct axiom_layout_interface {
    // Layout identification
    const char *name;
    enum axiom_layout_type_enhanced type;
    
    // Core lifecycle functions
    void (*on_enable)(struct axiom_layout_interface *layout);
    void (*on_disable)(struct axiom_layout_interface *layout);
    
    // Window lifecycle functions
    void (*on_window_created)(struct axiom_layout_interface *layout, 
                             struct axiom_window *window, 
                             enum axiom_direction direction);
    void (*on_window_created_tiling)(struct axiom_layout_interface *layout,
                                    struct axiom_window *window,
                                    enum axiom_direction direction);
    void (*on_window_created_floating)(struct axiom_layout_interface *layout,
                                      struct axiom_window *window);
    void (*on_window_removed)(struct axiom_layout_interface *layout,
                             struct axiom_window *window);
    void (*on_window_removed_tiling)(struct axiom_layout_interface *layout,
                                    struct axiom_window *window);
    void (*on_window_removed_floating)(struct axiom_layout_interface *layout,
                                      struct axiom_window *window);
    
    // Window state queries
    bool (*is_window_tiled)(struct axiom_layout_interface *layout,
                           struct axiom_window *window);
    bool (*is_window_reachable)(struct axiom_layout_interface *layout,
                               struct axiom_window *window);
    
    // Layout calculations and updates
    void (*recalculate_monitor)(struct axiom_layout_interface *layout,
                               struct axiom_monitor *monitor);
    void (*recalculate_window)(struct axiom_layout_interface *layout,
                              struct axiom_window *window);
    void (*recalculate_workspace)(struct axiom_layout_interface *layout,
                                 struct axiom_workspace *workspace);
    
    // Window manipulation
    void (*resize_active_window)(struct axiom_layout_interface *layout,
                               double delta_x, double delta_y,
                               enum axiom_rect_corner corner,
                               struct axiom_window *window);
    void (*move_active_window)(struct axiom_layout_interface *layout,
                              double delta_x, double delta_y,
                              struct axiom_window *window);
    void (*change_window_floating_mode)(struct axiom_layout_interface *layout,
                                       struct axiom_window *window);
    
    // Focus and navigation
    struct axiom_window *(*get_next_window_candidate)(struct axiom_layout_interface *layout,
                                                     struct axiom_window *current);
    void (*on_window_focus_change)(struct axiom_layout_interface *layout,
                                  struct axiom_window *window);
    void (*bring_window_to_top)(struct axiom_layout_interface *layout,
                               struct axiom_window *window);
    void (*request_focus_for_window)(struct axiom_layout_interface *layout,
                                    struct axiom_window *window);
    
    // Window operations
    void (*switch_windows)(struct axiom_layout_interface *layout,
                          struct axiom_window *window1,
                          struct axiom_window *window2);
    void (*move_window_to)(struct axiom_layout_interface *layout,
                          struct axiom_window *window,
                          const char *direction,
                          bool silent);
    void (*alter_split_ratio)(struct axiom_layout_interface *layout,
                             struct axiom_window *window,
                             float ratio,
                             bool exact);
    
    // Fullscreen management
    void (*fullscreen_request_for_window)(struct axiom_layout_interface *layout,
                                         struct axiom_window *window,
                                         bool enable);
    
    // Drag operations
    void (*on_begin_drag_window)(struct axiom_layout_interface *layout);
    void (*on_end_drag_window)(struct axiom_layout_interface *layout);
    void (*on_mouse_move)(struct axiom_layout_interface *layout,
                         double x, double y);
    
    // Layout customization and messaging
    void *(*layout_message)(struct axiom_layout_interface *layout,
                           struct axiom_layout_message_header *header,
                           const char *message);
    struct axiom_window_render_hints (*request_render_hints)(struct axiom_layout_interface *layout,
                                                            struct axiom_window *window);
    
    // Window prediction for new windows
    void (*predict_size_for_new_window_tiled)(struct axiom_layout_interface *layout,
                                             int *width, int *height);
    
    // Data replacement for window moves between workspaces
    void (*replace_window_data_with)(struct axiom_layout_interface *layout,
                                    struct axiom_window *from,
                                    struct axiom_window *to);
    
    // Layout-specific data
    void *layout_data;           // Layout-specific private data
    
    // Configuration
    struct {
        float master_ratio;
        int master_count;
        bool smart_gaps;
        bool smart_resizing;
        bool preserve_split;
        bool new_is_master;
        bool new_on_top;
        bool no_gaps_when_only;
        bool force_splits;
        bool always_center_master;
        int default_split_ratio;
        bool special_scale_factor;
        bool mouse_resize;
        bool pseudotile;
        bool respect_float_splits;
        bool inherit_fullscreen;
    } config;
};

// Layout manager structure
struct axiom_layout_manager {
    struct axiom_server *server;
    
    // Available layouts
    struct wl_list layouts;              // List of available layout interfaces
    struct axiom_layout_interface *current_layout;
    struct axiom_layout_interface *previous_layout;
    
    // Built-in layouts
    struct axiom_layout_interface *dwindle_layout;
    struct axiom_layout_interface *master_layout;
    struct axiom_layout_interface *grid_layout;
    struct axiom_layout_interface *spiral_layout;
    struct axiom_layout_interface *floating_layout;
    
    // Layout state
    enum axiom_layout_type_enhanced current_layout_type;
    bool layout_switching_in_progress;
    
    // Global layout configuration
    struct {
        bool animations_enabled;
        bool smart_gaps_enabled;
        bool smart_borders_enabled;
        bool preserve_split_on_layout_change;
        float default_split_ratio;
        int border_size;
        int gaps_in;
        int gaps_out;
        int gaps_workspaces;
    } global_config;
    
    // Performance tracking
    uint64_t last_recalc_time;
    uint32_t recalc_count;
    float avg_recalc_time;
    
    // Event listeners
    struct wl_listener workspace_change;
    struct wl_listener monitor_change;
    struct wl_listener window_focus_change;
};

// Layout manager functions
struct axiom_layout_manager *axiom_layout_manager_create(struct axiom_server *server);
void axiom_layout_manager_destroy(struct axiom_layout_manager *manager);

// Layout registration and management
bool axiom_layout_manager_register_layout(struct axiom_layout_manager *manager,
                                         struct axiom_layout_interface *layout);
bool axiom_layout_manager_unregister_layout(struct axiom_layout_manager *manager,
                                           struct axiom_layout_interface *layout);
void axiom_layout_manager_switch_to_layout(struct axiom_layout_manager *manager,
                                          enum axiom_layout_type_enhanced type);
void axiom_layout_manager_switch_to_layout_by_name(struct axiom_layout_manager *manager,
                                                  const char *name);

// Layout queries
struct axiom_layout_interface *axiom_layout_manager_get_current_layout(struct axiom_layout_manager *manager);
struct axiom_layout_interface *axiom_layout_manager_get_layout_by_type(struct axiom_layout_manager *manager,
                                                                       enum axiom_layout_type_enhanced type);
struct axiom_layout_interface *axiom_layout_manager_get_layout_by_name(struct axiom_layout_manager *manager,
                                                                       const char *name);
const char **axiom_layout_manager_get_layout_names(struct axiom_layout_manager *manager, int *count);

// Window operations through layout manager
void axiom_layout_manager_handle_new_window(struct axiom_layout_manager *manager,
                                           struct axiom_window *window,
                                           enum axiom_direction direction);
void axiom_layout_manager_handle_window_removed(struct axiom_layout_manager *manager,
                                               struct axiom_window *window);
void axiom_layout_manager_handle_window_floating_change(struct axiom_layout_manager *manager,
                                                       struct axiom_window *window);

// Layout recalculation triggers
void axiom_layout_manager_recalculate_monitor(struct axiom_layout_manager *manager,
                                             struct axiom_monitor *monitor);
void axiom_layout_manager_recalculate_workspace(struct axiom_layout_manager *manager,
                                               struct axiom_workspace *workspace);
void axiom_layout_manager_recalculate_all(struct axiom_layout_manager *manager);

// Window manipulation through layouts
void axiom_layout_manager_resize_window(struct axiom_layout_manager *manager,
                                       struct axiom_window *window,
                                       double delta_x, double delta_y,
                                       enum axiom_rect_corner corner);
void axiom_layout_manager_move_window(struct axiom_layout_manager *manager,
                                     struct axiom_window *window,
                                     double delta_x, double delta_y);
void axiom_layout_manager_switch_windows(struct axiom_layout_manager *manager,
                                        struct axiom_window *window1,
                                        struct axiom_window *window2);

// Focus management
struct axiom_window *axiom_layout_manager_get_next_window(struct axiom_layout_manager *manager,
                                                         struct axiom_window *current);
void axiom_layout_manager_focus_window(struct axiom_layout_manager *manager,
                                      struct axiom_window *window);

// Layout configuration
void axiom_layout_manager_set_master_ratio(struct axiom_layout_manager *manager, float ratio);
float axiom_layout_manager_get_master_ratio(struct axiom_layout_manager *manager);
void axiom_layout_manager_set_gaps(struct axiom_layout_manager *manager, 
                                  int gaps_in, int gaps_out, int gaps_workspaces);

// Built-in layout creators
struct axiom_layout_interface *axiom_dwindle_layout_create(void);
struct axiom_layout_interface *axiom_master_layout_create(void);
struct axiom_layout_interface *axiom_grid_layout_create(void);
struct axiom_layout_interface *axiom_spiral_layout_create(void);
struct axiom_layout_interface *axiom_floating_layout_create(void);

// Layout utilities
bool axiom_layout_interface_is_valid(struct axiom_layout_interface *layout);
void axiom_layout_interface_destroy(struct axiom_layout_interface *layout);

// Event handlers
void axiom_layout_manager_handle_workspace_change(struct wl_listener *listener, void *data);
void axiom_layout_manager_handle_monitor_change(struct wl_listener *listener, void *data);
void axiom_layout_manager_handle_window_focus_change(struct wl_listener *listener, void *data);

// Debug and monitoring
void axiom_layout_manager_print_info(struct axiom_layout_manager *manager);
void axiom_layout_manager_validate_state(struct axiom_layout_manager *manager);

#endif /* AXIOM_LAYOUT_MANAGER_H */
