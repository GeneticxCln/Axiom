#ifndef AXIOM_ENHANCED_LAYER_SHELL_H
#define AXIOM_ENHANCED_LAYER_SHELL_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/box.h>

struct axiom_server;

// Layer shell surface states
enum axiom_layer_surface_state {
    AXIOM_LAYER_SURFACE_UNMAPPED = 0,
    AXIOM_LAYER_SURFACE_MAPPED,
    AXIOM_LAYER_SURFACE_COMMITTED,
    AXIOM_LAYER_SURFACE_CLOSED
};

// Layer types with extended functionality
enum axiom_layer_type {
    AXIOM_LAYER_BACKGROUND = 0,  // Wallpapers, desktop backgrounds
    AXIOM_LAYER_BOTTOM,          // Widgets below windows
    AXIOM_LAYER_TOP,             // Panels, bars, notifications
    AXIOM_LAYER_OVERLAY,         // Tooltips, menus, popups
    AXIOM_LAYER_COUNT
};

// Anchor edges (bitmask)
enum axiom_layer_anchor {
    AXIOM_LAYER_ANCHOR_NONE = 0,
    AXIOM_LAYER_ANCHOR_TOP = (1 << 0),
    AXIOM_LAYER_ANCHOR_BOTTOM = (1 << 1),
    AXIOM_LAYER_ANCHOR_LEFT = (1 << 2),
    AXIOM_LAYER_ANCHOR_RIGHT = (1 << 3),
    // Convenience combinations
    AXIOM_LAYER_ANCHOR_ALL = (AXIOM_LAYER_ANCHOR_TOP | AXIOM_LAYER_ANCHOR_BOTTOM | 
                              AXIOM_LAYER_ANCHOR_LEFT | AXIOM_LAYER_ANCHOR_RIGHT),
    AXIOM_LAYER_ANCHOR_HORIZONTAL = (AXIOM_LAYER_ANCHOR_LEFT | AXIOM_LAYER_ANCHOR_RIGHT),
    AXIOM_LAYER_ANCHOR_VERTICAL = (AXIOM_LAYER_ANCHOR_TOP | AXIOM_LAYER_ANCHOR_BOTTOM)
};

// Keyboard interactivity modes
enum axiom_layer_keyboard_interactivity {
    AXIOM_LAYER_KEYBOARD_INTERACTIVITY_NONE = 0,
    AXIOM_LAYER_KEYBOARD_INTERACTIVITY_EXCLUSIVE,
    AXIOM_LAYER_KEYBOARD_INTERACTIVITY_ON_DEMAND
};

// Layer surface configuration
struct axiom_layer_surface_config {
    uint32_t anchor;                    // axiom_layer_anchor bitmask
    int32_t exclusive_zone;             // Area to exclude from window layout
    struct {
        int32_t top, right, bottom, left;
    } margin;                           // Margins from screen edges
    enum axiom_layer_keyboard_interactivity keyboard_interactive;
    uint32_t desired_width, desired_height;
    enum axiom_layer_type layer;
    char *namespace;                    // Application-defined namespace
};

// Layer surface wrapper
struct axiom_layer_surface {
    struct wlr_layer_surface_v1 *wlr_layer_surface;
    struct axiom_server *server;
    
    // State tracking
    enum axiom_layer_surface_state state;
    struct axiom_layer_surface_config current_config;
    struct axiom_layer_surface_config pending_config;
    
    // Output assignment
    struct wlr_output *wlr_output;
    char *output_name;  // Requested output name
    
    // Scene integration
    struct wlr_scene_layer_surface_v1 *scene_layer_surface;
    struct wlr_scene_tree *scene_tree;
    
    // Geometry
    struct wlr_box geometry;
    bool geometry_dirty;
    
    // Animation support
    bool fade_in;
    bool slide_in;
    uint32_t animation_duration;
    
    // Focus and input
    bool focused;
    bool accepts_input;
    uint32_t last_interaction_time;
    
    // Auto-hide functionality
    bool auto_hide;
    bool hidden;
    uint32_t hide_delay_ms;
    struct wl_event_source *hide_timer;
    
    // Event listeners
    struct wl_listener destroy;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener surface_commit;
    struct wl_listener output_destroy;
    struct wl_listener new_popup;
    
    struct wl_list link;  // Link in layer_shell_manager->surfaces[layer]
};

// Per-output layer arrangement
struct axiom_layer_output {
    struct wlr_output *wlr_output;
    struct axiom_server *server;
    
    // Layer trees for each layer type
    struct wl_list layers[AXIOM_LAYER_COUNT];  // axiom_layer_surface::link
    
    // Exclusive zones (areas reserved by layer surfaces)
    struct {
        int32_t top, right, bottom, left;
    } exclusive_zones;
    
    // Usable area (screen minus exclusive zones)
    struct wlr_box usable_area;
    bool usable_area_dirty;
    
    // Animation state
    bool animating;
    uint32_t animation_start_time;
    
    struct wl_list link;  // Link in layer_shell_manager->outputs
};

// Layer shell manager
struct axiom_layer_shell_manager {
    struct axiom_server *server;
    struct wlr_layer_shell_v1 *wlr_layer_shell;
    
    // Per-output tracking
    struct wl_list outputs;  // axiom_layer_output::link
    
    // Focus tracking
    struct axiom_layer_surface *focused_surface;
    struct axiom_layer_surface *exclusive_keyboard_surface;
    
