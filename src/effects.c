#include "effects.h"
#include "axiom.h"
#include "renderer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Helper macros
#define RGBA_COLOR(r, g, b, a) (((a) << 24) | ((b) << 16) | ((g) << 8) | (r))

bool axiom_effects_manager_init(struct axiom_effects_manager *manager, struct axiom_effects_config *config) {
    if (!manager) return false;
    memset(manager, 0, sizeof(*manager));

    // Apply configuration if provided
    if (config) {
        // Shadow configuration
        manager->shadow.enabled = config->shadows_enabled;
        manager->shadow.blur_radius = config->shadow_blur_radius;
        manager->shadow.offset_x = config->shadow_offset_x;
        manager->shadow.offset_y = config->shadow_offset_y;
        manager->shadow.opacity = config->shadow_opacity;
        
        // Parse shadow color from hex string to RGBA
        uint32_t shadow_color = RGBA_COLOR(0, 0, 0, 128); // Default black
        if (config->shadow_color && strlen(config->shadow_color) >= 7 && config->shadow_color[0] == '#') {
            // Simple hex color parsing
            unsigned int r, g, b;
            if (sscanf(config->shadow_color + 1, "%02x%02x%02x", &r, &g, &b) == 3) {
                shadow_color = RGBA_COLOR(r, g, b, (int)(config->shadow_opacity * 255));
            }
        }
        manager->shadow.color = shadow_color;

        // Blur configuration
        manager->blur.enabled = config->blur_enabled;
        manager->blur.radius = config->blur_radius;
        manager->blur.focus_only = config->blur_focus_only;
        manager->blur.intensity = config->blur_intensity;

        // Transparency configuration
        manager->transparency.enabled = config->transparency_enabled;
        manager->transparency.focused_opacity = config->focused_opacity;
        manager->transparency.unfocused_opacity = config->unfocused_opacity;
        manager->transparency.inactive_opacity = config->inactive_opacity;
        
        axiom_log_info("Effects manager configured: shadows=%s, blur=%s, transparency=%s",
               manager->shadow.enabled ? "on" : "off",
               manager->blur.enabled ? "on" : "off",
               manager->transparency.enabled ? "on" : "off");
    } else {
        // Use defaults
        manager->shadow.enabled = true;
        manager->shadow.blur_radius = 10;
        manager->shadow.offset_x = 5;
        manager->shadow.offset_y = 5;
        manager->shadow.opacity = 0.5;
        manager->shadow.color = RGBA_COLOR(0, 0, 0, 128);

        manager->blur.enabled = true;
        manager->blur.radius = 15;
        manager->blur.focus_only = false;
        manager->blur.intensity = 0.7;

        manager->transparency.enabled = true;
        manager->transparency.focused_opacity = 1.0;
        manager->transparency.unfocused_opacity = 0.85;
        manager->transparency.inactive_opacity = 0.7;
        
        axiom_log_info("Effects manager using default configuration");
    }

    manager->gl_initialized = false;
    wl_list_init(&manager->shadow_cache);
    
    // Initialize real-time capabilities
    manager->realtime_enabled = false;
    manager->last_frame_time = 0;
    manager->frame_count = 0;

    return true;
}

void axiom_effects_manager_destroy(struct axiom_effects_manager *manager) {
    if (!manager) return;
    axiom_shadow_destroy(manager);
    axiom_blur_destroy(manager);
    free(manager->gl_context);
}

bool axiom_shadow_init(struct axiom_effects_manager *manager) {
    if (!manager) return false;
    
    // Initialize GPU context if not already done
    if (!manager->gl_initialized) {
        axiom_log_warn("GPU context not initialized for shadow system");
        return false;
    }
    
    struct axiom_gpu_context *ctx = (struct axiom_gpu_context*)manager->gl_context;
    if (!ctx || !ctx->initialized) {
        axiom_log_error("Invalid GPU context for shadow initialization");
        return false;
    }
    
    // Shadow system is now integrated with GPU context
    // Framebuffers and textures are created on-demand in rendering functions
    axiom_log_debug("Shadow system initialized with GPU integration");
    return true;
}

