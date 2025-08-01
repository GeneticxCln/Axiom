#include "animation.h"
#include "axiom.h"
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

// Helper function to get current time in milliseconds
static uint32_t get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

// Initialize animation manager
void axiom_animation_manager_init(struct axiom_server *server) {
    struct axiom_animation_manager *manager = calloc(1, sizeof(struct axiom_animation_manager));
    if (!manager) {
        AXIOM_LOG_ERROR("Failed to allocate animation manager");
        return;
    }
    
    wl_list_init(&manager->animations);
    
    // Use configuration values if available
    if (server->config) {
        manager->enabled = server->config->animations_enabled;
        manager->global_speed_multiplier = server->config->animation_speed_multiplier;
        manager->debug_mode = server->config->animation_debug_mode;
    } else {
        // Fallback defaults
        manager->enabled = true;
        manager->global_speed_multiplier = 1.0f;
        manager->debug_mode = false;
    }
    
    server->animation_manager = manager;
    AXIOM_LOG_INFO("Animation manager initialized (enabled=%s, speed=%.1fx, debug=%s)", 
                   manager->enabled ? "true" : "false",
                   manager->global_speed_multiplier,
                   manager->debug_mode ? "true" : "false");
}

// Clean up animation manager
void axiom_animation_manager_cleanup(struct axiom_animation_manager *manager) {
    if (!manager) return;
    
    struct axiom_animation *anim, *temp;
    wl_list_for_each_safe(anim, temp, &manager->animations, link) {
        axiom_animation_destroy(anim);
    }
    free(manager);
    AXIOM_LOG_INFO("Animation manager cleaned up");
}

// Create a new animation
struct axiom_animation *axiom_animation_create(enum axiom_animation_type type, uint32_t duration_ms) {
    struct axiom_animation *anim = calloc(1, sizeof(struct axiom_animation));
    if (!anim) {
        AXIOM_LOG_ERROR("Failed to allocate animation");
        return NULL;
    }
    
    anim->type = type;
    anim->duration_ms = duration_ms;
    anim->state = AXIOM_ANIM_STOPPED;
    anim->easing = AXIOM_EASE_OUT_CUBIC;  // Default easing
    anim->auto_cleanup = true;
    wl_list_init(&anim->link);
    AXIOM_LOG_DEBUG("Created animation: type=%d, duration=%dms", type, duration_ms);
    return anim;
}

// Destroy an animation
void axiom_animation_destroy(struct axiom_animation *anim) {
    if (!anim) return;
    
    if (anim->auto_cleanup) {
        wl_list_remove(&anim->link);
        AXIOM_LOG_DEBUG("Destroyed animation: type=%d", anim->type);
        free(anim);
    }
}

// Start an animation
void axiom_animation_start(struct axiom_animation_manager *manager, struct axiom_animation *anim) {
    if (!manager || !anim) return;
    
    wl_list_insert(&manager->animations, &anim->link);
    anim->start_time_ms = anim->current_time_ms = get_current_time_ms();
    anim->state = AXIOM_ANIM_RUNNING;
    manager->active_count++;
    AXIOM_LOG_DEBUG("Started animation: type=%d", anim->type);
}

