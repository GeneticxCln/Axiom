#ifndef ADVANCED_TILING_H
#define ADVANCED_TILING_H

#include <wayland-server-core.h>
#include <wlr/util/box.h>
#include "axiom.h"
#include "window_manager.h"

// Advanced tiling modes
enum axiom_advanced_tiling_mode {
    AXIOM_TILING_MASTER_STACK,
    AXIOM_TILING_GRID,
    AXIOM_TILING_SPIRAL,
    AXIOM_TILING_BINARY_TREE,
    AXIOM_TILING_MODE_COUNT
};

// Advanced window layout structure for tiling
struct axiom_advanced_window_layout {
    struct axiom_window_geometry geometry;
    enum axiom_advanced_tiling_mode tiling_mode;
    bool is_master;
};

// Tiling context for layout calculation
struct axiom_tiling_context {
    struct wlr_box workspace_box;
    int window_count;
    float master_ratio;
    int master_count;
    int gap_size;
    int border_width;
};

// Layout algorithm interface
struct axiom_layout_algorithm {
    const char *name;
    void (*calculate)(struct axiom_layout_algorithm *algorithm,
                     struct axiom_tiling_context *context,
                     struct axiom_advanced_window_layout *layouts);
    bool supports_master_ratio;
    bool supports_master_count;
};

// Advanced tiling engine structure
struct axiom_advanced_tiling_engine {
    struct axiom_server *server;
    enum axiom_advanced_tiling_mode current_mode;
    float master_ratio;
    int master_count;
    int gap_size;
    int border_width;
    bool needs_recalculation;
    struct axiom_layout_algorithm *algorithms[AXIOM_TILING_MODE_COUNT];
    struct {
        struct axiom_advanced_window_layout *layouts;
        int window_count;
        int workspace_width;
        int workspace_height;
    } cache;
};

// Tiling engine lifecycle
struct axiom_advanced_tiling_engine *axiom_advanced_tiling_create(struct axiom_server *server);
void axiom_advanced_tiling_destroy(struct axiom_advanced_tiling_engine *engine);

// Tiling engine operations
void axiom_advanced_tiling_set_mode(struct axiom_advanced_tiling_engine *engine, 
                                   enum axiom_advanced_tiling_mode mode);
void axiom_advanced_tiling_arrange_windows(struct axiom_advanced_tiling_engine *engine,
                                          struct wl_list *windows, 
                                          struct wlr_box *workspace_box);
void axiom_advanced_tiling_apply_window_layout(struct axiom_advanced_tiling_engine *engine,
                                              struct axiom_window *window,
                                              struct axiom_advanced_window_layout *layout);
void axiom_advanced_tiling_adjust_master_ratio(struct axiom_advanced_tiling_engine *engine, float delta);
void axiom_advanced_tiling_adjust_master_count(struct axiom_advanced_tiling_engine *engine, int delta);
void axiom_advanced_tiling_set_gap_size(struct axiom_advanced_tiling_engine *engine, int gap_size);
void axiom_advanced_tiling_cycle_mode(struct axiom_advanced_tiling_engine *engine);

// Utility functions
const char *axiom_tiling_mode_name(enum axiom_advanced_tiling_mode mode);
void axiom_advanced_tiling_update_cache(struct axiom_advanced_tiling_engine *engine,
                                        struct axiom_advanced_window_layout *layouts,
                                        int window_count,
                                        struct wlr_box *workspace_box);
void axiom_advanced_tiling_apply_cached_layouts(struct axiom_advanced_tiling_engine *engine,
                                                struct wl_list *windows);

// Layout algorithm creation functions
struct axiom_layout_algorithm *axiom_layout_algorithm_master_stack_create(void);
struct axiom_layout_algorithm *axiom_layout_algorithm_grid_create(void);
struct axiom_layout_algorithm *axiom_layout_algorithm_spiral_create(void);
struct axiom_layout_algorithm *axiom_layout_algorithm_binary_tree_create(void);
void axiom_layout_algorithm_destroy(struct axiom_layout_algorithm *algorithm);

// Layout calculation functions
void axiom_layout_master_stack_calculate(struct axiom_layout_algorithm *algorithm,
                                         struct axiom_tiling_context *context,
                                         struct axiom_advanced_window_layout *layouts);
void axiom_layout_grid_calculate(struct axiom_layout_algorithm *algorithm,
                                 struct axiom_tiling_context *context,
                                 struct axiom_advanced_window_layout *layouts);
void axiom_layout_spiral_calculate(struct axiom_layout_algorithm *algorithm,
                                   struct axiom_tiling_context *context,
                                   struct axiom_advanced_window_layout *layouts);
void axiom_layout_binary_tree_calculate(struct axiom_layout_algorithm *algorithm,
                                        struct axiom_tiling_context *context,
                                        struct axiom_advanced_window_layout *layouts);
void axiom_layout_binary_tree_recursive(struct axiom_tiling_context *context,
                                        struct axiom_advanced_window_layout *layouts,
                                        int start, int end, struct wlr_box box, int depth);

#endif /* ADVANCED_TILING_H */

