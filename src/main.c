#include <stdio.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>

struct axiom_server {
    struct wl_display *wl_display;
    struct wl_event_loop *wl_event_loop;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    struct wlr_compositor *compositor;
    struct wlr_scene *scene;
    struct wlr_scene_output_layout *scene_layout;
    struct wlr_output_layout *output_layout;
    
    struct wl_listener new_output;
    bool running;
};

static void server_new_output(struct wl_listener *listener, void *data) {
    struct axiom_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;
    
    printf("New output: %s\n", wlr_output->name);
    
    // Set preferred mode
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);
    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
        wlr_output_state_set_mode(&state, mode);
    }
    
    wlr_output_commit_state(wlr_output, &state);
    wlr_output_layout_add_auto(server->output_layout, wlr_output);
    wlr_scene_output_create(server->scene, wlr_output);
}

int main(int argc, char *argv[]) {
    printf("Axiom Wayland Compositor v" AXIOM_VERSION "\n");
    
    struct axiom_server server = {0};
    
    server.wl_display = wl_display_create();
    if (!server.wl_display) {
        fprintf(stderr, "Failed to create Wayland display\n");
        return EXIT_FAILURE;
    }
    
    server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
    server.backend = wlr_backend_autocreate(server.wl_event_loop, NULL);
    if (!server.backend) {
        fprintf(stderr, "Failed to create backend\n");
        return EXIT_FAILURE;
    }
    
    server.renderer = wlr_renderer_autocreate(server.backend);
    if (!server.renderer) {
        fprintf(stderr, "Failed to create renderer\n");
        return EXIT_FAILURE;
    }
    
    wlr_renderer_init_wl_display(server.renderer, server.wl_display);
    
    server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
    server.compositor = wlr_compositor_create(server.wl_display, 5, server.renderer);
    server.scene = wlr_scene_create();
    server.output_layout = wlr_output_layout_create(server.wl_display);
    server.scene_layout = wlr_scene_attach_output_layout(server.scene, server.output_layout);
    
    server.new_output.notify = server_new_output;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);
    
    const char *socket = wl_display_add_socket_auto(server.wl_display);
    if (!socket) {
        fprintf(stderr, "Failed to add socket\n");
        return EXIT_FAILURE;
    }
    
    if (!wlr_backend_start(server.backend)) {
        fprintf(stderr, "Failed to start backend\n");
        return EXIT_FAILURE;
    }
    
    setenv("WAYLAND_DISPLAY", socket, true);
    server.running = true;
    
    printf("Axiom running on WAYLAND_DISPLAY=%s\n", socket);
    
    while (server.running) {
        wl_display_flush_clients(server.wl_display);
        if (wl_event_loop_dispatch(server.wl_event_loop, -1) < 0) {
            break;
        }
    }
    
    wl_display_destroy(server.wl_display);
    return EXIT_SUCCESS;
}
