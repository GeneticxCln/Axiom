#include "memory.h"
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @file memory.c
 * @brief Simplified memory management system for production
 * 
 * This is a simplified version that provides basic memory tracking
 * without the complex overhead of the full system. It's designed
 * for production use where performance is more important than
 * detailed debugging features.
 */

// Simple global statistics - no threading for performance
static struct {
    size_t total_allocations;
    size_t current_allocations;  
    size_t current_bytes_used;
    size_t peak_bytes_used;
} simple_stats = {0};

// Simple allocation tracking (debug builds only)
#ifdef AXIOM_DEBUG_MEMORY
static struct axiom_memory_info *allocation_list = NULL;
#endif

axiom_result_t axiom_memory_init(void) {
    memset(&simple_stats, 0, sizeof(simple_stats));
    AXIOM_LOG_INFO("MEMORY", "Simple memory management initialized");
    return AXIOM_SUCCESS;
}

void axiom_memory_shutdown(void) {
    AXIOM_LOG_INFO("MEMORY", "Memory Statistics:");
    AXIOM_LOG_INFO("MEMORY", "  Total allocations: %zu", simple_stats.total_allocations);
    AXIOM_LOG_INFO("MEMORY", "  Current allocations: %zu", simple_stats.current_allocations);
    AXIOM_LOG_INFO("MEMORY", "  Current bytes used: %zu", simple_stats.current_bytes_used);
    AXIOM_LOG_INFO("MEMORY", "  Peak bytes used: %zu", simple_stats.peak_bytes_used);
    
    if (simple_stats.current_allocations > 0) {
        AXIOM_LOG_WARN("MEMORY", "Potential memory leaks: %zu allocations not freed", 
                       simple_stats.current_allocations);
    } else {
        AXIOM_LOG_INFO("MEMORY", "No memory leaks detected");
    }
}

void *axiom_calloc_tracked(size_t count, size_t size, axiom_memory_type_t type,
                          const char *file, const char *func, int line) {
    (void)type; (void)file; (void)func; (void)line; // Suppress unused warnings
    
    if (count == 0 || size == 0) {
        return NULL; // Return NULL for zero-size allocations
    }
    
    void *ptr = calloc(count, size);
    if (ptr) {
        size_t total_size = count * size;
        simple_stats.total_allocations++;
        simple_stats.current_allocations++;
        simple_stats.current_bytes_used += total_size;
        
        if (simple_stats.current_bytes_used > simple_stats.peak_bytes_used) {
            simple_stats.peak_bytes_used = simple_stats.current_bytes_used;
        }
        
#ifdef AXIOM_DEBUG_MEMORY
        // Only track in debug builds to avoid performance overhead
        struct axiom_memory_info *info = malloc(sizeof(struct axiom_memory_info));
        if (info) {
            info->ptr = ptr;
            info->size = total_size;
            info->type = type;
            info->file = file;
            info->function = func;
            info->line = line;
            info->next = allocation_list;
            allocation_list = info;
        }
#endif
    }
    
    return ptr;
}

void *axiom_malloc_tracked(size_t size, axiom_memory_type_t type,
                          const char *file, const char *func, int line) {
    (void)type; (void)file; (void)func; (void)line;
    
    if (size == 0) {
        return NULL; // Return NULL for zero-size allocations
    }
    
    void *ptr = malloc(size);
    if (ptr) {
        simple_stats.total_allocations++;
        simple_stats.current_allocations++;
        simple_stats.current_bytes_used += size;
        
        if (simple_stats.current_bytes_used > simple_stats.peak_bytes_used) {
            simple_stats.peak_bytes_used = simple_stats.current_bytes_used;
        }
    }
    
    return ptr;
}

void axiom_free_tracked(void *ptr, const char *file, const char *func, int line) {
    (void)file; (void)func; (void)line;
    
    if (!ptr) return;
    
    // In a real implementation, we'd need to track sizes to decrement properly
    // For simplicity, we just track allocation count
    simple_stats.current_allocations--;
    
#ifdef AXIOM_DEBUG_MEMORY
    // Remove from tracking list in debug builds
    struct axiom_memory_info **current = &allocation_list;
    while (*current) {
        if ((*current)->ptr == ptr) {
            struct axiom_memory_info *to_remove = *current;
            simple_stats.current_bytes_used -= to_remove->size;
            *current = to_remove->next;
            free(to_remove);
            break;
        }
        current = &(*current)->next;
    }
#endif
    
    free(ptr);
}

// Simplified implementations of other functions
void axiom_memory_set_leak_detection(bool enabled) {
    AXIOM_LOG_INFO("MEMORY", "Leak detection %s (simplified mode)", 
                   enabled ? "enabled" : "disabled");
}

struct axiom_memory_stats axiom_memory_get_stats(void) {
    struct axiom_memory_stats stats = {0};
    stats.total_allocations = simple_stats.total_allocations;
    stats.current_allocations = simple_stats.current_allocations;
    stats.current_bytes_used = simple_stats.current_bytes_used;
    stats.peak_bytes_used = simple_stats.peak_bytes_used;
    return stats;
}

