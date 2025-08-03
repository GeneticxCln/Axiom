#include "multi_session.h"
#include "logging.h"
#include "memory.h"
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>

/**
 * Create and initialize the multi-session manager.
 */
struct axiom_multi_session_manager *axiom_multi_session_manager_create(struct axiom_server *server) {
    struct axiom_multi_session_manager *manager = axiom_calloc_tracked(1, 
        sizeof(struct axiom_multi_session_manager), AXIOM_MEM_TYPE_SESSION,
        __FILE__, __func__, __LINE__);
    if (!manager) {
        AXIOM_LOG_ERROR("SESSION", "Failed to allocate multi-session manager");
        return NULL;
    }
    
    manager->primary_server = server;
    manager->next_session_id = 1;
    wl_list_init(&manager->sessions);
    manager->switching_enabled = true;
    manager->max_sessions = AXIOM_MAX_SESSIONS;
    manager->max_sessions_per_user = 3;
    manager->allow_root_session = false;
    
    return manager;
}

/**
 * Initialize the multi-session manager.
 */
bool axiom_multi_session_manager_init(struct axiom_multi_session_manager *manager) {
    if (!manager) {
        AXIOM_LOG_ERROR("SESSION", "Multi-session manager is NULL");
        return false;
    }
    
    // Initialize configuration
    if (!axiom_multi_session_load_config(manager, "config/multi_session.ini")) {
        AXIOM_LOG_WARN("Failed to load multi-session configuration, using defaults");
    }
    
    AXIOM_LOG_INFO("Multi-session manager initialized successfully");
    return true;
}

/**
 * Destroy the multi-session manager.
 */
void axiom_multi_session_manager_destroy(struct axiom_multi_session_manager *manager) {
    if (!manager) {
        return;
    }
    
    // Destroy sessions
    struct axiom_user_session *session, *tmp;
    wl_list_for_each_safe(session, tmp, &manager->sessions, link) {
        axiom_user_session_destroy(session);
    }
    
    axiom_free_tracked(manager, __FILE__, __func__, __LINE__);
    AXIOM_LOG_INFO("Multi-session manager destroyed");
}

/**
 * Create a new user session.
 */
struct axiom_user_session *axiom_user_session_create(
    struct axiom_multi_session_manager *manager,
    const char *username,
    uid_t uid,
    gid_t gid,
    enum axiom_session_type type) {
    if (!manager || (uint32_t)wl_list_length(&manager->sessions) >= manager->max_sessions) {
        AXIOM_LOG_ERROR("SESSION", "Cannot create more sessions");
        return NULL;
    }

    struct axiom_user_session *session = axiom_calloc_tracked(1, 
        sizeof(struct axiom_user_session), AXIOM_MEM_TYPE_SESSION,
        __FILE__, __func__, __LINE__);
    if (!session) {
        AXIOM_LOG_ERROR("SESSION", "Failed to allocate user session");
        return NULL;
    }

    // Initialize session properties
    session->session_id = manager->next_session_id++;
    strncpy(session->username, username, AXIOM_SESSION_NAME_MAX - 1);
    session->username[AXIOM_SESSION_NAME_MAX - 1] = '\0';
    session->uid = uid;
    session->gid = gid;
    session->type = type;
    session->state = AXIOM_SESSION_INACTIVE;
    session->created_time = time(NULL);
    
    wl_list_insert(&manager->sessions, &session->link);
    
    AXIOM_LOG_INFO("SESSION", "Created new session for user %s", username);
    return session;
}

/**
 * Destroy a user session.
 */
void axiom_user_session_destroy(struct axiom_user_session *session) {
    if (!session) return;

    // Remove from session list
    wl_list_remove(&session->link);

    axiom_free_tracked(session, __FILE__, __func__, __LINE__);
    AXIOM_LOG_INFO("SESSION", "Destroyed session for user %s", session->username);
}

/**
 * Activate a user session.
 */
bool axiom_user_session_activate(struct axiom_user_session *session) {
    if (!session) return false;
    
    session->state = AXIOM_SESSION_ACTIVE;
    session->last_activity = time(NULL);
    
    AXIOM_LOG_INFO("SESSION", "Activated session for user %s", session->username);
    return true;
}

/**
 * Deactivate a user session.
 */
bool axiom_user_session_deactivate(struct axiom_user_session *session) {
    if (!session) return false;
    
    session->state = AXIOM_SESSION_INACTIVE;
    
    AXIOM_LOG_INFO("SESSION", "Deactivated session for user %s", session->username);
    return true;
}

/**
 * Switch to a specific user session.
 */
bool axiom_session_switch_to_user(struct axiom_multi_session_manager *manager, const char *username) {
    struct axiom_user_session *session;
    wl_list_for_each(session, &manager->sessions, link) {
        if (strcmp(session->username, username) == 0) {
            axiom_user_session_deactivate(manager->active_session);
            manager->active_session = session;
            axiom_user_session_activate(session);
            AXIOM_LOG_INFO("SESSION", "Switched to session for user %s", username);
            return true;
        }
    }
    
    AXIOM_LOG_WARN("SESSION", "No session found for user %s", username);
    return false;
}

/**
 * Utility functions.
 */
const char *axiom_session_type_to_string(enum axiom_session_type type) {
    switch (type) {
        case AXIOM_SESSION_USER: return "User";
        case AXIOM_SESSION_GREETER: return "Greeter";
        case AXIOM_SESSION_LOCK: return "Lock";
        case AXIOM_SESSION_SYSTEM: return "System";
        default: return "Unknown";
    }
}

