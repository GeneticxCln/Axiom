#include "xwayland.h"  // Use existing xwayland definitions
#include "logging.h"
#include "axiom.h"
#include "window_manager.h"  // For window integration
#include <wlr/xwayland.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Enhanced handler for new XWayland surfaces - provides detailed logging
static void enhanced_handle_new_xwayland_surface(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_manager *manager = wl_container_of(listener, manager, new_surface);
    struct wlr_xwayland_surface *wlr_surface = data;
    
    AXIOM_LOG_INFO("Enhanced XWayland surface created: title='%s', class='%s', override_redirect=%s, x=%d, y=%d, w=%d, h=%d",
                   wlr_surface->title ? wlr_surface->title : "(no title)",
                   wlr_surface->class ? wlr_surface->class : "(no class)",
                   wlr_surface->override_redirect ? "yes" : "no",
                   wlr_surface->x, wlr_surface->y,
                   wlr_surface->width, wlr_surface->height);
    
    // Create surface wrapper using existing functionality
    struct axiom_xwayland_surface *surface = axiom_xwayland_surface_create(wlr_surface, manager->server);
    if (!surface) {
        AXIOM_LOG_ERROR("Failed to create XWayland surface wrapper");
        return;
    }
    
    // Enhanced property logging for better debugging
    if (wlr_surface->class) {
        AXIOM_LOG_DEBUG("XWayland app class: %s", wlr_surface->class);
    }
    if (wlr_surface->instance) {
        AXIOM_LOG_DEBUG("XWayland app instance: %s", wlr_surface->instance);
    }
    if (wlr_surface->startup_id) {
        AXIOM_LOG_DEBUG("XWayland startup ID: %s", wlr_surface->startup_id);
    }
    
    // Check if this is an override-redirect window (tooltip, menu, etc.)
    if (wlr_surface->override_redirect) {
        AXIOM_LOG_INFO("Override-redirect window detected - will be unmanaged");
    } else {
        AXIOM_LOG_INFO("Regular X11 window - will be managed like native Wayland");
    }
    
    AXIOM_LOG_DEBUG("Successfully created XWayland surface wrapper for %s",
                    wlr_surface->title ? wlr_surface->title : "(no title)");
}

// Enhanced XWayland ready handler
static void enhanced_handle_xwayland_ready(struct wl_listener *listener, void *data) {
    struct axiom_xwayland_manager *manager = wl_container_of(listener, manager, ready);
    (void)data;
    
    AXIOM_LOG_INFO("Enhanced XWayland server is ready on display %s", 
                   manager->wlr_xwayland->display_name);
    
    // Set DISPLAY environment variable for X11 applications
    setenv("DISPLAY", manager->wlr_xwayland->display_name, 1);
    
    AXIOM_LOG_DEBUG("Set DISPLAY environment variable to %s", 
                    manager->wlr_xwayland->display_name);
}

// Create a truly minimal enhanced XWayland manager that only adds logging
// and uses the existing structure fields defined in xwayland.h


struct axiom_xwayland_manager *axiom_enhanced_xwayland_manager_create(struct axiom_server *server) {
    struct axiom_xwayland_manager *manager = calloc(1, sizeof(struct axiom_xwayland_manager));
    if (!manager) {
        AXIOM_LOG_ERROR("Failed to allocate XWayland manager");
        return NULL;
    }
    
    manager->server = server;
    manager->enabled = true;
    manager->lazy = false;

    // Create the XWayland instance
    manager->wlr_xwayland = wlr_xwayland_create(server->wl_display, server->compositor, true);
    if (!manager->wlr_xwayland) {
        AXIOM_LOG_ERROR("Failed to create XWayland");
        free(manager);
        return NULL;
    }

    // Set up listener for new surfaces
    manager->new_surface.notify = enhanced_handle_new_xwayland_surface;
    wl_signal_add(&manager->wlr_xwayland->events.new_surface, &manager->new_surface);

