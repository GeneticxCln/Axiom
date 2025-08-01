#ifndef AXIOM_PIP_MANAGER_H
#define AXIOM_PIP_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-server-core.h>
#include "config.h"

// Forward declarations
struct axiom_server;
struct axiom_window;
struct axiom_output;

// PiP corner positions
enum axiom_pip_corner {
    AXIOM_PIP_TOP_LEFT,
    AXIOM_PIP_TOP_RIGHT,
    AXIOM_PIP_BOTTOM_LEFT,
    AXIOM_PIP_BOTTOM_RIGHT,
    AXIOM_PIP_CENTER,
    AXIOM_PIP_CUSTOM,
};

// PiP size presets
enum axiom_pip_size_preset {
    AXIOM_PIP_SIZE_TINY,      // 240x180
    AXIOM_PIP_SIZE_SMALL,     // 320x240
    AXIOM_PIP_SIZE_MEDIUM,    // 480x360
    AXIOM_PIP_SIZE_LARGE,     // 640x480
    AXIOM_PIP_SIZE_CUSTOM,    // User-defined size
};

// PiP window state
struct axiom_pip_window {
    struct wl_list link;
    struct axiom_window *window;
    
    // PiP properties
    bool is_pip;
    bool always_on_top;
    bool auto_hide_on_unfocus;
    bool maintain_aspect_ratio;
    
    // Position and size
    enum axiom_pip_corner corner;
    enum axiom_pip_size_preset size_preset;
    int custom_x, custom_y;
    int custom_width, custom_height;
    int margin_x, margin_y;  // Margin from screen edges
    
    // Opacity and effects
    float pip_opacity;        // Opacity when in PiP mode
    float focused_opacity;    // Opacity when focused
    float unfocused_opacity;  // Opacity when unfocused
    
    // Animation state
    struct {
        bool animating;
        uint32_t start_time;
        uint32_t duration;
        int start_x, start_y, start_width, start_height;
        int target_x, target_y, target_width, target_height;
    } animation;
    
    // Saved state (for restoring from PiP)
    struct {
        int x, y, width, height;
        bool was_maximized;
        bool was_tiled;
        int workspace;
    } saved_state;
    
    // Auto-hide state
    struct {
        bool enabled;
        bool hidden;
        uint32_t hide_delay_ms;
        uint32_t last_focus_time;
    } auto_hide;
};

// Forward declaration - defined in config.h
struct axiom_pip_config;

// PiP manager statistics
struct axiom_pip_stats {
    uint32_t active_pip_windows;
    uint32_t total_pip_activations;
    uint32_t total_pip_deactivations;
    uint32_t auto_hide_activations;
    uint32_t position_changes;
};

// PiP manager
struct axiom_pip_manager {
    struct axiom_server *server;
    struct axiom_pip_config config;
    
    // Active PiP windows
    struct wl_list pip_windows;  // List of axiom_pip_window
    
    // Statistics
    struct axiom_pip_stats stats;
    
    // Event listeners
    struct wl_listener window_focus;
    struct wl_listener window_unfocus;
    struct wl_listener window_fullscreen;
    
    // Update timer for animations and auto-hide
    struct wl_event_source *update_timer;
};

// Core PiP management functions
struct axiom_pip_manager *axiom_pip_manager_create(struct axiom_server *server);
void axiom_pip_manager_destroy(struct axiom_pip_manager *manager);
bool axiom_pip_manager_init(struct axiom_pip_manager *manager, struct axiom_pip_config *config);

// PiP window management
bool axiom_pip_enable_for_window(struct axiom_pip_manager *manager, 
                                 struct axiom_window *window);
bool axiom_pip_disable_for_window(struct axiom_pip_manager *manager, 
                                  struct axiom_window *window);
bool axiom_pip_toggle_for_window(struct axiom_pip_manager *manager, 
                                 struct axiom_window *window);
bool axiom_pip_is_window_pip(struct axiom_pip_manager *manager, 
                             struct axiom_window *window);

