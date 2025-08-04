#ifndef AXIOM_H
#define AXIOM_H

#include <stdbool.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <xkbcommon/xkbcommon.h>

struct axiom_server;
struct axiom_output;
struct axiom_input_device;
struct axiom_config;
struct axiom_animation_manager;
struct axiom_effects_manager;
struct axiom_xwayland_manager;
struct axiom_tag_manager;
struct axiom_keybinding_manager;
struct axiom_focus_manager;
struct axiom_window_manager;
struct axiom_multi_session_manager;

// Enhanced window properties
struct axiom_window_tags {
    uint32_t tags;          // Bitmask for multiple tags
    bool is_sticky;         // Visible on all workspaces
    bool is_urgent;         // Window needs attention
    bool is_floating;       // Force floating mode
    bool is_private;        // Hide from switchers
    bool is_scratchpad;     // Hide/show toggle
    uint32_t workspace;     // Workspace assignment
};

/* Configuration structure - defined in config.h */

/* Forward declarations */
struct axiom_window {
    struct wl_list link;
    struct wlr_xdg_toplevel *xdg_toplevel;  // NULL for XWayland windows
    struct wlr_scene_tree *scene_tree;
    struct axiom_server *server;
    
    // Window type - either XDG or XWayland
    enum {
        AXIOM_WINDOW_XDG,
        AXIOM_WINDOW_XWAYLAND
    } type;
    
    // XWayland-specific data (NULL for XDG windows)
    struct axiom_xwayland_surface *xwayland_surface;
    
    // New window manager fields
    struct axiom_window_manager *manager;
    struct axiom_window_state *state;
    struct axiom_window_geometry *geometry;
    struct axiom_window_layout *layout;
    
    struct wl_list mapped_link;    // Link in manager's mapped windows list
    struct wl_list tiled_link;     // Link in manager's tiled windows list
    struct wl_list floating_link;  // Link in manager's floating windows list
    
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;

    // Surface for effects
    struct wlr_surface *surface;
    
    // Tiling properties
    bool is_tiled;
    bool is_focused;
    bool is_floating;
    bool is_fullscreen;
    bool is_maximized;
    int x, y, width, height;
    int saved_x, saved_y, saved_width, saved_height; // For restore operations
    
    // Window decorations - Enhanced rounded border system
    struct wlr_scene_tree *decoration_tree;
    
    // Title bar components
    struct wlr_scene_rect *title_bar;
    struct wlr_scene_rect *title_accent;
    
    // Title bar buttons (interactive) - containers for background + icons
    struct wlr_scene_tree *close_button_tree;
    struct wlr_scene_tree *minimize_button_tree;
    struct wlr_scene_tree *maximize_button_tree;
    
    // Button backgrounds
    struct wlr_scene_rect *close_button;
    struct wlr_scene_rect *minimize_button;
    struct wlr_scene_rect *maximize_button;
    
    // Button state
    bool close_button_hovered;
    bool minimize_button_hovered;
    bool maximize_button_hovered;
    
    // Border components (4-sided)
    struct wlr_scene_rect *border_top;
    struct wlr_scene_rect *border_bottom;
    struct wlr_scene_rect *border_left;
    struct wlr_scene_rect *border_right;
    
    // Corner rounding elements (8 corner pieces for smooth effect)
    struct wlr_scene_rect *corner_tl1;  // Top-left corner piece 1
    struct wlr_scene_rect *corner_tl2;  // Top-left corner piece 2
    struct wlr_scene_rect *corner_tr1;  // Top-right corner piece 1
    struct wlr_scene_rect *corner_tr2;  // Top-right corner piece 2
    struct wlr_scene_rect *corner_bl1;  // Bottom-left corner piece 1
    struct wlr_scene_rect *corner_bl2;  // Bottom-left corner piece 2
    struct wlr_scene_rect *corner_br1;  // Bottom-right corner piece 1
    struct wlr_scene_rect *corner_br2;  // Bottom-right corner piece 2
    
    // Visual effects
    struct wlr_scene_tree *shadow_tree;  // Shadow rendering tree
    bool effects_enabled;                // Window has effects enabled
    
    // Real-time effects support
    struct axiom_window_effects *effects; // Per-window effects state
    
    // Enhanced window properties
    struct axiom_window_tags *window_tags; // Tagging and urgency
};

struct axiom_workspace {
    struct wl_list windows;
    int width, height;
    char *name;                    // Custom workspace name
    int window_count;              // Cached window count
    bool persistent_layout;        // Remember layout between switches
    int saved_layout_type;         // Saved layout for this workspace
    float saved_master_ratio;      // Saved master ratio
};

struct axiom_output {
    struct wl_list link;
    struct axiom_server *server;
    struct wlr_output *wlr_output;
    struct wlr_scene_output *scene_output;
    
