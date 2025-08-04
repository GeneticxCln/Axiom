#include "xwayland.h"  // Use existing xwayland definitions
#include "logging.h"
#include "axiom.h"
#include <wlr/xwayland.h>
#include <stdlib.h>
#include <string.h>

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
    if (manager->wlr_xwayland)
        wlr_xwayland_destroy(manager->wlr_xwayland);

    free(manager);
}
