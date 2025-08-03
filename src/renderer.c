#include "renderer.h"
#include "axiom.h"
#include "effects.h"
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Shader sources
const char *axiom_shadow_vertex_shader = 
    "#version 300 es\n"
    "precision mediump float;\n"
    "layout(location = 0) in vec2 position;\n"
    "layout(location = 1) in vec2 texcoord;\n"
    "out vec2 v_texcoord;\n"
    "void main() {\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "    v_texcoord = texcoord;\n"
    "}\n";

const char *axiom_shadow_fragment_shader =
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform sampler2D u_texture;\n"
    "uniform vec2 u_shadow_offset;\n"
    "uniform vec4 u_shadow_color;\n"
    "uniform float u_shadow_opacity;\n"
    "uniform float u_blur_radius;\n"
    "uniform vec2 u_texture_size;\n"
    "in vec2 v_texcoord;\n"
    "out vec4 fragColor;\n"
    "\n"
    "void main() {\n"
    "    vec2 offset_uv = v_texcoord + u_shadow_offset / u_texture_size;\n"
    "    vec4 shadow = vec4(0.0);\n"
    "    \n"
    "    // Simple box blur for shadow\n"
    "    float blur_size = u_blur_radius / u_texture_size.x;\n"
    "    int samples = int(u_blur_radius);\n"
    "    float total_weight = 0.0;\n"
    "    \n"
    "    for (int x = -samples; x <= samples; x++) {\n"
    "        for (int y = -samples; y <= samples; y++) {\n"
    "            vec2 sample_uv = offset_uv + vec2(float(x), float(y)) * blur_size;\n"
    "            float alpha = texture(u_texture, sample_uv).a;\n"
    "            float weight = 1.0 - (length(vec2(x, y)) / float(samples));\n"
    "            shadow.a += alpha * weight;\n"
    "            total_weight += weight;\n"
    "        }\n"
    "    }\n"
    "    \n"
    "    shadow.a /= total_weight;\n"
    "    shadow.rgb = u_shadow_color.rgb;\n"
    "    shadow.a *= u_shadow_opacity;\n"
    "    \n"
    "    fragColor = shadow;\n"
    "}\n";

const char *axiom_blur_vertex_shader =
    "#version 300 es\n"
    "precision mediump float;\n"
    "layout(location = 0) in vec2 position;\n"
    "layout(location = 1) in vec2 texcoord;\n"
    "out vec2 v_texcoord;\n"
    "void main() {\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "    v_texcoord = texcoord;\n"
    "}\n";

const char *axiom_blur_fragment_shader =
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform sampler2D u_texture;\n"
    "uniform float u_blur_radius;\n"
    "uniform vec2 u_blur_direction;\n"
    "uniform vec2 u_texture_size;\n"
    "in vec2 v_texcoord;\n"
    "out vec4 fragColor;\n"
    "\n"
    "void main() {\n"
    "    vec4 color = vec4(0.0);\n"
    "    vec2 blur_step = u_blur_direction / u_texture_size;\n"
    "    int samples = int(u_blur_radius);\n"
    "    float total_weight = 0.0;\n"
    "    \n"
    "    // Gaussian blur\n"
    "    for (int i = -samples; i <= samples; i++) {\n"
    "        vec2 sample_uv = v_texcoord + float(i) * blur_step;\n"
    "        float weight = exp(-0.5 * pow(float(i) / (u_blur_radius * 0.3), 2.0));\n"
    "        color += texture(u_texture, sample_uv) * weight;\n"
    "        total_weight += weight;\n"
    "    }\n"
    "    \n"
    "    fragColor = color / total_weight;\n"
    "}\n";

const char *axiom_composite_vertex_shader =
    "#version 300 es\n"
    "precision mediump float;\n"
    "layout(location = 0) in vec2 position;\n"
    "layout(location = 1) in vec2 texcoord;\n"
    "out vec2 v_texcoord;\n"
    "void main() {\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "    v_texcoord = texcoord;\n"
    "}\n";

const char *axiom_composite_fragment_shader =
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform sampler2D u_texture;\n"
    "in vec2 v_texcoord;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    fragColor = texture(u_texture, v_texcoord);\n"
    "}\n";

