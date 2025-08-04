#ifndef AXIOM_DWINDLE_LAYOUT_H
#define AXIOM_DWINDLE_LAYOUT_H

#include "layout_manager.h"
#include <wayland-server-core.h>
#include <stdbool.h>
#include <stdint.h>

// Forward declarations
struct axiom_window;
struct axiom_workspace;
struct axiom_monitor;

// Dwindle node types
enum axiom_dwindle_node_type {
    AXIOM_DWINDLE_NODE_WINDOW = 0,   // Leaf node containing a window
    AXIOM_DWINDLE_NODE_CONTAINER     // Internal node (split container)
};

// Split direction for dwindle nodes
enum axiom_dwindle_split_direction {
    AXIOM_DWINDLE_SPLIT_HORIZONTAL = 0,  // Left/Right split
    AXIOM_DWINDLE_SPLIT_VERTICAL         // Top/Bottom split
};

// Dwindle node structure (binary tree for window layout)
struct axiom_dwindle_node {
    struct wl_list link;
    
    // Node identification
    enum axiom_dwindle_node_type type;
    struct axiom_workspace *workspace;
    
    // Tree structure
    struct axiom_dwindle_node *parent;
    struct axiom_dwindle_node *child1;  // Left/Top child
    struct axiom_dwindle_node *child2;  // Right/Bottom child
    
    // Window reference (for leaf nodes)
    struct axiom_window *window;
    
    // Container properties (for internal nodes)
    enum axiom_dwindle_split_direction split_direction;
    float split_ratio;               // Ratio for child1 (0.0 to 1.0)
    
    // Geometry
    int x, y;                        // Position
    int width, height;               // Size
    
    // State
    bool visible;
    bool recalc_needed;
    bool size_hints_applied;
    
    // Animation data
    struct {
        bool animating;
        int target_x, target_y;
        int target_width, target_height;
        float animation_progress;
        uint64_t animation_start_time;
        uint32_t animation_duration;
    } animation;
    
    // Layout hints
    struct {
        bool preserve_aspect_ratio;
        bool no_split;
        bool force_split_direction;
        enum axiom_dwindle_split_direction forced_direction;
        float min_split_ratio;
        float max_split_ratio;
    } hints;
};

// Dwindle layout configuration
struct axiom_dwindle_config {
    // Split behavior
    bool smart_split;                    // Smart split direction selection
    bool force_split;                    // Force splitting even for single windows
    bool pseudotile;                     // Enable pseudotiling
    bool preserve_split;                 // Preserve split ratios on window changes
    bool smart_resizing;                 // Smart resizing behavior
    bool use_active_for_splits;          // Use active window for split calculations
    
    // Split ratios
    float default_split_ratio;           // Default split ratio (0.5 = 50/50)
    float min_split_ratio;               // Minimum split ratio
    float max_split_ratio;               // Maximum split ratio
    
    // Window placement
    bool new_is_master;                  // New windows become master
    bool new_on_top;                     // New windows go on top
    bool col_maximize_one;               // Maximize single window in column
    bool special_scale_factor;           // Use special scale factor
    
    // Gaps and spacing
    bool no_gaps_when_only;              // No gaps when only one window
    int col_gap_size;                    // Gap between columns
    int row_gap_size;                    // Gap between rows
    
    // Animation settings
    bool animate_splits;                 // Animate split changes
    uint32_t split_animation_duration;   // Animation duration in ms
    char *split_animation_curve;         // Animation easing curve
};

// Dwindle layout data structure
struct axiom_dwindle_layout_data {
    struct axiom_workspace *workspace;
    
    // Tree structure
    struct axiom_dwindle_node *root;     // Root of the binary tree
    struct wl_list all_nodes;            // List of all nodes for cleanup
    
    // State tracking
    struct axiom_dwindle_node *focused_node;
    struct axiom_window *last_focused_window;
    
    // Configuration
    struct axiom_dwindle_config config;
    
    // Performance tracking
    uint32_t node_count;
    uint32_t window_count;
    uint64_t last_recalc_time;
    uint32_t recalc_count;
    
