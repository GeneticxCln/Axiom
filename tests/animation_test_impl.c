#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <wayland-server-core.h>

// Test-specific logging macros
#define AXIOM_LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define AXIOM_LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#define AXIOM_LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

#include "animation.h"

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
    manager->enabled = true;
    manager->global_speed_multiplier = 1.0f;
    manager->debug_mode = false;
    
    // Cast to test server and set the manager pointer
    struct test_server {
        struct axiom_animation_manager *animation_manager;
    } *test_srv = (struct test_server*)server;
    test_srv->animation_manager = manager;
    
    AXIOM_LOG_INFO("Animation manager initialized");
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
        case AXIOM_EASE_OUT_CUBIC:
            result = 1.0f - pow(1.0f - t, 3);
            break;
        // Add more easing implementations here
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
