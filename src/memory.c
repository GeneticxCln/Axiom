#include "memory.h"
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>

/**
 * @file memory.c
 * @brief Implementation of advanced memory management system
 * 
 * This implements comprehensive memory management with:
 * - Memory leak detection and tracking
 * - Reference counting for shared resources
 * - Memory pools for efficient allocation
 * - Automatic cleanup management
 * - Memory guards for corruption detection
 */

// =============================================================================
// GLOBAL STATE AND SYNCHRONIZATION
// =============================================================================

/** Global memory management state */
static struct {
    bool initialized;
    bool leak_detection_enabled;
    bool guards_enabled;
    pthread_mutex_t mutex;
    struct axiom_memory_stats stats;
    struct axiom_memory_info *allocations;
} g_memory_state = {
    .initialized = false,
    .leak_detection_enabled = false,
    .guards_enabled = false,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .stats = {0},
    .allocations = NULL
};

/** Memory type names for logging */
static const char *memory_type_names[AXIOM_MEM_TYPE_COUNT] = {
    "General", "Window", "Config", "Surface", "Scene", 
    "Buffer", "String", "Effects", "Animation"
};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/** Get current timestamp in microseconds */
static uint64_t get_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

/** Thread-safe, lock-free logging wrapper for memory events */
static void log_memory_event(const char *event, void *ptr, size_t size, 
                             axiom_memory_type_t type, const char *file, 
                             const char *func, int line) {
    // Use lock-free, allocation-free logging to prevent recursive deadlock
    // This uses stack allocation and direct system calls to avoid malloc
    
    char buffer[512];  // Stack-allocated buffer
    int len = snprintf(buffer, sizeof(buffer), 
                      "[MEMORY] %s: ptr=%p, size=%zu, type=%s at %s:%s:%d\n",
                      event, ptr, size, 
                      type < AXIOM_MEM_TYPE_COUNT ? memory_type_names[type] : "unknown",
                      file ? file : "unknown", 
                      func ? func : "unknown", 
                      line);
    
    if (len > 0 && len < (int)sizeof(buffer)) {
        // Use write() system call directly to avoid stdio buffering/allocation
        // STDERR_FILENO is typically 2
        write(STDERR_FILENO, buffer, (size_t)len);
    }
}

// =============================================================================
// MEMORY TRACKING IMPLEMENTATION
// =============================================================================

axiom_result_t axiom_memory_init(void) {
    pthread_mutex_lock(&g_memory_state.mutex);
    
    if (g_memory_state.initialized) {
        pthread_mutex_unlock(&g_memory_state.mutex);
        return AXIOM_SUCCESS;
    }
    
    // Initialize statistics
    memset(&g_memory_state.stats, 0, sizeof(g_memory_state.stats));
    g_memory_state.allocations = NULL;
    g_memory_state.leak_detection_enabled = true;
    g_memory_state.guards_enabled = false;
    g_memory_state.initialized = true;
    
    pthread_mutex_unlock(&g_memory_state.mutex);
    
    // Use direct write to avoid potential deadlock with logging system during init
    const char *msg = "[MEMORY] Memory management system initialized\n";
    write(STDERR_FILENO, msg, strlen(msg));
    return AXIOM_SUCCESS;
}

