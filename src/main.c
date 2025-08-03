#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <wayland-client.h>
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
#include "pip_manager.h"
#include "thumbnail_manager.h"
#include "keybindings.h"
#include "focus.h"
#include "layer_shell.h"
#include "screenshot.h"
#include "session.h"
#include "xwayland.h"
#include "logging.h"
#include "memory.h"
#include "constants.h"
#include "environment.h"
void axiom_calculate_window_layout(struct axiom_server *server, int index, int *x, int *y, int *width, int *height);

// Configuration reload function
void axiom_reload_configuration(struct axiom_server *server) {
    if (!server) return;
    
    axiom_log_info("Reloading configuration...");
    
    // Reload window rules
    if (server->window_rules_manager) {
        axiom_window_rules_reload_config(server->window_rules_manager);
        axiom_log_info("Window rules reloaded");
    }
    
    // Reload main configuration
    if (server->config) {
        struct axiom_config *new_config = axiom_config_create();
        if (new_config) {
            // Try to load from various paths
            const char *config_paths[] = {
                "./axiom.conf",
                "./examples/axiom.conf",
                "~/.config/axiom/axiom.conf",
                "/etc/axiom/axiom.conf",
                NULL
            };
            
            bool loaded = false;
            for (int i = 0; config_paths[i] && !loaded; i++) {
                loaded = axiom_config_load(new_config, config_paths[i]);
            }
            
            if (loaded) {
                // Replace old config with new one
                axiom_config_destroy(server->config);
                server->config = new_config;
                axiom_log_info("Main configuration reloaded");
                
                // Update effects manager with new config
                if (server->effects_manager && server->config->effects.shadows_enabled) {
                    axiom_effects_manager_destroy(server->effects_manager);
        server->effects_manager = axiom_calloc_tracked(1, sizeof(struct axiom_effects_manager), 
                                                      AXIOM_MEM_TYPE_EFFECTS, __FILE__, __func__, __LINE__);
                    if (server->effects_manager) {
                        axiom_effects_manager_init(server->effects_manager, &server->config->effects);
                        axiom_log_info("Effects configuration reloaded");
                    }
                }
            } else {
                axiom_config_destroy(new_config);
                axiom_log_warn("Failed to reload main configuration, keeping existing");
            }
        }
    }
    
    // Re-apply configuration to existing windows
    struct axiom_window *window;
    wl_list_for_each(window, &server->windows, link) {
        if (server->window_rules_manager) {
            axiom_window_rules_apply_to_window(server->window_rules_manager, window);
        }
    }
    
    axiom_log_info("Configuration reload complete");
}

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
        
        // Use the defined function instead of undefined one
        axiom_calculate_window_layout(server, index, &window->x, &window->y, &window->width, &window->height);
        
        // Validate window dimensions
        if (window->width <= 0) window->width = 400;
        if (window->height <= 0) window->height = 300;
        
        // Position the window using scene tree
        wlr_scene_node_set_position(&window->scene_tree->node, window->x, window->y);
        
        // Update window decorations to match new position and size
        axiom_update_window_decorations(window);
                
        uint32_t configure_serial = wlr_xdg_toplevel_set_size(
            window->xdg_toplevel, window->width, window->height);
        
        AXIOM_LOG_DEBUG("TILING", "Window %d positioned: x=%d, y=%d, w=%d, h=%d (serial=%u)", 
                        index, window->x, window->y, window->width, window->height, configure_serial);
        
        index++;
    }
}

