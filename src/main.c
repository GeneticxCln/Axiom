#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include "config.h"
#include "axiom.h"
#include "animation.h"
#include "effects.h"
#include "effects_realtime.h"
#include "renderer.h"
#include "window_rules.h"
#include "smart_gaps.h"
#include "window_snapping.h"
void axiom_calculate_window_layout(struct axiom_server *server, int index, int *x, int *y, int *width, int *height);

void axiom_arrange_windows(struct axiom_server *server) {
    if (!server->tiling_enabled || server->window_count == 0) {
        return;
    }

    struct axiom_window *window;
    int index = 0;
    
    wl_list_for_each(window, &server->windows, link) {
        if (!window->is_tiled) {
            continue;
        }
        
        axiom_calculate_window_layout_advanced(server, index, &window->x, &window->y, &window->width, &window->height);
        
        // Position the window using scene tree
        wlr_scene_node_set_position(&window->scene_tree->node, window->x, window->y);
        
        // Update window decorations to match new position and size
        axiom_update_window_decorations(window);
                
        uint32_t configure_serial = wlr_xdg_toplevel_set_size(
            window->xdg_toplevel, window->width, window->height);
        
        printf("Tiled window %d: x=%d, y=%d, w=%d, h=%d (serial=%u)\n", 
               index, window->x, window->y, window->width, window->height, configure_serial);
        
        index++;
    }
}

void axiom_calculate_window_layout(struct axiom_server *server, int index, int *x, int *y, int *width, int *height) {
    if (server->workspace_width <= 0 || server->workspace_height <= 0) {
        *x = *y = 0;
        *width = 800;
        *height = 600;
        return;
    }
    
    int window_count = server->window_count;
    if (window_count == 1) {
        *x = 0;
        *y = 0;
        *width = server->workspace_width;
        *height = server->workspace_height;
    } else if (window_count == 2) {
        *width = server->workspace_width / 2;
        *height = server->workspace_height;
        *x = index * (*width);
        *y = 0;
    } else {
        // Grid layout for more than 2 windows
        int cols = (int)ceil(sqrt(window_count));
        int rows = (int)ceil((double)window_count / cols);
        
        *width = server->workspace_width / cols;
        *height = server->workspace_height / rows;
        
        int col = index % cols;
        int row = index / cols;
        
        *x = col * (*width);
        *y = row * (*height);
    }
}

static void window_map(struct wl_listener *listener, void *data) {
    (void)data; // Suppress unused parameter warning
    struct axiom_window *window = wl_container_of(listener, window, map);
    printf("Window mapped: %s\n", window->xdg_toplevel->title ?: "(no title)");
    
    // Trigger window appear animation
    if (window->server && window->server->animation_manager) {
        axiom_animate_window_appear(window->server, window);
    }
    
    // Arrange windows with tiled layout
    if (window->server && window->server->tiling_enabled) {
        axiom_arrange_windows(window->server);
    }
}

static void window_unmap(struct wl_listener *listener, void *data) {
    (void)data; // Suppress unused parameter warning
    struct axiom_window *window = wl_container_of(listener, window, unmap);
    printf("Window unmapped\n");
}

static void window_destroy(struct wl_listener *listener, void *data) {
    (void)data; // Suppress unused parameter warning
    struct axiom_window *window = wl_container_of(listener, window, destroy);
    struct axiom_server *server = window->server;
    
    printf("Window destroyed\n");
    
    // Cleanup window effects
    if (window->effects) {
        axiom_window_effects_destroy(window);
    }
    
    // Update the window count properly
    if (window->is_tiled && server->window_count > 0) {
        server->window_count--;
        printf("Tiled window destroyed, remaining: %d\n", server->window_count);
        
        // Rearrange remaining windows if tiling is enabled
        if (server->tiling_enabled) {
            axiom_arrange_windows(server);
        }
    }
    
    // Clear focus if this was the focused window
    if (server->focused_window == window) {
        server->focused_window = NULL;
    }
    
    // Clear grabbed window if this was the grabbed window
    if (server->grabbed_window == window) {
        server->grabbed_window = NULL;
        server->cursor_mode = AXIOM_CURSOR_PASSTHROUGH;
    }
    
    wl_list_remove(&window->link);
    free(window);
}

