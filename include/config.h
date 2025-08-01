#ifndef AXIOM_CONFIG_H
#define AXIOM_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

/* Configuration structure */
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
};

// Configuration functions
struct axiom_config *axiom_config_create(void);
void axiom_config_destroy(struct axiom_config *config);
bool axiom_config_load(struct axiom_config *config, const char *path);

// Logging function (needs to be provided)
void axiom_log(const char *level, const char *format, ...);

#define AXIOM_LOG_INFO(fmt, ...) axiom_log("INFO", fmt, ##__VA_ARGS__)
#define AXIOM_LOG_ERROR(fmt, ...) axiom_log("ERROR", fmt, ##__VA_ARGS__)
#define AXIOM_LOG_DEBUG(fmt, ...) axiom_log("DEBUG", fmt, ##__VA_ARGS__)

#endif /* AXIOM_CONFIG_H */
