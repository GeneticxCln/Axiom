#include "axiom.h"
#include "logging.h"
#include "window_manager.h"
#include <wlr/util/log.h>

// Title bar dimensions and styling
#define TITLE_BAR_HEIGHT 30
#define BUTTON_SIZE 18
#define BUTTON_MARGIN 6
#define BUTTON_SPACING 2

// Button colors (RGBA format)
static const float close_button_color[4] = {0.85f, 0.35f, 0.35f, 1.0f};     // Red
static const float close_button_hover[4] = {1.0f, 0.4f, 0.4f, 1.0f};        // Brighter red
static const float minimize_button_color[4] = {0.95f, 0.75f, 0.3f, 1.0f};   // Yellow
static const float minimize_button_hover[4] = {1.0f, 0.85f, 0.4f, 1.0f};    // Brighter yellow
static const float maximize_button_color[4] = {0.4f, 0.75f, 0.4f, 1.0f};    // Green
static const float maximize_button_hover[4] = {0.5f, 0.85f, 0.5f, 1.0f};    // Brighter green

void axiom_create_title_bar_buttons(struct axiom_window *window) {
    if (!window || !window->decoration_tree) {
        AXIOM_LOG_ERROR_COMPONENT("Title Button", "Cannot create title bar buttons: invalid window or decoration tree");
        return;
    }

    // Get title bar position - buttons are positioned relative to window
    int window_width = window->width;
    
    // Calculate button positions (right-aligned in title bar)
    int close_x = window_width - BUTTON_MARGIN - BUTTON_SIZE;
    int minimize_x = close_x - BUTTON_SIZE - BUTTON_SPACING;
    int maximize_x = minimize_x - BUTTON_SIZE - BUTTON_SPACING;
    int button_y = (TITLE_BAR_HEIGHT - BUTTON_SIZE) / 2; // Center vertically in title bar

    // Note: button positions calculated for future use
    (void)maximize_x; // Mark as intentionally unused for now
    (void)button_y;   // Mark as intentionally unused for now

    // Initialize hover states
    window->close_button_hovered = false;
    window->minimize_button_hovered = false;
    window->maximize_button_hovered = false;

    AXIOM_LOG_INFO_COMPONENT("Title Button", "Title bar buttons created for window");
}

// Function to render simple button icons using lines/rectangles
void axiom_render_button_icon(struct wlr_scene_tree *parent, int x, int y, int size, const char *type) {
    if (!parent || !type) {
        return;
    }

    // Icon color (white/light gray for visibility)
    static const float icon_color[4] = {0.9f, 0.9f, 0.9f, 1.0f};
    
    int icon_margin = size / 6; // Margin from button edges
    int icon_size = size - (2 * icon_margin);
    int icon_x = x + icon_margin;
    int icon_y = y + icon_margin;
    
    if (strcmp(type, "close") == 0) {
        // Create X shape with two diagonal rectangles
        int line_thickness = 2;
        
        // Top-left to bottom-right diagonal
        struct wlr_scene_rect *diag1 = wlr_scene_rect_create(parent, 
            icon_size, line_thickness, icon_color);
        if (diag1) {
            // Rotate and position for diagonal effect (approximated with thin rectangles)
            wlr_scene_node_set_position(&diag1->node, icon_x, icon_y + icon_size/2 - line_thickness/2);
        }
        
        // Top-right to bottom-left diagonal (approximated)
        struct wlr_scene_rect *diag2 = wlr_scene_rect_create(parent,
            icon_size, line_thickness, icon_color);
        if (diag2) {
            wlr_scene_node_set_position(&diag2->node, icon_x, icon_y + icon_size/2 - line_thickness/2);
        }
        
        // Vertical line for X center
        struct wlr_scene_rect *vert = wlr_scene_rect_create(parent,
            line_thickness, icon_size, icon_color);
        if (vert) {
            wlr_scene_node_set_position(&vert->node, icon_x + icon_size/2 - line_thickness/2, icon_y);
        }
        
    } else if (strcmp(type, "minimize") == 0) {
        // Horizontal line at bottom
        int line_thickness = 2;
        struct wlr_scene_rect *line = wlr_scene_rect_create(parent,
            icon_size, line_thickness, icon_color);
        if (line) {
            wlr_scene_node_set_position(&line->node, icon_x, icon_y + icon_size - line_thickness);
        }
        
    } else if (strcmp(type, "maximize") == 0) {
        // Rectangle outline
        int border_thickness = 2;
        
        // Top border
        struct wlr_scene_rect *top = wlr_scene_rect_create(parent,
            icon_size, border_thickness, icon_color);
        if (top) {
            wlr_scene_node_set_position(&top->node, icon_x, icon_y);
        }
        
        // Bottom border
        struct wlr_scene_rect *bottom = wlr_scene_rect_create(parent,
            icon_size, border_thickness, icon_color);
        if (bottom) {
            wlr_scene_node_set_position(&bottom->node, icon_x, icon_y + icon_size - border_thickness);
        }
        
        // Left border
        struct wlr_scene_rect *left = wlr_scene_rect_create(parent,
            border_thickness, icon_size, icon_color);
        if (left) {
            wlr_scene_node_set_position(&left->node, icon_x, icon_y);
        }
        
        // Right border
        struct wlr_scene_rect *right = wlr_scene_rect_create(parent,
            border_thickness, icon_size, icon_color);
        if (right) {
            wlr_scene_node_set_position(&right->node, icon_x + icon_size - border_thickness, icon_y);
        }
    }
}

