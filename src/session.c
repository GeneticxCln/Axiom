#include "session.h"
#include "axiom.h"
#include "logging.h"
#include "memory.h"

/**
 * Create and initialize the session manager.
 */
struct axiom_session_manager *axiom_session_manager_create(struct axiom_server *server) {
    struct axiom_session_manager *manager = axiom_calloc_tracked(1, 
        sizeof(struct axiom_session_manager), AXIOM_MEM_TYPE_SESSION,
        __FILE__, __func__, __LINE__);
    if (!manager) {
        AXIOM_LOG_ERROR("SESSION", "Failed to allocate session manager");
        return NULL;
    }
    
    manager->server = server;
    manager->locked = false;
    manager->idle_inhibited = false;
    
    return manager;
}

/**
 * Initialize the session manager.
 */
bool axiom_session_manager_init(struct axiom_session_manager *manager) {
    if (!manager || !manager->server) {
        AXIOM_LOG_ERROR("SESSION", "Invalid manager or server");
        return false;
    }
    
    // Initialize session lock protocol
    manager->session_lock_manager = wlr_session_lock_manager_v1_create(manager->server->wl_display);
    if (!manager->session_lock_manager) {
        AXIOM_LOG_ERROR("SESSION", "Failed to create session lock manager");
        return false;
    }
    
    manager->new_lock.notify = axiom_session_handle_new_lock;
    wl_signal_add(&manager->session_lock_manager->events.new_lock, &manager->new_lock);
    
    // Initialize idle inhibit protocol
    manager->idle_inhibit_manager = wlr_idle_inhibit_v1_create(manager->server->wl_display);
    if (!manager->idle_inhibit_manager) {
        AXIOM_LOG_WARN("SESSION", "Failed to create idle inhibit manager (non-critical)");
    } else {
        manager->new_inhibitor.notify = axiom_session_handle_new_inhibitor;
        wl_signal_add(&manager->idle_inhibit_manager->events.new_inhibitor, &manager->new_inhibitor);
        
        manager->destroy_inhibitor.notify = axiom_session_handle_destroy_inhibitor;
        wl_signal_add(&manager->idle_inhibit_manager->events.new_inhibitor, &manager->destroy_inhibitor);
    }
    
    // Initialize idle notifier
    manager->idle_notifier = wlr_idle_notifier_v1_create(manager->server->wl_display);
    if (!manager->idle_notifier) {
        AXIOM_LOG_WARN("SESSION", "Failed to create idle notifier (non-critical)");
    }
    // Note: idle_notifier_v1 doesn't have activity/idle events in newer wlroots
    // These would be handled by the idle protocol implementation
    
    AXIOM_LOG_INFO("SESSION", "Session management protocols initialized successfully");
    return true;
}

/**
 * Destroy the session manager.
 */
void axiom_session_manager_destroy(struct axiom_session_manager *manager) {
    if (!manager) {
        return;
    }
    
    if (manager->session_lock_manager) {
        // Destroyed automatically with display
    }
    
    if (manager->idle_inhibit_manager) {
        // Destroyed automatically with display
    }
    
    if (manager->idle_notifier) {
        // Destroyed automatically with display
    }
    
    axiom_free_tracked(manager, __FILE__, __func__, __LINE__);
}

/**
 * Create a new session lock wrapper.
 */
struct axiom_session_lock *axiom_session_lock_create(
    struct wlr_session_lock_v1 *wlr_session_lock, struct axiom_server *server) {
    
    struct axiom_session_lock *lock = axiom_calloc_tracked(1, 
        sizeof(struct axiom_session_lock), AXIOM_MEM_TYPE_SESSION,
        __FILE__, __func__, __LINE__);
    if (!lock) {
        AXIOM_LOG_ERROR("SESSION", "Failed to allocate session lock");
        return NULL;
    }
    
    lock->wlr_session_lock = wlr_session_lock;
    lock->server = server;
    
    // Set up event listeners
    lock->new_surface.notify = axiom_session_lock_handle_new_surface;
    wl_signal_add(&wlr_session_lock->events.new_surface, &lock->new_surface);
    
    lock->unlock.notify = axiom_session_lock_handle_unlock;
    wl_signal_add(&wlr_session_lock->events.unlock, &lock->unlock);
    
    lock->destroy.notify = axiom_session_lock_handle_destroy;
    wl_signal_add(&wlr_session_lock->events.destroy, &lock->destroy);
    
    AXIOM_LOG_DEBUG("SESSION", "Created session lock");
    return lock;
}

/**
 * Destroy a session lock wrapper.
 */
void axiom_session_lock_destroy(struct axiom_session_lock *lock) {
    if (!lock) return;
    
    // Remove event listeners
    wl_list_remove(&lock->new_surface.link);
    wl_list_remove(&lock->unlock.link);
    wl_list_remove(&lock->destroy.link);
    
    axiom_free_tracked(lock, __FILE__, __func__, __LINE__);
}

/**
 * Create a new session lock surface wrapper.
 */
