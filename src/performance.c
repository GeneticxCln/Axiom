#include "performance.h"
#include "axiom.h"
#include "logging.h"
#include "memory.h"
#include <time.h>
#include <string.h>
#include <math.h>

/**
 * Performance monitoring and optimization system for Axiom compositor
 */

/**
 * Create performance monitor
 */
struct axiom_performance_monitor *axiom_performance_monitor_create(struct axiom_server *server) {
    struct axiom_performance_monitor *monitor = axiom_calloc_tracked(1,
        sizeof(struct axiom_performance_monitor), AXIOM_MEM_TYPE_PERFORMANCE,
        __FILE__, __func__, __LINE__);
    if (!monitor) {
        AXIOM_LOG_ERROR("PERF", "Failed to allocate performance monitor");
        return NULL;
    }
    
    monitor->server = server;
    monitor->enabled = true;
    monitor->frame_count = 0;
    monitor->last_fps_update = 0;
    monitor->current_fps = 0.0f;
    
    // Initialize timing buffers
    for (int i = 0; i < AXIOM_PERF_FRAME_HISTORY; i++) {
        monitor->frame_times[i] = 0;
        monitor->render_times[i] = 0;
        monitor->input_times[i] = 0;
    }
    
    monitor->frame_time_index = 0;
    
    AXIOM_LOG_INFO("PERF", "Performance monitor initialized");
    return monitor;
}

/**
 * Destroy performance monitor
 */
void axiom_performance_monitor_destroy(struct axiom_performance_monitor *monitor) {
    if (!monitor) return;
    
    // Print final statistics
    axiom_performance_print_report(monitor);
    
    axiom_free_tracked(monitor, __FILE__, __func__, __LINE__);
}

/**
 * Get current time in microseconds
 */
static uint64_t get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

/**
 * Start frame timing
 */
void axiom_performance_frame_start(struct axiom_performance_monitor *monitor) {
    if (!monitor || !monitor->enabled) return;
    
    monitor->frame_start_time = get_time_us();
}

/**
 * End frame timing and update statistics
 */
void axiom_performance_frame_end(struct axiom_performance_monitor *monitor) {
    if (!monitor || !monitor->enabled) return;
    
    uint64_t frame_end_time = get_time_us();
    uint64_t frame_time = frame_end_time - monitor->frame_start_time;
    
    // Store frame time
    monitor->frame_times[monitor->frame_time_index] = frame_time;
    monitor->frame_time_index = (monitor->frame_time_index + 1) % AXIOM_PERF_FRAME_HISTORY;
    
    monitor->frame_count++;
    
    // Update FPS every second
    uint64_t current_time_ms = frame_end_time / 1000;
    if (current_time_ms - monitor->last_fps_update >= 1000) {
        axiom_performance_update_fps(monitor);
        monitor->last_fps_update = current_time_ms;
    }
    
    // Check for performance issues
    if (frame_time > AXIOM_PERF_FRAME_THRESHOLD_US) {
        monitor->slow_frames++;
        if (monitor->slow_frames % 10 == 0) {
            AXIOM_LOG_WARN("PERF", "Slow frame detected: %.2fms (target: %.2fms)",
                          frame_time / 1000.0f, AXIOM_PERF_FRAME_THRESHOLD_US / 1000.0f);
        }
    }
}

/**
 * Start render timing
 */
void axiom_performance_render_start(struct axiom_performance_monitor *monitor) {
    if (!monitor || !monitor->enabled) return;
    
    monitor->render_start_time = get_time_us();
}

/**
 * End render timing
 */
void axiom_performance_render_end(struct axiom_performance_monitor *monitor) {
    if (!monitor || !monitor->enabled) return;
    
    uint64_t render_time = get_time_us() - monitor->render_start_time;
    
    // Store render time
    int index = (monitor->frame_time_index - 1 + AXIOM_PERF_FRAME_HISTORY) % AXIOM_PERF_FRAME_HISTORY;
    monitor->render_times[index] = render_time;
    
    if (render_time > AXIOM_PERF_RENDER_THRESHOLD_US) {
        monitor->slow_renders++;
    }
}