// Apply easing function
float axiom_easing_apply(enum axiom_easing_type easing, float t) {
    float result = t;
    switch (easing) {
        case AXIOM_EASE_LINEAR:
            result = t;
            break;
        case AXIOM_EASE_IN_QUAD:
            result = t * t;
            break;
        case AXIOM_EASE_OUT_QUAD:
            result = 1.0f - (1.0f - t) * (1.0f - t);
            break;
        case AXIOM_EASE_IN_OUT_QUAD:
            result = t < 0.5f ? 2.0f * t * t : 1.0f - pow(-2.0f * t + 2.0f, 2) / 2.0f;
            break;
        case AXIOM_EASE_IN_CUBIC:
            result = t * t * t;
            break;
        case AXIOM_EASE_OUT_CUBIC:
            result = 1.0f - pow(1.0f - t, 3);
            break;
        case AXIOM_EASE_IN_OUT_CUBIC:
            result = t < 0.5f ? 4.0f * t * t * t : 1.0f - pow(-2.0f * t + 2.0f, 3) / 2.0f;
            break;
        case AXIOM_EASE_OUT_BOUNCE: {
            const float n1 = 7.5625f;
            const float d1 = 2.75f;
            if (t < 1.0f / d1) {
                result = n1 * t * t;
            } else if (t < 2.0f / d1) {
                t -= 1.5f / d1;
                result = n1 * t * t + 0.75f;
            } else if (t < 2.5f / d1) {
                t -= 2.25f / d1;
                result = n1 * t * t + 0.9375f;
            } else {
                t -= 2.625f / d1;
                result = n1 * t * t + 0.984375f;
            }
            break;
        }
        case AXIOM_EASE_SPRING:
            result = sinf(t * M_PI * (0.2f + 2.5f * t * t * t)) * powf(1.0f - t, 2.2f) + t * (1.0f + (1.2f * (1.0f - t)));
            break;
        default:
            result = t;
            break;
    }
    return result;
}

// Update animation manager
void axiom_animation_manager_update(struct axiom_animation_manager *manager, uint32_t time_ms) {
    if (!manager || !manager->enabled) return;
    
    struct axiom_animation *anim, *temp;
    wl_list_for_each_safe(anim, temp, &manager->animations, link) {
        if (anim->state == AXIOM_ANIM_RUNNING) {
            float t = (time_ms - anim->start_time_ms) / (float)anim->duration_ms;
            anim->progress = axiom_easing_apply(anim->easing, fminf(1.0f, fmaxf(0.0f, t)));
            
            // Call update callback
            if (anim->on_update) {
                anim->on_update(anim, anim->user_data);
            }
            
            // Check if animation finished
            if (t >= 1.0f) {
                if (anim->repeat_count == 0) {
                    axiom_animation_stop(anim);
                } else if (anim->repeat_count > 0) {
                    anim->repeat_count--;
                    anim->start_time_ms = time_ms;
                    if (anim->reverse) {
                        // Swap start and end values for reversing
                        // TODO: Implement reverse logic here
                    }
                }
            }
        }
    }
}

// Stop an animation
void axiom_animation_stop(struct axiom_animation *anim) {
    if (anim == NULL) return;
    anim->state = AXIOM_ANIM_STOPPED;
    if (anim->on_complete) {
        anim->on_complete(anim, anim->user_data);
    }
    if (anim->auto_cleanup) axiom_animation_destroy(anim);
}

// Animation callback for window appearance
static void window_appear_update(struct axiom_animation *anim, void *user_data) {
    struct axiom_window *window = (struct axiom_window*)user_data;
    if (!window || !window->scene_tree) return;
    
    // Apply fade-in effect using opacity
    float opacity = anim->progress;
    
    // For now, we'll just log the progress - scene tree opacity would need wlroots buffer manipulation
    AXIOM_LOG_DEBUG("Window appear animation: %.2f opacity", opacity);
    
    // Update window decorations opacity based on progress
    if (window->decoration_tree) {
        // In a full implementation, we'd modify the alpha values of decoration rectangles
        AXIOM_LOG_DEBUG("Updating decoration opacity to %.2f", opacity);
    }
}

// Animation callback for window disappearance
static void window_disappear_update(struct axiom_animation *anim, void *user_data) {
    struct axiom_window *window = (struct axiom_window*)user_data;
    if (!window || !window->scene_tree) return;
    
    // Apply fade-out effect
    float opacity = 1.0f - anim->progress;
    
    AXIOM_LOG_DEBUG("Window disappear animation: %.2f opacity", opacity);
    
    // Update decorations
    if (window->decoration_tree) {
        AXIOM_LOG_DEBUG("Updating decoration opacity to %.2f", opacity);
    }
}

