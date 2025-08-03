#ifndef AXIOM_MEMORY_H
#define AXIOM_MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "errors.h"
#include "constants.h"

/**
 * @file memory.h
 * @brief Advanced memory management system with leak detection and reference counting
 * 
 * This implements the roadmap Phase 1.1 requirement:
 * "Fix memory management - Add proper cleanup in error paths, implement reference counting"
 */

// =============================================================================
// MEMORY TRACKING AND LEAK DETECTION
// =============================================================================

/** Memory allocation types for tracking */
typedef enum {
    AXIOM_MEM_TYPE_GENERAL = 0,
    AXIOM_MEM_TYPE_WINDOW,
    AXIOM_MEM_TYPE_CONFIG,
    AXIOM_MEM_TYPE_SURFACE,
    AXIOM_MEM_TYPE_SCENE,
    AXIOM_MEM_TYPE_BUFFER,
    AXIOM_MEM_TYPE_STRING,
    AXIOM_MEM_TYPE_EFFECTS,
    AXIOM_MEM_TYPE_PERFORMANCE,
    AXIOM_MEM_TYPE_LAYER_SHELL,
    AXIOM_MEM_TYPE_SCREENSHOT,
    AXIOM_MEM_TYPE_SESSION,
    AXIOM_MEM_TYPE_XWAYLAND,
    AXIOM_MEM_TYPE_WINDOW_MANAGER,
    AXIOM_MEM_TYPE_WINDOW_STATE,
    AXIOM_MEM_TYPE_WINDOW_GEOMETRY,
    AXIOM_MEM_TYPE_WINDOW_LAYOUT,
    AXIOM_MEM_TYPE_FOCUS,
    AXIOM_MEM_TYPE_COUNT
} axiom_memory_type_t;

/** Memory allocation info for leak tracking */
struct axiom_memory_info {
    void *ptr;
    size_t size;
    axiom_memory_type_t type;
    const char *file;
    const char *function;
    int line;
    uint64_t timestamp;
    struct axiom_memory_info *next;
};

/** Memory statistics */
struct axiom_memory_stats {
    uint64_t total_allocations;
    uint64_t total_deallocations;
    uint64_t current_allocations;
    uint64_t peak_allocations;
    uint64_t total_bytes_allocated;
    uint64_t total_bytes_freed;
    uint64_t current_bytes_used;
    uint64_t peak_bytes_used;
    uint64_t allocation_count_by_type[AXIOM_MEM_TYPE_COUNT];
    uint64_t bytes_by_type[AXIOM_MEM_TYPE_COUNT];
};

// =============================================================================
// REFERENCE COUNTING SYSTEM
// =============================================================================

/** Reference counted object header */
struct axiom_ref_count {
    uint32_t count;
    void (*destructor)(void *ptr);
    axiom_memory_type_t type;
};

/** Get reference count header from object pointer */
#define AXIOM_REF_HEADER(ptr) \
    ((struct axiom_ref_count *)((char *)(ptr) - sizeof(struct axiom_ref_count)))

// =============================================================================
// MEMORY MANAGEMENT API
// =============================================================================

/** Initialize memory management system */
axiom_result_t axiom_memory_init(void);

/** Shutdown memory management system and report leaks */
void axiom_memory_shutdown(void);

/** Enable/disable memory leak detection */
void axiom_memory_set_leak_detection(bool enabled);

/** Get current memory statistics */
struct axiom_memory_stats axiom_memory_get_stats(void);

/** Print memory statistics to log */
void axiom_memory_print_stats(void);

/** Print current memory leaks */
void axiom_memory_print_leaks(void);

/** Check for memory leaks and return count */
uint64_t axiom_memory_check_leaks(void);

// =============================================================================
// TRACKED ALLOCATION FUNCTIONS
// =============================================================================

/** Tracked malloc with leak detection */
void *axiom_malloc_tracked(size_t size, axiom_memory_type_t type, 
                          const char *file, const char *func, int line);

/** Tracked calloc with leak detection */
void *axiom_calloc_tracked(size_t count, size_t size, axiom_memory_type_t type,
                          const char *file, const char *func, int line);

/** Tracked realloc with leak detection */
void *axiom_realloc_tracked(void *ptr, size_t size, axiom_memory_type_t type,
                           const char *file, const char *func, int line);

/** Tracked free with leak detection */
void axiom_free_tracked(void *ptr, const char *file, const char *func, int line);

/** Tracked strdup with leak detection */
char *axiom_strdup_tracked(const char *str, const char *file, const char *func, int line);

// =============================================================================
// REFERENCE COUNTING API
// =============================================================================

/** Allocate reference counted object */
void *axiom_ref_alloc(size_t size, axiom_memory_type_t type, void (*destructor)(void *));

/** Increment reference count */
void *axiom_ref_retain(void *ptr);

/** Decrement reference count, free if zero */
void axiom_ref_release(void *ptr);