struct axiom_session_lock_surface *axiom_session_lock_surface_create(
    struct wlr_session_lock_surface_v1 *wlr_lock_surface, struct axiom_server *server) {
    
    struct axiom_session_lock_surface *surface = axiom_calloc_tracked(1, 
        sizeof(struct axiom_session_lock_surface), AXIOM_MEM_TYPE_SESSION,
        __FILE__, __func__, __LINE__);
    if (!surface) {
        AXIOM_LOG_ERROR("SESSION", "Failed to allocate session lock surface");
        return NULL;
    }
    
    surface->wlr_lock_surface = wlr_lock_surface;
    surface->server = server;
    
    // Create scene tree for the lock surface
    surface->scene_tree = wlr_scene_tree_create(&server->scene->tree);
    if (!surface->scene_tree) {
        AXIOM_LOG_ERROR("SESSION", "Failed to create scene tree for lock surface");
        axiom_session_lock_surface_destroy(surface);
        return NULL;
    }
    
    // Create scene surface for rendering
    wlr_scene_surface_create(surface->scene_tree, wlr_lock_surface->surface);
    
    // Set up event listeners
    surface->map.notify = axiom_session_lock_surface_handle_map;
    wl_signal_add(&wlr_lock_surface->surface->events.map, &surface->map);
    
    surface->unmap.notify = axiom_session_lock_surface_handle_unmap;
    wl_signal_add(&wlr_lock_surface->surface->events.unmap, &surface->unmap);
    
    surface->destroy.notify = axiom_session_lock_surface_handle_destroy;
    wl_signal_add(&wlr_lock_surface->events.destroy, &surface->destroy);
    
    surface->commit.notify = axiom_session_lock_surface_handle_commit;
    wl_signal_add(&wlr_lock_surface->surface->events.commit, &surface->commit);
    
    AXIOM_LOG_DEBUG("SESSION", "Created session lock surface");
    return surface;
}

/**
 * Destroy a session lock surface wrapper.
 */
void axiom_session_lock_surface_destroy(struct axiom_session_lock_surface *surface) {
    if (!surface) return;
    
    // Remove event listeners
    wl_list_remove(&surface->map.link);
    wl_list_remove(&surface->unmap.link);
    wl_list_remove(&surface->destroy.link);
    wl_list_remove(&surface->commit.link);
    
    if (surface->scene_tree) {
        wlr_scene_node_destroy(&surface->scene_tree->node);
    }
    
    axiom_free_tracked(surface, __FILE__, __func__, __LINE__);
}

// ===== Event Handlers =====

void axiom_session_handle_new_lock(struct wl_listener *listener, void *data) {
    struct axiom_session_manager *manager = wl_container_of(listener, manager, new_lock);
    struct wlr_session_lock_v1 *wlr_session_lock = data;
    
    AXIOM_LOG_INFO("SESSION", "New session lock requested");
    
    // Create session lock wrapper
    struct axiom_session_lock *lock = axiom_session_lock_create(wlr_session_lock, manager->server);
    if (!lock) {
        AXIOM_LOG_ERROR("SESSION", "Failed to create session lock wrapper");
        wlr_session_lock_v1_destroy(wlr_session_lock);
        return;
    }
    
    // Mark session as locked
    manager->locked = true;
    
    // Store reference for later retrieval
    wlr_session_lock->data = lock;
    
    // Send locked event to the client
    wlr_session_lock_v1_send_locked(wlr_session_lock);
    
    AXIOM_LOG_INFO("SESSION", "Session locked successfully");
}

void axiom_session_handle_new_inhibitor(struct wl_listener *listener, void *data) {
    struct axiom_session_manager *manager = wl_container_of(listener, manager, new_inhibitor);
    (void)data;
    
    AXIOM_LOG_DEBUG("SESSION", "New idle inhibitor created");
    manager->idle_inhibited = true;
}

void axiom_session_handle_destroy_inhibitor(struct wl_listener *listener, void *data) {
    struct axiom_session_manager *manager = wl_container_of(listener, manager, destroy_inhibitor);
    (void)data;
    
    AXIOM_LOG_DEBUG("SESSION", "Idle inhibitor destroyed");
    
    // Check if there are any remaining inhibitors
    if (wl_list_empty(&manager->idle_inhibit_manager->inhibitors)) {
        manager->idle_inhibited = false;
    }
}

void axiom_session_handle_idle_activity(struct wl_listener *listener, void *data) {
    struct axiom_session_manager *manager = wl_container_of(listener, manager, idle_activity);
    (void)data;
    
    AXIOM_LOG_DEBUG("SESSION", "User activity detected");
    // Reset idle timers, wake up displays, etc.
}

void axiom_session_handle_idle_timeout(struct wl_listener *listener, void *data) {
    struct axiom_session_manager *manager = wl_container_of(listener, manager, idle_timeout);
    (void)data;
    
    AXIOM_LOG_INFO("SESSION", "Idle timeout reached");
    
    if (!manager->idle_inhibited) {
        // Trigger idle actions: dim screen, lock session, etc.
        AXIOM_LOG_INFO("SESSION", "System going idle (no inhibitors active)");
    } else {
        AXIOM_LOG_DEBUG("SESSION", "Idle timeout ignored due to active inhibitors");
    }
}

