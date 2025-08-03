#include "screenshot.h"
#include "axiom.h"
#include "logging.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/**
 * Create and initialize the screenshot manager.
 */
struct axiom_screenshot_manager *axiom_screenshot_manager_create(struct axiom_server *server) {
    struct axiom_screenshot_manager *manager = axiom_calloc_tracked(1, 
        sizeof(struct axiom_screenshot_manager), AXIOM_MEM_TYPE_SCREENSHOT,
        __FILE__, __func__, __LINE__);
    if (!manager) {
        AXIOM_LOG_ERROR("SCREENSHOT", "Failed to allocate screenshot manager");
        return NULL;
    }
    
    manager->server = server;
    return manager;
}

/**
 * Initialize the screenshot manager.
 */
bool axiom_screenshot_manager_init(struct axiom_screenshot_manager *manager) {
    if (!manager || !manager->server) {
        AXIOM_LOG_ERROR("SCREENSHOT", "Invalid manager or server");
        return false;
    }
    
    // Initialize screencopy protocol
    manager->screencopy_manager = wlr_screencopy_manager_v1_create(manager->server->wl_display);
    if (!manager->screencopy_manager) {
        AXIOM_LOG_ERROR("SCREENSHOT", "Failed to create screencopy manager");
        return false;
    }
    
    // Note: In newer wlroots, screencopy events are handled differently
    // The manager will automatically handle frame creation
    
    // Initialize export dmabuf protocol
    manager->export_dmabuf_manager = wlr_export_dmabuf_manager_v1_create(manager->server->wl_display);
    if (!manager->export_dmabuf_manager) {
        AXIOM_LOG_WARN("SCREENSHOT", "Failed to create export dmabuf manager (non-critical)");
    } else {
        // Note: In newer wlroots, dmabuf export events are handled differently
        // The manager will automatically handle frame creation
    }
    
    AXIOM_LOG_INFO("SCREENSHOT", "Screenshot protocols initialized successfully");
    return true;
}

/**
 * Destroy the screenshot manager.
 */
void axiom_screenshot_manager_destroy(struct axiom_screenshot_manager *manager) {
    if (!manager) {
        return;
    }
    
    if (manager->screencopy_manager) {
        // Destroyed automatically with display
    }
    
    if (manager->export_dmabuf_manager) {
        // Destroyed automatically with display
    }
    
    axiom_free_tracked(manager, __FILE__, __func__, __LINE__);
}

/**
 * Create a new screenshot frame wrapper.
 */
struct axiom_screenshot_frame *axiom_screenshot_frame_create(
    struct wlr_screencopy_frame_v1 *wlr_frame, struct axiom_server *server) {
    
    struct axiom_screenshot_frame *frame = axiom_calloc_tracked(1, 
        sizeof(struct axiom_screenshot_frame), AXIOM_MEM_TYPE_SCREENSHOT,
        __FILE__, __func__, __LINE__);
    if (!frame) {
        AXIOM_LOG_ERROR("SCREENSHOT", "Failed to allocate screenshot frame");
        return NULL;
    }
    
    frame->wlr_frame = wlr_frame;
    frame->server = server;
    
    // Note: In newer wlroots, screencopy frame events are handled differently
    // The wlr_screencopy implementation manages these events internally
    
    AXIOM_LOG_DEBUG("SCREENSHOT", "Created screenshot frame");
    return frame;
}

/**
 * Destroy a screenshot frame wrapper.
 */
void axiom_screenshot_frame_destroy(struct axiom_screenshot_frame *frame) {
    if (!frame) return;
    
    // Note: Event listeners are managed by wlr_screencopy in newer versions
    
    axiom_free_tracked(frame, __FILE__, __func__, __LINE__);
}

// ===== Event Handlers =====

void axiom_screenshot_handle_new_screencopy_frame(struct wl_listener *listener, void *data) {
    struct axiom_screenshot_manager *manager = wl_container_of(listener, manager, new_screencopy_frame);
    struct wlr_screencopy_frame_v1 *wlr_frame = data;
    
    AXIOM_LOG_DEBUG("SCREENSHOT", "New screencopy frame requested");
    
    // Create frame wrapper
    struct axiom_screenshot_frame *frame = axiom_screenshot_frame_create(wlr_frame, manager->server);
    if (!frame) {
        AXIOM_LOG_ERROR("SCREENSHOT", "Failed to create screenshot frame wrapper");
        return;
    }
    
    // Store reference for later retrieval
    wlr_frame->data = frame;
}

void axiom_screenshot_handle_new_dmabuf_frame(struct wl_listener *listener, void *data) {
    struct axiom_screenshot_manager *manager = wl_container_of(listener, manager, new_dmabuf_frame);
    (void)data;
    
    AXIOM_LOG_DEBUG("SCREENSHOT", "New dmabuf export frame requested");
    // Basic dmabuf handling - in a full implementation you'd track these frames too
}