    // Configuration
    struct {
        bool enable_animations;
        uint32_t animation_duration;
        bool respect_exclusive_zones;
        bool auto_exclusive_zone_padding;
        int exclusive_zone_padding;
    } config;
    
    // Event listeners
    struct wl_listener new_surface;
    struct wl_listener output_create;
    struct wl_listener output_destroy;
    
    // Debug state
    bool debug_layer_borders;
    bool debug_exclusive_zones;
};

// Core layer shell functions
struct axiom_layer_shell_manager *axiom_layer_shell_manager_create(struct axiom_server *server);
void axiom_layer_shell_manager_destroy(struct axiom_layer_shell_manager *manager);
bool axiom_layer_shell_manager_init(struct axiom_layer_shell_manager *manager);

// Output management
struct axiom_layer_output *axiom_layer_output_create(
    struct axiom_layer_shell_manager *manager,
    struct wlr_output *output);
void axiom_layer_output_destroy(struct axiom_layer_output *layer_output);
struct axiom_layer_output *axiom_layer_output_from_wlr_output(
    struct axiom_layer_shell_manager *manager,
    struct wlr_output *output);

// Layer surface management
struct axiom_layer_surface *axiom_layer_surface_create(
    struct axiom_layer_shell_manager *manager,
    struct wlr_layer_surface_v1 *wlr_layer_surface);
void axiom_layer_surface_destroy(struct axiom_layer_surface *surface);
void axiom_layer_surface_map(struct axiom_layer_surface *surface);
void axiom_layer_surface_unmap(struct axiom_layer_surface *surface);

// Layout and positioning
void axiom_layer_shell_arrange_output(struct axiom_layer_output *layer_output);
void axiom_layer_shell_arrange_layer(struct axiom_layer_output *layer_output, 
                                     enum axiom_layer_type layer_type);
void axiom_layer_surface_configure(struct axiom_layer_surface *surface);
void axiom_layer_surface_update_geometry(struct axiom_layer_surface *surface);

// Exclusive zone management
void axiom_layer_output_update_exclusive_zones(struct axiom_layer_output *layer_output);
void axiom_layer_surface_update_exclusive_zone(struct axiom_layer_surface *surface);
struct wlr_box axiom_layer_output_get_usable_area(struct axiom_layer_output *layer_output);

// Focus and input management
void axiom_layer_shell_focus_surface(struct axiom_layer_surface *surface);
void axiom_layer_shell_unfocus_surface(struct axiom_layer_surface *surface);
bool axiom_layer_surface_accepts_input(struct axiom_layer_surface *surface);
struct axiom_layer_surface *axiom_layer_shell_surface_at(
    struct axiom_layer_shell_manager *manager,
    struct wlr_output *output,
    double ox, double oy,
    double *sx, double *sy);

// Keyboard interactivity
void axiom_layer_shell_set_exclusive_keyboard(struct axiom_layer_surface *surface);
void axiom_layer_shell_clear_exclusive_keyboard(struct axiom_layer_shell_manager *manager);
bool axiom_layer_shell_has_exclusive_keyboard(struct axiom_layer_shell_manager *manager);

// Popup support
void axiom_layer_surface_handle_new_popup(struct axiom_layer_surface *surface,
                                          struct wlr_xdg_popup *popup);

// Animation support
void axiom_layer_surface_animate_in(struct axiom_layer_surface *surface);
void axiom_layer_surface_animate_out(struct axiom_layer_surface *surface);
bool axiom_layer_surface_is_animating(struct axiom_layer_surface *surface);

// Auto-hide functionality
void axiom_layer_surface_set_auto_hide(struct axiom_layer_surface *surface, 
                                       bool enabled, uint32_t delay_ms);
void axiom_layer_surface_show(struct axiom_layer_surface *surface);
void axiom_layer_surface_hide(struct axiom_layer_surface *surface);

// Configuration and rules
void axiom_layer_shell_manager_configure(struct axiom_layer_shell_manager *manager);
bool axiom_layer_surface_matches_rule(struct axiom_layer_surface *surface,
                                     const char *namespace, const char *title);

// Utility functions
const char *axiom_layer_type_to_string(enum axiom_layer_type layer);
const char *axiom_layer_anchor_to_string(uint32_t anchor);
bool axiom_layer_surface_is_popup_parent(struct axiom_layer_surface *surface);
struct axiom_layer_surface *axiom_layer_surface_from_wlr_surface(
    struct axiom_layer_shell_manager *manager,
    struct wlr_surface *wlr_surface);

// Integration with window management
void axiom_layer_shell_notify_output_layout_change(struct axiom_layer_shell_manager *manager);
void axiom_layer_shell_handle_workspace_change(struct axiom_layer_shell_manager *manager, 
                                               int old_workspace, int new_workspace);

// Debug and diagnostics
void axiom_layer_surface_debug_info(struct axiom_layer_surface *surface);
void axiom_layer_output_debug_layout(struct axiom_layer_output *layer_output);
void axiom_layer_shell_manager_debug_state(struct axiom_layer_shell_manager *manager);

// Events that layer shell surfaces can trigger
struct axiom_layer_shell_events {
    struct wl_signal surface_mapped;     // axiom_layer_surface*
    struct wl_signal surface_unmapped;   // axiom_layer_surface*
    struct wl_signal exclusive_zone_changed;  // axiom_layer_output*
    struct wl_signal keyboard_interactive_changed;  // axiom_layer_surface*
};

#endif /* AXIOM_ENHANCED_LAYER_SHELL_H */
