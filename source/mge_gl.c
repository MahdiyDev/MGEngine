#include "mge_gl.h"
#include "mge.h"
#include "mge_math.h"
#include "mge_utils.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>

typedef struct MgeGL_DrawCall {
    int mode;
    int vertexAlignment;
    int vertexCount;
} MgeGL_DrawCall;

// mlib: growable draw-call list and matrix stack
#include "vec/vec.h"
DEFINE_VEC(MgeGL_DrawCall, MgeGL_DrawList);
DEFINE_VEC(Matrix, MgeGL_MatrixStack);

typedef struct GlData {
    struct {
        int vertexCounter;
        // one set of dynamic buffers per MGEGL_BATCH_BUFFERS; `currentBuffer`
        // advances on every flush so uploads don't stall on an in-flight buffer
        unsigned int vao[MGEGL_BATCH_BUFFERS];
        unsigned int vboPos[MGEGL_BATCH_BUFFERS];
        unsigned int vboColor[MGEGL_BATCH_BUFFERS];
        unsigned int vboTexcoord[MGEGL_BATCH_BUFFERS];
        unsigned int vboNormal[MGEGL_BATCH_BUFFERS];
        unsigned int ebo;            // shared: the index pattern is the same for every buffer
        int currentBuffer;
        unsigned int defaultTexture; // currently bound texture (white 1x1 by default)
        unsigned int whiteTexture;   // the 1x1 white texture, kept for restoring
        unsigned int defaultShaderID;
        unsigned int currentShaderID;
        unsigned char colorr, colorg, colorb, colora;
        float texcoordx, texcoordy;
        float normalx, normaly, normalz;

        Matrix modelview, projection, transform;
        Matrix* currentMatrix;
        int currentMatrixMode;
        MgeGL_MatrixStack stack;
        bool transformRequired;

        VertexData vertexBuffer;
        float currentDepth;
        int drawCalls;          // GL draw calls so far this frame
        int drawCallsLastFrame; // ...snapshotted at the start of each frame

        int framebufferWidth;
        int framebufferHeight;

        MgeGL_DrawList draws;
    } State;
} GlData;

static const char* vertexShaderCode = "#version 330 core\n"
                                      "layout(location = 0) in vec3 aPos;\n"
                                      "layout(location = 1) in vec4 aColor;\n"
                                      "layout(location = 2) in vec2 aTexCoord;\n"
                                      // location 3 (aNormal) is left to the lighting shader; the shared
                                      // VAO keeps it enabled, an unused attribute is harmless here.
                                      "out vec4 vertexColor;\n"
                                      "out vec2 texCoord;\n"
                                      "uniform mat4 modelview;\n"
                                      "uniform mat4 projection;\n"
                                      "void main()\n"
                                      "{\n"
                                      "	gl_Position = projection * modelview  * vec4(aPos, 1.0);\n"
                                      "	vertexColor = aColor;\n"
                                      "	texCoord = aTexCoord;\n"
                                      "}\n";

static const char* fragmentShaderCode = "#version 330 core\n"
                                        "out vec4 FragColor;\n"
                                        "in vec4 vertexColor;\n"
                                        "in vec2 texCoord;\n"
                                        "uniform sampler2D sampleTex;\n"
                                        "void main()\n"
                                        "{\n"
                                        "	FragColor = texture(sampleTex, texCoord) * vertexColor;\n"
                                        "}\n";

static GlData MGEGL = { 0 };

// The current (most recently opened) draw call.
static MgeGL_DrawCall* CurrentDraw(void)
{
    return &MGEGL.State.draws.items[MGEGL.State.draws.count - 1];
}

