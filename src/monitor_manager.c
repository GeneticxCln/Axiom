#include "monitor_manager.h"
#include "axiom.h"
#include "logging.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wlr/types/wlr_output_damage.h>
#include <wlr/util/log.h>

// Monitor manager creation
struct axiom_monitor_manager *axiom_monitor_manager_create(struct axiom_server *server) {
    AXIOM_LOG(AXIOM_LOG_INFO, "Creating monitor manager");
    
    struct axiom_monitor_manager *manager = calloc(1, sizeof(struct axiom_monitor_manager));
    if (!manager) {
        AXIOM_LOG(AXIOM_LOG_ERROR, "Failed to allocate monitor manager");
        return NULL;
    }
    
    manager->server = server;
    wl_list_init(&manager->monitors);
    wl_list_init(&manager->disabled_monitors);
    wl_list_init(&manager->monitor_rules);
    
    // Create output layout
    manager->layout = wlr_output_layout_create();
    if (!manager->layout) {
        AXIOM_LOG(AXIOM_LOG_ERROR, "Failed to create output layout");
        free(manager);
        return NULL;
    }
    
    // Initialize configuration
    manager->auto_configure = true;
    manager->default_scale = 1.0f;
    manager->next_monitor_id = 1;
    manager->in_unsafe_state = false;
    
    // Setup event listeners
    manager->new_output.notify = axiom_monitor_handle_new_output;
    wl_signal_add(&server->backend->events.new_output, &manager->new_output);
    
    manager->layout_change.notify = axiom_monitor_handle_layout_change;
    wl_signal_add(&manager->layout->events.change, &manager->layout_change);
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Monitor manager created successfully");
    return manager;
}

void axiom_monitor_manager_destroy(struct axiom_monitor_manager *manager) {
    if (!manager) return;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Destroying monitor manager");
    
    // Clean up monitors
    struct axiom_monitor *monitor, *tmp;
    wl_list_for_each_safe(monitor, tmp, &manager->monitors, link) {
        axiom_monitor_destroy(monitor);
    }
    
    wl_list_for_each_safe(monitor, tmp, &manager->disabled_monitors, link) {
        axiom_monitor_destroy(monitor);
    }
    
    // Clean up rules
    struct axiom_monitor_rule *rule, *rule_tmp;
    wl_list_for_each_safe(rule, rule_tmp, &manager->monitor_rules, link) {
        axiom_monitor_rule_destroy(rule);
    }
    
    // Clean up layout
    if (manager->layout) {
        wlr_output_layout_destroy(manager->layout);
    }
    
    // Remove event listeners
    wl_list_remove(&manager->new_output.link);
    wl_list_remove(&manager->layout_change.link);
    
    free(manager);
    AXIOM_LOG(AXIOM_LOG_INFO, "Monitor manager destroyed");
}

