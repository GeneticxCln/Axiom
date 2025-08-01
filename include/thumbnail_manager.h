#ifndef AXIOM_THUMBNAIL_MANAGER_H
#define AXIOM_THUMBNAIL_MANAGER_H

#include <wayland-server-core.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/render/wlr_texture.h>
#include <GLES3/gl3.h>

struct axiom_server;
struct axiom_window;

// Thumbnail configuration
#define AXIOM_THUMBNAIL_WIDTH 200
#define AXIOM_THUMBNAIL_HEIGHT 150
#define AXIOM_MAX_THUMBNAILS 32

// Individual window thumbnail
struct axiom_thumbnail {
    struct wl_list link;
    struct axiom_window *window;
    
    // Rendered thumbnail data
    struct wlr_texture *texture;
    GLuint gl_texture_id;
    int width, height;
    
    // Update tracking
    bool needs_update;
    uint32_t last_update_time;
    uint32_t update_interval_ms;  // Minimum time between updates
    
    // Thumbnail buffer
    uint8_t *pixel_data;
    size_t data_size;
};

// Thumbnail Manager
struct axiom_thumbnail_manager {
    struct axiom_server *server;
    
    // Thumbnail list
    struct wl_list thumbnails;  // List of axiom_thumbnail
    int thumbnail_count;
    
    // Rendering resources
    struct wlr_renderer *renderer;
    GLuint framebuffer;
    GLuint renderbuffer;
    
    // Configuration
    bool enabled;
    int thumbnail_width;
    int thumbnail_height;
    uint32_t update_interval_ms;
    
    // Update timer
    struct wl_event_source *update_timer;
    
    // Statistics
    struct {
        uint32_t thumbnails_created;
        uint32_t thumbnails_updated;
        uint32_t update_requests;
        uint32_t render_errors;
    } stats;
};

// Core thumbnail management
struct axiom_thumbnail_manager *axiom_thumbnail_manager_create(struct axiom_server *server);
void axiom_thumbnail_manager_destroy(struct axiom_thumbnail_manager *manager);
bool axiom_thumbnail_manager_init(struct axiom_thumbnail_manager *manager);

// Thumbnail operations
struct axiom_thumbnail *axiom_thumbnail_create(struct axiom_thumbnail_manager *manager, 
                                               struct axiom_window *window);
void axiom_thumbnail_destroy(struct axiom_thumbnail *thumbnail);
bool axiom_thumbnail_update(struct axiom_thumbnail_manager *manager, 
                            struct axiom_thumbnail *thumbnail);
bool axiom_thumbnail_render(struct axiom_thumbnail_manager *manager, 
                            struct axiom_thumbnail *thumbnail);

// Window integration
struct axiom_thumbnail *axiom_thumbnail_get_for_window(struct axiom_thumbnail_manager *manager,
                                                       struct axiom_window *window);
void axiom_thumbnail_on_window_mapped(struct axiom_thumbnail_manager *manager,
                                       struct axiom_window *window);
void axiom_thumbnail_on_window_unmapped(struct axiom_thumbnail_manager *manager,
                                         struct axiom_window *window);
void axiom_thumbnail_on_window_destroyed(struct axiom_thumbnail_manager *manager,
                                          struct axiom_window *window);

// Batch operations
void axiom_thumbnail_update_all(struct axiom_thumbnail_manager *manager);
void axiom_thumbnail_cleanup_stale(struct axiom_thumbnail_manager *manager);

// Configuration
void axiom_thumbnail_set_size(struct axiom_thumbnail_manager *manager, int width, int height);
void axiom_thumbnail_set_update_interval(struct axiom_thumbnail_manager *manager, uint32_t interval_ms);
void axiom_thumbnail_enable(struct axiom_thumbnail_manager *manager, bool enabled);

// Utility functions
bool axiom_thumbnail_save_to_file(struct axiom_thumbnail *thumbnail, const char *filename);
const uint8_t *axiom_thumbnail_get_pixel_data(struct axiom_thumbnail *thumbnail);
size_t axiom_thumbnail_get_data_size(struct axiom_thumbnail *thumbnail);

// Statistics and debugging
void axiom_thumbnail_print_stats(struct axiom_thumbnail_manager *manager);
void axiom_thumbnail_reset_stats(struct axiom_thumbnail_manager *manager);

// Event handlers (internal, not exposed)

#endif // AXIOM_THUMBNAIL_MANAGER_H