void axiom_update_button_hover_states(struct axiom_window *window, double x, double y) {
    if (!window) {
        return;
    }

    // Check if cursor is within title bar area
    if (y < 0 || y > TITLE_BAR_HEIGHT) {
        // Clear all hover states if outside title bar
        bool needs_update = window->close_button_hovered || window->minimize_button_hovered || window->maximize_button_hovered;
        window->close_button_hovered = false;
        window->minimize_button_hovered = false;
        window->maximize_button_hovered = false;
        
        if (needs_update) {
            axiom_update_title_bar_buttons(window);
        }
        return;
    }

    int window_width = window->width;
    
    // Calculate button positions
    int close_x = window_width - BUTTON_MARGIN - BUTTON_SIZE;
    int minimize_x = close_x - BUTTON_SIZE - BUTTON_SPACING;
    int maximize_x = minimize_x - BUTTON_SIZE - BUTTON_SPACING;
    int button_y = (TITLE_BAR_HEIGHT - BUTTON_SIZE) / 2;

    bool old_close_hover = window->close_button_hovered;
    bool old_minimize_hover = window->minimize_button_hovered;
    bool old_maximize_hover = window->maximize_button_hovered;

    // Check close button hover
    window->close_button_hovered = (x >= close_x && x <= close_x + BUTTON_SIZE &&
                                    y >= button_y && y <= button_y + BUTTON_SIZE);

    // Check minimize button hover
    window->minimize_button_hovered = (x >= minimize_x && x <= minimize_x + BUTTON_SIZE &&
                                       y >= button_y && y <= button_y + BUTTON_SIZE);

    // Check maximize button hover
    window->maximize_button_hovered = (x >= maximize_x && x <= maximize_x + BUTTON_SIZE &&
                                       y >= button_y && y <= button_y + BUTTON_SIZE);

    // Update visuals only if hover state changed
    if (old_close_hover != window->close_button_hovered ||
        old_minimize_hover != window->minimize_button_hovered ||
        old_maximize_hover != window->maximize_button_hovered) {
        axiom_update_title_bar_buttons(window);
    }
}