void axiom_session_lock_handle_new_surface(struct wl_listener *listener, void *data) {
    struct axiom_session_lock *lock = wl_container_of(listener, lock, new_surface);
    struct wlr_session_lock_surface_v1 *wlr_lock_surface = data;
    
    AXIOM_LOG_DEBUG("SESSION", "New session lock surface");
    
    // Create lock surface wrapper
    struct axiom_session_lock_surface *surface = axiom_session_lock_surface_create(wlr_lock_surface, lock->server);
    if (!surface) {
        AXIOM_LOG_ERROR("SESSION", "Failed to create session lock surface wrapper");
        return;
    }
    
    // Store reference for later retrieval
    wlr_lock_surface->data = surface;
}

void axiom_session_lock_handle_unlock(struct wl_listener *listener, void *data) {
    struct axiom_session_lock *lock = wl_container_of(listener, lock, unlock);
    (void)data;
    
    AXIOM_LOG_INFO("SESSION", "Session unlock requested");
    
    // Mark session as unlocked
    lock->server->session_manager->locked = false;
    
    // Destroy the session lock
    wlr_session_lock_v1_destroy(lock->wlr_session_lock);
}

void axiom_session_lock_handle_destroy(struct wl_listener *listener, void *data) {
    struct axiom_session_lock *lock = wl_container_of(listener, lock, destroy);
    (void)data;
    
    AXIOM_LOG_INFO("SESSION", "Session lock destroyed");
    
    // Ensure session is marked as unlocked
    lock->server->session_manager->locked = false;
    
    // Clean up the session lock wrapper
    axiom_session_lock_destroy(lock);
}

void axiom_session_lock_surface_handle_map(struct wl_listener *listener, void *data) {
    struct axiom_session_lock_surface *surface = wl_container_of(listener, surface, map);
    (void)data;
    
    AXIOM_LOG_DEBUG("SESSION", "Session lock surface mapped");
    
    // Configure the lock surface to cover the entire output
    struct wlr_output *output = surface->wlr_lock_surface->output;
    if (output) {
        wlr_session_lock_surface_v1_configure(surface->wlr_lock_surface, 
                                             output->width, output->height);
        
        // Position the scene tree to cover the output
        wlr_scene_node_set_position(&surface->scene_tree->node, 0, 0);
    }
}

void axiom_session_lock_surface_handle_unmap(struct wl_listener *listener, void *data) {
    struct axiom_session_lock_surface *surface = wl_container_of(listener, surface, unmap);
    (void)data;
    
    AXIOM_LOG_DEBUG("SESSION", "Session lock surface unmapped");
}

void axiom_session_lock_surface_handle_destroy(struct wl_listener *listener, void *data) {
    struct axiom_session_lock_surface *surface = wl_container_of(listener, surface, destroy);
    (void)data;
    
    AXIOM_LOG_DEBUG("SESSION", "Session lock surface destroyed");
    axiom_session_lock_surface_destroy(surface);
}

void axiom_session_lock_surface_handle_commit(struct wl_listener *listener, void *data) {
    struct axiom_session_lock_surface *surface = wl_container_of(listener, surface, commit);
    (void)data;
    
    AXIOM_LOG_DEBUG("SESSION", "Session lock surface committed");
    // Handle any surface updates if needed
}

// ===== Utility Functions =====

/**
 * Lock the screen programmatically.
 */
bool axiom_session_lock_screen(struct axiom_server *server) {
    if (!server || !server->session_manager) {
        AXIOM_LOG_ERROR("SESSION", "Session manager not initialized");
        return false;
    }
    
    if (server->session_manager->locked) {
        AXIOM_LOG_WARN("SESSION", "Session is already locked");
        return true;
    }
    
    AXIOM_LOG_INFO("SESSION", "Locking session programmatically");
    
    // In a full implementation, you would create a session lock request
    // For now, we'll just set the flag and notify
    server->session_manager->locked = true;
    
    return true;
}

/**
 * Unlock the screen programmatically.
 */
bool axiom_session_unlock_screen(struct axiom_server *server) {
    if (!server || !server->session_manager) {
        AXIOM_LOG_ERROR("SESSION", "Session manager not initialized");
        return false;
    }
    
    if (!server->session_manager->locked) {
        AXIOM_LOG_WARN("SESSION", "Session is not locked");
        return true;
    }
    
    AXIOM_LOG_INFO("SESSION", "Unlocking session programmatically");
    server->session_manager->locked = false;
    
    return true;
}

/**
 * Check if the session is currently locked.
 */
bool axiom_session_is_locked(struct axiom_server *server) {
    if (!server || !server->session_manager) {
        return false;
    }
    
    return server->session_manager->locked;
}

/**
 * Inhibit or allow idle behavior.
 */
void axiom_session_inhibit_idle(struct axiom_server *server, bool inhibit) {
    if (!server || !server->session_manager) {
        return;
    }
    
    server->session_manager->idle_inhibited = inhibit;
    
    AXIOM_LOG_DEBUG("SESSION", "Idle inhibition %s", inhibit ? "enabled" : "disabled");
}