// PiP positioning
bool axiom_pip_set_corner(struct axiom_pip_manager *manager, 
                          struct axiom_window *window, 
                          enum axiom_pip_corner corner);
bool axiom_pip_set_custom_position(struct axiom_pip_manager *manager,
                                   struct axiom_window *window,
                                   int x, int y);
bool axiom_pip_cycle_corners(struct axiom_pip_manager *manager,
                             struct axiom_window *window);

// PiP sizing
bool axiom_pip_set_size_preset(struct axiom_pip_manager *manager,
                               struct axiom_window *window,
                               enum axiom_pip_size_preset preset);
bool axiom_pip_set_custom_size(struct axiom_pip_manager *manager,
                               struct axiom_window *window,
                               int width, int height);
bool axiom_pip_resize_relative(struct axiom_pip_manager *manager,
                               struct axiom_window *window,
                               float scale_factor);

// PiP behavior control
bool axiom_pip_set_opacity(struct axiom_pip_manager *manager,
                           struct axiom_window *window,
                           float opacity);
bool axiom_pip_set_always_on_top(struct axiom_pip_manager *manager,
                                  struct axiom_window *window,
                                  bool enabled);
bool axiom_pip_set_auto_hide(struct axiom_pip_manager *manager,
                             struct axiom_window *window,
                             bool enabled);

// Auto-hide functionality
void axiom_pip_check_auto_hide(struct axiom_pip_manager *manager);
bool axiom_pip_hide_window(struct axiom_pip_manager *manager,
                           struct axiom_window *window);
bool axiom_pip_show_window(struct axiom_pip_manager *manager,
                           struct axiom_window *window);

// Animation system
bool axiom_pip_animate_to_position(struct axiom_pip_manager *manager,
                                   struct axiom_window *window,
                                   int target_x, int target_y,
                                   int target_width, int target_height);
void axiom_pip_update_animations(struct axiom_pip_manager *manager,
                                 uint32_t current_time);

// Position calculation utilities
void axiom_pip_calculate_corner_position(struct axiom_pip_manager *manager,
                                          struct axiom_output *output,
                                          enum axiom_pip_corner corner,
                                          int width, int height,
                                          int *x, int *y);
void axiom_pip_constrain_to_output(struct axiom_pip_manager *manager,
                                   struct axiom_output *output,
                                   int *x, int *y, int width, int height);

// Event handlers
void axiom_pip_on_window_focus(struct axiom_pip_manager *manager,
                               struct axiom_window *window);
void axiom_pip_on_window_unfocus(struct axiom_pip_manager *manager,
                                 struct axiom_window *window);
void axiom_pip_on_window_fullscreen(struct axiom_pip_manager *manager,
                                    struct axiom_window *window,
                                    bool fullscreen);
void axiom_pip_on_output_change(struct axiom_pip_manager *manager,
                                struct axiom_output *output);

// Configuration management
bool axiom_pip_load_config(struct axiom_pip_manager *manager, const char *config_path);
bool axiom_pip_save_config(struct axiom_pip_manager *manager, const char *config_path);
void axiom_pip_load_defaults(struct axiom_pip_manager *manager);

// Statistics and debugging
struct axiom_pip_stats axiom_pip_get_stats(struct axiom_pip_manager *manager);
void axiom_pip_reset_stats(struct axiom_pip_manager *manager);
void axiom_pip_print_stats(struct axiom_pip_manager *manager);
void axiom_pip_debug_window_state(struct axiom_pip_manager *manager,
                                  struct axiom_window *window);

// Utility functions
const char *axiom_pip_corner_to_string(enum axiom_pip_corner corner);
const char *axiom_pip_size_preset_to_string(enum axiom_pip_size_preset preset);
enum axiom_pip_corner axiom_pip_corner_from_string(const char *str);
enum axiom_pip_size_preset axiom_pip_size_preset_from_string(const char *str);

// Server integration
bool axiom_server_init_pip_manager(struct axiom_server *server, struct axiom_pip_config *config);
void axiom_server_destroy_pip_manager(struct axiom_server *server);

#endif // AXIOM_PIP_MANAGER_H
