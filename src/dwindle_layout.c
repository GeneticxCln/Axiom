#include "dwindle_layout.h"
#include "axiom.h"
#include "logging.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Dwindle layout creation
struct axiom_layout_interface *axiom_dwindle_layout_create(void) {
    AXIOM_LOG(AXIOM_LOG_INFO, "Creating dwindle layout");
    
    struct axiom_dwindle_layout *layout = calloc(1, sizeof(struct axiom_dwindle_layout));
    if (!layout) {
        AXIOM_LOG(AXIOM_LOG_ERROR, "Failed to allocate dwindle layout");
        return NULL;
    }
    
    // Initialize base interface
    layout->base.name = "dwindle";
    layout->base.type = AXIOM_LAYOUT_DWINDLE;
    
    // Set function pointers
    layout->base.on_enable = axiom_dwindle_on_enable;
    layout->base.on_disable = axiom_dwindle_on_disable;
    layout->base.on_window_created = axiom_dwindle_on_window_created;
    layout->base.on_window_created_tiling = axiom_dwindle_on_window_created_tiling;
    layout->base.on_window_removed = axiom_dwindle_on_window_removed;
    layout->base.on_window_removed_tiling = axiom_dwindle_on_window_removed_tiling;
    layout->base.is_window_tiled = axiom_dwindle_is_window_tiled;
    layout->base.recalculate_monitor = axiom_dwindle_recalculate_monitor;
    layout->base.recalculate_window = axiom_dwindle_recalculate_window;
    layout->base.resize_active_window = axiom_dwindle_resize_active_window;
    layout->base.switch_windows = axiom_dwindle_switch_windows;
    layout->base.move_window_to = axiom_dwindle_move_window_to;
    layout->base.alter_split_ratio = axiom_dwindle_alter_split_ratio;
    layout->base.get_next_window_candidate = axiom_dwindle_get_next_window_candidate;
    layout->base.request_render_hints = axiom_dwindle_request_render_hints;
    layout->base.predict_size_for_new_window_tiled = axiom_dwindle_predict_size_for_new_window_tiled;
    
    // Initialize workspace data list
    wl_list_init(&layout->workspace_data);
    
    // Set default configuration
    axiom_dwindle_reset_config_to_defaults(&layout->global_config);
    
    // Initialize statistics
    memset(&layout->stats, 0, sizeof(layout->stats));
    
    layout->base.layout_data = layout;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Dwindle layout created successfully");
    return &layout->base;
}

void axiom_dwindle_layout_destroy(struct axiom_dwindle_layout *layout) {
    if (!layout) return;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Destroying dwindle layout");
    
    // Clean up workspace data
    struct axiom_dwindle_layout_data *data, *tmp;
    wl_list_for_each_safe(data, tmp, &layout->workspace_data, link) {
        axiom_dwindle_destroy_workspace_data(data);
    }
    
    free(layout);
}

// Workspace data management
struct axiom_dwindle_layout_data *axiom_dwindle_get_workspace_data(struct axiom_dwindle_layout *layout,
                                                                  struct axiom_workspace *workspace) {
    if (!layout || !workspace) return NULL;
    
    struct axiom_dwindle_layout_data *data;
    wl_list_for_each(data, &layout->workspace_data, link) {
        if (data->workspace == workspace) {
            return data;
        }
    }
    
    return NULL;
}

struct axiom_dwindle_layout_data *axiom_dwindle_create_workspace_data(struct axiom_dwindle_layout *layout,
                                                                     struct axiom_workspace *workspace) {
    if (!layout || !workspace) return NULL;
    
    AXIOM_LOG(AXIOM_LOG_DEBUG, "Creating dwindle data for workspace %d", workspace->id);
    
    struct axiom_dwindle_layout_data *data = calloc(1, sizeof(struct axiom_dwindle_layout_data));
    if (!data) {
        AXIOM_LOG(AXIOM_LOG_ERROR, "Failed to allocate dwindle workspace data");
        return NULL;
    }
    
    data->workspace = workspace;
    data->root = NULL;
    wl_list_init(&data->all_nodes);
    data->config = layout->global_config;
    
    wl_list_insert(&layout->workspace_data, &data->link);
    
    return data;
}