void axiom_calculate_window_layout(struct axiom_server *server, int index, int *x, int *y, int *width, int *height) {
    // Ensure we have valid workspace dimensions
    if (server->workspace_width <= 0 || server->workspace_height <= 0) {
        AXIOM_LOG_WARN("WINDOW", "Invalid workspace dimensions: %dx%d, using fallback", 
                       server->workspace_width, server->workspace_height);
        *x = *y = 0;
        *width = AXIOM_DEFAULT_WINDOW_WIDTH;
        *height = AXIOM_DEFAULT_WINDOW_HEIGHT;
        return;
    }
    
    int window_count = server->window_count;
    
    // Add bounds checking for index
    if (index < 0) {
        AXIOM_LOG_ERROR("WINDOW", "Invalid window index: %d", index);
        index = 0;
    }
    
    if (window_count <= 0) {
        AXIOM_LOG_ERROR("WINDOW", "Invalid window count: %d", window_count);
        *x = *y = 0;
        *width = AXIOM_DEFAULT_WINDOW_WIDTH;
        *height = AXIOM_DEFAULT_WINDOW_HEIGHT;
        return;
    }
    
    if (window_count == 1) {
        // Single window takes full workspace with small margins
        const int margin = 20;
        *x = margin;
        *y = margin;
        *width = server->workspace_width - (2 * margin);
        *height = server->workspace_height - (2 * margin);
    } else if (window_count == 2) {
        // Two windows split vertically
        const int gap = 10;
        *width = (server->workspace_width - gap) / 2;
        *height = server->workspace_height - 40; // Leave room for title bars
        *x = index * (*width + gap);
        *y = 20; // Top margin
    } else {
        // Grid layout for 3+ windows
        int cols = (int)ceil(sqrt(window_count));
        int rows = (int)ceil((double)window_count / cols);
        
        // Ensure we don't exceed array bounds
        if (index >= window_count) {
            AXIOM_LOG_ERROR("WINDOW", "Window index %d exceeds count %d", index, window_count);
            index = window_count - 1;
        }
        
        const int gap = 5;
        const int title_bar_height = 30;
        
        *width = (server->workspace_width - (cols + 1) * gap) / cols;
        *height = (server->workspace_height - (rows + 1) * gap - title_bar_height) / rows;
        
        int col = index % cols;
        int row = index / cols;
        
        *x = gap + col * (*width + gap);
        *y = gap + title_bar_height + row * (*height + gap);
    }
    
    // Ensure minimum window dimensions
    if (*width < AXIOM_MIN_WINDOW_WIDTH) {
        AXIOM_LOG_WARN("WINDOW", "Calculated width %d below minimum, using %d", 
                       *width, AXIOM_MIN_WINDOW_WIDTH);
        *width = AXIOM_MIN_WINDOW_WIDTH;
    }
    if (*height < AXIOM_MIN_WINDOW_HEIGHT) {
        AXIOM_LOG_WARN("WINDOW", "Calculated height %d below minimum, using %d", 
                       *height, AXIOM_MIN_WINDOW_HEIGHT);
        *height = AXIOM_MIN_WINDOW_HEIGHT;
    }
    
    // Ensure windows don't go off-screen
    if (*x < 0) {
        AXIOM_LOG_WARN("WINDOW", "Window X position %d adjusted to 0", *x);
        *x = 0;
    }
    if (*y < 0) {
        AXIOM_LOG_WARN("WINDOW", "Window Y position %d adjusted to 0", *y);
        *y = 0;
    }
    if (*x + *width > server->workspace_width) {
        AXIOM_LOG_WARN("WINDOW", "Window extends beyond workspace width, adjusting");
        *x = server->workspace_width - *width;
        if (*x < 0) *x = 0;
    }
    if (*y + *height > server->workspace_height) {
        AXIOM_LOG_WARN("WINDOW", "Window extends beyond workspace height, adjusting");
        *y = server->workspace_height - *height;
        if (*y < 0) *y = 0;
    }
    
    AXIOM_LOG_DEBUG("WINDOW", "Layout calculated: index=%d, pos=(%d,%d), size=%dx%d, workspace=%dx%d", 
                    index, *x, *y, *width, *height, server->workspace_width, server->workspace_height);
}

static void window_map(struct wl_listener *listener, void *data) {
    (void)data; // Suppress unused parameter warning
    struct axiom_window *window = wl_container_of(listener, window, map);
    AXIOM_LOG_INFO("WINDOW", "Window mapped: %s", window->xdg_toplevel->title ?: "(no title)");
    
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
    AXIOM_LOG_INFO("WINDOW", "Window unmapped");
}

