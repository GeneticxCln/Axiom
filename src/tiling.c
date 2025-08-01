#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "axiom.h"

// Tiling layout types
enum axiom_layout_type {
    AXIOM_LAYOUT_GRID,
    AXIOM_LAYOUT_MASTER_STACK,
    AXIOM_LAYOUT_SPIRAL,
    AXIOM_LAYOUT_FLOATING
};

static enum axiom_layout_type current_layout = AXIOM_LAYOUT_MASTER_STACK;
static float master_ratio = 0.6f; // 60% for master window

void axiom_set_layout(enum axiom_layout_type layout) {
    current_layout = layout;
    AXIOM_LOG_INFO("Layout changed to: %d", layout);
}

enum axiom_layout_type axiom_get_layout(void) {
    return current_layout;
}

void axiom_adjust_master_ratio(float delta) {
    master_ratio += delta;
    if (master_ratio < 0.2f) master_ratio = 0.2f;
    if (master_ratio > 0.8f) master_ratio = 0.8f;
    AXIOM_LOG_INFO("Master ratio adjusted to: %.2f", master_ratio);
}

// Master-stack layout: one large window on left, others stacked on right
static void calculate_master_stack_layout(struct axiom_server *server, int index, 
                                         int *x, int *y, int *width, int *height) {
    if (server->workspace_width <= 0 || server->workspace_height <= 0) {
        *x = *y = 0;
        *width = 800;
        *height = 600;
        return;
    }
    
    int window_count = server->window_count;
    int gap = server->config->gap_size;
    int title_height = 24; // Account for title bar
    
    if (window_count == 1) {
        // Single window takes full space
        *x = gap;
        *y = gap + title_height;
        *width = server->workspace_width - (2 * gap);
        *height = server->workspace_height - (2 * gap) - title_height;
    } else {
        int master_width = (server->workspace_width - (3 * gap)) * master_ratio;
        int stack_width = server->workspace_width - master_width - (3 * gap);
        
        if (index == 0) {
            // Master window (first window)
            *x = gap;
            *y = gap + title_height;
            *width = master_width;
            *height = server->workspace_height - (2 * gap) - title_height;
        } else {
            // Stack windows (remaining windows)
            int stack_count = window_count - 1;
            int stack_height = (server->workspace_height - ((stack_count + 1) * gap) - title_height) / stack_count;
            
            *x = master_width + (2 * gap);
            *y = gap + title_height + (index - 1) * (stack_height + gap);
            *width = stack_width;
            *height = stack_height;
        }
    }
}

// Grid layout: arrange windows in a grid pattern
static void calculate_grid_layout(struct axiom_server *server, int index,
                                 int *x, int *y, int *width, int *height) {
    if (server->workspace_width <= 0 || server->workspace_height <= 0) {
        *x = *y = 0;
        *width = 800;
        *height = 600;
        return;
    }
    
    int window_count = server->window_count;
    int gap = server->config->gap_size;
    int title_height = 24;
    
    if (window_count == 1) {
        *x = gap;
        *y = gap + title_height;
        *width = server->workspace_width - (2 * gap);
        *height = server->workspace_height - (2 * gap) - title_height;
        return;
    }
    
    // Calculate grid dimensions
    int cols = (int)ceil(sqrt(window_count));
    int rows = (int)ceil((double)window_count / cols);
    
    int cell_width = (server->workspace_width - ((cols + 1) * gap)) / cols;
    int cell_height = (server->workspace_height - ((rows + 1) * gap) - title_height) / rows;
    
    int col = index % cols;
    int row = index / cols;
    
    *x = gap + col * (cell_width + gap);
    *y = gap + title_height + row * (cell_height + gap);
    *width = cell_width;
    *height = cell_height;
}

// Spiral layout: arrange windows in a spiral pattern
static void calculate_spiral_layout(struct axiom_server *server, int index,
                                   int *x, int *y, int *width, int *height) {
    if (server->workspace_width <= 0 || server->workspace_height <= 0) {
        *x = *y = 0;
        *width = 800;
        *height = 600;
        return;
    }
    
    int gap = server->config->gap_size;
    int title_height = 24;
    
    // For now, implement as a simplified fibonacci-like spiral
    // This is a complex algorithm, so we'll start with a basic implementation
    if (index == 0) {
        // Main window takes larger portion
        *x = gap;
        *y = gap + title_height;
        *width = (server->workspace_width - (3 * gap)) * 0.7f;
        *height = server->workspace_height - (2 * gap) - title_height;
    } else {
        // Secondary windows spiral around
        int remaining_width = server->workspace_width - *width - (3 * gap);
        int window_height = (server->workspace_height - ((server->window_count) * gap) - title_height) / (server->window_count - 1);
        
        *x = server->workspace_width - remaining_width - gap;
        *y = gap + title_height + (index - 1) * (window_height + gap);
        *width = remaining_width;
        *height = window_height;
    }
}

void axiom_calculate_window_layout_advanced(struct axiom_server *server, int index,
                                           int *x, int *y, int *width, int *height) {
    switch (current_layout) {
        case AXIOM_LAYOUT_MASTER_STACK:
            calculate_master_stack_layout(server, index, x, y, width, height);
            break;
        case AXIOM_LAYOUT_GRID:
            calculate_grid_layout(server, index, x, y, width, height);
            break;
        case AXIOM_LAYOUT_SPIRAL:
            calculate_spiral_layout(server, index, x, y, width, height);
            break;
        case AXIOM_LAYOUT_FLOATING:
            // Floating windows maintain their current position
            // Don't change x, y, width, height if already set
            if (*width == 0 || *height == 0) {
                *width = 800;
                *height = 600;
                *x = 100 + (index * 50); // Cascade new windows
                *y = 100 + (index * 50);
            }
            break;
        default:
            calculate_grid_layout(server, index, x, y, width, height);
            break;
    }
}

