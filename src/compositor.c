#include "compositor.h"
#include "axiom.h"
#include "config.h"
#include "animation.h"
#include "effects.h"
#include "window_manager.h"
#include "enhanced_xwayland.h"
#include "logging.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/util/log.h>
#include <wlr/backend/wayland.h>
#include <wlr/backend/session.h>
#include <wlr/backend/multi.h>
#include <wlr/backend/drm.h>
#include <wlr/backend/libinput.h>

// Forward declarations for event handlers
void axiom_new_output(struct wl_listener *listener, void *data);
void axiom_new_xdg_toplevel(struct wl_listener *listener, void *data);
void axiom_request_cursor(struct wl_listener *listener, void *data);
void axiom_request_set_selection(struct wl_listener *listener, void *data);
void axiom_backend_destroy(struct wl_listener *listener, void *data);

// Helper function to get current time in milliseconds
static uint32_t get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

bool axiom_compositor_init(struct axiom_server *server, bool nested) {
    if (!server) {
        AXIOM_LOG_ERROR("Server is NULL");
        return false;
    }

    AXIOM_LOG_INFO("Initializing Axiom compositor v%s", AXIOM_VERSION);
    
    // Initialize basic Wayland compositor
    server->wl_display = wl_display_create();
    if (!server->wl_display) {
        AXIOM_LOG_ERROR("Failed to create Wayland display");
        return false;
    }

    server->wl_event_loop = wl_display_get_event_loop(server->wl_display);
    
    // Create backend - THIS IS THE CRITICAL FIX
    if (nested) {
        AXIOM_LOG_INFO("Creating nested Wayland backend");
        // For nested mode, create a Wayland backend that connects to parent compositor
        server->backend = wlr_wl_backend_create(server->wl_event_loop, NULL);
    } else {
        AXIOM_LOG_INFO("Creating native backend for primary display server");
        // For primary display server, we need session + DRM + libinput
        
        // First create session for device access
        server->session = wlr_session_create(server->wl_event_loop);
        if (!server->session) {
            AXIOM_LOG_ERROR("Failed to create session - are you running from TTY?");
            AXIOM_LOG_ERROR("Primary mode requires running from TTY (Ctrl+Alt+F2)");
            wl_display_destroy(server->wl_display);
            return false;
        }
        
        // Create multi-backend to combine DRM + libinput
        server->backend = wlr_multi_backend_create(server->wl_event_loop);
        if (!server->backend) {
            AXIOM_LOG_ERROR("Failed to create multi backend");
            wlr_session_destroy(server->session);
            wl_display_destroy(server->wl_display);
            return false;
        }
        
        // Find and add DRM devices
        struct wlr_device *gpus[8];
        ssize_t num_gpus = wlr_session_find_gpus(server->session, 8, gpus);
        if (num_gpus < 0) {
            AXIOM_LOG_ERROR("Failed to find GPU devices");
            wlr_session_destroy(server->session);
            wl_display_destroy(server->wl_display);
            return false;
        }
        
        AXIOM_LOG_INFO("Found %zd GPU device(s)", num_gpus);
        
        // Add DRM backend for each GPU
        for (ssize_t i = 0; i < num_gpus; ++i) {
            struct wlr_backend *drm = wlr_drm_backend_create(server->session, gpus[i], NULL);
            if (!drm) {
                AXIOM_LOG_ERROR("Failed to create DRM backend for device %zd", i);
                continue;
            }
            wlr_multi_backend_add(server->backend, drm);
            AXIOM_LOG_INFO("Added DRM backend for device %zd", i);
        }
        
        // Add libinput backend for input devices
        struct wlr_backend *libinput = wlr_libinput_backend_create(server->session);
        if (!libinput) {
            AXIOM_LOG_ERROR("Failed to create libinput backend");
        } else {
            wlr_multi_backend_add(server->backend, libinput);
            AXIOM_LOG_INFO("Added libinput backend");
        }
        
        // Check if we have any backends
        if (wlr_multi_is_empty(server->backend)) {
            AXIOM_LOG_ERROR("No backends available - check permissions and hardware");
            wlr_session_destroy(server->session);
            wl_display_destroy(server->wl_display);
            return false;
        }
    }
    
    if (!server->backend) {
        AXIOM_LOG_ERROR("Failed to create backend");
        wl_display_destroy(server->wl_display);
        return false;
    }

    // Create renderer
    server->renderer = wlr_renderer_autocreate(server->backend);
    if (!server->renderer) {
        AXIOM_LOG_ERROR("Failed to create renderer");
        wl_display_destroy(server->wl_display);
        return false;
    }

    if (!wlr_renderer_init_wl_display(server->renderer, server->wl_display)) {
        AXIOM_LOG_ERROR("Failed to initialize renderer with display");
        wl_display_destroy(server->wl_display);
        return false;
    }

    // Create basic compositor protocols
    server->allocator = wlr_allocator_autocreate(server->backend, server->renderer);
    server->compositor = wlr_compositor_create(server->wl_display, 5, server->renderer);
    server->scene = wlr_scene_create();
    server->output_layout = wlr_output_layout_create(server->wl_display);
    server->scene_layout = wlr_scene_attach_output_layout(server->scene, server->output_layout);

    // Initialize window lists
    wl_list_init(&server->windows);
    wl_list_init(&server->outputs);
    wl_list_init(&server->input_devices);

    // Set up XDG shell
    server->xdg_shell = wlr_xdg_shell_create(server->wl_display, 3);
    if (!server->xdg_shell) {
        AXIOM_LOG_ERROR("Failed to create XDG shell");
        return false;
    }

    // Create seat
    server->seat = wlr_seat_create(server->wl_display, "seat0");
    server->data_device_manager = wlr_data_device_manager_create(server->wl_display);

    // Create cursor (don't attach to output layout yet)
    server->cursor = wlr_cursor_create();
    if (!server->cursor) {
        AXIOM_LOG_ERROR("Failed to create cursor");
        return false;
    }

    server->cursor_mode = AXIOM_CURSOR_PASSTHROUGH;

    // Initialize configuration
    server->config = axiom_config_create();
    if (!server->config) {
        AXIOM_LOG_WARN("Failed to create configuration, using defaults");
    }
    
    // Initialize animation manager
    axiom_animation_manager_init(server);
    if (!server->animation_manager) {
        AXIOM_LOG_WARN("Failed to initialize animation manager");
    }
    
    // Initialize effects manager
    server->effects_manager = calloc(1, sizeof(struct axiom_effects_manager));
    if (server->effects_manager) {
        struct axiom_effects_config *effects_config = server->config ? &server->config->effects : NULL;
        if (!axiom_effects_manager_init(server->effects_manager, effects_config)) {
            AXIOM_LOG_WARN("Failed to initialize effects manager");
            free(server->effects_manager);
            server->effects_manager = NULL;
        }
    }
    
    // Initialize window manager
    server->window_manager = axiom_window_manager_create(server);
    if (!server->window_manager) {
        AXIOM_LOG_WARN("Failed to initialize window manager");
    }
    
    // Initialize enhanced XWayland manager
    server->enhanced_xwayland_manager = axiom_enhanced_xwayland_manager_create(server);
    if (!server->enhanced_xwayland_manager) {
        AXIOM_LOG_WARN("Failed to initialize enhanced XWayland manager");
    }

    // Set up event listeners
    server->new_output.notify = axiom_new_output;
    wl_signal_add(&server->backend->events.new_output, &server->new_output);
    
    server->new_xdg_toplevel.notify = axiom_new_xdg_toplevel;
    wl_signal_add(&server->xdg_shell->events.new_toplevel, &server->new_xdg_toplevel);
    
    server->new_input.notify = axiom_new_input;
    wl_signal_add(&server->backend->events.new_input, &server->new_input);
    
    // Skip cursor setup entirely to avoid wlroots assertion issues
    // TODO: Implement proper cursor handling after output is configured
    AXIOM_LOG_INFO("Skipping cursor attachment to prevent assertion failures");
    
    // Seat event listeners
    server->request_cursor.notify = axiom_request_cursor;
    wl_signal_add(&server->seat->events.request_set_cursor, &server->request_cursor);
    
    server->request_set_selection.notify = axiom_request_set_selection;
    wl_signal_add(&server->seat->events.request_set_selection, &server->request_set_selection);
    
    // Backend destroy event
    server->backend_destroy.notify = axiom_backend_destroy;
    wl_signal_add(&server->backend->events.destroy, &server->backend_destroy);
    
    // Create a background surface to prevent black screen
    server->background = wlr_scene_rect_create(&server->scene->tree, 0, 0, 
                                              (float[]){0.1, 0.1, 0.1, 1.0});
    if (server->background) {
        AXIOM_LOG_INFO("Background surface created");
    }
    
    // Initialize cursor manager
    server->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
    if (server->cursor_mgr) {
        if (!wlr_xcursor_manager_load(server->cursor_mgr, 1)) {
            AXIOM_LOG_WARN("Failed to load cursor theme, will retry on first output");
        } else {
            AXIOM_LOG_INFO("Cursor theme loaded successfully");
        }
    }

    // Try to add socket
    const char *socket = wl_display_add_socket_auto(server->wl_display);
    if (!socket) {
        AXIOM_LOG_ERROR("Failed to add Wayland socket");
        return false;
    }

    // Start the backend
    if (!wlr_backend_start(server->backend)) {
        AXIOM_LOG_ERROR("Failed to start backend");
        return false;
    }
    
    // Set environment variable for clients
    setenv("WAYLAND_DISPLAY", socket, true);
    
    server->running = false; // Will be set to true when we start running

    AXIOM_LOG_INFO("Compositor initialized successfully on socket: %s", socket);
    return true;
}

