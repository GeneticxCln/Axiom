#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/smart_gaps.h"
#include "../include/axiom.h"

struct axiom_smart_gaps_manager *axiom_smart_gaps_manager_create(struct axiom_server *server) {
    struct axiom_smart_gaps_manager *manager = calloc(1, sizeof(struct axiom_smart_gaps_manager));
    if (!manager) {
        fprintf(stderr, "Failed to allocate smart gaps manager\n");
        return NULL;
    }
    manager->server = server;
    wl_list_init(&manager->gap_states);
    return manager;
}

void axiom_smart_gaps_manager_destroy(struct axiom_smart_gaps_manager *manager) {
    if (!manager) return;
    wl_list_remove(&manager->gap_states);
    free(manager);
}

bool axiom_smart_gaps_manager_init(struct axiom_smart_gaps_manager *manager, struct axiom_smart_gaps_config *config) {
    if (!manager || !config) return false;
    manager->enabled = config->enabled;
    manager->smart_gaps = config->adaptive_mode && strcmp(config->adaptive_mode, "count") == 0;
    manager->smart_borders = config->adaptive_mode && strcmp(config->adaptive_mode, "density") == 0;
    manager->outer_gaps_smart = manager->smart_gaps;
    manager->profile_count = 0;
    manager->default_profile = NULL;
    
    // Load default and adaptive profiles
    axiom_smart_gaps_load_defaults(manager);
    
    return true;
}

bool axiom_server_init_smart_gaps(struct axiom_server *server, struct axiom_smart_gaps_config *config) {
    server->smart_gaps_manager = axiom_smart_gaps_manager_create(server);
    if (!server->smart_gaps_manager) {
        fprintf(stderr, "Failed to create smart gaps manager\n");
        return false;
    }
    if (!axiom_smart_gaps_manager_init(server->smart_gaps_manager, config)) {
        fprintf(stderr, "Failed to initialize smart gaps manager\n");
        axiom_smart_gaps_manager_destroy(server->smart_gaps_manager);
        server->smart_gaps_manager = NULL;
        return false;
    }
    printf("Smart gaps system initialized\n");
    return true;
}

void axiom_server_destroy_smart_gaps(struct axiom_server *server) {
    if (!server->smart_gaps_manager) return;
    axiom_smart_gaps_manager_destroy(server->smart_gaps_manager);
    server->smart_gaps_manager = NULL;
}

// Gap profile management
bool axiom_smart_gaps_add_profile(struct axiom_smart_gaps_manager *manager, 
                                  const struct axiom_gap_profile *profile) {
    if (!manager || !profile || manager->profile_count >= AXIOM_MAX_GAP_PROFILES) {
        return false;
    }
    
    // Check for duplicate names
    for (int i = 0; i < manager->profile_count; i++) {
        if (strcmp(manager->profiles[i].name, profile->name) == 0) {
            return false; // Profile already exists
        }
    }
    
    // Copy profile
    memcpy(&manager->profiles[manager->profile_count], profile, sizeof(struct axiom_gap_profile));
    manager->profile_count++;
    
    // Set as default if it's the first profile
    if (manager->profile_count == 1) {
        manager->default_profile = &manager->profiles[0];
    }
    
    return true;
}

struct axiom_gap_profile *axiom_smart_gaps_get_profile(struct axiom_smart_gaps_manager *manager, 
                                                       const char *name) {
    if (!manager || !name) return NULL;
    
    for (int i = 0; i < manager->profile_count; i++) {
        if (strcmp(manager->profiles[i].name, name) == 0) {
            return &manager->profiles[i];
        }
    }
    
    return NULL;
}

bool axiom_smart_gaps_remove_profile(struct axiom_smart_gaps_manager *manager, const char *name) {
    if (!manager || !name) return false;
    
    for (int i = 0; i < manager->profile_count; i++) {
        if (strcmp(manager->profiles[i].name, name) == 0) {
            // Shift remaining profiles down
            for (int j = i; j < manager->profile_count - 1; j++) {
                memcpy(&manager->profiles[j], &manager->profiles[j + 1], sizeof(struct axiom_gap_profile));
            }
            manager->profile_count--;
            
            // Update default profile pointer if necessary
            if (manager->default_profile == &manager->profiles[i]) {
                manager->default_profile = manager->profile_count > 0 ? &manager->profiles[0] : NULL;
            }
            
            return true;
        }
    }
    
    return false;
}

