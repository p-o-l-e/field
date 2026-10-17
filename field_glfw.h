#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define _MULTI_THREADED
#include <pthread.h>

#include "field.h"
#define FIELD_TRANSPARENCY 1

/******************************************************************************************************************************
 * Minimal OpenGL 3.3 Core loader — no external dependencies.
 * Types and constants mirror the official glcorearb.h spec.
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
#define GL_INFO_LOG_LENGTH        0x8B84

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

#define GL_RENDERER  0x1F01
#define GL_VERSION   0x1F02

/* Function pointers — loaded once after context creation */
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
 *
 * Layer shader: full-screen textured quad in clip space.
 * Line shader:  line strip in 0-1 normalized coords, converted to clip
 *              space in the vertex shader. Uniform color.
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
layout(location = 0) in vec2 aPos;   /* 0-1 normalized, Y-up */
uniform vec4 uColor;
out vec4 vColor;
void main() {
    /* Map 0-1 (Y-up) to clip space (-1..1, Y-up) */
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
 * GL pipeline state — created once after context init, destroyed on exit.
 ******************************************************************************************************************************/

typedef struct {
    GLuint layer_program;
    GLuint layer_vao;
    GLuint layer_vbo_pos;   /* interleaved: pos.xy + uv.xy per vertex (4 floats) */
    GLint  layer_u_tex;

    GLuint line_program;
    GLuint line_vao;
    GLuint line_vbo;        /* dynamic: 2 floats per vertex */
    GLint  line_u_color;
} GLPipeline;

static GLPipeline g_pipeline = {0};

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

/* Full-screen quad in clip space with UVs.
 * Vertices: (pos.xy, uv.xy) interleaved, 4 floats per vertex, 6 vertices (2 triangles).
 * UV Y is flipped so that texture row 0 (top of Frame data) appears at the top. */
static bool pipeline_init(void)
{
    /* --- Layer program (textured quad) --- */
    g_pipeline.layer_program = link_program(LAYER_VS, LAYER_FS);
    if (!g_pipeline.layer_program) return false;
    g_pipeline.layer_u_tex = glGetUniformLocation(g_pipeline.layer_program, "uTex");

    /* Full-screen quad: 2 triangles, clip-space positions + UVs */
    static const float quad_data[] = {
        /* pos.x   pos.y    uv.x  uv.y */
        -1.0f, -1.0f,  0.0f, 1.0f,   /* bottom-left  → tex bottom */
        -1.0f,  1.0f,  0.0f, 0.0f,   /* top-left     → tex top    */
         1.0f,  1.0f,  1.0f, 0.0f,   /* top-right    → tex top    */
        -1.0f, -1.0f,  0.0f, 1.0f,   /* bottom-left  → tex bottom */
         1.0f,  1.0f,  1.0f, 0.0f,   /* top-right    → tex top    */
         1.0f, -1.0f,  1.0f, 1.0f,   /* bottom-right → tex bottom */
    };

    glGenVertexArrays(1, &g_pipeline.layer_vao);
    glBindVertexArray(g_pipeline.layer_vao);

    glGenBuffers(1, &g_pipeline.layer_vbo_pos);
    glBindBuffer(GL_ARRAY_BUFFER, g_pipeline.layer_vbo_pos);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_data), quad_data, GL_STATIC_DRAW);

    /* attrib 0: position (2 floats) */
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    /* attrib 1: uv (2 floats) */
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    /* --- Line program (colored line strip) --- */
    g_pipeline.line_program = link_program(LINE_VS, LINE_FS);
    if (!g_pipeline.line_program) return false;
    g_pipeline.line_u_color = glGetUniformLocation(g_pipeline.line_program, "uColor");

    glGenVertexArrays(1, &g_pipeline.line_vao);
    glBindVertexArray(g_pipeline.line_vao);

    /* Dynamic VBO — sized for SPLINE_SEGMENTS * 2 floats (the largest line we draw).
     * The oscillograph stream uses up to 128 points; cords use SPLINE_SEGMENTS (32).
     * Allocate for the max of both. */
    constexpr GLsizeiptr line_vbo_size = 128 * 2 * sizeof(float);
    glGenBuffers(1, &g_pipeline.line_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_pipeline.line_vbo);
    glBufferData(GL_ARRAY_BUFFER, line_vbo_size, nullptr, GL_DYNAMIC_DRAW);

    /* attrib 0: position (2 floats, tightly packed) */
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    return true;
}