    // Workspace-specific overrides
    bool force_recalc;
    bool layout_changed;
    
    // Animation state
    bool animation_in_progress;
    uint32_t active_animations;
};

// Dwindle layout structure
struct axiom_dwindle_layout {
    struct axiom_layout_interface base;
    
    // Layout-specific data per workspace
    struct wl_list workspace_data;       // List of axiom_dwindle_layout_data
    
    // Global configuration
    struct axiom_dwindle_config global_config;
    
    // Performance monitoring
    struct {
        uint64_t total_recalc_time;
        uint32_t total_recalcs;
        float avg_recalc_time;
        uint32_t total_splits;
        uint32_t total_merges;
    } stats;
};

// Dwindle layout functions
struct axiom_layout_interface *axiom_dwindle_layout_create(void);
void axiom_dwindle_layout_destroy(struct axiom_dwindle_layout *layout);

// Dwindle-specific functions
struct axiom_dwindle_layout_data *axiom_dwindle_get_workspace_data(struct axiom_dwindle_layout *layout,
                                                                  struct axiom_workspace *workspace);
struct axiom_dwindle_layout_data *axiom_dwindle_create_workspace_data(struct axiom_dwindle_layout *layout,
                                                                     struct axiom_workspace *workspace);
void axiom_dwindle_destroy_workspace_data(struct axiom_dwindle_layout_data *data);

// Node management
struct axiom_dwindle_node *axiom_dwindle_node_create(enum axiom_dwindle_node_type type);
void axiom_dwindle_node_destroy(struct axiom_dwindle_node *node);
struct axiom_dwindle_node *axiom_dwindle_node_find_window(struct axiom_dwindle_node *root,
                                                         struct axiom_window *window);
struct axiom_dwindle_node *axiom_dwindle_node_find_parent_of_window(struct axiom_dwindle_node *root,
                                                                   struct axiom_window *window);

// Tree operations
void axiom_dwindle_insert_window(struct axiom_dwindle_layout_data *data,
                                struct axiom_window *window,
                                enum axiom_direction direction);
void axiom_dwindle_remove_window(struct axiom_dwindle_layout_data *data,
                                struct axiom_window *window);
void axiom_dwindle_split_node(struct axiom_dwindle_node *node,
                             struct axiom_window *new_window,
                             enum axiom_dwindle_split_direction direction);
void axiom_dwindle_merge_nodes(struct axiom_dwindle_node *parent);

// Layout calculations
void axiom_dwindle_recalculate_tree(struct axiom_dwindle_layout_data *data);
void axiom_dwindle_calculate_node_geometry(struct axiom_dwindle_node *node,
                                          int x, int y, int width, int height);
void axiom_dwindle_apply_node_geometry(struct axiom_dwindle_node *node);

// Split ratio management
void axiom_dwindle_set_split_ratio(struct axiom_dwindle_node *node, float ratio);
float axiom_dwindle_get_split_ratio(struct axiom_dwindle_node *node);
void axiom_dwindle_adjust_split_ratio(struct axiom_dwindle_node *node, float delta, bool exact);

// Window navigation
struct axiom_window *axiom_dwindle_get_next_window_in_direction(struct axiom_dwindle_layout_data *data,
                                                               struct axiom_window *current,
                                                               enum axiom_direction direction);
struct axiom_window *axiom_dwindle_get_next_window_cyclic(struct axiom_dwindle_layout_data *data,
                                                         struct axiom_window *current,
                                                         bool reverse);

// Window manipulation
void axiom_dwindle_swap_windows(struct axiom_dwindle_layout_data *data,
                               struct axiom_window *window1,
                               struct axiom_window *window2);
void axiom_dwindle_move_window_in_direction(struct axiom_dwindle_layout_data *data,
                                           struct axiom_window *window,
                                           enum axiom_direction direction);
void axiom_dwindle_resize_window(struct axiom_dwindle_layout_data *data,
                                struct axiom_window *window,
                                double delta_x, double delta_y,
                                enum axiom_rect_corner corner);

// Smart features
enum axiom_dwindle_split_direction axiom_dwindle_smart_split_direction(struct axiom_dwindle_node *node,
                                                                      struct axiom_window *new_window);
