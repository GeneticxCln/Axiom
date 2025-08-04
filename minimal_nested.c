#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server.h>
#include <wlr/backend.h>
#include <wlr/backend/wayland.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_output_layout.h>

struct minimal_output {
    struct wlr_output *wlr_output;
    struct minimal_server *server;
    struct wl_listener frame;
};

struct minimal_server {
    struct wl_display *wl_display;
    struct wl_event_loop *wl_event_loop;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_compositor *compositor;
    struct wlr_scene *scene;
    struct wlr_output_layout *output_layout;
    struct wlr_scene_output_layout *scene_layout;
    
    struct wl_listener new_output;
    bool running;
};

static void output_frame(struct wl_listener *listener, void *data) {
    struct minimal_output *output = wl_container_of(listener, output, frame);
    struct wlr_output *wlr_output = output->wlr_output;
    
    printf("Frame callback for output %s\n", wlr_output->name);
    
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    
    struct wlr_render_pass *pass = wlr_output_begin_render_pass(wlr_output, &state, NULL);
    if (pass) {
        printf("Got render pass, drawing blue rectangle\n");
        // Clear with blue background
        wlr_render_pass_add_rect(pass, &(struct wlr_render_rect_options){
            .box = { .x = 0, .y = 0, .width = wlr_output->width, .height = wlr_output->height },
            .color = { .r = 0.2, .g = 0.4, .b = 0.8, .a = 1.0 },
        });
        
        wlr_render_pass_submit(pass);
        printf("Submitted render pass\n");
    } else {
        printf("Failed to get render pass\n");
    }
    
    if (!wlr_output_commit_state(wlr_output, &state)) {
        printf("Failed to commit output state\n");
    } else {
        printf("Successfully committed output state\n");
    }
    wlr_output_state_finish(&state);
}

static void new_output_notify(struct wl_listener *listener, void *data) {
    struct minimal_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;
    
    printf("New output created: %s (%dx%d)\n", 
           wlr_output->name, wlr_output->width, wlr_output->height);
    
    // Initialize the output
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);
    
    // Try to set a mode
    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
        if (mode) {
            wlr_output_state_set_mode(&state, mode);
            printf("Set mode: %dx%d@%d\n", mode->width, mode->height, mode->refresh);
        }
    }
    
    // Commit the output configuration
    if (!wlr_output_commit_state(wlr_output, &state)) {
        printf("Failed to commit output\n");
        wlr_output_state_finish(&state);
        return;
    }
    wlr_output_state_finish(&state);
    
    // Add to layout
    struct wlr_output_layout_output *lo = 
        wlr_output_layout_add_auto(server->output_layout, wlr_output);
    struct wlr_scene_output *scene_output = 
        wlr_scene_output_create(server->scene, wlr_output);
    wlr_scene_output_layout_add_output(server->scene_layout, lo, scene_output);
    
    // Create a colored rectangle to make the window visible
    wlr_scene_rect_create(&server->scene->tree, 
        wlr_output->width, wlr_output->height, 
        (float[]){0.2, 0.4, 0.8, 1.0}); // Blue background
    
    printf("Output configured successfully - window should be visible!\n");
    
    // Create output wrapper and add frame listener
    struct minimal_output *output = calloc(1, sizeof(struct minimal_output));
    output->wlr_output = wlr_output;
    output->server = server;
    output->frame.notify = output_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);
}

int main() {
    printf("Starting minimal nested compositor...\n");
    
    struct minimal_server server = {0};
    
    // Create display and event loop
    server.wl_display = wl_display_create();
    if (!server.wl_display) {
        printf("Failed to create display\n");
        return 1;
    }
    server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
    
    // Create wayland backend
    server.backend = wlr_wl_backend_create(server.wl_event_loop, NULL);
    if (!server.backend) {
        printf("Failed to create wayland backend\n");
        wl_display_destroy(server.wl_display);
        return 1;
    }
    printf("Created wayland backend\n");
    
    // Create renderer
    server.renderer = wlr_renderer_autocreate(server.backend);
    if (!server.renderer) {
        printf("Failed to create renderer\n");
        wl_display_destroy(server.wl_display);
        return 1;
    }
    printf("Created renderer\n");
    
    // Initialize renderer with display
    if (!wlr_renderer_init_wl_display(server.renderer, server.wl_display)) {
        printf("Failed to initialize renderer\n");
        wl_display_destroy(server.wl_display);
        return 1;
    }
    
    // Create compositor
    server.compositor = wlr_compositor_create(server.wl_display, 5, server.renderer);
    if (!server.compositor) {
        printf("Failed to create compositor\n");
        wl_display_destroy(server.wl_display);
        return 1;
    }
    printf("Created compositor\n");
    
    // Create scene graph
    server.scene = wlr_scene_create();
    server.output_layout = wlr_output_layout_create(server.wl_display);
    server.scene_layout = wlr_scene_attach_output_layout(server.scene, server.output_layout);
    
    // Set up output listener BEFORE starting backend
    server.new_output.notify = new_output_notify;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);
    
    // Add socket
    const char *socket = wl_display_add_socket_auto(server.wl_display);
    if (!socket) {
        printf("Failed to add socket\n");
        wl_display_destroy(server.wl_display);
        return 1;
    }
    printf("Created socket: %s\n", socket);
    
    // Start the backend FIRST
    if (!wlr_backend_start(server.backend)) {
        printf("Failed to start backend\n");
        wl_display_destroy(server.wl_display);
        return 1;
    }
    printf("Backend started\n");
    
    // Create the output AFTER backend is started (this should create the window)
    struct wlr_output *output = wlr_wl_output_create(server.backend);
    if (!output) {
        printf("Failed to create output\n");
        wl_display_destroy(server.wl_display);
        return 1;
    }
    printf("Created output - window should appear now!\n");
    
    // Set environment
    setenv("WAYLAND_DISPLAY", socket, 1);
    
    // Run event loop
    server.running = true;
    printf("Running event loop... (Press Ctrl+C to exit)\n");
    
    while (server.running) {
        wl_display_flush_clients(server.wl_display);
        if (wl_event_loop_dispatch(server.wl_event_loop, -1) < 0) {
            break;
        }
    }
    
    printf("Shutting down...\n");
    wl_display_destroy(server.wl_display);
    return 0;
}
