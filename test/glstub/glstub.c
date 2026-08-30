// No-op / recording implementations of the fake GL layer. See glstub.h.

#include "glstub.h"

#include <string.h>

GLStub glstub;

// mge_config.h defines SUPPORT_TRACELOG, so mge_gl.c's TRACE_LOG expands to this
void Trace_Log(int logType, const char* text, ...) { (void)logType; (void)text; }

void glstub_reset(void)
{
    GLuint keepId = glstub.nextId ? glstub.nextId : 1;
    GLint keepSamples = glstub.sampleCount;
    memset(&glstub, 0, sizeof(glstub));
    glstub.nextId = keepId;
    glstub.sampleCount = keepSamples;
}

int glstub_is_enabled(GLenum cap)
{
    for (int i = 0; i < glstub.enabledCount; i++)
        if (glstub.enabledCaps[i] == cap)
            return 1;
    return 0;
}

static GLuint next_id(void)
{
    if (glstub.nextId == 0)
        glstub.nextId = 1;
    return glstub.nextId++;
}

// ---- extension loading ----
int gladLoadGLLoader(GLADloadproc load) { (void)load; return 1; }
const GLubyte* glGetString(GLenum name) { (void)name; return (const GLubyte*)"stub"; }

// ---- object creation ----
void glGenBuffers(GLsizei n, GLuint* b) { for (GLsizei i = 0; i < n; i++) b[i] = next_id(); }
void glGenTextures(GLsizei n, GLuint* t) { for (GLsizei i = 0; i < n; i++) t[i] = next_id(); }
void glGenVertexArrays(GLsizei n, GLuint* a) { for (GLsizei i = 0; i < n; i++) a[i] = next_id(); }
GLuint glCreateShader(GLenum type) { (void)type; return next_id(); }
GLuint glCreateProgram(void) { return next_id(); }

void glDeleteBuffers(GLsizei n, const GLuint* b) { (void)n; (void)b; }
void glDeleteVertexArrays(GLsizei n, const GLuint* a) { (void)n; (void)a; }
void glDeleteShader(GLuint s) { (void)s; }
void glDeleteProgram(GLuint p) { glstub.deletedProgram = p; }

// ---- shaders ----
void glShaderSource(GLuint s, GLsizei c, const GLchar* const* str, const GLint* len)
{
    (void)s; (void)c; (void)len;
    glstub.lastShaderSource = (str && c > 0) ? str[0] : NULL;
}
void glCompileShader(GLuint s) { (void)s; }
void glLinkProgram(GLuint p) { (void)p; }
void glAttachShader(GLuint p, GLuint s) { (void)p; (void)s; glstub.attachCount++; }
void glGetShaderiv(GLuint s, GLenum pname, GLint* params) { (void)s; (void)pname; *params = GL_TRUE; }
void glGetProgramiv(GLuint p, GLenum pname, GLint* params) { (void)p; (void)pname; *params = GL_TRUE; }
void glGetShaderInfoLog(GLuint s, GLsizei n, GLsizei* l, GLchar* log) { (void)s; (void)n; (void)l; if (log && n > 0) log[0] = 0; }
void glGetProgramInfoLog(GLuint p, GLsizei n, GLsizei* l, GLchar* log) { (void)p; (void)n; (void)l; if (log && n > 0) log[0] = 0; }
void glUseProgram(GLuint p) { glstub.usedProgram = p; }
GLint glGetUniformLocation(GLuint p, const GLchar* n) { (void)p; (void)n; return 1; }
GLint glGetAttribLocation(GLuint p, const GLchar* n) { (void)p; (void)n; return 0; }

void glUniform1f(GLint l, GLfloat v) { (void)l; (void)v; }
void glUniform1i(GLint l, GLint v) { (void)l; (void)v; }
void glUniform3fv(GLint l, GLsizei c, const GLfloat* v) { (void)l; (void)c; (void)v; }
void glUniform4fv(GLint l, GLsizei c, const GLfloat* v) { (void)l; (void)c; (void)v; }
void glUniformMatrix4fv(GLint l, GLsizei c, GLboolean t, const GLfloat* v) { (void)l; (void)c; (void)t; (void)v; }

// ---- buffers / VAOs ----
void glBindBuffer(GLenum target, GLuint buffer) { (void)target; (void)buffer; }
void glBindVertexArray(GLuint a)
{
    if (glstub.vaoBindCount < GLSTUB_MAX_DRAWS)
        glstub.vaoBinds[glstub.vaoBindCount++] = a;
}
void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) { (void)target; (void)size; (void)data; (void)usage; }
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data)
{
    (void)target; (void)offset; (void)data;
    if (glstub.bufferSubDataCount < GLSTUB_MAX_SIZES)
        glstub.bufferSubData[glstub.bufferSubDataCount++] = size;
}
void glVertexAttribPointer(GLuint i, GLint s, GLenum t, GLboolean n, GLsizei st, const void* p) { (void)i; (void)s; (void)t; (void)n; (void)st; (void)p; }
void glEnableVertexAttribArray(GLuint i) { (void)i; }
void glDisableVertexAttribArray(GLuint i) { (void)i; }
void glVertexAttrib4f(GLuint i, GLfloat x, GLfloat y, GLfloat z, GLfloat w) { (void)i; (void)x; (void)y; (void)z; (void)w; }