void axiom_dwindle_destroy_workspace_data(struct axiom_dwindle_layout_data *data) {
    if (!data) return;
    
    AXIOM_LOG(AXIOM_LOG_DEBUG, "Destroying dwindle workspace data");
    
    // Clean up all nodes
    struct axiom_dwindle_node *node, *tmp;
    wl_list_for_each_safe(node, tmp, &data->all_nodes, link) {
        axiom_dwindle_node_destroy(node);
    }
    
    wl_list_remove(&data->link);
    free(data);
}

// Node management
struct axiom_dwindle_node *axiom_dwindle_node_create(enum axiom_dwindle_node_type type) {
    struct axiom_dwindle_node *node = calloc(1, sizeof(struct axiom_dwindle_node));
    if (!node) return NULL;
    
    node->type = type;
    node->split_ratio = 0.5f; // Default 50/50 split
    node->visible = true;
    
    // Set default layout hints
    node->hints.min_split_ratio = 0.1f;
    node->hints.max_split_ratio = 0.9f;
    
    // Initialize animation data
    node->animation.animation_duration = 250; // 250ms default
    
    return node;
}

void axiom_dwindle_node_destroy(struct axiom_dwindle_node *node) {
    if (!node) return;
    
    // Recursively destroy children
    if (node->child1) {
        axiom_dwindle_node_destroy(node->child1);
    }
    if (node->child2) {
        axiom_dwindle_node_destroy(node->child2);
    }
    
    wl_list_remove(&node->link);
    free(node);
}

struct axiom_dwindle_node *axiom_dwindle_node_find_window(struct axiom_dwindle_node *root,
                                                         struct axiom_window *window) {
    if (!root || !window) return NULL;
    
    if (root->type == AXIOM_DWINDLE_NODE_WINDOW && root->window == window) {
        return root;
    }
    
    if (root->type == AXIOM_DWINDLE_NODE_CONTAINER) {
        struct axiom_dwindle_node *result = axiom_dwindle_node_find_window(root->child1, window);
        if (result) return result;
        
        return axiom_dwindle_node_find_window(root->child2, window);
    }
    
    return NULL;
}

// Tree operations
void axiom_dwindle_insert_window(struct axiom_dwindle_layout_data *data,
                                struct axiom_window *window,
                                enum axiom_direction direction) {
    if (!data || !window) return;
    
    AXIOM_LOG(AXIOM_LOG_DEBUG, "Inserting window into dwindle tree");
    
    struct axiom_dwindle_node *new_node = axiom_dwindle_node_create(AXIOM_DWINDLE_NODE_WINDOW);
    if (!new_node) {
        AXIOM_LOG(AXIOM_LOG_ERROR, "Failed to create dwindle node for window");
        return;
    }
    
    new_node->window = window;
    new_node->workspace = data->workspace;
    wl_list_insert(&data->all_nodes, &new_node->link);
    data->window_count++;
    
    if (!data->root) {
        // First window becomes root
        data->root = new_node;
        data->focused_node = new_node;
        AXIOM_LOG(AXIOM_LOG_DEBUG, "Window becomes root of dwindle tree");
    } else {
        // Find focused node or use root
        struct axiom_dwindle_node *target = data->focused_node ? data->focused_node : data->root;
        
        // Determine split direction
        enum axiom_dwindle_split_direction split_dir = 
            axiom_dwindle_smart_split_direction(target, window);
        
        // Create split
        axiom_dwindle_split_node(target, window, split_dir);
        data->layout_changed = true;
    }
    
    // Recalculate layout
    axiom_dwindle_recalculate_tree(data);
    layout->stats.total_splits++;
}