static void window_destroy(struct wl_listener *listener, void *data) {
    (void)data; // Suppress unused parameter warning
    struct axiom_window *window = wl_container_of(listener, window, destroy);
    struct axiom_server *server = window->server;
    
    AXIOM_LOG_INFO("WINDOW", "Window destroyed");
    
    // Cleanup window effects
    if (window->effects) {
        axiom_window_effects_destroy(window);
    }
    
    // Update the window count properly
    if (window->is_tiled && server->window_count > 0) {
        server->window_count--;
        AXIOM_LOG_DEBUG("TILING", "Tiled window destroyed, remaining: %d", server->window_count);
        
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
    axiom_free_tracked(window, __FILE__, __func__, __LINE__);
}

static void server_new_xdg_toplevel(struct wl_listener *listener, void *data) {
    struct axiom_server *server = wl_container_of(listener, server, new_xdg_toplevel);
    struct wlr_xdg_toplevel *xdg_toplevel = data;
    
    AXIOM_LOG_INFO("New XDG toplevel: %s", xdg_toplevel->title ?: "(no title)");
    
    struct axiom_window *window = axiom_calloc_tracked(1, sizeof(struct axiom_window), 
                                                      AXIOM_MEM_TYPE_WINDOW, __FILE__, __func__, __LINE__);
    if (!window) {
        return;
    }
    
    window->xdg_toplevel = xdg_toplevel;
    window->scene_tree = wlr_scene_xdg_surface_create(&server->scene->tree, xdg_toplevel->base);
    if (!window->scene_tree) {
        axiom_free_tracked(window, __FILE__, __func__, __LINE__);
        return;
    }
    
    window->scene_tree->node.data = window;
    window->server = server;
    
    // Initialize window type
    window->type = AXIOM_WINDOW_XDG;
    
    // Initialize window dimensions (default values)
    window->width = AXIOM_DEFAULT_WINDOW_WIDTH;
    window->height = AXIOM_DEFAULT_WINDOW_HEIGHT;
    window->x = AXIOM_DEFAULT_WINDOW_X;
    window->y = AXIOM_DEFAULT_WINDOW_Y;
    
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
        
        // Create title bar buttons after title bar is created
        axiom_create_title_bar_buttons(window);
        
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
            AXIOM_LOG_ERROR("Failed to initialize effects for window");
        } else {
            AXIOM_LOG_INFO("Window effects initialized successfully");
        }
    }
    
    // Apply window rules if rules system is enabled
    if (server->window_rules_manager) {
        // Debug window properties
        axiom_window_rules_debug_window_properties(window);
        
        // Apply matching rules
        if (!axiom_window_rules_apply_to_window(server->window_rules_manager, window)) {
            AXIOM_LOG_DEBUG("No window rules applied to this window");
        }
    }
    
    window->map.notify = window_map;
    wl_signal_add(&xdg_toplevel->base->surface->events.map, &window->map);
    
    window->unmap.notify = window_unmap;
    wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &window->unmap);
    
    window->destroy.notify = window_destroy;
    wl_signal_add(&xdg_toplevel->base->events.destroy, &window->destroy);
    
    // Also add to global window list for iteration
    wl_list_insert(&server->windows, &window->link);
    
    AXIOM_LOG_INFO("Window added, total tiled windows: %d", server->window_count);
}

