#include "effects_realtime.h"
#include "effects.h"
#include "renderer.h"
#include "axiom.h"
#include "animation.h"
#include "window_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <GLES2/gl2.h>
#include <wlr/render/wlr_renderer.h>

// Animation callback functions
static void shadow_opacity_animation_callback(struct axiom_animation *anim, void *user_data);
static void blur_strength_animation_callback(struct axiom_animation *anim, void *user_data);

// Performance constants
#define EFFECT_UPDATE_THRESHOLD_MS 16  // ~60fps default
#define EFFECT_UPDATE_THRESHOLD_LOW_MS 33  // ~30fps for low-end hardware
#define EFFECT_UPDATE_THRESHOLD_HIGH_MS 8   // ~120fps for high-end hardware
#define SHADOW_OFFSET_X 5
#define SHADOW_OFFSET_Y 5
#define SHADOW_BLUR_RADIUS 10
#define MAX_BLUR_RADIUS 15
#define PERFORMANCE_SAMPLE_SIZE 60  // Sample 60 frames for performance analysis
#define TARGET_FRAME_TIME_MS 16     // Target 16ms frame time (60fps)
#define PERFORMANCE_ADJUSTMENT_THRESHOLD 5  // Adjust after 5 consecutive slow frames

// Performance tracking structure
struct axiom_performance_tracker {
    uint32_t frame_times[PERFORMANCE_SAMPLE_SIZE];
    int frame_index;
    uint32_t slow_frame_count;
    uint32_t update_threshold_ms;
    bool adaptive_performance;
};

static struct axiom_performance_tracker perf_tracker = {
    .frame_index = 0,
    .slow_frame_count = 0,
    .update_threshold_ms = EFFECT_UPDATE_THRESHOLD_MS,
    .adaptive_performance = true
};

// Real-time effects initialization
bool axiom_realtime_effects_init(struct axiom_effects_manager *manager) {
    if (!manager || !manager->gl_initialized) {
        axiom_log_error("Effects manager not initialized for real-time effects");
        return false;
    }

    axiom_log_info("Initializing real-time effects system...");
    
    // Initialize OpenGL state for effects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Check for required OpenGL extensions
    const char *extensions = (const char*)glGetString(GL_EXTENSIONS);
    if (!strstr(extensions, "GL_OES_texture_float")) {
        axiom_log_warn("Float textures not supported, effects may be limited");
    }
    
    manager->realtime_enabled = true;
    manager->last_frame_time = 0;
    manager->frame_count = 0;
    
    axiom_log_info("Real-time effects system initialized successfully");
    return true;
}

void axiom_realtime_effects_destroy(struct axiom_effects_manager *manager) {
    if (!manager) return;
    
    manager->realtime_enabled = false;
    axiom_log_info("Real-time effects system destroyed");
}