static void server_new_xdg_toplevel(struct wl_listener *listener, void *data) {
    struct axiom_server *server = wl_container_of(listener, server, new_xdg_toplevel);
    struct wlr_xdg_toplevel *xdg_toplevel = data;
    
    printf("New XDG toplevel: %s\n", xdg_toplevel->title ?: "(no title)");
    
    struct axiom_window *window = calloc(1, sizeof(struct axiom_window));
    if (!window) {
        return;
    }
    
    window->xdg_toplevel = xdg_toplevel;
    window->scene_tree = wlr_scene_xdg_surface_create(&server->scene->tree, xdg_toplevel->base);
    if (!window->scene_tree) {
        free(window);
        return;
    }
    
    window->scene_tree->node.data = window;


    window->server = server;
    
    // Initialize window dimensions (default values)
    window->width = 800;
    window->height = 600;
    window->x = 100;
    window->y = 100;
    
    // Create decoration tree
    window->decoration_tree = wlr_scene_tree_create(&server->scene->tree);
    if (window->decoration_tree) {
        // Create rounded border effect using multiple thin rectangles
        // We'll create a layered border with slightly different colors for depth
        float border_outer[4] = {0.3, 0.5, 0.9, 1.0}; // Darker blue outer
        float border_inner[4] = {0.4, 0.6, 1.0, 1.0}; // Brighter blue inner
        
        // Create border components for rounded effect
        // Top border
        window->border_top = wlr_scene_rect_create(window->decoration_tree, window->width + 4, 3, border_outer);
        // Bottom border  
        window->border_bottom = wlr_scene_rect_create(window->decoration_tree, window->width + 4, 3, border_outer);
        // Left border
        window->border_left = wlr_scene_rect_create(window->decoration_tree, 3, window->height + 28, border_outer);
        // Right border
        window->border_right = wlr_scene_rect_create(window->decoration_tree, 3, window->height + 28, border_outer);
        
        // Corner rounding simulation using smaller rectangles
        // Top-left corner pieces
        window->corner_tl1 = wlr_scene_rect_create(window->decoration_tree, 2, 2, border_inner);
        window->corner_tl2 = wlr_scene_rect_create(window->decoration_tree, 1, 1, border_inner);
        // Top-right corner pieces
        window->corner_tr1 = wlr_scene_rect_create(window->decoration_tree, 2, 2, border_inner);
        window->corner_tr2 = wlr_scene_rect_create(window->decoration_tree, 1, 1, border_inner);
        // Bottom-left corner pieces
        window->corner_bl1 = wlr_scene_rect_create(window->decoration_tree, 2, 2, border_inner);
        window->corner_bl2 = wlr_scene_rect_create(window->decoration_tree, 1, 1, border_inner);
        // Bottom-right corner pieces
        window->corner_br1 = wlr_scene_rect_create(window->decoration_tree, 2, 2, border_inner);
        window->corner_br2 = wlr_scene_rect_create(window->decoration_tree, 1, 1, border_inner);
        
        // Create enhanced title bar with gradient effect
        float title_bg[4] = {0.15, 0.15, 0.15, 0.95};     // Dark background
        float title_accent[4] = {0.25, 0.35, 0.55, 0.8};  // Subtle accent strip
        
        window->title_bar = wlr_scene_rect_create(window->decoration_tree, window->width, 24, title_bg);
        window->title_accent = wlr_scene_rect_create(window->decoration_tree, window->width, 2, title_accent);
        
        // Position all decoration elements
        if (window->title_bar) {
            window->title_bar->node.data = window;
            wlr_scene_node_set_position(&window->title_bar->node, window->x, window->y - 24);
        }
        
        if (window->title_accent) {
            window->title_accent->node.data = window;
            wlr_scene_node_set_position(&window->title_accent->node, window->x, window->y - 2);
        }
        
        // Position border elements
        if (window->border_top) {
            window->border_top->node.data = window;
            wlr_scene_node_set_position(&window->border_top->node, window->x - 2, window->y - 27);
        }
        
        if (window->border_bottom) {
            window->border_bottom->node.data = window;
            wlr_scene_node_set_position(&window->border_bottom->node, window->x - 2, window->y + window->height);
        }
        
        if (window->border_left) {
            window->border_left->node.data = window;
            wlr_scene_node_set_position(&window->border_left->node, window->x - 3, window->y - 26);
        }
        
        if (window->border_right) {
            window->border_right->node.data = window;
            wlr_scene_node_set_position(&window->border_right->node, window->x + window->width, window->y - 26);
        }
        
        // Position corner elements for rounded effect
        // Top-left corners
        if (window->corner_tl1) {
            window->corner_tl1->node.data = window;
            wlr_scene_node_set_position(&window->corner_tl1->node, window->x - 1, window->y - 25);
        }
        if (window->corner_tl2) {
            window->corner_tl2->node.data = window;
            wlr_scene_node_set_position(&window->corner_tl2->node, window->x, window->y - 24);
        }
        
        // Top-right corners
        if (window->corner_tr1) {
            window->corner_tr1->node.data = window;
            wlr_scene_node_set_position(&window->corner_tr1->node, window->x + window->width - 1, window->y - 25);
        }
        if (window->corner_tr2) {
            window->corner_tr2->node.data = window;
            wlr_scene_node_set_position(&window->corner_tr2->node, window->x + window->width - 1, window->y - 24);
        }
        
        // Bottom-left corners
        if (window->corner_bl1) {
            window->corner_bl1->node.data = window;
            wlr_scene_node_set_position(&window->corner_bl1->node, window->x - 1, window->y + window->height - 1);
        }
        if (window->corner_bl2) {
            window->corner_bl2->node.data = window;
            wlr_scene_node_set_position(&window->corner_bl2->node, window->x, window->y + window->height - 2);
        }
        
        // Bottom-right corners
        if (window->corner_br1) {
            window->corner_br1->node.data = window;
            wlr_scene_node_set_position(&window->corner_br1->node, window->x + window->width - 1, window->y + window->height - 1);
        }
        if (window->corner_br2) {
            window->corner_br2->node.data = window;
            wlr_scene_node_set_position(&window->corner_br2->node, window->x + window->width - 1, window->y + window->height - 2);
        }
    }
    // Initialize tiling properties
    window->is_tiled = server->tiling_enabled;
    if (window->is_tiled) {
        server->window_count++;
    }
    
    // Store surface reference for effects
    window->surface = xdg_toplevel->base->surface;
    
    // Initialize window effects if real-time effects are enabled
    if (server->effects_manager && server->effects_manager->realtime_enabled) {
        if (!axiom_window_effects_init(window)) {
            fprintf(stderr, "Failed to initialize effects for window\n");
        } else {
            printf("Window effects initialized successfully\n");
        }
    }
    
    // Apply window rules if rules system is enabled
    if (server->window_rules_manager) {
        // Debug window properties
        axiom_window_rules_debug_window_properties(window);
        
        // Apply matching rules
        if (!axiom_window_rules_apply_to_window(server->window_rules_manager, window)) {
            printf("No window rules applied to this window\n");
        }
    }
    
    window->map.notify = window_map;
    wl_signal_add(&xdg_toplevel->base->surface->events.map, &window->map);
    
    window->unmap.notify = window_unmap;
    wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &window->unmap);
    
    window->destroy.notify = window_destroy;
    wl_signal_add(&xdg_toplevel->base->events.destroy, &window->destroy);
    
    wl_list_insert(&server->windows, &window->link);
    
    printf("Window added, total tiled windows: %d\n", server->window_count);
}

