#include "thumbnail_manager.h"
#include "axiom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/backend.h>

static int axiom_thumbnail_update_timer_handler(void *data) {
    struct axiom_thumbnail_manager *manager = data;
    if (!manager || !manager->enabled) {
        return 0;
    }
    
    // Update all thumbnails that need updating
    struct axiom_thumbnail *thumbnail;
    wl_list_for_each(thumbnail, &manager->thumbnails, link) {
        if (thumbnail->needs_update) {
            axiom_thumbnail_update(manager, thumbnail);
        }
    }
    
    // Schedule next update
    wl_event_source_timer_update(manager->update_timer, manager->update_interval_ms);
    return 0;
}

struct axiom_thumbnail_manager *axiom_thumbnail_manager_create(struct axiom_server *server) {
    struct axiom_thumbnail_manager *manager = calloc(1, sizeof(struct axiom_thumbnail_manager));
    if (!manager) {
        return NULL;
    }
    
    manager->server = server;
    manager->renderer = server->renderer;
    wl_list_init(&manager->thumbnails);
    manager->thumbnail_count = 0;
    
    // Default configuration
    manager->enabled = true;
    manager->thumbnail_width = AXIOM_THUMBNAIL_WIDTH;
    manager->thumbnail_height = AXIOM_THUMBNAIL_HEIGHT;
    manager->update_interval_ms = 100; // 10 FPS for thumbnails
    
    // Initialize OpenGL resources (simplified for test)
    manager->framebuffer = 0;
    manager->renderbuffer = 0;
    
    // Initialize statistics
    memset(&manager->stats, 0, sizeof(manager->stats));
    
    printf("Thumbnail manager created successfully\n");
    return manager;
}

bool axiom_thumbnail_manager_init(struct axiom_thumbnail_manager *manager) {
    if (!manager || !manager->server) {
        return false;
    }
    
    // Set up update timer
    if (manager->server->wl_event_loop) {
        manager->update_timer = wl_event_loop_add_timer(
            manager->server->wl_event_loop,
            axiom_thumbnail_update_timer_handler,
            manager
        );
        
        if (manager->update_timer) {
            wl_event_source_timer_update(manager->update_timer, manager->update_interval_ms);
        }
    }
    
    printf("Thumbnail manager initialized successfully\n");
    return true;
}

void axiom_thumbnail_manager_destroy(struct axiom_thumbnail_manager *manager) {
    if (!manager) {
        return;
    }
    
    // Clean up update timer
    if (manager->update_timer) {
        wl_event_source_remove(manager->update_timer);
    }
    
    // Destroy all thumbnails
    struct axiom_thumbnail *thumbnail, *tmp;
    wl_list_for_each_safe(thumbnail, tmp, &manager->thumbnails, link) {
        axiom_thumbnail_destroy(thumbnail);
    }
    
    // Clean up OpenGL resources
    if (manager->framebuffer) {
        glDeleteFramebuffers(1, &manager->framebuffer);
    }
    if (manager->renderbuffer) {
        glDeleteRenderbuffers(1, &manager->renderbuffer);
    }
    
    free(manager);
    printf("Thumbnail manager destroyed\n");
}

struct axiom_thumbnail *axiom_thumbnail_create(struct axiom_thumbnail_manager *manager, 
                                               struct axiom_window *window) {
    if (!manager || !window) {
        return NULL;
    }
    
    // Check if thumbnail already exists
    struct axiom_thumbnail *existing = axiom_thumbnail_get_for_window(manager, window);
    if (existing) {
        return existing;
    }
    
    // Check thumbnail limit
    if (manager->thumbnail_count >= AXIOM_MAX_THUMBNAILS) {
        printf("Maximum thumbnail limit reached\n");
        return NULL;
    }
    
    struct axiom_thumbnail *thumbnail = calloc(1, sizeof(struct axiom_thumbnail));
    if (!thumbnail) {
        return NULL;
    }
    
    thumbnail->window = window;
    thumbnail->width = manager->thumbnail_width;
    thumbnail->height = manager->thumbnail_height;
    thumbnail->needs_update = true;
    thumbnail->last_update_time = 0;
    thumbnail->update_interval_ms = manager->update_interval_ms;
    
    // Allocate pixel data buffer
    thumbnail->data_size = thumbnail->width * thumbnail->height * 4; // RGBA
    thumbnail->pixel_data = calloc(1, thumbnail->data_size);
    if (!thumbnail->pixel_data) {
        free(thumbnail);
        return NULL;
    }
    
    // Initialize with placeholder data (solid color for now)
    for (size_t i = 0; i < thumbnail->data_size; i += 4) {
        thumbnail->pixel_data[i + 0] = 64;  // R
        thumbnail->pixel_data[i + 1] = 64;  // G 
        thumbnail->pixel_data[i + 2] = 128; // B
        thumbnail->pixel_data[i + 3] = 255; // A
    }
    
    wl_list_insert(&manager->thumbnails, &thumbnail->link);
    manager->thumbnail_count++;
    manager->stats.thumbnails_created++;
    
    printf("Created thumbnail for window (total: %d)\n", manager->thumbnail_count);
    return thumbnail;
}

