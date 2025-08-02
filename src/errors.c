#include "logging.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include "axiom.h"
#include "errors.h"
#include "constants.h"

/**
 * @file errors.c
 * @brief Implementation of comprehensive error handling system
 * 
 * This implements the roadmap Phase 1.1 requirement:
 * "Add comprehensive error handling - replace silent failures with proper error reporting"
 */

// Thread-local error context storage
static __thread struct axiom_error_context g_error_context = {0};
static __thread bool g_error_context_set = false;

/**
 * Convert error code to human-readable string
 */
const char* axiom_error_string(axiom_result_t result) {
    switch (result) {
        case AXIOM_SUCCESS:
            return "Success";
            
        // Memory errors
        case AXIOM_ERROR_MEMORY:
            return "Memory error";
        case AXIOM_ERROR_ALLOCATION_FAILED:
            return "Memory allocation failed";
        case AXIOM_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case AXIOM_ERROR_NULL_POINTER:
            return "Null pointer dereference";
            
        // Graphics errors
        case AXIOM_ERROR_GRAPHICS:
            return "Graphics error";
        case AXIOM_ERROR_RENDERER_INIT:
            return "Renderer initialization failed";
        case AXIOM_ERROR_SHADER_COMPILE:
            return "Shader compilation failed";
        case AXIOM_ERROR_TEXTURE_CREATION:
            return "Texture creation failed";
        case AXIOM_ERROR_FRAMEBUFFER:
            return "Framebuffer error";
        case AXIOM_ERROR_GPU_ACCELERATION:
            return "GPU acceleration unavailable";
            
        // Configuration errors
        case AXIOM_ERROR_CONFIG:
            return "Configuration error";
        case AXIOM_ERROR_CONFIG_PARSE:
            return "Configuration parsing failed";
        case AXIOM_ERROR_CONFIG_INVALID:
            return "Invalid configuration";
        case AXIOM_ERROR_CONFIG_FILE_NOT_FOUND:
            return "Configuration file not found";
        case AXIOM_ERROR_CONFIG_PERMISSION:
            return "Configuration file permission denied";
            
        // Protocol errors
        case AXIOM_ERROR_PROTOCOL:
            return "Protocol error";
        case AXIOM_ERROR_WAYLAND_DISPLAY:
            return "Wayland display creation failed";
        case AXIOM_ERROR_WAYLAND_BACKEND:
            return "Wayland backend initialization failed";
        case AXIOM_ERROR_XDG_SHELL:
            return "XDG shell creation failed";
        case AXIOM_ERROR_SEAT_CREATION:
            return "Seat creation failed";
        case AXIOM_ERROR_OUTPUT_CREATION:
            return "Output creation failed";
            
        // Window management errors
        case AXIOM_ERROR_WINDOW:
            return "Window management error";
        case AXIOM_ERROR_WINDOW_CREATION:
            return "Window creation failed";
        case AXIOM_ERROR_WINDOW_POSITIONING:
            return "Window positioning failed";
        case AXIOM_ERROR_WINDOW_FOCUS:
            return "Window focus failed";
        case AXIOM_ERROR_WINDOW_DECORATION:
            return "Window decoration failed";
        case AXIOM_ERROR_SCENE_TREE:
            return "Scene tree creation failed";
            
        // Input errors
        case AXIOM_ERROR_INPUT:
            return "Input handling error";
        case AXIOM_ERROR_KEYBOARD_INIT:
            return "Keyboard initialization failed";
        case AXIOM_ERROR_CURSOR_CREATION:
            return "Cursor creation failed";
        case AXIOM_ERROR_CURSOR_THEME:
            return "Cursor theme loading failed";
        case AXIOM_ERROR_INPUT_DEVICE:
            return "Input device error";
            
        // Animation errors
        case AXIOM_ERROR_ANIMATION:
            return "Animation error";
        case AXIOM_ERROR_ANIMATION_INIT:
            return "Animation system initialization failed";
        case AXIOM_ERROR_EFFECTS_INIT:
            return "Effects system initialization failed";
        case AXIOM_ERROR_REALTIME_EFFECTS:
            return "Real-time effects error";
            
        // File system errors
        case AXIOM_ERROR_FILE:
            return "File system error";
        case AXIOM_ERROR_FILE_READ:
            return "File read error";
        case AXIOM_ERROR_FILE_WRITE:
            return "File write error";
        case AXIOM_ERROR_FILE_PERMISSION:
            return "File permission denied";
        case AXIOM_ERROR_DIRECTORY_CREATE:
            return "Directory creation failed";
            
        // System errors
        case AXIOM_ERROR_SYSTEM:
            return "System error";
        case AXIOM_ERROR_SIGNAL_HANDLER:
            return "Signal handler error";
        case AXIOM_ERROR_PROCESS_SPAWN:
            return "Process spawn failed";
        case AXIOM_ERROR_PERMISSION_DENIED:
            return "Permission denied";
        case AXIOM_ERROR_RESOURCE_BUSY:
            return "Resource busy";
            
        // Debug errors
        case AXIOM_ERROR_DEBUG:
            return "Debug error";
        case AXIOM_ERROR_ASSERTION_FAILED:
            return "Assertion failed";
        case AXIOM_ERROR_INVALID_STATE:
            return "Invalid state";
        case AXIOM_ERROR_NOT_IMPLEMENTED:
            return "Feature not implemented";
            
        case AXIOM_ERROR_UNKNOWN:
        default:
            return "Unknown error";
    }
}