static void pipeline_destroy(void)
{
    if (g_pipeline.layer_vbo_pos) glDeleteBuffers(1, &g_pipeline.layer_vbo_pos);
    if (g_pipeline.layer_vao)     glDeleteVertexArrays(1, &g_pipeline.layer_vao);
    if (g_pipeline.layer_program) glDeleteProgram(g_pipeline.layer_program);

    if (g_pipeline.line_vbo)      glDeleteBuffers(1, &g_pipeline.line_vbo);
    if (g_pipeline.line_vao)      glDeleteVertexArrays(1, &g_pipeline.line_vao);
    if (g_pipeline.line_program)  glDeleteProgram(g_pipeline.line_program);

    memset(&g_pipeline, 0, sizeof(g_pipeline));
}

/******************************************************************************************************************************
 * Texture helpers
 *
 * Frame data is uint32_t pixels in 0xRRGGBBAA layout. Uploaded with
 * GL_RGBA + GL_UNSIGNED_BYTE; the fragment shader swizzles ABGR→RGBA
 * to compensate for little-endian byte order (AA BB GG RR in memory).
 ******************************************************************************************************************************/

static GLuint get_buffer(Frame* frame)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)frame->width, (GLsizei)frame->height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, frame->data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

/* Draw a layer texture as a full-screen quad. */
static void update_buffer(Frame* frame, GLuint texture)
{
    /* Upload updated pixels */
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)frame->width, (GLsizei)frame->height,
                    GL_RGBA, GL_UNSIGNED_BYTE, frame->data);

    /* Draw full-screen quad */
    glUseProgram(g_pipeline.layer_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(g_pipeline.layer_u_tex, 0);

    glBindVertexArray(g_pipeline.layer_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

/* Draw a line strip from float XY data (0-1 normalized, Y-up) with a uniform color.
 * `count` is the number of vertices. `data` is count*2 floats. */
static void draw_line_strip(const float* data, uint32_t count, uint32_t color)
{
    if (count < 2) return;

    glUseProgram(g_pipeline.line_program);

    /* Upload vertex data */
    glBindBuffer(GL_ARRAY_BUFFER, g_pipeline.line_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(count * 2 * sizeof(float)), data, GL_DYNAMIC_DRAW);

    /* Set uniform color: 0xRRGGBBAA → vec4(r, g, b, a) in 0-1 range */
    glUniform4f(g_pipeline.line_u_color,
                (float)ff_extract_byte(color, 3) / 255.0f,
                (float)ff_extract_byte(color, 2) / 255.0f,
                (float)ff_extract_byte(color, 1) / 255.0f,
                (float)ff_extract_byte(color, 0) / 255.0f);

    glBindVertexArray(g_pipeline.line_vao);
    glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)count);
    glBindVertexArray(0);
}

/******************************************************************************************************************************
 * Threading globals
 ******************************************************************************************************************************/

GLFWwindow* window;

static bool _switch_on = true;
static bool _window_on = true;
static bool _handle_events = true;

pthread_t field_thread;
pthread_t event_thread;

pthread_mutex_t _main_lock;
pthread_mutex_t _screen_lock;

pthread_barrier_t _init_barrier;
pthread_barrier_t _exit_barrier;

pthread_cond_t _repaint_condition;
pthread_cond_t _escape_condition;

/******************************************************************************************************************************
 * GLFW callbacks (unchanged logic)
 ******************************************************************************************************************************/

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    (void)window;
    glViewport(0, 0, width, height);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Field* context = (Field*)glfwGetWindowUserPointer(window);

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    ff_hit_test(context, (int)x, (int)y);
    if(context->current[IP_ENTITY] != 0)
    {
        auto cn = context->current[IP_NODE];
        auto ce = context->current[IP_ENTITY];
        scroll_entity[context->node[cn].at[ce].type](&context->node[cn].at[ce], (int)xoffset, (int)yoffset);
    }
    context->current[IP_ENTITY] = 0;
}