void axiom_smart_gaps_set_default_profile(struct axiom_smart_gaps_manager *manager, 
                                          const char *name) {
    if (!manager || !name) return;
    
    struct axiom_gap_profile *profile = axiom_smart_gaps_get_profile(manager, name);
    if (profile) {
        manager->default_profile = profile;
    }
}

// Default profiles creation
bool axiom_smart_gaps_load_defaults(struct axiom_smart_gaps_manager *manager) {
    if (!manager) return false;
    
    // Default profile - basic gaps
    struct axiom_gap_profile default_profile = {
        .name = "default",
        .enabled = true,
        .inner_gap = 10,
        .outer_gap = 5,
        .top_gap = 5,
        .bottom_gap = 5,
        .left_gap = 5,
        .right_gap = 5,
        .adaptation_mode = AXIOM_GAP_STATIC,
        .adaptive = {
            .min_gap = 2,
            .max_gap = 30,
            .scale_factor = 1.0,
            .threshold = 3
        },
        .conditions = {
            .min_windows = 1,
            .max_windows = 99,
            .fullscreen_disable = true,
            .floating_override = false,
            .workspace_pattern = NULL,
            .output_pattern = NULL
        },
        .animation = {
            .enabled = true,
            .duration_ms = 200,
            .easing = AXIOM_GAP_ANIM_EASE_OUT
        }
    };
    
    // Adaptive profile - adjusts based on window count
    struct axiom_gap_profile adaptive_profile = {
        .name = "adaptive",
        .enabled = true,
        .inner_gap = 15,
        .outer_gap = 8,
        .top_gap = 8,
        .bottom_gap = 8,
        .left_gap = 8,
        .right_gap = 8,
        .adaptation_mode = AXIOM_GAP_ADAPTIVE_COUNT,
        .adaptive = {
            .min_gap = 5,
            .max_gap = 25,
            .scale_factor = 0.8,
            .threshold = 4
        },
        .conditions = {
            .min_windows = 2,
            .max_windows = 99,
            .fullscreen_disable = true,
            .floating_override = true,
            .workspace_pattern = NULL,
            .output_pattern = NULL
        },
        .animation = {
            .enabled = true,
            .duration_ms = 300,
            .easing = AXIOM_GAP_ANIM_EASE_IN_OUT
        }
    };
    
    // Compact profile - minimal gaps for small screens
    struct axiom_gap_profile compact_profile = {
        .name = "compact",
        .enabled = true,
        .inner_gap = 5,
        .outer_gap = 2,
        .top_gap = 2,
        .bottom_gap = 2,
        .left_gap = 2,
        .right_gap = 2,
        .adaptation_mode = AXIOM_GAP_ADAPTIVE_DENSITY,
        .adaptive = {
            .min_gap = 1,
            .max_gap = 10,
            .scale_factor = 0.5,
            .threshold = 6
        },
        .conditions = {
            .min_windows = 1,
            .max_windows = 99,
            .fullscreen_disable = true,
            .floating_override = false,
            .workspace_pattern = NULL,
            .output_pattern = NULL
        },
        .animation = {
            .enabled = false,
            .duration_ms = 100,
            .easing = AXIOM_GAP_ANIM_LINEAR
        }
    };
    
    // Spacious profile - large gaps for focus
    struct axiom_gap_profile spacious_profile = {
        .name = "spacious",
        .enabled = true,
        .inner_gap = 25,
        .outer_gap = 15,
        .top_gap = 15,
        .bottom_gap = 15,
        .left_gap = 15,
        .right_gap = 15,
        .adaptation_mode = AXIOM_GAP_ADAPTIVE_FOCUS,
        .adaptive = {
            .min_gap = 15,
            .max_gap = 40,
            .scale_factor = 1.2,
            .threshold = 2
        },
        .conditions = {
            .min_windows = 1,
            .max_windows = 4,
            .fullscreen_disable = true,
            .floating_override = true,
            .workspace_pattern = NULL,
            .output_pattern = NULL
        },
        .animation = {
            .enabled = true,
            .duration_ms = 400,
            .easing = AXIOM_GAP_ANIM_EASE_IN_OUT
        }
    };
    
    // Add all default profiles
    bool success = true;
    success &= axiom_smart_gaps_add_profile(manager, &default_profile);
    success &= axiom_smart_gaps_add_profile(manager, &adaptive_profile);
    success &= axiom_smart_gaps_add_profile(manager, &compact_profile);
    success &= axiom_smart_gaps_add_profile(manager, &spacious_profile);
    
    if (success) {
        printf("Loaded %d default gap profiles\n", manager->profile_count);
    }
    
    return success;
}

