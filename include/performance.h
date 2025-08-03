#ifndef AXIOM_PERFORMANCE_H
#define AXIOM_PERFORMANCE_H

#include <stdint.h>
#include <stdbool.h>

struct axiom_server;

// Performance monitoring constants
#define AXIOM_PERF_FRAME_HISTORY 120      // Track last 2 seconds at 60fps
#define AXIOM_PERF_FRAME_THRESHOLD_US 20000  // 20ms = 50fps threshold
#define AXIOM_PERF_RENDER_THRESHOLD_US 10000 // 10ms render threshold
#define AXIOM_PERF_INPUT_THRESHOLD_US 5000   // 5ms input threshold

/**
 * Performance monitoring data structure
 */
struct axiom_performance_monitor {
    struct axiom_server *server;
    bool enabled;
    
    // Timing data
    uint64_t frame_start_time;
    uint64_t render_start_time;
    uint64_t input_start_time;
    
    // Frame timing history
    uint64_t frame_times[AXIOM_PERF_FRAME_HISTORY];
    uint64_t render_times[AXIOM_PERF_FRAME_HISTORY];
    uint64_t input_times[AXIOM_PERF_FRAME_HISTORY];
    int frame_time_index;
    
    // Statistics
    uint32_t frame_count;
    uint32_t slow_frames;
    uint32_t slow_renders;
    uint32_t slow_inputs;
    
    // FPS tracking
    float current_fps;
    float min_fps;
    float max_fps;
    uint64_t last_fps_update;
};

/**
 * Performance statistics structure
 */
struct axiom_performance_stats {
    float current_fps;
    float min_fps;
    float max_fps;
    uint32_t frame_count;
    uint32_t slow_frames;
    uint32_t slow_renders;
    uint32_t slow_inputs;
    float avg_frame_time_ms;
    float avg_render_time_ms;
    float avg_input_time_ms;
    float frame_time_variance;
    float smoothness_score;  // 0-100, higher is better
};

// Performance monitor functions
struct axiom_performance_monitor *axiom_performance_monitor_create(struct axiom_server *server);
void axiom_performance_monitor_destroy(struct axiom_performance_monitor *monitor);

// Timing functions
void axiom_performance_frame_start(struct axiom_performance_monitor *monitor);
void axiom_performance_frame_end(struct axiom_performance_monitor *monitor);
void axiom_performance_render_start(struct axiom_performance_monitor *monitor);
void axiom_performance_render_end(struct axiom_performance_monitor *monitor);
void axiom_performance_input_start(struct axiom_performance_monitor *monitor);
void axiom_performance_input_end(struct axiom_performance_monitor *monitor);

// Statistics functions
void axiom_performance_update_fps(struct axiom_performance_monitor *monitor);
void axiom_performance_get_stats(struct axiom_performance_monitor *monitor, 
                                struct axiom_performance_stats *stats);
void axiom_performance_print_report(struct axiom_performance_monitor *monitor);

// Control functions
void axiom_performance_set_enabled(struct axiom_performance_monitor *monitor, bool enabled);
void axiom_performance_reset_stats(struct axiom_performance_monitor *monitor);

#endif /* AXIOM_PERFORMANCE_H */
