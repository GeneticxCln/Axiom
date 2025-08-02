#include <stdio.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_output_layout.h>

struct minimal_server {
    struct wl_display *wl_display;
    struct wl_event_loop *wl_event_loop;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    struct wlr_compositor *compositor;
    struct wlr_scene *scene;
    struct wlr_scene_output_layout *scene_layout;
    struct wlr_output_layout *output_layout;
    struct wlr_xdg_shell *xdg_shell;
    struct wlr_cursor *cursor;
    struct wlr_xcursor_manager *cursor_mgr;
    struct wlr_seat *seat;
    
    struct wl_listener new_output;
    struct wl_listener new_xdg_toplevel;
    struct wl_listener cursor_motion;
    struct wl_listener cursor_button;
    
    struct wl_list windows;
    bool running;
};

struct minimal_window {
    struct wl_list link;
    struct wlr_xdg_toplevel *xdg_toplevel;
    struct wlr_scene_tree *scene_tree;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
};

static void new_output(struct wl_listener *listener, void *data) {
    struct minimal_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;
    
    // Set preferred mode
    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
        wlr_output_set_mode(wlr_output, mode);
    }
    
    wlr_output_commit(wlr_output);
    wlr_output_layout_add_auto(server->output_layout, wlr_output);
    
    struct wlr_scene_output *scene_output = wlr_scene_output_create(server->scene, wlr_output);
    wlr_scene_output_layout_add_output(server->scene_layout, wlr_output, scene_output);
    
    printf("New output: %s\n", wlr_output->name);
}

static void window_map(struct wl_listener *listener, void *data) {
    (void)data;
    struct minimal_window *window = wl_container_of(listener, window, map);
    printf("Window mapped: %s\n", window->xdg_toplevel->title ?: "(no title)");
}

static void window_unmap(struct wl_listener *listener, void *data) {
    (void)data;
    printf("Window unmapped\n");
}

static void window_destroy(struct wl_listener *listener, void *data) {
    (void)data;
    struct minimal_window *window = wl_container_of(listener, window, destroy);
    wl_list_remove(&window->link);
    free(window);
    printf("Window destroyed\n");
}

static void new_xdg_toplevel(struct wl_listener *listener, void *data) {
    struct minimal_server *server = wl_container_of(listener, server, new_xdg_toplevel);
    struct wlr_xdg_toplevel *xdg_toplevel = data;
    
    struct minimal_window *window = calloc(1, sizeof(struct minimal_window));
    if (!window) return;
    
    window->xdg_toplevel = xdg_toplevel;
    window->scene_tree = wlr_scene_xdg_surface_create(&server->scene->tree, xdg_toplevel->base);
    window->scene_tree->node.data = window;
    
    window->map.notify = window_map;
    wl_signal_add(&xdg_toplevel->base->surface->events.map, &window->map);
    
    window->unmap.notify = window_unmap;
    wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &window->unmap);
    
    window->destroy.notify = window_destroy;
    wl_signal_add(&xdg_toplevel->base->events.destroy, &window->destroy);
    
    wl_list_insert(&server->windows, &window->link);
    printf("New window: %s\n", xdg_toplevel->title ?: "(no title)");
}

static void cursor_motion(struct wl_listener *listener, void *data) {
    struct minimal_server *server = wl_container_of(listener, server, cursor_motion);
    struct wlr_pointer_motion_event *event = data;
    wlr_cursor_move(server->cursor, &event->pointer->base, event->delta_x, event->delta_y);
    wlr_seat_pointer_notify_motion(server->seat, event->time_msec, 
                                   server->cursor->x, server->cursor->y);
}

static void cursor_button(struct wl_listener *listener, void *data) {
    struct minimal_server *server = wl_container_of(listener, server, cursor_button);
    struct wlr_pointer_button_event *event = data;
    wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button, event->state);
}

int main(int argc, char *argv[]) {
    printf("Minimal Axiom Compositor\n");
    
    bool nested = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--nested") == 0) {
            nested = true;
        }
    }
    
    struct minimal_server server = {0};
    
    // Core Wayland setup
    server.wl_display = wl_display_create();
    if (!server.wl_display) {
        fprintf(stderr, "Failed to create display\n");
        return 1;
    }
    
    server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
    server.backend = wlr_backend_autocreate(server.wl_event_loop, NULL);
    if (!server.backend) {
        fprintf(stderr, "Failed to create backend\n");
        return 1;
    }
    
    server.renderer = wlr_renderer_autocreate(server.backend);
    if (!server.renderer) {
        fprintf(stderr, "Failed to create renderer\n");
        return 1;
    }
    
    wlr_renderer_init_wl_display(server.renderer, server.wl_display);
    
    server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
    server.compositor = wlr_compositor_create(server.wl_display, 5, server.renderer);
    server.scene = wlr_scene_create();
    server.output_layout = wlr_output_layout_create(server.wl_display);
    server.scene_layout = wlr_scene_attach_output_layout(server.scene, server.output_layout);
    
    // Initialize window list
    wl_list_init(&server.windows);
    
    // XDG Shell setup
    server.xdg_shell = wlr_xdg_shell_create(server.wl_display, 3);
    server.new_xdg_toplevel.notify = new_xdg_toplevel;
    wl_signal_add(&server.xdg_shell->events.new_toplevel, &server.new_xdg_toplevel);
    
    // Output setup
    server.new_output.notify = new_output;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);
    
    // Cursor setup
    server.cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(server.cursor, server.output_layout);
    
    server.cursor_mgr = wlr_xcursor_manager_create("default", 24);
    
    server.cursor_motion.notify = cursor_motion;
    wl_signal_add(&server.cursor->events.motion, &server.cursor_motion);
    
    server.cursor_button.notify = cursor_button;
    wl_signal_add(&server.cursor->events.button, &server.cursor_button);
    
    // Seat setup
    server.seat = wlr_seat_create(server.wl_display, "seat0");
    
    // Start the backend
    if (!wlr_backend_start(server.backend)) {
        fprintf(stderr, "Failed to start backend\n");
        return 1;
    }
    
    const char *socket = wl_display_add_socket_auto(server.wl_display);
    if (!socket) {
        fprintf(stderr, "Failed to add socket\n");
        return 1;
    }
    
    setenv("WAYLAND_DISPLAY", socket, true);
    server.running = true;
    
    printf("Compositor running on WAYLAND_DISPLAY=%s\n", socket);
    
    while (server.running) {
        wl_display_flush_clients(server.wl_display);
        if (wl_event_loop_dispatch(server.wl_event_loop, -1) < 0) {
            break;
        }
    }
    
    wl_display_destroy(server.wl_display);
    return 0;
}