/**
 * Start the compositor main loop
 */
void axiom_compositor_run(struct axiom_server *server) {
    if (!server) return;

    server->running = true;
    AXIOM_LOG_INFO("Axiom running on Wayland display");
    
    int consecutive_errors = 0;
    const int MAX_CONSECUTIVE_ERRORS = 10;

    while (server->running) {
        wl_display_flush_clients(server->wl_display);

        // Update animations if the animation manager is available
        if (server->animation_manager) {
            uint32_t current_time = get_current_time_ms();
            axiom_animation_manager_update(server->animation_manager, current_time);
        }

        int dispatch_result = wl_event_loop_dispatch(server->wl_event_loop, -1);
        if (dispatch_result < 0) {
            consecutive_errors++;
            AXIOM_LOG_ERROR("Event loop dispatch failed (attempt %d/%d)", 
                           consecutive_errors, MAX_CONSECUTIVE_ERRORS);
            
            // Only exit if we have too many consecutive errors
            if (consecutive_errors >= MAX_CONSECUTIVE_ERRORS) {
                AXIOM_LOG_ERROR("Too many consecutive errors, shutting down");
                break;
            }
            
            // Check if display is still valid
            if (!server->wl_display) {
                AXIOM_LOG_ERROR("Display destroyed, shutting down");
                break;
            }
            
            // Small delay to prevent tight error loop
            usleep(10000); // 10ms
        } else {
            // Reset error counter on successful dispatch
            consecutive_errors = 0;
        }
    }
}

