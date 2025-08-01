#ifndef AXIOM_EFFECTS_H
#define AXIOM_EFFECTS_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_scene.h>

// Forward declarations
struct axiom_server;
struct axiom_window;

// Shadow configuration
struct axiom_shadow_config {
    bool enabled;
    int blur_radius;        // Shadow blur radius in pixels
    int offset_x;          // Shadow X offset
    int offset_y;          // Shadow Y offset  
    float opacity;         // Shadow opacity (0.0-1.0)
    uint32_t color;        // Shadow color (RGBA)
};

// Blur configuration
struct axiom_blur_config {
    bool enabled;
    int radius;            // Blur radius in pixels
    bool focus_only;       // Only blur focused windows
    float intensity;       // Blur intensity (0.0-1.0)
};

// Transparency configuration
struct axiom_transparency_config {
    bool enabled;
    float focused_opacity;    // Opacity for focused windows
    float unfocused_opacity;  // Opacity for unfocused windows
    float inactive_opacity;   // Opacity for inactive windows
};

// Effects manager
struct axiom_effects_manager {
    struct axiom_shadow_config shadow;
    struct axiom_blur_config blur;
    struct axiom_transparency_config transparency;
    
    // Rendering context
    bool gl_initialized;
    void *gl_context;
    
    // Real-time capabilities
    bool realtime_enabled;
    uint32_t last_frame_time;
    uint32_t frame_count;
    
    // Shadow texture cache
    struct wl_list shadow_cache; // list of cached shadow textures
};

// Shadow rendering
struct axiom_shadow_texture {
    struct wl_list link;
    int width, height;
    int blur_radius;
    uint32_t color;
    void *texture_data;
    bool dirty;
};

// Effects manager functions
bool axiom_effects_manager_init(struct axiom_effects_manager *manager);
void axiom_effects_manager_destroy(struct axiom_effects_manager *manager);
void axiom_effects_apply_to_window(struct axiom_effects_manager *manager, 
                                   struct axiom_window *window);

// Shadow functions
bool axiom_shadow_init(struct axiom_effects_manager *manager);
void axiom_shadow_destroy(struct axiom_effects_manager *manager);
struct axiom_shadow_texture *axiom_shadow_create_texture(
    struct axiom_effects_manager *manager,
    int width, int height, int blur_radius, uint32_t color);
void axiom_shadow_render_for_window(struct axiom_effects_manager *manager,
                                    struct axiom_window *window);
void axiom_shadow_update_config(struct axiom_effects_manager *manager,
                                struct axiom_shadow_config *config);

// Blur functions  
bool axiom_blur_init(struct axiom_effects_manager *manager);
void axiom_blur_destroy(struct axiom_effects_manager *manager);
void axiom_blur_apply_to_window(struct axiom_effects_manager *manager,
                                struct axiom_window *window);
void axiom_blur_update_config(struct axiom_effects_manager *manager,
                              struct axiom_blur_config *config);

// Transparency functions
void axiom_transparency_apply_to_window(struct axiom_effects_manager *manager,
                                        struct axiom_window *window);
void axiom_transparency_update_config(struct axiom_effects_manager *manager,
                                      struct axiom_transparency_config *config);

// Utility functions
uint32_t axiom_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void axiom_effects_invalidate_cache(struct axiom_effects_manager *manager);
bool axiom_effects_gl_check_extensions(void);

// Configuration loading
bool axiom_effects_load_config(struct axiom_effects_manager *manager, 
                               const char *config_path);

#endif // AXIOM_EFFECTS_H