void MgeGL_Init(int width, int height)
{
    MGEGL.State.framebufferWidth = width;
    MGEGL.State.framebufferHeight = height;
    MGEGL.State.vertexCounter = 0;
    MGEGL.State.currentDepth = -1.0f;

    // elementCount counts batch "quads"; the vertex buffers hold 4 vertices each.
    const int quadCount = MAX_BUFFER_ELEMENTS;
    const int vertCount = quadCount * 4;
    MGEGL.State.vertexBuffer.elementCount = quadCount;
    MGEGL.State.vertexBuffer.vertices = (float*)malloc(vertCount * 3 * sizeof(float));
    MGEGL.State.vertexBuffer.colors = (unsigned char*)malloc(vertCount * 4 * sizeof(unsigned char));
    MGEGL.State.vertexBuffer.texcoords = (float*)malloc(vertCount * 2 * sizeof(float));
    MGEGL.State.vertexBuffer.normals = (float*)malloc(vertCount * 3 * sizeof(float));
    MGEGL.State.vertexBuffer.indices = (unsigned int*)malloc(quadCount * 6 * sizeof(unsigned int));

    for (int i = 0; i < vertCount * 3; i++)
        MGEGL.State.vertexBuffer.vertices[i] = 0.0f;
    for (int i = 0; i < vertCount * 4; i++)
        MGEGL.State.vertexBuffer.colors[i] = 0;
    for (int i = 0; i < vertCount * 2; i++)
        MGEGL.State.vertexBuffer.texcoords[i] = 0.0f;
    for (int i = 0; i < vertCount * 3; i++)
        MGEGL.State.vertexBuffer.normals[i] = 0.0f;

    MGEGL.State.normalx = 0.0f;
    MGEGL.State.normaly = 0.0f;
    MGEGL.State.normalz = 1.0f;

    for (int k = 0; k < quadCount; k++) {
        unsigned int* idx = &MGEGL.State.vertexBuffer.indices[k * 6];
        idx[0] = 4 * k + 0;
        idx[1] = 4 * k + 1;
        idx[2] = 4 * k + 2;
        idx[3] = 4 * k + 0;
        idx[4] = 4 * k + 2;
        idx[5] = 4 * k + 3;
    }

    // Start with a single empty draw call.
    MGEGL.State.draws = (MgeGL_DrawList){ 0 };
    MgeGL_DrawList_append(&MGEGL.State.draws,
        (MgeGL_DrawCall){ .mode = MGEGL_QUADS, .vertexAlignment = 0, .vertexCount = 0 });

    MGEGL.State.stack = (MgeGL_MatrixStack){ 0 };

    MGEGL.State.modelview = Matrix_Identity();
    MGEGL.State.projection = Matrix_Identity();
    MGEGL.State.currentMatrix = &MGEGL.State.modelview;

    // Load default (white 1x1) texture
    unsigned char pixels[4] = { 255, 255, 255, 255 };
    MGEGL.State.whiteTexture = MgeGL_LoadTexture(pixels, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
    MGEGL.State.defaultTexture = MGEGL.State.whiteTexture;

    // Init default shader
    unsigned int vertex = MgeGL_LoadShader(vertexShaderCode, GL_VERTEX_SHADER, "vertex");
    unsigned int fragment = MgeGL_LoadShader(fragmentShaderCode, GL_FRAGMENT_SHADER, "fragment");
    MGEGL.State.defaultShaderID = MgeGL_CreateShaderProgram(vertex, fragment);
    MGEGL.State.currentShaderID = MGEGL.State.defaultShaderID;

    // NOTE: attribute locations are FIXED (see AttribLocations) so each buffer's
    // VAO stays valid for every shader -- the default one and any custom/lighting
    // shader, all of which must declare `layout(location = N)` to match.

    // the index pattern (4 verts -> 6 indices per quad) never changes: one static
    // EBO shared across every buffer's VAO
    glGenBuffers(1, &MGEGL.State.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, MGEGL.State.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, quadCount * 6 * sizeof(unsigned int), MGEGL.State.vertexBuffer.indices, GL_STATIC_DRAW);

    MGEGL.State.currentBuffer = 0;
    for (int b = 0; b < MGEGL_BATCH_BUFFERS; b++) {
        glGenVertexArrays(1, &MGEGL.State.vao[b]);
        glBindVertexArray(MGEGL.State.vao[b]);

        // positions
        glGenBuffers(1, &MGEGL.State.vboPos[b]);
        glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.vboPos[b]);
        glBufferData(GL_ARRAY_BUFFER, vertCount * 3 * sizeof(float), MGEGL.State.vertexBuffer.vertices, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(VERTICE_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(VERTICE_LOCATION);
        // colors
        glGenBuffers(1, &MGEGL.State.vboColor[b]);
        glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.vboColor[b]);
        glBufferData(GL_ARRAY_BUFFER, vertCount * 4 * sizeof(unsigned char), MGEGL.State.vertexBuffer.colors, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(COLOR_LOCATION, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
        glEnableVertexAttribArray(COLOR_LOCATION);
        // texcoords
        glGenBuffers(1, &MGEGL.State.vboTexcoord[b]);
        glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.vboTexcoord[b]);
        glBufferData(GL_ARRAY_BUFFER, vertCount * 2 * sizeof(float), MGEGL.State.vertexBuffer.texcoords, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(TEXTURE_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(TEXTURE_LOCATION);
        // normals (consumed by the lighting shader)
        glGenBuffers(1, &MGEGL.State.vboNormal[b]);
        glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.vboNormal[b]);
        glBufferData(GL_ARRAY_BUFFER, vertCount * 3 * sizeof(float), MGEGL.State.vertexBuffer.normals, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(NORMAL_LOCATION);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, MGEGL.State.ebo); // recorded into this VAO
    }
    glBindVertexArray(0);

    glUseProgram(MGEGL.State.currentShaderID);

    // MSAA: harmless when the framebuffer isn't multisampled; on when the window
    // was created with GLFW_SAMPLES >= 2 (see Mge_SetMSAA / Mge_GetRequestedMSAA)
    glEnable(GL_MULTISAMPLE);
}

unsigned int MgeGL_GetDefaultShaderId(void)
{
    return MGEGL.State.defaultShaderID;
}

Matrix MgeGL_GetMatrixModelview(void)
{
    return MGEGL.State.modelview;
}

Matrix MgeGL_GetMatrixProjection(void)
{
    return MGEGL.State.projection;
}

unsigned int MgeGL_GetCurrentShaderId(void)
{
    return MGEGL.State.currentShaderID;
}

void MgeGL_SetShader(unsigned int id)
{
    if (MGEGL.State.currentShaderID != id) {
        MgeGL_Draw(); // flush whatever was queued with the previous shader
        MGEGL.State.currentShaderID = id;
        glUseProgram(id); // keep the active program in sync so MgeGL_Uniform* land on it
    }
}

int MgeGL_LoadTexture(const void* data, int width, int height, int format, int mipmapCount)
{
    (void)mipmapCount;
    unsigned int id = 0;

    if (data == NULL) {
        TRACE_LOG(LOG_WARNING, "TEXTURE: no pixel data to upload");
        return 0;
    }

    // map the engine's PixelFormat to a GL channel layout
    GLint internalFmt = GL_RGB8;
    GLenum channels = GL_RGB;
    int gray = 0; // 1 = R -> RRR1, 2 = RG -> RRRG

    switch (format) {
    case PIXELFORMAT_UNCOMPRESSED_GRAYSCALE:  internalFmt = GL_R8;    channels = GL_RED;  gray = 1; break;
    case PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA: internalFmt = GL_RG8;   channels = GL_RG;   gray = 2; break;
    case PIXELFORMAT_UNCOMPRESSED_R8G8B8:     internalFmt = GL_RGB8;  channels = GL_RGB;  break;
    case PIXELFORMAT_UNCOMPRESSED_R8G8B8A8:   internalFmt = GL_RGBA8; channels = GL_RGBA; break;
    default:                                  internalFmt = GL_RGB8;  channels = GL_RGB;  break;
    }

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // rows are not guaranteed 4-byte aligned
    glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, width, height, 0, channels, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (gray == 1) {
        GLint swz[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swz);
    } else if (gray == 2) {
        GLint swz[4] = { GL_RED, GL_RED, GL_RED, GL_GREEN };
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swz);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    return (int)id;
}

void MgeGL_Close(void)
{
    glDisableVertexAttribArray(VERTICE_LOCATION);
    glDisableVertexAttribArray(COLOR_LOCATION);
    glDisableVertexAttribArray(TEXTURE_LOCATION);
    glDisableVertexAttribArray(NORMAL_LOCATION);
    for (int b = 0; b < MGEGL_BATCH_BUFFERS; b++) {
        glDeleteBuffers(1, &MGEGL.State.vboPos[b]);
        glDeleteBuffers(1, &MGEGL.State.vboColor[b]);
        glDeleteBuffers(1, &MGEGL.State.vboTexcoord[b]);
        glDeleteBuffers(1, &MGEGL.State.vboNormal[b]);
        glDeleteVertexArrays(1, &MGEGL.State.vao[b]);
    }
    glDeleteBuffers(1, &MGEGL.State.ebo);
    glDeleteProgram(MGEGL.State.currentShaderID);

    free(MGEGL.State.vertexBuffer.vertices);
    free(MGEGL.State.vertexBuffer.colors);
    free(MGEGL.State.vertexBuffer.texcoords);
    free(MGEGL.State.vertexBuffer.normals);
    free(MGEGL.State.vertexBuffer.indices);
    MgeGL_DrawList_free(&MGEGL.State.draws);
    MgeGL_MatrixStack_free(&MGEGL.State.stack);
}

void MgeGL_Begin(int mode)
{
    if (CurrentDraw()->mode == mode)
        return;

    MgeGL_DrawCall* draw = CurrentDraw();
    if (draw->vertexCount > 0) {
        int align;
        if (draw->mode == MGEGL_LINES)
            align = (draw->vertexCount < 4) ? draw->vertexCount : draw->vertexCount % 4;
        else if (draw->mode == MGEGL_TRIANGLES)
            align = (draw->vertexCount < 4) ? 1 : (4 - (draw->vertexCount % 4));
        else
            align = 0;
        draw->vertexAlignment = align;

        // NOTE: MgeGL_CheckRenderBatchLimit() may flush (MgeGL_Draw), which
        // rebuilds the draw list -- re-fetch the current draw below.
        if (!MgeGL_CheckRenderBatchLimit(align)) {
            MGEGL.State.vertexCounter += align;
            MgeGL_DrawList_append(&MGEGL.State.draws, (MgeGL_DrawCall){ 0 });
        }
    }

    MgeGL_DrawCall* cur = CurrentDraw();
    cur->mode = mode;
    cur->vertexCount = 0;
    cur->vertexAlignment = 0;
}

bool MgeGL_CheckRenderBatchLimit(int vCount)
{
    bool overflow = false;

    if ((MGEGL.State.vertexCounter + vCount) >= (MGEGL.State.vertexBuffer.elementCount * 4)) {
        overflow = true;

        int currentMode = CurrentDraw()->mode;

        MgeGL_Draw();

        // Restore state of last batch so we can keep adding vertices
        CurrentDraw()->mode = currentMode;
    }

    return overflow;
}

void MgeGL_End(void)
{
    MGEGL.State.currentDepth += (1.0f / 20000.0f);
}

void MgeGL_Draw(void)
{
    if (MGEGL.State.vertexCounter > 0) {
        // NOTE: glUniform* affects the *active* program, so bind it first.
        glUseProgram(MGEGL.State.currentShaderID);

        MgeGL_UniformMatrix4fv("modelview", MGEGL.State.modelview);
        MgeGL_UniformMatrix4fv("projection", MGEGL.State.projection);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, MGEGL.State.defaultTexture);

        // this flush writes to (and draws from) the next buffer in the ring, so
        // the upload doesn't wait on a buffer the GPU still has in flight
        const int b = MGEGL.State.currentBuffer;
        glBindVertexArray(MGEGL.State.vao[b]);

        glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.vboPos[b]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, MGEGL.State.vertexCounter * 3 * sizeof(float), MGEGL.State.vertexBuffer.vertices);
        glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.vboColor[b]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, MGEGL.State.vertexCounter * 4 * sizeof(unsigned char), MGEGL.State.vertexBuffer.colors);
        glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.vboTexcoord[b]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, MGEGL.State.vertexCounter * 2 * sizeof(float), MGEGL.State.vertexBuffer.texcoords);
        glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.vboNormal[b]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, MGEGL.State.vertexCounter * 3 * sizeof(float), MGEGL.State.vertexBuffer.normals);

        int vertexOffset = 0;
        for (size_t i = 0; i < MGEGL.State.draws.count; i++) {
            MgeGL_DrawCall* d = &MGEGL.State.draws.items[i];
            if (d->vertexCount > 0) {
                if (d->mode == MGEGL_LINES || d->mode == MGEGL_TRIANGLES) {
                    glDrawArrays(d->mode, vertexOffset, d->vertexCount);
                } else {
                    glDrawElements(GL_TRIANGLES, d->vertexCount / 4 * 6, GL_UNSIGNED_INT,
                        (GLvoid*)(intptr_t)(vertexOffset / 4 * 6 * sizeof(GLuint)));
                }
                MGEGL.State.drawCalls++;
            }
            vertexOffset += (d->vertexCount + d->vertexAlignment);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);

        MGEGL.State.currentBuffer = (MGEGL.State.currentBuffer + 1) % MGEGL_BATCH_BUFFERS;
    }

    MGEGL.State.vertexCounter = 0;
    MGEGL.State.currentDepth = -1.0f;

    // Collapse back to a single empty draw call.
    MGEGL.State.draws.count = 0;
    MgeGL_DrawList_append(&MGEGL.State.draws, (MgeGL_DrawCall){ .mode = MGEGL_QUADS });
}