/**
 * Clean shutdown of all compositor subsystems
 */
void axiom_compositor_cleanup(struct axiom_server *server) {
    if (!server) return;
    
    AXIOM_LOG_INFO("Starting compositor cleanup...");
    
    server->running = false;
    
    // Basic cleanup
    if (server->wl_display) {
        wl_display_destroy(server->wl_display);
        server->wl_display = NULL;
    }

    AXIOM_LOG_INFO("Compositor cleanup complete");
}

/**
 * Reload compositor configuration
 */
void axiom_compositor_reload_config(struct axiom_server *server) {
    if (!server) return;
    
    AXIOM_LOG_INFO("Reloading configuration...");
    
    // Get configuration path
    const char *config_path = getenv("AXIOM_CONFIG_PATH");
    if (!config_path) {
        config_path = "/home/.config/axiom/config.ini"; // Default path  
    }
    
    // Create new configuration and load it
    struct axiom_config *new_config = axiom_config_create();
    if (!new_config) {
        AXIOM_LOG_ERROR("Failed to create new configuration");
        return;
    }
    
    if (!axiom_config_load(new_config, config_path)) {
        AXIOM_LOG_ERROR("Failed to load configuration from %s", config_path);
        axiom_config_destroy(new_config);
        return;
    }
    
    if (!axiom_config_validate(new_config)) {
        AXIOM_LOG_ERROR("Configuration validation failed");
        axiom_config_destroy(new_config);
        return;
    }
    
    // Replace old configuration
    if (server->config) {
        axiom_config_destroy(server->config);
    }
    server->config = new_config;
    
    // Apply configuration changes to subsystems
    if (server->animation_manager) {
        server->animation_manager->enabled = new_config->animations_enabled;
        server->animation_manager->global_speed_multiplier = new_config->animation_speed_multiplier;
        server->animation_manager->debug_mode = new_config->animation_debug_mode;
    }
    
    if (server->effects_manager) {
        // Update effects configuration
        axiom_shadow_update_config(server->effects_manager, &server->effects_manager->shadow);
        axiom_blur_update_config(server->effects_manager, &server->effects_manager->blur);
        axiom_transparency_update_config(server->effects_manager, &server->effects_manager->transparency);
    }
    
    if (server->window_manager) {
        // Update window manager settings
        server->window_manager->border_width = new_config->border_width;
        server->window_manager->gap_size = new_config->gap_size;
        
        // Rearrange windows with new settings
        axiom_window_manager_arrange_all(server->window_manager);
    }
    
    AXIOM_LOG_INFO("Configuration reloaded successfully");
}