// Monitor creation
struct axiom_monitor *axiom_monitor_create(struct axiom_monitor_manager *manager, 
                                          struct wlr_output *wlr_output) {
    AXIOM_LOG(AXIOM_LOG_INFO, "Creating monitor: %s", wlr_output->name);
    
    struct axiom_monitor *monitor = calloc(1, sizeof(struct axiom_monitor));
    if (!monitor) {
        AXIOM_LOG(AXIOM_LOG_ERROR, "Failed to allocate monitor");
        return NULL;
    }
    
    monitor->server = manager->server;
    monitor->wlr_output = wlr_output;
    monitor->id = manager->next_monitor_id++;
    
    // Copy basic properties
    monitor->name = strdup(wlr_output->name);
    monitor->description = strdup(wlr_output->description ? wlr_output->description : "Unknown");
    monitor->manufacturer = strdup(wlr_output->make ? wlr_output->make : "Unknown");
    monitor->model = strdup(wlr_output->model ? wlr_output->model : "Unknown");
    
    // Initialize geometry
    monitor->width = wlr_output->width;
    monitor->height = wlr_output->height;
    monitor->phys_width = wlr_output->phys_width;
    monitor->phys_height = wlr_output->phys_height;
    monitor->scale = wlr_output->scale;
    monitor->refresh_rate = wlr_output->refresh;
    monitor->transform = wlr_output->transform;
    
    // Initialize state
    monitor->state.owner = monitor;
    monitor->enabled = wlr_output->enabled;
    monitor->connected = true;
    monitor->dpms_enabled = true;
    
    // Initialize lists
    wl_list_init(&monitor->mirrors);
    
    // Setup damage tracking
    wlr_damage_ring_init(&monitor->damage_ring);
    
    // Create scene output
    monitor->scene_output = wlr_scene_output_create(manager->server->scene, wlr_output);
    if (!monitor->scene_output) {
        AXIOM_LOG(AXIOM_LOG_ERROR, "Failed to create scene output for monitor %s", monitor->name);
        axiom_monitor_destroy(monitor);
        return NULL;
    }
    
    // Setup event listeners
    monitor->frame.notify = axiom_monitor_handle_frame;
    wl_signal_add(&wlr_output->events.frame, &monitor->frame);
    
    monitor->destroy.notify = axiom_monitor_handle_destroy;
    wl_signal_add(&wlr_output->events.destroy, &monitor->destroy);
    
    monitor->mode.notify = axiom_monitor_handle_mode;
    wl_signal_add(&wlr_output->events.mode, &monitor->mode);
    
    monitor->enable.notify = axiom_monitor_handle_enable;
    wl_signal_add(&wlr_output->events.enable, &monitor->enable);
    
    monitor->present.notify = axiom_monitor_handle_present;
    wl_signal_add(&wlr_output->events.present, &monitor->present);
    
    // Add to monitor list
    wl_list_insert(&manager->monitors, &monitor->link);
    manager->total_monitors++;
    if (monitor->enabled) {
        manager->active_monitors++;
    }
    
    // Set as primary if first monitor
    if (!manager->primary_monitor && monitor->enabled) {
        axiom_monitor_manager_set_primary(manager, monitor);
    }
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Monitor created: %s (%dx%d@%.2fHz)", 
              monitor->name, monitor->width, monitor->height, monitor->refresh_rate);
    
    return monitor;
}

void axiom_monitor_destroy(struct axiom_monitor *monitor) {
    if (!monitor) return;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Destroying monitor: %s", monitor->name);
    
    // Remove from lists
    wl_list_remove(&monitor->link);
    
    // Clean up mirroring
    if (monitor->mirror_of) {
        wl_list_remove(&monitor->link); // Remove from mirror list
    }
    
    struct axiom_monitor *mirror, *tmp;
    wl_list_for_each_safe(mirror, tmp, &monitor->mirrors, link) {
        mirror->mirror_of = NULL;
        wl_list_remove(&mirror->link);
    }
    
    // Remove event listeners
    wl_list_remove(&monitor->frame.link);
    wl_list_remove(&monitor->destroy.link);
    wl_list_remove(&monitor->mode.link);
    wl_list_remove(&monitor->enable.link);
    wl_list_remove(&monitor->present.link);
    
    // Clean up damage ring
    wlr_damage_ring_finish(&monitor->damage_ring);
    
    // Free strings
    free(monitor->name);
    free(monitor->description);
    free(monitor->manufacturer);
    free(monitor->model);
    
    free(monitor);
}

// Monitor configuration functions
bool axiom_monitor_apply_rule(struct axiom_monitor *monitor, struct axiom_monitor_rule *rule) {
    if (!monitor || !rule) return false;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Applying rule to monitor %s", monitor->name);
    
    // Copy rule to active rule
    monitor->active_rule = *rule;
    
    // Apply configuration
    if (rule->width > 0 && rule->height > 0) {
        axiom_monitor_set_mode(monitor, rule->width, rule->height, rule->refresh_rate);
    }
    
    if (rule->scale > 0) {
        axiom_monitor_set_scale(monitor, rule->scale);
    }
    
    if (rule->transform != WL_OUTPUT_TRANSFORM_NORMAL) {
        axiom_monitor_set_transform(monitor, rule->transform);
    }
    
    // Set position
    axiom_monitor_set_position(monitor, rule->x, rule->y);
    
    // Handle mirroring
    if (rule->mirror_of && strlen(rule->mirror_of) > 0) {
        struct axiom_monitor_manager *manager = 
            (struct axiom_monitor_manager *)monitor->server->monitor_manager;
        struct axiom_monitor *source = axiom_monitor_from_name(manager, rule->mirror_of);
        if (source) {
            axiom_monitor_set_mirror(monitor, source);
        }
    }
    
    // Handle disable
    if (rule->disabled) {
        axiom_monitor_disable(monitor);
    } else {
        axiom_monitor_enable(monitor);
    }
    
    return true;
}

