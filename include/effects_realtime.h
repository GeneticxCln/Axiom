#ifndef AXIOM_EFFECTS_REALTIME_H
#define AXIOM_EFFECTS_REALTIME_H

#include "effects.h"
#include "renderer.h"
#include <wlr/types/wlr_scene.h>
#include <wlr/render/wlr_texture.h>

// Forward declarations
struct axiom_server;
struct axiom_window;

// Real-time shadow system
struct axiom_realtime_shadow {
    GLuint shadow_texture;
    GLuint shadow_fbo;
    int width, height;
    bool needs_update;
    uint32_t last_update_time;
};

// Real-time blur system
struct axiom_realtime_blur {
    GLuint blur_texture_h; // Horizontal pass
    GLuint blur_texture_v; // Vertical pass
    GLuint blur_fbo_h;
    GLuint blur_fbo_v;
    int width, height;
    bool needs_update;
};

// Window effects state
struct axiom_window_effects {
    struct axiom_realtime_shadow shadow;
    struct axiom_realtime_blur blur;
    struct wlr_scene_tree *effects_tree;
    struct wlr_scene_rect *shadow_rect;
    bool effects_enabled;
    float current_opacity;
    uint32_t last_frame_time;
};

// Real-time effects functions
bool axiom_realtime_effects_init(struct axiom_effects_manager *manager);
void axiom_realtime_effects_destroy(struct axiom_effects_manager *manager);

// Window effects lifecycle
bool axiom_window_effects_init(struct axiom_window *window);
void axiom_window_effects_destroy(struct axiom_window *window);
void axiom_window_effects_update(struct axiom_window *window, uint32_t time_ms);

// Shadow rendering
bool axiom_realtime_shadow_create(struct axiom_realtime_shadow *shadow, 
                                   int width, int height);
void axiom_realtime_shadow_destroy(struct axiom_realtime_shadow *shadow);
bool axiom_realtime_shadow_render(struct axiom_effects_manager *manager,
                                   struct axiom_window *window,
                                   struct axiom_realtime_shadow *shadow);
void axiom_realtime_shadow_update_scene(struct axiom_window *window);

// Blur rendering
bool axiom_realtime_blur_create(struct axiom_realtime_blur *blur,
                                 int width, int height);
void axiom_realtime_blur_destroy(struct axiom_realtime_blur *blur);
bool axiom_realtime_blur_render(struct axiom_effects_manager *manager,
                                 struct axiom_window *window,
                                 struct axiom_realtime_blur *blur);

// Window content capture
GLuint axiom_capture_window_texture(struct axiom_window *window);
bool axiom_upload_window_content(struct axiom_window *window, GLuint texture);

// Scene graph integration
void axiom_effects_create_shadow_node(struct axiom_window *window);
void axiom_effects_update_shadow_position(struct axiom_window *window);
void axiom_effects_set_window_opacity(struct axiom_window *window, float opacity);

// Performance optimization
void axiom_effects_mark_dirty(struct axiom_window *window);
bool axiom_effects_should_update(struct axiom_window *window, uint32_t current_time);
void axiom_effects_throttle_updates(struct axiom_effects_manager *manager, uint32_t *effect_update_threshold_ms);

// Configuration helpers
void axiom_effects_update_shadow_config(struct axiom_window *window,
                                         struct axiom_shadow_config *config);
void axiom_effects_update_blur_config(struct axiom_window *window,
                                       struct axiom_blur_config *config);
void axiom_effects_update_transparency_config(struct axiom_window *window,
                                               struct axiom_transparency_config *config);

// Debug and profiling
void axiom_effects_debug_render_times(struct axiom_effects_manager *manager);
void axiom_effects_profile_frame(struct axiom_effects_manager *manager, uint32_t frame_time);

// Animation integration
void axiom_effects_animate_shadow_opacity(struct axiom_window *window,
                                           float target_opacity, uint32_t duration);
void axiom_effects_animate_blur_strength(struct axiom_window *window,
                                          float target_strength, uint32_t duration);

// Helper functions for window content capture
GLuint axiom_create_placeholder_texture(struct axiom_window *window);
void axiom_generate_window_fallback_content(struct axiom_window *window, GLubyte *data, int width, int height);
bool axiom_copy_texture_via_fbo(GLuint source_texture, GLuint target_texture, int width, int height);

#endif // AXIOM_EFFECTS_REALTIME_H
