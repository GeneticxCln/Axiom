// Shader sources for testing (extracted from renderer.c)

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

// Error handling function
const char *axiom_gpu_get_error_string(unsigned int error) {
    switch (error) {
        case 0x0000: return "No error";           // GL_NO_ERROR
        case 0x0500: return "Invalid enum";       // GL_INVALID_ENUM
        case 0x0501: return "Invalid value";      // GL_INVALID_VALUE
        case 0x0502: return "Invalid operation";  // GL_INVALID_OPERATION
        case 0x0505: return "Out of memory";      // GL_OUT_OF_MEMORY
        default: return "Unknown error";
    }
}
