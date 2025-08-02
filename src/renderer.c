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
        axiom_log_error("Failed to get EGL display");
        return false;
    }

    EGLint major, minor;
    if (!eglInitialize(ctx->egl_display, &major, &minor)) {
        axiom_log_error("Failed to initialize EGL");
        return false;
    }

    axiom_log_info("EGL initialized: %d.%d", major, minor);

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
        axiom_log_error("Failed to choose EGL config");
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
        axiom_log_error("Failed to create EGL context");
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
        axiom_log_error("Failed to create EGL surface");
        return false;
    }

    // Make context current
    if (!axiom_gpu_make_current(ctx)) {
        return false;
    }

    // Load shaders
    if (!axiom_gpu_load_shaders(ctx)) {
        axiom_log_error("Failed to load GPU shaders");
        axiom_gpu_context_destroy(ctx);
        return false;
    }
    
    // Create vertex buffer objects
    if (!axiom_gpu_create_buffers(ctx)) {
        axiom_log_error("Failed to create GPU buffers");
        axiom_gpu_context_destroy(ctx);
        return false;
    }
    
    ctx->initialized = true;
    axiom_log_info("GPU context initialized successfully");
    return true;
}



static GLuint compile_shader(const char *source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        axiom_log_error("Shader compilation failed: %s", info_log);
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

static GLuint link_program(GLuint vertex_shader, GLuint fragment_shader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(program, 512, NULL, info_log);
        axiom_log_error("Program linking failed: %s", info_log);
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}


bool axiom_gpu_create_buffers(struct axiom_gpu_context *ctx) {
    if (!ctx) return false;
    
    // Create vertex array object
    glGenVertexArrays(1, &ctx->quad_vao);
    glBindVertexArray(ctx->quad_vao);
    
    // Create vertex buffer
    glGenBuffers(1, &ctx->quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    
    // Create element buffer
    glGenBuffers(1, &ctx->quad_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx->quad_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);
    
    // Set vertex attributes
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    axiom_log_debug("GPU buffers created successfully");
    return true;
}

void axiom_gpu_context_destroy(struct axiom_gpu_context *ctx) {
    if (!ctx) return;

    if (ctx->initialized) {
        axiom_gpu_make_current(ctx);
        
        axiom_gpu_destroy_shaders(ctx);
        axiom_gpu_destroy_framebuffers(ctx);
        
        if (ctx->quad_vbo) glDeleteBuffers(1, &ctx->quad_vbo);
        if (ctx->quad_vao) glDeleteVertexArrays(1, &ctx->quad_vao);
        if (ctx->quad_ebo) glDeleteBuffers(1, &ctx->quad_ebo);
    }

    if (ctx->egl_context != EGL_NO_CONTEXT) {
        eglDestroyContext(ctx->egl_display, ctx->egl_context);
    }
    if (ctx->egl_surface != EGL_NO_SURFACE) {
        eglDestroySurface(ctx->egl_display, ctx->egl_surface);
    }
    if (ctx->egl_display != EGL_NO_DISPLAY) {
        eglTerminate(ctx->egl_display);
    }

    memset(ctx, 0, sizeof(*ctx));
}

bool axiom_gpu_make_current(struct axiom_gpu_context *ctx) {
    if (!ctx || ctx->egl_display == EGL_NO_DISPLAY) return false;
    
    if (!eglMakeCurrent(ctx->egl_display, ctx->egl_surface, ctx->egl_surface, ctx->egl_context)) {
        axiom_log_error("Failed to make EGL context current");
        return false;
    }
    
    return true;
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
        axiom_log_error("Shader compilation failed: %s", info_log);
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
        axiom_log_error("Program linking failed: %s", info_log);
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

bool axiom_gpu_load_shaders(struct axiom_gpu_context *ctx) {
    if (!ctx) return false;
    
    // Compile shadow shaders
    GLuint shadow_vs = axiom_gpu_compile_shader(GL_VERTEX_SHADER, axiom_shadow_vertex_shader);
    GLuint shadow_fs = axiom_gpu_compile_shader(GL_FRAGMENT_SHADER, axiom_shadow_fragment_shader);
    if (!shadow_vs || !shadow_fs) {
        if (shadow_vs) glDeleteShader(shadow_vs);
        if (shadow_fs) glDeleteShader(shadow_fs);
        return false;
    }

    ctx->shadow_program = axiom_gpu_link_program(shadow_vs, shadow_fs);
    glDeleteShader(shadow_vs);
    glDeleteShader(shadow_fs);
    if (!ctx->shadow_program) return false;

    // Compile blur shaders
    GLuint blur_vs = axiom_gpu_compile_shader(GL_VERTEX_SHADER, axiom_blur_vertex_shader);
    GLuint blur_fs = axiom_gpu_compile_shader(GL_FRAGMENT_SHADER, axiom_blur_fragment_shader);
    if (!blur_vs || !blur_fs) {
        if (blur_vs) glDeleteShader(blur_vs);
        if (blur_fs) glDeleteShader(blur_fs);
        return false;
    }

    ctx->blur_program = axiom_gpu_link_program(blur_vs, blur_fs);
    glDeleteShader(blur_vs);
    glDeleteShader(blur_fs);
    if (!ctx->blur_program) return false;

    // Compile composite shaders
    GLuint comp_vs = axiom_gpu_compile_shader(GL_VERTEX_SHADER, axiom_composite_vertex_shader);
    GLuint comp_fs = axiom_gpu_compile_shader(GL_FRAGMENT_SHADER, axiom_composite_fragment_shader);
    if (!comp_vs || !comp_fs) {
        if (comp_vs) glDeleteShader(comp_vs);
        if (comp_fs) glDeleteShader(comp_fs);
        return false;
    }

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

    axiom_log_debug("GPU shaders loaded successfully");
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
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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
        axiom_log_error("OpenGL error in %s: %s", operation, axiom_gpu_get_error_string(error));
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
    if (!ctx || width <= 0 || height <= 0) return false;
    
    // Store dimensions
    ctx->framebuffer_width = width;
    ctx->framebuffer_height = height;
    
    // Create shadow framebuffer and texture
    glGenFramebuffers(1, &ctx->shadow_fbo);
    ctx->shadow_texture = axiom_gpu_create_texture(width, height, GL_RGBA8);
    if (!ctx->shadow_texture) {
        axiom_log_error("Failed to create shadow texture");
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, ctx->shadow_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 
                          ctx->shadow_texture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        axiom_log_error("Shadow framebuffer incomplete");
        axiom_gpu_destroy_framebuffers(ctx);
        return false;
    }
    
    // Create blur framebuffer and texture
    glGenFramebuffers(1, &ctx->blur_fbo);
    ctx->blur_texture = axiom_gpu_create_texture(width, height, GL_RGBA8);
    if (!ctx->blur_texture) {
        axiom_log_error("Failed to create blur texture");
        axiom_gpu_destroy_framebuffers(ctx);
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, ctx->blur_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 
                          ctx->blur_texture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        axiom_log_error("Blur framebuffer incomplete");
        axiom_gpu_destroy_framebuffers(ctx);
        return false;
    }
    
    // Create temporary framebuffer and texture for multi-pass effects
    glGenFramebuffers(1, &ctx->temp_fbo);
    ctx->temp_texture = axiom_gpu_create_texture(width, height, GL_RGBA8);
    if (!ctx->temp_texture) {
        axiom_log_error("Failed to create temp texture");
        axiom_gpu_destroy_framebuffers(ctx);
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, ctx->temp_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 
                          ctx->temp_texture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        axiom_log_error("Temp framebuffer incomplete");
        axiom_gpu_destroy_framebuffers(ctx);
        return false;
    }
    
    // Restore default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    axiom_log_debug("GPU framebuffers created: %dx%d", width, height);
    return axiom_gpu_check_error("create framebuffers");
}

void axiom_gpu_destroy_framebuffers(struct axiom_gpu_context *ctx) {
    if (!ctx) return;
    
    // Delete framebuffers
    if (ctx->shadow_fbo) {
        glDeleteFramebuffers(1, &ctx->shadow_fbo);
        ctx->shadow_fbo = 0;
    }
    if (ctx->blur_fbo) {
        glDeleteFramebuffers(1, &ctx->blur_fbo);
        ctx->blur_fbo = 0;
    }
    if (ctx->temp_fbo) {
        glDeleteFramebuffers(1, &ctx->temp_fbo);
        ctx->temp_fbo = 0;
    }
    
    // Delete textures
    if (ctx->shadow_texture) {
        glDeleteTextures(1, &ctx->shadow_texture);
        ctx->shadow_texture = 0;
    }
    if (ctx->blur_texture) {
        glDeleteTextures(1, &ctx->blur_texture);
        ctx->blur_texture = 0;
    }
    if (ctx->temp_texture) {
        glDeleteTextures(1, &ctx->temp_texture);
        ctx->temp_texture = 0;
    }
    
    ctx->framebuffer_width = 0;
    ctx->framebuffer_height = 0;
    
    axiom_log_debug("GPU framebuffers destroyed");
}

bool axiom_gpu_resize_framebuffers(struct axiom_gpu_context *ctx, int width, int height) {
    if (!ctx || width <= 0 || height <= 0) return false;
    
    // If dimensions haven't changed, no need to resize
    if (ctx->framebuffer_width == width && ctx->framebuffer_height == height) {
        return true;
    }
    
    axiom_log_debug("Resizing framebuffers from %dx%d to %dx%d", 
           ctx->framebuffer_width, ctx->framebuffer_height, width, height);
    
    // Destroy existing framebuffers
    axiom_gpu_destroy_framebuffers(ctx);
    
    // Create new framebuffers with new dimensions
    return axiom_gpu_create_framebuffers(ctx, width, height);
}

bool axiom_gpu_upload_texture_data(GLuint texture, int width, int height, 
                                   const void *data, GLenum format) {
    if (!texture || width <= 0 || height <= 0) return false;
    
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Determine the appropriate internal format based on the data format
    GLenum internal_format;
    GLenum data_format;
    GLenum data_type = GL_UNSIGNED_BYTE;
    
    switch (format) {
        case GL_RGBA:
            internal_format = GL_RGBA8;
            data_format = GL_RGBA;
            break;
        case GL_RGB:
            internal_format = GL_RGB8;
            data_format = GL_RGB;
            break;
        case GL_LUMINANCE:
            internal_format = GL_R8;
            data_format = GL_RED;
            break;
        case GL_ALPHA:
            internal_format = GL_R8;
            data_format = GL_RED;
            break;
        default:
            internal_format = GL_RGBA8;
            data_format = GL_RGBA;
            break;
    }
    
    // Upload the texture data
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, 
                 data_format, data_type, data);
    
    // Set texture parameters for proper filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    bool success = axiom_gpu_check_error("upload texture data");
    if (success) {
        axiom_log_debug("Uploaded texture data: %dx%d, format: 0x%x", width, height, format);
    }
    
    return success;
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
    if (!manager || !window || !manager->gl_initialized || !manager->shadow.enabled) return;
    
    struct axiom_gpu_context *ctx = (struct axiom_gpu_context*)manager->gl_context;
    if (!ctx || !ctx->initialized) return;
    
    // Get window dimensions
    int width = window->geometry.width > 0 ? window->geometry.width : window->width;
    int height = window->geometry.height > 0 ? window->geometry.height : window->height;
    
    if (width <= 0 || height <= 0) return;
    
    // Ensure framebuffers are sized correctly
    if (ctx->framebuffer_width != width || ctx->framebuffer_height != height) {
        if (!axiom_gpu_resize_framebuffers(ctx, width, height)) {
            axiom_log_error("Failed to resize framebuffers for shadow rendering");
            return;
        }
    }
    
    // Create shadow parameters from configuration
    struct axiom_shadow_params params = {
        .offset_x = (float)manager->shadow.offset_x,
        .offset_y = (float)manager->shadow.offset_y,
        .blur_radius = (float)manager->shadow.blur_radius,
        .opacity = manager->shadow.opacity,
        .width = width,
        .height = height
    };
    
    // Extract RGBA components from shadow color
    uint32_t color = manager->shadow.color;
    params.color[0] = ((color >> 0) & 0xFF) / 255.0f;  // R
    params.color[1] = ((color >> 8) & 0xFF) / 255.0f;  // G
    params.color[2] = ((color >> 16) & 0xFF) / 255.0f; // B
    params.color[3] = ((color >> 24) & 0xFF) / 255.0f; // A
    
    // Try to capture window content as source texture
    GLuint window_texture = 0;
    if (window->surface) {
        // Create a simple colored rectangle as fallback for now
        // In a full implementation, this would capture the actual window content
        window_texture = axiom_gpu_create_texture(width, height, GL_RGBA8);
        if (window_texture) {
            // Fill with a simple pattern for testing
            GLubyte *test_data = malloc(width * height * 4);
            if (test_data) {
                for (int i = 0; i < width * height; i++) {
                    test_data[i*4 + 0] = 128; // R
                    test_data[i*4 + 1] = 128; // G
                    test_data[i*4 + 2] = 200; // B
                    test_data[i*4 + 3] = 255; // A
                }
                axiom_gpu_upload_texture_data(window_texture, width, height, test_data, GL_RGBA);
                free(test_data);
            }
        }
    }
    
    if (window_texture) {
        // Bind shadow framebuffer as render target
        glBindFramebuffer(GL_FRAMEBUFFER, ctx->shadow_fbo);
        
        // Render shadow effect
        if (axiom_gpu_render_shadow(ctx, &params, window_texture, ctx->shadow_texture)) {
            axiom_log_debug("Shadow rendered for window %dx%d", width, height);
        } else {
            axiom_log_warn("Failed to render shadow for window");
        }
        
        // Restore default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        // Clean up temporary texture
        axiom_gpu_destroy_texture(window_texture);
    }
}

void axiom_effects_gpu_render_window_blur(struct axiom_effects_manager *manager,
                                           struct axiom_window *window) {
    if (!manager || !window || !manager->gl_initialized || !manager->blur.enabled) return;
    
    struct axiom_gpu_context *ctx = (struct axiom_gpu_context*)manager->gl_context;
    if (!ctx || !ctx->initialized) return;
    
    // Get window dimensions
    int width = window->geometry.width > 0 ? window->geometry.width : window->width;
    int height = window->geometry.height > 0 ? window->geometry.height : window->height;
    
    if (width <= 0 || height <= 0) return;
    
    // Ensure framebuffers are sized correctly
    if (ctx->framebuffer_width != width || ctx->framebuffer_height != height) {
        if (!axiom_gpu_resize_framebuffers(ctx, width, height)) {
            axiom_log_error("Failed to resize framebuffers for blur rendering");
            return;
        }
    }
    
    // Try to capture window content as source texture
    GLuint window_texture = 0;
    if (window->surface) {
        // Create a simple colored rectangle as fallback for now
        window_texture = axiom_gpu_create_texture(width, height, GL_RGBA8);
        if (window_texture) {
            // Fill with a gradient pattern for testing blur
            GLubyte *test_data = malloc(width * height * 4);
            if (test_data) {
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        int idx = (y * width + x) * 4;
                        test_data[idx + 0] = (x * 255) / width;     // R gradient
                        test_data[idx + 1] = (y * 255) / height;    // G gradient
                        test_data[idx + 2] = 150;                   // B constant
                        test_data[idx + 3] = 200;                   // A semi-transparent
                    }
                }
                axiom_gpu_upload_texture_data(window_texture, width, height, test_data, GL_RGBA);
                free(test_data);
            }
        }
    }
    
    if (window_texture) {
        // Two-pass blur: horizontal then vertical
        struct axiom_blur_params h_params = {
            .radius = (float)manager->blur.radius,
            .intensity = manager->blur.intensity,
            .horizontal = true,
            .width = width,
            .height = height
        };
        
        struct axiom_blur_params v_params = {
            .radius = (float)manager->blur.radius,
            .intensity = manager->blur.intensity,
            .horizontal = false,
            .width = width,
            .height = height
        };
        
        // Horizontal pass: window_texture -> temp_texture
        glBindFramebuffer(GL_FRAMEBUFFER, ctx->temp_fbo);
        if (axiom_gpu_render_blur(ctx, &h_params, window_texture, ctx->temp_texture)) {
            // Vertical pass: temp_texture -> blur_texture
            glBindFramebuffer(GL_FRAMEBUFFER, ctx->blur_fbo);
            if (axiom_gpu_render_blur(ctx, &v_params, ctx->temp_texture, ctx->blur_texture)) {
                axiom_log_debug("Blur rendered for window %dx%d", width, height);
            } else {
                axiom_log_warn("Failed to render vertical blur pass");
            }
        } else {
            axiom_log_warn("Failed to render horizontal blur pass");
        }
        
        // Restore default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        // Clean up temporary texture
        axiom_gpu_destroy_texture(window_texture);
    }
}
