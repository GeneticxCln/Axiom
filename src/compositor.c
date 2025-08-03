#include "compositor.h"
#include "axiom.h"
#include "config.h"
#include "animation.h"
#include "effects.h"
#include "window_manager.h"
#include "logging.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/util/log.h>

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
    
    // Create backend
    if (nested) {
        AXIOM_LOG_INFO("Creating nested backend");
        server->backend = wlr_backend_autocreate(server->wl_event_loop, NULL);
    } else {
        AXIOM_LOG_INFO("Creating native backend");
        server->backend = wlr_backend_autocreate(server->wl_event_loop, NULL);
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

    // Create cursor
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

    // Set up event listeners
    server->new_output.notify = axiom_new_output;
    wl_signal_add(&server->backend->events.new_output, &server->new_output);
    
    server->new_xdg_toplevel.notify = axiom_new_xdg_toplevel;
    wl_signal_add(&server->xdg_shell->events.new_toplevel, &server->new_xdg_toplevel);
    
    server->new_input.notify = axiom_new_input;
    wl_signal_add(&server->backend->events.new_input, &server->new_input);
    
    // Cursor event listeners
    wlr_cursor_attach_output_layout(server->cursor, server->output_layout);
    
    server->cursor_motion.notify = axiom_cursor_motion;
    wl_signal_add(&server->cursor->events.motion, &server->cursor_motion);
    
    server->cursor_motion_absolute.notify = axiom_cursor_motion_absolute;
    wl_signal_add(&server->cursor->events.motion_absolute, &server->cursor_motion_absolute);
    
    server->cursor_button.notify = axiom_cursor_button;
    wl_signal_add(&server->cursor->events.button, &server->cursor_button);
    
    server->cursor_axis.notify = axiom_cursor_axis;
    wl_signal_add(&server->cursor->events.axis, &server->cursor_axis);
    
    server->cursor_frame.notify = axiom_cursor_frame;
    wl_signal_add(&server->cursor->events.frame, &server->cursor_frame);
    
    // Seat event listeners
    server->request_cursor.notify = axiom_request_cursor;
    wl_signal_add(&server->seat->events.request_set_cursor, &server->request_cursor);
    
    server->request_set_selection.notify = axiom_request_set_selection;
    wl_signal_add(&server->seat->events.request_set_selection, &server->request_set_selection);
    
    // Backend destroy event
    server->backend_destroy.notify = axiom_backend_destroy;
    wl_signal_add(&server->backend->events.destroy, &server->backend_destroy);
    
    // Initialize cursor manager
    server->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
    if (server->cursor_mgr) {
        wlr_xcursor_manager_load(server->cursor_mgr, 1);
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

    while (server->running) {
        wl_display_flush_clients(server->wl_display);

    // Update animations if the animation manager is available
    if (server->animation_manager) {
        uint32_t current_time = get_current_time_ms();
        axiom_animation_manager_update(server->animation_manager, current_time);
    }

        if (wl_event_loop_dispatch(server->wl_event_loop, -1) < 0) {
            break;
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
    
    AXIOM_LOG_INFO("New output: %s", wlr_output->name);
    
    // Use the new wlroots 0.19 output state API
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    
    // Set preferred mode if available
    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
        wlr_output_state_set_mode(&state, mode);
        wlr_output_state_set_enabled(&state, true);
    }
    
    if (!wlr_output_commit_state(wlr_output, &state)) {
        AXIOM_LOG_WARN("Failed to commit output %s", wlr_output->name);
        wlr_output_state_finish(&state);
        return;
    }
    
    wlr_output_state_finish(&state);
    
    // Create and configure the output
    struct wlr_output_layout_output *lo = wlr_output_layout_add_auto(server->output_layout, wlr_output);
    struct wlr_scene_output *scene_output = wlr_scene_output_create(server->scene, wlr_output);
    wlr_scene_output_layout_add_output(server->scene_layout, lo, scene_output);
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
    struct axiom_server *server = wl_container_of(listener, server, backend_destroy);
    AXIOM_LOG_INFO("Backend destroyed, shutting down compositor");
    server->running = false;
}