void axiom_update_title_bar_buttons(struct axiom_window *window) {
    if (!window) {
        return;
    }

    // Update button colors based on hover state
    if (window->close_button) {
        const float *color = window->close_button_hovered ? close_button_hover : close_button_color;
        wlr_scene_rect_set_color(window->close_button, color);
    }

    if (window->minimize_button) {
        const float *color = window->minimize_button_hovered ? minimize_button_hover : minimize_button_color;
        wlr_scene_rect_set_color(window->minimize_button, color);
    }

    if (window->maximize_button) {
        const float *color = window->maximize_button_hovered ? maximize_button_hover : maximize_button_color;
        wlr_scene_rect_set_color(window->maximize_button, color);
    }

    // Update button positions when window is resized
    if (window->close_button || window->minimize_button || window->maximize_button) {
        int window_width = window->width;
        
        int close_x = window_width - BUTTON_MARGIN - BUTTON_SIZE;
        int minimize_x = close_x - BUTTON_SIZE - BUTTON_SPACING;
        int maximize_x = minimize_x - BUTTON_SIZE - BUTTON_SPACING;
        int button_y = (TITLE_BAR_HEIGHT - BUTTON_SIZE) / 2;

        if (window->close_button) {
            wlr_scene_node_set_position(&window->close_button->node, close_x, button_y);
        }
        if (window->minimize_button) {
            wlr_scene_node_set_position(&window->minimize_button->node, minimize_x, button_y);
        }
        if (window->maximize_button) {
            wlr_scene_node_set_position(&window->maximize_button->node, maximize_x, button_y);
        }
    }
}

bool axiom_handle_title_bar_click(struct axiom_window *window, double x, double y) {
    if (!window) {
        return false;
    }

    // Check if click is within title bar area
    if (y < 0 || y > TITLE_BAR_HEIGHT) {
        return false;
    }

    int window_width = window->width;
    
    // Calculate button positions
    int close_x = window_width - BUTTON_MARGIN - BUTTON_SIZE;
    int minimize_x = close_x - BUTTON_SIZE - BUTTON_SPACING;
    int maximize_x = minimize_x - BUTTON_SIZE - BUTTON_SPACING;
    int button_y = (TITLE_BAR_HEIGHT - BUTTON_SIZE) / 2;

    // Check close button
    if (x >= close_x && x <= close_x + BUTTON_SIZE &&
        y >= button_y && y <= button_y + BUTTON_SIZE) {
        AXIOM_LOG_INFO_COMPONENT("Title Button", "Close button clicked");
        axiom_window_close(window);
        return true;
    }

    // Check minimize button
    if (x >= minimize_x && x <= minimize_x + BUTTON_SIZE &&
        y >= button_y && y <= button_y + BUTTON_SIZE) {
        AXIOM_LOG_INFO_COMPONENT("Title Button", "Minimize button clicked");
        axiom_window_minimize(window);
        return true;
    }

    // Check maximize button
    if (x >= maximize_x && x <= maximize_x + BUTTON_SIZE &&
        y >= button_y && y <= button_y + BUTTON_SIZE) {
        AXIOM_LOG_INFO_COMPONENT("Title Button", "Maximize button clicked");
        axiom_window_toggle_maximize(window);
        return true;
    }

    return false;
}

void axiom_window_close(struct axiom_window *window) {
    if (!window) {
        AXIOM_LOG_ERROR_COMPONENT("Title Button", "Cannot close window: window is NULL");
        return;
    }

    AXIOM_LOG_INFO_COMPONENT("Title Button", "Closing window");

    if (window->type == AXIOM_WINDOW_XDG && window->xdg_toplevel) {
        // Send close request to XDG toplevel
        wlr_xdg_toplevel_send_close(window->xdg_toplevel);
    } else if (window->type == AXIOM_WINDOW_XWAYLAND && window->xwayland_surface) {
        // For XWayland windows, we would send WM_DELETE_WINDOW
        // This would be implemented when XWayland support is added
        AXIOM_LOG_DEBUG_COMPONENT("Title Button", "XWayland window close not yet implemented");
    } else {
        AXIOM_LOG_WARN_COMPONENT("Title Button", "Unknown window type for close operation");
    }
}

