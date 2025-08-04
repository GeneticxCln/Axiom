#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <wlr/util/box.h>
#include <wlr/types/wlr_scene.h>
#include "advanced_tiling.h"
#include "axiom.h"
#include "logging.h"

// Advanced tiling engine implementation

struct axiom_advanced_tiling_engine *axiom_advanced_tiling_create(struct axiom_server *server) {
    struct axiom_advanced_tiling_engine *engine = calloc(1, sizeof(struct axiom_advanced_tiling_engine));
    if (!engine) {
        AXIOM_LOG_ERROR("Failed to allocate advanced tiling engine");
        return NULL;
    }
    
    engine->server = server;
    engine->current_mode = AXIOM_TILING_MASTER_STACK;
    engine->master_ratio = 0.6f;
    engine->master_count = 1;
    engine->gap_size = 10;
    engine->border_width = 2;
    engine->needs_recalculation = true;
    
    // Initialize layout algorithms
    engine->algorithms[AXIOM_TILING_MASTER_STACK] = axiom_layout_algorithm_master_stack_create();
    engine->algorithms[AXIOM_TILING_GRID] = axiom_layout_algorithm_grid_create();
    engine->algorithms[AXIOM_TILING_SPIRAL] = axiom_layout_algorithm_spiral_create();
    engine->algorithms[AXIOM_TILING_BINARY_TREE] = axiom_layout_algorithm_binary_tree_create();
    
    AXIOM_LOG_INFO("Advanced tiling engine created");
    return engine;
}

void axiom_advanced_tiling_destroy(struct axiom_advanced_tiling_engine *engine) {
    if (!engine) return;
    
    // Destroy layout algorithms
    for (int i = 0; i < AXIOM_TILING_MODE_COUNT; i++) {
        if (engine->algorithms[i]) {
            axiom_layout_algorithm_destroy(engine->algorithms[i]);
        }
    }
    
    // Clear layout cache
    if (engine->cache.layouts) {
        free(engine->cache.layouts);
    }
    
    free(engine);
    AXIOM_LOG_INFO("Advanced tiling engine destroyed");
}

void axiom_advanced_tiling_set_mode(struct axiom_advanced_tiling_engine *engine, 
                                   enum axiom_advanced_tiling_mode mode) {
    if (!engine || mode >= AXIOM_TILING_MODE_COUNT) return;
    
    if (engine->current_mode != mode) {
        engine->current_mode = mode;
        engine->needs_recalculation = true;
        AXIOM_LOG_INFO("Tiling mode changed to: %s", axiom_tiling_mode_name(mode));
    }
}

void axiom_advanced_tiling_arrange_windows(struct axiom_advanced_tiling_engine *engine,
                                          struct wl_list *windows, 
                                          struct wlr_box *workspace_box) {
    if (!engine || !windows || !workspace_box) return;
    
    // Count tiled windows
    int window_count = 0;
    struct axiom_window *window;
    wl_list_for_each(window, windows, link) {
        if (!window->is_floating && !window->is_fullscreen && !window->is_maximized) {
            window_count++;
        }
    }
    
    if (window_count == 0) return;
    
    // Check if recalculation is needed
    if (!engine->needs_recalculation && 
        engine->cache.window_count == window_count &&
        engine->cache.workspace_width == workspace_box->width &&
        engine->cache.workspace_height == workspace_box->height) {
        // Use cached layouts
        axiom_advanced_tiling_apply_cached_layouts(engine, windows);
        return;
    }
    
    // Calculate new layouts
    struct axiom_tiling_context context = {
        .workspace_box = *workspace_box,
        .window_count = window_count,
        .master_ratio = engine->master_ratio,
        .master_count = engine->master_count,
        .gap_size = engine->gap_size,
        .border_width = engine->border_width
    };
    
    struct axiom_layout_algorithm *algorithm = engine->algorithms[engine->current_mode];
    if (!algorithm) {
        AXIOM_LOG_ERROR("No algorithm available for tiling mode %d", engine->current_mode);
        return;
    }
    
    // Allocate layout results
    struct axiom_advanced_window_layout *layouts = calloc(window_count, sizeof(struct axiom_advanced_window_layout));
    if (!layouts) {
        AXIOM_LOG_ERROR("Failed to allocate layout results");
        return;
    }
    
    // Calculate layouts using the algorithm
    algorithm->calculate(algorithm, &context, layouts);
    
    // Apply layouts to windows
    int layout_index = 0;
    wl_list_for_each(window, windows, link) {
        if (!window->is_floating && !window->is_fullscreen && !window->is_maximized) {
            if (layout_index < window_count) {
                axiom_advanced_tiling_apply_window_layout(engine, window, &layouts[layout_index]);
                layout_index++;
            }
        }
    }
    
    // Update cache
    axiom_advanced_tiling_update_cache(engine, layouts, window_count, workspace_box);
    
    free(layouts);
    engine->needs_recalculation = false;
}