void axiom_monitor_set_mode(struct axiom_monitor *monitor, int width, int height, float refresh) {
    if (!monitor || !monitor->wlr_output) return;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Setting mode for monitor %s: %dx%d@%.2fHz", 
              monitor->name, width, height, refresh);
    
    struct wlr_output_mode *mode = wlr_output_preferred_mode(monitor->wlr_output);
    struct wlr_output_mode *best_mode = NULL;
    
    // Find best matching mode
    wl_list_for_each(mode, &monitor->wlr_output->modes, link) {
        if (mode->width == width && mode->height == height) {
            if (refresh <= 0 || abs(mode->refresh - (int)(refresh * 1000)) < 100) {
                best_mode = mode;
                break;
            }
        }
    }
    
    if (best_mode) {
        wlr_output_set_mode(monitor->wlr_output, best_mode);
        monitor->width = best_mode->width;
        monitor->height = best_mode->height;
        monitor->refresh_rate = best_mode->refresh / 1000.0f;
    } else {
        AXIOM_LOG(AXIOM_LOG_WARN, "No suitable mode found for %dx%d@%.2fHz on monitor %s",
                  width, height, refresh, monitor->name);
    }
}

void axiom_monitor_set_scale(struct axiom_monitor *monitor, float scale) {
    if (!monitor || !monitor->wlr_output) return;
    
    if (scale <= 0 || scale > 4.0f) {
        AXIOM_LOG(AXIOM_LOG_WARN, "Invalid scale factor %.2f for monitor %s", scale, monitor->name);
        return;
    }
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Setting scale for monitor %s: %.2f", monitor->name, scale);
    
    wlr_output_set_scale(monitor->wlr_output, scale);
    monitor->scale = scale;
}

void axiom_monitor_set_transform(struct axiom_monitor *monitor, enum wl_output_transform transform) {
    if (!monitor || !monitor->wlr_output) return;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Setting transform for monitor %s: %d", monitor->name, transform);
    
    wlr_output_set_transform(monitor->wlr_output, transform);
    monitor->transform = transform;
}

void axiom_monitor_set_position(struct axiom_monitor *monitor, int x, int y) {
    if (!monitor) return;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Setting position for monitor %s: %d,%d", monitor->name, x, y);
    
    monitor->x = x;
    monitor->y = y;
    
    // Update output layout
    struct axiom_monitor_manager *manager = 
        (struct axiom_monitor_manager *)monitor->server->monitor_manager;
    if (manager && manager->layout) {
        wlr_output_layout_add(manager->layout, monitor->wlr_output, x, y);
        manager->layout_changed = true;
    }
}

// Monitor queries
struct axiom_monitor *axiom_monitor_from_name(struct axiom_monitor_manager *manager, const char *name) {
    if (!manager || !name) return NULL;
    
    struct axiom_monitor *monitor;
    wl_list_for_each(monitor, &manager->monitors, link) {
        if (strcmp(monitor->name, name) == 0) {
            return monitor;
        }
    }
    
    return NULL;
}

struct axiom_monitor *axiom_monitor_from_id(struct axiom_monitor_manager *manager, uint32_t id) {
    if (!manager) return NULL;
    
    struct axiom_monitor *monitor;
    wl_list_for_each(monitor, &manager->monitors, link) {
        if (monitor->id == id) {
            return monitor;
        }
    }
    
    return NULL;
}

