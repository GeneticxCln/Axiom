#ifndef AXIOM_ANIMATION_H
#define AXIOM_ANIMATION_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-server-core.h>

// Forward declarations
struct axiom_server;
struct axiom_window;
struct wlr_scene_node;
struct axiom_animation;

// Animation types
enum axiom_animation_type {
    AXIOM_ANIM_WINDOW_APPEAR,
    AXIOM_ANIM_WINDOW_DISAPPEAR, 
    AXIOM_ANIM_WINDOW_MOVE,
    AXIOM_ANIM_WINDOW_RESIZE,
    AXIOM_ANIM_LAYOUT_CHANGE,
    AXIOM_ANIM_WORKSPACE_SWITCH,
    AXIOM_ANIM_FOCUS_RING,
    AXIOM_ANIM_FADE,
    AXIOM_ANIM_SLIDE,
    AXIOM_ANIM_SCALE
};

// Easing functions
enum axiom_easing_type {
    AXIOM_EASE_LINEAR,
    AXIOM_EASE_IN_QUAD,
    AXIOM_EASE_OUT_QUAD,
    AXIOM_EASE_IN_OUT_QUAD,
    AXIOM_EASE_IN_CUBIC,
    AXIOM_EASE_OUT_CUBIC,
    AXIOM_EASE_IN_OUT_CUBIC,
    AXIOM_EASE_IN_BOUNCE,
    AXIOM_EASE_OUT_BOUNCE,
    AXIOM_EASE_SPRING
};

// Animation state
enum axiom_animation_state {
    AXIOM_ANIM_STOPPED,
    AXIOM_ANIM_RUNNING,
    AXIOM_ANIM_PAUSED,
    AXIOM_ANIM_FINISHED
};

// Animation callback function type
typedef void (*axiom_animation_callback_t)(struct axiom_animation *anim, void *user_data);

// Core animation structure
struct axiom_animation {
    struct wl_list link;  // Link in animation manager list
    
    // Animation properties
    enum axiom_animation_type type;
    enum axiom_easing_type easing;
    enum axiom_animation_state state;
    
    // Timing
    uint32_t duration_ms;      // Total animation duration
    uint32_t start_time_ms;    // Animation start timestamp
    uint32_t current_time_ms;  // Current timestamp
    float progress;            // Progress from 0.0 to 1.0
    
    // Target objects
    struct axiom_window *window;
    struct wlr_scene_node *scene_node;
    
    // Animation values (start -> end)
    struct {
        double x, y, width, height;  // Geometry
        float opacity;               // Transparency
        float scale_x, scale_y;     // Scaling
        float rotation;             // Rotation in radians
    } start_values, end_values, current_values;
    
    // Callbacks
    axiom_animation_callback_t on_update;
    axiom_animation_callback_t on_complete;
    void *user_data;
    
    // Animation-specific data
    bool auto_cleanup;  // Automatically cleanup on completion
    int repeat_count;   // -1 for infinite, 0 for no repeat, >0 for count
    bool reverse;       // Play in reverse on repeat
};

// Animation manager
struct axiom_animation_manager {
    struct wl_list animations;  // List of active animations
    struct wl_event_source *timer_source;
    bool enabled;
    int active_count;
    
    // Performance tracking
    uint32_t frame_count;
    uint32_t last_fps_time;
    float current_fps;
    
    // Global animation settings
    float global_speed_multiplier;  // 1.0 = normal, 0.5 = half speed, 2.0 = double speed
    bool debug_mode;
};

// Configuration for different animation types
struct axiom_animation_config {
    bool window_animations;
    bool workspace_animations;
    bool focus_animations;
    bool layout_animations;
    
    uint32_t window_appear_duration;
    uint32_t window_disappear_duration;
    uint32_t window_move_duration;
    uint32_t window_resize_duration;
    uint32_t workspace_switch_duration;
    uint32_t focus_ring_duration;
    uint32_t layout_change_duration;
    
    enum axiom_easing_type default_easing;
    float default_opacity_start;
    float default_scale_start;
};

// Animation system functions
void axiom_animation_manager_init(struct axiom_server *server);
void axiom_animation_manager_cleanup(struct axiom_animation_manager *manager);
void axiom_animation_manager_update(struct axiom_animation_manager *manager, uint32_t time_ms);

// Animation creation and control
struct axiom_animation *axiom_animation_create(enum axiom_animation_type type, 
                                              uint32_t duration_ms);
void axiom_animation_destroy(struct axiom_animation *anim);
void axiom_animation_start(struct axiom_animation_manager *manager, struct axiom_animation *anim);
void axiom_animation_stop(struct axiom_animation *anim);
void axiom_animation_pause(struct axiom_animation *anim);
void axiom_animation_resume(struct axiom_animation *anim);

// Easing functions
float axiom_easing_apply(enum axiom_easing_type easing, float t);

// High-level animation functions
void axiom_animate_window_appear(struct axiom_server *server, struct axiom_window *window);
void axiom_animate_window_disappear(struct axiom_server *server, struct axiom_window *window);
void axiom_animate_window_move(struct axiom_server *server, struct axiom_window *window, 
                              int target_x, int target_y);
void axiom_animate_window_resize(struct axiom_server *server, struct axiom_window *window,
                                int target_width, int target_height);
void axiom_animate_layout_change(struct axiom_server *server, struct axiom_window *window);
void axiom_animate_workspace_switch(struct axiom_server *server, int from_workspace, int to_workspace);
void axiom_animate_focus_ring(struct axiom_server *server, struct axiom_window *window);

// Configuration functions
struct axiom_animation_config *axiom_animation_config_create(void);
void axiom_animation_config_destroy(struct axiom_animation_config *config);
void axiom_animation_config_load(struct axiom_animation_config *config, const char *config_path);

// Utility functions
bool axiom_animation_is_running(struct axiom_animation *anim);
float axiom_animation_get_progress(struct axiom_animation *anim);
void axiom_animation_set_progress(struct axiom_animation *anim, float progress);

// Debug functions
void axiom_animation_debug_print(struct axiom_animation *anim);
void axiom_animation_manager_debug_stats(struct axiom_animation_manager *manager);

#endif /* AXIOM_ANIMATION_H */