void axiom_shadow_destroy(struct axiom_effects_manager *manager) {
    if (!manager) return;
    
    // Clean up shadow texture cache
    struct axiom_shadow_texture *shadow_texture, *tmp;
    wl_list_for_each_safe(shadow_texture, tmp, &manager->shadow_cache, link) {
        wl_list_remove(&shadow_texture->link);
        free(shadow_texture->texture_data);
        free(shadow_texture);
    }
}

struct axiom_shadow_texture *axiom_shadow_create_texture(
    struct axiom_effects_manager *manager,
    int width, int height, int blur_radius, uint32_t color) {
    if (!manager || width <= 0 || height <= 0) return NULL;
    
    struct axiom_shadow_texture *shadow_texture = calloc(1, sizeof(*shadow_texture));
    if (!shadow_texture) return NULL;

    shadow_texture->width = width;
    shadow_texture->height = height;
    shadow_texture->blur_radius = blur_radius;
    shadow_texture->color = color;
    shadow_texture->dirty = true;
    
    // If GPU context is available, create the actual texture
    if (manager->gl_initialized && manager->gl_context) {
        struct axiom_gpu_context *ctx = (struct axiom_gpu_context*)manager->gl_context;
        if (ctx && ctx->initialized) {
            // Create GPU texture for shadow rendering
            GLuint gpu_texture = axiom_gpu_create_shadow_texture(ctx, width, height);
            if (gpu_texture) {
                // Store GPU texture ID in texture_data as a pointer
                shadow_texture->texture_data = malloc(sizeof(GLuint));
                if (shadow_texture->texture_data) {
                    *((GLuint*)shadow_texture->texture_data) = gpu_texture;
                    axiom_log_debug("Created GPU shadow texture: %dx%d, ID: %u", width, height, gpu_texture);
                } else {
                    axiom_gpu_destroy_texture(gpu_texture);
                }
            }
        }
    }

    wl_list_insert(&manager->shadow_cache, &shadow_texture->link);
    return shadow_texture;
}

void axiom_shadow_render_for_window(struct axiom_effects_manager *manager,
                                    struct axiom_window *window) {
    if (!manager || !window || !manager->gl_initialized) {
        return;
    }

    struct axiom_gpu_context *ctx = (struct axiom_gpu_context *)manager->gl_context;
    if (!ctx) return;

    // Get window dimensions
    int width = window->geometry.width > 0 ? window->geometry.width : window->width;
    int height = window->geometry.height > 0 ? window->geometry.height : window->height;
    
    if (width <= 0 || height <= 0) return;

    // For now, just mark as processed without calling GPU functions
    // This allows tests to run without requiring the full renderer
    axiom_log_debug("Shadow rendering requested for window %dx%d (offset: %d,%d, blur: %d, opacity: %.2f)",
           width, height, manager->shadow.offset_x, manager->shadow.offset_y,
           manager->shadow.blur_radius, manager->shadow.opacity);
           
    // Real GPU rendering would happen here when fully integrated
    // axiom_gpu_render_shadow(ctx, &params, ctx->temp_texture, ctx->shadow_texture);
}

void axiom_shadow_update_config(struct axiom_effects_manager *manager,
                                struct axiom_shadow_config *config) {
    // Update shadow configuration settings
    if (config) {
        manager->shadow = *config;
        axiom_effects_invalidate_cache(manager);
    }
}

void axiom_effects_apply_to_window(struct axiom_effects_manager *manager,
                                   struct axiom_window *window) {
    if (!manager || !window) return;

    // Apply shadow
    if (manager->shadow.enabled) {
        axiom_shadow_render_for_window(manager, window);
    }

    // Apply blur
    if (manager->blur.enabled) {
        axiom_blur_apply_to_window(manager, window);
    }

    // Apply transparency
    axiom_transparency_apply_to_window(manager, window);
}