static void server_new_output(struct wl_listener *listener, void *data) {
    struct axiom_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;
    
    AXIOM_LOG_INFO("New output: %s", wlr_output->name);
    AXIOM_LOG_DEBUG("Renderer=%p, cursor_mgr=%p, cursor=%p", 
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
        AXIOM_LOG_ERROR("Failed to commit output state for %s", wlr_output->name);
    }
    wlr_output_state_finish(&state);
    wlr_output_layout_add_auto(server->output_layout, wlr_output);
    struct axiom_output *output = axiom_calloc_tracked(1, sizeof(struct axiom_output), 
                                                       AXIOM_MEM_TYPE_SURFACE, __FILE__, __func__, __LINE__);
    if (!output) {
        wlr_output_state_finish(&state);
        return;
    }
    
    output->server = server;
    output->wlr_output = wlr_output;
    
    AXIOM_LOG_DEBUG("Creating scene output with scene=%p, wlr_output=%p", 
           (void*)server->scene, (void*)wlr_output);
    
    output->scene_output = wlr_scene_output_create(server->scene, wlr_output);
    if (!output->scene_output) {
        AXIOM_LOG_ERROR("Failed to create scene output");
        axiom_free_tracked(output, __FILE__, __func__, __LINE__);
        wlr_output_state_finish(&state);
        return;
    }
    
    AXIOM_LOG_DEBUG("Scene output created successfully for %s", wlr_output->name);
    
    // Lock software cursors to avoid hardware cursor renderer issues in nested mode
    wlr_output_lock_software_cursors(wlr_output, true);
    AXIOM_LOG_DEBUG("Locked software cursors for output %s", wlr_output->name);
    
    wl_list_insert(&server->outputs, &output->link);
    
    // Defer cursor theme loading completely to avoid renderer assertion during setup
    AXIOM_LOG_DEBUG("Deferring cursor theme loading for output %s", wlr_output->name);
    
    // Defer cursor attachment completely to avoid triggering cursor setup
    AXIOM_LOG_DEBUG("Deferring cursor attachment to output layout until cursor motion");
    
    // Update workspace dimensions based on output size
    if (wlr_output->current_mode) {
        server->workspace_width = wlr_output->current_mode->width;
        server->workspace_height = wlr_output->current_mode->height;
        AXIOM_LOG_INFO("Workspace dimensions set to: %dx%d", server->workspace_width, server->workspace_height);
        
        // Rearrange existing windows if tiling is enabled
        if (server->tiling_enabled) {
            axiom_arrange_windows(server);
        }
    }
}

