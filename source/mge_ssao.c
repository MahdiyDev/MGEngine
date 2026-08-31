// Screen-space ambient occlusion -- LearnOpenGL Advanced-Lighting/SSAO.
//
// From the deferred G-buffer: for every pixel, sample a hemisphere of points
// around its surface (oriented by the normal, jittered by a tiled noise texture)
// and count how many land behind nearby geometry. That fraction darkens the
// ambient term, so creases and contact points pick up soft shadowing no light
// actually computes. A 4x4 box blur removes the noise-texture tiling.
//
//   SSAO ao = Mge_LoadSSAO(w, h);
//   ... Mge_BeginGeometryPass / draw / Mge_EndGeometryPass ...
//   Mge_ComputeSSAO(&ao, g, cam);
//   Mge_DeferredLightingAO(g, lights, n, cam, ao.aoBlur.texture.id);

#include "mge.h"
#include "mge_gl.h"

#include <glad/glad.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

static const char* fsVert =
    "#version 330 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "layout(location = 1) in vec2 aUV;\n"
    "out vec2 vUV;\n"
    "void main() { vUV = aUV; gl_Position = vec4(aPos, 0.0, 1.0); }\n";

static const char* ssaoFrag =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 vUV;\n"
    "uniform sampler2D gPosition;\n"   // world-space position
    "uniform sampler2D gNormal;\n"     // world-space normal
    "uniform sampler2D noiseTex;\n"
    "uniform vec3 samples[64];\n"
    "uniform int kernelSize;\n"
    "uniform float radius;\n"
    "uniform float bias;\n"
    "uniform float power;\n"
    "uniform vec2 noiseScale;\n"       // screen size / 4
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "    vec3 Nw = texture(gNormal, vUV).rgb;\n"
    "    if (dot(Nw, Nw) < 0.25) { FragColor = vec4(1.0); return; }\n" // background: no occlusion
    "    vec3 Pw = texture(gPosition, vUV).rgb;\n"
    "    vec3 P = (view * vec4(Pw, 1.0)).xyz;\n"                       // to view space
    "    vec3 N = normalize(mat3(view) * Nw);\n"
    "    vec3 rv = normalize(texture(noiseTex, vUV * noiseScale).xyz);\n"
    "    vec3 T = normalize(rv - N * dot(rv, N));\n"
    "    mat3 TBN = mat3(T, cross(N, T), N);\n"
    "    float occ = 0.0;\n"
    "    for (int i = 0; i < kernelSize; i++) {\n"
    "        vec3 sp = P + (TBN * samples[i]) * radius;\n"
    "        vec4 off = projection * vec4(sp, 1.0);\n"
    "        off.xyz /= off.w;\n"
    "        off.xyz = off.xyz * 0.5 + 0.5;\n"
    "        float sd = (view * vec4(texture(gPosition, off.xy).xyz, 1.0)).z;\n"
    "        float rc = smoothstep(0.0, 1.0, radius / max(abs(P.z - sd), 1e-4));\n"
    "        occ += (sd >= sp.z + bias ? 1.0 : 0.0) * rc;\n"
    "    }\n"
    "    occ = 1.0 - occ / float(kernelSize);\n"
    "    FragColor = vec4(pow(clamp(occ, 0.0, 1.0), power));\n"
    "}\n";

static const char* blurFrag =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 vUV;\n"
    "uniform sampler2D aoInput;\n"
    "uniform vec2 texelSize;\n"
    "void main()\n"
    "{\n"
    "    float r = 0.0;\n"
    "    for (int x = -2; x < 2; x++)\n"
    "        for (int y = -2; y < 2; y++)\n"
    "            r += texture(aoInput, vUV + vec2(x, y) * texelSize).r;\n"
    "    FragColor = vec4(r / 16.0);\n"
    "}\n";

static unsigned int s_ssao = 0, s_blur = 0, s_vao = 0, s_vbo = 0;

