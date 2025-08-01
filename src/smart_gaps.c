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

bool axiom_smart_gaps_manager_init(struct axiom_smart_gaps_manager *manager) {
    if (!manager) return false;
    manager->enabled = true;
    manager->smart_gaps = true;
    manager->smart_borders = true;
    manager->outer_gaps_smart = true;
    manager->profile_count = 0;
    manager->default_profile = NULL;
    return true;
}

bool axiom_server_init_smart_gaps(struct axiom_server *server) {
    server->smart_gaps_manager = axiom_smart_gaps_manager_create(server);
    if (!server->smart_gaps_manager) {
        fprintf(stderr, "Failed to create smart gaps manager\n");
        return false;
    }
    if (!axiom_smart_gaps_manager_init(server->smart_gaps_manager)) {
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
