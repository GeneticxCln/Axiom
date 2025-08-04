#ifndef AXIOM_WORKSPACE_MANAGER_H
#define AXIOM_WORKSPACE_MANAGER_H

#include <wayland-server-core.h>
#include <stdbool.h>
#include <stdint.h>

// Forward declarations
struct axiom_server;
struct axiom_window;
struct axiom_monitor;
struct axiom_layout_interface;

// Workspace types
enum axiom_workspace_type {
    AXIOM_WORKSPACE_REGULAR = 0,
    AXIOM_WORKSPACE_SPECIAL,     // Special/scratchpad workspace
    AXIOM_WORKSPACE_PERSISTENT,  // Persistent workspace
    AXIOM_WORKSPACE_TEMPORARY    // Temporary workspace
};

// Fullscreen modes (inspired by Hyprland)
enum axiom_fullscreen_mode {
    AXIOM_FSMODE_NONE = 0,
    AXIOM_FSMODE_MAXIMIZED = 1 << 0,
    AXIOM_FSMODE_FULLSCREEN = 1 << 1,
    AXIOM_FSMODE_FAKE_FULLSCREEN = 1 << 2,
    AXIOM_FSMODE_MAX = (1 << 3) - 1
};

// Workspace rule structure
struct axiom_workspace_rule {
    struct wl_list link;
    char *name_pattern;          // Workspace name pattern
    char *monitor_pattern;       // Monitor pattern for this workspace
    int default_layout;          // Default layout for this workspace
    bool default_floating;       // Default floating state
    bool gapless_when_only;      // No gaps when only one window
    int border_size;             // Custom border size
    int gaps_in;                 // Custom inner gaps
    int gaps_out;                // Custom outer gaps
    bool persistent;             // Workspace persists when empty
    bool no_focus_on_activate;   // Don't focus when activated
    float alpha;                 // Workspace transparency
    bool silent;                 // Silent workspace switching
};

// Enhanced workspace structure (inspired by Hyprland's CWorkspace)
struct axiom_workspace {
    struct wl_list link;
    struct axiom_server *server;
    
    // Workspace identification
    int id;                      // Workspace ID (>0 for regular, <0 for special)
    char *name;                  // Workspace name
    enum axiom_workspace_type type;
    
    // Monitor association
    struct axiom_monitor *monitor;
    char *last_monitor_name;     // For reconnection
    
    // Window management
    struct wl_list windows;      // Windows in this workspace
    struct axiom_window *last_focused_window;
    struct axiom_window *fullscreen_window;
    int window_count;
    int tiled_window_count;
    int floating_window_count;
    
    // Fullscreen state
    bool has_fullscreen_window;
    enum axiom_fullscreen_mode fullscreen_mode;
    
    // Layout and appearance
    struct axiom_layout_interface *layout;
    bool visible;                // Currently visible
    bool force_rendering;        // Force rendering even when not visible
    
    // Animation properties
    double render_offset_x, render_offset_y;
    float alpha;
    bool animating_in;
    bool animating_out;
    
    // Workspace-specific settings
    bool default_floating;
    bool default_pseudo;
    bool gapless_when_only;
    int custom_border_size;
    int custom_gaps_in;
    int custom_gaps_out;
    
    // State tracking
    bool was_created_empty;
    bool is_persistent;
    bool needs_arrangement;
    bool inert;                  // Marked for destruction
    
    // Performance tracking
    uint64_t last_focus_time;
    uint64_t creation_time;
    uint32_t focus_count;
    
    // Configuration
    struct axiom_workspace_rule *active_rule;
    
    // Event listeners and signals
    struct {
        struct wl_signal destroy;
        struct wl_signal rename;
        struct wl_signal monitor_changed;
        struct wl_signal active_changed;
        struct wl_signal window_added;
        struct wl_signal window_removed;
        struct wl_signal fullscreen_changed;
    } events;
    
    // Animation and effects data
    struct {
        bool fade_enabled;
        float fade_duration;
        bool slide_enabled;
        char *slide_direction;
        float slide_distance;
        bool zoom_enabled;
        float zoom_factor;
    } animation_config;
};

// Workspace manager structure
struct axiom_workspace_manager {
    struct axiom_server *server;
    
    // Workspace collections
    struct wl_list workspaces;           // All workspaces
    struct wl_list persistent_workspaces; // Persistent workspaces
    struct wl_list special_workspaces;   // Special workspaces
    
    // Current state
    struct axiom_workspace *active_workspace;
    struct axiom_workspace *previous_workspace;
    struct axiom_workspace *active_special_workspace;
    
