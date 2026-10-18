#pragma once

/******************************************************************************************************************************
 * OpenGL 3.3 Core rendering backend for the Field GUI framework.
 *
 * Implements the common renderer interface (see comment below) so that
 * field_glfw.h can use either this backend or vulkan_backend.h, selected
 * at build time via the LOOM_RENDERER CMake option.
 *
 * The CPU-side scene rendering (Frame pixel buffers in 0xRRGGBBAA) is
 * unchanged — this backend handles GPU presentation of those buffers
 * plus line-strip overlays (oscillograph stream, connection cords).
 *
 * Common renderer interface (both backends implement these):
 *
 *   typedef struct RendererContext RendererContext;
 *   static bool renderer_init(RendererContext*, GLFWwindow*, uint32_t w, uint32_t h);
 *   static void renderer_upload_layer(RendererContext*, int layer, const uint32_t* px, uint32_t w, uint32_t h);
 *   static bool renderer_begin_frame(RendererContext*, uint32_t* image_index);
 *   static void renderer_update_layer_rect(RendererContext*, int layer, const uint32_t* px,
 *                                          uint32_t w, uint32_t h, uint32_t x, uint32_t y, uint32_t rw, uint32_t rh);
 *   static void renderer_begin_render(RendererContext*, uint32_t image_index);
 *   static void renderer_draw_layer(RendererContext*, int layer);
 *   static void renderer_draw_line(RendererContext*, const float* data, uint32_t count,
 *                                  uint32_t color, int32_t sl, int32_t st, uint32_t sw, uint32_t sh);
 *   static void renderer_end_frame(RendererContext*, uint32_t image_index);
 *   static void renderer_destroy(RendererContext*);
 *
 * Layer indices (compositing order, back to front):
 *   0=BG  1=NG  2=FG  3=TL  4=ST  5=DO
 ******************************************************************************************************************************/

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "field.h"

/* Layer count matching the renderer interface. */
#define RENDERER_LAYER_COUNT 6

/* Maximum vertices for a single line strip draw (oscillograph uses 128). */
#define GL_LINE_MAX_VERTICES 128

/******************************************************************************************************************************
 * Minimal OpenGL 3.3 Core loader — no external dependencies.
 ******************************************************************************************************************************/

#ifndef APIENTRY
#define APIENTRY
#endif

typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef signed char    GLbyte;
typedef short          GLshort;
typedef int            GLint;
typedef int            GLsizei;
typedef unsigned char  GLubyte;
typedef unsigned short GLushort;
typedef unsigned int   GLuint;
typedef float          GLfloat;
typedef float          GLclampf;
typedef double         GLdouble;
typedef void           GLvoid;
typedef char           GLchar;
typedef ptrdiff_t      GLsizeiptr;
typedef ptrdiff_t      GLintptr;

#define GL_FALSE                  0
#define GL_TRUE                   1
#define GL_COLOR_BUFFER_BIT       0x00004000
#define GL_TRIANGLES              0x0004
#define GL_LINE_STRIP             0x0003
#define GL_FLOAT                  0x1406
#define GL_UNSIGNED_BYTE          0x1401
#define GL_TEXTURE_2D             0x0DE1
#define GL_TEXTURE_MAG_FILTER     0x2800
#define GL_TEXTURE_MIN_FILTER     0x2801
#define GL_TEXTURE_WRAP_S         0x2802
#define GL_TEXTURE_WRAP_T         0x2803
#define GL_CLAMP_TO_EDGE          0x812F
#define GL_LINEAR                 0x2601
#define GL_RGBA                   0x1908
#define GL_RGBA8                  0x8058
#define GL_UNPACK_ROW_LENGTH      0x0CF2
#define GL_UNPACK_SKIP_ROWS       0x0CF3
#define GL_UNPACK_SKIP_PIXELS     0x0CF4
#define GL_UNPACK_ALIGNMENT       0x0CF5
#define GL_TEXTURE0               0x84C0
#define GL_BLEND                  0x0BE2
#define GL_SRC_ALPHA              0x0302
#define GL_ONE_MINUS_SRC_ALPHA    0x0303
#define GL_SCISSOR_TEST           0x0C11
#define GL_VERTEX_SHADER          0x8B31
#define GL_FRAGMENT_SHADER        0x8B30
#define GL_COMPILE_STATUS         0x8B81
#define GL_LINK_STATUS            0x8B82
#define GL_ARRAY_BUFFER           0x8892
#define GL_DYNAMIC_DRAW           0x88E8
#define GL_STATIC_DRAW            0x88E4

