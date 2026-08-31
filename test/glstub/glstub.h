#pragma once
// Recording state for the fake GL layer (test/glstub/glstub.c). Tests call
// glstub_reset(), run engine code, then inspect `glstub`.

#include "glad/glad.h"

#define GLSTUB_MAX_DRAWS 64
#define GLSTUB_MAX_SIZES 32

typedef struct {
    GLenum mode;
    GLint first;
    GLsizei count;
} StubDrawArrays;

typedef struct {
    // id allocation for glGen* / glCreate*
    GLuint nextId;

    // last-value records
    struct { GLint x, y, w, h; } viewport;
    GLfloat clearColor[4];
    GLbitfield lastClearMask;
    GLuint usedProgram;

    GLenum depthFunc;
    GLboolean depthMask;
    GLenum cullFace;
    GLenum frontFace;
    struct { GLenum func; GLint ref; GLuint mask; } stencilFunc;
    struct { GLenum sfail, dpfail, dppass; } stencilOp;
    GLuint stencilMask;
    GLboolean colorMask; // r channel (all four are set the same by the engine)
    struct { GLfloat factor, units; } polygonOffset;
    struct { GLenum src, dst; } blendFunc;

    GLenum activeTexture;         // last glActiveTexture
    GLuint slotTexture[8];        // texture id bound while unit N was active
    struct { GLint internalFormat, format; GLsizei w, h; } texImage; // last glTexImage2D

    GLint sampleCount;            // what glGetIntegerv(GL_SAMPLES) returns

    // enable/disable tracking (small fixed set)
    GLenum enabledCaps[16];
    int enabledCount;

    // shader bookkeeping
    int attachCount;              // glAttachShader calls since reset
    const GLchar* lastShaderSource;
    GLuint deletedProgram;        // last glDeleteProgram
    GLuint lastDeletedTexture;    // last id passed to glDeleteTextures
    int deletedTextureCount;      // glDeleteTextures calls since reset

    // draw + buffer bursts (cleared on reset)
    StubDrawArrays drawArrays[GLSTUB_MAX_DRAWS];
    int drawArraysCount;
    GLsizei drawElements[GLSTUB_MAX_DRAWS];
    int drawElementsCount;
    GLsizeiptr bufferSubData[GLSTUB_MAX_SIZES];
    int bufferSubDataCount;

    // VAO bind history (to observe the batch-buffer ring)
    GLuint vaoBinds[GLSTUB_MAX_DRAWS];
    int vaoBindCount;

    GLDEBUGPROC debugCallback; // last glDebugMessageCallback
} GLStub;

extern GLStub glstub;

void glstub_reset(void);
int glstub_is_enabled(GLenum cap);
