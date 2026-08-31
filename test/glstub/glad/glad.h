#pragma once
// Fake <glad/glad.h> for unit-testing source/mge_gl.c with no GL context.
// Declares exactly the GL surface mge_gl.c touches; the implementations in
// test/glstub/glstub.c are no-ops that record their arguments. Constants use
// their real GL values so assertions read naturally.

#include <stddef.h>

typedef unsigned int  GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int  GLbitfield;
typedef void          GLvoid;
typedef int           GLint;
typedef int           GLsizei;
typedef unsigned int  GLuint;
typedef unsigned char  GLubyte;
typedef float         GLfloat;
typedef double        GLdouble;
typedef char          GLchar;
typedef ptrdiff_t     GLsizeiptr;
typedef ptrdiff_t     GLintptr;

typedef void* (*GLADloadproc)(const char* name);
int gladLoadGLLoader(GLADloadproc load);

#ifndef APIENTRY
#define APIENTRY
#endif
typedef void (APIENTRY* GLDEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* userParam);
typedef void (*PFNGLDEBUGMESSAGECALLBACKPROC)(GLDEBUGPROC callback, const void* userParam);
extern PFNGLDEBUGMESSAGECALLBACKPROC glad_glDebugMessageCallback; // real glad exposes it as a pointer
#define glDebugMessageCallback glad_glDebugMessageCallback
void glDebugMessageControl(GLenum source, GLenum type, GLenum severity, GLsizei count,
    const GLuint* ids, GLboolean enabled);

#define GL_FALSE 0
#define GL_TRUE  1
#define GL_NONE  0
#define GL_ZERO  0
#define GL_ONE   1

#define GL_NEVER    0x0200
#define GL_LESS     0x0201
#define GL_EQUAL    0x0202
#define GL_LEQUAL   0x0203
#define GL_GREATER  0x0204
#define GL_NOTEQUAL 0x0205
#define GL_GEQUAL   0x0206
#define GL_ALWAYS   0x0207

#define GL_KEEP      0x1E00
#define GL_REPLACE   0x1E01
#define GL_INCR      0x1E02
#define GL_DECR      0x1E03
#define GL_INVERT    0x150A
#define GL_INCR_WRAP 0x8507
#define GL_DECR_WRAP 0x8508

#define GL_FRONT           0x0404
#define GL_BACK            0x0405
#define GL_FRONT_AND_BACK  0x0408
#define GL_CW              0x0900
#define GL_CCW             0x0901
#define GL_CULL_FACE       0x0B44

#define GL_LINES     0x0001
#define GL_TRIANGLES 0x0004

#define GL_BLEND               0x0BE2
#define GL_SRC_ALPHA           0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_DEPTH_TEST          0x0B71
#define GL_STENCIL_TEST        0x0B90
#define GL_MULTISAMPLE         0x809D
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_FRAMEBUFFER_SRGB    0x8DB9

#define GL_COLOR_BUFFER_BIT   0x00004000
#define GL_DEPTH_BUFFER_BIT   0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400

#define GL_ARRAY_BUFFER         0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW          0x88E4
#define GL_DYNAMIC_DRAW         0x88E8

#define GL_FLOAT         0x1406
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_INT  0x1405

#define GL_TEXTURE_2D            0x0DE1
#define GL_TEXTURE0             0x84C0
#define GL_TEXTURE_MIN_FILTER   0x2801
#define GL_TEXTURE_MAG_FILTER   0x2800
#define GL_TEXTURE_WRAP_S       0x2802
#define GL_TEXTURE_WRAP_T       0x2803
#define GL_TEXTURE_SWIZZLE_RGBA 0x8E46
#define GL_LINEAR               0x2601
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_REPEAT               0x2901
#define GL_CLAMP_TO_EDGE        0x812F
#define GL_MIRRORED_REPEAT      0x8370
#define GL_MIRROR_CLAMP_TO_EDGE 0x8743
#define GL_UNPACK_ALIGNMENT     0x0CF5