struct axiom_monitor *axiom_monitor_from_wlr_output(struct axiom_monitor_manager *manager, 
                                                   struct wlr_output *wlr_output) {
    if (!manager || !wlr_output) return NULL;
    
    struct axiom_monitor *monitor;
    wl_list_for_each(monitor, &manager->monitors, link) {
        if (monitor->wlr_output == wlr_output) {
            return monitor;
        }
    }
    
    return NULL;
}

struct axiom_monitor *axiom_monitor_from_cursor(struct axiom_monitor_manager *manager) {
    if (!manager || !manager->server->cursor) return manager->primary_monitor;
    
    struct wlr_output *wlr_output = wlr_output_layout_output_at(manager->layout,
                                                               manager->server->cursor->x,
                                                               manager->server->cursor->y);
    
    if (wlr_output) {
        return axiom_monitor_from_wlr_output(manager, wlr_output);
    }
    
    return manager->primary_monitor;
}

struct axiom_monitor *axiom_monitor_from_point(struct axiom_monitor_manager *manager, 
                                               double x, double y) {
    if (!manager) return NULL;
    
    struct wlr_output *wlr_output = wlr_output_layout_output_at(manager->layout, x, y);
    
    if (wlr_output) {
        return axiom_monitor_from_wlr_output(manager, wlr_output);
    }
    
    return NULL;
}

// Monitor enable/disable
bool axiom_monitor_enable(struct axiom_monitor *monitor) {
    if (!monitor || !monitor->wlr_output) return false;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Enabling monitor: %s", monitor->name);
    
    if (!wlr_output_init_render(monitor->wlr_output, 
                               monitor->server->allocator, 
                               monitor->server->renderer)) {
        AXIOM_LOG(AXIOM_LOG_ERROR, "Failed to initialize render for monitor %s", monitor->name);
        return false;
    }
    
    // Try to set preferred mode
    struct wlr_output_mode *mode = wlr_output_preferred_mode(monitor->wlr_output);
    if (mode) {
        wlr_output_set_mode(monitor->wlr_output, mode);
    }
    
    wlr_output_enable(monitor->wlr_output, true);
    
    if (!wlr_output_commit(monitor->wlr_output)) {
        AXIOM_LOG(AXIOM_LOG_ERROR, "Failed to commit monitor %s", monitor->name);
        return false;
    }
    
    monitor->enabled = true;
    
    // Move from disabled list to active list if needed
    struct axiom_monitor_manager *manager = 
        (struct axiom_monitor_manager *)monitor->server->monitor_manager;
    
    bool was_in_disabled = false;
    struct axiom_monitor *m;
    wl_list_for_each(m, &manager->disabled_monitors, link) {
        if (m == monitor) {
            was_in_disabled = true;
            break;
        }
    }
    
    if (was_in_disabled) {
        wl_list_remove(&monitor->link);
        wl_list_insert(&manager->monitors, &monitor->link);
        manager->active_monitors++;
    }
    
    // Add to layout
    wlr_output_layout_add_auto(manager->layout, monitor->wlr_output);
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Monitor %s enabled successfully", monitor->name);
    return true;
}

void axiom_monitor_disable(struct axiom_monitor *monitor) {
    if (!monitor || !monitor->wlr_output) return;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Disabling monitor: %s", monitor->name);
    
    wlr_output_enable(monitor->wlr_output, false);
    wlr_output_commit(monitor->wlr_output);
    
    monitor->enabled = false;
    
    // Move to disabled list
    struct axiom_monitor_manager *manager = 
        (struct axiom_monitor_manager *)monitor->server->monitor_manager;
    
    wl_list_remove(&monitor->link);
    wl_list_insert(&manager->disabled_monitors, &monitor->link);
    manager->active_monitors--;
    
    // Remove from layout
    wlr_output_layout_remove(manager->layout, monitor->wlr_output);
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Monitor %s disabled", monitor->name);
}

// Primary monitor management
void axiom_monitor_manager_set_primary(struct axiom_monitor_manager *manager, 
                                       struct axiom_monitor *monitor) {
    if (!manager || !monitor) return;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Setting primary monitor: %s", monitor->name);
    
    if (manager->primary_monitor) {
        manager->primary_monitor->is_primary = false;
    }
    
    manager->primary_monitor = monitor;
    monitor->is_primary = true;
}

