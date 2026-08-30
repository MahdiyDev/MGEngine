// Shadow mapping (directional / spot lights).
//
// Pass 1 (Mge_BeginShadowPass / Mge_EndShadowPass): render the occluders from
// the light's point of view into a depth-only framebuffer. Because the engine
// submits geometry in world space, the depth vertex shader only needs one
// matrix -- light projection * light view.
//
// Pass 2: Mge_BeginLighting3DShadowed feeds that depth texture (unit 1) and the
// light-space matrix to the lighting shader, which does a 3x3 PCF compare.

#include "mge.h"
#include "mge_gl.h"
#include "mge_math.h"
#include "mge_utils.h"

#include <glad/glad.h>
#include <math.h>
#include <stddef.h>

// ---- depth-only program (pass 1) ----

static const char* depthVert =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "uniform mat4 lightSpaceMatrix;\n"
    "void main() { gl_Position = lightSpaceMatrix * vec4(aPos, 1.0); }\n";

static const char* depthFrag =
    "#version 330 core\n"
    "void main() { }\n";

// ---- debug blit (Mge_DrawShadowMap) ----

static const char* blitVert =
    "#version 330 core\n"
    "layout(location = 0) in vec2 aCorner;\n" // unit quad 0..1
    "uniform vec4 ndc;\n"                     // (x0, y0, x1, y1) in clip space
    "out vec2 vUV;\n"
    "void main() {\n"
    "    vUV = aCorner;\n"
    "    gl_Position = vec4(mix(ndc.xy, ndc.zw, aCorner), 0.0, 1.0);\n"
    "}\n";

static const char* blitFrag =
    "#version 330 core\n"
    "in vec2 vUV;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D depthTex;\n"
    "void main() { float d = texture(depthTex, vUV).r; FragColor = vec4(vec3(d), 1.0); }\n";

static unsigned int s_depthProgram = 0;
// ---- point-light depth: store linear distance from the light (pass 1) ----

static const char* ptVert =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "uniform mat4 lightVP;\n" // face projection * face view
    "out vec3 vWorld;\n"
    "void main() { vWorld = aPos; gl_Position = lightVP * vec4(aPos, 1.0); }\n";

static const char* ptFrag =
    "#version 330 core\n"
    "in vec3 vWorld;\n"
    "uniform vec3 lightPos;\n"
    "uniform float farPlane;\n"
    "void main() { gl_FragDepth = length(vWorld - lightPos) / farPlane; }\n";

static unsigned int s_blitProgram = 0;
static unsigned int s_ptProgram = 0;
static unsigned int s_quadVao = 0;
static unsigned int s_quadVbo = 0;