    // Listener for when XWayland is ready
    manager->ready.notify = enhanced_handle_xwayland_ready;
    wl_signal_add(&manager->wlr_xwayland->events.ready, &manager->ready);
    
    AXIOM_LOG_INFO("Enhanced XWayland manager created successfully");

    return manager;
}

void axiom_enhanced_xwayland_manager_destroy(struct axiom_xwayland_manager *manager) {
    if (!manager) return;
    
    if (manager->wlr_xwayland) {
        wlr_xwayland_destroy(manager->wlr_xwayland);
    }
    
    AXIOM_LOG_INFO("Enhanced XWayland manager destroyed");
    free(manager);
}

// Enhanced surface classification for better window management
bool axiom_enhanced_xwayland_surface_should_manage(struct wlr_xwayland_surface *wlr_surface) {
    if (!wlr_surface) return false;
    
    // Don't manage override-redirect windows (tooltips, menus, etc.)
    if (wlr_surface->override_redirect) {
        AXIOM_LOG_DEBUG("Surface override_redirect=true, will not manage");
        return false;
    }
    
    // Don't manage windows that don't have a surface yet
    if (!wlr_surface->surface) {
        AXIOM_LOG_DEBUG("Surface not ready yet, deferring management");
        return false;
    }
    
    // Check for known application types that should be managed
    if (wlr_surface->class) {
        const char *class = wlr_surface->class;
        
        // File managers
        if (strcmp(class, "Thunar") == 0 || 
            strcmp(class, "thunar") == 0 ||
            strcmp(class, "Pcmanfm") == 0) {
            AXIOM_LOG_INFO("Detected file manager: %s - will manage", class);
            return true;
        }
        
        // Text editors
        if (strcmp(class, "Mousepad") == 0 || 
            strcmp(class, "mousepad") == 0 ||
            strcmp(class, "Code") == 0 ||
            strcmp(class, "code") == 0) {
            AXIOM_LOG_INFO("Detected text editor: %s - will manage", class);
            return true;
        }
        
        // Steam and games
        if (strcmp(class, "Steam") == 0 || 
            strcmp(class, "steam") == 0) {
            AXIOM_LOG_INFO("Detected Steam: %s - will manage", class);
            return true;
        }
        
        AXIOM_LOG_DEBUG("Unknown application class: %s - will manage by default", class);
    }
    
    return true;  // Manage by default
}

// Enhanced logging for X11 window properties
void axiom_enhanced_xwayland_log_properties(struct wlr_xwayland_surface *wlr_surface) {
    if (!wlr_surface) return;
    
    AXIOM_LOG_DEBUG("=== XWayland Surface Properties ===");
    AXIOM_LOG_DEBUG("Title: '%s'", wlr_surface->title ? wlr_surface->title : "(none)");
    AXIOM_LOG_DEBUG("Class: '%s'", wlr_surface->class ? wlr_surface->class : "(none)");
    AXIOM_LOG_DEBUG("Instance: '%s'", wlr_surface->instance ? wlr_surface->instance : "(none)");
    AXIOM_LOG_DEBUG("Role: '%s'", wlr_surface->role ? wlr_surface->role : "(none)");
    AXIOM_LOG_DEBUG("Startup ID: '%s'", wlr_surface->startup_id ? wlr_surface->startup_id : "(none)");
    AXIOM_LOG_DEBUG("Override Redirect: %s", wlr_surface->override_redirect ? "yes" : "no");
    AXIOM_LOG_DEBUG("Surface: %s", wlr_surface->surface ? "present" : "none");
    AXIOM_LOG_DEBUG("Position: (%d, %d)", wlr_surface->x, wlr_surface->y);
    AXIOM_LOG_DEBUG("Size: %dx%d", wlr_surface->width, wlr_surface->height);
    AXIOM_LOG_DEBUG("PID: %d", wlr_surface->pid);
    AXIOM_LOG_DEBUG("=================================");
}