/**
 * Set error context for detailed error reporting
 */
void axiom_error_set_context(axiom_result_t code, const char *message, 
                            const char *function, const char *file, int line) {
    g_error_context.code = code;
    g_error_context.message = message;
    g_error_context.function = function;
    g_error_context.file = file;
    g_error_context.line = line;
    g_error_context.cause = NULL; // TODO: Implement error chaining
    g_error_context_set = true;
}

/**
 * Get the last error context
 */
const struct axiom_error_context* axiom_error_get_context(void) {
    return g_error_context_set ? &g_error_context : NULL;
}

/**
 * Clear the error context
 */
void axiom_error_clear_context(void) {
    memset(&g_error_context, 0, sizeof(g_error_context));
    g_error_context_set = false;
}

/**
 * Log error with full context information
 */
void axiom_error_log(axiom_result_t result) {
    const char *error_str = axiom_error_string(result);
    
    if (g_error_context_set) {
        AXIOM_LOG_ERROR("Error %d (%s): %s at %s() in %s:%d", 
                       result, error_str, g_error_context.message,
                       g_error_context.function, g_error_context.file, g_error_context.line);
    } else {
        AXIOM_LOG_ERROR("Error %d: %s", result, error_str);
    }
}

/**
 * Safe memory allocation with error handling
 */
void* axiom_malloc_safe(size_t size, axiom_result_t *result) {
    if (size == 0) {
        if (result) *result = AXIOM_ERROR_ALLOCATION_FAILED;
        axiom_error_set_context(AXIOM_ERROR_ALLOCATION_FAILED, "Zero size allocation", 
                               __func__, __FILE__, __LINE__);
        return NULL;
    }
    
    void *ptr = malloc(size);
    if (!ptr) {
        if (result) *result = AXIOM_ERROR_ALLOCATION_FAILED;
        axiom_error_set_context(AXIOM_ERROR_ALLOCATION_FAILED, "malloc() failed", 
                               __func__, __FILE__, __LINE__);
        AXIOM_LOG_ERROR("Failed to allocate %zu bytes", size);
        return NULL;
    }
    
    if (result) *result = AXIOM_SUCCESS;
    return ptr;
}

/**
 * Safe zero-initialized memory allocation
 */
void* axiom_calloc_safe(size_t count, size_t size, axiom_result_t *result) {
    if (count == 0 || size == 0) {
        if (result) *result = AXIOM_ERROR_ALLOCATION_FAILED;
        axiom_error_set_context(AXIOM_ERROR_ALLOCATION_FAILED, "Zero size calloc", 
                               __func__, __FILE__, __LINE__);
        return NULL;
    }
    
    void *ptr = calloc(count, size);
    if (!ptr) {
        if (result) *result = AXIOM_ERROR_ALLOCATION_FAILED;
        axiom_error_set_context(AXIOM_ERROR_ALLOCATION_FAILED, "calloc() failed", 
                               __func__, __FILE__, __LINE__);
        AXIOM_LOG_ERROR("Failed to allocate %zu * %zu bytes", count, size);
        return NULL;
    }
    
    if (result) *result = AXIOM_SUCCESS;
    return ptr;
}

/**
 * Safe memory reallocation
 */
void* axiom_realloc_safe(void *ptr, size_t size, axiom_result_t *result) {
    if (size == 0) {
        // realloc with size 0 is equivalent to free
        free(ptr);
        if (result) *result = AXIOM_SUCCESS;
        return NULL;
    }
    
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        if (result) *result = AXIOM_ERROR_ALLOCATION_FAILED;
        axiom_error_set_context(AXIOM_ERROR_ALLOCATION_FAILED, "realloc() failed", 
                               __func__, __FILE__, __LINE__);
        AXIOM_LOG_ERROR("Failed to reallocate to %zu bytes", size);
        return NULL;
    }
    
    if (result) *result = AXIOM_SUCCESS;
    return new_ptr;
}

/**
 * Safe memory deallocation (always succeeds)
 */
void axiom_free_safe(void *ptr) {
    free(ptr); // free() handles NULL pointers gracefully
}