void axiom_dwindle_remove_window(struct axiom_dwindle_layout_data *data,
                                struct axiom_window *window) {
    if (!data || !window) return;
    
    AXIOM_LOG(AXIOM_LOG_DEBUG, "Removing window from dwindle tree");
    
    struct axiom_dwindle_node *node = axiom_dwindle_node_find_window(data->root, window);
    if (!node) {
        AXIOM_LOG(AXIOM_LOG_WARN, "Window not found in dwindle tree");
        return;
    }
    
    data->window_count--;
    
    if (node == data->root && !node->parent) {
        // Removing root node
        data->root = NULL;
        data->focused_node = NULL;
    } else {
        // Remove node and merge parent
        struct axiom_dwindle_node *parent = node->parent;
        if (parent) {
            axiom_dwindle_merge_nodes(parent);
        }
    }
    
    axiom_dwindle_node_destroy(node);
    data->layout_changed = true;
    
    // Recalculate layout
    if (data->root) {
        axiom_dwindle_recalculate_tree(data);
    }
    
    layout->stats.total_merges++;
}

void axiom_dwindle_split_node(struct axiom_dwindle_node *node,
                             struct axiom_window *new_window,
                             enum axiom_dwindle_split_direction direction) {
    if (!node || !new_window) return;
    
    AXIOM_LOG(AXIOM_LOG_DEBUG, "Splitting dwindle node");
    
    // Create new container node
    struct axiom_dwindle_node *container = axiom_dwindle_node_create(AXIOM_DWINDLE_NODE_CONTAINER);
    if (!container) return;
    
    container->split_direction = direction;
    container->workspace = node->workspace;
    container->parent = node->parent;
    
    // Create new window node
    struct axiom_dwindle_node *new_node = axiom_dwindle_node_create(AXIOM_DWINDLE_NODE_WINDOW);
    if (!new_node) {
        axiom_dwindle_node_destroy(container);
        return;
    }
    
    new_node->window = new_window;
    new_node->workspace = node->workspace;
    
    // Update parent relationships
    if (node->parent) {
        if (node->parent->child1 == node) {
            node->parent->child1 = container;
        } else {
            node->parent->child2 = container;
        }
    }
    
    // Set up container children
    container->child1 = node;
    container->child2 = new_node;
    node->parent = container;
    new_node->parent = container;
    
    // Update workspace root if necessary
    struct axiom_dwindle_layout_data *data = NULL; // Get from context
    if (data && data->root == node) {
        data->root = container;
    }
}

void axiom_dwindle_merge_nodes(struct axiom_dwindle_node *parent) {
    if (!parent || parent->type != AXIOM_DWINDLE_NODE_CONTAINER) return;
    
    AXIOM_LOG(AXIOM_LOG_DEBUG, "Merging dwindle nodes");
    
    struct axiom_dwindle_node *remaining_child = NULL;
    
    // Determine which child remains
    if (parent->child1 && !parent->child2) {
        remaining_child = parent->child1;
    } else if (parent->child2 && !parent->child1) {
        remaining_child = parent->child2;
    }
    
    if (!remaining_child) return;
    
    // Update parent's parent
    if (parent->parent) {
        remaining_child->parent = parent->parent;
        if (parent->parent->child1 == parent) {
            parent->parent->child1 = remaining_child;
        } else {
            parent->parent->child2 = remaining_child;
        }
    } else {
        // Parent was root, remaining child becomes new root
        remaining_child->parent = NULL;
        // Update workspace root - would need workspace data context
    }
    
    // Clean up parent node
    parent->child1 = NULL;
    parent->child2 = NULL;
    axiom_dwindle_node_destroy(parent);
}

