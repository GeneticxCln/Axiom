#ifndef AXIOM_WINDOW_MANAGER_H
#define AXIOM_WINDOW_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>

struct axiom_server;
struct axiom_window;

// Window state flags
enum axiom_window_state_flags {
    AXIOM_WINDOW_STATE_NORMAL = 0,
    AXIOM_WINDOW_STATE_MAXIMIZED = (1 << 0),
    AXIOM_WINDOW_STATE_FULLSCREEN = (1 << 1),
    AXIOM_WINDOW_STATE_MINIMIZED = (1 << 2),
    AXIOM_WINDOW_STATE_TILED = (1 << 3),
    AXIOM_WINDOW_STATE_FLOATING = (1 << 4),
    AXIOM_WINDOW_STATE_URGENT = (1 << 5),
    AXIOM_WINDOW_STATE_STICKY = (1 << 6),
    AXIOM_WINDOW_STATE_ALWAYS_ON_TOP = (1 << 7),
    AXIOM_WINDOW_STATE_HIDDEN = (1 << 8),
    AXIOM_WINDOW_STATE_MAPPED = (1 << 9),
    AXIOM_WINDOW_STATE_CONFIGURED = (1 << 10),
    AXIOM_WINDOW_STATE_FOCUSED = (1 << 11)
};

// Window positioning constraints
struct axiom_window_constraints {
    int min_width, min_height;
    int max_width, max_height;
    bool has_min_size;
    bool has_max_size;
    bool respect_aspect_ratio;
    double aspect_ratio;
};

// Window geometry with validation
struct axiom_window_geometry {
    int x, y;
    int width, height;
    
    // Saved geometry for restore operations
    int saved_x, saved_y;
    int saved_width, saved_height;
    bool has_saved_geometry;
    
    // Current and pending configure serials
    uint32_t configure_serial;
    uint32_t pending_configure_serial;
    
    // Constraints from the client
    struct axiom_window_constraints constraints;
    
    // Calculated usable area (excluding decorations)
    struct wlr_box content_box;
    
    // Decoration area
    struct wlr_box decoration_box;
    
    // Full window area (content + decorations)
    struct wlr_box full_box;
};

// Window positioning and layout calculations
struct axiom_window_layout {
    // Current layout mode
    enum {
        AXIOM_WM_LAYOUT_TILED,
        AXIOM_WM_LAYOUT_FLOATING,
        AXIOM_WM_LAYOUT_MAXIMIZED,
        AXIOM_WM_LAYOUT_FULLSCREEN
    } mode;
    
    // Tiling specific data
    struct {
        int grid_x, grid_y;        // Position in tiling grid
        int grid_cols, grid_rows;  // Grid dimensions
        bool is_master;            // Is this the master window
        float split_ratio;         // Split ratio for this window
        enum {
            AXIOM_TILING_NONE,
            AXIOM_TILING_LEFT,
            AXIOM_TILING_RIGHT,
            AXIOM_TILING_TOP,
            AXIOM_TILING_BOTTOM
        } tiling_edge;
    } tiling;
    
    // Workspace assignment
    int workspace;
    bool workspace_sticky;      // Visible on all workspaces
    
    // Z-order management
    int z_index;
    bool always_on_top;
    bool always_on_bottom;
};

// Window state tracking
struct axiom_window_state {
    uint32_t state_flags;       // Combination of axiom_window_state flags
    uint32_t previous_state;    // Previous state for restoration
    
    // Focus state
    bool is_focused;
    bool can_focus;
    uint32_t last_focus_time;
    
    // Interaction state
    bool is_being_moved;
    bool is_being_resized;
    uint32_t resize_edges;
    
    // Visual state
    bool is_visible;
    bool decorations_visible;
    float opacity;
    
    // Animation state
    bool is_animating;
    uint32_t animation_id;
    
    // Client state synchronization
    bool needs_configure;
    bool configure_pending;
    struct timespec last_configure_time;
};

// Window manager structure
struct axiom_window_manager {
    struct axiom_server *server;
    