void axiom_advanced_tiling_apply_window_layout(struct axiom_advanced_tiling_engine *engine,
                                              struct axiom_window *window,
                                              struct axiom_advanced_window_layout *layout) {
    if (!engine || !window || !layout) return;
    
    // Apply the calculated geometry
    window->x = layout->geometry.x;
    window->y = layout->geometry.y;
    window->width = layout->geometry.width;
    window->height = layout->geometry.height;
    
    // Update window's tiling state  
    window->is_tiled = true;
    
    // Configure the XDG surface
    if (window->xdg_toplevel) {
        wlr_xdg_toplevel_set_size(window->xdg_toplevel, layout->geometry.width, layout->geometry.height);
    }
    
    // Update scene tree position
    if (window->scene_tree) {
        wlr_scene_node_set_position(&window->scene_tree->node, layout->geometry.x, layout->geometry.y);
    }
    
    // Update decorations
    axiom_update_window_decorations(window);
    
    AXIOM_LOG_DEBUG("Applied layout to window: %dx%d at (%d,%d)", 
                    layout->geometry.width, layout->geometry.height,
                    layout->geometry.x, layout->geometry.y);
}

void axiom_advanced_tiling_adjust_master_ratio(struct axiom_advanced_tiling_engine *engine, float delta) {
    if (!engine) return;
    
    engine->master_ratio += delta;
    if (engine->master_ratio < 0.1f) engine->master_ratio = 0.1f;
    if (engine->master_ratio > 0.9f) engine->master_ratio = 0.9f;
    
    engine->needs_recalculation = true;
    AXIOM_LOG_INFO("Master ratio adjusted to: %.2f", engine->master_ratio);
}

void axiom_advanced_tiling_adjust_master_count(struct axiom_advanced_tiling_engine *engine, int delta) {
    if (!engine) return;
    
    engine->master_count += delta;
    if (engine->master_count < 1) engine->master_count = 1;
    if (engine->master_count > 10) engine->master_count = 10;
    
    engine->needs_recalculation = true;
    AXIOM_LOG_INFO("Master count adjusted to: %d", engine->master_count);
}

void axiom_advanced_tiling_cycle_mode(struct axiom_advanced_tiling_engine *engine) {
    if (!engine) return;
    
    enum axiom_advanced_tiling_mode next_mode = (engine->current_mode + 1) % AXIOM_TILING_MODE_COUNT;
    axiom_advanced_tiling_set_mode(engine, next_mode);
}

// Layout algorithm implementations

struct axiom_layout_algorithm *axiom_layout_algorithm_master_stack_create(void) {
    struct axiom_layout_algorithm *algorithm = calloc(1, sizeof(struct axiom_layout_algorithm));
    if (!algorithm) return NULL;
    
    algorithm->name = "Master-Stack";
    algorithm->calculate = axiom_layout_master_stack_calculate;
    algorithm->supports_master_ratio = true;
    algorithm->supports_master_count = true;
    
    return algorithm;
}