static void cursor_position_callback(GLFWwindow* window, double x, double y)
{
    Field* context = (Field*)glfwGetWindowUserPointer(window);

    if(context->drag)
        ff_hit_test_drag(context, (int)x, (int)y);
    else
        ff_hit_test(context, (int)x, (int)y);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int/* mods*/)
{
    Field* context = (Field*)glfwGetWindowUserPointer(window);

    switch(button)
    {
        case GLFW_MOUSE_BUTTON_LEFT:
        {
            if(action == GLFW_PRESS) {
                double x, y;
                glfwGetCursorPos(window, &x, &y);

                ff_hit_test_down(context, (int)x, (int)y, LMB);
                auto cn = context->current[IP_NODE];
                auto ce = context->current[IP_ENTITY];
                if(context->node[cn].at[ce].flags & MOVEABLE)
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

            }
            else if(action == GLFW_RELEASE) {
                double x, y;

                glfwGetCursorPos(window, &x, &y);
                ff_hit_test_up(context, (int)x, (int)y, LMB);
                context->move = false;

                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
        break;

        case GLFW_MOUSE_BUTTON_RIGHT:
        {
            if(action == GLFW_PRESS) {
                double x, y;
                glfwGetCursorPos(window, &x, &y);
                ff_hit_test_down(context, (int)x, (int)y, RMB);
            }
            else if(action == GLFW_RELEASE)
                printf("RMB Released\n");
        }
        break;

        case GLFW_MOUSE_BUTTON_MIDDLE:
        {
            if(action == GLFW_PRESS)
                printf("MMB Pressed\n");
            else if(action == GLFW_RELEASE)
                printf("MMB Released\n");
        }
        break;

    }
}

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
    printf("Error: %d\n", error);
}

static void key_callback(GLFWwindow* /*window*/, int key, int/* scancode*/, int action, int/* mods*/)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        _window_on = false;
    }
}

void window_pos_callback(GLFWwindow* window, int, int)
{
    Field* context = (Field*)glfwGetWindowUserPointer(window);
}

/******************************************************************************************************************************
 * Thread functions
 ******************************************************************************************************************************/

void* events_process(void*)
{
    pthread_barrier_wait(&_init_barrier);

    while(_handle_events)
    {
        glfwWaitEvents();
        pthread_cond_signal(&_repaint_condition);
    }

    printf("Event handler terminated...\n");
    pthread_barrier_wait(&_exit_barrier);

    return nullptr;
}