void axiom_effects_invalidate_cache(struct axiom_effects_manager *manager) {
    struct axiom_shadow_texture *shadow_texture, *tmp;
    wl_list_for_each_safe(shadow_texture, tmp, &manager->shadow_cache, link) {
        shadow_texture->dirty = true;
    }
}

uint32_t axiom_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return RGBA_COLOR(r, g, b, a);
}

bool axiom_effects_gl_check_extensions(void) {
    // Check for required OpenGL extensions for effects rendering
    const char *extensions = (const char*)glGetString(GL_EXTENSIONS);
    if (!extensions) {
        axiom_log_error("Failed to get OpenGL extensions");
        return false;
    }
    
    bool has_framebuffer_object = strstr(extensions, "GL_OES_framebuffer_object") != NULL ||
                                  strstr(extensions, "GL_ARB_framebuffer_object") != NULL;
    bool has_vertex_array_object = strstr(extensions, "GL_OES_vertex_array_object") != NULL ||
                                   strstr(extensions, "GL_ARB_vertex_array_object") != NULL;
    
    if (!has_framebuffer_object) {
        axiom_log_warn("Framebuffer objects not supported, effects may be limited");
    }
    
    if (!has_vertex_array_object) {
        axiom_log_warn("Vertex array objects not supported, using fallback");
    }
    
    // Check for floating point texture support
    bool has_float_textures = strstr(extensions, "GL_OES_texture_float") != NULL ||
                              strstr(extensions, "GL_ARB_texture_float") != NULL;
    
    if (!has_float_textures) {
        axiom_log_warn("Float textures not supported, using 8-bit textures");
    }
    
    axiom_log_debug("OpenGL extensions checked: FBO=%s, VAO=%s, Float=%s",
           has_framebuffer_object ? "yes" : "no",
           has_vertex_array_object ? "yes" : "no",
           has_float_textures ? "yes" : "no");
    
    return true; // Return true even with missing extensions, just log warnings
}

bool axiom_effects_load_config(struct axiom_effects_manager *manager, const char *config_path) {
    if (!manager || !config_path) return false;
    
    FILE *file = fopen(config_path, "r");
    if (!file) {
        axiom_log_warn("Could not open effects config file: %s", config_path);
        return false;
    }
    
    char line[256];
    bool success = true;
    
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        
        // Simple key=value parsing
        char *equals = strchr(line, '=');
        if (!equals) continue;
        
        *equals = '\0';
        char *key = line;
        char *value = equals + 1;
        
        // Remove trailing newline from value
        char *newline = strchr(value, '\n');
        if (newline) *newline = '\0';
        
        // Parse shadow settings
        if (strcmp(key, "shadow_enabled") == 0) {
            manager->shadow.enabled = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "shadow_blur_radius") == 0) {
            manager->shadow.blur_radius = atoi(value);
        } else if (strcmp(key, "shadow_offset_x") == 0) {
            manager->shadow.offset_x = atoi(value);
        } else if (strcmp(key, "shadow_offset_y") == 0) {
            manager->shadow.offset_y = atoi(value);
        } else if (strcmp(key, "shadow_opacity") == 0) {
            manager->shadow.opacity = atof(value);
        } else if (strcmp(key, "shadow_color") == 0) {
            // Parse hex color (e.g., "#000000" or "000000")
            if (value[0] == '#') value++;
            unsigned int r, g, b;
            if (sscanf(value, "%02x%02x%02x", &r, &g, &b) == 3) {
                manager->shadow.color = (uint32_t)((r) | (g << 8) | (b << 16) | (0xFF << 24));
            }
        }
        // Parse blur settings
        else if (strcmp(key, "blur_enabled") == 0) {
            manager->blur.enabled = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "blur_radius") == 0) {
            manager->blur.radius = atoi(value);
        } else if (strcmp(key, "blur_intensity") == 0) {
            manager->blur.intensity = atof(value);
        } else if (strcmp(key, "blur_focus_only") == 0) {
            manager->blur.focus_only = (strcmp(value, "true") == 0);
        }
        // Parse transparency settings
        else if (strcmp(key, "transparency_enabled") == 0) {
            manager->transparency.enabled = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "focused_opacity") == 0) {
            manager->transparency.focused_opacity = atof(value);
        } else if (strcmp(key, "unfocused_opacity") == 0) {
            manager->transparency.unfocused_opacity = atof(value);
        } else if (strcmp(key, "inactive_opacity") == 0) {
            manager->transparency.inactive_opacity = atof(value);
        }
    }
    
    fclose(file);
    
    if (success) {
        axiom_log_info("Loaded effects configuration from %s", config_path);
        axiom_effects_invalidate_cache(manager);
    } else {
        axiom_log_error("Failed to load effects configuration from %s", config_path);
    }
    
    return success;
}