#define GL_RED  0x1903
#define GL_GREEN 0x1904
#define GL_RG   0x8227
#define GL_RGB  0x1907
#define GL_RGBA 0x1908
#define GL_R8            0x8229
#define GL_RG8           0x822B
#define GL_RGB8          0x8051
#define GL_RGBA8         0x8058
#define GL_SRGB8         0x8C41
#define GL_SRGB8_ALPHA8  0x8C43
#define GL_RGB16F        0x881B
#define GL_RGBA16F       0x881A

#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS    0x8B82
#define GL_VERTEX_SHADER   0x8B31
#define GL_FRAGMENT_SHADER 0x8B30

#define GL_SAMPLES 0x80A8

#define GL_DONT_CARE                     0x1100
#define GL_DEBUG_OUTPUT                   0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242
#define GL_DEBUG_SEVERITY_HIGH           0x9146
#define GL_DEBUG_SEVERITY_MEDIUM         0x9147
#define GL_DEBUG_SEVERITY_LOW            0x9148
#define GL_DEBUG_SEVERITY_NOTIFICATION   0x826B
#define GL_DEBUG_SOURCE_API             0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM   0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER 0x8248

#define GL_VENDOR                   0x1F00
#define GL_RENDERER                 0x1F01
#define GL_VERSION                  0x1F02
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C

void glActiveTexture(GLenum texture);
void glAttachShader(GLuint program, GLuint shader);
void glBindBuffer(GLenum target, GLuint buffer);
void glBindTexture(GLenum target, GLuint texture);
void glBindVertexArray(GLuint array);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data);
void glClear(GLbitfield mask);
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a);
void glCompileShader(GLuint shader);
GLuint glCreateProgram(void);
GLuint glCreateShader(GLenum type);
void glCullFace(GLenum mode);
void glDeleteBuffers(GLsizei n, const GLuint* buffers);
void glDeleteProgram(GLuint program);
void glDeleteShader(GLuint shader);
void glDeleteVertexArrays(GLsizei n, const GLuint* arrays);
void glDepthFunc(GLenum func);
void glDepthMask(GLboolean flag);
void glDisable(GLenum cap);
void glDisableVertexAttribArray(GLuint index);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices);
void glEnable(GLenum cap);
void glEnableVertexAttribArray(GLuint index);
void glFrontFace(GLenum mode);
void glGenBuffers(GLsizei n, GLuint* buffers);
void glGenTextures(GLsizei n, GLuint* textures);
void glDeleteTextures(GLsizei n, const GLuint* textures);
void glGenVertexArrays(GLsizei n, GLuint* arrays);
void glGenerateMipmap(GLenum target);
GLint glGetAttribLocation(GLuint program, const GLchar* name);
void glGetIntegerv(GLenum pname, GLint* data);
void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
void glGetProgramiv(GLuint program, GLenum pname, GLint* params);
void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
void glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
const GLubyte* glGetString(GLenum name);
GLint glGetUniformLocation(GLuint program, const GLchar* name);
GLboolean glIsEnabled(GLenum cap);
void glLinkProgram(GLuint program);
void glPixelStorei(GLenum pname, GLint param);
void glPolygonOffset(GLfloat factor, GLfloat units);
void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
void glStencilFunc(GLenum func, GLint ref, GLuint mask);
void glStencilMask(GLuint mask);
void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
    GLint border, GLenum format, GLenum type, const void* pixels);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexParameteriv(GLenum target, GLenum pname, const GLint* params);
void glUniform1f(GLint location, GLfloat v0);
void glUniform1i(GLint location, GLint v0);
void glUniform2fv(GLint location, GLsizei count, const GLfloat* value);
void glUniform3fv(GLint location, GLsizei count, const GLfloat* value);
void glUniform4fv(GLint location, GLsizei count, const GLfloat* value);
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUseProgram(GLuint program);
void glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized,
    GLsizei stride, const void* pointer);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