// ---- draws ----
void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    if (glstub.drawArraysCount < GLSTUB_MAX_DRAWS)
        glstub.drawArrays[glstub.drawArraysCount++] = (StubDrawArrays){ mode, first, count };
}
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
    (void)mode; (void)type; (void)indices;
    if (glstub.drawElementsCount < GLSTUB_MAX_DRAWS)
        glstub.drawElements[glstub.drawElementsCount++] = count;
}

// ---- textures ----
void glActiveTexture(GLenum t) { glstub.activeTexture = t; }
void glBindTexture(GLenum target, GLuint texture)
{
    (void)target;
    int unit = (int)(glstub.activeTexture - GL_TEXTURE0);
    if (unit >= 0 && unit < 8)
        glstub.slotTexture[unit] = texture;
}
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei w, GLsizei h,
    GLint border, GLenum format, GLenum type, const void* pixels)
{
    (void)target; (void)level; (void)border; (void)type; (void)pixels;
    glstub.texImage.internalFormat = internalformat;
    glstub.texImage.format = (GLint)format;
    glstub.texImage.w = w;
    glstub.texImage.h = h;
}
void glTexParameteri(GLenum a, GLenum b, GLint c) { (void)a; (void)b; (void)c; }
void glTexParameteriv(GLenum a, GLenum b, const GLint* c) { (void)a; (void)b; (void)c; }
void glGenerateMipmap(GLenum t) { (void)t; }
void glPixelStorei(GLenum p, GLint v) { (void)p; (void)v; }

// ---- fixed-function state ----
void glViewport(GLint x, GLint y, GLsizei w, GLsizei h) { glstub.viewport.x = x; glstub.viewport.y = y; glstub.viewport.w = w; glstub.viewport.h = h; }
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { glstub.clearColor[0] = r; glstub.clearColor[1] = g; glstub.clearColor[2] = b; glstub.clearColor[3] = a; }
void glClear(GLbitfield mask) { glstub.lastClearMask = mask; }
void glDepthFunc(GLenum f) { glstub.depthFunc = f; }
void glDepthMask(GLboolean f) { glstub.depthMask = f; }
void glCullFace(GLenum m) { glstub.cullFace = m; }
void glFrontFace(GLenum m) { glstub.frontFace = m; }
void glStencilFunc(GLenum f, GLint r, GLuint m) { glstub.stencilFunc.func = f; glstub.stencilFunc.ref = r; glstub.stencilFunc.mask = m; }
void glStencilOp(GLenum sf, GLenum df, GLenum dp) { glstub.stencilOp.sfail = sf; glstub.stencilOp.dpfail = df; glstub.stencilOp.dppass = dp; }
void glStencilMask(GLuint m) { glstub.stencilMask = m; }
void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) { (void)g; (void)b; (void)a; glstub.colorMask = r; }
void glPolygonOffset(GLfloat factor, GLfloat units) { glstub.polygonOffset.factor = factor; glstub.polygonOffset.units = units; }
void glBlendFunc(GLenum s, GLenum d) { glstub.blendFunc.src = s; glstub.blendFunc.dst = d; }
void glGetIntegerv(GLenum pname, GLint* data) { *data = (pname == GL_SAMPLES) ? glstub.sampleCount : 0; }

void glEnable(GLenum cap)
{
    if (!glstub_is_enabled(cap) && glstub.enabledCount < 16)
        glstub.enabledCaps[glstub.enabledCount++] = cap;
}
void glDisable(GLenum cap)
{
    for (int i = 0; i < glstub.enabledCount; i++)
        if (glstub.enabledCaps[i] == cap) {
            glstub.enabledCaps[i] = glstub.enabledCaps[--glstub.enabledCount];
            return;
        }
}
GLboolean glIsEnabled(GLenum cap) { return (GLboolean)(glstub_is_enabled(cap) ? GL_TRUE : GL_FALSE); }

// ---- KHR_debug ----
static void stub_debugMessageCallback(GLDEBUGPROC cb, const void* user) { (void)user; glstub.debugCallback = cb; }
PFNGLDEBUGMESSAGECALLBACKPROC glad_glDebugMessageCallback = stub_debugMessageCallback;
void glDebugMessageControl(GLenum s, GLenum t, GLenum sev, GLsizei n, const GLuint* ids, GLboolean e)
{
    (void)s; (void)t; (void)sev; (void)n; (void)ids; (void)e;
}