    struct wl_listener frame;
    struct wl_listener destroy;
};

struct axiom_input_device {
    struct wl_list link;
    struct axiom_server *server;
    struct wlr_input_device *wlr_device;
    
    struct wl_listener destroy;
    
    union {
        struct {
            struct wl_listener modifiers;
            struct wl_listener key;
            struct wl_listener repeat_info;
        } keyboard;
        
        struct {
            struct wl_listener motion;
            struct wl_listener motion_absolute;
            struct wl_listener button;
            struct wl_listener axis;
        } pointer;
    };
};

enum axiom_cursor_mode {
    AXIOM_CURSOR_PASSTHROUGH,
    AXIOM_CURSOR_MOVE,
    AXIOM_CURSOR_RESIZE,
};

// Phase 2: Tiling layout types
enum axiom_layout_type {
    AXIOM_LAYOUT_GRID,
    AXIOM_LAYOUT_MASTER_STACK,
    AXIOM_LAYOUT_SPIRAL,
    AXIOM_LAYOUT_FLOATING
};

struct axiom_server {
    struct wl_display *wl_display;
    struct wl_event_loop *wl_event_loop;
    struct wlr_backend *backend;
    struct wlr_session *session;  // Session for primary display server mode
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    struct wlr_compositor *compositor;
    struct wlr_scene *scene;
    struct wlr_scene_output_layout *scene_layout;
    struct wlr_output_layout *output_layout;
    struct wlr_scene_rect *background;  // Background to prevent black screen
    
    // Input management
    struct wlr_cursor *cursor;
    struct wlr_xcursor_manager *cursor_mgr;
    struct wlr_seat *seat;
    struct wlr_data_device_manager *data_device_manager;
    struct axiom_input_manager *input_manager; // Enhanced input manager
    struct wl_list input_devices; // Legacy compatibility
    
    // Window management
    struct wlr_xdg_shell *xdg_shell;
    struct wl_list windows;
    struct wl_list outputs;
    struct axiom_window *focused_window;
    struct axiom_window *grabbed_window;
    
    // Cursor state
    enum axiom_cursor_mode cursor_mode;
    double grab_x, grab_y;
    struct wlr_box grab_geobox;
    uint32_t resize_edges;
    
    // Event listeners
    struct wl_listener new_output;
    struct wl_listener new_xdg_toplevel;
    struct wl_listener new_input;
    struct wl_listener cursor_motion;
    struct wl_listener cursor_motion_absolute;
    struct wl_listener cursor_button;
    struct wl_listener cursor_axis;
    struct wl_listener cursor_frame;
    struct wl_listener request_cursor;
    struct wl_listener request_set_selection;
    struct wl_listener backend_destroy;
    
    bool running;
    
    // Tiling management
    bool tiling_enabled;
    int workspace_width;
    int workspace_height;
    int window_count;
    struct axiom_workspace *workspaces;
    int current_workspace;
    int max_workspaces;
    
    // Configuration
    struct axiom_config *config;
    
    // Animation system
    struct axiom_animation_manager *animation_manager;
    
    // Visual effects system
    struct axiom_effects_manager *effects_manager;
    
    // Window rules system (Phase 3.1)
    struct axiom_window_rules_manager *window_rules_manager;
    
    // Smart gaps system (Phase 3.2)
    struct axiom_smart_gaps_manager *smart_gaps_manager;
    
    // Window snapping system (Phase 3.3)
    struct axiom_pip_manager *pip_manager;
    struct axiom_window_snapping_manager *window_snapping_manager;
    
    // Thumbnail system (Phase 3.4)
    struct axiom_thumbnail_manager *thumbnail_manager;
    
    // XWayland system
    struct axiom_xwayland_manager *xwayland_manager;
    struct axiom_xwayland_manager *enhanced_xwayland_manager;  // Enhanced XWayland support
    
    // Enhanced systems
    struct axiom_window_manager *window_manager;         // Professional window management
    struct axiom_tag_manager *tag_manager;             // Tagging system
    struct axiom_keybinding_manager *keybinding_manager; // Key bindings
    struct axiom_focus_manager *focus_manager;           // Focus and stacking
    
    // Desktop integration protocols
    struct axiom_layer_shell_manager *layer_shell_manager;   // Layer shell for panels/bars
    struct axiom_screenshot_manager *screenshot_manager;      // Screenshot functionality
    struct axiom_session_manager *session_manager;            // Session management
    
    // Multi-session support
    struct axiom_multi_session_manager *multi_session_manager; // Multi-user session management
    
    // Performance monitoring
    struct axiom_performance_monitor *performance_monitor;   // Performance monitoring and optimization
};