// Animation callback for window movement
static void window_move_update(struct axiom_animation *anim, void *user_data) {
    struct axiom_window *window = (struct axiom_window*)user_data;
    if (!window || !window->scene_tree) return;
    
    // Interpolate position
    int current_x = (int)(anim->start_values.x + (anim->end_values.x - anim->start_values.x) * anim->progress);
    int current_y = (int)(anim->start_values.y + (anim->end_values.y - anim->start_values.y) * anim->progress);
    
    // Update window position
    window->x = current_x;
    window->y = current_y;
    
    // Update scene node position
    wlr_scene_node_set_position(&window->scene_tree->node, current_x, current_y);
    
    // Update decorations
    axiom_update_window_decorations(window);
    
    AXIOM_LOG_DEBUG("Window move animation: (%d, %d)", current_x, current_y);
}

// Animation callback for window resizing
static void window_resize_update(struct axiom_animation *anim, void *user_data) {
    struct axiom_window *window = (struct axiom_window*)user_data;
    if (!window || !window->scene_tree) return;
    
    // Interpolate size
    int current_width = (int)(anim->start_values.width + (anim->end_values.width - anim->start_values.width) * anim->progress);
    int current_height = (int)(anim->start_values.height + (anim->end_values.height - anim->start_values.height) * anim->progress);
    
    // Update window size
    window->width = current_width;
    window->height = current_height;
    
    // Update XDG toplevel size
    wlr_xdg_toplevel_set_size(window->xdg_toplevel, current_width, current_height);
    
    // Update decorations
    axiom_update_window_decorations(window);
    
    AXIOM_LOG_DEBUG("Window resize animation: %dx%d", current_width, current_height);
}

// Animation callback for focus ring
static void focus_ring_update(struct axiom_animation *anim, void *user_data) {
    struct axiom_window *window = (struct axiom_window*)user_data;
    if (!window || !window->decoration_tree) return;
    
    // Create a pulsing effect for focus
    float pulse = 0.8f + 0.2f * sinf(anim->progress * 6.28f); // 2*PI for full cycle
    
    AXIOM_LOG_DEBUG("Focus ring animation: %.2f intensity", pulse);
    
    // In a full implementation, we'd update border colors with the pulsing effect
    // For now, just log the effect
}

// High-level animation functions
void axiom_animate_window_appear(struct axiom_server *server, struct axiom_window *window) {
    if (!server || !server->animation_manager || !window) return;
    if (!server->config || !server->config->window_animations) return;
    
    uint32_t duration = server->config ? server->config->window_appear_duration : 300;
    struct axiom_animation *anim = axiom_animation_create(AXIOM_ANIM_WINDOW_APPEAR, duration);
    if (!anim) return;
    
    anim->window = window;
    anim->on_update = window_appear_update;
    anim->user_data = window;
    anim->easing = AXIOM_EASE_OUT_CUBIC;
    
    // Set animation values
    anim->start_values.opacity = 0.0f;
    anim->end_values.opacity = 1.0f;
    
    axiom_animation_start(server->animation_manager, anim);
    AXIOM_LOG_INFO("Started window appear animation");
}

void axiom_animate_window_disappear(struct axiom_server *server, struct axiom_window *window) {
    if (!server || !server->animation_manager || !window) return;
    
    struct axiom_animation *anim = axiom_animation_create(AXIOM_ANIM_WINDOW_DISAPPEAR, 250);
    if (!anim) return;
    
    anim->window = window;
    anim->on_update = window_disappear_update;
    anim->user_data = window;
    anim->easing = AXIOM_EASE_IN_CUBIC;
    
    // Set animation values
    anim->start_values.opacity = 1.0f;
    anim->end_values.opacity = 0.0f;
    
    axiom_animation_start(server->animation_manager, anim);
    AXIOM_LOG_INFO("Started window disappear animation");
}