void MgeGL_SetTexture(unsigned int id)
{
    unsigned int tex = (id != 0) ? id : MGEGL.State.whiteTexture;
    if (tex == MGEGL.State.defaultTexture)
        return;

    // queued geometry was recorded against the old texture -- one bind is
    // active per batch, so flush before switching
    if (MGEGL.State.vertexCounter > 0)
        MgeGL_Draw();

    MGEGL.State.defaultTexture = tex;
}

void MgeGL_RegisterDrawCall(void)
{
    MGEGL.State.drawCalls++; // for GL draws issued outside MgeGL_Draw (meshes, feature modules)
}

int MgeGL_GetDrawCalls(void)
{
    return MGEGL.State.drawCallsLastFrame; // the last *completed* frame -- stable to read mid-frame
}

void MgeGL_ResetDrawCalls(void)
{
    MGEGL.State.drawCallsLastFrame = MGEGL.State.drawCalls;
    MGEGL.State.drawCalls = 0;
}

unsigned int MgeGL_GetWhiteTexture(void)
{
    return MGEGL.State.whiteTexture;
}

void MgeGL_MatrixMode(int mode)
{
    if (mode == MGEGL_PROJECTION)
        MGEGL.State.currentMatrix = &MGEGL.State.projection;
    else if (mode == MGEGL_MODELVIEW)
        MGEGL.State.currentMatrix = &MGEGL.State.modelview;

    MGEGL.State.currentMatrixMode = mode;
}