void axiom_memory_shutdown(void) {
    pthread_mutex_lock(&g_memory_state.mutex);
    
    if (!g_memory_state.initialized) {
        pthread_mutex_unlock(&g_memory_state.mutex);
        return;
    }
    
    // Get statistics before cleanup to avoid deadlock in axiom_memory_get_stats
    struct axiom_memory_stats stats = g_memory_state.stats;
    
    // Check for leaks - calculate while we have the lock
    uint64_t leak_count = 0;
    struct axiom_memory_info *info = g_memory_state.allocations;
    while (info) {
        leak_count++;
        info = info->next;
    }
    
    // Free leak tracking data
    info = g_memory_state.allocations;
    while (info) {
        struct axiom_memory_info *next = info->next;
        free(info);
        info = next;
    }
    
    g_memory_state.initialized = false;
    pthread_mutex_unlock(&g_memory_state.mutex);
    
    // Use stack buffer and write() to avoid potential deadlock during shutdown
    char buffer[1024];
    int len = snprintf(buffer, sizeof(buffer),
        "[MEMORY] === Memory Statistics ===\n"
        "[MEMORY] Total allocations: %lu\n"
        "[MEMORY] Total deallocations: %lu\n"
        "[MEMORY] Current allocations: %lu\n"
        "[MEMORY] Peak allocations: %lu\n"
        "[MEMORY] Total bytes allocated: %lu\n"
        "[MEMORY] Total bytes freed: %lu\n"
        "[MEMORY] Current bytes used: %lu\n"
        "[MEMORY] Peak bytes used: %lu\n",
        (unsigned long)stats.total_allocations,
        (unsigned long)stats.total_deallocations,
        (unsigned long)stats.current_allocations,
        (unsigned long)stats.peak_allocations,
        (unsigned long)stats.total_bytes_allocated,
        (unsigned long)stats.total_bytes_freed,
        (unsigned long)stats.current_bytes_used,
        (unsigned long)stats.peak_bytes_used);
    
    if (len > 0 && len < (int)sizeof(buffer)) {
        write(STDERR_FILENO, buffer, (size_t)len);
    }
    
    // Print allocations by type
    for (int i = 0; i < AXIOM_MEM_TYPE_COUNT; i++) {
        if (stats.allocation_count_by_type[i] > 0) {
            len = snprintf(buffer, sizeof(buffer),
                "[MEMORY]   %s: %lu allocations (%lu bytes)\n",
                memory_type_names[i], 
                (unsigned long)stats.allocation_count_by_type[i],
                (unsigned long)stats.bytes_by_type[i]);
            if (len > 0 && len < (int)sizeof(buffer)) {
                write(STDERR_FILENO, buffer, (size_t)len);
            }
        }
    }
    
    // Print leak status
    if (leak_count > 0) {
        len = snprintf(buffer, sizeof(buffer), "Memory leaks detected: %lu allocations\n", (unsigned long)leak_count);
    } else {
        len = snprintf(buffer, sizeof(buffer), "No memory leaks detected\n");
    }
    if (len > 0 && len < (int)sizeof(buffer)) {
        write(STDERR_FILENO, buffer, (size_t)len);
    }
    
    const char *shutdown_msg = "Memory management system shut down\n";
    write(STDERR_FILENO, shutdown_msg, strlen(shutdown_msg));
}

void axiom_memory_set_leak_detection(bool enabled) {
    pthread_mutex_lock(&g_memory_state.mutex);
    g_memory_state.leak_detection_enabled = enabled;
    pthread_mutex_unlock(&g_memory_state.mutex);
    
    AXIOM_LOG_INFO("MEMORY", "Leak detection %s", enabled ? "enabled" : "disabled");
}

struct axiom_memory_stats axiom_memory_get_stats(void) {
    pthread_mutex_lock(&g_memory_state.mutex);
    struct axiom_memory_stats stats = g_memory_state.stats;
    pthread_mutex_unlock(&g_memory_state.mutex);
    return stats;
}

void axiom_memory_print_stats(void) {
    struct axiom_memory_stats stats = axiom_memory_get_stats();
    
    // Use logging system here since this is not during init/shutdown
    AXIOM_LOG_INFO("MEMORY", "=== Memory Statistics ===");
    AXIOM_LOG_INFO("MEMORY", "Total allocations: %lu", (unsigned long)stats.total_allocations);
    AXIOM_LOG_INFO("MEMORY", "Total deallocations: %lu", (unsigned long)stats.total_deallocations);
    AXIOM_LOG_INFO("MEMORY", "Current allocations: %lu", (unsigned long)stats.current_allocations);
    AXIOM_LOG_INFO("MEMORY", "Peak allocations: %lu", (unsigned long)stats.peak_allocations);
    AXIOM_LOG_INFO("MEMORY", "Total bytes allocated: %lu", (unsigned long)stats.total_bytes_allocated);
    AXIOM_LOG_INFO("MEMORY", "Total bytes freed: %lu", (unsigned long)stats.total_bytes_freed);
    AXIOM_LOG_INFO("MEMORY", "Current bytes used: %lu", (unsigned long)stats.current_bytes_used);
    AXIOM_LOG_INFO("MEMORY", "Peak bytes used: %lu", (unsigned long)stats.peak_bytes_used);
    
    AXIOM_LOG_INFO("MEMORY", "Allocations by type:");
    for (int i = 0; i < AXIOM_MEM_TYPE_COUNT; i++) {
        if (stats.allocation_count_by_type[i] > 0) {
            AXIOM_LOG_INFO("MEMORY", "  %s: %lu allocations (%lu bytes)",
                          memory_type_names[i], 
                          (unsigned long)stats.allocation_count_by_type[i],
                          (unsigned long)stats.bytes_by_type[i]);
        }
    }
}