// Adaptive gap algorithms
int axiom_smart_gaps_adapt_by_count(const struct axiom_gap_profile *profile, int window_count) {
    if (!profile || window_count <= 0) return profile ? profile->inner_gap : 10;
    
    if (window_count <= profile->adaptive.threshold) {
        return profile->inner_gap;
    }
    
    // Scale down gaps as window count increases
    float factor = 1.0f - ((float)(window_count - profile->adaptive.threshold) * 0.1f * profile->adaptive.scale_factor);
    factor = fmax(0.2f, fmin(1.0f, factor));
    
    int adapted_gap = (int)(profile->inner_gap * factor);
    return fmax(profile->adaptive.min_gap, fmin(profile->adaptive.max_gap, adapted_gap));
}

int axiom_smart_gaps_adapt_by_density(const struct axiom_gap_profile *profile, float density) {
    if (!profile || density <= 0) return profile ? profile->inner_gap : 10;
    
    // Adapt gaps based on screen density (DPI)
    // Higher density = smaller gaps
    float base_density = 96.0f; // Standard DPI
    float density_factor = base_density / density * profile->adaptive.scale_factor;
    
    int adapted_gap = (int)(profile->inner_gap * density_factor);
    return fmax(profile->adaptive.min_gap, fmin(profile->adaptive.max_gap, adapted_gap));
}

int axiom_smart_gaps_adapt_by_focus(const struct axiom_gap_profile *profile, struct axiom_window *focused_window) {
    if (!profile) return 10;
    
    // Increase gaps around focused window for better visual emphasis
    if (focused_window) {
        float focus_factor = 1.0f + (0.2f * profile->adaptive.scale_factor);
        int adapted_gap = (int)(profile->inner_gap * focus_factor);
        return fmax(profile->adaptive.min_gap, fmin(profile->adaptive.max_gap, adapted_gap));
    }
    
    return profile->inner_gap;
}

// Gap calculation and application
bool axiom_smart_gaps_calculate_gaps(struct axiom_smart_gaps_manager *manager,
                                     struct axiom_gap_context *context,
                                     int *inner, int *outer, int *top, int *bottom, int *left, int *right) {
    if (!manager || !context || !inner || !outer || !top || !bottom || !left || !right) {
        return false;
    }
    
    if (!manager->enabled) {
        *inner = *outer = *top = *bottom = *left = *right = 0;
        return true;
    }
    
    // Select appropriate profile
    struct axiom_gap_profile *profile = axiom_smart_gaps_select_profile(manager, context);
    if (!profile) {
        profile = manager->default_profile;
    }
    
    if (!profile) {
        *inner = *outer = *top = *bottom = *left = *right = 10; // Fallback
        return false;
    }
    
    // Handle fullscreen case
    if (context->has_fullscreen && profile->conditions.fullscreen_disable) {
        *inner = *outer = *top = *bottom = *left = *right = 0;
        return true;
    }
    
    // Handle single window case with smart gaps
    if (manager->smart_gaps && context->tiled_windows <= 1) {
        *inner = 0;
        *outer = manager->outer_gaps_smart ? 0 : profile->outer_gap;
        *top = *bottom = *left = *right = *outer;
        return true;
    }
    
    // Calculate adaptive gaps based on profile settings
    int base_inner = profile->inner_gap;
    
    switch (profile->adaptation_mode) {
        case AXIOM_GAP_ADAPTIVE_COUNT:
            base_inner = axiom_smart_gaps_adapt_by_count(profile, context->window_count);
            break;
        case AXIOM_GAP_ADAPTIVE_DENSITY:
            base_inner = axiom_smart_gaps_adapt_by_density(profile, context->density);
            break;
        case AXIOM_GAP_ADAPTIVE_FOCUS:
            base_inner = axiom_smart_gaps_adapt_by_focus(profile, context->focused_window);
            break;
        case AXIOM_GAP_ADAPTIVE_MIXED:
            // Combine multiple adaptation factors
            base_inner = axiom_smart_gaps_adapt_by_count(profile, context->window_count);
            base_inner = (base_inner + axiom_smart_gaps_adapt_by_density(profile, context->density)) / 2;
            break;
        case AXIOM_GAP_STATIC:
        default:
            // Use profile's base gap sizes
            break;
    }
    
    // Apply calculated gaps
    *inner = base_inner;
    *outer = profile->outer_gap;
    *top = profile->top_gap;
    *bottom = profile->bottom_gap;
    *left = profile->left_gap;
    *right = profile->right_gap;
    
    return true;
}