// Quad vertices for fullscreen rendering
static const float quad_vertices[] = {
    -1.0f, -1.0f, 0.0f, 0.0f,  // bottom-left
     1.0f, -1.0f, 1.0f, 0.0f,  // bottom-right
     1.0f,  1.0f, 1.0f, 1.0f,  // top-right
    -1.0f,  1.0f, 0.0f, 1.0f   // top-left
};


bool axiom_gpu_context_init(struct axiom_gpu_context *ctx, struct axiom_server *server) {
    if (!ctx || !server) return false;
    memset(ctx, 0, sizeof(*ctx));

    // Initialize EGL
    ctx->egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (ctx->egl_display == EGL_NO_DISPLAY) {
        AXIOM_LOG_ERROR("Failed to get EGL display");
        return false;
    }

    EGLint major, minor;
    if (!eglInitialize(ctx->egl_display, &major, &minor)) {
        AXIOM_LOG_ERROR("Failed to initialize EGL");
        return false;
    }

    AXIOM_LOG_INFO("EGL initialized: %d.%d", major, minor);

    // Choose EGL config
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };

    EGLint num_configs;
    if (!eglChooseConfig(ctx->egl_display, config_attribs, &ctx->egl_config, 1, &num_configs)) {
        AXIOM_LOG_ERROR("Failed to choose EGL config");
        return false;
    }

    // Create EGL context
    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    ctx->egl_context = eglCreateContext(ctx->egl_display, ctx->egl_config, 
                                        EGL_NO_CONTEXT, context_attribs);
    if (ctx->egl_context == EGL_NO_CONTEXT) {
        AXIOM_LOG_ERROR("Failed to create EGL context");
        return false;
    }

    // Create a dummy pbuffer surface for off-screen rendering
    EGLint pbuffer_attribs[] = {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_NONE
    };

    ctx->egl_surface = eglCreatePbufferSurface(ctx->egl_display, ctx->egl_config, pbuffer_attribs);
    if (ctx->egl_surface == EGL_NO_SURFACE) {
        AXIOM_LOG_ERROR("Failed to create EGL surface");
        return false;
    }

    // Make context current
    if (!axiom_gpu_make_current(ctx)) {
        return false;
    }

    // Load shaders
    if (!axiom_gpu_load_shaders(ctx)) {
        AXIOM_LOG_ERROR("Failed to load shaders");
        return false;
    }

    // Setup quad geometry
    if (!axiom_gpu_setup_quad_geometry(ctx)) {
        AXIOM_LOG_ERROR("Failed to setup quad geometry");
        return false;
    }

    ctx->initialized = true;
    AXIOM_LOG_INFO("GPU context initialized successfully");
    return true;
}

void axiom_gpu_context_destroy(struct axiom_gpu_context *ctx) {
    if (!ctx || !ctx->initialized) return;

    axiom_gpu_make_current(ctx);
    
    axiom_gpu_destroy_shaders(ctx);
    axiom_gpu_destroy_framebuffers(ctx);
    
    if (ctx->quad_vbo) glDeleteBuffers(1, &ctx->quad_vbo);
    if (ctx->quad_vao) glDeleteVertexArrays(1, &ctx->quad_vao);

    if (ctx->egl_context != EGL_NO_CONTEXT) {
        eglDestroyContext(ctx->egl_display, ctx->egl_context);
    }
    if (ctx->egl_surface != EGL_NO_SURFACE) {
        eglDestroySurface(ctx->egl_display, ctx->egl_surface);
    }
    if (ctx->egl_display != EGL_NO_DISPLAY) {
        eglTerminate(ctx->egl_display);
    }

    ctx->initialized = false;
}

bool axiom_gpu_make_current(struct axiom_gpu_context *ctx) {
    if (!ctx) return false;
    return eglMakeCurrent(ctx->egl_display, ctx->egl_surface, 
                         ctx->egl_surface, ctx->egl_context);
}