/* Function declarations */
// Workspace management
void axiom_switch_workspace(struct axiom_server *server, int workspace);
void axiom_init_workspaces(struct axiom_server *server);
void axiom_move_window_to_workspace(struct axiom_server *server, struct axiom_window *window, int workspace);
int axiom_get_workspace_window_count(struct axiom_server *server, int workspace);
void axiom_cleanup_workspaces(struct axiom_server *server);

// Phase 2: Advanced workspace management
void axiom_switch_to_workspace_by_number(struct axiom_server *server, int number);
void axiom_move_focused_window_to_workspace(struct axiom_server *server, int workspace);
void axiom_set_workspace_name(struct axiom_server *server, int workspace, const char *name);
const char* axiom_get_workspace_name(struct axiom_server *server, int workspace);
void axiom_save_workspace_layout(struct axiom_server *server, int workspace);
void axiom_restore_workspace_layout(struct axiom_server *server, int workspace);
int axiom_get_current_workspace(struct axiom_server *server);
void axiom_update_workspace_indicators(struct axiom_server *server);

// Core functions
bool axiom_server_init(struct axiom_server *server);
void axiom_server_run(struct axiom_server *server);
void axiom_server_cleanup(struct axiom_server *server);

// Input handling
void axiom_new_input(struct wl_listener *listener, void *data);
void axiom_new_keyboard(struct axiom_server *server, struct wlr_input_device *device);
void axiom_new_pointer(struct axiom_server *server, struct wlr_input_device *device);
void axiom_remove_input_devices(struct axiom_server *server);

// Cursor and input event handlers
void axiom_cursor_motion(struct wl_listener *listener, void *data);
void axiom_cursor_motion_absolute(struct wl_listener *listener, void *data);
void axiom_cursor_button(struct wl_listener *listener, void *data);
void axiom_cursor_axis(struct wl_listener *listener, void *data);
void axiom_cursor_frame(struct wl_listener *listener, void *data);

// Enhanced cursor edge detection and resizing
uint32_t axiom_detect_resize_edges(struct axiom_window *window, double cursor_x, double cursor_y);
void axiom_set_resize_cursor(struct axiom_server *server, uint32_t edges);
void axiom_process_cursor_motion(struct axiom_server *server, uint32_t time);
void axiom_process_cursor_resize(struct axiom_server *server, uint32_t time);
void axiom_process_cursor_move(struct axiom_server *server, uint32_t time);

// Window management
struct axiom_window *axiom_window_at(struct axiom_server *server, double lx, double ly, 
                                     struct wlr_surface **surface, double *sx, double *sy);
void axiom_focus_window_legacy(struct axiom_server *server, struct axiom_window *window, 
                                struct wlr_surface *surface);
void axiom_begin_interactive(struct axiom_window *window, enum axiom_cursor_mode mode, 
                             uint32_t edges);

// Tiling system
void axiom_arrange_windows(struct axiom_server *server);
void axiom_calculate_window_layout(struct axiom_server *server, int index, 
                                   int *x, int *y, int *width, int *height);
void axiom_calculate_window_layout_advanced(struct axiom_server *server, int index,
                                           int *x, int *y, int *width, int *height);
void axiom_cycle_layout(struct axiom_server *server);
void axiom_toggle_window_floating(struct axiom_server *server, struct axiom_window *window);
void axiom_adjust_master_ratio(float delta);
const char* axiom_get_layout_name(void);
void axiom_update_window_decorations(struct axiom_window *window);

// Title bar button functions
void axiom_create_title_bar_buttons(struct axiom_window *window);
void axiom_update_title_bar_buttons(struct axiom_window *window);
void axiom_update_button_hover_states(struct axiom_window *window, double x, double y);
bool axiom_handle_title_bar_click(struct axiom_window *window, double x, double y);
void axiom_window_close(struct axiom_window *window);
void axiom_window_minimize(struct axiom_window *window);
void axiom_window_toggle_maximize(struct axiom_window *window);

// Phase 2: Layout management
void axiom_set_layout(enum axiom_layout_type layout);
enum axiom_layout_type axiom_get_layout(void);

// Output management
void axiom_new_output(struct wl_listener *listener, void *data);

// Process management
void axiom_process_init(struct axiom_server *server);
void axiom_process_cleanup(void);
pid_t axiom_spawn_process(const char *command, bool autostart);
void axiom_spawn_terminal(void);
void axiom_spawn_rofi(void);
void axiom_spawn_waybar(struct axiom_server *server);
void axiom_kill_waybar(void);
bool axiom_process_exists(const char *name);

// Configuration functions in config.h
void axiom_reload_configuration(struct axiom_server *server);

// Logging system is defined in logging.h
#include "logging.h"

#endif /* AXIOM_H */