// Layout calculations
void axiom_dwindle_recalculate_tree(struct axiom_dwindle_layout_data *data) {
    if (!data || !data->root) return;
    
    AXIOM_LOG(AXIOM_LOG_DEBUG, "Recalculating dwindle tree");
    
    uint64_t start_time = time(NULL);
    
    // Get workspace geometry from monitor
    int workspace_x = 0, workspace_y = 0;
    int workspace_width = 1920, workspace_height = 1080; // Default, should get from monitor
    
    if (data->workspace && data->workspace->monitor) {
        workspace_width = data->workspace->monitor->width;
        workspace_height = data->workspace->monitor->height;
        
        // Account for reserved areas
        workspace_x += data->workspace->monitor->reserved_area.left;
        workspace_y += data->workspace->monitor->reserved_area.top;
        workspace_width -= data->workspace->monitor->reserved_area.left + 
                          data->workspace->monitor->reserved_area.right;
        workspace_height -= data->workspace->monitor->reserved_area.top + 
                           data->workspace->monitor->reserved_area.bottom;
    }
    
    // Account for gaps
    if (data->config.no_gaps_when_only && data->window_count == 1) {
        // No gaps for single window
    } else {
        workspace_x += data->config.col_gap_size;
        workspace_y += data->config.row_gap_size;
        workspace_width -= 2 * data->config.col_gap_size;
        workspace_height -= 2 * data->config.row_gap_size;
    }
    
    // Calculate geometry for all nodes
    axiom_dwindle_calculate_node_geometry(data->root, 
                                         workspace_x, workspace_y,
                                         workspace_width, workspace_height);
    
    // Apply geometry to windows
    axiom_dwindle_apply_node_geometry(data->root);
    
    // Update statistics
    data->last_recalc_time = time(NULL);
    data->recalc_count++;
    
    struct axiom_dwindle_layout *layout = 
        (struct axiom_dwindle_layout *)data->workspace; // Get layout from context
    if (layout) {
        uint64_t recalc_time = time(NULL) - start_time;
        layout->stats.total_recalc_time += recalc_time;
        layout->stats.total_recalcs++;
        layout->stats.avg_recalc_time = 
            (float)layout->stats.total_recalc_time / layout->stats.total_recalcs;
    }
    
    data->layout_changed = false;
}

void axiom_dwindle_calculate_node_geometry(struct axiom_dwindle_node *node,
                                          int x, int y, int width, int height) {
    if (!node) return;
    
    node->x = x;
    node->y = y;
    node->width = width;
    node->height = height;
    
    if (node->type == AXIOM_DWINDLE_NODE_WINDOW) {
        // Leaf node - geometry is final
        return;
    }
    
    // Container node - split between children
    if (!node->child1 || !node->child2) return;
    
    int child1_x = x, child1_y = y;
    int child1_width, child1_height;
    int child2_x, child2_y;
    int child2_width, child2_height;
    
    if (node->split_direction == AXIOM_DWINDLE_SPLIT_HORIZONTAL) {
        // Horizontal split (left/right)
        child1_width = (int)(width * node->split_ratio);
        child1_height = height;
        
        child2_x = x + child1_width;
        child2_y = y;
        child2_width = width - child1_width;
        child2_height = height;
    } else {
        // Vertical split (top/bottom)
        child1_width = width;
        child1_height = (int)(height * node->split_ratio);
        
        child2_x = x;
        child2_y = y + child1_height;
        child2_width = width;
        child2_height = height - child1_height;
    }
    
    // Recursively calculate children
    axiom_dwindle_calculate_node_geometry(node->child1, 
                                         child1_x, child1_y, 
                                         child1_width, child1_height);
    axiom_dwindle_calculate_node_geometry(node->child2,
                                         child2_x, child2_y,
                                         child2_width, child2_height);
}

void axiom_dwindle_apply_node_geometry(struct axiom_dwindle_node *node) {
    if (!node) return;
    
    if (node->type == AXIOM_DWINDLE_NODE_WINDOW && node->window) {
        // Apply geometry to window
        AXIOM_LOG(AXIOM_LOG_DEBUG, "Applying geometry to window: %dx%d@%d,%d",
                  node->width, node->height, node->x, node->y);
        
        // Set window position and size
        node->window->x = node->x;
        node->window->y = node->y;
        node->window->width = node->width;
        node->window->height = node->height;
        
        // Update window's scene node position
        if (node->window->scene_tree) {
            wlr_scene_node_set_position(&node->window->scene_tree->node, node->x, node->y);
        }
        
        // Send configure to client
        if (node->window->xdg_toplevel) {
            wlr_xdg_toplevel_set_size(node->window->xdg_toplevel, node->width, node->height);
        }
    } else if (node->type == AXIOM_DWINDLE_NODE_CONTAINER) {
        // Recursively apply to children
        axiom_dwindle_apply_node_geometry(node->child1);
        axiom_dwindle_apply_node_geometry(node->child2);
    }
}