const char *axiom_session_state_to_string(enum axiom_session_state state) {
    switch (state) {
        case AXIOM_SESSION_INACTIVE: return "Inactive";
        case AXIOM_SESSION_ACTIVE: return "Active";
        case AXIOM_SESSION_LOCKED: return "Locked";
        case AXIOM_SESSION_SUSPENDED: return "Suspended";
        case AXIOM_SESSION_SWITCHING: return "Switching";
        default: return "Unknown";
    }
}

/**
 * Load multi-session configuration (stub implementation)
 */
bool axiom_multi_session_load_config(struct axiom_multi_session_manager *manager, const char *config_path) {
    if (!manager || !config_path) {
        return false;
    }
    
    // For now, use default values
    // TODO: Implement actual configuration file loading
    AXIOM_LOG_DEBUG("SESSION", "Using default multi-session configuration");
    return true;
}

/**
 * Find session by username
 */
struct axiom_user_session *axiom_session_find_by_username(
    struct axiom_multi_session_manager *manager, const char *username) {
    if (!manager || !username) {
        return NULL;
    }
    
    struct axiom_user_session *session;
    wl_list_for_each(session, &manager->sessions, link) {
        if (strcmp(session->username, username) == 0) {
            return session;
        }
    }
    
    return NULL;
}

/**
 * Find session by ID
 */
struct axiom_user_session *axiom_session_find_by_id(
    struct axiom_multi_session_manager *manager, uint32_t session_id) {
    if (!manager) {
        return NULL;
    }
    
    struct axiom_user_session *session;
    wl_list_for_each(session, &manager->sessions, link) {
        if (session->session_id == session_id) {
            return session;
        }
    }
    
    return NULL;
}

/**
 * Get active session
 */
struct axiom_user_session *axiom_session_get_active(struct axiom_multi_session_manager *manager) {
    if (!manager) {
        return NULL;
    }
    
    return manager->active_session;
}

/**
 * Session switching functions
 */
bool axiom_session_switch_to_id(struct axiom_multi_session_manager *manager, uint32_t session_id) {
    struct axiom_user_session *session = axiom_session_find_by_id(manager, session_id);
    if (!session) {
        AXIOM_LOG_WARN("SESSION", "No session found with ID %u", session_id);
        return false;
    }
    
    if (manager->active_session) {
        axiom_user_session_deactivate(manager->active_session);
    }
    
    manager->active_session = session;
    axiom_user_session_activate(session);
    
    AXIOM_LOG_INFO("SESSION", "Switched to session ID %u for user %s", session_id, session->username);
    return true;
}

bool axiom_session_switch_next(struct axiom_multi_session_manager *manager) {
    if (!manager || wl_list_empty(&manager->sessions)) {
        return false;
    }
    
    struct axiom_user_session *next_session;
    if (manager->active_session) {
        // Find next session in list
        next_session = wl_container_of(manager->active_session->link.next, next_session, link);
        // If we're at the end, wrap to beginning
        if (&next_session->link == &manager->sessions) {
            next_session = wl_container_of(manager->sessions.next, next_session, link);
        }
    } else {
        // No active session, get first one
        next_session = wl_container_of(manager->sessions.next, next_session, link);
    }
    
    return axiom_session_switch_to_id(manager, next_session->session_id);
}

bool axiom_session_switch_previous(struct axiom_multi_session_manager *manager) {
    if (!manager || wl_list_empty(&manager->sessions)) {
        return false;
    }
    
    struct axiom_user_session *prev_session;
    if (manager->active_session) {
        // Find previous session in list
        prev_session = wl_container_of(manager->active_session->link.prev, prev_session, link);
        // If we're at the beginning, wrap to end
        if (&prev_session->link == &manager->sessions) {
            prev_session = wl_container_of(manager->sessions.prev, prev_session, link);
        }
    } else {
        // No active session, get last one
        prev_session = wl_container_of(manager->sessions.prev, prev_session, link);
    }
    
    return axiom_session_switch_to_id(manager, prev_session->session_id);
}

/**
 * Print session status for debugging
 */
void axiom_multi_session_print_status(struct axiom_multi_session_manager *manager) {
    if (!manager) {
        return;
    }
    
    AXIOM_LOG_INFO("SESSION", "Multi-session manager status:");
    AXIOM_LOG_INFO("SESSION", "  Active sessions: %d", wl_list_length(&manager->sessions));
    AXIOM_LOG_INFO("SESSION", "  Max sessions: %u", manager->max_sessions);
    AXIOM_LOG_INFO("SESSION", "  Switching enabled: %s", manager->switching_enabled ? "yes" : "no");
    
    if (manager->active_session) {
        AXIOM_LOG_INFO("SESSION", "  Active session: %s (ID: %u)", 
                       manager->active_session->username, 
                       manager->active_session->session_id);
    } else {
        AXIOM_LOG_INFO("SESSION", "  No active session");
    }
    
    struct axiom_user_session *session;
    wl_list_for_each(session, &manager->sessions, link) {
        AXIOM_LOG_INFO("SESSION", "  Session %u: %s (%s) - %s", 
                       session->session_id,
                       session->username,
                       axiom_session_type_to_string(session->type),
                       axiom_session_state_to_string(session->state));
    }
}

/**
 * Check if session is valid
 */
bool axiom_session_is_valid(struct axiom_user_session *session) {
    return session && session->session_id > 0 && strlen(session->username) > 0;
}

/**
 * Get session count for a specific user
 */
uint32_t axiom_session_get_user_session_count(struct axiom_multi_session_manager *manager, uid_t uid) {
    if (!manager) {
        return 0;
    }
    
    uint32_t count = 0;
    struct axiom_user_session *session;
    wl_list_for_each(session, &manager->sessions, link) {
        if (session->uid == uid) {
            count++;
        }
    }
    
    return count;
}