/**
 * Window creation with comprehensive error handling
 * This replaces the silent failures in window creation
 */
axiom_result_t axiom_window_create_safe(struct axiom_server *server, 
                                       struct wlr_xdg_toplevel *xdg_toplevel,
                                       struct axiom_window **window) {
    AXIOM_CHECK_NULL(server, AXIOM_ERROR_NULL_POINTER);
    AXIOM_CHECK_NULL(xdg_toplevel, AXIOM_ERROR_NULL_POINTER);
    AXIOM_CHECK_NULL(window, AXIOM_ERROR_NULL_POINTER);
    
    // Allocate window structure
    axiom_result_t result;
    struct axiom_window *new_window = axiom_calloc_safe(1, sizeof(struct axiom_window), &result);
    if (axiom_is_error(result)) {
        return result;
    }
    
    // Initialize window basic properties
    new_window->server = server;
    new_window->xdg_toplevel = xdg_toplevel;
    new_window->width = AXIOM_DEFAULT_WINDOW_WIDTH;
    new_window->height = AXIOM_DEFAULT_WINDOW_HEIGHT;
    new_window->x = AXIOM_DEFAULT_WINDOW_X;
    new_window->y = AXIOM_DEFAULT_WINDOW_Y;
    
    // Create scene tree
    new_window->scene_tree = wlr_scene_xdg_surface_create(&server->scene->tree, xdg_toplevel->base);
    if (!new_window->scene_tree) {
        axiom_free_safe(new_window);
        return AXIOM_ERROR_SCENE_TREE;
    }
    
    new_window->scene_tree->node.data = new_window;
    
    // Initialize tiling properties
    new_window->is_tiled = server->tiling_enabled;
    if (new_window->is_tiled) {
        server->window_count++;
    }
    
    *window = new_window;
    return AXIOM_SUCCESS;
}

/**
 * Server initialization with proper error handling
 * This replaces silent failures in server initialization
 */
axiom_result_t axiom_server_init_safe(struct axiom_server *server) {
    AXIOM_CHECK_NULL(server, AXIOM_ERROR_NULL_POINTER);
    
    // Initialize Wayland display
    server->wl_display = wl_display_create();
    AXIOM_CHECK_CONDITION(server->wl_display, AXIOM_ERROR_WAYLAND_DISPLAY, 
                         "Failed to create Wayland display");
    
    // Get event loop
    server->wl_event_loop = wl_display_get_event_loop(server->wl_display);
    AXIOM_CHECK_CONDITION(server->wl_event_loop, AXIOM_ERROR_WAYLAND_DISPLAY,
                         "Failed to get Wayland event loop");
    
    // Create backend
    server->backend = wlr_backend_autocreate(server->wl_event_loop, NULL);
    AXIOM_CHECK_CONDITION(server->backend, AXIOM_ERROR_WAYLAND_BACKEND,
                         "Failed to create Wayland backend");
    
    // Create renderer
    server->renderer = wlr_renderer_autocreate(server->backend);
    AXIOM_CHECK_CONDITION(server->renderer, AXIOM_ERROR_RENDERER_INIT,
                         "Failed to create renderer");
    
    // Initialize renderer with display
    bool renderer_init = wlr_renderer_init_wl_display(server->renderer, server->wl_display);
    AXIOM_CHECK_CONDITION(renderer_init, AXIOM_ERROR_RENDERER_INIT,
                         "Failed to initialize renderer with Wayland display");
    
    // Initialize lists
    wl_list_init(&server->windows);
    wl_list_init(&server->outputs);
    wl_list_init(&server->input_devices);
    
    // Set initial state
    server->running = false;
    server->tiling_enabled = true;
    server->window_count = 0;
    server->workspace_width = AXIOM_DEFAULT_WORKSPACE_WIDTH;
    server->workspace_height = AXIOM_DEFAULT_WORKSPACE_HEIGHT;
    server->focused_window = NULL;
    server->grabbed_window = NULL;
    server->cursor_mode = AXIOM_CURSOR_PASSTHROUGH;
    
    return AXIOM_SUCCESS;
}

/**
 * Configuration loading with error handling
 */
axiom_result_t axiom_config_load_safe(const char *config_path) {
    AXIOM_CHECK_NULL(config_path, AXIOM_ERROR_NULL_POINTER);
    
    // Check if file exists and is readable
    FILE *file = fopen(config_path, "r");
    if (!file) {
        axiom_error_set_context(AXIOM_ERROR_CONFIG_FILE_NOT_FOUND, 
                               "Configuration file not found or not readable",
                               __func__, __FILE__, __LINE__);
        return AXIOM_ERROR_CONFIG_FILE_NOT_FOUND;
    }
    
    fclose(file);
    
    // TODO: Implement actual configuration parsing
    // For now, just verify the file exists
    
    return AXIOM_SUCCESS;
}