/**
 * Start input processing timing
 */
void axiom_performance_input_start(struct axiom_performance_monitor *monitor) {
    if (!monitor || !monitor->enabled) return;
    
    monitor->input_start_time = get_time_us();
}

/**
 * End input processing timing
 */
void axiom_performance_input_end(struct axiom_performance_monitor *monitor) {
    if (!monitor || !monitor->enabled) return;
    
    uint64_t input_time = get_time_us() - monitor->input_start_time;
    
    // Store input time
    int index = (monitor->frame_time_index - 1 + AXIOM_PERF_FRAME_HISTORY) % AXIOM_PERF_FRAME_HISTORY;
    monitor->input_times[index] = input_time;
    
    if (input_time > AXIOM_PERF_INPUT_THRESHOLD_US) {
        monitor->slow_inputs++;
    }
}

/**
 * Update FPS calculation
 */
void axiom_performance_update_fps(struct axiom_performance_monitor *monitor) {
    if (!monitor) return;
    
    // Calculate average frame time over the last samples
    uint64_t total_time = 0;
    int valid_samples = 0;
    
    for (int i = 0; i < AXIOM_PERF_FRAME_HISTORY; i++) {
        if (monitor->frame_times[i] > 0) {
            total_time += monitor->frame_times[i];
            valid_samples++;
        }
    }
    
    if (valid_samples > 0) {
        uint64_t avg_frame_time_us = total_time / valid_samples;
        monitor->current_fps = 1000000.0f / avg_frame_time_us;
        
        // Update min/max FPS
        if (monitor->current_fps > monitor->max_fps) {
            monitor->max_fps = monitor->current_fps;
        }
        if (monitor->min_fps == 0.0f || monitor->current_fps < monitor->min_fps) {
            monitor->min_fps = monitor->current_fps;
        }
    }
}

/**
 * Get current performance statistics
 */
void axiom_performance_get_stats(struct axiom_performance_monitor *monitor, 
                                struct axiom_performance_stats *stats) {
    if (!monitor || !stats) return;
    
    memset(stats, 0, sizeof(struct axiom_performance_stats));
    
    stats->current_fps = monitor->current_fps;
    stats->min_fps = monitor->min_fps;
    stats->max_fps = monitor->max_fps;
    stats->frame_count = monitor->frame_count;
    stats->slow_frames = monitor->slow_frames;
    stats->slow_renders = monitor->slow_renders;
    stats->slow_inputs = monitor->slow_inputs;
    
    // Calculate averages
    uint64_t total_frame_time = 0;
    uint64_t total_render_time = 0;
    uint64_t total_input_time = 0;
    int valid_samples = 0;
    
    for (int i = 0; i < AXIOM_PERF_FRAME_HISTORY; i++) {
        if (monitor->frame_times[i] > 0) {
            total_frame_time += monitor->frame_times[i];
            total_render_time += monitor->render_times[i];
            total_input_time += monitor->input_times[i];
            valid_samples++;
        }
    }
    
    if (valid_samples > 0) {
        stats->avg_frame_time_ms = (total_frame_time / valid_samples) / 1000.0f;
        stats->avg_render_time_ms = (total_render_time / valid_samples) / 1000.0f;
        stats->avg_input_time_ms = (total_input_time / valid_samples) / 1000.0f;
        
        // Calculate frame time variance for smoothness metric
        float mean_frame_time = stats->avg_frame_time_ms;
        float variance = 0.0f;
        
        for (int i = 0; i < AXIOM_PERF_FRAME_HISTORY; i++) {
            if (monitor->frame_times[i] > 0) {
                float frame_time_ms = monitor->frame_times[i] / 1000.0f;
                float diff = frame_time_ms - mean_frame_time;
                variance += diff * diff;
            }
        }
        
        variance /= valid_samples;
        stats->frame_time_variance = variance;
        stats->smoothness_score = 100.0f / (1.0f + sqrtf(variance));
    }
}