void axiom_memory_print_leaks(void) {
    pthread_mutex_lock(&g_memory_state.mutex);
    
    AXIOM_LOG_INFO("MEMORY", "=== Memory Leaks ===");
    struct axiom_memory_info *info = g_memory_state.allocations;
    uint64_t leak_count = 0;
    uint64_t leak_bytes = 0;
    
    while (info) {
        AXIOM_LOG_WARN("MEMORY", "LEAK: ptr=%p, size=%zu, type=%s, allocated at %s:%s:%d",
                      info->ptr, info->size, memory_type_names[info->type],
                      info->file ? info->file : "unknown",
                      info->function ? info->function : "unknown",
                      info->line);
        leak_count++;
        leak_bytes += info->size;
        info = info->next;
    }
    
    if (leak_count > 0) {
        AXIOM_LOG_ERROR("MEMORY", "Total leaks: %lu allocations, %lu bytes", leak_count, leak_bytes);
    }
    
    pthread_mutex_unlock(&g_memory_state.mutex);
}

uint64_t axiom_memory_check_leaks(void) {
    pthread_mutex_lock(&g_memory_state.mutex);
    
    uint64_t leak_count = 0;
    struct axiom_memory_info *info = g_memory_state.allocations;
    while (info) {
        leak_count++;
        info = info->next;
    }
    
    pthread_mutex_unlock(&g_memory_state.mutex);
    return leak_count;
}

// =============================================================================
// TRACKED ALLOCATION IMPLEMENTATION
// =============================================================================

/** Add allocation to tracking list */
static void track_allocation(void *ptr, size_t size, axiom_memory_type_t type,
                           const char *file, const char *func, int line) {
    if (!g_memory_state.leak_detection_enabled || !ptr) return;
    
    struct axiom_memory_info *info = malloc(sizeof(struct axiom_memory_info));
    if (!info) return; // Can't track this allocation
    
    info->ptr = ptr;
    info->size = size;
    info->type = type;
    info->file = file;
    info->function = func;
    info->line = line;
    info->timestamp = get_timestamp_us();
    info->next = g_memory_state.allocations;
    g_memory_state.allocations = info;
    
    // Update statistics
    g_memory_state.stats.total_allocations++;
    g_memory_state.stats.current_allocations++;
    g_memory_state.stats.total_bytes_allocated += size;
    g_memory_state.stats.current_bytes_used += size;
    g_memory_state.stats.allocation_count_by_type[type]++;
    g_memory_state.stats.bytes_by_type[type] += size;
    
    if (g_memory_state.stats.current_allocations > g_memory_state.stats.peak_allocations) {
        g_memory_state.stats.peak_allocations = g_memory_state.stats.current_allocations;
    }
    if (g_memory_state.stats.current_bytes_used > g_memory_state.stats.peak_bytes_used) {
        g_memory_state.stats.peak_bytes_used = g_memory_state.stats.current_bytes_used;
    }
}

/** Remove allocation from tracking list */
static void untrack_allocation(void *ptr) {
    if (!g_memory_state.leak_detection_enabled || !ptr) return;
    
    struct axiom_memory_info **current = &g_memory_state.allocations;
    while (*current) {
        if ((*current)->ptr == ptr) {
            struct axiom_memory_info *to_remove = *current;
            *current = (*current)->next;
            
            // Update statistics
            g_memory_state.stats.total_deallocations++;
            g_memory_state.stats.current_allocations--;
            g_memory_state.stats.total_bytes_freed += to_remove->size;
            g_memory_state.stats.current_bytes_used -= to_remove->size;
            g_memory_state.stats.bytes_by_type[to_remove->type] -= to_remove->size;
            
            free(to_remove);
            return;
        }
        current = &(*current)->next;
    }
}