    // Configuration
    struct wl_list workspace_rules;      // List of workspace rules
    int max_workspaces;
    int next_workspace_id;
    int next_special_id;
    
    // Named workspaces
    char **workspace_names;
    int workspace_names_count;
    
    // Auto-creation settings
    bool auto_create_workspaces;
    bool auto_destroy_empty_workspaces;
    bool remember_window_workspace;
    
    // Animation settings
    bool workspace_animations_enabled;
    float animation_speed;
    char *animation_curve;
    
    // Performance tracking
    uint32_t total_workspaces;
    uint32_t active_workspaces;
    uint64_t last_switch_time;
    uint32_t switch_count;
    
    // Event listeners
    struct wl_listener window_focus_change;
    struct wl_listener monitor_change;
    
    // Special workspace management
    struct {
        bool scratchpad_enabled;
        int max_special_workspaces;
        float special_scale_factor;
        bool special_center;
        int special_gap_size;
        bool special_shadow;
    } special_config;
};

// Workspace manager functions
struct axiom_workspace_manager *axiom_workspace_manager_create(struct axiom_server *server);
void axiom_workspace_manager_destroy(struct axiom_workspace_manager *manager);

// Workspace lifecycle
struct axiom_workspace *axiom_workspace_create(struct axiom_workspace_manager *manager,
                                              int id, const char *name,
                                              struct axiom_monitor *monitor,
                                              enum axiom_workspace_type type);
void axiom_workspace_destroy(struct axiom_workspace *workspace);
void axiom_workspace_mark_inert(struct axiom_workspace *workspace);
bool axiom_workspace_is_inert(struct axiom_workspace *workspace);

// Workspace queries
struct axiom_workspace *axiom_workspace_manager_get_by_id(struct axiom_workspace_manager *manager, int id);
struct axiom_workspace *axiom_workspace_manager_get_by_name(struct axiom_workspace_manager *manager, const char *name);
struct axiom_workspace *axiom_workspace_manager_get_by_string(struct axiom_workspace_manager *manager, const char *str);
struct axiom_workspace *axiom_workspace_manager_get_active(struct axiom_workspace_manager *manager);
struct axiom_workspace *axiom_workspace_manager_get_previous(struct axiom_workspace_manager *manager);

// Workspace navigation
void axiom_workspace_manager_switch_to_workspace(struct axiom_workspace_manager *manager,
                                                struct axiom_workspace *workspace,
                                                bool no_warp_cursor);
void axiom_workspace_manager_switch_to_workspace_by_id(struct axiom_workspace_manager *manager, int id);
void axiom_workspace_manager_switch_to_workspace_by_name(struct axiom_workspace_manager *manager, const char *name);
void axiom_workspace_manager_switch_to_previous(struct axiom_workspace_manager *manager);

// Window-workspace operations
void axiom_workspace_manager_move_window_to_workspace(struct axiom_workspace_manager *manager,
                                                     struct axiom_window *window,
                                                     struct axiom_workspace *workspace);
void axiom_workspace_manager_move_window_to_workspace_by_id(struct axiom_workspace_manager *manager,
                                                           struct axiom_window *window,
                                                           int workspace_id);
void axiom_workspace_manager_move_window_silently(struct axiom_workspace_manager *manager,
                                                 struct axiom_window *window,
                                                 struct axiom_workspace *workspace);

// Workspace-monitor operations
void axiom_workspace_manager_move_workspace_to_monitor(struct axiom_workspace_manager *manager,
                                                      struct axiom_workspace *workspace,
                                                      struct axiom_monitor *monitor);
void axiom_workspace_manager_swap_workspace_monitors(struct axiom_workspace_manager *manager,
                                                    struct axiom_workspace *workspace1,
                                                    struct axiom_workspace *workspace2);

// Workspace arrangement and layout
void axiom_workspace_arrange_windows(struct axiom_workspace *workspace);
void axiom_workspace_update_window_decorations(struct axiom_workspace *workspace);
void axiom_workspace_force_report_sizes_to_windows(struct axiom_workspace *workspace);
void axiom_workspace_update_windows(struct axiom_workspace *workspace);

// Workspace properties
bool axiom_workspace_is_visible(struct axiom_workspace *workspace);
bool axiom_workspace_is_visible_not_covered(struct axiom_workspace *workspace);
bool axiom_workspace_has_urgent_window(struct axiom_workspace *workspace);
int axiom_workspace_get_window_count(struct axiom_workspace *workspace,
                                    bool only_tiled, bool only_visible);