struct axiom_monitor *axiom_monitor_manager_get_primary(struct axiom_monitor_manager *manager) {
    return manager ? manager->primary_monitor : NULL;
}

// Layout management
void axiom_monitor_manager_arrange_monitors(struct axiom_monitor_manager *manager) {
    if (!manager) return;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Arranging monitors");
    
    struct axiom_monitor *monitor;
    wl_list_for_each(monitor, &manager->monitors, link) {
        if (!monitor->enabled) continue;
        
        // Apply auto-positioning if configured
        if (monitor->auto_dir != AXIOM_DIR_AUTO_NONE) {
            // Implement auto-positioning logic here
            // This would calculate positions based on auto_dir
        }
    }
    
    manager->layout_changed = true;
    manager->last_recalc_time = time(NULL);
}

void axiom_monitor_manager_recalculate_layout(struct axiom_monitor_manager *manager) {
    if (!manager) return;
    
    AXIOM_LOG(AXIOM_LOG_DEBUG, "Recalculating monitor layout");
    
    // Trigger workspace recalculation on all monitors
    struct axiom_monitor *monitor;
    wl_list_for_each(monitor, &manager->monitors, link) {
        if (monitor->enabled && monitor->active_workspace) {
            // Trigger workspace layout recalculation
            // This would call into the layout manager
        }
    }
    
    manager->layout_changed = false;
}

// Performance functions
void axiom_monitor_schedule_frame(struct axiom_monitor *monitor) {
    if (!monitor || !monitor->wlr_output) return;
    
    wlr_output_schedule_frame(monitor->wlr_output);
    monitor->schedule_frame_pending = true;
}

void axiom_monitor_update_damage(struct axiom_monitor *monitor, struct wlr_box *damage) {
    if (!monitor || !damage) return;
    
    wlr_damage_ring_add_box(&monitor->damage_ring, damage);
    monitor->needs_frame = true;
}

// Event handlers
void axiom_monitor_handle_frame(struct wl_listener *listener, void *data) {
    struct axiom_monitor *monitor = wl_container_of(listener, monitor, frame);
    struct wlr_scene_output *scene_output = monitor->scene_output;
    
    struct wlr_scene_output_state_options opts = {0};
    struct wlr_scene_output_state state;
    wlr_scene_output_state_init(&state, scene_output, &opts);
    
    wlr_scene_output_commit(scene_output, &state);
    
    wlr_scene_output_state_finish(&state);
    
    // Update performance tracking
    monitor->frame_count++;
    monitor->last_frame_time = time(NULL);
    monitor->schedule_frame_pending = false;
}

void axiom_monitor_handle_destroy(struct wl_listener *listener, void *data) {
    struct axiom_monitor *monitor = wl_container_of(listener, monitor, destroy);
    AXIOM_LOG(AXIOM_LOG_INFO, "Monitor destroyed: %s", monitor->name);
    axiom_monitor_destroy(monitor);
}

void axiom_monitor_handle_mode(struct wl_listener *listener, void *data) {
    struct axiom_monitor *monitor = wl_container_of(listener, monitor, mode);
    AXIOM_LOG(AXIOM_LOG_INFO, "Monitor mode changed: %s", monitor->name);
    
    // Update monitor properties
    monitor->width = monitor->wlr_output->width;
    monitor->height = monitor->wlr_output->height;
    monitor->refresh_rate = monitor->wlr_output->refresh / 1000.0f;
    
    // Trigger layout recalculation
    struct axiom_monitor_manager *manager = 
        (struct axiom_monitor_manager *)monitor->server->monitor_manager;
    if (manager) {
        axiom_monitor_manager_recalculate_layout(manager);
    }
}

void axiom_monitor_handle_enable(struct wl_listener *listener, void *data) {
    struct axiom_monitor *monitor = wl_container_of(listener, monitor, enable);
    AXIOM_LOG(AXIOM_LOG_INFO, "Monitor enable state changed: %s -> %s", 
              monitor->name, monitor->wlr_output->enabled ? "enabled" : "disabled");
    
    monitor->enabled = monitor->wlr_output->enabled;
    
    struct axiom_monitor_manager *manager = 
        (struct axiom_monitor_manager *)monitor->server->monitor_manager;
    if (manager) {
        if (monitor->enabled) {
            manager->active_monitors++;
        } else {
            manager->active_monitors--;
        }
        
        axiom_monitor_manager_recalculate_layout(manager);
    }
}