static void EnsureResources(void)
{
    if (s_depthProgram != 0)
        return;

    s_depthProgram = MgeGL_CreateShaderProgram(
        MgeGL_LoadShader(depthVert, GL_VERTEX_SHADER, "shadow depth vertex"),
        MgeGL_LoadShader(depthFrag, GL_FRAGMENT_SHADER, "shadow depth fragment"));
    s_blitProgram = MgeGL_CreateShaderProgram(
        MgeGL_LoadShader(blitVert, GL_VERTEX_SHADER, "shadow blit vertex"),
        MgeGL_LoadShader(blitFrag, GL_FRAGMENT_SHADER, "shadow blit fragment"));
    s_ptProgram = MgeGL_CreateShaderProgram(
        MgeGL_LoadShader(ptVert, GL_VERTEX_SHADER, "point shadow vertex"),
        MgeGL_LoadShader(ptFrag, GL_FRAGMENT_SHADER, "point shadow fragment"));

    const float quad[12] = { 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1 };
    glGenVertexArrays(1, &s_quadVao);
    glGenBuffers(1, &s_quadVbo);
    glBindVertexArray(s_quadVao);
    glBindBuffer(GL_ARRAY_BUFFER, s_quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ---- resource lifetime ----

ShadowMap Mge_LoadShadowMap(int size)
{
    if (size < 64)
        size = 64;

    ShadowMap sm = { 0 };
    sm.size = size;
    sm.lightSpaceMatrix = Matrix_Identity();

    glGenTextures(1, &sm.depthTexture);
    glBindTexture(GL_TEXTURE_2D, sm.depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size, size, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // outside the map = fully lit
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    glGenFramebuffers(1, &sm.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, sm.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sm.depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        TRACE_LOG(LOG_WARNING, "SHADOW: depth framebuffer [%dx%d] is not complete", size, size);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    return sm;
}

void Mge_UnloadShadowMap(ShadowMap* sm)
{
    if (sm == NULL)
        return;
    if (sm->depthTexture != 0)
        glDeleteTextures(1, &sm->depthTexture);
    if (sm->fbo != 0)
        glDeleteFramebuffers(1, &sm->fbo);
    sm->depthTexture = 0;
    sm->fbo = 0;
    sm->size = 0;
}

// ---- pass 1 ----

void Mge_BeginShadowPass(ShadowMap* sm, Light light, Vector3 center, float radius)
{
    if (sm == NULL || sm->fbo == 0)
        return;
    EnsureResources();
    MgeGL_Draw(); // flush anything queued for the window

    if (radius < 0.01f)
        radius = 1.0f;

    Vector3 dir = (light.type == LIGHT_DIRECTIONAL)
        ? Vector3Normalize(light.direction)
        : Vector3Normalize(Vector3_Subtract(center, light.position));
    Vector3 eye = (light.type == LIGHT_DIRECTIONAL)
        ? Vector3_Subtract(center, Vector3_Scale(dir, radius * 2.5f))
        : light.position;
    Vector3 up = (fabsf(dir.y) > 0.99f) ? (Vector3){ 0.0f, 0.0f, 1.0f } : (Vector3){ 0.0f, 1.0f, 0.0f };

    Matrix view = MatrixLookAt(eye, center, up);
    Matrix proj;
    if (light.type == LIGHT_DIRECTIONAL) {
        proj = MatrixOrtho(-radius, radius, -radius, radius, 0.05, radius * 5.0);
    } else {
        double d = (double)Vector3_Length(Vector3_Subtract(light.position, center));
        proj = MatrixPerspective(100.0 * DEG2RAD, 1.0, 0.1, d + radius * 2.0);
    }
    sm->lightSpaceMatrix = Matrix_Multiply(view, proj); // engine order: apply view, then proj

    glBindFramebuffer(GL_FRAMEBUFFER, sm->fbo);
    MgeGL_Viewport(0, 0, sm->size, sm->size);
    MgeGL_EnableDepthTest();
    MgeGL_SetDepthFunc(DEPTH_LESS);
    MgeGL_SetDepthMask(true);
    glClear(GL_DEPTH_BUFFER_BIT);

    MgeGL_SetShader(s_depthProgram);
    MgeGL_UniformMatrix4fv("lightSpaceMatrix", sm->lightSpaceMatrix);
}

void Mge_EndShadowPass(void)
{
    MgeGL_Draw();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    MgeGL_Viewport(0, 0, Mge_GetScreenWidth(), Mge_GetScreenHeight());
    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
}

// ---- pass 2 ----

void Mge_BeginLighting3DShadowed(const Light* lights, int count, Camera3D camera, ShadowMap sm)
{
    Mge_BeginLighting3DEx(lights, count, camera); // normal setup; lighting shader now active

    MgeGL_Uniform1i("shadowsEnabled", 1);
    MgeGL_Uniform1i("shadowMap", 1);
    MgeGL_UniformMatrix4fv("lightSpaceMatrix", sm.lightSpaceMatrix);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sm.depthTexture);
    glActiveTexture(GL_TEXTURE0);
}

// ---- debug ----

void Mge_DrawShadowMap(ShadowMap sm, int x, int y, int size)
{
    if (sm.depthTexture == 0)
        return;
    EnsureResources();
    MgeGL_Draw();

    float sw = (float)Mge_GetScreenWidth();
    float sh = (float)Mge_GetScreenHeight();
    if (sw <= 0.0f || sh <= 0.0f)
        return;

    Vector4 ndc = {
        (float)x / sw * 2.0f - 1.0f,
        1.0f - (float)(y + size) / sh * 2.0f,
        (float)(x + size) / sw * 2.0f - 1.0f,
        1.0f - (float)y / sh * 2.0f,
    };

    MgeGL_SetShader(s_blitProgram);
    MgeGL_Uniform4fv("ndc", ndc);
    MgeGL_Uniform1i("depthTex", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sm.depthTexture);
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(s_quadVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    MgeGL_RegisterDrawCall();
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    MgeGL_EnableDepthTest();
    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
}

// ==========================================================================
// Point (omnidirectional) shadows -- a depth cubemap, one render per face.
// ==========================================================================

static Matrix s_ptProj;       // 90deg perspective for the current pass
static Vector3 s_ptLightPos;  // light position, remembered across the 6 faces
static unsigned int s_ptCube; // the cubemap being written this pass

PointShadowMap Mge_LoadPointShadowMap(int size)
{
    if (size < 64)
        size = 64;

    PointShadowMap sm = { 0 };
    sm.size = size;

    glGenTextures(1, &sm.depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sm.depthCubemap);
    for (int f = 0; f < 6; f++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, 0, GL_DEPTH_COMPONENT24,
            size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &sm.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, sm.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, sm.depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        TRACE_LOG(LOG_WARNING, "SHADOW: point-shadow cubemap [%d] is not complete", size);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return sm;
}

void Mge_UnloadPointShadowMap(PointShadowMap* sm)
{
    if (sm == NULL)
        return;
    if (sm->depthCubemap != 0)
        glDeleteTextures(1, &sm->depthCubemap);
    if (sm->fbo != 0)
        glDeleteFramebuffers(1, &sm->fbo);
    sm->depthCubemap = 0;
    sm->fbo = 0;
    sm->size = 0;
}

void Mge_BeginPointShadowPass(PointShadowMap* sm, Light light, float farPlane)
{
    if (sm == NULL || sm->fbo == 0)
        return;
    EnsureResources();
    MgeGL_Draw();

    if (farPlane < 0.1f)
        farPlane = 25.0f;
    sm->lightPos = light.position;
    sm->farPlane = farPlane;
    s_ptProj = MatrixPerspective(90.0 * DEG2RAD, 1.0, 0.05, (double)farPlane);

    glBindFramebuffer(GL_FRAMEBUFFER, sm->fbo);
    MgeGL_Viewport(0, 0, sm->size, sm->size);
    MgeGL_EnableDepthTest();
    MgeGL_SetDepthFunc(DEPTH_LESS);
    MgeGL_SetDepthMask(true);

    MgeGL_SetShader(s_ptProgram);
    MgeGL_Uniform3fv("lightPos", light.position);
    MgeGL_Uniform1f("farPlane", farPlane);

    // remembered for Mge_SetPointShadowFace
    s_ptLightPos = light.position;
    s_ptCube = sm->depthCubemap;
}

void Mge_SetPointShadowFace(int face)
{
    if (face < 0 || face > 5 || s_ptCube == 0)
        return;
    MgeGL_Draw(); // flush the previous face's geometry

    static const Vector3 fwd[6] = {
        { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }
    };
    static const Vector3 up[6] = {
        { 0, -1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }, { 0, -1, 0 }, { 0, -1, 0 }
    };

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, s_ptCube, 0);
    glClear(GL_DEPTH_BUFFER_BIT);

    Matrix view = MatrixLookAt(s_ptLightPos, Vector3_Add(s_ptLightPos, fwd[face]), up[face]);
    MgeGL_UniformMatrix4fv("lightVP", Matrix_Multiply(view, s_ptProj));
}

void Mge_EndPointShadowPass(void)
{
    MgeGL_Draw();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    MgeGL_Viewport(0, 0, Mge_GetScreenWidth(), Mge_GetScreenHeight());
    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
    s_ptCube = 0;
}

void Mge_BeginLighting3DPointShadowed(const Light* lights, int count, Camera3D camera, PointShadowMap sm)
{
    Mge_BeginLighting3DEx(lights, count, camera);

    MgeGL_Uniform1i("pointShadowEnabled", 1);
    MgeGL_Uniform1i("pointShadowMap", 2);
    MgeGL_Uniform3fv("pointShadowLightPos", sm.lightPos);
    MgeGL_Uniform1f("pointShadowFar", sm.farPlane);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sm.depthCubemap);
    glActiveTexture(GL_TEXTURE0);
}
