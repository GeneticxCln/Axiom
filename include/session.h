#ifndef AXIOM_SESSION_H
#define AXIOM_SESSION_H

#include <stdbool.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_session_lock_v1.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>
#include <wlr/types/wlr_idle_notify_v1.h>

struct axiom_server;

/**
 * Session manager for session control, locking, and power management
 */
struct axiom_session_manager {
    struct wlr_session_lock_manager_v1 *session_lock_manager;
    struct wlr_idle_inhibit_manager_v1 *idle_inhibit_manager;
    struct wlr_idle_notifier_v1 *idle_notifier;
    struct axiom_server *server;
    
    // Session state
    bool locked;
    bool idle_inhibited;
    
    // Event listeners
    struct wl_listener new_lock;
    struct wl_listener new_inhibitor;
    struct wl_listener destroy_inhibitor;
    struct wl_listener idle_activity;
    struct wl_listener idle_timeout;
};

/**
 * Session lock state
 */
struct axiom_session_lock {
    struct wlr_session_lock_v1 *wlr_session_lock;
    struct axiom_server *server;
    
    // Event listeners
    struct wl_listener new_surface;
    struct wl_listener unlock;
    struct wl_listener destroy;
};

/**
 * Session lock surface
 */
struct axiom_session_lock_surface {
    struct wlr_session_lock_surface_v1 *wlr_lock_surface;
    struct axiom_server *server;
    struct wlr_scene_tree *scene_tree;
    
    // Event listeners
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener commit;
};

// Session manager functions
struct axiom_session_manager *axiom_session_manager_create(struct axiom_server *server);
void axiom_session_manager_destroy(struct axiom_session_manager *manager);
bool axiom_session_manager_init(struct axiom_session_manager *manager);

// Session lock functions
struct axiom_session_lock *axiom_session_lock_create(
    struct wlr_session_lock_v1 *wlr_session_lock, struct axiom_server *server);
void axiom_session_lock_destroy(struct axiom_session_lock *lock);

struct axiom_session_lock_surface *axiom_session_lock_surface_create(
    struct wlr_session_lock_surface_v1 *wlr_lock_surface, struct axiom_server *server);
void axiom_session_lock_surface_destroy(struct axiom_session_lock_surface *surface);

// Event handlers
void axiom_session_handle_new_lock(struct wl_listener *listener, void *data);
void axiom_session_handle_new_inhibitor(struct wl_listener *listener, void *data);
void axiom_session_handle_destroy_inhibitor(struct wl_listener *listener, void *data);
void axiom_session_handle_idle_activity(struct wl_listener *listener, void *data);
void axiom_session_handle_idle_timeout(struct wl_listener *listener, void *data);

void axiom_session_lock_handle_new_surface(struct wl_listener *listener, void *data);
void axiom_session_lock_handle_unlock(struct wl_listener *listener, void *data);
void axiom_session_lock_handle_destroy(struct wl_listener *listener, void *data);

void axiom_session_lock_surface_handle_map(struct wl_listener *listener, void *data);
void axiom_session_lock_surface_handle_unmap(struct wl_listener *listener, void *data);
void axiom_session_lock_surface_handle_destroy(struct wl_listener *listener, void *data);
void axiom_session_lock_surface_handle_commit(struct wl_listener *listener, void *data);

// Utility functions
bool axiom_session_lock_screen(struct axiom_server *server);
bool axiom_session_unlock_screen(struct axiom_server *server);
bool axiom_session_is_locked(struct axiom_server *server);
void axiom_session_inhibit_idle(struct axiom_server *server, bool inhibit);

#endif /* AXIOM_SESSION_H */