/** Get current reference count */
uint32_t axiom_ref_count(void *ptr);

// =============================================================================
// SAFE ALLOCATION WRAPPERS
// =============================================================================

/** Safe allocation with automatic error handling */
#define AXIOM_SAFE_ALLOC(type, size, mem_type) \
    axiom_safe_alloc_impl(sizeof(type), size, mem_type, __FILE__, __func__, __LINE__)

/** Safe calloc with automatic error handling */
#define AXIOM_SAFE_CALLOC(type, count, mem_type) \
    axiom_safe_calloc_impl(sizeof(type), count, mem_type, __FILE__, __func__, __LINE__)

/** Implementation functions for safe allocation */
void *axiom_safe_alloc_impl(size_t type_size, size_t size, axiom_memory_type_t mem_type,
                           const char *file, const char *func, int line);
void *axiom_safe_calloc_impl(size_t type_size, size_t count, axiom_memory_type_t mem_type,
                            const char *file, const char *func, int line);

// =============================================================================
// MEMORY POOL SYSTEM
// =============================================================================

/** Memory pool for efficient small allocations */
struct axiom_memory_pool;

/** Create memory pool for specific object size */
struct axiom_memory_pool *axiom_memory_pool_create(size_t object_size, size_t initial_count);

/** Destroy memory pool and free all objects */
void axiom_memory_pool_destroy(struct axiom_memory_pool *pool);

/** Allocate object from pool */
void *axiom_memory_pool_alloc(struct axiom_memory_pool *pool);

/** Return object to pool */
void axiom_memory_pool_free(struct axiom_memory_pool *pool, void *ptr);

/** Get pool statistics */
void axiom_memory_pool_stats(struct axiom_memory_pool *pool, 
                            size_t *total_objects, size_t *free_objects, size_t *used_objects);

// =============================================================================
// CONVENIENCE MACROS
// =============================================================================

#ifdef AXIOM_MEMORY_DEBUG

/** Debug-enabled allocation macros */
#define axiom_malloc(size, type) \
    axiom_malloc_tracked(size, type, __FILE__, __func__, __LINE__)

#define axiom_calloc(count, size, type) \
    axiom_calloc_tracked(count, size, type, __FILE__, __func__, __LINE__)

#define axiom_realloc(ptr, size, type) \
    axiom_realloc_tracked(ptr, size, type, __FILE__, __func__, __LINE__)

#define axiom_free(ptr) \
    axiom_free_tracked(ptr, __FILE__, __func__, __LINE__)

#define axiom_strdup(str) \
    axiom_strdup_tracked(str, __FILE__, __func__, __LINE__)

#else

/** Release-mode allocation macros (fall back to standard functions with error handling) */
#define axiom_malloc(size, type) \
    axiom_malloc_safe(size, NULL)

#define axiom_calloc(count, size, type) \
    axiom_calloc_safe(count, size, NULL)

#define axiom_realloc(ptr, size, type) \
    axiom_realloc_safe(ptr, size, NULL)

#define axiom_free(ptr) \
    axiom_free_safe(ptr)

#define axiom_strdup(str) \
    strdup(str)

#endif /* AXIOM_MEMORY_DEBUG */

// =============================================================================
// CLEANUP HELPERS
// =============================================================================

/** Cleanup function type */
typedef void (*axiom_cleanup_func_t)(void *data);

/** Cleanup manager for automatic resource cleanup */
struct axiom_cleanup_manager;

/** Create cleanup manager */
struct axiom_cleanup_manager *axiom_cleanup_manager_create(void);

/** Destroy cleanup manager and run all cleanups */
void axiom_cleanup_manager_destroy(struct axiom_cleanup_manager *manager);

/** Register cleanup function */
axiom_result_t axiom_cleanup_register(struct axiom_cleanup_manager *manager,
                                     void *data, axiom_cleanup_func_t cleanup_func);

/** Run all cleanups immediately */
void axiom_cleanup_run_all(struct axiom_cleanup_manager *manager);

/** Cleanup manager macro for automatic scope cleanup */
#define AXIOM_CLEANUP_SCOPE(manager) \
    __attribute__((cleanup(axiom_cleanup_manager_destroy_wrapper))) struct axiom_cleanup_manager *manager = axiom_cleanup_manager_create()

/** Wrapper for cleanup attribute */
void axiom_cleanup_manager_destroy_wrapper(struct axiom_cleanup_manager **manager);

// =============================================================================
// MEMORY GUARD SYSTEM
// =============================================================================

/** Memory guard for detecting buffer overruns */
struct axiom_memory_guard;

/** Enable memory guards (adds overhead but detects corruption) */
void axiom_memory_guards_enable(bool enable);

/** Check all memory guards for corruption */
bool axiom_memory_guards_check_all(void);

/** Check specific memory guard */
bool axiom_memory_guard_check(void *ptr);

#endif /* AXIOM_MEMORY_H */