    // Window lists organized by state
    struct wl_list all_windows;        // All windows
    struct wl_list mapped_windows;     // Currently mapped windows
    struct wl_list floating_windows;   // Floating windows
    struct wl_list tiled_windows;      // Tiled windows
    struct wl_list urgent_windows;     // Windows needing attention
    
    // Focus management
    struct axiom_window *focused_window;
    struct axiom_window *last_focused_window;
    struct wl_list focus_history;      // Focus history stack
    
    // Layout state
    int workspace_width, workspace_height;
    int title_bar_height;
    int border_width;
    int gap_size;
    
    // Configuration
    bool auto_focus_new_windows;
    bool focus_follows_mouse;
    bool click_to_focus;
    int max_focus_history;
    
    // Performance optimization
    bool layout_calculation_pending;
    bool geometry_update_pending;
    struct wl_event_source *layout_timer;
};

// Core window management functions
struct axiom_window_manager *axiom_window_manager_create(struct axiom_server *server);
void axiom_window_manager_destroy(struct axiom_window_manager *manager);

// Window lifecycle management
void axiom_window_manager_add_window(struct axiom_window_manager *manager, struct axiom_window *window);
void axiom_window_manager_remove_window(struct axiom_window_manager *manager, struct axiom_window *window);
void axiom_window_manager_map_window(struct axiom_window_manager *manager, struct axiom_window *window);
void axiom_window_manager_unmap_window(struct axiom_window_manager *manager, struct axiom_window *window);

// Window positioning and geometry
bool axiom_window_calculate_geometry(struct axiom_window_manager *manager, 
                                    struct axiom_window *window,
                                    struct axiom_window_geometry *geometry);
void axiom_window_apply_geometry(struct axiom_window *window, 
                                const struct axiom_window_geometry *geometry);
void axiom_window_validate_geometry(struct axiom_window *window, 
                                   struct axiom_window_geometry *geometry);
bool axiom_window_geometry_changed(const struct axiom_window_geometry *old_geo,
                                  const struct axiom_window_geometry *new_geo);

// Window state management
void axiom_window_set_state(struct axiom_window *window, enum axiom_window_state_flags state, bool enabled);
bool axiom_window_has_state(struct axiom_window *window, enum axiom_window_state_flags state);
void axiom_window_save_state(struct axiom_window *window);
void axiom_window_restore_state(struct axiom_window *window);
void axiom_window_update_state(struct axiom_window *window);

// Focus management functions
void axiom_window_manager_focus_window(struct axiom_window_manager *manager, struct axiom_window *window);
void axiom_window_manager_focus_next(struct axiom_window_manager *manager);
void axiom_window_manager_focus_prev(struct axiom_window_manager *manager);
struct axiom_window *axiom_window_manager_find_focusable_window(struct axiom_window_manager *manager);
void axiom_window_manager_update_focus_history(struct axiom_window_manager *manager, struct axiom_window *window);

// Layout and arrangement
void axiom_window_manager_arrange_all(struct axiom_window_manager *manager);
void axiom_window_manager_arrange_workspace(struct axiom_window_manager *manager, int workspace);
void axiom_window_manager_arrange_tiled_windows(struct axiom_window_manager *manager);
void axiom_window_manager_update_window_layout(struct axiom_window_manager *manager, struct axiom_window *window);

// Window operations
void axiom_window_manager_maximize_window(struct axiom_window_manager *manager, struct axiom_window *window);
void axiom_window_manager_unmaximize_window(struct axiom_window_manager *manager, struct axiom_window *window);
void axiom_window_manager_fullscreen_window(struct axiom_window_manager *manager, struct axiom_window *window);
void axiom_window_manager_unfullscreen_window(struct axiom_window_manager *manager, struct axiom_window *window);
void axiom_window_manager_minimize_window(struct axiom_window_manager *manager, struct axiom_window *window);
void axiom_window_manager_restore_window(struct axiom_window_manager *manager, struct axiom_window *window);
void axiom_window_manager_close_window(struct axiom_window_manager *manager, struct axiom_window *window);

