#ifndef AXIOM_MONITOR_MANAGER_H
#define AXIOM_MONITOR_MANAGER_H

#include <wayland-server-core.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <stdbool.h>
#include <stdint.h>

// Forward declarations
struct axiom_server;
struct axiom_workspace;
struct axiom_advanced_tiling;

// Monitor auto-direction types (inspired by Hyprland)
enum axiom_auto_dir {
    AXIOM_DIR_AUTO_NONE = 0,
    AXIOM_DIR_AUTO_UP,
    AXIOM_DIR_AUTO_DOWN,
    AXIOM_DIR_AUTO_LEFT,
    AXIOM_DIR_AUTO_RIGHT,
    AXIOM_DIR_AUTO_CENTER_UP,
    AXIOM_DIR_AUTO_CENTER_DOWN,
    AXIOM_DIR_AUTO_CENTER_LEFT,
    AXIOM_DIR_AUTO_CENTER_RIGHT
};

// Monitor configuration rule
struct axiom_monitor_rule {
    enum axiom_auto_dir auto_dir;
    char *name;
    int width, height;
    int x, y;                    // Position offset
    float scale;
    float refresh_rate;
    bool disabled;
    enum wl_output_transform transform;
    char *mirror_of;             // Monitor to mirror
    bool enable_10bit;
    bool enable_vrr;             // Variable refresh rate
    bool primary;                // Primary monitor flag
};

// Monitor state management
struct axiom_monitor_state {
    struct axiom_monitor *owner;
    bool needs_commit;
    bool test_only;
    bool buffer_present;
};

// Enhanced monitor structure (inspired by Hyprland's CMonitor)
struct axiom_monitor {
    struct wl_list link;
    struct axiom_server *server;
    
    // Core wlroots objects
    struct wlr_output *wlr_output;
    struct wlr_scene_output *scene_output;
    
    // Monitor identification
    uint32_t id;
    char *name;
    char *description;
    char *manufacturer;
    char *model;
    
    // Geometry and positioning
    int x, y;                    // Position in layout
    int width, height;           // Current resolution
    int phys_width, phys_height; // Physical dimensions in mm
    float scale;                 // Current scale factor
    float refresh_rate;          // Current refresh rate
    enum wl_output_transform transform;
    
    // Auto-positioning
    enum axiom_auto_dir auto_dir;
    
    // State management
    struct axiom_monitor_state state;
    bool enabled;
    bool connected;
    bool dpms_enabled;
    bool vrr_active;
    bool tearing_capable;
    bool is_primary;
    bool is_fallback;            // Fallback monitor for unsafe state
    
    // Workspace management
    struct axiom_workspace *active_workspace;
    struct axiom_workspace *active_special_workspace;
    struct wl_list workspaces;           // List of workspaces for this monitor
    struct wl_list special_workspaces;   // List of special workspaces
    int workspace_count;
    int max_workspaces;                  // Maximum workspaces per monitor
    
    // Per-monitor tiling engine
    struct axiom_advanced_tiling *tiling_engine;
    bool tiling_enabled;
    struct {
        float master_ratio;
        int master_count;
        int gap_size;
        enum axiom_tiling_mode current_mode;
        bool smart_gaps;                 // Hide gaps when only one window
        bool smart_borders;              // Hide borders when only one window
    } tiling_config;
    
    // Configuration
    struct axiom_monitor_rule active_rule;
    
    // Mirroring support
    struct axiom_monitor *mirror_of;
    struct wl_list mirrors;      // List of monitors mirroring this one
    
    // Reserved areas (for panels, bars, etc.)
    struct {
        int top, bottom, left, right;
    } reserved_area;
    
    // Animation and effects
    float special_workspace_fade;
    bool force_full_frames;
    bool schedule_frame_pending;
    
    // Performance tracking
    uint64_t last_frame_time;
    uint32_t frame_count;
    float avg_frame_time;
    
