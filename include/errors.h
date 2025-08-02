#ifndef AXIOM_ERRORS_H
#define AXIOM_ERRORS_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @file errors.h
 * @brief Comprehensive error handling system for Axiom compositor
 * 
 * This implements proper error codes and handling as specified in the roadmap
 * Phase 1.1: Add comprehensive error handling
 */

// Forward declarations
struct axiom_server;
struct axiom_window;

/**
 * Axiom error codes - comprehensive error handling system
 * These replace silent failures with proper error reporting
 */
typedef enum {
    AXIOM_SUCCESS = 0,
    
    // Memory errors
    AXIOM_ERROR_MEMORY = 1,
    AXIOM_ERROR_ALLOCATION_FAILED,
    AXIOM_ERROR_OUT_OF_MEMORY,
    AXIOM_ERROR_NULL_POINTER,
    
    // Graphics and rendering errors
    AXIOM_ERROR_GRAPHICS = 100,
    AXIOM_ERROR_RENDERER_INIT,
    AXIOM_ERROR_SHADER_COMPILE,
    AXIOM_ERROR_TEXTURE_CREATION,
    AXIOM_ERROR_FRAMEBUFFER,
    AXIOM_ERROR_GPU_ACCELERATION,
    
    // Configuration errors
    AXIOM_ERROR_CONFIG = 200,
    AXIOM_ERROR_CONFIG_PARSE,
    AXIOM_ERROR_CONFIG_INVALID,
    AXIOM_ERROR_CONFIG_FILE_NOT_FOUND,
    AXIOM_ERROR_CONFIG_PERMISSION,
    
    // Protocol and Wayland errors
    AXIOM_ERROR_PROTOCOL = 300,
    AXIOM_ERROR_WAYLAND_DISPLAY,
    AXIOM_ERROR_WAYLAND_BACKEND,
    AXIOM_ERROR_XDG_SHELL,
    AXIOM_ERROR_SEAT_CREATION,
    AXIOM_ERROR_OUTPUT_CREATION,
    
    // Window management errors
    AXIOM_ERROR_WINDOW = 400,
    AXIOM_ERROR_WINDOW_CREATION,
    AXIOM_ERROR_WINDOW_POSITIONING,
    AXIOM_ERROR_WINDOW_FOCUS,
    AXIOM_ERROR_WINDOW_DECORATION,
    AXIOM_ERROR_SCENE_TREE,
    
    // Input handling errors
    AXIOM_ERROR_INPUT = 500,
    AXIOM_ERROR_KEYBOARD_INIT,
    AXIOM_ERROR_CURSOR_CREATION,
    AXIOM_ERROR_CURSOR_THEME,
    AXIOM_ERROR_INPUT_DEVICE,
    
    // Animation and effects errors
    AXIOM_ERROR_ANIMATION = 600,
    AXIOM_ERROR_ANIMATION_INIT,
    AXIOM_ERROR_EFFECTS_INIT,
    AXIOM_ERROR_REALTIME_EFFECTS,
    
    // File system errors
    AXIOM_ERROR_FILE = 700,
    AXIOM_ERROR_FILE_READ,
    AXIOM_ERROR_FILE_WRITE,
    AXIOM_ERROR_FILE_PERMISSION,
    AXIOM_ERROR_DIRECTORY_CREATE,
    
    // System errors
    AXIOM_ERROR_SYSTEM = 800,
    AXIOM_ERROR_SIGNAL_HANDLER,
    AXIOM_ERROR_PROCESS_SPAWN,
    AXIOM_ERROR_PERMISSION_DENIED,
    AXIOM_ERROR_RESOURCE_BUSY,
    
    // Development and debugging errors
    AXIOM_ERROR_DEBUG = 900,
    AXIOM_ERROR_ASSERTION_FAILED,
    AXIOM_ERROR_INVALID_STATE,
    AXIOM_ERROR_NOT_IMPLEMENTED,
    
    // Generic catch-all
    AXIOM_ERROR_UNKNOWN = 999
} axiom_result_t;

/**
 * Error context - provides additional information about errors
 */
struct axiom_error_context {
    axiom_result_t code;
    const char *message;
    const char *function;
    const char *file;
    int line;
    struct axiom_error_context *cause; // Chain errors
};

/**
 * Error handling macros - replace silent failures with proper error reporting
 * These implement the roadmap requirement: "Add comprehensive error handling"
 */