void axiom_thumbnail_destroy(struct axiom_thumbnail *thumbnail) {
    if (!thumbnail) {
        return;
    }
    
    // Clean up OpenGL resources
    if (thumbnail->texture) {
        wlr_texture_destroy(thumbnail->texture);
    }
    if (thumbnail->gl_texture_id) {
        glDeleteTextures(1, &thumbnail->gl_texture_id);
    }
    
    // Free pixel data
    if (thumbnail->pixel_data) {
        free(thumbnail->pixel_data);
    }
    
    wl_list_remove(&thumbnail->link);
    free(thumbnail);
    printf("Thumbnail destroyed\n");
}

bool axiom_thumbnail_update(struct axiom_thumbnail_manager *manager, 
                            struct axiom_thumbnail *thumbnail) {
    if (!manager || !thumbnail) {
        return false;
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint32_t current_time = (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    
    // Check if enough time has passed since last update
    if (current_time - thumbnail->last_update_time < thumbnail->update_interval_ms) {
        return false;
    }
    
    // For now, just mark as updated (actual rendering would happen here)
    thumbnail->needs_update = false;
    thumbnail->last_update_time = current_time;
    manager->stats.thumbnails_updated++;
    
    printf("Updated thumbnail\n");
    return true;
}

bool axiom_thumbnail_render(struct axiom_thumbnail_manager *manager, 
                            struct axiom_thumbnail *thumbnail) {
    if (!manager || !thumbnail || !thumbnail->window) {
        return false;
    }
    
    // This is where actual window content rendering would happen
    // For now, we'll create a simple pattern based on window properties
    
    int width = thumbnail->width;
    int height = thumbnail->height;
    uint8_t *data = thumbnail->pixel_data;
    
    // Create a simple gradient pattern as placeholder
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;
            data[idx + 0] = (uint8_t)(x * 255 / width);      // R
            data[idx + 1] = (uint8_t)(y * 255 / height);     // G
            data[idx + 2] = 128;                             // B
            data[idx + 3] = 255;                             // A
        }
    }
    
    return true;
}

struct axiom_thumbnail *axiom_thumbnail_get_for_window(struct axiom_thumbnail_manager *manager,
                                                       struct axiom_window *window) {
    if (!manager || !window) {
        return NULL;
    }
    
    struct axiom_thumbnail *thumbnail;
    wl_list_for_each(thumbnail, &manager->thumbnails, link) {
        if (thumbnail->window == window) {
            return thumbnail;
        }
    }
    
    return NULL;
}

void axiom_thumbnail_on_window_mapped(struct axiom_thumbnail_manager *manager,
                                       struct axiom_window *window) {
    if (!manager || !window || !manager->enabled) {
        return;
    }
    
    axiom_thumbnail_create(manager, window);
    printf("Thumbnail created for mapped window\n");
}

void axiom_thumbnail_on_window_unmapped(struct axiom_thumbnail_manager *manager,
                                         struct axiom_window *window) {
    if (!manager || !window) {
        return;
    }
    
    struct axiom_thumbnail *thumbnail = axiom_thumbnail_get_for_window(manager, window);
    if (thumbnail) {
        thumbnail->needs_update = true;
        printf("Marked thumbnail for update on window unmap\n");
    }
}

