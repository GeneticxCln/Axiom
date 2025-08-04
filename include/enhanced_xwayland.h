#ifndef AXIOM_ENHANCED_XWAYLAND_H
#define AXIOM_ENHANCED_XWAYLAND_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-server-core.h>
#include <wlr/xwayland.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>

struct axiom_server;
struct axiom_window;

// XWayland surface states - inspired by Hyprland's approach
enum axiom_xwayland_surface_state {
    AXIOM_XWAYLAND_SURFACE_UNMAPPED = 0,
    AXIOM_XWAYLAND_SURFACE_UNMANAGED,  // Override-redirect windows
    AXIOM_XWAYLAND_SURFACE_MAPPED,
    AXIOM_XWAYLAND_SURFACE_REQUEST_CONFIGURE,
    AXIOM_XWAYLAND_SURFACE_REQUEST_FULLSCREEN,
    AXIOM_XWAYLAND_SURFACE_REQUEST_MINIMIZE,
    AXIOM_XWAYLAND_SURFACE_REQUEST_MAXIMIZE,
    AXIOM_XWAYLAND_SURFACE_REQUEST_ACTIVATE
};

// XWayland-specific window properties
struct axiom_xwayland_surface_props {
    // ICCCM properties
    char *title;
    char *class;
    char *instance;
    char *startup_id;
    
    // Window manager hints
    struct {
        bool input_hint;
        bool state_hint;
        bool urgent_hint;
        bool group_hint;
        int initial_state;
        int icon_pixmap;
        int icon_window;
        int icon_x, icon_y;
        int icon_mask;
        int window_group;
    } wm_hints;
    
    // Size hints
    struct {
        bool has_min_size;
        bool has_max_size;
        bool has_base_size;
        bool has_resize_increments;
        bool has_aspect_ratio;
        bool has_win_gravity;
        
        int min_width, min_height;
        int max_width, max_height;
        int base_width, base_height;
        int width_inc, height_inc;
        int min_aspect_num, min_aspect_den;
        int max_aspect_num, max_aspect_den;
        int win_gravity;
    } size_hints;
    
    // NET_WM properties
    struct {
        uint32_t *window_types;
        size_t window_types_len;
        uint32_t *states;
        size_t states_len;
        char *desktop_name;
        uint32_t desktop;
        bool above;
        bool below;
        bool fullscreen;
        bool maximized_horz;
        bool maximized_vert;
        bool minimized;
        bool skip_taskbar;
        bool skip_pager;
        bool demands_attention;
    } net_wm;
    
    // Motif window manager hints
    struct {
        bool has_decorations;
        bool has_functions;
        bool has_input_mode;
    } motif;
};

// XWayland surface wrapper - integrates with your window system
struct axiom_xwayland_surface {
    struct wlr_xwayland_surface *wlr_surface;
    struct axiom_window *window;  // Associated window
    struct axiom_server *server;
    
    enum axiom_xwayland_surface_state state;
    struct axiom_xwayland_surface_props props;
    
    // Scene integration
    struct wlr_scene_tree *scene_tree;
    struct wlr_scene_rect *debug_border;  // For debugging override-redirect
    
    // Geometry management
    bool has_geometry;
    struct wlr_box geometry;
    struct wlr_box pending_geometry;
    
    // Override-redirect handling
    bool override_redirect;
    bool override_redirect_wants_focus;
    
    // Event listeners
    struct wl_listener associate;
    struct wl_listener dissociate; 
    struct wl_listener destroy;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener configure;
    struct wl_listener set_title;
    struct wl_listener set_class;
    struct wl_listener set_role;
    struct wl_listener set_startup_id;
    struct wl_listener set_window_type;
    struct wl_listener set_hints;
    struct wl_listener set_decorations;
    struct wl_listener set_override_redirect;
    struct wl_listener request_configure;
    struct wl_listener request_fullscreen;
    struct wl_listener request_minimize;
    struct wl_listener request_maximize;
    struct wl_listener request_activate;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener surface_commit;
    
    struct wl_list link;  // Link in xwayland_manager->surfaces
};

// Enhanced XWayland manager - based on Hyprland's approach
struct axiom_xwayland_manager {
    struct axiom_server *server;
    struct wlr_xwayland *wlr_xwayland;
    
    // Surface tracking
    struct wl_list surfaces;  // axiom_xwayland_surface::link
    struct wl_list unmanaged_surfaces;  // Override-redirect surfaces
    
    // Server state
    bool server_ready;
    char *display_name;
    int display_fd;
    
    // Configuration
    bool lazy_start;  // Start XWayland on demand
    bool enable_wm;   // Enable window management
    bool scale_override_redirect;  // Scale override-redirect windows
    
    // Focus tracking
    struct axiom_xwayland_surface *focused_surface;
    