struct axiom_window *axiom_workspace_get_first_window(struct axiom_workspace *workspace);
struct axiom_window *axiom_workspace_get_top_left_window(struct axiom_workspace *workspace);
struct axiom_window *axiom_workspace_get_fullscreen_window(struct axiom_workspace *workspace);

// Workspace naming and identification
void axiom_workspace_rename(struct axiom_workspace *workspace, const char *new_name);
char *axiom_workspace_get_config_name(struct axiom_workspace *workspace);
bool axiom_workspace_matches_static_selector(struct axiom_workspace *workspace, const char *selector);

// Workspace state management
void axiom_workspace_set_persistent(struct axiom_workspace *workspace, bool persistent);
bool axiom_workspace_is_persistent(struct axiom_workspace *workspace);
void axiom_workspace_start_animation(struct axiom_workspace *workspace, bool in, bool left, bool instant);

// Special workspace management
struct axiom_workspace *axiom_workspace_manager_create_special_workspace(struct axiom_workspace_manager *manager,
                                                                        const char *name,
                                                                        struct axiom_monitor *monitor);
void axiom_workspace_manager_toggle_special_workspace(struct axiom_workspace_manager *manager,
                                                     const char *name);
void axiom_workspace_manager_close_special_workspace(struct axiom_workspace_manager *manager,
                                                    struct axiom_workspace *workspace);

// Workspace rules management
void axiom_workspace_manager_add_rule(struct axiom_workspace_manager *manager,
                                     struct axiom_workspace_rule *rule);
void axiom_workspace_manager_remove_rule(struct axiom_workspace_manager *manager,
                                        struct axiom_workspace_rule *rule);
void axiom_workspace_manager_clear_rules(struct axiom_workspace_manager *manager);
struct axiom_workspace_rule *axiom_workspace_manager_find_rule(struct axiom_workspace_manager *manager,
                                                              struct axiom_workspace *workspace);

// Workspace auto-management
void axiom_workspace_manager_cleanup_empty_workspaces(struct axiom_workspace_manager *manager);
struct axiom_workspace *axiom_workspace_manager_ensure_workspace_exists(struct axiom_workspace_manager *manager,
                                                                       int id,
                                                                       struct axiom_monitor *monitor);
int axiom_workspace_manager_get_next_available_id(struct axiom_workspace_manager *manager);

// Workspace persistence
void axiom_workspace_manager_save_workspace_state(struct axiom_workspace_manager *manager,
                                                 struct axiom_workspace *workspace);
void axiom_workspace_manager_restore_workspace_state(struct axiom_workspace_manager *manager,
                                                    struct axiom_workspace *workspace);
void axiom_workspace_manager_ensure_persistent_workspaces(struct axiom_workspace_manager *manager);

// Focus history management
void axiom_workspace_remember_previous_workspace(struct axiom_workspace *workspace,
                                                struct axiom_workspace *previous);
struct axiom_workspace *axiom_workspace_get_previous_workspace(struct axiom_workspace *workspace);

// Workspace configuration
void axiom_workspace_manager_set_workspace_names(struct axiom_workspace_manager *manager,
                                                char **names, int count);
char **axiom_workspace_manager_get_workspace_names(struct axiom_workspace_manager *manager, int *count);
void axiom_workspace_manager_set_max_workspaces(struct axiom_workspace_manager *manager, int max);

// Event handlers
void axiom_workspace_manager_handle_window_focus_change(struct wl_listener *listener, void *data);
void axiom_workspace_manager_handle_monitor_change(struct wl_listener *listener, void *data);

// Helper functions
struct axiom_workspace_rule *axiom_workspace_rule_create(void);
void axiom_workspace_rule_destroy(struct axiom_workspace_rule *rule);
bool axiom_workspace_rule_matches(struct axiom_workspace_rule *rule, struct axiom_workspace *workspace);

// Debug and monitoring
void axiom_workspace_print_info(struct axiom_workspace *workspace);
void axiom_workspace_manager_print_all_workspaces(struct axiom_workspace_manager *manager);
void axiom_workspace_manager_validate_state(struct axiom_workspace_manager *manager);

// Workspace utilities
bool axiom_workspace_is_empty(struct axiom_workspace *workspace);
bool axiom_workspace_is_special(struct axiom_workspace *workspace);
bool axiom_workspace_has_windows(struct axiom_workspace *workspace);
int axiom_workspace_get_monitor_id(struct axiom_workspace *workspace);

#endif /* AXIOM_WORKSPACE_MANAGER_H */