// Profile selection based on context
struct axiom_gap_profile *axiom_smart_gaps_select_profile(struct axiom_smart_gaps_manager *manager,
                                                          struct axiom_gap_context *context) {
    if (!manager || !context) return NULL;
    
    struct axiom_gap_profile *best_match = manager->default_profile;
    int best_score = 0;
    
    for (int i = 0; i < manager->profile_count; i++) {
        struct axiom_gap_profile *profile = &manager->profiles[i];
        
        if (!profile->enabled) continue;
        
        int score = 0;
        
        // Check window count conditions
        if (context->window_count >= profile->conditions.min_windows &&
            context->window_count <= profile->conditions.max_windows) {
            score += 10;
        } else {
            continue; // Hard requirement
        }
        
        // Prefer profiles that match adaptation mode with context
        if (profile->adaptation_mode == AXIOM_GAP_ADAPTIVE_COUNT && context->window_count > 2) {
            score += 5;
        }
        if (profile->adaptation_mode == AXIOM_GAP_ADAPTIVE_DENSITY && context->density != 96.0f) {
            score += 5;
        }
        if (profile->adaptation_mode == AXIOM_GAP_ADAPTIVE_FOCUS && context->focused_window) {
            score += 5;
        }
        
        // TODO: Add workspace and output pattern matching
        
        if (score > best_score) {
            best_score = score;
            best_match = profile;
        }
    }
    
    return best_match;
}

// Gap state management
struct axiom_gap_state *axiom_smart_gaps_get_output_state(struct axiom_smart_gaps_manager *manager,
                                                          struct axiom_output *output) {
    if (!manager || !output) return NULL;
    
    struct axiom_gap_state *state;
    wl_list_for_each(state, &manager->gap_states, link) {
        if (state->output == output) {
            return state;
        }
    }
    
    // Create new state if not found
    state = calloc(1, sizeof(struct axiom_gap_state));
    if (!state) return NULL;
    
    state->output = output;
    state->active_profile = manager->default_profile;
    state->current_inner = state->active_profile ? state->active_profile->inner_gap : 10;
    state->current_outer = state->active_profile ? state->active_profile->outer_gap : 5;
    state->current_top = state->active_profile ? state->active_profile->top_gap : 5;
    state->current_bottom = state->active_profile ? state->active_profile->bottom_gap : 5;
    state->current_left = state->active_profile ? state->active_profile->left_gap : 5;
    state->current_right = state->active_profile ? state->active_profile->right_gap : 5;
    
    wl_list_insert(&manager->gap_states, &state->link);
    
    return state;
}