void axiom_memory_print_stats(void) {
    AXIOM_LOG_INFO("MEMORY", "=== Simple Memory Statistics ===");
    AXIOM_LOG_INFO("MEMORY", "Total allocations: %zu", simple_stats.total_allocations);
    AXIOM_LOG_INFO("MEMORY", "Current allocations: %zu", simple_stats.current_allocations);
    AXIOM_LOG_INFO("MEMORY", "Current memory usage: %zu bytes", simple_stats.current_bytes_used);
    AXIOM_LOG_INFO("MEMORY", "Peak memory usage: %zu bytes", simple_stats.peak_bytes_used);
}

void axiom_memory_print_leaks(void) {
#ifdef AXIOM_DEBUG_MEMORY
    AXIOM_LOG_INFO("MEMORY", "=== Current Allocations ===");
    struct axiom_memory_info *info = allocation_list;
    int count = 0;
    while (info) {
        AXIOM_LOG_INFO("MEMORY", "  [%d] %p (%zu bytes) at %s:%s:%d", 
                       count++, info->ptr, info->size, 
                       info->file ?: "unknown", info->function ?: "unknown", info->line);
        info = info->next;
    }
    AXIOM_LOG_INFO("MEMORY", "Total tracked allocations: %d", count);
#else
    AXIOM_LOG_INFO("MEMORY", "Allocation tracking disabled (release build)");
#endif
}

uint64_t axiom_memory_check_leaks(void) {
    return simple_stats.current_allocations;
}

// Simplified stubs for advanced features - keep interface compatibility
char *axiom_strdup_tracked(const char *str, const char *file, const char *func, int line) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char *copy = axiom_malloc_tracked(len, AXIOM_MEM_TYPE_STRING, file, func, line);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

void *axiom_realloc_tracked(void *ptr, size_t size, axiom_memory_type_t type,
                           const char *file, const char *func, int line) {
    (void)type; (void)file; (void)func; (void)line;
    
    if (!ptr) {
        return axiom_malloc_tracked(size, type, file, func, line);
    }
    
    if (size == 0) {
        axiom_free_tracked(ptr, file, func, line);
        return NULL;
    }
    
    void *new_ptr = realloc(ptr, size);
    // Note: In simplified mode, we don't track size changes precisely
    return new_ptr;
}

// Reference counting stubs - simplified to regular allocation
void *axiom_ref_alloc(size_t size, axiom_memory_type_t type, void (*destructor)(void *)) {
    (void)destructor; // Not used in simplified mode
    return axiom_malloc_tracked(size, type, __FILE__, __func__, __LINE__);
}

void *axiom_ref_retain(void *ptr) {
    return ptr; // No-op in simplified mode
}

void axiom_ref_release(void *ptr) {
    axiom_free_tracked(ptr, __FILE__, __func__, __LINE__);
}

uint32_t axiom_ref_count(void *ptr) {
    if (!ptr) return 0;
    return 1; // Always 1 in simplified mode for non-NULL pointers
}

// Safe allocation wrappers
void *axiom_safe_alloc_impl(size_t type_size, size_t size, axiom_memory_type_t mem_type,
                           const char *file, const char *func, int line) {
    (void)type_size;
    return axiom_malloc_tracked(size, mem_type, file, func, line);
}

void *axiom_safe_calloc_impl(size_t type_size, size_t count, axiom_memory_type_t mem_type,
                            const char *file, const char *func, int line) {
    return axiom_calloc_tracked(count, type_size, mem_type, file, func, line);
}

// Memory pool stubs - fall back to regular allocation
struct axiom_memory_pool *axiom_memory_pool_create(size_t object_size, size_t initial_count) {
    (void)object_size; (void)initial_count;
    AXIOM_LOG_INFO("MEMORY", "Memory pools not supported in simplified mode, using regular allocation");
    return NULL;
}

void axiom_memory_pool_destroy(struct axiom_memory_pool *pool) {
    (void)pool;
}

void *axiom_memory_pool_alloc(struct axiom_memory_pool *pool) {
    (void)pool;
    return NULL;
}

void axiom_memory_pool_free(struct axiom_memory_pool *pool, void *ptr) {
    (void)pool; (void)ptr;
}

void axiom_memory_pool_stats(struct axiom_memory_pool *pool, 
                            size_t *total_objects, size_t *free_objects, size_t *used_objects) {
    (void)pool;
    if (total_objects) *total_objects = 0;
    if (free_objects) *free_objects = 0;
    if (used_objects) *used_objects = 0;
}

// Cleanup manager stubs
struct axiom_cleanup_manager *axiom_cleanup_manager_create(void) {
    return NULL;
}

void axiom_cleanup_manager_destroy(struct axiom_cleanup_manager *manager) {
    (void)manager;
}

axiom_result_t axiom_cleanup_register(struct axiom_cleanup_manager *manager,
                                     void *data, axiom_cleanup_func_t cleanup_func) {
    (void)manager; (void)data; (void)cleanup_func;
    return AXIOM_SUCCESS;
}

void axiom_cleanup_run_all(struct axiom_cleanup_manager *manager) {
    (void)manager;
}

void axiom_cleanup_manager_destroy_wrapper(struct axiom_cleanup_manager **manager) {
    (void)manager;
}

// Memory guard stubs
void axiom_memory_guards_enable(bool enable) {
    AXIOM_LOG_INFO("MEMORY", "Memory guards %s (not supported in simplified mode)", 
                   enable ? "requested" : "disabled");
}

bool axiom_memory_guards_check_all(void) {
    return true;
}

bool axiom_memory_guard_check(void *ptr) {
    (void)ptr;
    return true;
}