void MgeGL_LoadIdentity(void)
{
    *MGEGL.State.currentMatrix = Matrix_Identity();
}

void MgeGL_Translatef(float x, float y, float z)
{
    Matrix matTranslation = {
        1.0f, 0.0f, 0.0f, x,
        0.0f, 1.0f, 0.0f, y,
        0.0f, 0.0f, 1.0f, z,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    *MGEGL.State.currentMatrix = Matrix_Multiply(matTranslation, *MGEGL.State.currentMatrix);
}

void MgeGL_Rotatef(float angle, float x, float y, float z)
{
    Matrix matRotation = Matrix_Identity();

    float lengthSquared = x * x + y * y + z * z;
    if ((lengthSquared != 1.0f) && (lengthSquared != 0.0f)) {
        float inverseLength = 1.0f / sqrtf(lengthSquared);
        x *= inverseLength;
        y *= inverseLength;
        z *= inverseLength;
    }

    float sinres = sinf(DEG2RAD * angle);
    float cosres = cosf(DEG2RAD * angle);
    float t = 1.0f - cosres;

    matRotation.m0 = x * x * t + cosres;
    matRotation.m1 = y * x * t + z * sinres;
    matRotation.m2 = z * x * t - y * sinres;
    matRotation.m3 = 0.0f;

    matRotation.m4 = x * y * t - z * sinres;
    matRotation.m5 = y * y * t + cosres;
    matRotation.m6 = z * y * t + x * sinres;
    matRotation.m7 = 0.0f;

    matRotation.m8 = x * z * t + y * sinres;
    matRotation.m9 = y * z * t - x * sinres;
    matRotation.m10 = z * z * t + cosres;
    matRotation.m11 = 0.0f;

    matRotation.m12 = 0.0f;
    matRotation.m13 = 0.0f;
    matRotation.m14 = 0.0f;
    matRotation.m15 = 1.0f;

    *MGEGL.State.currentMatrix = Matrix_Multiply(matRotation, *MGEGL.State.currentMatrix);
}

void MgeGL_MultMatrixf(const float* matf)
{
    Matrix mat = {
        matf[0], matf[4], matf[8], matf[12],
        matf[1], matf[5], matf[9], matf[13],
        matf[2], matf[6], matf[10], matf[14],
        matf[3], matf[7], matf[11], matf[15]
    };

    *MGEGL.State.currentMatrix = Matrix_Multiply(*MGEGL.State.currentMatrix, mat);
}

void MgeGL_PushMatrix(void)
{
    if (MGEGL.State.currentMatrixMode == MGEGL_MODELVIEW) {
        MGEGL.State.transformRequired = true;
        MGEGL.State.currentMatrix = &MGEGL.State.transform;
    }

    MgeGL_MatrixStack_append(&MGEGL.State.stack, *MGEGL.State.currentMatrix);
}

void MgeGL_PopMatrix(void)
{
    if (MGEGL.State.stack.count > 0) {
        *MGEGL.State.currentMatrix = MGEGL.State.stack.items[MGEGL.State.stack.count - 1];
        MGEGL.State.stack.count--;
    }

    if ((MGEGL.State.stack.count == 0) && (MGEGL.State.currentMatrixMode == MGEGL_MODELVIEW)) {
        MGEGL.State.currentMatrix = &MGEGL.State.modelview;
        MGEGL.State.transformRequired = false;
    }
}

void MgeGL_Color4ub(unsigned char x, unsigned char y, unsigned char z, unsigned char w)
{
    MGEGL.State.colorr = x;
    MGEGL.State.colorg = y;
    MGEGL.State.colorb = z;
    MGEGL.State.colora = w;
}

void MgeGL_TexCoord2f(float x, float y)
{
    MGEGL.State.texcoordx = x;
    MGEGL.State.texcoordy = y;
}

void MgeGL_Normal3f(float x, float y, float z)
{
    MGEGL.State.normalx = x;
    MGEGL.State.normaly = y;
    MGEGL.State.normalz = z;
}

void MgeGL_Vertex2i(int x, int y)
{
    MgeGL_Vertex3f((float)x, (float)y, MGEGL.State.currentDepth);
}

void MgeGL_Vertex2f(float x, float y)
{
    MgeGL_Vertex3f(x, y, MGEGL.State.currentDepth);
}

void MgeGL_Vertex3f(float x, float y, float z)
{
    float tx = x;
    float ty = y;
    float tz = z;

    if (MGEGL.State.transformRequired) {
        tx = MGEGL.State.transform.m0 * x + MGEGL.State.transform.m4 * y + MGEGL.State.transform.m8 * z + MGEGL.State.transform.m12;
        ty = MGEGL.State.transform.m1 * x + MGEGL.State.transform.m5 * y + MGEGL.State.transform.m9 * z + MGEGL.State.transform.m13;
        tz = MGEGL.State.transform.m2 * x + MGEGL.State.transform.m6 * y + MGEGL.State.transform.m10 * z + MGEGL.State.transform.m14;
    }

    if (MGEGL.State.vertexCounter > (MGEGL.State.vertexBuffer.elementCount * 4 - 4)) {
        int mode = CurrentDraw()->mode;
        int vcount = CurrentDraw()->vertexCount;
        if ((mode == MGEGL_LINES) && (vcount % 2 == 0))
            MgeGL_CheckRenderBatchLimit(2 + 1);
        else if ((mode == MGEGL_TRIANGLES) && (vcount % 3 == 0))
            MgeGL_CheckRenderBatchLimit(3 + 1);
        else if ((mode == MGEGL_QUADS) && (vcount % 4 == 0))
            MgeGL_CheckRenderBatchLimit(4 + 1);
    }

    int vc = MGEGL.State.vertexCounter;
    MGEGL.State.vertexBuffer.vertices[3 * vc + 0] = tx;
    MGEGL.State.vertexBuffer.vertices[3 * vc + 1] = ty;
    MGEGL.State.vertexBuffer.vertices[3 * vc + 2] = tz;
    MGEGL.State.vertexBuffer.colors[4 * vc + 0] = MGEGL.State.colorr;
    MGEGL.State.vertexBuffer.colors[4 * vc + 1] = MGEGL.State.colorg;
    MGEGL.State.vertexBuffer.colors[4 * vc + 2] = MGEGL.State.colorb;
    MGEGL.State.vertexBuffer.colors[4 * vc + 3] = MGEGL.State.colora;
    MGEGL.State.vertexBuffer.texcoords[2 * vc + 0] = MGEGL.State.texcoordx;
    MGEGL.State.vertexBuffer.texcoords[2 * vc + 1] = MGEGL.State.texcoordy;

    float nx = MGEGL.State.normalx, ny = MGEGL.State.normaly, nz = MGEGL.State.normalz;
    if (MGEGL.State.transformRequired) {
        // rotate the normal by the transform's upper 3x3 (translation ignored)
        float rx = MGEGL.State.transform.m0 * nx + MGEGL.State.transform.m4 * ny + MGEGL.State.transform.m8 * nz;
        float ry = MGEGL.State.transform.m1 * nx + MGEGL.State.transform.m5 * ny + MGEGL.State.transform.m9 * nz;
        float rz = MGEGL.State.transform.m2 * nx + MGEGL.State.transform.m6 * ny + MGEGL.State.transform.m10 * nz;
        nx = rx;
        ny = ry;
        nz = rz;
    }
    MGEGL.State.vertexBuffer.normals[3 * vc + 0] = nx;
    MGEGL.State.vertexBuffer.normals[3 * vc + 1] = ny;
    MGEGL.State.vertexBuffer.normals[3 * vc + 2] = nz;

    MGEGL.State.vertexCounter++;
    CurrentDraw()->vertexCount++;
}

void MgeGL_Frustum(double left, double right, double bottom, double top, double znear, double zfar)
{
    Matrix matFrustum = { 0 };

    float rl = (float)(right - left);
    float tb = (float)(top - bottom);
    float fn = (float)(zfar - znear);

    matFrustum.m0 = ((float)znear * 2.0f) / rl;
    matFrustum.m5 = ((float)znear * 2.0f) / tb;
    matFrustum.m8 = ((float)right + (float)left) / rl;
    matFrustum.m9 = ((float)top + (float)bottom) / tb;
    matFrustum.m10 = -((float)zfar + (float)znear) / fn;
    matFrustum.m11 = -1.0f;
    matFrustum.m14 = -((float)zfar * (float)znear * 2.0f) / fn;
    matFrustum.m15 = 0.0f;

    *MGEGL.State.currentMatrix = Matrix_Multiply(*MGEGL.State.currentMatrix, matFrustum);
}

void MgeGL_Ortho(double left, double right, double bottom, double top, double znear, double zfar)
{
    Matrix matOrtho = { 0 };

    float rl = (float)(right - left);
    float tb = (float)(top - bottom);
    float fn = (float)(zfar - znear);

    matOrtho.m0 = 2.0f / rl;
    matOrtho.m5 = 2.0f / tb;
    matOrtho.m10 = -2.0f / fn;
    matOrtho.m12 = -((float)left + (float)right) / rl;
    matOrtho.m13 = -((float)top + (float)bottom) / tb;
    matOrtho.m14 = -((float)zfar + (float)znear) / fn;
    matOrtho.m15 = 1.0f;

    *MGEGL.State.currentMatrix = Matrix_Multiply(*MGEGL.State.currentMatrix, matOrtho);
}

int MgeGL_GetAttribLoc(const char* name)
{
    return glGetAttribLocation(MGEGL.State.currentShaderID, name);
}

void MgeGL_Uniform1i(const char* name, const int value)
{
    glUniform1i(glGetUniformLocation(MGEGL.State.currentShaderID, name), value);
}

void MgeGL_Uniform1f(const char* name, float value)
{
    glUniform1f(glGetUniformLocation(MGEGL.State.currentShaderID, name), value);
}

void MgeGL_Uniform3fv(const char* name, Vector3 value)
{
    float vectorValue[3] = { value.x, value.y, value.z };
    glUniform3fv(glGetUniformLocation(MGEGL.State.currentShaderID, name), 1, vectorValue);
}

void MgeGL_Uniform4fv(const char* name, Vector4 value)
{
    float vectorValue[4] = { value.x, value.y, value.z, value.w };
    glUniform4fv(glGetUniformLocation(MGEGL.State.currentShaderID, name), 1, vectorValue);
}

void MgeGL_UniformMatrix4fv(const char* name, Matrix value)
{
    float matValue[16] = {
        value.m0, value.m1, value.m2, value.m3,
        value.m4, value.m5, value.m6, value.m7,
        value.m8, value.m9, value.m10, value.m11,
        value.m12, value.m13, value.m14, value.m15
    };
    glUniformMatrix4fv(glGetUniformLocation(MGEGL.State.currentShaderID, name), 1, GL_FALSE, matValue);
}

unsigned int MgeGL_LoadShader(const char* code, unsigned int shaderType, const char* typeName)
{
    if (code == NULL) {
        TRACE_LOG(LOG_ERROR, "Shader: %s shader source is NULL", typeName);
        return 0;
    }

    unsigned int shader = glCreateShader(shaderType);
    if (shader == 0) {
        TRACE_LOG(LOG_ERROR, "Shader: Unable to create shader: %s", typeName);
        return 0;
    }
    TRACE_LOG(LOG_INFO, "Shader: %s shader created", typeName);

    glShaderSource(shader, 1, &code, NULL);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512] = { 0 };
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        TRACE_LOG(LOG_ERROR, "Shader: %s shader not compiled Error: %s", typeName, infoLog);
    } else {
        TRACE_LOG(LOG_INFO, "Shader: %s shader compiled", typeName);
    }
    return shader;
}