#define GL_RENDERER  0x1F01
#define GL_VERSION   0x1F02

/* Function pointer typedefs */
typedef void           (APIENTRY *PFNGLVIEWPORTPROC)(GLint, GLint, GLsizei, GLsizei);
typedef void           (APIENTRY *PFNGLCLEARCOLORPROC)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void           (APIENTRY *PFNGLCLEARPROC)(GLbitfield);
typedef void           (APIENTRY *PFNGLENABLEPROC)(GLenum);
typedef void           (APIENTRY *PFNGLDISABLEPROC)(GLenum);
typedef void           (APIENTRY *PFNGLBLENDFUNCPROC)(GLenum, GLenum);
typedef void           (APIENTRY *PFNGLSCISSORPROC)(GLint, GLint, GLsizei, GLsizei);
typedef void           (APIENTRY *PFNGLDRAWARRAYSPROC)(GLenum, GLint, GLsizei);
typedef void           (APIENTRY *PFNGLGENTEXTURESPROC)(GLsizei, GLuint*);
typedef void           (APIENTRY *PFNGLDELETETEXTURESPROC)(GLsizei, const GLuint*);
typedef void           (APIENTRY *PFNGLBINDTEXTUREPROC)(GLenum, GLuint);
typedef void           (APIENTRY *PFNGLTEXPARAMETERIPROC)(GLenum, GLenum, GLint);
typedef void           (APIENTRY *PFNGLTEXIMAGE2DPROC)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
typedef void           (APIENTRY *PFNGLTEXSUBIMAGE2DPROC)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*);
typedef void           (APIENTRY *PFNGLPIXELSTOREIPROC)(GLenum, GLint);
typedef void           (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum);
typedef void           (APIENTRY *PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint*);
typedef void           (APIENTRY *PFNGLDELETEVERTEXARRAYSPROC)(GLsizei, const GLuint*);
typedef void           (APIENTRY *PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void           (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei, GLuint*);
typedef void           (APIENTRY *PFNGLDELETEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void           (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void           (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void           (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef void           (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef GLuint         (APIENTRY *PFNGLCREATESHADERPROC)(GLenum);
typedef void           (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void           (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint);
typedef void           (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef void           (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void           (APIENTRY *PFNGLDELETESHADERPROC)(GLuint);
typedef GLuint         (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef void           (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void           (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint);
typedef void           (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void           (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void           (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint);
typedef void           (APIENTRY *PFNGLDELETEPROGRAMPROC)(GLuint);
typedef GLint          (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar*);
typedef void           (APIENTRY *PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void           (APIENTRY *PFNGLUNIFORM4FPROC)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef const GLubyte* (APIENTRY *PFNGLGETSTRINGPROC)(GLenum);

static PFNGLVIEWPORTPROC              pglViewport              = nullptr;
static PFNGLCLEARCOLORPROC            pglClearColor            = nullptr;
static PFNGLCLEARPROC                 pglClear                 = nullptr;
static PFNGLENABLEPROC                pglEnable                = nullptr;
static PFNGLDISABLEPROC               pglDisable               = nullptr;
static PFNGLBLENDFUNCPROC             pglBlendFunc             = nullptr;
static PFNGLSCISSORPROC               pglScissor               = nullptr;
static PFNGLDRAWARRAYSPROC            pglDrawArrays            = nullptr;
static PFNGLGENTEXTURESPROC           pglGenTextures           = nullptr;
static PFNGLDELETETEXTURESPROC        pglDeleteTextures        = nullptr;
static PFNGLBINDTEXTUREPROC           pglBindTexture           = nullptr;
static PFNGLTEXPARAMETERIPROC         pglTexParameteri         = nullptr;
static PFNGLTEXIMAGE2DPROC            pglTexImage2D            = nullptr;
static PFNGLTEXSUBIMAGE2DPROC         pglTexSubImage2D         = nullptr;
static PFNGLPIXELSTOREIPROC           pglPixelStorei           = nullptr;
static PFNGLACTIVETEXTUREPROC         pglActiveTexture         = nullptr;
static PFNGLGENVERTEXARRAYSPROC       pglGenVertexArrays       = nullptr;
static PFNGLDELETEVERTEXARRAYSPROC    pglDeleteVertexArrays    = nullptr;
static PFNGLBINDVERTEXARRAYPROC       pglBindVertexArray       = nullptr;
static PFNGLGENBUFFERSPROC            pglGenBuffers            = nullptr;
static PFNGLDELETEBUFFERSPROC         pglDeleteBuffers         = nullptr;
static PFNGLBINDBUFFERPROC            pglBindBuffer            = nullptr;
static PFNGLBUFFERDATAPROC            pglBufferData            = nullptr;
static PFNGLVERTEXATTRIBPOINTERPROC   pglVertexAttribPointer   = nullptr;
static PFNGLENABLEVERTEXATTRIBARRAYPROC pglEnableVertexAttribArray = nullptr;
static PFNGLCREATESHADERPROC          pglCreateShader          = nullptr;
static PFNGLSHADERSOURCEPROC          pglShaderSource          = nullptr;
static PFNGLCOMPILESHADERPROC         pglCompileShader         = nullptr;
static PFNGLGETSHADERIVPROC           pglGetShaderiv           = nullptr;
static PFNGLGETSHADERINFOLOGPROC      pglGetShaderInfoLog      = nullptr;
static PFNGLDELETESHADERPROC          pglDeleteShader          = nullptr;
static PFNGLCREATEPROGRAMPROC         pglCreateProgram         = nullptr;
static PFNGLATTACHSHADERPROC          pglAttachShader          = nullptr;
static PFNGLLINKPROGRAMPROC           pglLinkProgram           = nullptr;
static PFNGLGETPROGRAMIVPROC          pglGetProgramiv          = nullptr;
static PFNGLGETPROGRAMINFOLOGPROC     pglGetProgramInfoLog     = nullptr;
static PFNGLUSEPROGRAMPROC            pglUseProgram            = nullptr;
static PFNGLDELETEPROGRAMPROC         pglDeleteProgram         = nullptr;
static PFNGLGETUNIFORMLOCATIONPROC    pglGetUniformLocation    = nullptr;
static PFNGLUNIFORM1IPROC             pglUniform1i             = nullptr;
static PFNGLUNIFORM4FPROC             pglUniform4f             = nullptr;
static PFNGLGETSTRINGPROC             pglGetString             = nullptr;

#define glViewport              pglViewport
#define glClearColor            pglClearColor
#define glClear                 pglClear
#define glEnable                pglEnable
#define glDisable               pglDisable
#define glBlendFunc             pglBlendFunc
#define glScissor               pglScissor
#define glDrawArrays            pglDrawArrays
#define glGenTextures           pglGenTextures
#define glDeleteTextures        pglDeleteTextures
#define glBindTexture           pglBindTexture
#define glTexParameteri         pglTexParameteri
#define glTexImage2D            pglTexImage2D
#define glTexSubImage2D         pglTexSubImage2D
#define glPixelStorei           pglPixelStorei
#define glActiveTexture         pglActiveTexture
#define glGenVertexArrays       pglGenVertexArrays
#define glDeleteVertexArrays    pglDeleteVertexArrays
#define glBindVertexArray       pglBindVertexArray
#define glGenBuffers            pglGenBuffers
#define glDeleteBuffers         pglDeleteBuffers
#define glBindBuffer            pglBindBuffer
#define glBufferData            pglBufferData
#define glVertexAttribPointer   pglVertexAttribPointer
#define glEnableVertexAttribArray pglEnableVertexAttribArray
#define glCreateShader          pglCreateShader
#define glShaderSource          pglShaderSource
#define glCompileShader         pglCompileShader
#define glGetShaderiv           pglGetShaderiv
#define glGetShaderInfoLog      pglGetShaderInfoLog
#define glDeleteShader          pglDeleteShader
#define glCreateProgram         pglCreateProgram
#define glAttachShader          pglAttachShader
#define glLinkProgram           pglLinkProgram
#define glGetProgramiv          pglGetProgramiv
#define glGetProgramInfoLog     pglGetProgramInfoLog
#define glUseProgram            pglUseProgram
#define glDeleteProgram         pglDeleteProgram
#define glGetUniformLocation    pglGetUniformLocation
#define glUniform1i             pglUniform1i
#define glUniform4f             pglUniform4f
#define glGetString             pglGetString

static bool load_gl_functions(void)
{
    #define LOAD(func, pfn_type) \
        p##func = (pfn_type)glfwGetProcAddress(#func); \
        if (!p##func) { fprintf(stderr, "Failed to load %s\n", #func); return false; }

    LOAD(glViewport,              PFNGLVIEWPORTPROC)
    LOAD(glClearColor,            PFNGLCLEARCOLORPROC)
    LOAD(glClear,                 PFNGLCLEARPROC)
    LOAD(glEnable,                PFNGLENABLEPROC)
    LOAD(glDisable,               PFNGLDISABLEPROC)
    LOAD(glBlendFunc,             PFNGLBLENDFUNCPROC)
    LOAD(glScissor,               PFNGLSCISSORPROC)
    LOAD(glDrawArrays,            PFNGLDRAWARRAYSPROC)
    LOAD(glGenTextures,           PFNGLGENTEXTURESPROC)
    LOAD(glDeleteTextures,        PFNGLDELETETEXTURESPROC)
    LOAD(glBindTexture,           PFNGLBINDTEXTUREPROC)
    LOAD(glTexParameteri,         PFNGLTEXPARAMETERIPROC)
    LOAD(glTexImage2D,            PFNGLTEXIMAGE2DPROC)
    LOAD(glTexSubImage2D,         PFNGLTEXSUBIMAGE2DPROC)
    LOAD(glPixelStorei,           PFNGLPIXELSTOREIPROC)
    LOAD(glActiveTexture,         PFNGLACTIVETEXTUREPROC)
    LOAD(glGenVertexArrays,       PFNGLGENVERTEXARRAYSPROC)
    LOAD(glDeleteVertexArrays,    PFNGLDELETEVERTEXARRAYSPROC)
    LOAD(glBindVertexArray,       PFNGLBINDVERTEXARRAYPROC)
    LOAD(glGenBuffers,            PFNGLGENBUFFERSPROC)
    LOAD(glDeleteBuffers,         PFNGLDELETEBUFFERSPROC)
    LOAD(glBindBuffer,            PFNGLBINDBUFFERPROC)
    LOAD(glBufferData,            PFNGLBUFFERDATAPROC)
    LOAD(glVertexAttribPointer,   PFNGLVERTEXATTRIBPOINTERPROC)
    LOAD(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC)
    LOAD(glCreateShader,          PFNGLCREATESHADERPROC)
    LOAD(glShaderSource,          PFNGLSHADERSOURCEPROC)
    LOAD(glCompileShader,         PFNGLCOMPILESHADERPROC)
    LOAD(glGetShaderiv,           PFNGLGETSHADERIVPROC)
    LOAD(glGetShaderInfoLog,      PFNGLGETSHADERINFOLOGPROC)
    LOAD(glDeleteShader,          PFNGLDELETESHADERPROC)
    LOAD(glCreateProgram,         PFNGLCREATEPROGRAMPROC)
    LOAD(glAttachShader,          PFNGLATTACHSHADERPROC)
    LOAD(glLinkProgram,           PFNGLLINKPROGRAMPROC)
    LOAD(glGetProgramiv,          PFNGLGETPROGRAMIVPROC)
    LOAD(glGetProgramInfoLog,     PFNGLGETPROGRAMINFOLOGPROC)
    LOAD(glUseProgram,            PFNGLUSEPROGRAMPROC)
    LOAD(glDeleteProgram,         PFNGLDELETEPROGRAMPROC)
    LOAD(glGetUniformLocation,    PFNGLGETUNIFORMLOCATIONPROC)
    LOAD(glUniform1i,             PFNGLUNIFORM1IPROC)
    LOAD(glUniform4f,             PFNGLUNIFORM4FPROC)
    LOAD(glGetString,             PFNGLGETSTRINGPROC)

    #undef LOAD
    return true;
}

/******************************************************************************************************************************
 * Shaders
 ******************************************************************************************************************************/

static const char* LAYER_VS = R"GLSL(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
out vec2 vUV;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vUV = aUV;
}
)GLSL";

static const char* LAYER_FS = R"GLSL(
#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTex;
void main() {
    vec4 c = texture(uTex, vUV);
    /* Frame data is 0xRRGGBBAA uint32_t. On little-endian, bytes in memory
     * are AA BB GG RR, so GL_RGBA+GL_UNSIGNED_BYTE reads them as R=A, G=B,
     * B=G, A=R. Swizzle back: output = (texA, texB, texG, texR) = (R, G, B, A). */
    FragColor = vec4(c.a, c.b, c.g, c.r);
}
)GLSL";

static const char* LINE_VS = R"GLSL(
#version 330 core
layout(location = 0) in vec2 aPos;
uniform vec4 uColor;
out vec4 vColor;
void main() {
    vec2 clip = vec2(aPos.x * 2.0 - 1.0, aPos.y * 2.0 - 1.0);
    gl_Position = vec4(clip, 0.0, 1.0);
    vColor = uColor;
}
)GLSL";

static const char* LINE_FS = R"GLSL(
#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() {
    FragColor = vColor;
}
)GLSL";

/******************************************************************************************************************************
 * GL pipeline state
 ******************************************************************************************************************************/

typedef struct {
    GLuint layer_program;
    GLuint layer_vao;
    GLuint layer_vbo;
    GLint  layer_u_tex;

    GLuint line_program;
    GLuint line_vao;
    GLuint line_vbo;
    GLint  line_u_color;
} GLPipeline;

/******************************************************************************************************************************
 * Renderer context
 ******************************************************************************************************************************/

typedef struct RendererContext {
    GLFWwindow*  window;
    uint32_t     width;
    uint32_t     height;
    GLuint       layer_textures[RENDERER_LAYER_COUNT];
    GLPipeline   pipeline;
} RendererContext;

/******************************************************************************************************************************
 * Shader helpers
 ******************************************************************************************************************************/

static GLuint compile_shader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        fprintf(stderr, "Shader compile error: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint link_program(const char* vs_src, const char* fs_src)
{
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        fprintf(stderr, "Program link error: %s\n", log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

/******************************************************************************************************************************
 * Renderer interface implementation
 ******************************************************************************************************************************/

[[nodiscard]] static bool renderer_init(RendererContext* restrict ctx, GLFWwindow* restrict window,
                           uint32_t width, uint32_t height)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->window = window;
    ctx->width  = width;
    ctx->height = height;

    if (!load_gl_functions()) {
        fprintf(stderr, "Failed to load OpenGL functions\n");
        return false;
    }

    printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL Version:  %s\n", glGetString(GL_VERSION));

    /* Layer pipeline (textured quad) */
    ctx->pipeline.layer_program = link_program(LAYER_VS, LAYER_FS);
    if (!ctx->pipeline.layer_program) return false;
    ctx->pipeline.layer_u_tex = glGetUniformLocation(ctx->pipeline.layer_program, "uTex");

    static const float quad_data[] = {
        -1.0f, -1.0f,  0.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 0.0f,
        -1.0f, -1.0f,  0.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 1.0f,
    };

    glGenVertexArrays(1, &ctx->pipeline.layer_vao);
    glBindVertexArray(ctx->pipeline.layer_vao);
    glGenBuffers(1, &ctx->pipeline.layer_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->pipeline.layer_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_data), quad_data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    /* Line pipeline (colored line strip) */
    ctx->pipeline.line_program = link_program(LINE_VS, LINE_FS);
    if (!ctx->pipeline.line_program) return false;
    ctx->pipeline.line_u_color = glGetUniformLocation(ctx->pipeline.line_program, "uColor");

    glGenVertexArrays(1, &ctx->pipeline.line_vao);
    glBindVertexArray(ctx->pipeline.line_vao);
    constexpr GLsizeiptr line_vbo_size = GL_LINE_MAX_VERTICES * 2 * sizeof(float);
    glGenBuffers(1, &ctx->pipeline.line_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->pipeline.line_vbo);
    glBufferData(GL_ARRAY_BUFFER, line_vbo_size, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    /* Fixed state */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.f, 0.f, 0.f, 0.f);

    /* Create layer textures (empty, will be filled by renderer_upload_layer) */
    glGenTextures(RENDERER_LAYER_COUNT, ctx->layer_textures);
    for (int i = 0; i < RENDERER_LAYER_COUNT; ++i) {
        glBindTexture(GL_TEXTURE_2D, ctx->layer_textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

/* One-time initial upload of layer pixels (called once per layer at startup). */
static void renderer_upload_layer(RendererContext* restrict ctx, int layer_index,
                                   const uint32_t* restrict pixels, uint32_t width, uint32_t height)
{
    glBindTexture(GL_TEXTURE_2D, ctx->layer_textures[layer_index]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)width, (GLsizei)height,
                    GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
}

/* Begin frame: clear the framebuffer. image_index is always 0 for OpenGL. */
[[nodiscard]] static bool renderer_begin_frame(RendererContext* restrict ctx, uint32_t* restrict image_index)
{
    (void)ctx;
    glClear(GL_COLOR_BUFFER_BIT);
    *image_index = 0;
    return true;
}

/* Per-frame layer texture update of a sub-region (dirty-rect upload).
 * Source pixels are addressed as pixels[(y + row) * width + x + col] — the
 * row stride stays the full layer width via GL_UNPACK_ROW_LENGTH, so no
 * intermediate copy is needed. Pixel store state is restored afterwards. */
static void renderer_update_layer_rect(RendererContext* restrict ctx, int layer_index,
                                       const uint32_t* restrict pixels, uint32_t width, uint32_t height,
                                       uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    (void)height; /* upload extents are (w, h); height only bounds the source rows */
    glBindTexture(GL_TEXTURE_2D, ctx->layer_textures[layer_index]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint)width);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, (GLint)x);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, (GLint)y);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); /* rows start on uint32_t boundaries */
    glTexSubImage2D(GL_TEXTURE_2D, 0, (GLint)x, (GLint)y, (GLsizei)w, (GLsizei)h,
                    GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
}

/* No render pass concept in OpenGL — this is a no-op. */
static void renderer_begin_render(RendererContext* restrict ctx, uint32_t image_index)
{
    (void)ctx; (void)image_index;
}

/* Draw a layer texture as a full-screen quad. */
static void renderer_draw_layer(RendererContext* restrict ctx, int layer_index)
{
    glUseProgram(ctx->pipeline.layer_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ctx->layer_textures[layer_index]);
    glUniform1i(ctx->pipeline.layer_u_tex, 0);

    glBindVertexArray(ctx->pipeline.layer_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

/* Draw a line strip with a uniform color.
 * `data` is count*2 floats in 0-1 normalized coords (Y-up).
 * Scissor is in screen coords (top-left origin); converted to GL bottom-left origin. */
static void renderer_draw_line(RendererContext* restrict ctx,
                                const float* restrict data, uint32_t count,
                                uint32_t color,
                                int32_t scissor_l, int32_t scissor_t,
                                uint32_t scissor_w, uint32_t scissor_h)
{
    if (count < 2) return;

    /* Set scissor (convert top-left Y to bottom-left Y for OpenGL) */
    if (scissor_w > 0 && scissor_h > 0) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(scissor_l,
                  (GLint)ctx->height - scissor_t - (GLint)scissor_h,
                  (GLsizei)scissor_w, (GLsizei)scissor_h);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }

    glUseProgram(ctx->pipeline.line_program);

    /* Upload vertex data — OpenGL handles this correctly per-draw since
     * glBufferData + glDrawArrays are executed in order on the GPU. */
    glBindBuffer(GL_ARRAY_BUFFER, ctx->pipeline.line_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(count * 2 * sizeof(float)), data, GL_DYNAMIC_DRAW);

    /* Uniform color: 0xRRGGBBAA → vec4(r, g, b, a) in 0-1 range */
    glUniform4f(ctx->pipeline.line_u_color,
                (float)ff_extract_byte(color, 3) / 255.0f,
                (float)ff_extract_byte(color, 2) / 255.0f,
                (float)ff_extract_byte(color, 1) / 255.0f,
                (float)ff_extract_byte(color, 0) / 255.0f);

    glBindVertexArray(ctx->pipeline.line_vao);
    glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)count);
    glBindVertexArray(0);

    glDisable(GL_SCISSOR_TEST);
}

/* End frame: swap buffers. */
static void renderer_end_frame(RendererContext* restrict ctx, uint32_t image_index)
{
    (void)image_index;
    glfwSwapBuffers(ctx->window);
}

/* Cleanup. */
static void renderer_destroy(RendererContext* restrict ctx)
{
    glDeleteTextures(RENDERER_LAYER_COUNT, ctx->layer_textures);

    if (ctx->pipeline.layer_vbo)     glDeleteBuffers(1, &ctx->pipeline.layer_vbo);
    if (ctx->pipeline.layer_vao)     glDeleteVertexArrays(1, &ctx->pipeline.layer_vao);
    if (ctx->pipeline.layer_program) glDeleteProgram(ctx->pipeline.layer_program);

    if (ctx->pipeline.line_vbo)      glDeleteBuffers(1, &ctx->pipeline.line_vbo);
    if (ctx->pipeline.line_vao)      glDeleteVertexArrays(1, &ctx->pipeline.line_vao);
    if (ctx->pipeline.line_program)  glDeleteProgram(ctx->pipeline.line_program);

    memset(ctx, 0, sizeof(*ctx));
}