void axiom_monitor_handle_present(struct wl_listener *listener, void *data) {
    struct axiom_monitor *monitor = wl_container_of(listener, monitor, present);
    
    // Update performance metrics
    uint64_t current_time = time(NULL);
    if (monitor->last_frame_time > 0) {
        float frame_time = (float)(current_time - monitor->last_frame_time) / 1000.0f;
        monitor->avg_frame_time = (monitor->avg_frame_time * 0.9f) + (frame_time * 0.1f);
    }
}

// New output handler
void axiom_monitor_handle_new_output(struct wl_listener *listener, void *data) {
    struct axiom_monitor_manager *manager = wl_container_of(listener, manager, new_output);
    struct wlr_output *wlr_output = data;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "New output detected: %s", wlr_output->name);
    
    struct axiom_monitor *monitor = axiom_monitor_create(manager, wlr_output);
    if (!monitor) {
        AXIOM_LOG(AXIOM_LOG_ERROR, "Failed to create monitor for output %s", wlr_output->name);
        return;
    }
    
    // Try to apply matching rule
    struct axiom_monitor_rule *rule;
    wl_list_for_each(rule, &manager->monitor_rules, link) {
        if (axiom_monitor_rule_matches(rule, monitor)) {
            axiom_monitor_apply_rule(monitor, rule);
            break;
        }
    }
    
    // Enable monitor if no rule disabled it
    if (!monitor->active_rule.disabled) {
        axiom_monitor_enable(monitor);
    }
    
    // Arrange monitors
    axiom_monitor_manager_arrange_monitors(manager);
}

// Layout change handler
void axiom_monitor_handle_layout_change(struct wl_listener *listener, void *data) {
    struct axiom_monitor_manager *manager = wl_container_of(listener, manager, layout_change);
    AXIOM_LOG(AXIOM_LOG_DEBUG, "Monitor layout changed");
    
    axiom_monitor_manager_recalculate_layout(manager);
}

// Monitor rule functions
struct axiom_monitor_rule *axiom_monitor_rule_create(void) {
    struct axiom_monitor_rule *rule = calloc(1, sizeof(struct axiom_monitor_rule));
    if (!rule) return NULL;
    
    // Set defaults
    rule->auto_dir = AXIOM_DIR_AUTO_NONE;
    rule->width = -1;
    rule->height = -1;
    rule->scale = 1.0f;
    rule->refresh_rate = 60.0f;
    rule->transform = WL_OUTPUT_TRANSFORM_NORMAL;
    rule->disabled = false;
    rule->enable_10bit = false;
    rule->enable_vrr = false;
    rule->primary = false;
    
    return rule;
}

void axiom_monitor_rule_destroy(struct axiom_monitor_rule *rule) {
    if (!rule) return;
    
    free(rule->name);
    free(rule->mirror_of);
    free(rule);
}

bool axiom_monitor_rule_matches(struct axiom_monitor_rule *rule, struct axiom_monitor *monitor) {
    if (!rule || !monitor) return false;
    
    // Match by name pattern
    if (rule->name && strlen(rule->name) > 0) {
        if (strcmp(rule->name, monitor->name) == 0 || strcmp(rule->name, "*") == 0) {
            return true;
        }
    }
    
    return false;
}