// Window effects lifecycle
bool axiom_window_effects_init(struct axiom_window *window) {
    if (!window) return false;
    
    window->effects = calloc(1, sizeof(struct axiom_window_effects));
    if (!window->effects) {
        axiom_log_error("Failed to allocate window effects");
        return false;
    }
    
    struct axiom_window_effects *effects = window->effects;
    effects->effects_enabled = true;
    effects->current_opacity = 1.0f;
    effects->last_frame_time = 0;
    
    // Initialize shadow system using configuration
    struct axiom_effects_manager *effects_manager = window->server->effects_manager;
    int shadow_blur_radius = effects_manager ? effects_manager->shadow.blur_radius : SHADOW_BLUR_RADIUS;
    int shadow_width = (window->geometry->width > 0 ? window->geometry->width : window->width) + shadow_blur_radius * 2;
    int shadow_height = (window->geometry->height > 0 ? window->geometry->height : window->height) + shadow_blur_radius * 2;
    if (!axiom_realtime_shadow_create(&effects->shadow, shadow_width, shadow_height)) {
        axiom_log_error("Failed to create shadow for window");
        free(window->effects);
        window->effects = NULL;
        return false;
    }
    
    // Initialize blur system
    int blur_width = window->geometry->width > 0 ? window->geometry->width : window->width;
    int blur_height = window->geometry->height > 0 ? window->geometry->height : window->height;
    if (!axiom_realtime_blur_create(&effects->blur, blur_width, blur_height)) {
        axiom_log_error("Failed to create blur for window");
        axiom_realtime_shadow_destroy(&effects->shadow);
        free(window->effects);
        window->effects = NULL;
        return false;
    }
    
    // Create scene graph nodes for effects
    axiom_effects_create_shadow_node(window);
    
    axiom_log_debug("Window effects initialized for window %p", (void*)window);
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
    
    axiom_log_debug("Window effects destroyed for window %p", (void*)window);
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
    struct axiom_effects_manager *effects_manager = window->server->effects_manager;
    int shadow_blur_radius = effects_manager ? effects_manager->shadow.blur_radius : SHADOW_BLUR_RADIUS;
    if (effects->shadow.width != window->geometry->width + shadow_blur_radius * 2 ||
        effects->shadow.height != window->geometry->height + shadow_blur_radius * 2) {
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
        axiom_log_error("Shadow framebuffer incomplete");
        axiom_realtime_shadow_destroy(shadow);
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    axiom_log_debug("Shadow created: %dx%d", width, height);
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
        // Create shadow parameters from configuration
        struct axiom_shadow_params params = {
            .offset_x = manager->shadow.offset_x > 0 ? manager->shadow.offset_x : SHADOW_OFFSET_X,
            .offset_y = manager->shadow.offset_y > 0 ? manager->shadow.offset_y : SHADOW_OFFSET_Y,
            .blur_radius = manager->shadow.blur_radius > 0 ? manager->shadow.blur_radius : SHADOW_BLUR_RADIUS,
            .opacity = manager->shadow.opacity > 0.0f ? manager->shadow.opacity : 0.5f,
            .color = {0.0f, 0.0f, 0.0f, manager->shadow.opacity > 0.0f ? manager->shadow.opacity : 0.5f},
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
    
    // Update shadow position relative to window using configuration
    struct axiom_effects_manager *effects_manager = window->server->effects_manager;
    int offset_x = effects_manager && effects_manager->shadow.offset_x > 0 ? 
                   effects_manager->shadow.offset_x : SHADOW_OFFSET_X;
    int offset_y = effects_manager && effects_manager->shadow.offset_y > 0 ? 
                   effects_manager->shadow.offset_y : SHADOW_OFFSET_Y;
    int shadow_x = window->geometry->x + offset_x;
    int shadow_y = window->geometry->y + offset_y;
    
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
        axiom_log_error("Blur framebuffer incomplete");
        axiom_realtime_blur_destroy(blur);
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    axiom_log_debug("Blur created: %dx%d", width, height);
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

// Window content capture from wlroots texture system
GLuint axiom_capture_window_texture(struct axiom_window *window) {
    if (!window || !window->surface) return 0;
    
    // Get the wlr_texture from the window's surface
    struct wlr_texture *wlr_tex = wlr_surface_get_texture(window->surface);
    if (!wlr_tex) {
        // If no texture available, create placeholder
        return axiom_create_placeholder_texture(window);
    }
    
    // Get texture dimensions from wlroots
    int tex_width = wlr_tex->width;
    int tex_height = wlr_tex->height;
    
    if (tex_width <= 0 || tex_height <= 0) {
        return axiom_create_placeholder_texture(window);
    }
    
    // Create OpenGL texture
    GLuint gl_texture;
    glGenTextures(1, &gl_texture);
    glBindTexture(GL_TEXTURE_2D, gl_texture);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Try to extract OpenGL texture ID from wlroots texture
    // This is implementation-specific and may vary based on wlroots version
    bool content_copied = false;
    
    // Method 1: Try to read pixel data using wlroots 0.19 API
    if (!content_copied) {
        void *pixel_data = malloc(tex_width * tex_height * 4);
        if (pixel_data) {
            // Use the correct wlroots 0.19 API signature
            if (wlr_texture_read_pixels(wlr_tex, pixel_data)) {
                // Upload pixel data to our OpenGL texture
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0, 
                            GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);
                content_copied = true;
                axiom_log_debug("Successfully captured window content via pixel readback");
            }
            free(pixel_data);
        }
    }
    
    // Method 3: Final fallback - create window-based content
    if (!content_copied) {
        axiom_log_warn("Could not capture window content, using window-based fallback");
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        
        // Create simple gradient based on window properties
        GLubyte *fallback_data = malloc(tex_width * tex_height * 4);
        if (fallback_data) {
            axiom_generate_window_fallback_content(window, fallback_data, tex_width, tex_height);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width, tex_height, GL_RGBA, GL_UNSIGNED_BYTE, fallback_data);
            free(fallback_data);
        } else {
            axiom_log_error("Failed to allocate fallback data buffer (%d bytes)", tex_width * tex_height * 4);
        }
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    axiom_log_debug("Captured window texture: %dx%d (ID: %u)", tex_width, tex_height, gl_texture);
    return gl_texture;
}

bool axiom_upload_window_content(struct axiom_window *window, GLuint texture) {
    if (!window || !texture || !window->surface) return false;
    
    // Get the wlr_texture from the window's surface
    struct wlr_texture *wlr_tex = wlr_surface_get_texture(window->surface);
    if (!wlr_tex) {
        axiom_log_debug("No wlroots texture available for window");
        return false;
    }
    
    // Get texture dimensions
    int width = wlr_tex->width;
    int height = wlr_tex->height;
    
    if (width <= 0 || height <= 0) {
        axiom_log_warn("Invalid texture dimensions: %dx%d", width, height);
        return false;
    }
    
    // Bind our target texture
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Try to read pixel data from wlroots texture
    void *pixel_data = malloc(width * height * 4);
    if (!pixel_data) {
        axiom_log_error("Failed to allocate pixel data buffer (%d bytes)", width * height * 4);
        glBindTexture(GL_TEXTURE_2D, 0);
        return false;
        return false;
    }
    
    bool success = false;
    if (wlr_texture_read_pixels(wlr_tex, pixel_data)) {
        // Upload the pixel data to our OpenGL texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, 
                    GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);
        success = true;
        axiom_log_debug("Successfully uploaded window content to texture: %dx%d", width, height);
    } else {
        axiom_log_debug("Failed to read pixels from wlroots texture");
    }
    
    free(pixel_data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return success;
}

// Scene graph integration
void axiom_effects_create_shadow_node(struct axiom_window *window) {
    if (!window || !window->effects || !window->scene_tree) return;
    
    struct axiom_window_effects *effects = window->effects;
    
    // Create effects tree as sibling of window scene tree (same parent)
    if (window->scene_tree->node.parent) {
        effects->effects_tree = wlr_scene_tree_create(window->scene_tree->node.parent);
    } else {
        // Fallback: create as child if no parent
        effects->effects_tree = wlr_scene_tree_create(window->scene_tree);
    }
    
    // Create shadow rectangle node
    effects->shadow_rect = wlr_scene_rect_create(effects->effects_tree,
                                                  effects->shadow.width,
                                                  effects->shadow.height,
                                                  (float[4]){0.0f, 0.0f, 0.0f, 0.3f});
    
    // Position shadow behind the window (now they have the same parent)
    if (window->scene_tree->node.parent) {
        wlr_scene_node_place_below(&effects->effects_tree->node, &window->scene_tree->node);
    }
    
    axiom_log_debug("Shadow scene node created for window %p", (void*)window);
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

void axiom_effects_throttle_updates(struct axiom_effects_manager *manager, uint32_t *effect_update_threshold_ms) {
    if (!manager || !effect_update_threshold_ms) return;

    // Get current time with better precision
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint32_t current_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    
    // Record frame time
    uint32_t frame_time = 0;
    if (manager->last_frame_time > 0) {
        frame_time = current_time - manager->last_frame_time;
        
        // Update performance tracker
        perf_tracker.frame_times[perf_tracker.frame_index] = frame_time;
        perf_tracker.frame_index = (perf_tracker.frame_index + 1) % PERFORMANCE_SAMPLE_SIZE;
        
        // Check if frame was slow
        if (frame_time > TARGET_FRAME_TIME_MS + PERFORMANCE_ADJUSTMENT_THRESHOLD) {
            perf_tracker.slow_frame_count++;
        } else {
            perf_tracker.slow_frame_count = 0; // Reset counter on good frame
        }
    }
    
    manager->frame_count++;
    manager->last_frame_time = current_time;

    // Adaptive performance adjustment
    if (perf_tracker.adaptive_performance && manager->frame_count % 30 == 0) { // Check every 30 frames
        // Calculate average frame time over sample window
        uint32_t total_time = 0;
        uint32_t valid_samples = 0;
        
        for (int i = 0; i < PERFORMANCE_SAMPLE_SIZE; i++) {
            if (perf_tracker.frame_times[i] > 0) {
                total_time += perf_tracker.frame_times[i];
                valid_samples++;
            }
        }
        
        if (valid_samples > 10) {
            uint32_t average_frame_time = total_time / valid_samples;
            
            // Adjust threshold based on performance
            if (perf_tracker.slow_frame_count >= PERFORMANCE_ADJUSTMENT_THRESHOLD) {
                // Multiple slow frames detected, lower quality/frequency
                perf_tracker.update_threshold_ms = EFFECT_UPDATE_THRESHOLD_LOW_MS;
                *effect_update_threshold_ms = perf_tracker.update_threshold_ms;
                axiom_log_info("Performance: Reducing effects frequency (avg: %ums, slow frames: %u)", 
                       average_frame_time, perf_tracker.slow_frame_count);
                perf_tracker.slow_frame_count = 0;
            } else if (average_frame_time < TARGET_FRAME_TIME_MS - 2) {
                // Good performance, can increase quality
                if (perf_tracker.update_threshold_ms > EFFECT_UPDATE_THRESHOLD_HIGH_MS) {
                    perf_tracker.update_threshold_ms = EFFECT_UPDATE_THRESHOLD_MS;
                    *effect_update_threshold_ms = perf_tracker.update_threshold_ms;
                    axiom_log_debug("Performance: Restoring normal effects frequency (avg: %ums)", average_frame_time);
                }
            }
        }
    }

    // Log performance stats periodically
    if (manager->frame_count % 300 == 0) { // Every ~5 seconds at 60fps
        uint32_t fps = (1000 * 300) / (current_time - (manager->last_frame_time - 5000));
        axiom_log_debug("Effects performance: ~%u FPS, threshold: %ums", fps, perf_tracker.update_threshold_ms);
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
    
    // Use appropriate opacity based on window focus state
    float target_opacity;
    if (window->is_focused) {
        target_opacity = config->focused_opacity;
    } else if (window->server && window->server->focused_window != NULL) {
        // Window is not focused and there's another focused window
        target_opacity = config->unfocused_opacity;
    } else {
        // No window is focused, use inactive opacity
        target_opacity = config->inactive_opacity;
    }
    
    axiom_effects_set_window_opacity(window, target_opacity);
}

// Debug and profiling
void axiom_effects_debug_render_times(struct axiom_effects_manager *manager) {
    if (!manager) return;
    
    axiom_log_debug("Effects render times - Frame: %u, Last: %u ms",
           manager->frame_count, manager->last_frame_time);
}

void axiom_effects_profile_frame(struct axiom_effects_manager *manager, uint32_t frame_time) {
    if (!manager) return;
    
    manager->last_frame_time = frame_time;
    // Additional profiling logic could be added here
}

// Animation integration 
void axiom_effects_animate_shadow_opacity(struct axiom_window *window,
                                           float target_opacity, uint32_t duration) {
    if (!window || !window->effects || !window->server || !window->server->animation_manager) return;
    
    // Create fade animation for shadow opacity
    struct axiom_animation *anim = axiom_animation_create(AXIOM_ANIM_FADE, duration);
    if (!anim) {
        axiom_log_error("Failed to create shadow opacity animation");
        return;
    }
    
    // Set animation parameters
    anim->window = window;
    anim->easing = AXIOM_EASE_OUT_CUBIC;
    // Get shadow opacity from effects manager if available
    struct axiom_effects_manager *effects_manager = window->server->effects_manager;
    anim->start_values.opacity = effects_manager ? effects_manager->shadow.opacity : 0.5f;
    anim->end_values.opacity = target_opacity;
    anim->auto_cleanup = true;
    
    // Set update callback to modify shadow opacity
    anim->on_update = shadow_opacity_animation_callback;
    anim->user_data = window;
    
    // Start the animation
    axiom_animation_start(window->server->animation_manager, anim);
    axiom_log_debug("Started shadow opacity animation: %.2f -> %.2f over %u ms", 
           anim->start_values.opacity, target_opacity, duration);
}

void axiom_effects_animate_blur_strength(struct axiom_window *window,
                                          float target_strength, uint32_t duration) {
    if (!window || !window->effects || !window->server || !window->server->animation_manager) return;
    
    // Create custom animation for blur strength
    struct axiom_animation *anim = axiom_animation_create(AXIOM_ANIM_FADE, duration);
    if (!anim) {
        axiom_log_error("Failed to create blur strength animation");
        return;
    }
    
    // Set animation parameters
    anim->window = window;
    anim->easing = AXIOM_EASE_OUT_CUBIC;
    // Get blur intensity from effects manager if available
    struct axiom_effects_manager *effects_manager = window->server->effects_manager;
    anim->start_values.opacity = effects_manager ? effects_manager->blur.intensity : 0.7f;
    anim->end_values.opacity = target_strength;
    anim->auto_cleanup = true;
    
    // Set update callback to modify blur strength
    anim->on_update = blur_strength_animation_callback;
    anim->user_data = window;
    
    // Start the animation
    axiom_animation_start(window->server->animation_manager, anim);
    axiom_log_debug("Started blur strength animation: %.2f -> %.2f over %u ms", 
           anim->start_values.opacity, target_strength, duration);
}

// Helper functions for window content capture
GLuint axiom_create_placeholder_texture(struct axiom_window *window) {
    if (!window) return 0;
    
    int width = window->geometry->width > 0 ? window->geometry->width : window->width;
    int height = window->geometry->height > 0 ? window->geometry->height : window->height;
    
    if (width <= 0) width = 400;
    if (height <= 0) height = 300;
    
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Create placeholder data
    GLubyte *data = malloc(width * height * 4);
    if (data) {
        axiom_generate_window_fallback_content(window, data, width, height);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        free(data);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

void axiom_generate_window_fallback_content(struct axiom_window *window, GLubyte *data, int width, int height) {
    if (!data || width <= 0 || height <= 0) return;
    
    // Generate content based on window properties
    const char *app_id = window->xdg_toplevel ? window->xdg_toplevel->app_id : NULL;
    // Note: title could be used for additional content generation in the future
    
    // Use app_id or title to determine color scheme
    uint8_t base_r = 100, base_g = 100, base_b = 100;
    
    if (app_id) {
        // Simple hash to color mapping
        uint32_t hash = 0;
        for (const char *p = app_id; *p; p++) {
            hash = hash * 31 + *p;
        }
        base_r = 80 + (hash % 100);
        base_g = 80 + ((hash >> 8) % 100);
        base_b = 80 + ((hash >> 16) % 100);
    }
    
    // Create gradient effect
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;
            
            float fx = (float)x / width;
            float fy = (float)y / height;
            
            // Create radial gradient from center
            float dx = fx - 0.5f;
            float dy = fy - 0.5f;
            float dist = sqrt(dx * dx + dy * dy);
            float fade = 1.0f - (dist * 1.4f);
            if (fade < 0.0f) fade = 0.0f;
            
            data[idx] = (uint8_t)(base_r * fade);
            data[idx + 1] = (uint8_t)(base_g * fade);
            data[idx + 2] = (uint8_t)(base_b * fade);
            data[idx + 3] = window->is_focused ? 220 : 180; // More opaque if focused
        }
    }
}

bool axiom_copy_texture_via_fbo(GLuint source_texture, GLuint target_texture, int width, int height) {
    // Create temporary FBO for copying
    GLuint temp_fbo;
    glGenFramebuffers(1, &temp_fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, temp_fbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, source_texture, 0);
    
    if (glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glDeleteFramebuffers(1, &temp_fbo);
        return false;
    }
    
    // Bind target texture to another FBO
    GLuint target_fbo;
    glGenFramebuffers(1, &target_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target_fbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target_texture, 0);
    
    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glDeleteFramebuffers(1, &temp_fbo);
        glDeleteFramebuffers(1, &target_fbo);
        return false;
    }
    
    // Copy from source to target
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    
    // Cleanup
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &temp_fbo);
    glDeleteFramebuffers(1, &target_fbo);
    
    return glGetError() == GL_NO_ERROR;
}

// Animation callback implementations
static void shadow_opacity_animation_callback(struct axiom_animation *anim, void *user_data) {
    struct axiom_window *window = (struct axiom_window *)user_data;
    if (!window || !window->effects || !anim || !window->server) return;
    
    // Calculate current opacity using easing function
    float progress = axiom_easing_apply(anim->easing, anim->progress);
    float current_opacity = anim->start_values.opacity + 
        (anim->end_values.opacity - anim->start_values.opacity) * progress;
    
    // Update shadow opacity in effects manager if available
    struct axiom_effects_manager *effects_manager = window->server->effects_manager;
    if (effects_manager) {
        effects_manager->shadow.opacity = current_opacity;
    }
    
    // Mark shadow for re-rendering
    window->effects->shadow.needs_update = true;
    
    // Update shadow rectangle opacity in scene graph if available
    if (window->effects->shadow_rect) {
        // Note: wlroots scene rect doesn't have direct opacity setting, 
        // so we'd need to recreate or use different approach for opacity changes
        // For now, we just mark it as needing potential recreation
    }
}

static void blur_strength_animation_callback(struct axiom_animation *anim, void *user_data) {
    struct axiom_window *window = (struct axiom_window *)user_data;
    if (!window || !window->effects || !anim) return;
    
    // Calculate current blur strength using easing function
    float progress = axiom_easing_apply(anim->easing, anim->progress);
    float current_strength = anim->start_values.opacity + 
        (anim->end_values.opacity - anim->start_values.opacity) * progress;
    
    // Update blur intensity in effects manager if available
    struct axiom_effects_manager *effects_manager = window->server->effects_manager;
    if (effects_manager) {
        effects_manager->blur.intensity = current_strength;
    }
    
    // Mark blur for re-rendering
    window->effects->blur.needs_update = true;
}
