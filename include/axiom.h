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

    // Tiling properties
    bool is_tiled;
    bool is_focused;
    bool is_fullscreen;
    bool is_maximized;
    int x, y, width, height;
    int saved_x, saved_y, saved_width, saved_height; // For restore operations
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
    int current_workspace;
    int max_workspaces;
    
    // Configuration
    struct {
        char *cursor_theme;
        int cursor_size;
        uint32_t repeat_rate;
        uint32_t repeat_delay;
    } config;
};

/* Function declarations */
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

// Output management
void axiom_new_output(struct wl_listener *listener, void *data);

// Logging
void axiom_log(const char *level, const char *format, ...);

#define AXIOM_LOG_INFO(fmt, ...) axiom_log("INFO", fmt, ##__VA_ARGS__)
#define AXIOM_LOG_ERROR(fmt, ...) axiom_log("ERROR", fmt, ##__VA_ARGS__)
#define AXIOM_LOG_DEBUG(fmt, ...) axiom_log("DEBUG", fmt, ##__VA_ARGS__)

#endif /* AXIOM_H */