struct axiom_layout_algorithm *axiom_layout_algorithm_grid_create(void) {
    struct axiom_layout_algorithm *algorithm = calloc(1, sizeof(struct axiom_layout_algorithm));
    if (!algorithm) return NULL;
    
    algorithm->name = "Grid";
    algorithm->calculate = axiom_layout_grid_calculate;
    algorithm->supports_master_ratio = false;
    algorithm->supports_master_count = false;
    
    return algorithm;
}

struct axiom_layout_algorithm *axiom_layout_algorithm_spiral_create(void) {
    struct axiom_layout_algorithm *algorithm = calloc(1, sizeof(struct axiom_layout_algorithm));
    if (!algorithm) return NULL;
    
    algorithm->name = "Spiral";
    algorithm->calculate = axiom_layout_spiral_calculate;
    algorithm->supports_master_ratio = true;
    algorithm->supports_master_count = false;
    
    return algorithm;
}

struct axiom_layout_algorithm *axiom_layout_algorithm_binary_tree_create(void) {
    struct axiom_layout_algorithm *algorithm = calloc(1, sizeof(struct axiom_layout_algorithm));
    if (!algorithm) return NULL;
    
    algorithm->name = "Binary Tree";
    algorithm->calculate = axiom_layout_binary_tree_calculate;
    algorithm->supports_master_ratio = true;
    algorithm->supports_master_count = false;
    
    return algorithm;
}

void axiom_layout_algorithm_destroy(struct axiom_layout_algorithm *algorithm) {
    if (algorithm) {
        free(algorithm);
    }
}

// Master-Stack layout implementation
void axiom_layout_master_stack_calculate(struct axiom_layout_algorithm *algorithm,
                                         struct axiom_tiling_context *context,
                                         struct axiom_advanced_window_layout *layouts) {
    if (!algorithm || !context || !layouts || context->window_count == 0) return;
    
    int master_width = context->workspace_box.width;
    int master_height = context->workspace_box.height;
    int stack_width = 0;
    int stack_height = context->workspace_box.height;
    
    // Calculate master and stack areas
    if (context->window_count > context->master_count) {
        master_width = (int)(context->workspace_box.width * context->master_ratio);
        stack_width = context->workspace_box.width - master_width - context->gap_size;
    }
    
    // Position master windows
    int master_window_height = master_height / context->master_count;
    for (int i = 0; i < context->master_count && i < context->window_count; i++) {
        layouts[i].geometry.x = context->workspace_box.x + context->border_width;
        layouts[i].geometry.y = context->workspace_box.y + (i * master_window_height) + context->border_width;
        layouts[i].geometry.width = master_width - (2 * context->border_width);
        layouts[i].geometry.height = master_window_height - (2 * context->border_width) - context->gap_size;
        layouts[i].tiling_mode = AXIOM_TILING_MASTER_STACK;
        layouts[i].is_master = true;
    }
    
    // Position stack windows
    int stack_count = context->window_count - context->master_count;
    if (stack_count > 0) {
        int stack_window_height = stack_height / stack_count;
        for (int i = 0; i < stack_count; i++) {
            int layout_idx = context->master_count + i;
            layouts[layout_idx].geometry.x = context->workspace_box.x + master_width + context->gap_size + context->border_width;
            layouts[layout_idx].geometry.y = context->workspace_box.y + (i * stack_window_height) + context->border_width;
            layouts[layout_idx].geometry.width = stack_width - (2 * context->border_width);
            layouts[layout_idx].geometry.height = stack_window_height - (2 * context->border_width) - context->gap_size;
            layouts[layout_idx].tiling_mode = AXIOM_TILING_MASTER_STACK;
            layouts[layout_idx].is_master = false;
        }
    }
}