int main(int argc, char *argv[]) {
    AXIOM_LOG_INFO("Axiom Wayland Compositor v" AXIOM_VERSION);
    
    // Initialize memory management system first
    axiom_result_t mem_result = axiom_memory_init();
    if (mem_result != AXIOM_SUCCESS) {
        AXIOM_LOG_ERROR("Failed to initialize memory management system");
        return EXIT_FAILURE;
    }
    
    // Set up proper environment for Wayland compositor operation
    if (!axiom_environment_setup()) {
        AXIOM_LOG_WARN("Environment setup completed with warnings, continuing...");
    }
    
    // Print environment information for debugging
    axiom_environment_print_info();
    
    // Parse command line arguments
    bool nested = false;
    AXIOM_LOG_DEBUG("Parsing %d command line arguments", argc);
    
    for (int i = 1; i < argc; i++) {
        AXIOM_LOG_DEBUG("Processing argument: '%s'", argv[i]);
        if (strcmp(argv[i], "--nested") == 0) {
            nested = true;
            AXIOM_LOG_INFO("Nested mode enabled");
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
        AXIOM_LOG_ERROR("Failed to create Wayland display");
        return EXIT_FAILURE;
    }
    
    server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
    // Create backend with session handling for better compatibility
    if (nested) {
        AXIOM_LOG_INFO("Running in nested mode");
        // For nested mode, we need to use a different backend approach
        server.backend = wlr_backend_autocreate(server.wl_event_loop, NULL);
    } else {
        // For main session mode, trust the login manager and let wlroots handle seat management
        AXIOM_LOG_INFO("Starting main session backend...");
        
        // Check for existing sessions before attempting to take control
        const char *wayland_display = getenv("WAYLAND_DISPLAY");
        const char *display = getenv("DISPLAY");
        
        if (wayland_display) {
            AXIOM_LOG_WARN("Running inside existing Wayland session (%s)", wayland_display);
            AXIOM_LOG_WARN("Consider using --nested flag or terminating existing session");
        }
        if (display) {
            AXIOM_LOG_WARN("Running inside existing X11 session (%s)", display);
        }
        
        // DO NOT clear environment variables - apps need them!
        
        // Create backend - wlroots will handle all seat management, DRM access, etc.
        server.backend = wlr_backend_autocreate(server.wl_event_loop, NULL);
    }
    
    if (!server.backend) {
        if (nested) {
            AXIOM_LOG_ERROR("Failed to create nested backend");
            AXIOM_LOG_ERROR("Make sure you're running inside a Wayland compositor");
        } else {
            AXIOM_LOG_ERROR("Failed to create backend");
            AXIOM_LOG_ERROR("This usually means:");
            AXIOM_LOG_ERROR("  1. Another session is already active on this seat");
            AXIOM_LOG_ERROR("  2. You don't have permission to access DRM devices");
            AXIOM_LOG_ERROR("  3. No suitable display hardware was found");
            AXIOM_LOG_ERROR("");
            AXIOM_LOG_ERROR("Try:");
            AXIOM_LOG_ERROR("  - Logging out of all other sessions");
            AXIOM_LOG_ERROR("  - Running with --nested flag for testing");
            AXIOM_LOG_ERROR("  - Checking if you're in the 'video' group: groups $USER");
        }
        return EXIT_FAILURE;
    }
    
    server.renderer = wlr_renderer_autocreate(server.backend);
    if (!server.renderer) {
        AXIOM_LOG_ERROR("Failed to create renderer");
        return EXIT_FAILURE;
    }
    
    AXIOM_LOG_DEBUG("Renderer created successfully: %p", (void*)server.renderer);
    
    if (!wlr_renderer_init_wl_display(server.renderer, server.wl_display)) {
        AXIOM_LOG_ERROR("Failed to initialize renderer with Wayland display");
        return EXIT_FAILURE;
    }
    
    AXIOM_LOG_DEBUG("Renderer initialized with Wayland display");
    
    server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
    server.compositor = wlr_compositor_create(server.wl_display, 5, server.renderer);
    server.scene = wlr_scene_create();
    server.output_layout = wlr_output_layout_create(server.wl_display);
    server.scene_layout = wlr_scene_attach_output_layout(server.scene, server.output_layout);
    
    // Add background to prevent black screen
    float bg_color[4] = {0.1, 0.1, 0.15, 1.0}; // Dark blue-grey background
    server.background = wlr_scene_rect_create(&server.scene->tree, 1920, 1080, bg_color);
    if (server.background) {
        wlr_scene_node_set_position(&server.background->node, 0, 0);
        AXIOM_LOG_DEBUG("Background created to prevent black screen");
    }
    
    // Ensure the scene is properly initialized with the renderer
    AXIOM_LOG_DEBUG("Scene created: %p, scene_layout: %p", (void*)server.scene, (void*)server.scene_layout);
    
    // Initialize lists
    wl_list_init(&server.windows);
    wl_list_init(&server.outputs);
    
    // Initialize window management
    server.tiling_enabled = true;  // Enable tiling by default
    server.window_count = 0;
    server.workspace_width = 1920;  // Default fallback
    server.workspace_height = 1080;
    
    // Initialize workspace management
    AXIOM_LOG_DEBUG("About to initialize workspaces...");
    axiom_init_workspaces(&server);
    AXIOM_LOG_DEBUG("Workspaces initialized successfully");
    server.xdg_shell = wlr_xdg_shell_create(server.wl_display, 3);
    if (!server.xdg_shell) {
        AXIOM_LOG_ERROR("Failed to create XDG shell");
        goto cleanup_error;
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
        AXIOM_LOG_ERROR("Failed to create configuration");
        goto cleanup_error;
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
        AXIOM_LOG_ERROR("Failed to initialize window snapping manager");
    } else {
        // Convert configuration structure and initialize snapping
        struct axiom_snapping_config snapping_config = {
            .snap_threshold = server.config->window_snapping.snap_threshold,
            .edge_resistance = server.config->window_snapping.edge_resistance,
            .magnetism_strength = server.config->window_snapping.magnetism_strength,
            .animation_duration = 200, // Default animation duration
            .smart_corners = server.config->window_snapping.smart_corners,
            .multi_monitor_snapping = server.config->window_snapping.multi_monitor_snapping,
            .window_to_window_snapping = server.config->window_snapping.window_to_window_snapping,
            .edge_snapping = server.config->window_snapping.edge_snapping
        };
        
        if (!axiom_window_snapping_manager_init(server.window_snapping_manager, &snapping_config)) {
            AXIOM_LOG_ERROR("Failed to initialize window snapping configuration");
        } else {
            AXIOM_LOG_INFO("Window snapping system initialized successfully");
        }
    }

    // Initialize process management
    axiom_process_init(&server);

    // Initialize animation system
    axiom_animation_manager_init(&server);
    
    // Initialize visual effects system
    server.effects_manager = axiom_calloc_tracked(1, sizeof(struct axiom_effects_manager), 
                                                  AXIOM_MEM_TYPE_EFFECTS, __FILE__, __func__, __LINE__);
    if (server.effects_manager && !axiom_effects_manager_init(server.effects_manager, &server.config->effects)) {
        AXIOM_LOG_ERROR("Failed to initialize effects manager");
        axiom_free_tracked(server.effects_manager, __FILE__, __func__, __LINE__);
        server.effects_manager = NULL;
    } else {
        AXIOM_LOG_INFO("Effects manager initialized successfully");
        
        // Initialize GPU acceleration for effects
        if (server.effects_manager && axiom_effects_gpu_init(server.effects_manager, &server)) {
            AXIOM_LOG_INFO("GPU acceleration enabled for visual effects");
            
            // Initialize real-time effects system
            if (!axiom_realtime_effects_init(server.effects_manager)) {
                AXIOM_LOG_ERROR("Failed to initialize real-time effects");
            } else {
                AXIOM_LOG_INFO("Real-time effects system initialized");
            }
        } else {
            AXIOM_LOG_INFO("GPU acceleration not available, using software fallback");
        }
    }
    
    // Initialize window rules system (Phase 3.1)
    if (!axiom_server_init_window_rules(&server)) {
        AXIOM_LOG_ERROR("Failed to initialize window rules system");
    } else {
        AXIOM_LOG_INFO("Window rules system initialized successfully");
        
        // Print loaded rules for debugging
        if (server.window_rules_manager) {
            axiom_window_rules_print_rules(server.window_rules_manager);
        }
    }
    
    // Initialize smart gaps system (Phase 3.2)
    if (!axiom_server_init_smart_gaps(&server, &server.config->smart_gaps)) {
        AXIOM_LOG_ERROR("Failed to initialize smart gaps system");
    } else {
        AXIOM_LOG_INFO("Smart gaps system initialized successfully");
        AXIOM_LOG_INFO("Default gap profiles loaded: %d profiles", 
               server.smart_gaps_manager ? server.smart_gaps_manager->profile_count : 0);
    }
    
    // Initialize thumbnail manager
    server.thumbnail_manager = axiom_thumbnail_manager_create(&server);
    if (!server.thumbnail_manager) {
        AXIOM_LOG_ERROR("Failed to initialize thumbnail manager");
    } else {
        AXIOM_LOG_INFO("Thumbnail manager initialized successfully");
    }
    
    // Initialize Picture-in-Picture system (Phase 3.1)
    if (!axiom_server_init_pip_manager(&server, &server.config->picture_in_picture)) {
        AXIOM_LOG_ERROR("Failed to initialize PiP manager");
    } else {
        AXIOM_LOG_INFO("Picture-in-Picture system initialized successfully");
        
        // Print PiP statistics for debugging
        if (server.pip_manager) {
            axiom_pip_print_stats(server.pip_manager);
        }
    }
    
    // Initialize keybinding manager (Phase 2.1 - Critical for keyboard shortcuts)
    server.keybinding_manager = axiom_calloc_tracked(1, sizeof(struct axiom_keybinding_manager), 
                                                     AXIOM_MEM_TYPE_CONFIG, __FILE__, __func__, __LINE__);
    if (!server.keybinding_manager) {
        AXIOM_LOG_ERROR("Failed to allocate keybinding manager");
    } else {
        axiom_keybinding_manager_init(server.keybinding_manager);
        AXIOM_LOG_INFO("Keybinding manager initialized with default shortcuts");
        
        // Print all loaded keybindings for debugging
        axiom_keybinding_print_all(server.keybinding_manager);
    }
    
    // Initialize focus manager (Phase 2.1 - Critical for Alt+Tab and focus management)
    server.focus_manager = axiom_calloc_tracked(1, sizeof(struct axiom_focus_manager), 
                                                AXIOM_MEM_TYPE_CONFIG, __FILE__, __func__, __LINE__);
    if (!server.focus_manager) {
        AXIOM_LOG_ERROR("Failed to allocate focus manager");
    } else {
        axiom_focus_manager_init(server.focus_manager);
        AXIOM_LOG_INFO("Focus manager initialized for window switching and focus management");
    }
    
    // Initialize desktop integration protocols
    
    // Initialize layer shell manager for panels/bars
    server.layer_shell_manager = axiom_layer_shell_manager_create(&server);
    if (!server.layer_shell_manager) {
        AXIOM_LOG_ERROR("Failed to create layer shell manager");
    } else if (!axiom_layer_shell_manager_init(server.layer_shell_manager)) {
        AXIOM_LOG_ERROR("Failed to initialize layer shell manager");
        axiom_layer_shell_manager_destroy(server.layer_shell_manager);
        server.layer_shell_manager = NULL;
    } else {
        AXIOM_LOG_INFO("Layer shell protocol initialized - panels and bars can now connect");
    }
    
    // Initialize screenshot manager
    server.screenshot_manager = axiom_screenshot_manager_create(&server);
    if (!server.screenshot_manager) {
        AXIOM_LOG_ERROR("Failed to create screenshot manager");
    } else if (!axiom_screenshot_manager_init(server.screenshot_manager)) {
        AXIOM_LOG_ERROR("Failed to initialize screenshot manager");
        axiom_screenshot_manager_destroy(server.screenshot_manager);
        server.screenshot_manager = NULL;
    } else {
        AXIOM_LOG_INFO("Screenshot protocols initialized - screen capture available");
    }
    
    // Initialize session manager
    server.session_manager = axiom_session_manager_create(&server);
    if (!server.session_manager) {
        AXIOM_LOG_ERROR("Failed to create session manager");
    } else if (!axiom_session_manager_init(server.session_manager)) {
        AXIOM_LOG_ERROR("Failed to initialize session manager");
        axiom_session_manager_destroy(server.session_manager);
        server.session_manager = NULL;
    } else {
        AXIOM_LOG_INFO("Session management protocols initialized - locking and idle control available");
    }
    
    // Initialize XWayland support
    server.xwayland_manager = axiom_xwayland_manager_create(&server);
    if (!server.xwayland_manager) {
        AXIOM_LOG_ERROR("Failed to create XWayland manager");
    } else if (!axiom_xwayland_manager_init(server.xwayland_manager)) {
        AXIOM_LOG_ERROR("Failed to initialize XWayland manager");
        axiom_xwayland_manager_destroy(server.xwayland_manager);
        server.xwayland_manager = NULL;
    } else {
        AXIOM_LOG_INFO("XWayland support initialized - X11 applications can now run");
    }
    
    // Set up input management
    wl_list_init(&server.input_devices);
    
    server.cursor = wlr_cursor_create();
    if (!server.cursor) {
        AXIOM_LOG_ERROR("Failed to create cursor");
        return EXIT_FAILURE;
    }
    
    AXIOM_LOG_DEBUG("Cursor created: %p", (void*)server.cursor);
    
    // Try to create cursor manager even in nested mode, but with special handling
    server.cursor_mgr = wlr_xcursor_manager_create(server.config->cursor_theme, server.config->cursor_size);
    if (!server.cursor_mgr) {
        if (nested) {
            AXIOM_LOG_DEBUG("Failed to create cursor manager in nested mode (expected)");
        } else {
            AXIOM_LOG_ERROR("Failed to create cursor manager");
            return EXIT_FAILURE;
        }
    } else {
        AXIOM_LOG_DEBUG("Cursor manager created: %p (theme=%s, size=%d)", 
               (void*)server.cursor_mgr, server.config->cursor_theme, server.config->cursor_size);
    }
    
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
    
    // Note: Cursor will be attached to output layout after backend starts
    
    server.new_input.notify = axiom_new_input;
    wl_signal_add(&server.backend->events.new_input, &server.new_input);
    
    const char *socket = wl_display_add_socket_auto(server.wl_display);
    if (!socket) {
        AXIOM_LOG_ERROR("Failed to add socket");
        return EXIT_FAILURE;
    }
    
    AXIOM_LOG_INFO("Starting backend...");
    if (!wlr_backend_start(server.backend)) {
        if (nested) {
            AXIOM_LOG_ERROR("Failed to start nested backend");
            AXIOM_LOG_ERROR("Make sure you're running inside a Wayland compositor");
        } else {
            AXIOM_LOG_ERROR("Failed to start backend");
            AXIOM_LOG_ERROR("This usually indicates:");
            AXIOM_LOG_ERROR("  1. Another session is controlling the display");
            AXIOM_LOG_ERROR("  2. Permission denied accessing hardware");
            AXIOM_LOG_ERROR("  3. Display manager conflict");
            AXIOM_LOG_ERROR("");
            AXIOM_LOG_ERROR("To fix this:");
            AXIOM_LOG_ERROR("  - Log out completely from other desktop sessions");
            AXIOM_LOG_ERROR("  - Make sure no other compositor is running");
            AXIOM_LOG_ERROR("  - Try: sudo loginctl terminate-session <session-id>");
            AXIOM_LOG_ERROR("  - Or use --nested flag for development/testing");
        }
        
        // Clean exit instead of allowing potential abort()
        axiom_config_destroy(server.config);
        wl_display_destroy(server.wl_display);
        return EXIT_FAILURE;
    }
    
    axiom_environment_set_wayland_display(socket);
    server.running = true;
    
    AXIOM_LOG_INFO("Axiom running on WAYLAND_DISPLAY=%s", socket);
    
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
                uint32_t threshold_ms = 16;
                axiom_effects_throttle_updates(server.effects_manager, &threshold_ms);
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
        axiom_free_tracked(server.effects_manager, __FILE__, __func__, __LINE__);
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
    
// Cleanup PiP manager
    if (server.pip_manager) {
    axiom_server_destroy_pip_manager(&server);
    }

    // Cleanup thumbnail manager
    if (server.thumbnail_manager) {
        axiom_thumbnail_manager_destroy(server.thumbnail_manager);
    }
    
    // Cleanup keybinding manager
    if (server.keybinding_manager) {
        axiom_keybinding_manager_cleanup(server.keybinding_manager);
        axiom_free_tracked(server.keybinding_manager, __FILE__, __func__, __LINE__);
    }
    
    // Cleanup focus manager
    if (server.focus_manager) {
        axiom_focus_manager_cleanup(server.focus_manager);
        axiom_free_tracked(server.focus_manager, __FILE__, __func__, __LINE__);
    }
    
    // Cleanup desktop integration protocols
    if (server.layer_shell_manager) {
        axiom_layer_shell_manager_destroy(server.layer_shell_manager);
    }
    
    if (server.screenshot_manager) {
        axiom_screenshot_manager_destroy(server.screenshot_manager);
    }
    
    if (server.session_manager) {
        axiom_session_manager_destroy(server.session_manager);
    }
    
    if (server.xwayland_manager) {
        axiom_xwayland_manager_destroy(server.xwayland_manager);
    }
    
    axiom_config_destroy(server.config);
    wl_display_destroy(server.wl_display);
    
    // Shutdown memory management system last
    axiom_memory_shutdown();
    
    return EXIT_SUCCESS;

cleanup_error:
    axiom_log_error("Fatal error during initialization, cleaning up...");
    
    // Cleanup in reverse order of initialization
    if (server.effects_manager) {
        axiom_effects_manager_destroy(server.effects_manager);
        axiom_free_tracked(server.effects_manager, __FILE__, __func__, __LINE__);
    }
    
    if (server.config) {
        axiom_config_destroy(server.config);
    }
    
    if (server.wl_display) {
        wl_display_destroy(server.wl_display);
    }
    
    return EXIT_FAILURE;
}
