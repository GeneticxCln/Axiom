#ifndef AXIOM_SMART_GAPS_H
#define AXIOM_SMART_GAPS_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-server-core.h>
#include "config.h"

// Forward declarations
struct axiom_server;
struct axiom_window;
struct axiom_output;

// Maximum number of gap profiles
#define AXIOM_MAX_GAP_PROFILES 16

// Gap types
enum axiom_gap_type {
    AXIOM_GAP_INNER,    // Between windows
    AXIOM_GAP_OUTER,    // Between windows and screen edges
    AXIOM_GAP_TOP,      // Top edge only
    AXIOM_GAP_BOTTOM,   // Bottom edge only
    AXIOM_GAP_LEFT,     // Left edge only
    AXIOM_GAP_RIGHT,    // Right edge only
};

// Gap adaptation modes
enum axiom_gap_adaptation_mode {
    AXIOM_GAP_STATIC,           // Fixed gaps
    AXIOM_GAP_ADAPTIVE_COUNT,   // Adapt based on window count
    AXIOM_GAP_ADAPTIVE_DENSITY, // Adapt based on screen density
    AXIOM_GAP_ADAPTIVE_FOCUS,   // Adapt based on focused window
    AXIOM_GAP_ADAPTIVE_MIXED,   // Multiple adaptation factors
};

// Gap profile for different scenarios
struct axiom_gap_profile {
    char name[64];
    bool enabled;
    
    // Base gap sizes (in pixels)
    int inner_gap;
    int outer_gap;
    int top_gap;
    int bottom_gap;
    int left_gap;
    int right_gap;
    
    // Adaptation settings
    enum axiom_gap_adaptation_mode adaptation_mode;
    
    // Adaptive parameters
    struct {
        int min_gap;        // Minimum gap size
        int max_gap;        // Maximum gap size
        float scale_factor; // Scaling factor for adaptation
        int threshold;      // Threshold for adaptation triggers
    } adaptive;
    
    // Conditions for profile activation
    struct {
        int min_windows;    // Minimum window count
        int max_windows;    // Maximum window count
        bool fullscreen_disable; // Disable gaps in fullscreen
        bool floating_override;  // Different gaps for floating windows
        char *workspace_pattern; // Workspace name pattern
        char *output_pattern;    // Output name pattern
    } conditions;
    
    // Animation settings
    struct {
        bool enabled;
        uint32_t duration_ms;
        enum {
            AXIOM_GAP_ANIM_LINEAR,
            AXIOM_GAP_ANIM_EASE_IN,
            AXIOM_GAP_ANIM_EASE_OUT,
            AXIOM_GAP_ANIM_EASE_IN_OUT,
        } easing;
    } animation;
};

// Current gap state for an output
struct axiom_gap_state {
    struct wl_list link;  // For gap_states list
    struct axiom_output *output;
    struct axiom_gap_profile *active_profile;
    
    // Current effective gap sizes
    int current_inner;
    int current_outer;
    int current_top;
    int current_bottom;
    int current_left;
    int current_right;
    
    // Animation state
    struct {
        bool active;
        uint32_t start_time;
        uint32_t duration;
        int start_values[6]; // Starting gap values
        int target_values[6]; // Target gap values
    } animation;
    
    // Statistics
    uint32_t adaptations_count;
    uint32_t last_adaptation_time;
};

// Smart gaps manager
struct axiom_smart_gaps_manager {
    struct axiom_server *server;
    
    // Gap profiles
    struct axiom_gap_profile profiles[AXIOM_MAX_GAP_PROFILES];
    int profile_count;
    
    // Global settings
    bool enabled;
    bool smart_borders;      // Hide borders when gaps are disabled
    bool smart_gaps;         // Hide gaps when only one window
    bool outer_gaps_smart;   // Smart outer gaps behavior
    
    // Per-output gap states
    struct wl_list gap_states; // List of axiom_gap_state
    
    // Default profile
    struct axiom_gap_profile *default_profile;
    
    // Statistics
    struct {
        uint32_t total_adaptations;
        uint32_t profile_switches;
        uint32_t animation_frames;
    } stats;
};

// Gap calculation context
struct axiom_gap_context {
    struct axiom_output *output;
    struct axiom_gap_state *gap_state;
    int window_count;
    int tiled_windows;
    int floating_windows;
    bool has_fullscreen;
    struct axiom_window *focused_window;
    int screen_width;
    int screen_height;
    float density; // DPI-based density factor
};

// Core gap management functions
struct axiom_smart_gaps_manager *axiom_smart_gaps_manager_create(struct axiom_server *server);
void axiom_smart_gaps_manager_destroy(struct axiom_smart_gaps_manager *manager);
bool axiom_smart_gaps_manager_init(struct axiom_smart_gaps_manager *manager, struct axiom_smart_gaps_config *config);