unsigned int MgeGL_CreateShaderProgram(unsigned int vertex, unsigned int fragment)
{
    unsigned int programID = glCreateProgram();
    glAttachShader(programID, vertex);
    glAttachShader(programID, fragment);
    glLinkProgram(programID);

    int success = 0;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512] = { 0 };
        glGetProgramInfoLog(programID, 512, NULL, infoLog);
        TRACE_LOG(LOG_ERROR, "Shader: program not created\n%s", infoLog);
    } else {
        TRACE_LOG(LOG_INFO, "Shader: program created");
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return programID;
}

unsigned int MgeGL_CreateShaderProgramGeo(unsigned int vertex, unsigned int geometry, unsigned int fragment)
{
    unsigned int programID = glCreateProgram();
    glAttachShader(programID, vertex);
    if (geometry != 0)
        glAttachShader(programID, geometry);
    glAttachShader(programID, fragment);
    glLinkProgram(programID);

    int success = 0;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512] = { 0 };
        glGetProgramInfoLog(programID, 512, NULL, infoLog);
        TRACE_LOG(LOG_ERROR, "Shader: geometry program not created\n%s", infoLog);
    }

    glDeleteShader(vertex);
    if (geometry != 0)
        glDeleteShader(geometry);
    glDeleteShader(fragment);
    return programID;
}