void axiom_animate_window_move(struct axiom_server *server, struct axiom_window *window, 
                              int target_x, int target_y) {
    if (!server || !server->animation_manager || !window) return;
    
    struct axiom_animation *anim = axiom_animation_create(AXIOM_ANIM_WINDOW_MOVE, 200);
    if (!anim) return;
    
    anim->window = window;
    anim->on_update = window_move_update;
    anim->user_data = window;
    anim->easing = AXIOM_EASE_OUT_CUBIC;
    
    // Set animation values
    anim->start_values.x = window->x;
    anim->start_values.y = window->y;
    anim->end_values.x = target_x;
    anim->end_values.y = target_y;
    
    axiom_animation_start(server->animation_manager, anim);
    AXIOM_LOG_INFO("Started window move animation from (%d,%d) to (%d,%d)", 
                   (int)anim->start_values.x, (int)anim->start_values.y, target_x, target_y);
}

void axiom_animate_window_resize(struct axiom_server *server, struct axiom_window *window,
                                int target_width, int target_height) {
    if (!server || !server->animation_manager || !window) return;
    
    struct axiom_animation *anim = axiom_animation_create(AXIOM_ANIM_WINDOW_RESIZE, 250);
    if (!anim) return;
    
    anim->window = window;
    anim->on_update = window_resize_update;
    anim->user_data = window;
    anim->easing = AXIOM_EASE_OUT_CUBIC;
    
    // Set animation values
    anim->start_values.width = window->width;
    anim->start_values.height = window->height;
    anim->end_values.width = target_width;
    anim->end_values.height = target_height;
    
    axiom_animation_start(server->animation_manager, anim);
    AXIOM_LOG_INFO("Started window resize animation from %dx%d to %dx%d", 
                   (int)anim->start_values.width, (int)anim->start_values.height, 
                   target_width, target_height);
}

void axiom_animate_layout_change(struct axiom_server *server, struct axiom_window *window) {
    if (!server || !server->animation_manager || !window) return;
    
    // For layout changes, we animate the window to its new tiled position
    struct axiom_animation *anim = axiom_animation_create(AXIOM_ANIM_LAYOUT_CHANGE, 300);
    if (!anim) return;
    
    anim->window = window;
    anim->on_update = window_move_update; // Reuse move animation for layout changes
    anim->user_data = window;
    anim->easing = AXIOM_EASE_OUT_CUBIC;
    
    // The new position should already be calculated by the tiling system
    // We just animate from current position to the new one
    anim->start_values.x = window->x;
    anim->start_values.y = window->y;
    // end values would be set by the calling tiling function
    
    AXIOM_LOG_INFO("Started layout change animation for window");
}

void axiom_animate_workspace_switch(struct axiom_server *server, int from_workspace, int to_workspace) {
    if (!server || !server->animation_manager) return;
    
    struct axiom_animation *anim = axiom_animation_create(AXIOM_ANIM_WORKSPACE_SWITCH, 400);
    if (!anim) return;
    
    anim->easing = AXIOM_EASE_OUT_CUBIC;
    anim->user_data = server;
    
    // For workspace switching, we'd typically animate a slide transition
    // This would involve moving all windows of both workspaces
    
    axiom_animation_start(server->animation_manager, anim);
    AXIOM_LOG_INFO("Started workspace switch animation from %d to %d", from_workspace, to_workspace);
}

void axiom_animate_focus_ring(struct axiom_server *server, struct axiom_window *window) {
    if (!server || !server->animation_manager || !window) return;
    
    struct axiom_animation *anim = axiom_animation_create(AXIOM_ANIM_FOCUS_RING, 500);
    if (!anim) return;
    
    anim->window = window;
    anim->on_update = focus_ring_update;
    anim->user_data = window;
    anim->easing = AXIOM_EASE_LINEAR; // Linear for smooth pulsing
    anim->repeat_count = 2; // Pulse a few times
    
    axiom_animation_start(server->animation_manager, anim);
    AXIOM_LOG_INFO("Started focus ring animation");
}