// Grid layout implementation
void axiom_layout_grid_calculate(struct axiom_layout_algorithm *algorithm,
                                 struct axiom_tiling_context *context,
                                 struct axiom_advanced_window_layout *layouts) {
    if (!algorithm || !context || !layouts || context->window_count == 0) return;
    
    // Calculate optimal grid dimensions
    int cols = (int)ceil(sqrt(context->window_count));
    int rows = (int)ceil((double)context->window_count / cols);
    
    int window_width = (context->workspace_box.width - (cols - 1) * context->gap_size) / cols;
    int window_height = (context->workspace_box.height - (rows - 1) * context->gap_size) / rows;
    
    for (int i = 0; i < context->window_count; i++) {
        int col = i % cols;
        int row = i / cols;
        
        layouts[i].geometry.x = context->workspace_box.x + col * (window_width + context->gap_size) + context->border_width;
        layouts[i].geometry.y = context->workspace_box.y + row * (window_height + context->gap_size) + context->border_width;
        layouts[i].geometry.width = window_width - (2 * context->border_width);
        layouts[i].geometry.height = window_height - (2 * context->border_width);
        layouts[i].tiling_mode = AXIOM_TILING_GRID;
        layouts[i].is_master = false;
    }
}

// Spiral layout implementation
void axiom_layout_spiral_calculate(struct axiom_layout_algorithm *algorithm,
                                   struct axiom_tiling_context *context,
                                   struct axiom_advanced_window_layout *layouts) {
    if (!algorithm || !context || !layouts || context->window_count == 0) return;
    
    // Start with the full workspace for the first window
    struct wlr_box current_box = context->workspace_box;
    
    for (int i = 0; i < context->window_count; i++) {
        if (i == 0) {
            // First window gets the full space (or master ratio if more windows)
            if (context->window_count > 1) {
                layouts[i].geometry.width = (int)(current_box.width * context->master_ratio) - (2 * context->border_width);
                layouts[i].geometry.height = current_box.height - (2 * context->border_width);
                layouts[i].geometry.x = current_box.x + context->border_width;
                layouts[i].geometry.y = current_box.y + context->border_width;
                layouts[i].is_master = true;
                
                // Update remaining space
                current_box.x += layouts[i].geometry.width + context->gap_size + (2 * context->border_width);
                current_box.width -= layouts[i].geometry.width + context->gap_size + (2 * context->border_width);
            } else {
                layouts[i].geometry = (struct axiom_window_geometry){
                    .x = current_box.x + context->border_width,
                    .y = current_box.y + context->border_width,
                    .width = current_box.width - (2 * context->border_width),
                    .height = current_box.height - (2 * context->border_width)
                };
                layouts[i].is_master = true;
            }
        } else {
            // Spiral pattern for remaining windows
            int remaining = context->window_count - i;
            bool split_vertically = (i % 2) == 1;
            
            if (split_vertically) {
                int split_height = current_box.height / (remaining > 1 ? 2 : 1);
                layouts[i].geometry.x = current_box.x + context->border_width;
                layouts[i].geometry.y = current_box.y + context->border_width;
                layouts[i].geometry.width = current_box.width - (2 * context->border_width);
                layouts[i].geometry.height = split_height - (2 * context->border_width) - context->gap_size;
                
                current_box.y += split_height + context->gap_size;
                current_box.height -= split_height + context->gap_size;
            } else {
                int split_width = current_box.width / (remaining > 1 ? 2 : 1);
                layouts[i].geometry.x = current_box.x + context->border_width;
                layouts[i].geometry.y = current_box.y + context->border_width;
                layouts[i].geometry.width = split_width - (2 * context->border_width) - context->gap_size;
                layouts[i].geometry.height = current_box.height - (2 * context->border_width);
                
                current_box.x += split_width + context->gap_size;
                current_box.width -= split_width + context->gap_size;
            }
            layouts[i].is_master = false;
        }
        
        layouts[i].tiling_mode = AXIOM_TILING_SPIRAL;
    }
}

// Binary tree layout implementation
void axiom_layout_binary_tree_calculate(struct axiom_layout_algorithm *algorithm,
                                        struct axiom_tiling_context *context,
                                        struct axiom_advanced_window_layout *layouts) {
    if (!algorithm || !context || !layouts || context->window_count == 0) return;
    
    // Simple binary tree implementation - each window splits the remaining space
    axiom_layout_binary_tree_recursive(context, layouts, 0, context->window_count - 1, 
                                       context->workspace_box, 0);
}