void *axiom_malloc_tracked(size_t size, axiom_memory_type_t type, 
                          const char *file, const char *func, int line) {
    if (size == 0) {
        AXIOM_LOG_WARN("MEMORY", "Zero-size allocation at %s:%s:%d", 
                       file ? file : "unknown", func ? func : "unknown", line);
        return NULL;
    }
    
    pthread_mutex_lock(&g_memory_state.mutex);
    
    void *ptr = malloc(size);
    if (ptr) {
        track_allocation(ptr, size, type, file, func, line);
        log_memory_event("ALLOC", ptr, size, type, file, func, line);
    } else {
        AXIOM_LOG_ERROR("MEMORY", "malloc failed for %zu bytes at %s:%s:%d",
                       size, file ? file : "unknown", func ? func : "unknown", line);
    }
    
    pthread_mutex_unlock(&g_memory_state.mutex);
    return ptr;
}

void *axiom_calloc_tracked(size_t count, size_t size, axiom_memory_type_t type,
                          const char *file, const char *func, int line) {
    if (count == 0 || size == 0) {
        AXIOM_LOG_WARN("MEMORY", "Zero-size calloc at %s:%s:%d", 
                       file ? file : "unknown", func ? func : "unknown", line);
        return NULL;
    }
    
    pthread_mutex_lock(&g_memory_state.mutex);
    
    void *ptr = calloc(count, size);
    if (ptr) {
        size_t total_size = count * size;
        track_allocation(ptr, total_size, type, file, func, line);
        log_memory_event("CALLOC", ptr, total_size, type, file, func, line);
    } else {
        AXIOM_LOG_ERROR("MEMORY", "calloc failed for %zu * %zu bytes at %s:%s:%d",
                       count, size, file ? file : "unknown", func ? func : "unknown", line);
    }
    
    pthread_mutex_unlock(&g_memory_state.mutex);
    return ptr;
}

void *axiom_realloc_tracked(void *ptr, size_t size, axiom_memory_type_t type,
                           const char *file, const char *func, int line) {
    pthread_mutex_lock(&g_memory_state.mutex);
    
    // Handle realloc(NULL, size) as malloc
    if (!ptr) {
        pthread_mutex_unlock(&g_memory_state.mutex);
        return axiom_malloc_tracked(size, type, file, func, line);
    }
    
    // Handle realloc(ptr, 0) as free
    if (size == 0) {
        untrack_allocation(ptr);
        free(ptr);
        pthread_mutex_unlock(&g_memory_state.mutex);
        return NULL;
    }
    
    void *new_ptr = realloc(ptr, size);
    if (new_ptr) {
        if (new_ptr != ptr) {
            // Pointer changed, update tracking
            // Note: original ptr is already freed by realloc at this point
            // We need to find and remove it from tracking without using ptr value
            struct axiom_memory_info **current = &g_memory_state.allocations;
            while (*current) {
                if ((*current)->ptr == ptr) {
                    struct axiom_memory_info *to_remove = *current;
                    *current = (*current)->next;
                    
                    // Update statistics
                    g_memory_state.stats.total_deallocations++;
                    g_memory_state.stats.current_allocations--;
                    g_memory_state.stats.total_bytes_freed += to_remove->size;
                    g_memory_state.stats.current_bytes_used -= to_remove->size;
                    g_memory_state.stats.bytes_by_type[to_remove->type] -= to_remove->size;
                    
                    free(to_remove);
                    break;
                }
                current = &(*current)->next;
            }
            track_allocation(new_ptr, size, type, file, func, line);
        }
        log_memory_event("REALLOC", new_ptr, size, type, file, func, line);
    } else {
        AXIOM_LOG_ERROR("MEMORY", "realloc failed for %zu bytes at %s:%s:%d",
                       size, file ? file : "unknown", func ? func : "unknown", line);
    }
    
    pthread_mutex_unlock(&g_memory_state.mutex);
    return new_ptr;
}