// Smart split direction selection
enum axiom_dwindle_split_direction axiom_dwindle_smart_split_direction(struct axiom_dwindle_node *node,
                                                                      struct axiom_window *new_window) {
    if (!node) return AXIOM_DWINDLE_SPLIT_HORIZONTAL;
    
    // If forced direction is set, use it
    if (node->hints.force_split_direction) {
        return node->hints.forced_direction;
    }
    
    // Smart split based on node dimensions
    if (node->width > node->height) {
        return AXIOM_DWINDLE_SPLIT_HORIZONTAL; // Split wider dimension
    } else {
        return AXIOM_DWINDLE_SPLIT_VERTICAL;   // Split taller dimension
    }
}

// Split ratio management
void axiom_dwindle_set_split_ratio(struct axiom_dwindle_node *node, float ratio) {
    if (!node || node->type != AXIOM_DWINDLE_NODE_CONTAINER) return;
    
    // Clamp ratio to valid range
    ratio = fmaxf(node->hints.min_split_ratio, 
                  fminf(node->hints.max_split_ratio, ratio));
    
    node->split_ratio = ratio;
    node->recalc_needed = true;
}

float axiom_dwindle_get_split_ratio(struct axiom_dwindle_node *node) {
    return node && node->type == AXIOM_DWINDLE_NODE_CONTAINER ? node->split_ratio : 0.5f;
}

void axiom_dwindle_adjust_split_ratio(struct axiom_dwindle_node *node, float delta, bool exact) {
    if (!node || node->type != AXIOM_DWINDLE_NODE_CONTAINER) return;
    
    float new_ratio;
    if (exact) {
        new_ratio = delta;
    } else {
        new_ratio = node->split_ratio + delta;
    }
    
    axiom_dwindle_set_split_ratio(node, new_ratio);
}

// Configuration functions
void axiom_dwindle_reset_config_to_defaults(struct axiom_dwindle_config *config) {
    if (!config) return;
    
    config->smart_split = true;
    config->force_split = false;
    config->pseudotile = false;
    config->preserve_split = true;
    config->smart_resizing = true;
    config->use_active_for_splits = true;
    
    config->default_split_ratio = 0.5f;
    config->min_split_ratio = 0.1f;
    config->max_split_ratio = 0.9f;
    
    config->new_is_master = false;
    config->new_on_top = true;
    config->col_maximize_one = false;
    config->special_scale_factor = false;
    
    config->no_gaps_when_only = true;
    config->col_gap_size = 5;
    config->row_gap_size = 5;
    
    config->animate_splits = true;
    config->split_animation_duration = 250;
    config->split_animation_curve = strdup("ease_out_cubic");
}

// Layout interface implementations
void axiom_dwindle_on_enable(struct axiom_layout_interface *layout) {
    AXIOM_LOG(AXIOM_LOG_INFO, "Dwindle layout enabled");
}

void axiom_dwindle_on_disable(struct axiom_layout_interface *layout) {
    AXIOM_LOG(AXIOM_LOG_INFO, "Dwindle layout disabled");
}

void axiom_dwindle_on_window_created(struct axiom_layout_interface *layout,
                                    struct axiom_window *window,
                                    enum axiom_direction direction) {
    if (window->floating) {
        // Handle floating window creation
        return;
    }
    
    axiom_dwindle_on_window_created_tiling(layout, window, direction);
}

void axiom_dwindle_on_window_created_tiling(struct axiom_layout_interface *layout,
                                           struct axiom_window *window,
                                           enum axiom_direction direction) {
    struct axiom_dwindle_layout *dwindle = (struct axiom_dwindle_layout *)layout->layout_data;
    if (!dwindle) return;
    
    // Get or create workspace data
    struct axiom_dwindle_layout_data *data = 
        axiom_dwindle_get_workspace_data(dwindle, window->workspace);
    if (!data) {
        data = axiom_dwindle_create_workspace_data(dwindle, window->workspace);
        if (!data) return;
    }
    
    axiom_dwindle_insert_window(data, window, direction);
}

