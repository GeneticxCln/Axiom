#ifndef AXIOM_CONFIG_H
#define AXIOM_CONFIG_H
#define AXIOM_VERSION "4.3.1"

#include <stdbool.h>
#include <stdint.h>

/* Effects configuration */
struct axiom_effects_config {
    bool shadows_enabled;
    bool blur_enabled;
    bool transparency_enabled;
    
    // Shadow settings
    int shadow_blur_radius;
    int shadow_offset_x;
    int shadow_offset_y;
    float shadow_opacity;
    char *shadow_color;
    
    // Blur settings
    int blur_radius;
    bool blur_focus_only;
    float blur_intensity;
    
    // Transparency settings
    float focused_opacity;
    float unfocused_opacity;
    float inactive_opacity;
};

/* Smart gaps configuration */
struct axiom_smart_gaps_config {
    bool enabled;
    int base_gap;
    int min_gap;
    int max_gap;
    int single_window_gap;
    char *adaptive_mode;
};

/* Window snapping configuration */
struct axiom_window_snapping_config {
    bool enabled;
    int snap_threshold;
    int edge_resistance;
    float magnetism_strength;
    bool smart_corners;
    bool multi_monitor_snapping;
    bool window_to_window_snapping;
    bool edge_snapping;
    bool show_snap_preview;
};

/* Picture-in-Picture configuration */
struct axiom_pip_config {
    bool enabled;
    
    // Default settings
    char *default_corner;           // "bottom-right", "top-left", etc.
    char *default_size;             // "small", "medium", "large", "custom"
    int default_margin_x;
    int default_margin_y;
    
    // Behavior settings
    bool always_on_top;
    bool auto_hide_unfocused;
    bool maintain_aspect_ratio;
    bool animate_transitions;
    
    // Opacity settings
    float pip_opacity;
    float focused_opacity;
    float unfocused_opacity;
    
    // Animation settings
    uint32_t animation_duration;
    
    // Auto-hide settings
    uint32_t auto_hide_delay;
    bool auto_hide_on_fullscreen;
    
    // Size constraints
    int min_width, min_height;
    int max_width, max_height;
};

/* Workspaces configuration */
struct axiom_workspaces_config {
    int max_workspaces;
    char **names;
    int names_count;
    bool persistent_layouts;
};

/* XWayland configuration */
struct axiom_xwayland_config {
    bool enabled;
    bool lazy;                  // Start XWayland on demand
    bool force_zero_scaling;    // Force scaling to 1.0 for X11 apps
};

/* Main configuration structure */
struct axiom_config {
    // Input settings
    char *cursor_theme;
    int cursor_size;
    uint32_t repeat_rate;
    uint32_t repeat_delay;
    
    // Tiling settings
    bool tiling_enabled;
    int border_width;
    int gap_size;
    
    // Appearance settings
    char *background_color;
    char *border_active;
    char *border_inactive;
    
    // Animation settings
    bool animations_enabled;
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
    
    float animation_speed_multiplier;
    char *default_easing;
    bool animation_debug_mode;
    
    // Extended configuration sections
    struct axiom_effects_config effects;
    struct axiom_smart_gaps_config smart_gaps;
    struct axiom_window_snapping_config window_snapping;
    struct axiom_pip_config picture_in_picture;
    struct axiom_workspaces_config workspaces;
    struct axiom_xwayland_config xwayland;
};


// Forward declarations
struct axiom_server;

// Configuration functions
struct axiom_config *axiom_config_create(void);
void axiom_config_destroy(struct axiom_config *config);
bool axiom_config_load(struct axiom_config *config, const char *path);
bool axiom_config_validate(struct axiom_config *config);


// Logging functions are now defined in axiom.h

#endif /* AXIOM_CONFIG_H */