static void server_new_output(struct wl_listener *listener, void *data) {
    struct axiom_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;
    
    printf("New output: %s\n", wlr_output->name);
    printf("Debug: renderer=%p, cursor_mgr=%p, cursor=%p\n", 
           (void*)server->renderer, (void*)server->cursor_mgr, (void*)server->cursor);
    
    // Set preferred mode
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);
    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
        wlr_output_state_set_mode(&state, mode);
    }
    
    if (!wlr_output_commit_state(wlr_output, &state)) {
        fprintf(stderr, "Failed to commit output state for %s\n", wlr_output->name);
    }
    wlr_output_state_finish(&state);
    wlr_output_layout_add_auto(server->output_layout, wlr_output);
    struct axiom_output *output = calloc(1, sizeof(struct axiom_output));
    if (!output) {
        wlr_output_state_finish(&state);
        return;
    }
    
    output->server = server;
    output->wlr_output = wlr_output;
    
    printf("Debug: Creating scene output with scene=%p, wlr_output=%p\n", 
           (void*)server->scene, (void*)wlr_output);
    
    output->scene_output = wlr_scene_output_create(server->scene, wlr_output);
    if (!output->scene_output) {
        printf("Error: Failed to create scene output\n");
        free(output);
        wlr_output_state_finish(&state);
        return;
    }
    
    printf("Debug: Scene output created: %p\n", (void*)output->scene_output);
    
    // Lock software cursors to avoid hardware cursor renderer issues in nested mode
    wlr_output_lock_software_cursors(wlr_output, true);
    printf("Debug: Locked software cursors for output %s\n", wlr_output->name);
    
    wl_list_insert(&server->outputs, &output->link);
    
    // Skip cursor setup entirely during output creation to avoid renderer assertion
    // The cursor will be handled by the software cursor system
    printf("Debug: Skipping cursor setup to avoid renderer assertion in nested mode\n");
    
    // Load cursor theme for this output scale but don't set it immediately 
    if (server->cursor_mgr) {
        printf("Debug: Loading cursor theme for output %s (scale=%.2f)\n", 
               wlr_output->name, wlr_output->scale);
        
        if (wlr_xcursor_manager_load(server->cursor_mgr, wlr_output->scale)) {
            printf("Debug: Cursor theme loaded successfully (not setting cursor yet)\n");
        } else {
            printf("Warning: Failed to load cursor theme for output %s\n", wlr_output->name);
        }
    }
    
    // Update workspace dimensions based on output size
    if (wlr_output->current_mode) {
        server->workspace_width = wlr_output->current_mode->width;
        server->workspace_height = wlr_output->current_mode->height;
        printf("Workspace dimensions set to: %dx%d\n", server->workspace_width, server->workspace_height);
        
        // Rearrange existing windows if tiling is enabled
        if (server->tiling_enabled) {
            axiom_arrange_windows(server);
        }
    }
}