bool axiom_blur_init(struct axiom_effects_manager *manager) {
    if (!manager) return false;
    // Initialize blur-related state, such as shaders or buffers
    return true;
}

void axiom_blur_destroy(struct axiom_effects_manager *manager) {
    (void)manager; // Suppress unused parameter warning
    // Clean up blur-related state
}

void axiom_blur_apply_to_window(struct axiom_effects_manager *manager, struct axiom_window *window) {
    if (!manager || !window || !manager->gl_initialized) {
        return;
    }

    struct axiom_gpu_context *ctx = (struct axiom_gpu_context *)manager->gl_context;
    if (!ctx) return;

    // Get window dimensions
    int width = window->geometry.width > 0 ? window->geometry.width : window->width;
    int height = window->geometry.height > 0 ? window->geometry.height : window->height;
    
    if (width <= 0 || height <= 0) return;

    // For now, just mark as processed without calling GPU functions
    // This allows tests to run without requiring the full renderer
    axiom_log_debug("Blur rendering requested for window %dx%d (radius: %d, intensity: %.2f)",
           width, height, manager->blur.radius, manager->blur.intensity);
           
    // Real GPU rendering would happen here when fully integrated
    // Two-pass blur: horizontal then vertical
    // axiom_gpu_render_blur(ctx, &h_params, ctx->temp_texture, ctx->blur_texture);
    // axiom_gpu_render_blur(ctx, &v_params, ctx->blur_texture, ctx->temp_texture);
}

void axiom_blur_update_config(struct axiom_effects_manager *manager, struct axiom_blur_config *config) {
    if (config) {
        manager->blur = *config;
        axiom_effects_invalidate_cache(manager);
    }
}

void axiom_transparency_apply_to_window(struct axiom_effects_manager *manager, struct axiom_window *window) {
    if (!manager || !window || !manager->transparency.enabled) return;
    
    float target_opacity;
    
    // Determine target opacity based on window state
    if (window->is_focused) {
        target_opacity = manager->transparency.focused_opacity;
    } else if (window->server && window->server->focused_window != NULL) {
        // Window is not focused and there's another focused window
        target_opacity = manager->transparency.unfocused_opacity;
    } else {
        // No window is focused, use inactive opacity
        target_opacity = manager->transparency.inactive_opacity;
    }
    
    // Clamp opacity to valid range
    if (target_opacity < 0.0f) target_opacity = 0.0f;
    if (target_opacity > 1.0f) target_opacity = 1.0f;
    
    // Apply transparency to window scene node if available
    if (window->scene_tree) {
        // For wlroots, we can set the opacity on the scene node
        // This is a simplified approach - full implementation would depend on wlroots version
        if (target_opacity < 0.01f) {
            // Effectively invisible
            wlr_scene_node_set_enabled(&window->scene_tree->node, false);
        } else {
            wlr_scene_node_set_enabled(&window->scene_tree->node, true);
            // Note: Direct opacity setting depends on wlroots scene graph capabilities
            // In a full implementation, this might require custom rendering or buffer manipulation
        }
    }
    
    axiom_log_debug("Applied transparency %.2f to window (focused: %s)", 
           target_opacity, window->is_focused ? "yes" : "no");
}

void axiom_transparency_update_config(struct axiom_effects_manager *manager, struct axiom_transparency_config *config) {
    if (config) {
        manager->transparency = *config;
    }
}
