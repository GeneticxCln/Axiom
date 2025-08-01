#ifndef AXIOM_RENDERER_H
#define AXIOM_RENDERER_H

#include <stdbool.h>
#include <stdint.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_scene.h>

// Forward declarations
struct axiom_server;
struct axiom_window;
struct axiom_effects_manager;

// GPU rendering context
struct axiom_gpu_context {
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLConfig egl_config;
    EGLSurface egl_surface;
    bool initialized;
    
    // OpenGL state
    GLuint shadow_program;
    GLuint blur_program;
    GLuint composite_program;
    
    // Framebuffers for effects
    GLuint shadow_fbo;
    GLuint blur_fbo;
    GLuint temp_fbo;
    
    // Textures
    GLuint shadow_texture;
    GLuint blur_texture;
    GLuint temp_texture;
    
    // Vertex buffer for quad rendering
    GLuint quad_vbo;
    GLuint quad_vao;
    
    // Shader uniform locations
    struct {
        GLint shadow_offset;
        GLint shadow_color;
        GLint shadow_opacity;
        GLint blur_radius;
        GLint texture_size;
        GLint blur_direction;
    } uniforms;
};

// Shadow rendering parameters
struct axiom_shadow_params {
    float offset_x, offset_y;
    float blur_radius;
    float opacity;
    float color[4]; // RGBA
    int width, height;
};

// Blur rendering parameters  
struct axiom_blur_params {
    float radius;
    float intensity;
    bool horizontal; // For two-pass blur
    int width, height;
};

// GPU renderer functions
bool axiom_gpu_context_init(struct axiom_gpu_context *ctx, struct axiom_server *server);
void axiom_gpu_context_destroy(struct axiom_gpu_context *ctx);
bool axiom_gpu_make_current(struct axiom_gpu_context *ctx);

// Shader management
bool axiom_gpu_load_shaders(struct axiom_gpu_context *ctx);
void axiom_gpu_destroy_shaders(struct axiom_gpu_context *ctx);
GLuint axiom_gpu_compile_shader(GLenum type, const char *source);
GLuint axiom_gpu_link_program(GLuint vertex_shader, GLuint fragment_shader);

// Shadow rendering
bool axiom_gpu_render_shadow(struct axiom_gpu_context *ctx, 
                              struct axiom_shadow_params *params,
                              GLuint source_texture,
                              GLuint target_texture);
GLuint axiom_gpu_create_shadow_texture(struct axiom_gpu_context *ctx,
                                       int width, int height);

// Blur rendering
bool axiom_gpu_render_blur(struct axiom_gpu_context *ctx,
                           struct axiom_blur_params *params,
                           GLuint source_texture,
                           GLuint target_texture);

// Framebuffer management
bool axiom_gpu_create_framebuffers(struct axiom_gpu_context *ctx, int width, int height);
void axiom_gpu_destroy_framebuffers(struct axiom_gpu_context *ctx);
bool axiom_gpu_resize_framebuffers(struct axiom_gpu_context *ctx, int width, int height);

// Texture utilities
GLuint axiom_gpu_create_texture(int width, int height, GLenum format);
void axiom_gpu_destroy_texture(GLuint texture);
bool axiom_gpu_upload_texture_data(GLuint texture, int width, int height, 
                                   const void *data, GLenum format);

// Rendering utilities
void axiom_gpu_render_quad(struct axiom_gpu_context *ctx);
bool axiom_gpu_setup_quad_geometry(struct axiom_gpu_context *ctx);
void axiom_gpu_set_viewport(int width, int height);

// Error handling
const char *axiom_gpu_get_error_string(GLenum error);
bool axiom_gpu_check_error(const char *operation);
void axiom_gpu_clear_errors(void);

// Integration with effects system
bool axiom_effects_gpu_init(struct axiom_effects_manager *manager, 
                             struct axiom_server *server);
void axiom_effects_gpu_render_window_shadow(struct axiom_effects_manager *manager,
                                             struct axiom_window *window);
void axiom_effects_gpu_render_window_blur(struct axiom_effects_manager *manager,
                                           struct axiom_window *window);

// Shader source constants
extern const char *axiom_shadow_vertex_shader;
extern const char *axiom_shadow_fragment_shader;
extern const char *axiom_blur_vertex_shader;
extern const char *axiom_blur_fragment_shader;
extern const char *axiom_composite_vertex_shader;
extern const char *axiom_composite_fragment_shader;

#endif // AXIOM_RENDERER_H
