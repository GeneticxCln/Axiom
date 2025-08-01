#include "effects_realtime.h"
#include "effects.h"
#include "renderer.h"
#include "axiom.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <GLES2/gl2.h>
#include <wlr/render/wlr_renderer.h>

// Performance constants
#define EFFECT_UPDATE_THRESHOLD_MS 16  // ~60fps
#define SHADOW_OFFSET_X 5
#define SHADOW_OFFSET_Y 5
#define SHADOW_BLUR_RADIUS 10
#define MAX_BLUR_RADIUS 15

// Real-time effects initialization
bool axiom_realtime_effects_init(struct axiom_effects_manager *manager) {
    if (!manager || !manager->gl_initialized) {
        fprintf(stderr, "Effects manager not initialized for real-time effects\n");
        return false;
    }

    printf("Initializing real-time effects system...\n");
    
    // Initialize OpenGL state for effects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Check for required OpenGL extensions
    const char *extensions = (const char*)glGetString(GL_EXTENSIONS);
    if (!strstr(extensions, "GL_OES_texture_float")) {
        fprintf(stderr, "Warning: Float textures not supported, effects may be limited\n");
    }
    
    manager->realtime_enabled = true;
    manager->last_frame_time = 0;
    manager->frame_count = 0;
    
    printf("Real-time effects system initialized successfully\n");
    return true;
}

void axiom_realtime_effects_destroy(struct axiom_effects_manager *manager) {
    if (!manager) return;
    
    manager->realtime_enabled = false;
    printf("Real-time effects system destroyed\n");
}

// Window effects lifecycle
bool axiom_window_effects_init(struct axiom_window *window) {
    if (!window) return false;
    
    window->effects = calloc(1, sizeof(struct axiom_window_effects));
    if (!window->effects) {
        fprintf(stderr, "Failed to allocate window effects\n");
        return false;
    }
    
    struct axiom_window_effects *effects = window->effects;
    effects->effects_enabled = true;
    effects->current_opacity = 1.0f;
    effects->last_frame_time = 0;
    
    // Initialize shadow system
    int shadow_width = (window->geometry.width > 0 ? window->geometry.width : window->width) + SHADOW_BLUR_RADIUS * 2;
    int shadow_height = (window->geometry.height > 0 ? window->geometry.height : window->height) + SHADOW_BLUR_RADIUS * 2;
    if (!axiom_realtime_shadow_create(&effects->shadow, shadow_width, shadow_height)) {
        fprintf(stderr, "Failed to create shadow for window\n");
        free(window->effects);
        window->effects = NULL;
        return false;
    }
    
    // Initialize blur system
    int blur_width = window->geometry.width > 0 ? window->geometry.width : window->width;
    int blur_height = window->geometry.height > 0 ? window->geometry.height : window->height;
    if (!axiom_realtime_blur_create(&effects->blur, blur_width, blur_height)) {
        fprintf(stderr, "Failed to create blur for window\n");
        axiom_realtime_shadow_destroy(&effects->shadow);
        free(window->effects);
        window->effects = NULL;
        return false;
    }
    
    // Create scene graph nodes for effects
    axiom_effects_create_shadow_node(window);
    
    printf("Window effects initialized for window %p\n", (void*)window);
    return true;
}

void axiom_window_effects_destroy(struct axiom_window *window) {
    if (!window || !window->effects) return;
    
    struct axiom_window_effects *effects = window->effects;
    
    axiom_realtime_shadow_destroy(&effects->shadow);
    axiom_realtime_blur_destroy(&effects->blur);
    
    if (effects->shadow_rect) {
        wlr_scene_node_destroy(&effects->shadow_rect->node);
    }
    
    if (effects->effects_tree) {
        wlr_scene_node_destroy(&effects->effects_tree->node);
    }
    
    free(window->effects);
    window->effects = NULL;
    
    printf("Window effects destroyed for window %p\n", (void*)window);
}