void MgeGL_UnloadShaderProgram(unsigned int id)
{
    glDeleteProgram(id);
    TRACE_LOG(LOG_INFO, "Shader: [ID %i] Unloaded shader program data from VRAM (GPU)", id);
}

void MgeGL_ClearColor(Color color)
{
    glClearColor((float)color.r / 255.0f, (float)color.g / 255.0f,
        (float)color.b / 255.0f, (float)color.a / 255.0f);
}

void MgeGL_ClearScreenBuffers(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void MgeGL_Viewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void MgeGL_Load_Extensions(void* loader)
{
    if (gladLoadGLLoader((GLADloadproc)loader) == 0) {
        TRACE_LOG(LOG_WARNING, "GLAD: Cannot load OpenGL extensions");
    } else {
        TRACE_LOG(LOG_INFO, "GLAD: OpenGL extensions loaded successfully");
    }

    TRACE_LOG(LOG_INFO, "GL: OpenGL device information:");
    TRACE_LOG(LOG_INFO, "	> Vendor:   %s", glGetString(GL_VENDOR));
    TRACE_LOG(LOG_INFO, "	> Renderer: %s", glGetString(GL_RENDERER));
    TRACE_LOG(LOG_INFO, "	> Version:  %s", glGetString(GL_VERSION));
    TRACE_LOG(LOG_INFO, "	> GLSL:	 %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
}

void MgeGL_EnableDepthTest(void) { glEnable(GL_DEPTH_TEST); }
void MgeGL_DisableDepthTest(void) { glDisable(GL_DEPTH_TEST); }

void MgeGL_SetDepthFunc(int depthFunc)
{
    GLenum f = GL_LESS;
    switch (depthFunc) {
    case DEPTH_NEVER:    f = GL_NEVER;    break;
    case DEPTH_LESS:     f = GL_LESS;     break;
    case DEPTH_EQUAL:    f = GL_EQUAL;    break;
    case DEPTH_LEQUAL:   f = GL_LEQUAL;   break;
    case DEPTH_GREATER:  f = GL_GREATER;  break;
    case DEPTH_NOTEQUAL: f = GL_NOTEQUAL; break;
    case DEPTH_GEQUAL:   f = GL_GEQUAL;   break;
    case DEPTH_ALWAYS:   f = GL_ALWAYS;   break;
    default: break;
    }
    MgeGL_Draw(); // flush geometry queued under the old func
    glDepthFunc(f);
}

void MgeGL_SetDepthMask(bool writeEnabled)
{
    MgeGL_Draw();
    glDepthMask(writeEnabled ? GL_TRUE : GL_FALSE);
}

void MgeGL_SetPolygonOffset(bool enabled, float factor, float units)
{
    MgeGL_Draw();
    if (enabled) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(factor, units);
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}

bool MgeGL_IsDepthTestEnabled(void)
{
    return glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
}

// map the shared StencilFunc / DepthFunc ordering to GL comparison enums
static GLenum ToGlCompare(int func)
{
    switch (func) {
    case 0:  return GL_NEVER;
    case 1:  return GL_LESS;
    case 2:  return GL_EQUAL;
    case 3:  return GL_LEQUAL;
    case 4:  return GL_GREATER;
    case 5:  return GL_NOTEQUAL;
    case 6:  return GL_GEQUAL;
    default: return GL_ALWAYS;
    }
}

static GLenum ToGlStencilOp(int op)
{
    switch (op) {
    case STENCIL_ZERO:      return GL_ZERO;
    case STENCIL_REPLACE:   return GL_REPLACE;
    case STENCIL_INCR:      return GL_INCR;
    case STENCIL_INCR_WRAP: return GL_INCR_WRAP;
    case STENCIL_DECR:      return GL_DECR;
    case STENCIL_DECR_WRAP: return GL_DECR_WRAP;
    case STENCIL_INVERT:    return GL_INVERT;
    default:                return GL_KEEP;
    }
}

void MgeGL_EnableStencilTest(void)
{
    MgeGL_Draw();
    glEnable(GL_STENCIL_TEST);
}

void MgeGL_DisableStencilTest(void)
{
    MgeGL_Draw();
    glDisable(GL_STENCIL_TEST);
}

void MgeGL_SetStencilFunc(int func, int ref, unsigned int mask)
{
    MgeGL_Draw();
    glStencilFunc(ToGlCompare(func), ref, mask);
}

void MgeGL_SetStencilOp(int onStencilFail, int onDepthFail, int onPass)
{
    MgeGL_Draw();
    glStencilOp(ToGlStencilOp(onStencilFail), ToGlStencilOp(onDepthFail), ToGlStencilOp(onPass));
}

void MgeGL_SetStencilMask(unsigned int mask)
{
    MgeGL_Draw();
    glStencilMask(mask);
}

void MgeGL_SetColorMask(bool enabled)
{
    MgeGL_Draw();
    GLboolean v = enabled ? GL_TRUE : GL_FALSE;
    glColorMask(v, v, v, v);
}

void MgeGL_ClearStencil(void)
{
    MgeGL_Draw();
    glClear(GL_STENCIL_BUFFER_BIT);
}

void MgeGL_SetFaceCulling(bool enabled)
{
    MgeGL_Draw();
    if (enabled)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
}

void MgeGL_SetCullFace(int face)
{
    GLenum f = (face == CULL_FRONT) ? GL_FRONT
        : (face == CULL_FRONT_AND_BACK) ? GL_FRONT_AND_BACK
                                        : GL_BACK;
    MgeGL_Draw();
    glCullFace(f);
}

void MgeGL_SetFrontFace(int winding)
{
    MgeGL_Draw();
    glFrontFace((winding == WINDING_CW) ? GL_CW : GL_CCW);
}

int MgeGL_GetSampleCount(void)
{
    GLint samples = 0;
    glGetIntegerv(GL_SAMPLES, &samples);
    return (int)samples;
}

// ----- retained indexed meshes -----
//
// Each mesh keeps its own VAO/VBO/EBO. Vertices are interleaved floats:
//   [0..2] position   [3..5] normal   [6..7] texcoord   (stride 8 floats)
// matching `Vertex` in mge.h and the fixed AttribLocations.

void MgeGL_UploadMesh(unsigned int* vao, unsigned int* vbo, unsigned int* ebo,
    const void* vertices, int vertexCount, const unsigned int* indices, int indexCount)
{
    const int stride = 8 * (int)sizeof(float);

    glGenVertexArrays(1, vao);
    glGenBuffers(1, vbo);
    glGenBuffers(1, ebo);

    glBindVertexArray(*vao);

    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vertexCount * stride, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        (GLsizeiptr)indexCount * (int)sizeof(unsigned int), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(VERTICE_LOCATION);
    glVertexAttribPointer(VERTICE_LOCATION, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(NORMAL_LOCATION);
    glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(TEXTURE_LOCATION);
    glVertexAttribPointer(TEXTURE_LOCATION, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// Batched vertex attributes: one VBO, but the attributes are NOT interleaved --
// all positions, then all normals, then all texcoords. `normals` / `texcoords`
// may be NULL. The VAO records the block offsets, so drawing is identical.
void MgeGL_UploadMeshBatched(unsigned int* vao, unsigned int* vbo, unsigned int* ebo,
    const void* positions, const void* normals, const void* texcoords,
    int vertexCount, const unsigned int* indices, int indexCount)
{
    const GLsizeiptr posBytes = (GLsizeiptr)vertexCount * 3 * (int)sizeof(float);
    const GLsizeiptr nrmBytes = (normals != NULL) ? (GLsizeiptr)vertexCount * 3 * (int)sizeof(float) : 0;
    const GLsizeiptr uvBytes = (texcoords != NULL) ? (GLsizeiptr)vertexCount * 2 * (int)sizeof(float) : 0;

    glGenVertexArrays(1, vao);
    glGenBuffers(1, vbo);
    glGenBuffers(1, ebo);

    glBindVertexArray(*vao);

    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, posBytes + nrmBytes + uvBytes, NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, posBytes, positions);
    if (nrmBytes > 0)
        glBufferSubData(GL_ARRAY_BUFFER, posBytes, nrmBytes, normals);
    if (uvBytes > 0)
        glBufferSubData(GL_ARRAY_BUFFER, posBytes + nrmBytes, uvBytes, texcoords);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        (GLsizeiptr)indexCount * (int)sizeof(unsigned int), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(VERTICE_LOCATION);
    glVertexAttribPointer(VERTICE_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    if (nrmBytes > 0) {
        glEnableVertexAttribArray(NORMAL_LOCATION);
        glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, (void*)(size_t)posBytes);
    }
    if (uvBytes > 0) {
        glEnableVertexAttribArray(TEXTURE_LOCATION);
        glVertexAttribPointer(TEXTURE_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, (void*)(size_t)(posBytes + nrmBytes));
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void MgeGL_DrawMesh(unsigned int vao, int indexCount, unsigned int textureId)
{
    if (vao == 0 || indexCount <= 0)
        return;

    MgeGL_Draw(); // flush whatever the immediate-mode API had queued

    glUseProgram(MGEGL.State.currentShaderID);
    MgeGL_UniformMatrix4fv("modelview", MGEGL.State.modelview);
    MgeGL_UniformMatrix4fv("projection", MGEGL.State.projection);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (textureId != 0) ? textureId : MGEGL.State.whiteTexture);

    // the mesh VAO carries no colour attribute: feed a constant white so the
    // shader's `texture(sampleTex, uv) * vColor` leaves the texture untinted
    glVertexAttrib4f(COLOR_LOCATION, 1.0f, 1.0f, 1.0f, 1.0f);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    MGEGL.State.drawCalls++;
}

void MgeGL_UnloadMesh(unsigned int vao, unsigned int vbo, unsigned int ebo)
{
    if (vbo != 0)
        glDeleteBuffers(1, &vbo);
    if (ebo != 0)
        glDeleteBuffers(1, &ebo);
    if (vao != 0)
        glDeleteVertexArrays(1, &vao);
}