    // Event listeners
    struct wl_listener frame;
    struct wl_listener destroy;
    struct wl_listener mode;
    struct wl_listener enable;
    struct wl_listener present;
    
    // Damage tracking
    struct wlr_damage_ring damage_ring;
    bool needs_frame;
    
    // Color management
    bool supports_hdr;
    bool hdr_enabled;
    float brightness;
    float contrast;
    float gamma;
};

// Monitor manager structure
struct axiom_monitor_manager {
    struct axiom_server *server;
    
    // Monitor collections
    struct wl_list monitors;         // Active monitors
    struct wl_list disabled_monitors; // Disabled but connected monitors
    
    // Primary monitor tracking
    struct axiom_monitor *primary_monitor;
    struct axiom_monitor *last_focused_monitor;
    
    // Configuration
    struct wl_list monitor_rules;    // List of monitor rules
    bool auto_configure;
    float default_scale;
    
    // Layout management
    struct wlr_output_layout *layout;
    bool layout_changed;
    
    // Event listeners
    struct wl_listener new_output;
    struct wl_listener layout_change;
    
    // State tracking
    uint32_t next_monitor_id;
    bool in_unsafe_state;
    struct axiom_monitor *unsafe_fallback;
    
    // Performance monitoring
    uint32_t total_monitors;
    uint32_t active_monitors;
    uint64_t last_recalc_time;
};

// Monitor manager functions
struct axiom_monitor_manager *axiom_monitor_manager_create(struct axiom_server *server);
void axiom_monitor_manager_destroy(struct axiom_monitor_manager *manager);

// Monitor lifecycle
struct axiom_monitor *axiom_monitor_create(struct axiom_monitor_manager *manager, 
                                          struct wlr_output *wlr_output);
void axiom_monitor_destroy(struct axiom_monitor *monitor);
bool axiom_monitor_enable(struct axiom_monitor *monitor);
void axiom_monitor_disable(struct axiom_monitor *monitor);

// Monitor configuration
bool axiom_monitor_apply_rule(struct axiom_monitor *monitor, struct axiom_monitor_rule *rule);
void axiom_monitor_set_mode(struct axiom_monitor *monitor, int width, int height, float refresh);
void axiom_monitor_set_scale(struct axiom_monitor *monitor, float scale);
void axiom_monitor_set_transform(struct axiom_monitor *monitor, enum wl_output_transform transform);
void axiom_monitor_set_position(struct axiom_monitor *monitor, int x, int y);

// Monitor queries
struct axiom_monitor *axiom_monitor_from_name(struct axiom_monitor_manager *manager, const char *name);
struct axiom_monitor *axiom_monitor_from_id(struct axiom_monitor_manager *manager, uint32_t id);
struct axiom_monitor *axiom_monitor_from_wlr_output(struct axiom_monitor_manager *manager, 
                                                   struct wlr_output *wlr_output);
struct axiom_monitor *axiom_monitor_from_cursor(struct axiom_monitor_manager *manager);
struct axiom_monitor *axiom_monitor_from_point(struct axiom_monitor_manager *manager, 
                                               double x, double y);

// Monitor layout management
void axiom_monitor_manager_arrange_monitors(struct axiom_monitor_manager *manager);
void axiom_monitor_manager_recalculate_layout(struct axiom_monitor_manager *manager);
bool axiom_monitor_manager_is_point_on_monitor(struct axiom_monitor_manager *manager, 
                                               double x, double y);

// Monitor rule management
void axiom_monitor_manager_add_rule(struct axiom_monitor_manager *manager, 
                                   struct axiom_monitor_rule *rule);
void axiom_monitor_manager_remove_rule(struct axiom_monitor_manager *manager, 
                                      struct axiom_monitor_rule *rule);
void axiom_monitor_manager_clear_rules(struct axiom_monitor_manager *manager);

// Primary monitor management
void axiom_monitor_manager_set_primary(struct axiom_monitor_manager *manager, 
                                       struct axiom_monitor *monitor);