void axiom_dwindle_optimize_tree(struct axiom_dwindle_layout_data *data);
void axiom_dwindle_balance_tree(struct axiom_dwindle_layout_data *data);

// Configuration
void axiom_dwindle_set_config(struct axiom_dwindle_layout *layout,
                             struct axiom_dwindle_config *config);
struct axiom_dwindle_config *axiom_dwindle_get_config(struct axiom_dwindle_layout *layout);
void axiom_dwindle_reset_config_to_defaults(struct axiom_dwindle_config *config);

// Animation support
void axiom_dwindle_start_node_animation(struct axiom_dwindle_node *node,
                                       int target_x, int target_y,
                                       int target_width, int target_height,
                                       uint32_t duration);
void axiom_dwindle_update_animations(struct axiom_dwindle_layout_data *data, uint64_t current_time);
bool axiom_dwindle_has_active_animations(struct axiom_dwindle_layout_data *data);

// Tree traversal and utilities
void axiom_dwindle_traverse_tree(struct axiom_dwindle_node *node,
                                void (*callback)(struct axiom_dwindle_node *node, void *user_data),
                                void *user_data);
int axiom_dwindle_count_windows_in_tree(struct axiom_dwindle_node *node);
int axiom_dwindle_get_tree_depth(struct axiom_dwindle_node *node);
bool axiom_dwindle_is_tree_balanced(struct axiom_dwindle_node *node);

// Debug and validation
void axiom_dwindle_print_tree(struct axiom_dwindle_node *node, int depth);
void axiom_dwindle_validate_tree(struct axiom_dwindle_node *node);
void axiom_dwindle_print_layout_stats(struct axiom_dwindle_layout *layout);

// Layout interface implementation functions
void axiom_dwindle_on_enable(struct axiom_layout_interface *layout);
void axiom_dwindle_on_disable(struct axiom_layout_interface *layout);
void axiom_dwindle_on_window_created(struct axiom_layout_interface *layout,
                                    struct axiom_window *window,
                                    enum axiom_direction direction);
void axiom_dwindle_on_window_created_tiling(struct axiom_layout_interface *layout,
                                           struct axiom_window *window,
                                           enum axiom_direction direction);
void axiom_dwindle_on_window_removed(struct axiom_layout_interface *layout,
                                    struct axiom_window *window);
void axiom_dwindle_on_window_removed_tiling(struct axiom_layout_interface *layout,
                                           struct axiom_window *window);
bool axiom_dwindle_is_window_tiled(struct axiom_layout_interface *layout,
                                  struct axiom_window *window);
void axiom_dwindle_recalculate_monitor(struct axiom_layout_interface *layout,
                                      struct axiom_monitor *monitor);
void axiom_dwindle_recalculate_window(struct axiom_layout_interface *layout,
                                     struct axiom_window *window);
void axiom_dwindle_resize_active_window(struct axiom_layout_interface *layout,
                                       double delta_x, double delta_y,
                                       enum axiom_rect_corner corner,
                                       struct axiom_window *window);
void axiom_dwindle_switch_windows(struct axiom_layout_interface *layout,
                                 struct axiom_window *window1,
                                 struct axiom_window *window2);
void axiom_dwindle_move_window_to(struct axiom_layout_interface *layout,
                                 struct axiom_window *window,
                                 const char *direction,
                                 bool silent);
void axiom_dwindle_alter_split_ratio(struct axiom_layout_interface *layout,
                                    struct axiom_window *window,
                                    float ratio,
                                    bool exact);
struct axiom_window *axiom_dwindle_get_next_window_candidate(struct axiom_layout_interface *layout,
                                                           struct axiom_window *current);
struct axiom_window_render_hints axiom_dwindle_request_render_hints(struct axiom_layout_interface *layout,
                                                                   struct axiom_window *window);
void axiom_dwindle_predict_size_for_new_window_tiled(struct axiom_layout_interface *layout,
                                                    int *width, int *height);

#endif /* AXIOM_DWINDLE_LAYOUT_H */