void axiom_window_minimize(struct axiom_window *window) {
    if (!window) {
        AXIOM_LOG_ERROR_COMPONENT("Title Button", "Cannot minimize window: window is NULL");
        return;
    }

    AXIOM_LOG_INFO_COMPONENT("Title Button", "Minimizing window");

    // Hide the window by setting its scene tree invisible
    if (window->scene_tree) {
        wlr_scene_node_set_enabled(&window->scene_tree->node, false);
    }

    // Remove focus from minimized window
    if (window->server && window->server->focused_window == window) {
        window->server->focused_window = NULL;
        
        // Try to focus another window
        struct axiom_window *next_window;
        wl_list_for_each(next_window, &window->server->windows, link) {
            if (next_window != window && next_window->scene_tree && 
                next_window->scene_tree->node.enabled) {
                // Use the focus manager if available
                if (window->server->focus_manager) {
                    // Focus management will be handled by the focus manager
                    AXIOM_LOG_DEBUG_COMPONENT("Title Button", "Focus manager will handle window focus change");
                } else {
                    axiom_focus_window_legacy(window->server, next_window, next_window->surface);
                }
                break;
            }
        }
    }

    AXIOM_LOG_DEBUG_COMPONENT("Title Button", "Window minimized successfully");
}

void axiom_window_toggle_maximize(struct axiom_window *window) {
    if (!window || !window->server) {
        AXIOM_LOG_ERROR_COMPONENT("Title Button", "Cannot maximize window: invalid window or server");
        return;
    }

    if (window->is_maximized) {
        // Restore window to previous size
        AXIOM_LOG_INFO_COMPONENT("Title Button", "Restoring window from maximized state");
        
        window->x = window->saved_x;
        window->y = window->saved_y;
        window->width = window->saved_width;
        window->height = window->saved_height;
        window->is_maximized = false;

        // Update geometry
        window->geometry->x = window->x;
        window->geometry->y = window->y;
        window->geometry->width = window->width;
        window->geometry->height = window->height;

        // Send configure to XDG toplevel
        if (window->type == AXIOM_WINDOW_XDG && window->xdg_toplevel) {
            wlr_xdg_toplevel_set_maximized(window->xdg_toplevel, false);
            wlr_xdg_toplevel_set_size(window->xdg_toplevel, window->width, window->height);
        }
    } else {
        // Save current window state and maximize
        AXIOM_LOG_INFO_COMPONENT("Title Button", "Maximizing window");
        
        window->saved_x = window->x;
        window->saved_y = window->y;
        window->saved_width = window->width;
        window->saved_height = window->height;
        window->is_maximized = true;

        // Get output dimensions for maximizing
        struct axiom_output *output;
        wl_list_for_each(output, &window->server->outputs, link) {
            if (output->wlr_output) {
                int output_width, output_height;
                wlr_output_effective_resolution(output->wlr_output, &output_width, &output_height);
                
                window->x = 0;
                window->y = 0;
                window->width = output_width;
                window->height = output_height;
                
                // Update geometry
                window->geometry->x = window->x;
                window->geometry->y = window->y;
                window->geometry->width = window->width;
                window->geometry->height = window->height;
                
                // Send configure to XDG toplevel
                if (window->type == AXIOM_WINDOW_XDG && window->xdg_toplevel) {
                    wlr_xdg_toplevel_set_maximized(window->xdg_toplevel, true);
                    wlr_xdg_toplevel_set_size(window->xdg_toplevel, window->width, window->height);
                }
                
                break; // Use first output for now
            }
        }
    }

    // Update window position in scene tree
    if (window->scene_tree) {
        wlr_scene_node_set_position(&window->scene_tree->node, window->x, window->y);
    }

    // Update decorations to match new size
    axiom_update_window_decorations(window);
    axiom_update_title_bar_buttons(window);

    AXIOM_LOG_DEBUG_COMPONENT("Title Button", "Window maximize toggle completed");
}