void axiom_dwindle_on_window_removed(struct axiom_layout_interface *layout,
                                    struct axiom_window *window) {
    if (window->floating) {
        return;
    }
    
    axiom_dwindle_on_window_removed_tiling(layout, window);
}

void axiom_dwindle_on_window_removed_tiling(struct axiom_layout_interface *layout,
                                           struct axiom_window *window) {
    struct axiom_dwindle_layout *dwindle = (struct axiom_dwindle_layout *)layout->layout_data;
    if (!dwindle) return;
    
    struct axiom_dwindle_layout_data *data = 
        axiom_dwindle_get_workspace_data(dwindle, window->workspace);
    if (!data) return;
    
    axiom_dwindle_remove_window(data, window);
}

bool axiom_dwindle_is_window_tiled(struct axiom_layout_interface *layout,
                                  struct axiom_window *window) {
    if (!window) return false;
    return !window->floating;
}

void axiom_dwindle_recalculate_monitor(struct axiom_layout_interface *layout,
                                      struct axiom_monitor *monitor) {
    struct axiom_dwindle_layout *dwindle = (struct axiom_dwindle_layout *)layout->layout_data;
    if (!dwindle || !monitor) return;
    
    AXIOM_LOG(AXIOM_LOG_DEBUG, "Recalculating dwindle layout for monitor");
    
    // Recalculate all workspaces on this monitor
    struct axiom_dwindle_layout_data *data;
    wl_list_for_each(data, &dwindle->workspace_data, link) {
        if (data->workspace && data->workspace->monitor == monitor) {
            axiom_dwindle_recalculate_tree(data);
        }
    }
}

void axiom_dwindle_recalculate_window(struct axiom_layout_interface *layout,
                                     struct axiom_window *window) {
    struct axiom_dwindle_layout *dwindle = (struct axiom_dwindle_layout *)layout->layout_data;
    if (!dwindle || !window) return;
    
    struct axiom_dwindle_layout_data *data = 
        axiom_dwindle_get_workspace_data(dwindle, window->workspace);
    if (!data) return;
    
    axiom_dwindle_recalculate_tree(data);
}

void axiom_dwindle_resize_active_window(struct axiom_layout_interface *layout,
                                       double delta_x, double delta_y,
                                       enum axiom_rect_corner corner,
                                       struct axiom_window *window) {
    struct axiom_dwindle_layout *dwindle = (struct axiom_dwindle_layout *)layout->layout_data;
    if (!dwindle || !window) return;
    
    struct axiom_dwindle_layout_data *data = 
        axiom_dwindle_get_workspace_data(dwindle, window->workspace);
    if (!data) return;
    
    axiom_dwindle_resize_window(data, window, delta_x, delta_y, corner);
}

void axiom_dwindle_resize_window(struct axiom_dwindle_layout_data *data,
                                struct axiom_window *window,
                                double delta_x, double delta_y,
                                enum axiom_rect_corner corner) {
    if (!data || !window) return;
    
    struct axiom_dwindle_node *node = axiom_dwindle_node_find_window(data->root, window);
    if (!node || !node->parent) return;
    
    struct axiom_dwindle_node *parent = node->parent;
    
    // Calculate ratio adjustment based on resize direction
    float ratio_delta = 0.0f;
    
    if (parent->split_direction == AXIOM_DWINDLE_SPLIT_HORIZONTAL) {
        // Horizontal split - use delta_x
        ratio_delta = delta_x / (float)parent->width;
        if (parent->child2 == node) {
            ratio_delta = -ratio_delta; // Invert for right child
        }
    } else {
        // Vertical split - use delta_y
        ratio_delta = delta_y / (float)parent->height;
        if (parent->child2 == node) {
            ratio_delta = -ratio_delta; // Invert for bottom child
        }
    }
    
    axiom_dwindle_adjust_split_ratio(parent, ratio_delta, false);
    axiom_dwindle_recalculate_tree(data);
}