/**
 * Event handler implementations
 */
void axiom_new_output(struct wl_listener *listener, void *data) {
    struct axiom_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;
    
    AXIOM_LOG_INFO("New output: %s (w=%d, h=%d, refresh=%dmHz, scale=%.2f)", 
                   wlr_output->name, wlr_output->width, wlr_output->height, 
                   wlr_output->refresh, wlr_output->scale);
    
    // Use the new wlroots 0.19 output state API
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    
    // Always enable the output first
    wlr_output_state_set_enabled(&state, true);
    
    // Set preferred mode if available and output supports modes
    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
        if (mode) {
            AXIOM_LOG_INFO("Setting mode for output %s: %dx%d@%dmHz", wlr_output->name,
                          mode->width, mode->height, mode->refresh);
            wlr_output_state_set_mode(&state, mode);
        } else {
            AXIOM_LOG_WARN("No preferred mode found for output %s", wlr_output->name);
        }
    } else {
        // For nested outputs or outputs without modes list, try setting a custom mode
        AXIOM_LOG_INFO("Output %s has no modes list, trying custom mode", wlr_output->name);
        // Only set custom mode if the output supports it
        if (wlr_output->width > 0 && wlr_output->height > 0) {
            wlr_output_state_set_custom_mode(&state, wlr_output->width, wlr_output->height, wlr_output->refresh);
            AXIOM_LOG_INFO("Set custom mode for output %s: %dx%d@%dmHz", wlr_output->name,
                          wlr_output->width, wlr_output->height, wlr_output->refresh);
        } else {
            AXIOM_LOG_INFO("Output %s: enabled with default mode", wlr_output->name);
        }
    }
    
    // Test the configuration before committing
    if (!wlr_output_test_state(wlr_output, &state)) {
        AXIOM_LOG_WARN("Output configuration test failed for %s, trying fallback", wlr_output->name);
        
        // Fallback: try enabling without mode changes
        wlr_output_state_finish(&state);
        wlr_output_state_init(&state);
        wlr_output_state_set_enabled(&state, true);
        
        if (!wlr_output_test_state(wlr_output, &state)) {
            AXIOM_LOG_ERROR("Even fallback configuration failed for output %s", wlr_output->name);
            wlr_output_state_finish(&state);
            return;
        }
    }
    
    // Commit the tested configuration
    if (!wlr_output_commit_state(wlr_output, &state)) {
        AXIOM_LOG_ERROR("Failed to commit output %s - hardware or driver issue", wlr_output->name);
        wlr_output_state_finish(&state);
        return;
    }
    
    wlr_output_state_finish(&state);
    
    // Create and configure the output
    struct wlr_output_layout_output *lo = wlr_output_layout_add_auto(server->output_layout, wlr_output);
    struct wlr_scene_output *scene_output = wlr_scene_output_create(server->scene, wlr_output);
    wlr_scene_output_layout_add_output(server->scene_layout, lo, scene_output);
    
    // Update background size to cover the entire output layout
    struct wlr_box layout_box;
    wlr_output_layout_get_box(server->output_layout, NULL, &layout_box);
    if (server->background) {
        wlr_scene_rect_set_size(server->background, layout_box.width, layout_box.height);
        AXIOM_LOG_INFO("Background updated to %dx%d", layout_box.width, layout_box.height);
    }
    
    // Skip cursor setup for now to avoid wlroots assertion issue
    AXIOM_LOG_INFO("Output %s configured successfully, skipping cursor for stability", wlr_output->name);
}