int main(int argc, char *argv[]) {
    printf("Axiom Wayland Compositor v" AXIOM_VERSION "\n");
    
    // Parse command line arguments
    bool nested = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--nested") == 0) {
            nested = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [OPTIONS]\n", argv[0]);
            printf("Options:\n");
            printf("  --nested    Run in nested mode (within another compositor)\n");
            printf("  --help, -h  Show this help message\n");
            return EXIT_SUCCESS;
        }
    }
    
    struct axiom_server server = {0};
    
    server.wl_display = wl_display_create();
    if (!server.wl_display) {
        fprintf(stderr, "Failed to create Wayland display\n");
        return EXIT_FAILURE;
    }
    
    server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
    // Create backend with session handling for better compatibility
    if (nested) {
        printf("Running in nested mode\n");
    }
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
    
    printf("Debug: Renderer created successfully: %p\n", (void*)server.renderer);
    
    if (!wlr_renderer_init_wl_display(server.renderer, server.wl_display)) {
        fprintf(stderr, "Failed to initialize renderer with Wayland display\n");
        return EXIT_FAILURE;
    }
    
    printf("Debug: Renderer initialized with Wayland display\n");
    
    server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
    server.compositor = wlr_compositor_create(server.wl_display, 5, server.renderer);
    server.scene = wlr_scene_create();
    server.output_layout = wlr_output_layout_create(server.wl_display);
    server.scene_layout = wlr_scene_attach_output_layout(server.scene, server.output_layout);
    
    // Ensure the scene is properly initialized with the renderer
    printf("Debug: Scene created: %p, scene_layout: %p\n", (void*)server.scene, (void*)server.scene_layout);
    
    // Initialize lists
    wl_list_init(&server.windows);
    wl_list_init(&server.outputs);
    
    // Initialize window management
    server.tiling_enabled = true;  // Enable tiling by default
    server.window_count = 0;
    server.workspace_width = 1920;  // Default fallback
    server.workspace_height = 1080;
    
    // Initialize workspace management
    axiom_init_workspaces(&server);
    server.xdg_shell = wlr_xdg_shell_create(server.wl_display, 3);
    if (!server.xdg_shell) {
        fprintf(stderr, "Failed to create XDG shell\n");
        return EXIT_FAILURE;
    }
    
    server.new_xdg_toplevel.notify = server_new_xdg_toplevel;
    wl_signal_add(&server.xdg_shell->events.new_toplevel, &server.new_xdg_toplevel);
    
    // Create seat and data device manager
    server.seat = wlr_seat_create(server.wl_display, "seat0");
    server.data_device_manager = wlr_data_device_manager_create(server.wl_display);
    
    server.new_output.notify = server_new_output;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);
    
    // Initialize configuration
    server.config = axiom_config_create();
    if (!server.config) {
        fprintf(stderr, "Failed to create configuration\n");
        return EXIT_FAILURE;
    }
    
    // Try to load config file from standard locations
    const char *config_paths[] = {
        "./axiom.conf",
        "./examples/axiom.conf", 
        "/etc/axiom/axiom.conf",
        NULL
    };
    
    for (int i = 0; config_paths[i]; i++) {
        if (axiom_config_load(server.config, config_paths[i])) {
            break;
        }
    }
    
    // Apply configuration to server state
    server.tiling_enabled = server.config->tiling_enabled;
    
    // Initialize window snapping system
    server.window_snapping_manager = axiom_window_snapping_manager_create(&server);
    if (!server.window_snapping_manager) {
        fprintf(stderr, "Failed to initialize window snapping manager\n");
    }

    // Initialize process management
    axiom_process_init(&server);

    // Initialize animation system
    axiom_animation_manager_init(&server);
    
    // Initialize visual effects system
    server.effects_manager = calloc(1, sizeof(struct axiom_effects_manager));
    if (server.effects_manager && !axiom_effects_manager_init(server.effects_manager)) {
        fprintf(stderr, "Failed to initialize effects manager\n");
        free(server.effects_manager);
        server.effects_manager = NULL;
    } else {
        printf("Effects manager initialized successfully\n");
        
        // Initialize GPU acceleration for effects
        if (server.effects_manager && axiom_effects_gpu_init(server.effects_manager, &server)) {
            printf("GPU acceleration enabled for visual effects\n");
            
            // Initialize real-time effects system
            if (!axiom_realtime_effects_init(server.effects_manager)) {
                fprintf(stderr, "Failed to initialize real-time effects\n");
            } else {
                printf("Real-time effects system initialized\n");
            }
        } else {
            printf("GPU acceleration not available, using software fallback\n");
        }
    }
    
    // Initialize window rules system (Phase 3.1)
    if (!axiom_server_init_window_rules(&server)) {
        fprintf(stderr, "Failed to initialize window rules system\n");
    } else {
        printf("Window rules system initialized successfully\n");
        
        // Print loaded rules for debugging
        if (server.window_rules_manager) {
            axiom_window_rules_print_rules(server.window_rules_manager);
        }
    }
    
    // Initialize smart gaps system (Phase 3.2)
    if (!axiom_server_init_smart_gaps(&server)) {
        fprintf(stderr, "Failed to initialize smart gaps system\n");
    } else {
        printf("Smart gaps system initialized successfully\n");
        
        // Load default gap profiles
        if (server.smart_gaps_manager) {
            if (!axiom_smart_gaps_load_defaults(server.smart_gaps_manager)) {
                fprintf(stderr, "Failed to load default gap profiles\n");
            }
        }
    }
    
    // Set up input management
    wl_list_init(&server.input_devices);
    
    server.cursor = wlr_cursor_create();
    if (!server.cursor) {
        fprintf(stderr, "Failed to create cursor\n");
        return EXIT_FAILURE;
    }
    
    printf("Debug: Cursor created: %p\n", (void*)server.cursor);
    
    server.cursor_mgr = wlr_xcursor_manager_create(server.config->cursor_theme, server.config->cursor_size);
    if (!server.cursor_mgr) {
        fprintf(stderr, "Failed to create cursor manager\n");
        return EXIT_FAILURE;
    }
    
    printf("Debug: Cursor manager created: %p (theme=%s, size=%d)\n", 
           (void*)server.cursor_mgr, server.config->cursor_theme, server.config->cursor_size);
    
    wlr_cursor_attach_output_layout(server.cursor, server.output_layout);
    printf("Debug: Cursor attached to output layout\n");
    
    server.cursor_mode = AXIOM_CURSOR_PASSTHROUGH;
    
    server.cursor_motion.notify = axiom_cursor_motion;
    wl_signal_add(&server.cursor->events.motion, &server.cursor_motion);
    server.cursor_motion_absolute.notify = axiom_cursor_motion_absolute;
    wl_signal_add(&server.cursor->events.motion_absolute, &server.cursor_motion_absolute);
    server.cursor_button.notify = axiom_cursor_button;
    wl_signal_add(&server.cursor->events.button, &server.cursor_button);
    server.cursor_axis.notify = axiom_cursor_axis;
    wl_signal_add(&server.cursor->events.axis, &server.cursor_axis);
    server.cursor_frame.notify = axiom_cursor_frame;
    wl_signal_add(&server.cursor->events.frame, &server.cursor_frame);
    
    server.new_input.notify = axiom_new_input;
    wl_signal_add(&server.backend->events.new_input, &server.new_input);
    
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
        
        // Update animations
        if (server.animation_manager) {
            uint32_t current_time = 0;
            struct timespec ts;
            if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
                current_time = (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
            }
            axiom_animation_manager_update(server.animation_manager, current_time);
            
            // Update real-time window effects
            if (server.effects_manager && server.effects_manager->realtime_enabled) {
                struct axiom_window *window;
                wl_list_for_each(window, &server.windows, link) {
                    if (window->effects) {
                        axiom_window_effects_update(window, current_time);
                    }
                }
                
                // Throttle effects updates for performance
                axiom_effects_throttle_updates(server.effects_manager);
            }
        }
        
        if (wl_event_loop_dispatch(server.wl_event_loop, -1) < 0) {
            break;
        }
    }
    
    // Cleanup
    axiom_process_cleanup();
    
    // Cleanup effects manager
    if (server.effects_manager) {
        axiom_effects_manager_destroy(server.effects_manager);
        free(server.effects_manager);
    }
    
// Cleanup window snapping system
    if (server.window_snapping_manager) {
        axiom_window_snapping_manager_destroy(server.window_snapping_manager);
    }

    // Cleanup window rules system
    if (server.window_rules_manager) {
        axiom_server_destroy_window_rules(&server);
    }
    
    // Cleanup smart gaps system
    if (server.smart_gaps_manager) {
        axiom_server_destroy_smart_gaps(&server);
    }
    
    axiom_config_destroy(server.config);
    wl_display_destroy(server.wl_display);
    return EXIT_SUCCESS;
}