void axiom_screenshot_frame_handle_destroy(struct wl_listener *listener, void *data) {
    struct axiom_screenshot_frame *frame = wl_container_of(listener, frame, destroy);
    (void)data;
    
    AXIOM_LOG_DEBUG("SCREENSHOT", "Screenshot frame destroyed");
    axiom_screenshot_frame_destroy(frame);
}

void axiom_screenshot_frame_handle_ready(struct wl_listener *listener, void *data) {
    struct axiom_screenshot_frame *frame = wl_container_of(listener, frame, ready);
    (void)data;
    
    AXIOM_LOG_INFO("SCREENSHOT", "Screenshot frame ready for capture");
    
    // At this point, the client can copy the buffer
    // The wlr_screencopy implementation handles the actual data transfer
}

void axiom_screenshot_frame_handle_failed(struct wl_listener *listener, void *data) {
    struct axiom_screenshot_frame *frame = wl_container_of(listener, frame, failed);
    (void)data;
    
    AXIOM_LOG_WARN("SCREENSHOT", "Screenshot frame capture failed");
}

// ===== Utility Functions =====

/**
 * Take a screenshot of the specified output.
 */
bool axiom_take_screenshot(struct axiom_server *server, const char *output_name, const char *filename) {
    if (!server || !server->screenshot_manager) {
        AXIOM_LOG_ERROR("SCREENSHOT", "Screenshot manager not initialized");
        return false;
    }
    
    struct axiom_output *target_output = NULL;
    
    // Find the target output
    if (output_name) {
        struct axiom_output *output;
        wl_list_for_each(output, &server->outputs, link) {
            if (strcmp(output->wlr_output->name, output_name) == 0) {
                target_output = output;
                break;
            }
        }
        
        if (!target_output) {
            AXIOM_LOG_ERROR("SCREENSHOT", "Output '%s' not found", output_name);
            return false;
        }
    } else {
        // Use the first available output
        if (!wl_list_empty(&server->outputs)) {
            target_output = wl_container_of(server->outputs.next, target_output, link);
        }
    }
    
    if (!target_output) {
        AXIOM_LOG_ERROR("SCREENSHOT", "No output available for screenshot");
        return false;
    }
    
    // Generate filename if not provided
    char default_filename[256];
    if (!filename) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(default_filename, sizeof(default_filename), "axiom-screenshot-%Y%m%d-%H%M%S.png", tm_info);
        filename = default_filename;
    }
    
    AXIOM_LOG_INFO("SCREENSHOT", "Taking screenshot of output '%s' -> '%s'", 
                   target_output->wlr_output->name, filename);
    
    // In a complete implementation, you would:
    // 1. Create a screencopy frame
    // 2. Set up the buffer
    // 3. Capture the frame data
    // 4. Save to file using a library like libpng
    
    // For now, we'll use grim as an external tool
    char command[512];
    if (output_name) {
        snprintf(command, sizeof(command), "grim -o '%s' '%s'", output_name, filename);
    } else {
        snprintf(command, sizeof(command), "grim '%s'", filename);
    }
    
    int result = system(command);
    if (result == 0) {
        AXIOM_LOG_INFO("SCREENSHOT", "Screenshot saved successfully: %s", filename);
        return true;
    } else {
        AXIOM_LOG_ERROR("SCREENSHOT", "Failed to take screenshot (grim not available?)");
        return false;
    }
}

/**
 * Take a screenshot of a specific region.
 */
bool axiom_take_region_screenshot(struct axiom_server *server, int x, int y, int width, int height, const char *filename) {
    if (!server || !server->screenshot_manager) {
        AXIOM_LOG_ERROR("SCREENSHOT", "Screenshot manager not initialized");
        return false;
    }
    
    // Generate filename if not provided
    char default_filename[256];
    if (!filename) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(default_filename, sizeof(default_filename), "axiom-region-%Y%m%d-%H%M%S.png", tm_info);
        filename = default_filename;
    }
    
    AXIOM_LOG_INFO("SCREENSHOT", "Taking region screenshot (%d,%d %dx%d) -> '%s'", 
                   x, y, width, height, filename);
    
    // Use grim for region capture
    char command[512];
    snprintf(command, sizeof(command), "grim -g '%d,%d %dx%d' '%s'", x, y, width, height, filename);
    
    int result = system(command);
    if (result == 0) {
        AXIOM_LOG_INFO("SCREENSHOT", "Region screenshot saved successfully: %s", filename);
        return true;
    } else {
        AXIOM_LOG_ERROR("SCREENSHOT", "Failed to take region screenshot (grim not available?)");
        return false;
    }
}