bool axiom_smart_gaps_update_output_state(struct axiom_smart_gaps_manager *manager,
                                          struct axiom_output *output) {
    if (!manager || !output) return false;
    
    struct axiom_gap_state *state = axiom_smart_gaps_get_output_state(manager, output);
    if (!state) return false;
    
    // Build gap context
    struct axiom_gap_context context = {
        .output = output,
        .gap_state = state,
        .window_count = 0, // TODO: Get actual window count from output
        .tiled_windows = 0, // TODO: Get actual tiled window count
        .floating_windows = 0, // TODO: Get actual floating window count
        .has_fullscreen = false, // TODO: Check for fullscreen windows
        .focused_window = NULL, // TODO: Get focused window
        .screen_width = 1920, // TODO: Get actual screen width
        .screen_height = 1080, // TODO: Get actual screen height
        .density = 96.0f // TODO: Calculate actual DPI
    };
    
    // Calculate new gaps
    int new_gaps[6];
    if (!axiom_smart_gaps_calculate_gaps(manager, &context, 
                                         &new_gaps[0], &new_gaps[1], &new_gaps[2], 
                                         &new_gaps[3], &new_gaps[4], &new_gaps[5])) {
        return false;
    }
    
    // Check if gaps have changed
    bool changed = (new_gaps[0] != state->current_inner ||
                   new_gaps[1] != state->current_outer ||
                   new_gaps[2] != state->current_top ||
                   new_gaps[3] != state->current_bottom ||
                   new_gaps[4] != state->current_left ||
                   new_gaps[5] != state->current_right);
    
    if (!changed) return true;
    
    // Start animation if enabled
    struct axiom_gap_profile *profile = axiom_smart_gaps_select_profile(manager, &context);
    if (profile && profile->animation.enabled) {
        axiom_smart_gaps_start_animation(state, new_gaps, profile->animation.duration_ms);
    } else {
        // Apply immediately
        state->current_inner = new_gaps[0];
        state->current_outer = new_gaps[1];
        state->current_top = new_gaps[2];
        state->current_bottom = new_gaps[3];
        state->current_left = new_gaps[4];
        state->current_right = new_gaps[5];
    }
    
    state->adaptations_count++;
    state->last_adaptation_time = 0; // TODO: Get current time
    
    return true;
}

// Animation system implementation
bool axiom_smart_gaps_start_animation(struct axiom_gap_state *state,
                                      const int target_gaps[6],
                                      uint32_t duration_ms) {
    if (!state || !target_gaps) return false;
    
    // Set up animation state
    state->animation.active = true;
    state->animation.start_time = 0; // TODO: Get current time in milliseconds
    state->animation.duration = duration_ms;
    
    // Store starting values
    state->animation.start_values[0] = state->current_inner;
    state->animation.start_values[1] = state->current_outer;
    state->animation.start_values[2] = state->current_top;
    state->animation.start_values[3] = state->current_bottom;
    state->animation.start_values[4] = state->current_left;
    state->animation.start_values[5] = state->current_right;
    
    // Store target values
    for (int i = 0; i < 6; i++) {
        state->animation.target_values[i] = target_gaps[i];
    }
    
    printf("Started gap animation: duration %u ms\n", duration_ms);
    return true;
}

bool axiom_smart_gaps_update_animation(struct axiom_gap_state *state, uint32_t current_time) {
    if (!state || !state->animation.active) return false;
    
    if (state->animation.start_time == 0) {
        state->animation.start_time = current_time;
        return true;
    }
    
    uint32_t elapsed = current_time - state->animation.start_time;
    if (elapsed >= state->animation.duration) {
        // Animation complete - set final values
        state->current_inner = state->animation.target_values[0];
        state->current_outer = state->animation.target_values[1];
        state->current_top = state->animation.target_values[2];
        state->current_bottom = state->animation.target_values[3];
        state->current_left = state->animation.target_values[4];
        state->current_right = state->animation.target_values[5];
        state->animation.active = false;
        return false; // Animation finished
    }
    
    // Calculate interpolated values
    float progress = (float)elapsed / (float)state->animation.duration;
    // Simple linear interpolation for now
    
    state->current_inner = state->animation.start_values[0] + 
                          (int)((state->animation.target_values[0] - state->animation.start_values[0]) * progress);
    state->current_outer = state->animation.start_values[1] + 
                          (int)((state->animation.target_values[1] - state->animation.start_values[1]) * progress);
    state->current_top = state->animation.start_values[2] + 
                        (int)((state->animation.target_values[2] - state->animation.start_values[2]) * progress);
    state->current_bottom = state->animation.start_values[3] + 
                           (int)((state->animation.target_values[3] - state->animation.start_values[3]) * progress);
    state->current_left = state->animation.start_values[4] + 
                         (int)((state->animation.target_values[4] - state->animation.start_values[4]) * progress);
    state->current_right = state->animation.start_values[5] + 
                          (int)((state->animation.target_values[5] - state->animation.start_values[5]) * progress);
    
    return true; // Animation continues
}

bool axiom_smart_gaps_is_animating(const struct axiom_gap_state *state) {
    return state && state->animation.active;
}