void axiom_layout_binary_tree_recursive(struct axiom_tiling_context *context,
                                        struct axiom_advanced_window_layout *layouts,
                                        int start, int end, struct wlr_box box, int depth) {
    if (start > end) return;
    
    if (start == end) {
        // Single window gets the entire box
        layouts[start].geometry.x = box.x + context->border_width;
        layouts[start].geometry.y = box.y + context->border_width;
        layouts[start].geometry.width = box.width - (2 * context->border_width);
        layouts[start].geometry.height = box.height - (2 * context->border_width);
        layouts[start].tiling_mode = AXIOM_TILING_BINARY_TREE;
        layouts[start].is_master = (start == 0);
        return;
    }
    
    // Split the box
    bool split_vertically = (depth % 2) == 0;
    int mid = (start + end) / 2;
    
    if (split_vertically) {
        int split_height = box.height / 2;
        struct wlr_box top_box = {box.x, box.y, box.width, split_height - context->gap_size / 2};
        struct wlr_box bottom_box = {box.x, box.y + split_height + context->gap_size / 2, 
                                    box.width, box.height - split_height - context->gap_size / 2};
        
        axiom_layout_binary_tree_recursive(context, layouts, start, mid, top_box, depth + 1);
        axiom_layout_binary_tree_recursive(context, layouts, mid + 1, end, bottom_box, depth + 1);
    } else {
        int split_width = box.width / 2;
        struct wlr_box left_box = {box.x, box.y, split_width - context->gap_size / 2, box.height};
        struct wlr_box right_box = {box.x + split_width + context->gap_size / 2, box.y, 
                                   box.width - split_width - context->gap_size / 2, box.height};
        
        axiom_layout_binary_tree_recursive(context, layouts, start, mid, left_box, depth + 1);
        axiom_layout_binary_tree_recursive(context, layouts, mid + 1, end, right_box, depth + 1);
    }
}

// Utility functions
const char *axiom_tiling_mode_name(enum axiom_advanced_tiling_mode mode) {
    switch (mode) {
        case AXIOM_TILING_MASTER_STACK: return "Master-Stack";
        case AXIOM_TILING_GRID: return "Grid";  
        case AXIOM_TILING_SPIRAL: return "Spiral";
        case AXIOM_TILING_BINARY_TREE: return "Binary Tree";
        default: return "Unknown";
    }
}

void axiom_advanced_tiling_update_cache(struct axiom_advanced_tiling_engine *engine,
                                        struct axiom_advanced_window_layout *layouts,
                                        int window_count,
                                        struct wlr_box *workspace_box) {
    if (!engine) return;
    
    // Free old cache
    if (engine->cache.layouts) {
        free(engine->cache.layouts);
    }
    
    // Allocate new cache
    engine->cache.layouts = calloc(window_count, sizeof(struct axiom_advanced_window_layout));
    if (engine->cache.layouts) {
        memcpy(engine->cache.layouts, layouts, window_count * sizeof(struct axiom_advanced_window_layout));
        engine->cache.window_count = window_count;
        engine->cache.workspace_width = workspace_box->width;
        engine->cache.workspace_height = workspace_box->height;
    }
}

void axiom_advanced_tiling_apply_cached_layouts(struct axiom_advanced_tiling_engine *engine,
                                                struct wl_list *windows) {
    if (!engine || !engine->cache.layouts) return;
    
    int layout_index = 0;
    struct axiom_window *window;
    wl_list_for_each(window, windows, link) {
        if (!window->is_floating && !window->is_fullscreen && !window->is_maximized) {
            if (layout_index < engine->cache.window_count) {
                axiom_advanced_tiling_apply_window_layout(engine, window, &engine->cache.layouts[layout_index]);
                layout_index++;
            }
        }
    }
}