void axiom_dwindle_switch_windows(struct axiom_layout_interface *layout,
                                 struct axiom_window *window1,
                                 struct axiom_window *window2) {
    struct axiom_dwindle_layout *dwindle = (struct axiom_dwindle_layout *)layout->layout_data;
    if (!dwindle || !window1 || !window2) return;
    
    // Find workspace data
    struct axiom_dwindle_layout_data *data = 
        axiom_dwindle_get_workspace_data(dwindle, window1->workspace);
    if (!data) return;
    
    axiom_dwindle_swap_windows(data, window1, window2);
}

void axiom_dwindle_swap_windows(struct axiom_dwindle_layout_data *data,
                               struct axiom_window *window1,
                               struct axiom_window *window2) {
    if (!data || !window1 || !window2) return;
    
    struct axiom_dwindle_node *node1 = axiom_dwindle_node_find_window(data->root, window1);
    struct axiom_dwindle_node *node2 = axiom_dwindle_node_find_window(data->root, window2);
    
    if (!node1 || !node2) return;
    
    // Swap window references
    node1->window = window2;
    node2->window = window1;
    
    // Update window workspace references
    window1->workspace = data->workspace;
    window2->workspace = data->workspace;
    
    axiom_dwindle_recalculate_tree(data);
}

void axiom_dwindle_move_window_to(struct axiom_layout_interface *layout,
                                 struct axiom_window *window,
                                 const char *direction,
                                 bool silent) {
    // Implementation for directional window movement
    // This would move windows in the specified direction within the tree
}

void axiom_dwindle_alter_split_ratio(struct axiom_layout_interface *layout,
                                    struct axiom_window *window,
                                    float ratio,
                                    bool exact) {
    struct axiom_dwindle_layout *dwindle = (struct axiom_dwindle_layout *)layout->layout_data;
    if (!dwindle || !window) return;
    
    struct axiom_dwindle_layout_data *data = 
        axiom_dwindle_get_workspace_data(dwindle, window->workspace);
    if (!data) return;
    
    struct axiom_dwindle_node *node = axiom_dwindle_node_find_window(data->root, window);
    if (!node || !node->parent) return;
    
    axiom_dwindle_adjust_split_ratio(node->parent, ratio, exact);
    axiom_dwindle_recalculate_tree(data);
}

struct axiom_window *axiom_dwindle_get_next_window_candidate(struct axiom_layout_interface *layout,
                                                           struct axiom_window *current) {
    struct axiom_dwindle_layout *dwindle = (struct axiom_dwindle_layout *)layout->layout_data;
    if (!dwindle || !current) return NULL;
    
    struct axiom_dwindle_layout_data *data = 
        axiom_dwindle_get_workspace_data(dwindle, current->workspace);
    if (!data) return NULL;
    
    return axiom_dwindle_get_next_window_cyclic(data, current, false);
}

struct axiom_window *axiom_dwindle_get_next_window_cyclic(struct axiom_dwindle_layout_data *data,
                                                         struct axiom_window *current,
                                                         bool reverse) {
    if (!data || !current) return NULL;
    
    // Simple implementation - get next window in list
    // A more sophisticated implementation would traverse the tree
    
    bool found_current = false;
    struct axiom_window *first_window = NULL;
    struct axiom_window *next_window = NULL;
    
    struct axiom_window *window;
    wl_list_for_each(window, &data->workspace->windows, workspace_link) {
        if (!window->floating) {
            if (!first_window) first_window = window;
            
            if (found_current) {
                next_window = window;
                break;
            }
            
            if (window == current) {
                found_current = true;
                if (reverse && first_window != current) {
                    // Return previous window (first_window in this simple case)
                    return first_window;
                }
            }
        }
    }
    
    return next_window ? next_window : first_window;
}