// Check for null pointers and return error
#define AXIOM_CHECK_NULL(ptr, error_code) \
    do { \
        if (!(ptr)) { \
            axiom_error_set_context(error_code, "NULL pointer: " #ptr, __func__, __FILE__, __LINE__); \
            return error_code; \
        } \
    } while (0)

// Check for memory allocation failures
#define AXIOM_CHECK_ALLOC(ptr) \
    do { \
        if (!(ptr)) { \
            axiom_error_set_context(AXIOM_ERROR_ALLOCATION_FAILED, "Memory allocation failed: " #ptr, __func__, __FILE__, __LINE__); \
            return AXIOM_ERROR_ALLOCATION_FAILED; \
        } \
    } while (0)

// Check for boolean conditions and return error if false
#define AXIOM_CHECK_CONDITION(condition, error_code, message) \
    do { \
        if (!(condition)) { \
            axiom_error_set_context(error_code, message, __func__, __FILE__, __LINE__); \
            return error_code; \
        } \
    } while (0)

// Execute code and propagate errors
#define AXIOM_TRY(call) \
    do { \
        axiom_result_t _result = (call); \
        if (_result != AXIOM_SUCCESS) { \
            return _result; \
        } \
    } while (0)

// Execute code with custom error handling
#define AXIOM_TRY_CLEANUP(call, cleanup) \
    do { \
        axiom_result_t _result = (call); \
        if (_result != AXIOM_SUCCESS) { \
            cleanup; \
            return _result; \
        } \
    } while (0)

/**
 * Core error handling functions
 */

/**
 * Convert error code to human-readable string
 * @param result Error code
 * @return Human-readable error description
 */
const char* axiom_error_string(axiom_result_t result);

/**
 * Set error context for detailed error reporting
 * @param code Error code
 * @param message Error message
 * @param function Function name where error occurred
 * @param file File name where error occurred
 * @param line Line number where error occurred
 */
void axiom_error_set_context(axiom_result_t code, const char *message, 
                            const char *function, const char *file, int line);

/**
 * Get the last error context
 * @return Pointer to last error context, or NULL if no error
 */
const struct axiom_error_context* axiom_error_get_context(void);

/**
 * Clear the error context
 */
void axiom_error_clear_context(void);

/**
 * Log error with full context information
 * @param result Error code to log
 */
void axiom_error_log(axiom_result_t result);

/**
 * Check if result indicates success
 * @param result Result code to check
 * @return true if success, false if error
 */
static inline bool axiom_is_success(axiom_result_t result) {
    return result == AXIOM_SUCCESS;
}

/**
 * Check if result indicates error
 * @param result Result code to check  
 * @return true if error, false if success
 */
static inline bool axiom_is_error(axiom_result_t result) {
    return result != AXIOM_SUCCESS;
}

/**
 * Enhanced memory allocation with error handling
 * These replace the raw malloc/calloc calls mentioned in the roadmap
 */

/**
 * Safe memory allocation with error handling
 * @param size Size to allocate
 * @param result Pointer to store result code
 * @return Allocated memory or NULL on failure
 */
void* axiom_malloc_safe(size_t size, axiom_result_t *result);

/**
 * Safe zero-initialized memory allocation
 * @param count Number of elements
 * @param size Size of each element
 * @param result Pointer to store result code
 * @return Allocated memory or NULL on failure
 */
void* axiom_calloc_safe(size_t count, size_t size, axiom_result_t *result);

/**
 * Safe memory reallocation
 * @param ptr Pointer to reallocate
 * @param size New size
 * @param result Pointer to store result code
 * @return Reallocated memory or NULL on failure
 */
void* axiom_realloc_safe(void *ptr, size_t size, axiom_result_t *result);

/**
 * Safe memory deallocation (always succeeds)
 * @param ptr Pointer to free (can be NULL)
 */
void axiom_free_safe(void *ptr);

/**
 * Specialized error handling for different subsystems
 */

/**
 * Window creation with comprehensive error handling
 * @param server Axiom server instance
 * @param xdg_toplevel XDG toplevel surface
 * @param window Output window pointer
 * @return Result code
 */
axiom_result_t axiom_window_create_safe(struct axiom_server *server, 
                                       struct wlr_xdg_toplevel *xdg_toplevel,
                                       struct axiom_window **window);

/**
 * Server initialization with proper error handling
 * @param server Server instance to initialize
 * @return Result code
 */
axiom_result_t axiom_server_init_safe(struct axiom_server *server);

/**
 * Configuration loading with error handling
 * @param config_path Path to configuration file
 * @return Result code
 */
axiom_result_t axiom_config_load_safe(const char *config_path);

#endif /* AXIOM_ERRORS_H */