void axiom_window_effects_update(struct axiom_window *window, uint32_t time_ms) {
    if (!window || !window->effects || !window->effects->effects_enabled) return;
    
    struct axiom_window_effects *effects = window->effects;
    
    // Throttle updates for performance
    if (!axiom_effects_should_update(window, time_ms)) {
        return;
    }
    
    effects->last_frame_time = time_ms;
    
    // Mark effects for update if window geometry changed
    if (effects->shadow.width != window->geometry.width + SHADOW_BLUR_RADIUS * 2 ||
        effects->shadow.height != window->geometry.height + SHADOW_BLUR_RADIUS * 2) {
        effects->shadow.needs_update = true;
        effects->blur.needs_update = true;
    }
    
    // Update shadow position in scene graph
    axiom_effects_update_shadow_position(window);
}

// Shadow rendering implementation
bool axiom_realtime_shadow_create(struct axiom_realtime_shadow *shadow, 
                                   int width, int height) {
    if (!shadow) return false;
    
    shadow->width = width;
    shadow->height = height;
    shadow->needs_update = true;
    shadow->last_update_time = 0;
    
    // Create shadow texture
    glGenTextures(1, &shadow->shadow_texture);
    glBindTexture(GL_TEXTURE_2D, shadow->shadow_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Create framebuffer for shadow rendering
    glGenFramebuffers(1, &shadow->shadow_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, shadow->shadow_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 
                          shadow->shadow_texture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Shadow framebuffer incomplete\n");
        axiom_realtime_shadow_destroy(shadow);
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    printf("Shadow created: %dx%d\n", width, height);
    return true;
}

void axiom_realtime_shadow_destroy(struct axiom_realtime_shadow *shadow) {
    if (!shadow) return;
    
    if (shadow->shadow_texture) {
        glDeleteTextures(1, &shadow->shadow_texture);
        shadow->shadow_texture = 0;
    }
    
    if (shadow->shadow_fbo) {
        glDeleteFramebuffers(1, &shadow->shadow_fbo);
        shadow->shadow_fbo = 0;
    }
}

bool axiom_realtime_shadow_render(struct axiom_effects_manager *manager,
                                   struct axiom_window *window,
                                   struct axiom_realtime_shadow *shadow) {
    if (!manager || !window || !shadow || !manager->gl_initialized) return false;
    
    // Skip if shadow doesn't need update
    if (!shadow->needs_update) return true;
    
    struct axiom_gpu_context *ctx = (struct axiom_gpu_context *)manager->gl_context;
    
    // Bind shadow framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, shadow->shadow_fbo);
    glViewport(0, 0, shadow->width, shadow->height);
    
    // Clear with transparent black
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Capture window content and render as shadow
    GLuint window_texture = axiom_capture_window_texture(window);
    if (window_texture) {
        // Create shadow parameters
        struct axiom_shadow_params params = {
            .offset_x = SHADOW_OFFSET_X,
            .offset_y = SHADOW_OFFSET_Y,
            .blur_radius = SHADOW_BLUR_RADIUS,
            .opacity = 0.5f,
            .color = {0.0f, 0.0f, 0.0f, 0.5f},
            .width = shadow->width,
            .height = shadow->height
        };
        // Render shadow with parameters
        axiom_gpu_render_shadow(ctx, &params, window_texture, shadow->shadow_texture);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    shadow->needs_update = false;
    shadow->last_update_time = time(NULL) * 1000; // Convert to milliseconds
    
    return true;
}

void axiom_realtime_shadow_update_scene(struct axiom_window *window) {
    if (!window || !window->effects || !window->effects->shadow_rect) return;
    
    // Update shadow position relative to window
    int shadow_x = window->geometry.x + SHADOW_OFFSET_X;
    int shadow_y = window->geometry.y + SHADOW_OFFSET_Y;
    
    wlr_scene_node_set_position(&window->effects->shadow_rect->node, shadow_x, shadow_y);
}

// Blur rendering implementation
bool axiom_realtime_blur_create(struct axiom_realtime_blur *blur,
                                 int width, int height) {
    if (!blur) return false;
    
    blur->width = width;
    blur->height = height;
    blur->needs_update = true;
    
    // Create horizontal pass texture and framebuffer
    glGenTextures(1, &blur->blur_texture_h);
    glBindTexture(GL_TEXTURE_2D, blur->blur_texture_h);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glGenFramebuffers(1, &blur->blur_fbo_h);
    glBindFramebuffer(GL_FRAMEBUFFER, blur->blur_fbo_h);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 
                          blur->blur_texture_h, 0);
    
    // Create vertical pass texture and framebuffer
    glGenTextures(1, &blur->blur_texture_v);
    glBindTexture(GL_TEXTURE_2D, blur->blur_texture_v);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glGenFramebuffers(1, &blur->blur_fbo_v);
    glBindFramebuffer(GL_FRAMEBUFFER, blur->blur_fbo_v);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 
                          blur->blur_texture_v, 0);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Blur framebuffer incomplete\n");
        axiom_realtime_blur_destroy(blur);
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    printf("Blur created: %dx%d\n", width, height);
    return true;
}

void axiom_realtime_blur_destroy(struct axiom_realtime_blur *blur) {
    if (!blur) return;
    
    if (blur->blur_texture_h) {
        glDeleteTextures(1, &blur->blur_texture_h);
        blur->blur_texture_h = 0;
    }
    
    if (blur->blur_texture_v) {
        glDeleteTextures(1, &blur->blur_texture_v);
        blur->blur_texture_v = 0;
    }
    
    if (blur->blur_fbo_h) {
        glDeleteFramebuffers(1, &blur->blur_fbo_h);
        blur->blur_fbo_h = 0;
    }
    
    if (blur->blur_fbo_v) {
        glDeleteFramebuffers(1, &blur->blur_fbo_v);
        blur->blur_fbo_v = 0;
    }
}

bool axiom_realtime_blur_render(struct axiom_effects_manager *manager,
                                 struct axiom_window *window,
                                 struct axiom_realtime_blur *blur) {
    if (!manager || !window || !blur || !manager->gl_initialized) return false;
    
    if (!blur->needs_update) return true;
    
    struct axiom_gpu_context *ctx = (struct axiom_gpu_context *)manager->gl_context;
    
    // Capture window content
    GLuint window_texture = axiom_capture_window_texture(window);
    if (!window_texture) return false;
    
    // Horizontal blur pass
    glBindFramebuffer(GL_FRAMEBUFFER, blur->blur_fbo_h);
    glViewport(0, 0, blur->width, blur->height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    struct axiom_blur_params h_params = {
        .radius = MAX_BLUR_RADIUS,
        .intensity = 1.0f,
        .horizontal = true,
        .width = blur->width,
        .height = blur->height
    };
    axiom_gpu_render_blur(ctx, &h_params, window_texture, blur->blur_texture_h);
    
    // Vertical blur pass
    glBindFramebuffer(GL_FRAMEBUFFER, blur->blur_fbo_v);
    glViewport(0, 0, blur->width, blur->height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    struct axiom_blur_params v_params = {
        .radius = MAX_BLUR_RADIUS,
        .intensity = 1.0f,
        .horizontal = false,
        .width = blur->width,
        .height = blur->height
    };
    axiom_gpu_render_blur(ctx, &v_params, blur->blur_texture_h, blur->blur_texture_v);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    blur->needs_update = false;
    return true;
}

// Window content capture
GLuint axiom_capture_window_texture(struct axiom_window *window) {
    if (!window || !window->surface) return 0;
    
    // Get the wlr_texture from the window's surface
    struct wlr_texture *wlr_tex = wlr_surface_get_texture(window->surface);
    if (!wlr_tex) return 0;
    
    // Create a new texture for window content
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Initialize texture with window dimensions
    int width = window->geometry.width > 0 ? window->geometry.width : window->width;
    int height = window->geometry.height > 0 ? window->geometry.height : window->height;
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // TODO: Copy actual window surface content to texture
    // This would require integration with wlroots texture system
    // For now, create a solid color texture as placeholder
    GLubyte *placeholder_data = malloc(width * height * 4);
    if (placeholder_data) {
        // Fill with semi-transparent white for shadow generation
        for (int i = 0; i < width * height * 4; i += 4) {
            placeholder_data[i] = 255;     // R
            placeholder_data[i+1] = 255;   // G
            placeholder_data[i+2] = 255;   // B
            placeholder_data[i+3] = 200;   // A (semi-transparent)
        }
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, placeholder_data);
        free(placeholder_data);
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

bool axiom_upload_window_content(struct axiom_window *window, GLuint texture) {
    if (!window || !texture) return false;
    
    // Implementation would copy window surface data to the provided texture
    // This is a placeholder implementation
    return true;
}

// Scene graph integration
void axiom_effects_create_shadow_node(struct axiom_window *window) {
    if (!window || !window->effects || !window->scene_tree) return;
    
    struct axiom_window_effects *effects = window->effects;
    
    // Create effects tree as child of window scene tree
    effects->effects_tree = wlr_scene_tree_create(window->scene_tree);
    
    // Create shadow rectangle node
    effects->shadow_rect = wlr_scene_rect_create(effects->effects_tree,
                                                  effects->shadow.width,
                                                  effects->shadow.height,
                                                  (float[4]){0.0f, 0.0f, 0.0f, 0.3f});
    
    // Position shadow behind the window
    wlr_scene_node_place_below(&effects->shadow_rect->node, &window->scene_tree->node);
    
    printf("Shadow scene node created for window %p\n", (void*)window);
}

void axiom_effects_update_shadow_position(struct axiom_window *window) {
    axiom_realtime_shadow_update_scene(window);
}

void axiom_effects_set_window_opacity(struct axiom_window *window, float opacity) {
    if (!window || !window->effects) return;
    
    window->effects->current_opacity = opacity;
    
    // Update scene node opacity if available
    if (window->scene_tree) {
        wlr_scene_node_set_enabled(&window->scene_tree->node, opacity > 0.0f);
    }
}

// Performance optimization
void axiom_effects_mark_dirty(struct axiom_window *window) {
    if (!window || !window->effects) return;
    
    window->effects->shadow.needs_update = true;
    window->effects->blur.needs_update = true;
}

bool axiom_effects_should_update(struct axiom_window *window, uint32_t current_time) {
    if (!window || !window->effects) return false;
    
    uint32_t elapsed = current_time - window->effects->last_frame_time;
    return elapsed >= EFFECT_UPDATE_THRESHOLD_MS;
}

void axiom_effects_throttle_updates(struct axiom_effects_manager *manager) {
    if (!manager) return;
    
    // Implement frame rate limiting for effects updates
    manager->frame_count++;
    
    uint32_t current_time = time(NULL) * 1000;
    if (current_time - manager->last_frame_time >= 1000) {
        printf("Effects FPS: %u\n", manager->frame_count);
        manager->frame_count = 0;
        manager->last_frame_time = current_time;
    }
}

// Configuration helpers
void axiom_effects_update_shadow_config(struct axiom_window *window,
                                         struct axiom_shadow_config *config) {
    if (!window || !window->effects || !config) return;
    
    // Apply new shadow configuration
    axiom_effects_mark_dirty(window);
}

void axiom_effects_update_blur_config(struct axiom_window *window,
                                       struct axiom_blur_config *config) {
    if (!window || !window->effects || !config) return;
    
    // Apply new blur configuration
    axiom_effects_mark_dirty(window);
}

void axiom_effects_update_transparency_config(struct axiom_window *window,
                                               struct axiom_transparency_config *config) {
    if (!window || !window->effects || !config) return;
    
    // Use focused_opacity as default opacity for window
    // TODO: Implement focused/unfocused state tracking
    axiom_effects_set_window_opacity(window, config->focused_opacity);
}

// Debug and profiling
void axiom_effects_debug_render_times(struct axiom_effects_manager *manager) {
    if (!manager) return;
    
    printf("Effects render times - Frame: %u, Last: %u ms\n",
           manager->frame_count, manager->last_frame_time);
}

void axiom_effects_profile_frame(struct axiom_effects_manager *manager, uint32_t frame_time) {
    if (!manager) return;
    
    manager->last_frame_time = frame_time;
    // Additional profiling logic could be added here
}

// Animation integration (placeholder implementations)
void axiom_effects_animate_shadow_opacity(struct axiom_window *window,
                                           float target_opacity, uint32_t duration) {
    if (!window || !window->effects) return;
    
    // Placeholder for shadow opacity animation
    printf("Animating shadow opacity to %.2f over %u ms\n", target_opacity, duration);
}

void axiom_effects_animate_blur_strength(struct axiom_window *window,
                                          float target_strength, uint32_t duration) {
    if (!window || !window->effects) return;
    
    // Placeholder for blur strength animation
    printf("Animating blur strength to %.2f over %u ms\n", target_strength, duration);
}