/**
 * Print performance report
 */
void axiom_performance_print_report(struct axiom_performance_monitor *monitor) {
    if (!monitor) return;
    
    struct axiom_performance_stats stats;
    axiom_performance_get_stats(monitor, &stats);
    
    AXIOM_LOG_INFO("PERF", "=== Performance Report ===");
    AXIOM_LOG_INFO("PERF", "Frames rendered: %u", stats.frame_count);
    AXIOM_LOG_INFO("PERF", "Current FPS: %.1f", stats.current_fps);
    AXIOM_LOG_INFO("PERF", "FPS range: %.1f - %.1f", stats.min_fps, stats.max_fps);
    AXIOM_LOG_INFO("PERF", "Average frame time: %.2f ms", stats.avg_frame_time_ms);
    AXIOM_LOG_INFO("PERF", "Average render time: %.2f ms", stats.avg_render_time_ms);
    AXIOM_LOG_INFO("PERF", "Average input time: %.2f ms", stats.avg_input_time_ms);
    AXIOM_LOG_INFO("PERF", "Smoothness score: %.1f/100", stats.smoothness_score);
    
    if (stats.slow_frames > 0) {
        AXIOM_LOG_WARN("PERF", "Slow frames: %u (%.1f%%)", 
                      stats.slow_frames, (stats.slow_frames * 100.0f) / stats.frame_count);
    }
    
    if (stats.slow_renders > 0) {
        AXIOM_LOG_WARN("PERF", "Slow renders: %u", stats.slow_renders);
    }
    
    if (stats.slow_inputs > 0) {
        AXIOM_LOG_WARN("PERF", "Slow input processing: %u", stats.slow_inputs);
    }
    
    // Performance recommendations
    if (stats.current_fps < 30.0f) {
        AXIOM_LOG_WARN("PERF", "Recommendation: FPS is very low, consider:");
        AXIOM_LOG_WARN("PERF", "  - Reducing visual effects");
        AXIOM_LOG_WARN("PERF", "  - Disabling animations");
        AXIOM_LOG_WARN("PERF", "  - Checking GPU drivers");
    } else if (stats.smoothness_score < 80.0f) {
        AXIOM_LOG_WARN("PERF", "Recommendation: Frame timing is inconsistent, consider:");
        AXIOM_LOG_WARN("PERF", "  - Enabling VSync");
        AXIOM_LOG_WARN("PERF", "  - Reducing background processes");
    }
}

/**
 * Enable/disable performance monitoring
 */
void axiom_performance_set_enabled(struct axiom_performance_monitor *monitor, bool enabled) {
    if (!monitor) return;
    
    monitor->enabled = enabled;
    AXIOM_LOG_INFO("PERF", "Performance monitoring %s", enabled ? "enabled" : "disabled");
}

/**
 * Reset performance statistics
 */
void axiom_performance_reset_stats(struct axiom_performance_monitor *monitor) {
    if (!monitor) return;
    
    monitor->frame_count = 0;
    monitor->slow_frames = 0;
    monitor->slow_renders = 0;
    monitor->slow_inputs = 0;
    monitor->min_fps = 0.0f;
    monitor->max_fps = 0.0f;
    monitor->current_fps = 0.0f;
    
    for (int i = 0; i < AXIOM_PERF_FRAME_HISTORY; i++) {
        monitor->frame_times[i] = 0;
        monitor->render_times[i] = 0;
        monitor->input_times[i] = 0;
    }
    
    monitor->frame_time_index = 0;
    
    AXIOM_LOG_INFO("PERF", "Performance statistics reset");
}