// Gap profile management
bool axiom_smart_gaps_add_profile(struct axiom_smart_gaps_manager *manager, 
                                  const struct axiom_gap_profile *profile);
struct axiom_gap_profile *axiom_smart_gaps_get_profile(struct axiom_smart_gaps_manager *manager, 
                                                       const char *name);
bool axiom_smart_gaps_remove_profile(struct axiom_smart_gaps_manager *manager, const char *name);
void axiom_smart_gaps_set_default_profile(struct axiom_smart_gaps_manager *manager, 
                                          const char *name);

// Gap state management
struct axiom_gap_state *axiom_smart_gaps_get_output_state(struct axiom_smart_gaps_manager *manager,
                                                          struct axiom_output *output);
bool axiom_smart_gaps_update_output_state(struct axiom_smart_gaps_manager *manager,
                                          struct axiom_output *output);

// Gap calculation and application
bool axiom_smart_gaps_calculate_gaps(struct axiom_smart_gaps_manager *manager,
                                     struct axiom_gap_context *context,
                                     int *inner, int *outer, int *top, int *bottom, int *left, int *right);
bool axiom_smart_gaps_apply_to_layout(struct axiom_smart_gaps_manager *manager,
                                      struct axiom_output *output);

// Adaptive gap algorithms
int axiom_smart_gaps_adapt_by_count(const struct axiom_gap_profile *profile, int window_count);
int axiom_smart_gaps_adapt_by_density(const struct axiom_gap_profile *profile, float density);
int axiom_smart_gaps_adapt_by_focus(const struct axiom_gap_profile *profile, 
                                    struct axiom_window *focused_window);

// Profile selection
struct axiom_gap_profile *axiom_smart_gaps_select_profile(struct axiom_smart_gaps_manager *manager,
                                                          struct axiom_gap_context *context);

// Animation system
bool axiom_smart_gaps_start_animation(struct axiom_gap_state *state,
                                      const int target_gaps[6],
                                      uint32_t duration_ms);
bool axiom_smart_gaps_update_animation(struct axiom_gap_state *state, uint32_t current_time);
bool axiom_smart_gaps_is_animating(const struct axiom_gap_state *state);

// Configuration
bool axiom_smart_gaps_load_config(struct axiom_smart_gaps_manager *manager, const char *config_path);
bool axiom_smart_gaps_load_defaults(struct axiom_smart_gaps_manager *manager);
bool axiom_smart_gaps_save_config(struct axiom_smart_gaps_manager *manager, const char *config_path);

// Integration with window management
void axiom_smart_gaps_on_window_create(struct axiom_smart_gaps_manager *manager, 
                                       struct axiom_window *window);
void axiom_smart_gaps_on_window_destroy(struct axiom_smart_gaps_manager *manager, 
                                        struct axiom_window *window);
void axiom_smart_gaps_on_window_focus(struct axiom_smart_gaps_manager *manager, 
                                      struct axiom_window *window);
void axiom_smart_gaps_on_workspace_switch(struct axiom_smart_gaps_manager *manager, 
                                          int workspace);
void axiom_smart_gaps_on_fullscreen_toggle(struct axiom_smart_gaps_manager *manager,
                                           struct axiom_window *window, bool fullscreen);

// Utility functions
void axiom_smart_gaps_print_profiles(struct axiom_smart_gaps_manager *manager);
void axiom_smart_gaps_print_stats(struct axiom_smart_gaps_manager *manager);
void axiom_smart_gaps_debug_output_state(struct axiom_gap_state *state);

// Runtime control
bool axiom_smart_gaps_toggle(struct axiom_smart_gaps_manager *manager);
bool axiom_smart_gaps_set_enabled(struct axiom_smart_gaps_manager *manager, bool enabled);
bool axiom_smart_gaps_increase_gaps(struct axiom_smart_gaps_manager *manager, int delta);
bool axiom_smart_gaps_decrease_gaps(struct axiom_smart_gaps_manager *manager, int delta);
bool axiom_smart_gaps_reset_gaps(struct axiom_smart_gaps_manager *manager);

// Advanced features
bool axiom_smart_gaps_create_profile_from_current(struct axiom_smart_gaps_manager *manager,
                                                  const char *name);
bool axiom_smart_gaps_auto_adjust_for_dpi(struct axiom_smart_gaps_manager *manager,
                                          struct axiom_output *output);
bool axiom_smart_gaps_apply_workspace_profile(struct axiom_smart_gaps_manager *manager,
                                              int workspace, const char *profile_name);

// Server integration
bool axiom_server_init_smart_gaps(struct axiom_server *server, struct axiom_smart_gaps_config *config);
void axiom_server_destroy_smart_gaps(struct axiom_server *server);

#endif // AXIOM_SMART_GAPS_H
