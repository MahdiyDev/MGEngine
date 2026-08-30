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
static unsigned int s_blitProgram = 0;
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