    // Event listeners  
    struct wl_listener new_surface;
    struct wl_listener ready;
    struct wl_listener remove;
    
    // Override-redirect management
    struct {
        bool enable_management;
        int layer;  // Scene layer for override-redirect
        bool focus_follows_mouse;
        bool auto_raise;
    } override_redirect;
};

// Core XWayland manager functions
struct axiom_xwayland_manager *axiom_xwayland_manager_create(struct axiom_server *server);
void axiom_xwayland_manager_destroy(struct axiom_xwayland_manager *manager);

// Enhanced XWayland manager functions
struct axiom_xwayland_manager *axiom_enhanced_xwayland_manager_create(struct axiom_server *server);
void axiom_enhanced_xwayland_manager_destroy(struct axiom_xwayland_manager *manager);
bool axiom_xwayland_manager_start(struct axiom_xwayland_manager *manager);
void axiom_xwayland_manager_stop(struct axiom_xwayland_manager *manager);

// Surface management
struct axiom_xwayland_surface *axiom_xwayland_surface_create(
    struct axiom_xwayland_manager *manager, 
    struct wlr_xwayland_surface *wlr_surface);
void axiom_xwayland_surface_destroy(struct axiom_xwayland_surface *surface);
void axiom_xwayland_surface_map(struct axiom_xwayland_surface *surface);
void axiom_xwayland_surface_unmap(struct axiom_xwayland_surface *surface);

// Window integration
void axiom_xwayland_surface_associate_window(
    struct axiom_xwayland_surface *surface,
    struct axiom_window *window);
void axiom_xwayland_surface_dissociate_window(struct axiom_xwayland_surface *surface);

// Property management
void axiom_xwayland_surface_update_properties(struct axiom_xwayland_surface *surface);
void axiom_xwayland_surface_handle_set_title(struct axiom_xwayland_surface *surface);
void axiom_xwayland_surface_handle_set_class(struct axiom_xwayland_surface *surface);
void axiom_xwayland_surface_handle_set_hints(struct axiom_xwayland_surface *surface);

// Window management integration
bool axiom_xwayland_surface_should_manage(struct axiom_xwayland_surface *surface);
bool axiom_xwayland_surface_wants_focus(struct axiom_xwayland_surface *surface);
void axiom_xwayland_surface_set_focus(struct axiom_xwayland_surface *surface, bool focused);
void axiom_xwayland_surface_configure(struct axiom_xwayland_surface *surface,
                                     int x, int y, int width, int height);

// Override-redirect management
void axiom_xwayland_manager_handle_override_redirect(
    struct axiom_xwayland_manager *manager,
    struct axiom_xwayland_surface *surface);
void axiom_xwayland_override_redirect_focus(struct axiom_xwayland_surface *surface);
void axiom_xwayland_override_redirect_restack(struct axiom_xwayland_surface *surface);

// Utility functions
struct axiom_xwayland_surface *axiom_xwayland_surface_from_wlr_surface(
    struct axiom_xwayland_manager *manager,
    struct wlr_surface *wlr_surface);
const char *axiom_xwayland_surface_get_string_prop(
    struct axiom_xwayland_surface *surface,
    const char *property_name);
bool axiom_xwayland_surface_is_unmanaged(struct axiom_xwayland_surface *surface);

// Integration with window rules
bool axiom_xwayland_surface_matches_rule(struct axiom_xwayland_surface *surface,
                                        const char *app_id, const char *title);

// Debug and diagnostics
void axiom_xwayland_surface_debug_properties(struct axiom_xwayland_surface *surface);
void axiom_xwayland_manager_debug_state(struct axiom_xwayland_manager *manager);

// Phase 1.3: Advanced Features
void axiom_enhanced_xwayland_handle_override_redirect(struct axiom_xwayland_manager *manager,
                                                     struct wlr_xwayland_surface *wlr_surface,
                                                     struct axiom_xwayland_surface *surface);
void axiom_enhanced_xwayland_handle_managed_window(struct axiom_xwayland_manager *manager,
                                                   struct wlr_xwayland_surface *wlr_surface,
                                                   struct axiom_xwayland_surface *surface);
void axiom_enhanced_xwayland_focus_management(struct axiom_xwayland_manager *manager,
                                             struct axiom_xwayland_surface *surface,
                                             bool should_focus);
void axiom_enhanced_xwayland_scene_integration(struct axiom_xwayland_manager *manager,
                                              struct axiom_xwayland_surface *surface);
void axiom_enhanced_xwayland_stacking_management(struct axiom_xwayland_manager *manager);
void axiom_enhanced_xwayland_log_properties(struct wlr_xwayland_surface *wlr_surface);
bool axiom_enhanced_xwayland_surface_should_manage(struct wlr_xwayland_surface *wlr_surface);

#endif /* AXIOM_ENHANCED_XWAYLAND_H */