void axiom_free_tracked(void *ptr, const char *file, const char *func, int line) {
    if (!ptr) return;
    
    pthread_mutex_lock(&g_memory_state.mutex);
    
    // Log before freeing to avoid use-after-free warnings
    log_memory_event("FREE", ptr, 0, AXIOM_MEM_TYPE_GENERAL, file, func, line);
    untrack_allocation(ptr);
    free(ptr);
    
    pthread_mutex_unlock(&g_memory_state.mutex);
}

char *axiom_strdup_tracked(const char *str, const char *file, const char *func, int line) {
    if (!str) return NULL;
    
    size_t len = strlen(str) + 1;
    char *copy = axiom_malloc_tracked(len, AXIOM_MEM_TYPE_STRING, file, func, line);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

// =============================================================================
// REFERENCE COUNTING IMPLEMENTATION
// =============================================================================

void *axiom_ref_alloc(size_t size, axiom_memory_type_t type, void (*destructor)(void *)) {
    size_t total_size = sizeof(struct axiom_ref_count) + size;
    struct axiom_ref_count *header = axiom_malloc_tracked(total_size, type, __FILE__, __func__, __LINE__);
    if (!header) return NULL;
    
    header->count = 1;
    header->destructor = destructor;
    header->type = type;
    
    void *ptr = (char *)header + sizeof(struct axiom_ref_count);
    memset(ptr, 0, size);
    
    AXIOM_LOG_DEBUG("MEMORY", "REF_ALLOC: ptr=%p, size=%zu, type=%s", 
                    ptr, size, memory_type_names[type]);
    
    return ptr;
}

void *axiom_ref_retain(void *ptr) {
    if (!ptr) return NULL;
    
    struct axiom_ref_count *header = AXIOM_REF_HEADER(ptr);
    __sync_fetch_and_add(&header->count, 1);
    
    AXIOM_LOG_DEBUG("MEMORY", "REF_RETAIN: ptr=%p, count=%u", ptr, header->count);
    
    return ptr;
}

void axiom_ref_release(void *ptr) {
    if (!ptr) return;
    
    struct axiom_ref_count *header = AXIOM_REF_HEADER(ptr);
    uint32_t new_count = __sync_sub_and_fetch(&header->count, 1);
    
    AXIOM_LOG_DEBUG("MEMORY", "REF_RELEASE: ptr=%p, count=%u", ptr, new_count);
    
    if (new_count == 0) {
        if (header->destructor) {
            header->destructor(ptr);
        }
        axiom_free_tracked(header, __FILE__, __func__, __LINE__);
    }
}

uint32_t axiom_ref_count(void *ptr) {
    if (!ptr) return 0;
    
    struct axiom_ref_count *header = AXIOM_REF_HEADER(ptr);
    return header->count;
}

// =============================================================================
// SAFE ALLOCATION WRAPPERS
// =============================================================================

void *axiom_safe_alloc_impl(size_t type_size __attribute__((unused)), size_t size, axiom_memory_type_t mem_type,
                           const char *file, const char *func, int line) {
    if (size == 0) {
        AXIOM_LOG_ERROR("MEMORY", "Zero-size allocation at %s:%s:%d", file, func, line);
        return NULL;
    }
    
    void *ptr = axiom_malloc_tracked(size, mem_type, file, func, line);
    if (!ptr) {
        AXIOM_LOG_ERROR("MEMORY", "Allocation failed for %zu bytes at %s:%s:%d", 
                       size, file, func, line);
    }
    
    return ptr;
}

void *axiom_safe_calloc_impl(size_t type_size, size_t count, axiom_memory_type_t mem_type,
                            const char *file, const char *func, int line) {
    if (count == 0) {
        AXIOM_LOG_ERROR("MEMORY", "Zero-count allocation at %s:%s:%d", file, func, line);
        return NULL;
    }
    
    void *ptr = axiom_calloc_tracked(count, type_size, mem_type, file, func, line);
    if (!ptr) {
        AXIOM_LOG_ERROR("MEMORY", "Allocation failed for %zu * %zu bytes at %s:%s:%d", 
                       count, type_size, file, func, line);
    }
    
    return ptr;
}

// =============================================================================
// MEMORY POOL IMPLEMENTATION
// =============================================================================

/** Memory pool structure */
struct axiom_memory_pool {
    size_t object_size;
    size_t total_objects;
    size_t free_objects;
    void **free_list;
    char *memory_block;
    pthread_mutex_t mutex;
};

struct axiom_memory_pool *axiom_memory_pool_create(size_t object_size, size_t initial_count) {
    if (object_size == 0 || initial_count == 0) return NULL;
    
    struct axiom_memory_pool *pool = axiom_malloc_tracked(sizeof(struct axiom_memory_pool), 
                                                         AXIOM_MEM_TYPE_GENERAL, 
                                                         __FILE__, __func__, __LINE__);
    if (!pool) return NULL;
    
    // Ensure object size is at least pointer size for free list
    if (object_size < sizeof(void*)) {
        object_size = sizeof(void*);
    }
    
    pool->object_size = object_size;
    pool->total_objects = initial_count;
    pool->free_objects = initial_count;
    pthread_mutex_init(&pool->mutex, NULL);
    
    // Allocate memory block
    pool->memory_block = axiom_malloc_tracked(object_size * initial_count, 
                                            AXIOM_MEM_TYPE_BUFFER,
                                            __FILE__, __func__, __LINE__);
    if (!pool->memory_block) {
        axiom_free_tracked(pool, __FILE__, __func__, __LINE__);
        return NULL;
    }
    
    // Initialize free list
    pool->free_list = axiom_malloc_tracked(sizeof(void*) * initial_count,
                                         AXIOM_MEM_TYPE_BUFFER, 
                                         __FILE__, __func__, __LINE__);
    if (!pool->free_list) {
        axiom_free_tracked(pool->memory_block, __FILE__, __func__, __LINE__);
        axiom_free_tracked(pool, __FILE__, __func__, __LINE__);
        return NULL;
    }
    
    // Link all objects into free list
    for (size_t i = 0; i < initial_count; i++) {
        pool->free_list[i] = pool->memory_block + (i * object_size);
    }
    
    AXIOM_LOG_INFO("MEMORY", "Created memory pool: object_size=%zu, initial_count=%zu", 
                   object_size, initial_count);
    
    return pool;
}

void axiom_memory_pool_destroy(struct axiom_memory_pool *pool) {
    if (!pool) return;
    
    pthread_mutex_lock(&pool->mutex);
    
    if (pool->free_objects != pool->total_objects) {
        AXIOM_LOG_WARN("MEMORY", "Destroying pool with %zu objects still in use", 
                       pool->total_objects - pool->free_objects);
    }
    
    axiom_free_tracked(pool->memory_block, __FILE__, __func__, __LINE__);
    axiom_free_tracked(pool->free_list, __FILE__, __func__, __LINE__);
    
    pthread_mutex_unlock(&pool->mutex);
    pthread_mutex_destroy(&pool->mutex);
    
    axiom_free_tracked(pool, __FILE__, __func__, __LINE__);
}

void *axiom_memory_pool_alloc(struct axiom_memory_pool *pool) {
    if (!pool) return NULL;
    
    pthread_mutex_lock(&pool->mutex);
    
    if (pool->free_objects == 0) {
        pthread_mutex_unlock(&pool->mutex);
        AXIOM_LOG_WARN("MEMORY", "Memory pool exhausted");
        return NULL;
    }
    
    void *ptr = pool->free_list[--pool->free_objects];
    memset(ptr, 0, pool->object_size);
    
    pthread_mutex_unlock(&pool->mutex);
    
    return ptr;
}

void axiom_memory_pool_free(struct axiom_memory_pool *pool, void *ptr) {
    if (!pool || !ptr) return;
    
    pthread_mutex_lock(&pool->mutex);
    
    // Verify pointer is within our memory block
    char *char_ptr = (char *)ptr;
    if (char_ptr < pool->memory_block || 
        char_ptr >= pool->memory_block + (pool->object_size * pool->total_objects)) {
        pthread_mutex_unlock(&pool->mutex);
        AXIOM_LOG_ERROR("MEMORY", "Attempting to free pointer not from this pool");
        return;
    }
    
    if (pool->free_objects >= pool->total_objects) {
        pthread_mutex_unlock(&pool->mutex);
        AXIOM_LOG_ERROR("MEMORY", "Double free detected in memory pool");
        return;
    }
    
    pool->free_list[pool->free_objects++] = ptr;
    
    pthread_mutex_unlock(&pool->mutex);
}

void axiom_memory_pool_stats(struct axiom_memory_pool *pool, 
                            size_t *total_objects, size_t *free_objects, size_t *used_objects) {
    if (!pool) return;
    
    pthread_mutex_lock(&pool->mutex);
    
    if (total_objects) *total_objects = pool->total_objects;
    if (free_objects) *free_objects = pool->free_objects;
    if (used_objects) *used_objects = pool->total_objects - pool->free_objects;
    
    pthread_mutex_unlock(&pool->mutex);
}

// =============================================================================
// CLEANUP MANAGER IMPLEMENTATION
// =============================================================================

/** Cleanup entry */
struct axiom_cleanup_entry {
    void *data;
    axiom_cleanup_func_t cleanup_func;
    struct axiom_cleanup_entry *next;
};

/** Cleanup manager */
struct axiom_cleanup_manager {
    struct axiom_cleanup_entry *entries;
    pthread_mutex_t mutex;
};

struct axiom_cleanup_manager *axiom_cleanup_manager_create(void) {
    struct axiom_cleanup_manager *manager = axiom_malloc_tracked(sizeof(struct axiom_cleanup_manager),
                                                               AXIOM_MEM_TYPE_GENERAL,
                                                               __FILE__, __func__, __LINE__);
    if (!manager) return NULL;
    
    manager->entries = NULL;
    pthread_mutex_init(&manager->mutex, NULL);
    
    return manager;
}

void axiom_cleanup_manager_destroy(struct axiom_cleanup_manager *manager) {
    if (!manager) return;
    
    axiom_cleanup_run_all(manager);
    
    pthread_mutex_destroy(&manager->mutex);
    axiom_free_tracked(manager, __FILE__, __func__, __LINE__);
}

axiom_result_t axiom_cleanup_register(struct axiom_cleanup_manager *manager,
                                     void *data, axiom_cleanup_func_t cleanup_func) {
    if (!manager || !cleanup_func) return AXIOM_ERROR_NULL_POINTER;
    
    struct axiom_cleanup_entry *entry = axiom_malloc_tracked(sizeof(struct axiom_cleanup_entry),
                                                           AXIOM_MEM_TYPE_GENERAL,
                                                           __FILE__, __func__, __LINE__);
    if (!entry) return AXIOM_ERROR_ALLOCATION_FAILED;
    
    entry->data = data;
    entry->cleanup_func = cleanup_func;
    
    pthread_mutex_lock(&manager->mutex);
    entry->next = manager->entries;
    manager->entries = entry;
    pthread_mutex_unlock(&manager->mutex);
    
    return AXIOM_SUCCESS;
}

void axiom_cleanup_run_all(struct axiom_cleanup_manager *manager) {
    if (!manager) return;
    
    pthread_mutex_lock(&manager->mutex);
    
    struct axiom_cleanup_entry *entry = manager->entries;
    while (entry) {
        struct axiom_cleanup_entry *next = entry->next;
        
        if (entry->cleanup_func) {
            entry->cleanup_func(entry->data);
        }
        
        axiom_free_tracked(entry, __FILE__, __func__, __LINE__);
        entry = next;
    }
    
    manager->entries = NULL;
    pthread_mutex_unlock(&manager->mutex);
}

void axiom_cleanup_manager_destroy_wrapper(struct axiom_cleanup_manager **manager) {
    if (manager && *manager) {
        axiom_cleanup_manager_destroy(*manager);
        *manager = NULL;
    }
}

// =============================================================================
// MEMORY GUARDS IMPLEMENTATION
// =============================================================================

void axiom_memory_guards_enable(bool enable) {
    pthread_mutex_lock(&g_memory_state.mutex);
    g_memory_state.guards_enabled = enable;
    pthread_mutex_unlock(&g_memory_state.mutex);
    
    AXIOM_LOG_INFO("MEMORY", "Memory guards %s", enable ? "enabled" : "disabled");
}

bool axiom_memory_guards_check_all(void) {
    // TODO: Implement memory guard checking
    // This would iterate through all tracked allocations and verify guard patterns
    return true;
}

bool axiom_memory_guard_check(void *ptr) {
    // TODO: Implement specific memory guard checking
    // This would check guard patterns around a specific allocation
    (void)ptr;
    return true;
}
