#include "compositor.h"
#include "logging.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

    // Try to add socket
    const char *socket = wl_display_add_socket_auto(server->wl_display);
    if (!socket) {
        AXIOM_LOG_ERROR("Failed to add Wayland socket");
        return false;
    }

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

        // Update animations - TODO: implement when animation manager is ready
        // if (server->animation_manager) {
        //     axiom_animation_manager_update(server->animation_manager, current_time);
        // }

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
    
    AXIOM_LOG_INFO("Configuration reload requested - not yet implemented");
    // TODO: Implement when config system is ready
}