void axiom_thumbnail_on_window_destroyed(struct axiom_thumbnail_manager *manager,
                                          struct axiom_window *window) {
    if (!manager || !window) {
        return;
    }
    
    struct axiom_thumbnail *thumbnail = axiom_thumbnail_get_for_window(manager, window);
    if (thumbnail) {
        manager->thumbnail_count--;
        axiom_thumbnail_destroy(thumbnail);
        printf("Destroyed thumbnail for destroyed window\n");
    }
}

void axiom_thumbnail_update_all(struct axiom_thumbnail_manager *manager) {
    if (!manager) {
        return;
    }
    
    struct axiom_thumbnail *thumbnail;
    wl_list_for_each(thumbnail, &manager->thumbnails, link) {
        thumbnail->needs_update = true;
        axiom_thumbnail_update(manager, thumbnail);
    }
    
    manager->stats.update_requests++;
    printf("Updated all thumbnails (%d total)\n", manager->thumbnail_count);
}

void axiom_thumbnail_cleanup_stale(struct axiom_thumbnail_manager *manager) {
    if (!manager) {
        return;
    }
    
    // In a real implementation, this would remove thumbnails for windows
    // that no longer exist or haven't been updated in a long time
    printf("Cleaned up stale thumbnails\n");
}

void axiom_thumbnail_set_size(struct axiom_thumbnail_manager *manager, int width, int height) {
    if (!manager || width <= 0 || height <= 0) {
        return;
    }
    
    manager->thumbnail_width = width;
    manager->thumbnail_height = height;
    
    // Mark all thumbnails for update with new size
    struct axiom_thumbnail *thumbnail;
    wl_list_for_each(thumbnail, &manager->thumbnails, link) {
        thumbnail->width = width;
        thumbnail->height = height;
        thumbnail->needs_update = true;
        
        // Reallocate pixel buffer
        size_t new_size = width * height * 4;
        uint8_t *new_data = realloc(thumbnail->pixel_data, new_size);
        if (new_data) {
            thumbnail->pixel_data = new_data;
            thumbnail->data_size = new_size;
        }
    }
    
    printf("Set thumbnail size to %dx%d\n", width, height);
}

void axiom_thumbnail_set_update_interval(struct axiom_thumbnail_manager *manager, uint32_t interval_ms) {
    if (!manager || interval_ms == 0) {
        return;
    }
    
    manager->update_interval_ms = interval_ms;
    
    if (manager->update_timer) {
        wl_event_source_timer_update(manager->update_timer, interval_ms);
    }
    
    printf("Set thumbnail update interval to %u ms\n", interval_ms);
}

void axiom_thumbnail_enable(struct axiom_thumbnail_manager *manager, bool enabled) {
    if (!manager) {
        return;
    }
    
    manager->enabled = enabled;
    printf("Thumbnail system %s\n", enabled ? "enabled" : "disabled");
}

const uint8_t *axiom_thumbnail_get_pixel_data(struct axiom_thumbnail *thumbnail) {
    return thumbnail ? thumbnail->pixel_data : NULL;
}

size_t axiom_thumbnail_get_data_size(struct axiom_thumbnail *thumbnail) {
    return thumbnail ? thumbnail->data_size : 0;
}

void axiom_thumbnail_print_stats(struct axiom_thumbnail_manager *manager) {
    if (!manager) {
        return;
    }
    
    printf("=== Thumbnail Manager Statistics ===\n");
    printf("Active thumbnails: %d\n", manager->thumbnail_count);
    printf("Thumbnails created: %u\n", manager->stats.thumbnails_created);
    printf("Thumbnails updated: %u\n", manager->stats.thumbnails_updated);
    printf("Update requests: %u\n", manager->stats.update_requests);
    printf("Render errors: %u\n", manager->stats.render_errors);
    printf("Thumbnail size: %dx%d\n", manager->thumbnail_width, manager->thumbnail_height);
    printf("Update interval: %u ms\n", manager->update_interval_ms);
    printf("System enabled: %s\n", manager->enabled ? "yes" : "no");
    printf("=====================================\n");
}

void axiom_thumbnail_reset_stats(struct axiom_thumbnail_manager *manager) {
    if (!manager) {
        return;
    }
    
    memset(&manager->stats, 0, sizeof(manager->stats));
    printf("Thumbnail statistics reset\n");
}