void axiom_cycle_layout(struct axiom_server *server) {
    switch (current_layout) {
        case AXIOM_LAYOUT_GRID:
            current_layout = AXIOM_LAYOUT_MASTER_STACK;
            AXIOM_LOG_INFO("Switched to Master-Stack layout");
            break;
        case AXIOM_LAYOUT_MASTER_STACK:
            current_layout = AXIOM_LAYOUT_SPIRAL;
            AXIOM_LOG_INFO("Switched to Spiral layout");
            break;
        case AXIOM_LAYOUT_SPIRAL:
            current_layout = AXIOM_LAYOUT_FLOATING;
            AXIOM_LOG_INFO("Switched to Floating layout");
            break;
        case AXIOM_LAYOUT_FLOATING:
            current_layout = AXIOM_LAYOUT_GRID;
            AXIOM_LOG_INFO("Switched to Grid layout");
            break;
        default:
            current_layout = AXIOM_LAYOUT_MASTER_STACK;
            break;
    }
    
    // Re-arrange windows with new layout
    axiom_arrange_windows(server);
}

void axiom_toggle_window_floating(struct axiom_server *server, struct axiom_window *window) {
    if (!window) {
        window = server->focused_window;
    }
    
    if (!window) {
        AXIOM_LOG_INFO("No window to toggle floating");
        return;
    }
    
    window->is_tiled = !window->is_tiled;
    
    if (window->is_tiled) {
        // Window becomes tiled
        server->window_count++;
        AXIOM_LOG_INFO("Window is now tiled (count: %d)", server->window_count);
    } else {
        // Window becomes floating
        if (server->window_count > 0) {
            server->window_count--;
        }
        
        // Save current position as floating position
        window->saved_x = window->x;
        window->saved_y = window->y;
        window->saved_width = window->width;
        window->saved_height = window->height;
        
        AXIOM_LOG_INFO("Window is now floating (tiled count: %d)", server->window_count);
    }
    
    // Re-arrange all windows
    axiom_arrange_windows(server);
}

const char* axiom_get_layout_name(void) {
    switch (current_layout) {
        case AXIOM_LAYOUT_GRID:
            return "Grid";
        case AXIOM_LAYOUT_MASTER_STACK:
            return "Master-Stack";
        case AXIOM_LAYOUT_SPIRAL:
            return "Spiral";
        case AXIOM_LAYOUT_FLOATING:
            return "Floating";
        default:
            return "Unknown";
    }
}

void axiom_update_window_decorations(struct axiom_window *window) {
    if (!window || !window->decoration_tree) {
        return;
    }
    
    struct axiom_server *server = window->server;
    if (!server || !server->config) {
        return;
    }
    
    // Update title bar components
    if (window->title_bar) {
        wlr_scene_node_set_position(&window->title_bar->node, window->x, window->y - 24);
    }
    
    if (window->title_accent) {
        wlr_scene_node_set_position(&window->title_accent->node, window->x, window->y - 2);
    }
    
    // Update border positions
    if (window->border_top) {
        wlr_scene_node_set_position(&window->border_top->node, window->x - 2, window->y - 27);
    }
    
    if (window->border_bottom) {
        wlr_scene_node_set_position(&window->border_bottom->node, window->x - 2, window->y + window->height);
    }
    
    if (window->border_left) {
        wlr_scene_node_set_position(&window->border_left->node, window->x - 3, window->y - 26);
    }
    
    if (window->border_right) {
        wlr_scene_node_set_position(&window->border_right->node, window->x + window->width, window->y - 26);
    }
    
    // Update corner positions
    // Top-left corners
    if (window->corner_tl1) {
        wlr_scene_node_set_position(&window->corner_tl1->node, window->x - 1, window->y - 25);
    }
    if (window->corner_tl2) {
        wlr_scene_node_set_position(&window->corner_tl2->node, window->x, window->y - 24);
    }
    
    // Top-right corners
    if (window->corner_tr1) {
        wlr_scene_node_set_position(&window->corner_tr1->node, window->x + window->width - 1, window->y - 25);
    }
    if (window->corner_tr2) {
        wlr_scene_node_set_position(&window->corner_tr2->node, window->x + window->width - 1, window->y - 24);
    }
    
    // Bottom-left corners
    if (window->corner_bl1) {
        wlr_scene_node_set_position(&window->corner_bl1->node, window->x - 1, window->y + window->height - 1);
    }
    if (window->corner_bl2) {
        wlr_scene_node_set_position(&window->corner_bl2->node, window->x, window->y + window->height - 2);
    }
    
    // Bottom-right corners
    if (window->corner_br1) {
        wlr_scene_node_set_position(&window->corner_br1->node, window->x + window->width - 1, window->y + window->height - 1);
    }
    if (window->corner_br2) {
        wlr_scene_node_set_position(&window->corner_br2->node, window->x + window->width - 1, window->y + window->height - 2);
    }
    
    // Change border color based on focus state
    if (window->is_focused) {
        // Active border color (from config)
        AXIOM_LOG_DEBUG("Window focused, should use active border color");
    } else {
        // Inactive border color (from config)
        AXIOM_LOG_DEBUG("Window unfocused, should use inactive border color");
    }
}