GLuint axiom_gpu_compile_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        AXIOM_LOG_ERROR("Shader compilation failed: %s", info_log);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint axiom_gpu_link_program(GLuint vertex_shader, GLuint fragment_shader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(program, 512, NULL, info_log);
        AXIOM_LOG_ERROR("Program linking failed: %s", info_log);
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

bool axiom_gpu_load_shaders(struct axiom_gpu_context *ctx) {
    // Compile shadow shaders
    GLuint shadow_vs = axiom_gpu_compile_shader(GL_VERTEX_SHADER, axiom_shadow_vertex_shader);
    GLuint shadow_fs = axiom_gpu_compile_shader(GL_FRAGMENT_SHADER, axiom_shadow_fragment_shader);
    if (!shadow_vs || !shadow_fs) return false;

    ctx->shadow_program = axiom_gpu_link_program(shadow_vs, shadow_fs);
    glDeleteShader(shadow_vs);
    glDeleteShader(shadow_fs);
    if (!ctx->shadow_program) return false;

    // Compile blur shaders
    GLuint blur_vs = axiom_gpu_compile_shader(GL_VERTEX_SHADER, axiom_blur_vertex_shader);
    GLuint blur_fs = axiom_gpu_compile_shader(GL_FRAGMENT_SHADER, axiom_blur_fragment_shader);
    if (!blur_vs || !blur_fs) return false;

    ctx->blur_program = axiom_gpu_link_program(blur_vs, blur_fs);
    glDeleteShader(blur_vs);
    glDeleteShader(blur_fs);
    if (!ctx->blur_program) return false;

    // Compile composite shaders
    GLuint comp_vs = axiom_gpu_compile_shader(GL_VERTEX_SHADER, axiom_composite_vertex_shader);
    GLuint comp_fs = axiom_gpu_compile_shader(GL_FRAGMENT_SHADER, axiom_composite_fragment_shader);
    if (!comp_vs || !comp_fs) return false;

    ctx->composite_program = axiom_gpu_link_program(comp_vs, comp_fs);
    glDeleteShader(comp_vs);
    glDeleteShader(comp_fs);
    if (!ctx->composite_program) return false;

    // Get uniform locations
    ctx->uniforms.shadow_offset = glGetUniformLocation(ctx->shadow_program, "u_shadow_offset");
    ctx->uniforms.shadow_color = glGetUniformLocation(ctx->shadow_program, "u_shadow_color");
    ctx->uniforms.shadow_opacity = glGetUniformLocation(ctx->shadow_program, "u_shadow_opacity");
    ctx->uniforms.blur_radius = glGetUniformLocation(ctx->blur_program, "u_blur_radius");
    ctx->uniforms.texture_size = glGetUniformLocation(ctx->blur_program, "u_texture_size");
    ctx->uniforms.blur_direction = glGetUniformLocation(ctx->blur_program, "u_blur_direction");

    return true;
}

void axiom_gpu_destroy_shaders(struct axiom_gpu_context *ctx) {
    if (ctx->shadow_program) glDeleteProgram(ctx->shadow_program);
    if (ctx->blur_program) glDeleteProgram(ctx->blur_program);
    if (ctx->composite_program) glDeleteProgram(ctx->composite_program);
}

bool axiom_gpu_setup_quad_geometry(struct axiom_gpu_context *ctx) {
    glGenVertexArrays(1, &ctx->quad_vao);
    glGenBuffers(1, &ctx->quad_vbo);

    glBindVertexArray(ctx->quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return axiom_gpu_check_error("setup quad geometry");
}

void axiom_gpu_composite_layers(struct axiom_gpu_context *ctx, GLuint *textures, int layer_count) {
    if (!ctx || !textures || layer_count <= 0) return;

    glUseProgram(ctx->composite_program);
    for (int i = 0; i < layer_count; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glUniform1i(glGetUniformLocation(ctx->composite_program, "u_texture"), i);

        axiom_gpu_render_quad(ctx);
    }
    glUseProgram(0);
}

void axiom_gpu_render_quad(struct axiom_gpu_context *ctx) {
    glBindVertexArray(ctx->quad_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

GLuint axiom_gpu_create_texture(int width, int height, GLenum format) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

void axiom_gpu_destroy_texture(GLuint texture) {
    if (texture) glDeleteTextures(1, &texture);
}

bool axiom_gpu_check_error(const char *operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        AXIOM_LOG_ERROR("OpenGL error in %s: %s", operation, axiom_gpu_get_error_string(error));
        return false;
    }
    return true;
}

const char *axiom_gpu_get_error_string(GLenum error) {
    switch (error) {
        case GL_NO_ERROR: return "No error";
        case GL_INVALID_ENUM: return "Invalid enum";
        case GL_INVALID_VALUE: return "Invalid value";
        case GL_INVALID_OPERATION: return "Invalid operation";
        case GL_OUT_OF_MEMORY: return "Out of memory";
        default: return "Unknown error";
    }
}

void axiom_gpu_clear_errors(void) {
    while (glGetError() != GL_NO_ERROR) {}
}

// Actual shadow rendering implementation
bool axiom_gpu_render_shadow(struct axiom_gpu_context *ctx,
                              struct axiom_shadow_params *params,
                              GLuint source_texture,
                              GLuint target_texture) {
    (void)target_texture; // Suppress unused parameter warning
    if (!ctx || !params || !source_texture) {
        return false;
    }
    
    // Save current OpenGL state
    GLint prev_fbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    
    // Use shadow shader program
    glUseProgram(ctx->shadow_program);
    
    // Set viewport for shadow rendering
    glViewport(0, 0, params->width, params->height);
    
    // Bind source texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source_texture);
    glUniform1i(glGetUniformLocation(ctx->shadow_program, "u_texture"), 0);
    
    // Set shadow uniforms
    glUniform2f(ctx->uniforms.shadow_offset, params->offset_x, params->offset_y);
    glUniform4fv(ctx->uniforms.shadow_color, 1, params->color);
    glUniform1f(ctx->uniforms.shadow_opacity, params->opacity);
    glUniform1f(glGetUniformLocation(ctx->shadow_program, "u_blur_radius"), params->blur_radius);
    glUniform2f(glGetUniformLocation(ctx->shadow_program, "u_texture_size"), 
                (float)params->width, (float)params->height);
    
    // Enable blending for shadow transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Render shadow quad
    axiom_gpu_render_quad(ctx);
    
    // Restore OpenGL state
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
    glUseProgram(0);
    
    return axiom_gpu_check_error("shadow rendering");
}

bool axiom_gpu_render_blur(struct axiom_gpu_context *ctx,
                           struct axiom_blur_params *params,
                           GLuint source_texture,
                           GLuint target_texture) {
    (void)target_texture; // Suppress unused parameter warning
    if (!ctx || !params || !source_texture) {
        return false;
    }
    
    // Save current OpenGL state
    GLint prev_fbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    
    // Determine blur direction for two-pass blur
    float blur_direction[2] = {0.0f, 0.0f};
    if (params->horizontal) {
        blur_direction[0] = 1.0f;  // Horizontal pass
    } else {
        blur_direction[1] = 1.0f;  // Vertical pass
    }
    
    // Use blur shader program
    glUseProgram(ctx->blur_program);
    
    // Set viewport for blur rendering
    glViewport(0, 0, params->width, params->height);
    
    // Bind source texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source_texture);
    glUniform1i(glGetUniformLocation(ctx->blur_program, "u_texture"), 0);
    
    // Set blur uniforms
    glUniform1f(ctx->uniforms.blur_radius, params->radius * params->intensity);
    glUniform2fv(ctx->uniforms.blur_direction, 1, blur_direction);
    glUniform2f(ctx->uniforms.texture_size, (float)params->width, (float)params->height);
    
    // Enable blending for smooth blur compositing
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Clear the target with transparent black
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Render blur quad
    axiom_gpu_render_quad(ctx);
    
    // Restore OpenGL state
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
    glUseProgram(0);
    
    return axiom_gpu_check_error("blur rendering");
}

// Framebuffer management
bool axiom_gpu_create_framebuffers(struct axiom_gpu_context *ctx, int width, int height) {
    if (!ctx) return false;
    
    ctx->framebuffer_width = width;
    ctx->framebuffer_height = height;
    
    glGenFramebuffers(1, &ctx->shadow_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, ctx->shadow_fbo);
    ctx->shadow_texture = axiom_gpu_create_texture(width, height, GL_RGBA8);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx->shadow_texture, 0);
    
    glGenFramebuffers(1, &ctx->blur_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, ctx->blur_fbo);
    ctx->blur_texture = axiom_gpu_create_texture(width, height, GL_RGBA8);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx->blur_texture, 0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return axiom_gpu_check_error("creating framebuffers");
}

void axiom_gpu_destroy_framebuffers(struct axiom_gpu_context *ctx) {
    if (!ctx) return;
    
    if (ctx->shadow_fbo) {
        glDeleteFramebuffers(1, &ctx->shadow_fbo);
        axiom_gpu_destroy_texture(ctx->shadow_texture);
    }
    if (ctx->blur_fbo) {
        glDeleteFramebuffers(1, &ctx->blur_fbo);
        axiom_gpu_destroy_texture(ctx->blur_texture);
    }
}

bool axiom_gpu_resize_framebuffers(struct axiom_gpu_context *ctx, int width, int height) {
    (void)ctx; (void)width; (void)height; // Suppress unused parameter warnings
    // Resize framebuffers
    // Placeholder implementation
    return true;
}

bool axiom_gpu_upload_texture_data(GLuint texture, int width, int height, 
                                   const void *data, GLenum format) {
    if (!data) return false;

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return axiom_gpu_check_error("uploading texture data");
}

void axiom_gpu_set_viewport(int width, int height) {
    glViewport(0, 0, width, height);
}

GLuint axiom_gpu_create_shadow_texture(struct axiom_gpu_context *ctx,
                                       int width, int height) {
    (void)ctx; // Suppress unused parameter warning
    return axiom_gpu_create_texture(width, height, GL_RGBA8);
}

// Integration functions
bool axiom_effects_gpu_init(struct axiom_effects_manager *manager, 
                             struct axiom_server *server) {
    if (!manager || !server) return false;
    
    manager->gl_context = calloc(1, sizeof(struct axiom_gpu_context));
    if (!manager->gl_context) return false;
    
    if (!axiom_gpu_context_init((struct axiom_gpu_context*)manager->gl_context, server)) {
        free(manager->gl_context);
        manager->gl_context = NULL;
        return false;
    }
    
    manager->gl_initialized = true;
    return true;
}

void axiom_effects_gpu_render_window_shadow(struct axiom_effects_manager *manager,
                                             struct axiom_window *window) {
    // Placeholder for actual window shadow rendering
    if (!manager || !window || !manager->gl_initialized) return;
    
    // This would render shadows for the specific window
    // Integration with wlroots scene graph would happen here
}

void axiom_effects_gpu_render_window_blur(struct axiom_effects_manager *manager,
                                           struct axiom_window *window) {
    // Placeholder for actual window blur rendering  
    if (!manager || !window || !manager->gl_initialized) return;
    
    // This would render blur effects for the specific window
    // Integration with wlroots scene graph would happen here
}

// Hardware cursor support
bool axiom_gpu_render_cursor(struct axiom_gpu_context *ctx, GLuint cursor_texture, 
                             int x, int y, int width, int height) {
    if (!ctx || !cursor_texture) return false;
    
    // Save current state
    GLint prev_viewport[4];
    glGetIntegerv(GL_VIEWPORT, prev_viewport);
    
    glUseProgram(ctx->composite_program);
    
    // Set up cursor rendering viewport
    glViewport(x, y, width, height);
    
    // Bind cursor texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cursor_texture);
    glUniform1i(glGetUniformLocation(ctx->composite_program, "u_texture"), 0);
    
    // Enable blending for cursor transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Render cursor quad
    axiom_gpu_render_quad(ctx);
    
    // Restore state
    glDisable(GL_BLEND);
    glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);
    glUseProgram(0);
    
    return axiom_gpu_check_error("cursor rendering");
}

GLuint axiom_gpu_upload_cursor_texture(const void *cursor_data, int width, int height) {
    if (!cursor_data) return 0;
    
    GLuint texture = axiom_gpu_create_texture(width, height, GL_RGBA8);
    if (texture) {
        axiom_gpu_upload_texture_data(texture, width, height, cursor_data, GL_RGBA);
    }
    
    return texture;
}

// VSync and presentation timing
bool axiom_gpu_enable_vsync(struct axiom_gpu_context *ctx, bool enable) {
    if (!ctx) return false;
    
    // Configure EGL swap interval for VSync
    return eglSwapInterval(ctx->egl_display, enable ? 1 : 0);
}

bool axiom_gpu_present_frame(struct axiom_gpu_context *ctx) {
    if (!ctx) return false;
    
    // Present the rendered frame with VSync
    return eglSwapBuffers(ctx->egl_display, ctx->egl_surface);
}
