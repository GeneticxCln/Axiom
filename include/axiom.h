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

/* Configuration structure - defined in config.h */

/* Forward declarations */
struct axiom_window {
    struct wl_list link;
    struct wlr_xdg_toplevel *xdg_toplevel;
    struct wlr_scene_tree *scene_tree;
    struct axiom_server *server;
    
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;

    // Surface for effects
    struct wlr_surface *surface;
    
    // Geometry convenience access
    struct wlr_box geometry;

    // Tiling properties
    bool is_tiled;
    bool is_focused;
    bool is_fullscreen;
    bool is_maximized;
    int x, y, width, height;
    int saved_x, saved_y, saved_width, saved_height; // For restore operations
    
    // Window decorations - Enhanced rounded border system
    struct wlr_scene_tree *decoration_tree;
    
    // Title bar components
    struct wlr_scene_rect *title_bar;
    struct wlr_scene_rect *title_accent;
    
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
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    struct wlr_compositor *compositor;
    struct wlr_scene *scene;
    struct wlr_scene_output_layout *scene_layout;
    struct wlr_output_layout *output_layout;
    
    // Input management
    struct wlr_cursor *cursor;
    struct wlr_xcursor_manager *cursor_mgr;
    struct wlr_seat *seat;
    struct wlr_data_device_manager *data_device_manager;
    struct wl_list input_devices;
    
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

// Cursor management
void axiom_cursor_motion(struct wl_listener *listener, void *data);
void axiom_cursor_motion_absolute(struct wl_listener *listener, void *data);
void axiom_cursor_button(struct wl_listener *listener, void *data);
void axiom_cursor_axis(struct wl_listener *listener, void *data);
void axiom_cursor_frame(struct wl_listener *listener, void *data);
void axiom_process_cursor_motion(struct axiom_server *server, uint32_t time);
void axiom_process_cursor_resize(struct axiom_server *server, uint32_t time);
void axiom_process_cursor_move(struct axiom_server *server, uint32_t time);

// Window management
struct axiom_window *axiom_window_at(struct axiom_server *server, double lx, double ly, 
                                     struct wlr_surface **surface, double *sx, double *sy);
void axiom_focus_window(struct axiom_server *server, struct axiom_window *window, 
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

// Logging
void axiom_log(const char *level, const char *format, ...);

#define AXIOM_LOG_INFO(fmt, ...) axiom_log("INFO", fmt, ##__VA_ARGS__)
#define AXIOM_LOG_ERROR(fmt, ...) axiom_log("ERROR", fmt, ##__VA_ARGS__)
#define AXIOM_LOG_DEBUG(fmt, ...) axiom_log("DEBUG", fmt, ##__VA_ARGS__)

#endif /* AXIOM_H */
