#ifndef AXIOM_MULTI_SESSION_H
#define AXIOM_MULTI_SESSION_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <wayland-server-core.h>

struct axiom_server;

#define AXIOM_MAX_SESSIONS 32
#define AXIOM_SESSION_NAME_MAX 64
#define AXIOM_SESSION_PATH_MAX 256

/**
 * Session types
 */
enum axiom_session_type {
    AXIOM_SESSION_USER,        // Regular user session
    AXIOM_SESSION_GREETER,     // Login greeter session
    AXIOM_SESSION_LOCK,        // Lock screen session
    AXIOM_SESSION_SYSTEM,      // System maintenance session
};

/**
 * Session state
 */
enum axiom_session_state {
    AXIOM_SESSION_INACTIVE,    // Session is not running
    AXIOM_SESSION_ACTIVE,      // Session is currently active
    AXIOM_SESSION_LOCKED,      // Session is locked
    AXIOM_SESSION_SUSPENDED,   // Session is suspended
    AXIOM_SESSION_SWITCHING,   // Session is switching
};

/**
 * Individual session data
 */
struct axiom_user_session {
    struct wl_list link;
    
    // Session identification
    uint32_t session_id;
    char session_name[AXIOM_SESSION_NAME_MAX];
    char seat_name[AXIOM_SESSION_NAME_MAX];
    
    // User information
    uid_t uid;
    gid_t gid;
    char username[AXIOM_SESSION_NAME_MAX];
    char home_directory[AXIOM_SESSION_PATH_MAX];
    char display_name[AXIOM_SESSION_NAME_MAX];
    
    // Session properties
    enum axiom_session_type type;
    enum axiom_session_state state;
    pid_t session_pid;
    
    // Wayland display for this session
    struct wl_display *wl_display;
    char wayland_socket[AXIOM_SESSION_NAME_MAX];
    
    // Session resources
    struct axiom_server *server;          // Dedicated server instance
    void *backend_session;                 // Backend-specific session data
    
    // Timing information
    time_t created_time;
    time_t last_activity;
    time_t locked_time;
    
    // Session configuration
    bool auto_lock;
    uint32_t idle_timeout;
    bool allow_background;
    bool persistent;
    
    // Event listeners
    struct wl_listener session_destroy;
    struct wl_listener session_activate;
    struct wl_listener session_deactivate;
    
    // Session-specific data
    void *user_data;
};

/**
 * Multi-session manager
 */
struct axiom_multi_session_manager {
    struct axiom_server *primary_server;
    
    // Session management
    struct wl_list sessions;              // List of axiom_user_session
    struct axiom_user_session *active_session;
    struct axiom_user_session *previous_session;
    uint32_t next_session_id;
    
    // Session switching
    bool switching_enabled;
    bool switch_in_progress;
    uint32_t switch_timeout_ms;
    struct wl_event_source *switch_timer;
    
    // Authentication
    bool require_authentication;
    char auth_command[AXIOM_SESSION_PATH_MAX];
    
    // Session limits
    uint32_t max_sessions;
    uint32_t max_sessions_per_user;
    bool allow_root_session;
    
    // Event listeners
    struct wl_listener new_session;
    struct wl_listener session_changed;
    struct wl_listener user_activity;
    
    // Session storage
    char session_storage_path[AXIOM_SESSION_PATH_MAX];
    bool save_session_state;
    
    // Statistics
    uint32_t total_sessions_created;
    uint32_t active_session_count;
    time_t manager_start_time;
};

/**
 * Session switching context
 */
struct axiom_session_switch_context {
    struct axiom_user_session *from_session;
    struct axiom_user_session *to_session;
    bool preserve_state;
    uint32_t switch_reason;
    void (*completion_callback)(struct axiom_session_switch_context *ctx, bool success);
};

// Multi-session manager functions
struct axiom_multi_session_manager *axiom_multi_session_manager_create(struct axiom_server *server);
void axiom_multi_session_manager_destroy(struct axiom_multi_session_manager *manager);
bool axiom_multi_session_manager_init(struct axiom_multi_session_manager *manager);

// Session management functions
struct axiom_user_session *axiom_user_session_create(
    struct axiom_multi_session_manager *manager,
    const char *username,
    uid_t uid,
    gid_t gid,
    enum axiom_session_type type
);

void axiom_user_session_destroy(struct axiom_user_session *session);
bool axiom_user_session_start(struct axiom_user_session *session);
bool axiom_user_session_stop(struct axiom_user_session *session);
bool axiom_user_session_activate(struct axiom_user_session *session);
bool axiom_user_session_deactivate(struct axiom_user_session *session);

// Session switching functions
bool axiom_session_switch_to_user(struct axiom_multi_session_manager *manager, const char *username);
bool axiom_session_switch_to_id(struct axiom_multi_session_manager *manager, uint32_t session_id);
bool axiom_session_switch_next(struct axiom_multi_session_manager *manager);
bool axiom_session_switch_previous(struct axiom_multi_session_manager *manager);

// Session query functions
struct axiom_user_session *axiom_session_find_by_username(
    struct axiom_multi_session_manager *manager, const char *username);
struct axiom_user_session *axiom_session_find_by_id(
    struct axiom_multi_session_manager *manager, uint32_t session_id);
struct axiom_user_session *axiom_session_get_active(struct axiom_multi_session_manager *manager);

// Session state management
bool axiom_session_lock(struct axiom_user_session *session);
bool axiom_session_unlock(struct axiom_user_session *session);
bool axiom_session_suspend(struct axiom_user_session *session);
bool axiom_session_resume(struct axiom_user_session *session);

// Session persistence
bool axiom_session_save_state(struct axiom_user_session *session);
bool axiom_session_restore_state(struct axiom_user_session *session);
bool axiom_session_clear_saved_state(struct axiom_user_session *session);

// Authentication functions
bool axiom_session_authenticate_user(const char *username, const char *password);
bool axiom_session_verify_user_permissions(uid_t uid, gid_t gid);
bool axiom_session_setup_user_environment(struct axiom_user_session *session);

// Session isolation functions
bool axiom_session_create_namespace(struct axiom_user_session *session);
bool axiom_session_setup_cgroups(struct axiom_user_session *session);
bool axiom_session_apply_security_context(struct axiom_user_session *session);

// Event handlers
void axiom_multi_session_handle_new_session(struct wl_listener *listener, void *data);
void axiom_multi_session_handle_session_destroy(struct wl_listener *listener, void *data);
void axiom_multi_session_handle_session_activate(struct wl_listener *listener, void *data);
void axiom_multi_session_handle_session_deactivate(struct wl_listener *listener, void *data);
void axiom_multi_session_handle_user_activity(struct wl_listener *listener, void *data);

// Utility functions  
const char *axiom_session_type_to_string(enum axiom_session_type type);
const char *axiom_session_state_to_string(enum axiom_session_state state);
bool axiom_session_is_valid(struct axiom_user_session *session);
uint32_t axiom_session_get_user_session_count(struct axiom_multi_session_manager *manager, uid_t uid);

// Configuration functions
bool axiom_multi_session_load_config(struct axiom_multi_session_manager *manager, const char *config_path);
bool axiom_multi_session_validate_config(struct axiom_multi_session_manager *manager);

// Debug and monitoring functions
void axiom_multi_session_print_status(struct axiom_multi_session_manager *manager);
void axiom_session_print_info(struct axiom_user_session *session);
bool axiom_multi_session_export_status(struct axiom_multi_session_manager *manager, const char *output_path);

#endif /* AXIOM_MULTI_SESSION_H */