struct axiom_window_render_hints axiom_dwindle_request_render_hints(struct axiom_layout_interface *layout,
                                                                   struct axiom_window *window) {
    struct axiom_window_render_hints hints = {0};
    
    // Default render hints for dwindle layout
    hints.is_border_gradient = false;
    hints.force_opaque = false;
    hints.disable_rounded_corners = false;
    hints.custom_shadow = false;
    hints.alpha_override = 1.0f;
    
    return hints;
}

void axiom_dwindle_predict_size_for_new_window_tiled(struct axiom_layout_interface *layout,
                                                    int *width, int *height) {
    // Predict size based on current layout state
    // This is a simplified implementation
    if (width) *width = 800;
    if (height) *height = 600;
}

// Tree traversal utilities
void axiom_dwindle_traverse_tree(struct axiom_dwindle_node *node,
                                void (*callback)(struct axiom_dwindle_node *node, void *user_data),
                                void *user_data) {
    if (!node || !callback) return;
    
    callback(node, user_data);
    
    if (node->type == AXIOM_DWINDLE_NODE_CONTAINER) {
        if (node->child1) {
            axiom_dwindle_traverse_tree(node->child1, callback, user_data);
        }
        if (node->child2) {
            axiom_dwindle_traverse_tree(node->child2, callback, user_data);
        }
    }
}

int axiom_dwindle_count_windows_in_tree(struct axiom_dwindle_node *node) {
    if (!node) return 0;
    
    if (node->type == AXIOM_DWINDLE_NODE_WINDOW) {
        return 1;
    }
    
    return axiom_dwindle_count_windows_in_tree(node->child1) +
           axiom_dwindle_count_windows_in_tree(node->child2);
}

int axiom_dwindle_get_tree_depth(struct axiom_dwindle_node *node) {
    if (!node) return 0;
    
    if (node->type == AXIOM_DWINDLE_NODE_WINDOW) {
        return 1;
    }
    
    int depth1 = axiom_dwindle_get_tree_depth(node->child1);
    int depth2 = axiom_dwindle_get_tree_depth(node->child2);
    
    return 1 + (depth1 > depth2 ? depth1 : depth2);
}

// Debug functions
void axiom_dwindle_print_tree(struct axiom_dwindle_node *node, int depth) {
    if (!node) return;
    
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    
    if (node->type == AXIOM_DWINDLE_NODE_WINDOW) {
        printf("Window: %p (%dx%d@%d,%d)\n", (void*)node->window,
               node->width, node->height, node->x, node->y);
    } else {
        printf("Container: %s split %.2f (%dx%d@%d,%d)\n",
               node->split_direction == AXIOM_DWINDLE_SPLIT_HORIZONTAL ? "H" : "V",
               node->split_ratio, node->width, node->height, node->x, node->y);
        
        axiom_dwindle_print_tree(node->child1, depth + 1);
        axiom_dwindle_print_tree(node->child2, depth + 1);
    }
}

void axiom_dwindle_validate_tree(struct axiom_dwindle_node *node) {
    if (!node) return;
    
    if (node->type == AXIOM_DWINDLE_NODE_CONTAINER) {
        if (!node->child1 || !node->child2) {
            AXIOM_LOG(AXIOM_LOG_ERROR, "Container node missing children");
            return;
        }
        
        if (node->child1->parent != node || node->child2->parent != node) {
            AXIOM_LOG(AXIOM_LOG_ERROR, "Invalid parent relationships in tree");
        }
        
        axiom_dwindle_validate_tree(node->child1);
        axiom_dwindle_validate_tree(node->child2);
    }
}

void axiom_dwindle_print_layout_stats(struct axiom_dwindle_layout *layout) {
    if (!layout) return;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Dwindle Layout Statistics:");
    AXIOM_LOG(AXIOM_LOG_INFO, "  Total recalculations: %u", layout->stats.total_recalcs);
    AXIOM_LOG(AXIOM_LOG_INFO, "  Average recalc time: %.2fms", layout->stats.avg_recalc_time);
    AXIOM_LOG(AXIOM_LOG_INFO, "  Total splits: %u", layout->stats.total_splits);
    AXIOM_LOG(AXIOM_LOG_INFO, "  Total merges: %u", layout->stats.total_merges);
}