void axiom_new_xdg_toplevel(struct wl_listener *listener, void *data) {
    struct axiom_server *server = wl_container_of(listener, server, new_xdg_toplevel);
    struct wlr_xdg_toplevel *xdg_toplevel = data;
    
    AXIOM_LOG_INFO("New XDG toplevel: %s", xdg_toplevel->title ?: "(untitled)");
    
    // Create a new window structure for the XDG toplevel
    struct axiom_window *window = calloc(1, sizeof(struct axiom_window));
    if (!window) {
        AXIOM_LOG_ERROR("Failed to allocate window structure");
        return;
    }
    
    // Initialize window with XDG toplevel
    window->type = AXIOM_WINDOW_XDG;
    window->xdg_toplevel = xdg_toplevel;
    window->server = server;
    
    // Add to window list
    wl_list_insert(&server->windows, &window->link);
    
    // Delegate to window manager if available
    if (server->window_manager) {
        axiom_window_manager_add_window(server->window_manager, window);
    }
}

void axiom_request_cursor(struct wl_listener *listener, void *data) {
    struct axiom_server *server = wl_container_of(listener, server, request_cursor);
    struct wlr_seat_pointer_request_set_cursor_event *event = data;
    struct wlr_seat_client *focused_client = server->seat->pointer_state.focused_client;
    
    // Only honor cursor requests from the focused client
    if (focused_client == event->seat_client) {
        wlr_cursor_set_surface(server->cursor, event->surface, 
                               event->hotspot_x, event->hotspot_y);
    }
}

void axiom_request_set_selection(struct wl_listener *listener, void *data) {
    struct axiom_server *server = wl_container_of(listener, server, request_set_selection);
    struct wlr_seat_request_set_selection_event *event = data;
    wlr_seat_set_selection(server->seat, event->source, event->serial);
}

void axiom_backend_destroy(struct wl_listener *listener, void *data) {
    (void)data; // Unused parameter
    struct axiom_server *server = wl_container_of(listener, server, backend_destroy);
    AXIOM_LOG_INFO("Backend destroyed, shutting down compositor");
    server->running = false;
}