static void EnsureResources(void)
{
    if (s_vao != 0)
        return;
    unsigned int vs = MgeGL_LoadShader(fsVert, GL_VERTEX_SHADER, "ssao vertex");
    s_ssao = MgeGL_CreateShaderProgram(vs, MgeGL_LoadShader(ssaoFrag, GL_FRAGMENT_SHADER, "ssao fragment"));
    s_blur = MgeGL_CreateShaderProgram(vs, MgeGL_LoadShader(blurFrag, GL_FRAGMENT_SHADER, "ssao blur"));

    const float quad[24] = {
        -1, -1, 0, 0, 1, -1, 1, 0, 1, 1, 1, 1, -1, -1, 0, 0, 1, 1, 1, 1, -1, 1, 0, 1,
    };
    glGenVertexArrays(1, &s_vao);
    glGenBuffers(1, &s_vbo);
    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// small deterministic PRNG so the kernel/noise are stable between runs
static float frand(unsigned int* st)
{
    *st = *st * 1103515245u + 12345u;
    return (float)((*st >> 8) & 0xFFFFFFu) / (float)0xFFFFFFu;
}

SSAO Mge_LoadSSAO(int width, int height)
{
    SSAO s = { 0 };
    s.width = width;
    s.height = height;
    s.kernelSize = 32;
    s.radius = 0.5f;
    s.bias = 0.025f;
    s.power = 2.0f;

    unsigned int st = 1337u;
    for (int i = 0; i < 64; i++) {
        Vector3 v = { frand(&st) * 2.0f - 1.0f, frand(&st) * 2.0f - 1.0f, frand(&st) }; // hemisphere
        float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z) + 1e-5f;
        v.x /= len; v.y /= len; v.z /= len;
        v.x *= frand(&st); v.y *= frand(&st); v.z *= frand(&st); // random length toward the origin
        float scale = (float)i / 64.0f;
        scale = 0.1f + 0.9f * scale * scale;                     // cluster samples near the centre
        s.kernel[i].x = v.x * scale;
        s.kernel[i].y = v.y * scale;
        s.kernel[i].z = v.z * scale;
    }

    float noise[16 * 3];
    for (int i = 0; i < 16; i++) {
        noise[i * 3 + 0] = frand(&st) * 2.0f - 1.0f;
        noise[i * 3 + 1] = frand(&st) * 2.0f - 1.0f;
        noise[i * 3 + 2] = 0.0f;
    }
    glGenTextures(1, &s.noiseTex);
    glBindTexture(GL_TEXTURE_2D, s.noiseTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, noise);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    s.aoRaw = Mge_LoadRenderTexture(width, height);
    s.aoBlur = Mge_LoadRenderTexture(width, height);
    return s;
}

void Mge_UnloadSSAO(SSAO* ssao)
{
    if (ssao == NULL)
        return;
    if (ssao->noiseTex)
        glDeleteTextures(1, &ssao->noiseTex);
    Mge_UnloadRenderTexture(ssao->aoRaw);
    Mge_UnloadRenderTexture(ssao->aoBlur);
    *ssao = (SSAO){ 0 };
}

static void DrawQuad(void)
{
    glBindVertexArray(s_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    MgeGL_RegisterDrawCall();
    glBindVertexArray(0);
}

void Mge_ComputeSSAO(SSAO* s, GBuffer g, Camera3D camera)
{
    if (s == NULL || s->aoRaw.fbo == 0 || g.fbo == 0)
        return;
    EnsureResources();

    MgeGL_Draw();
    glDisable(GL_DEPTH_TEST);

    int w = Mge_GetScreenWidth(), h = Mge_GetScreenHeight();
    float aspect = (h != 0) ? (float)w / (float)h : 1.0f;
    Matrix view = Mge_GetCameraViewMatrix(camera);
    Matrix proj = Mge_GetCameraProjectionMatrix(camera, aspect);

    // --- occlusion pass -> aoRaw ---
    MgeGL_SetShader(s_ssao);
    glBindFramebuffer(GL_FRAMEBUFFER, s->aoRaw.fbo);
    MgeGL_Viewport(0, 0, s->width, s->height);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g.position.id);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g.normal.id);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, s->noiseTex);
    glActiveTexture(GL_TEXTURE0);
    MgeGL_Uniform1i("gPosition", 0);
    MgeGL_Uniform1i("gNormal", 1);
    MgeGL_Uniform1i("noiseTex", 2);
    MgeGL_UniformMatrix4fv("view", view);
    MgeGL_UniformMatrix4fv("projection", proj);
    MgeGL_Uniform1i("kernelSize", (s->kernelSize > 64) ? 64 : (s->kernelSize < 1 ? 1 : s->kernelSize));
    MgeGL_Uniform1f("radius", s->radius);
    MgeGL_Uniform1f("bias", s->bias);
    MgeGL_Uniform1f("power", (s->power > 0.0f) ? s->power : 1.0f);
    {
        Vector2 ns = { (float)s->width / 4.0f, (float)s->height / 4.0f };
        MgeGL_Uniform2fv("noiseScale", ns);
    }
    char name[16];
    int ks = (s->kernelSize > 64) ? 64 : s->kernelSize;
    for (int i = 0; i < ks; i++) {
        snprintf(name, sizeof(name), "samples[%d]", i);
        MgeGL_Uniform3fv(name, s->kernel[i]);
    }
    DrawQuad();

    // --- 4x4 box blur -> aoBlur ---
    MgeGL_SetShader(s_blur);
    glBindFramebuffer(GL_FRAMEBUFFER, s->aoBlur.fbo);
    MgeGL_Viewport(0, 0, s->width, s->height);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s->aoRaw.texture.id);
    MgeGL_Uniform1i("aoInput", 0);
    {
        Vector2 ts = { 1.0f / (float)s->width, 1.0f / (float)s->height };
        MgeGL_Uniform2fv("texelSize", ts);
    }
    DrawQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    MgeGL_Viewport(0, 0, w, h);
    glBindTexture(GL_TEXTURE_2D, 0);
    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
}