struct axiom_monitor *axiom_monitor_manager_get_primary(struct axiom_monitor_manager *manager);

// Multi-monitor operations
void axiom_monitor_manager_move_workspace_to_monitor(struct axiom_monitor_manager *manager,
                                                    struct axiom_workspace *workspace,
                                                    struct axiom_monitor *monitor);
void axiom_monitor_manager_swap_workspaces(struct axiom_monitor_manager *manager,
                                          struct axiom_monitor *monitor1,
                                          struct axiom_monitor *monitor2);

// Monitor mirroring
bool axiom_monitor_set_mirror(struct axiom_monitor *monitor, struct axiom_monitor *source);
void axiom_monitor_clear_mirror(struct axiom_monitor *monitor);
void axiom_monitor_update_mirrors(struct axiom_monitor *monitor);

// Performance and optimization
void axiom_monitor_schedule_frame(struct axiom_monitor *monitor);
void axiom_monitor_update_damage(struct axiom_monitor *monitor, struct wlr_box *damage);
void axiom_monitor_manager_optimize_layout(struct axiom_monitor_manager *manager);

// Safety and fallback
void axiom_monitor_manager_enter_unsafe_state(struct axiom_monitor_manager *manager);
void axiom_monitor_manager_leave_unsafe_state(struct axiom_monitor_manager *manager);
bool axiom_monitor_manager_create_fallback_monitor(struct axiom_monitor_manager *manager);

// Configuration helpers
struct axiom_monitor_rule *axiom_monitor_rule_create(void);
void axiom_monitor_rule_destroy(struct axiom_monitor_rule *rule);
bool axiom_monitor_rule_matches(struct axiom_monitor_rule *rule, struct axiom_monitor *monitor);

// Event handlers
void axiom_monitor_handle_frame(struct wl_listener *listener, void *data);
void axiom_monitor_handle_destroy(struct wl_listener *listener, void *data);
void axiom_monitor_handle_mode(struct wl_listener *listener, void *data);
void axiom_monitor_handle_enable(struct wl_listener *listener, void *data);
void axiom_monitor_handle_present(struct wl_listener *listener, void *data);

// Per-monitor tiling management
bool axiom_monitor_set_tiling_enabled(struct axiom_monitor *monitor, bool enabled);
void axiom_monitor_set_tiling_mode(struct axiom_monitor *monitor, enum axiom_tiling_mode mode);
void axiom_monitor_set_master_ratio(struct axiom_monitor *monitor, float ratio);
void axiom_monitor_set_master_count(struct axiom_monitor *monitor, int count);
void axiom_monitor_set_gap_size(struct axiom_monitor *monitor, int gap_size);
void axiom_monitor_cycle_tiling_mode(struct axiom_monitor *monitor);
void axiom_monitor_apply_tiling_config(struct axiom_monitor *monitor);
void axiom_monitor_refresh_tiling_layout(struct axiom_monitor *monitor);

// Monitor tiling queries
struct axiom_advanced_tiling *axiom_monitor_get_tiling_engine(struct axiom_monitor *monitor);
bool axiom_monitor_is_tiling_enabled(struct axiom_monitor *monitor);
enum axiom_tiling_mode axiom_monitor_get_tiling_mode(struct axiom_monitor *monitor);
float axiom_monitor_get_master_ratio(struct axiom_monitor *monitor);
int axiom_monitor_get_master_count(struct axiom_monitor *monitor);
int axiom_monitor_get_gap_size(struct axiom_monitor *monitor);

// Debug and monitoring
void axiom_monitor_print_info(struct axiom_monitor *monitor);
void axiom_monitor_manager_print_layout(struct axiom_monitor_manager *manager);
void axiom_monitor_manager_validate_state(struct axiom_monitor_manager *manager);

#endif /* AXIOM_MONITOR_MANAGER_H */