void* draw_field(void* arg)
{
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) exit(EXIT_FAILURE);

    printf("Compiled against GLFW %i.%i.%i\n", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);
    printf("%s\n", glfwGetVersionString());

    /* Request a modern OpenGL 3.3 Core Profile context */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_POSITION_X, 100);
    glfwWindowHint(GLFW_POSITION_Y, 100);

    Field* context = (Field*)arg;

    window = glfwCreateWindow((int)context->layer[FG]->width, (int)context->layer[FG]->height, "FIELD", nullptr, nullptr);

    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetWindowSizeLimits(window, context->layer[FG]->width, context->layer[FG]->height, context->layer[FG]->width, context->layer[FG]->height);
    glfwSetWindowSize(window, context->layer[FG]->width, context->layer[FG]->height);

    glfwSetWindowUserPointer(window, context);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetWindowPosCallback(window, window_pos_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    /* Load OpenGL 3.3 Core function pointers */
    if (!load_gl_functions())
    {
        fprintf(stderr, "Failed to load OpenGL functions\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL Version:  %s\n", glGetString(GL_VERSION));

    /* Initialize shader pipeline (programs, VAOs, VBOs) */
    if (!pipeline_init())
    {
        fprintf(stderr, "Failed to initialize GL pipeline\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    /* Fixed state: alpha blending, transparent clear */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.f, 0.f, 0.f, 0.f);

    /* Create layer textures */
    GLuint bg = get_buffer(context->layer[BG]);
    GLuint ng = get_buffer(context->layer[NG]);
    GLuint fg = get_buffer(context->layer[FG]);
    GLuint st = get_buffer(context->layer[ST]);
    GLuint tl = get_buffer(context->layer[TL]);
    constexpr uint32_t CORD_NORMAL = GRAY;
    constexpr uint32_t CORD_SELECT = GREEN;

    #ifdef DEBUG_OVERLAY
        GLuint debug_layer = get_buffer(context->layer[DO]);
    #endif

    pthread_barrier_wait(&_init_barrier);
    printf("GLFW Initialized...\n");

    while (_window_on)
    {
        pthread_mutex_lock(&_screen_lock);
        pthread_cond_wait(&_repaint_condition, &_screen_lock);
        pthread_mutex_unlock(&_screen_lock);

        #ifdef DEBUG_OVERLAY
            if(atomic_load(&force_repaint)) context->repaint = true;
        #endif

        bool swap = false;
        bool update_stream = false;

        if(atomic_load_explicit(&trigger_stream_update, memory_order_acquire)) {
            atomic_store_explicit(&trigger_stream_update, false, memory_order_release);
            context->repaint = true;
            update_stream = true;
        }

        if(context->repaint)
        {
            update_stream = true;
            glClear(GL_COLOR_BUFFER_BIT);

            /* Re-render the CPU-side scene into layer frames */
            ff_draw_scene(context);

            /* Composite layers as textured quads (back to front) */
            update_buffer(context->layer[BG], bg);
            update_buffer(context->layer[NG], ng);
            update_buffer(context->layer[FG], fg);
            update_buffer(context->layer[TL], tl);

            if(context->staging) {
                update_buffer(context->layer[ST], st);
            }

            #ifdef DEBUG_OVERLAY
                if(atomic_load_explicit(&force_repaint, memory_order_acquire)) {
                    atomic_store_explicit(&force_repaint, false, memory_order_release);
                }
                update_buffer(context->layer[DO], debug_layer);
            #endif

            swap = true;
        }

        if(update_stream) {
            constexpr size_t bsize = 128;
            static size_t w = 0;

            static float buffer[bsize * 2];

            auto ext = (Oscillograph*)context->stream[0]->extension;
            if(ext->x && *ext->x && ext->y && *ext->y) {

                float scale = (float)context->stream[0]->width / (float)context->width;
                float cx = (float)(context->stream[0]->bounds.l + context->stream[0]->width / 2) / (float)context->width;
                float cy = 1.0f - (float)(context->stream[0]->bounds.t + context->stream[0]->height / 2) / (float)context->height;

                buffer[w++] = **ext->x * scale + cx;
                buffer[w++] = **ext->y * scale + cy;
                if(w >= bsize * 2) w = 0;
            }

            /* Scissor to the stream widget bounds (GL origin is bottom-left) */
            glEnable(GL_SCISSOR_TEST);
            glScissor((GLint)context->stream[0]->bounds.l,
                      (GLint)(context->height - context->stream[0]->bounds.t - context->stream[0]->height),
                      (GLsizei)context->stream[0]->width,
                      (GLsizei)context->stream[0]->height);

            draw_line_strip(buffer, (uint32_t)bsize, 0x40A090FF);

            glDisable(GL_SCISSOR_TEST);

            update_stream = false;
        }

        if(context->connecting)
        {
            draw_line_strip((const float*)context->pressed->data, SPLINE_SEGMENTS, CORD_SELECT);
            swap = true;
        }

        if(swap)
        {
            /* Draw connection cords */
            for(uint32_t n = 0; n < context->nodes; ++n) {
                for(uint32_t i = 0; i < context->node[n].entities; ++i) {
                    auto entity = &context->node[n].at[i];
                    if(entity->connected && entity->has_data) {
                        auto colour = entity->hovered || entity->connection->hovered ? CORD_SELECT : CORD_NORMAL;
                        draw_line_strip((const float*)entity->data, SPLINE_SEGMENTS, colour);
                    }
                }
            }

            glfwSwapBuffers(window);
        }
    }

    /* Cleanup GL resources */
    glDeleteTextures(1, &bg);
    glDeleteTextures(1, &ng);
    glDeleteTextures(1, &fg);
    glDeleteTextures(1, &st);
    glDeleteTextures(1, &tl);
    #ifdef DEBUG_OVERLAY
        glDeleteTextures(1, &debug_layer);
    #endif
    pipeline_destroy();

    pthread_cond_signal(&_escape_condition);

    _switch_on      = false;
    _handle_events  = false;
    glfwPostEmptyEvent();

    ffDestroyField(context);
    glfwSetWindowShouldClose(window, GL_TRUE);
    glfwDestroyWindow(window);

    glfwTerminate();
    printf("GLFW Terminated...\n");

    pthread_barrier_wait(&_exit_barrier);

    return nullptr;
}

static void field_loop(Field* restrict context) {
    pthread_mutex_init(&_screen_lock, nullptr);
    pthread_mutex_init(&_main_lock, nullptr);

    pthread_barrier_init(&_init_barrier, nullptr, 2);
    printf("Creating threads\n");

    pthread_create(&field_thread, nullptr, draw_field, context);
    pthread_create(&event_thread, nullptr, events_process, nullptr);

    pthread_barrier_destroy(&_init_barrier);

    while (_switch_on)
    {
        pthread_mutex_lock(&_main_lock);
        pthread_cond_wait(&_escape_condition, &_main_lock);
        pthread_mutex_unlock(&_main_lock);
    }

    pthread_barrier_init(&_exit_barrier, nullptr, 3);
    pthread_barrier_wait(&_exit_barrier);
    pthread_barrier_destroy(&_exit_barrier);

    printf("Main Terminated...\n");
    pthread_mutex_destroy(&_screen_lock);
    pthread_mutex_destroy(&_main_lock);

    exit(EXIT_SUCCESS);
}