// Debug functions
void axiom_monitor_print_info(struct axiom_monitor *monitor) {
    if (!monitor) return;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Monitor Info: %s", monitor->name);
    AXIOM_LOG(AXIOM_LOG_INFO, "  ID: %u", monitor->id);
    AXIOM_LOG(AXIOM_LOG_INFO, "  Resolution: %dx%d", monitor->width, monitor->height);
    AXIOM_LOG(AXIOM_LOG_INFO, "  Position: %d,%d", monitor->x, monitor->y);
    AXIOM_LOG(AXIOM_LOG_INFO, "  Scale: %.2f", monitor->scale);
    AXIOM_LOG(AXIOM_LOG_INFO, "  Refresh: %.2fHz", monitor->refresh_rate);
    AXIOM_LOG(AXIOM_LOG_INFO, "  Enabled: %s", monitor->enabled ? "yes" : "no");
    AXIOM_LOG(AXIOM_LOG_INFO, "  Primary: %s", monitor->is_primary ? "yes" : "no");
    AXIOM_LOG(AXIOM_LOG_INFO, "  Manufacturer: %s", monitor->manufacturer);
    AXIOM_LOG(AXIOM_LOG_INFO, "  Model: %s", monitor->model);
}

void axiom_monitor_manager_print_layout(struct axiom_monitor_manager *manager) {
    if (!manager) return;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Monitor Layout:");
    AXIOM_LOG(AXIOM_LOG_INFO, "  Total monitors: %u", manager->total_monitors);
    AXIOM_LOG(AXIOM_LOG_INFO, "  Active monitors: %u", manager->active_monitors);
    AXIOM_LOG(AXIOM_LOG_INFO, "  Primary: %s", 
              manager->primary_monitor ? manager->primary_monitor->name : "none");
    
    struct axiom_monitor *monitor;
    wl_list_for_each(monitor, &manager->monitors, link) {
        AXIOM_LOG(AXIOM_LOG_INFO, "  Monitor: %s (%dx%d@%d,%d) %s", 
                  monitor->name, monitor->width, monitor->height, 
                  monitor->x, monitor->y, monitor->enabled ? "enabled" : "disabled");
    }
}

void axiom_monitor_manager_validate_state(struct axiom_monitor_manager *manager) {
    if (!manager) return;
    
    // Count active monitors
    uint32_t active_count = 0;
    struct axiom_monitor *monitor;
    wl_list_for_each(monitor, &manager->monitors, link) {
        if (monitor->enabled) active_count++;
    }
    
    if (active_count != manager->active_monitors) {
        AXIOM_LOG(AXIOM_LOG_WARN, "Monitor count mismatch: counted %u, stored %u", 
                  active_count, manager->active_monitors);
        manager->active_monitors = active_count;
    }
    
    // Check for unsafe state
    if (manager->active_monitors == 0 && !manager->in_unsafe_state) {
        AXIOM_LOG(AXIOM_LOG_WARN, "No active monitors detected, entering unsafe state");
        axiom_monitor_manager_enter_unsafe_state(manager);
    } else if (manager->active_monitors > 0 && manager->in_unsafe_state) {
        AXIOM_LOG(AXIOM_LOG_INFO, "Active monitors detected, leaving unsafe state");
        axiom_monitor_manager_leave_unsafe_state(manager);
    }
}

// Safety state management
void axiom_monitor_manager_enter_unsafe_state(struct axiom_monitor_manager *manager) {
    if (!manager || manager->in_unsafe_state) return;
    
    AXIOM_LOG(AXIOM_LOG_WARN, "Entering unsafe state - no active monitors");
    
    manager->in_unsafe_state = true;
    
    // Create fallback monitor if needed
    if (!manager->unsafe_fallback) {
        axiom_monitor_manager_create_fallback_monitor(manager);
    }
}

void axiom_monitor_manager_leave_unsafe_state(struct axiom_monitor_manager *manager) {
    if (!manager || !manager->in_unsafe_state) return;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Leaving unsafe state - monitors available");
    
    manager->in_unsafe_state = false;
    
    // Clean up fallback monitor
    if (manager->unsafe_fallback) {
        axiom_monitor_destroy(manager->unsafe_fallback);
        manager->unsafe_fallback = NULL;
    }
}

bool axiom_monitor_manager_create_fallback_monitor(struct axiom_monitor_manager *manager) {
    if (!manager) return false;
    
    AXIOM_LOG(AXIOM_LOG_INFO, "Creating fallback monitor for unsafe state");
    
    // This would create a virtual/fallback output for emergency situations
    // Implementation depends on wlroots backend capabilities
    
    return true;
}
