#include "renderer.h"
#include "axiom.h"
#include "effects.h"
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

static const unsigned int quad_indices[] = {
    0, 1, 2,  // first triangle
    0, 2, 3   // second triangle
};

bool axiom_gpu_context_init(struct axiom_gpu_context *ctx, struct axiom_server *server) {
    if (!ctx || !server) return false;
    memset(ctx, 0, sizeof(*ctx));

    // Initialize EGL
    ctx->egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (ctx->egl_display == EGL_NO_DISPLAY) {
        printf("Failed to get EGL display\n");
        return false;
    }

    EGLint major, minor;
    if (!eglInitialize(ctx->egl_display, &major, &minor)) {
        printf("Failed to initialize EGL\n");
        return false;
    }

    printf("EGL initialized: %d.%d\n", major, minor);

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
        printf("Failed to choose EGL config\n");
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
        printf("Failed to create EGL context\n");
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
        printf("Failed to create EGL surface\n");
        return false;
    }

    // Make context current
    if (!axiom_gpu_make_current(ctx)) {
        return false;
    }

    // Load shaders
    if (!axiom_gpu_load_shaders(ctx)) {
        printf("Failed to load shaders\n");
        return false;
    }

    // Setup quad geometry
    if (!axiom_gpu_setup_quad_geometry(ctx)) {
        printf("Failed to setup quad geometry\n");
        return false;
    }

    ctx->initialized = true;
    printf("GPU context initialized successfully\n");
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
        printf("Shader compilation failed: %s\n", info_log);
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
        printf("Program linking failed: %s\n", info_log);
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
        printf("OpenGL error in %s: %s\n", operation, axiom_gpu_get_error_string(error));
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

// Placeholder implementations for full effect rendering
bool axiom_gpu_render_shadow(struct axiom_gpu_context *ctx, 
                              struct axiom_shadow_params *params,
                              GLuint source_texture,
                              GLuint target_texture) {
    // This would implement actual shadow rendering
    // For now, return success to indicate the framework is ready
    return true;
}

bool axiom_gpu_render_blur(struct axiom_gpu_context *ctx,
                           struct axiom_blur_params *params,
                           GLuint source_texture,
                           GLuint target_texture) {
    // This would implement actual blur rendering
    // For now, return success to indicate the framework is ready
    return true;
}

// Framebuffer management
bool axiom_gpu_create_framebuffers(struct axiom_gpu_context *ctx, int width, int height) {
    // Create framebuffers for effects rendering
    // Placeholder implementation
    return true;
}

void axiom_gpu_destroy_framebuffers(struct axiom_gpu_context *ctx) {
    // Destroy framebuffers
    // Placeholder implementation
}

bool axiom_gpu_resize_framebuffers(struct axiom_gpu_context *ctx, int width, int height) {
    // Resize framebuffers
    // Placeholder implementation
    return true;
}

bool axiom_gpu_upload_texture_data(GLuint texture, int width, int height, 
                                   const void *data, GLenum format) {
    // Upload texture data
    // Placeholder implementation
    return true;
}

void axiom_gpu_set_viewport(int width, int height) {
    glViewport(0, 0, width, height);
}

GLuint axiom_gpu_create_shadow_texture(struct axiom_gpu_context *ctx,
                                       int width, int height) {
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