// Window movement and resizing
void axiom_window_manager_move_window(struct axiom_window_manager *manager, 
                                     struct axiom_window *window, int x, int y);
void axiom_window_manager_resize_window(struct axiom_window_manager *manager,
                                       struct axiom_window *window, int width, int height);
void axiom_window_manager_move_resize_window(struct axiom_window_manager *manager,
                                            struct axiom_window *window,
                                            int x, int y, int width, int height);

// Window finding and querying
struct axiom_window *axiom_window_manager_window_at(struct axiom_window_manager *manager, 
                                                   double x, double y,
                                                   struct wlr_surface **surface,
                                                   double *sx, double *sy);
struct axiom_window *axiom_window_manager_find_window_by_surface(struct axiom_window_manager *manager,
                                                                struct wlr_surface *surface);
int axiom_window_manager_count_windows(struct axiom_window_manager *manager, int workspace);
int axiom_window_manager_count_tiled_windows(struct axiom_window_manager *manager, int workspace);

// Workspace management
void axiom_window_manager_move_window_to_workspace(struct axiom_window_manager *manager,
                                                  struct axiom_window *window, int workspace);
void axiom_window_manager_show_workspace(struct axiom_window_manager *manager, int workspace);
void axiom_window_manager_hide_workspace(struct axiom_window_manager *manager, int workspace);

// Utility functions
bool axiom_window_manager_is_window_visible(struct axiom_window_manager *manager, struct axiom_window *window);
bool axiom_window_manager_can_tile_window(struct axiom_window_manager *manager, struct axiom_window *window);
void axiom_window_manager_set_workspace_geometry(struct axiom_window_manager *manager, 
                                                 int width, int height);
void axiom_window_manager_configure(struct axiom_window_manager *manager);

// Event handling
void axiom_window_manager_handle_new_toplevel(struct axiom_window_manager *manager, 
                                             struct wlr_xdg_toplevel *toplevel);
void axiom_window_manager_handle_surface_commit(struct axiom_window_manager *manager,
                                               struct axiom_window *window);
void axiom_window_manager_handle_configure_ack(struct axiom_window_manager *manager,
                                              struct axiom_window *window,
                                              uint32_t serial);

// Enhanced window geometry functions
void axiom_window_geometry_init(struct axiom_window_geometry *geometry, int x, int y, int width, int height);
void axiom_window_geometry_copy(struct axiom_window_geometry *dst, const struct axiom_window_geometry *src);
bool axiom_window_geometry_equals(const struct axiom_window_geometry *a, const struct axiom_window_geometry *b);
void axiom_window_geometry_save(struct axiom_window_geometry *geometry);
void axiom_window_geometry_restore(struct axiom_window_geometry *geometry);
void axiom_window_geometry_update_boxes(struct axiom_window_geometry *geometry, 
                                       int title_bar_height, int border_width);

// Window constraints functions
void axiom_window_constraints_init(struct axiom_window_constraints *constraints);
void axiom_window_constraints_apply(struct axiom_window_constraints *constraints,
                                   int *width, int *height);
bool axiom_window_constraints_validate_size(const struct axiom_window_constraints *constraints,
                                           int width, int height);

// Layout calculation functions
void axiom_window_layout_init(struct axiom_window_layout *layout);
void axiom_window_layout_calculate_tiled_position(struct axiom_window_manager *manager,
                                                 struct axiom_window *window,
                                                 struct axiom_window_layout *layout,
                                                 int *x, int *y, int *width, int *height);
void axiom_window_layout_calculate_floating_position(struct axiom_window_manager *manager,
                                                    struct axiom_window *window,
                                                    struct axiom_window_layout *layout,
                                                    int *x, int *y, int *width, int *height);

// Window state functions
void axiom_window_state_init(struct axiom_window_state *state);
void axiom_window_state_copy(struct axiom_window_state *dst, const struct axiom_window_state *src);
bool axiom_window_state_changed(const struct axiom_window_state *old_state,
                                const struct axiom_window_state *new_state);

#endif /* AXIOM_WINDOW_MANAGER_H */
