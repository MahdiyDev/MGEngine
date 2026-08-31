// Deferred shading -- LearnOpenGL Advanced-Lighting/Deferred-Shading.
//
//   1. geometry pass  -- draw the scene once into a G-buffer (world position,
//                        world normal, albedo + specular), no lighting
//   2. lighting pass  -- one full-screen quad shades every pixel against ALL the
//                        lights, sampling the G-buffer
//
// So the lighting cost is O(pixels x lights) instead of O(fragments x lights) --
// dozens of lights stay cheap no matter how much overdraw the geometry has.
//
//   GBuffer g = Mge_LoadGBuffer(w, h);
//   Mge_BeginMode3D(cam);
//       Mge_BeginGeometryPass(&g, cam);
//           Mge_DrawObject(obj); Draw_Cube(...);      // same draw calls as forward
//       Mge_EndGeometryPass();
//   Mge_EndMode3D();
//   Mge_DeferredLighting(g, lights, count, cam);      // -> the bound framebuffer
//   // optional: Mge_BlitGBufferDepth(g); then forward-draw a skybox / lamps
//   Mge_UnloadGBuffer(&g);

#include "mge.h"
#include "mge_gl.h"
#include "mge_utils.h"

#include <glad/glad.h>
#include <stddef.h>

// geometry pass: reuse the standard attribute layout; geometry is world-space
static const char* geoVert =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 1) in vec4 aColor;\n"
    "layout(location = 2) in vec2 aTexCoord;\n"
    "layout(location = 3) in vec3 aNormal;\n"
    "out vec4 vColor;\n"
    "out vec2 vTexCoord;\n"
    "out vec3 vFragPos;\n"
    "out vec3 vNormal;\n"
    "uniform mat4 modelview;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = projection * modelview * vec4(aPos, 1.0);\n"
    "    vColor = aColor; vTexCoord = aTexCoord; vFragPos = aPos; vNormal = aNormal;\n"
    "}\n";

static const char* geoFrag =
    "#version 330 core\n"
    "layout(location = 0) out vec4 gPosition;\n"
    "layout(location = 1) out vec4 gNormal;\n"
    "layout(location = 2) out vec4 gAlbedoSpec;\n"
    "in vec4 vColor;\n"
    "in vec2 vTexCoord;\n"
    "in vec3 vFragPos;\n"
    "in vec3 vNormal;\n"
    "uniform sampler2D sampleTex;\n"
    "uniform float matDiffuse;\n"
    "uniform float matSpecular;\n"
    "void main()\n"
    "{\n"
    "    gPosition = vec4(vFragPos, 1.0);\n"
    "    gNormal = vec4(normalize(vNormal), 1.0);\n"
    "    vec3 albedo = (texture(sampleTex, vTexCoord) * vColor).rgb * matDiffuse;\n"
    "    gAlbedoSpec = vec4(albedo, matSpecular);\n"
    "}\n";

// lighting pass: full-screen, Blinn-Phong over every light (mirrors mge_light.c)
static const char* lightVert =
    "#version 330 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "layout(location = 1) in vec2 aUV;\n"
    "out vec2 vUV;\n"
    "void main() { vUV = aUV; gl_Position = vec4(aPos, 0.0, 1.0); }\n";

static const char* lightFrag =
    "#version 330 core\n"
    "#define MAX_LIGHTS 32\n"
    "out vec4 FragColor;\n"
    "in vec2 vUV;\n"
    "struct Light {\n"
    "    int type; int enabled; vec3 position; vec3 direction; vec3 color;\n"
    "    float ambient; float diffuse; float specular;\n"
    "    float constant; float linear; float quadratic;\n"
    "    float innerCutoff; float outerCutoff;\n"
    "};\n"
    "uniform Light lights[MAX_LIGHTS];\n"
    "uniform int lightCount;\n"
    "uniform vec3 viewPos;\n"
    "uniform int blinn;\n"
    "uniform float shininess;\n"
    "uniform sampler2D gPosition;\n"
    "uniform sampler2D gNormal;\n"
    "uniform sampler2D gAlbedoSpec;\n"
    "void main()\n"
    "{\n"
    "    vec3 N = texture(gNormal, vUV).rgb;\n"
    "    if (dot(N, N) < 0.25) discard;\n"           // no geometry here -> leave the clear
    "    N = normalize(N);\n"
    "    vec3 P = texture(gPosition, vUV).rgb;\n"
    "    vec4 as = texture(gAlbedoSpec, vUV);\n"
    "    vec3 albedo = as.rgb; float matSpec = as.a;\n"
    "    vec3 V = normalize(viewPos - P);\n"
    "    vec3 lit = vec3(0.0);\n"
    "    for (int i = 0; i < lightCount && i < MAX_LIGHTS; i++) {\n"
    "        if (lights[i].enabled == 0) continue;\n"
    "        vec3 L; float atten = 1.0;\n"
    "        if (lights[i].type == 0) { L = normalize(-lights[i].direction); }\n"
    "        else {\n"
    "            vec3 toL = lights[i].position - P; float d = length(toL);\n"
    "            L = toL / max(d, 1e-4);\n"
    "            atten = 1.0 / (lights[i].constant + lights[i].linear * d + lights[i].quadratic * d * d);\n"
    "        }\n"
    "        float intensity = 1.0;\n"
    "        if (lights[i].type == 2) {\n"
    "            float th = dot(L, normalize(-lights[i].direction));\n"
    "            float e = max(lights[i].innerCutoff - lights[i].outerCutoff, 1e-4);\n"
    "            intensity = clamp((th - lights[i].outerCutoff) / e, 0.0, 1.0);\n"
    "        }\n"
    "        float diff = max(dot(N, L), 0.0);\n"
    "        float sa = (blinn == 1) ? max(dot(N, normalize(L + V)), 0.0)\n"
    "                                : max(dot(V, reflect(-L, N)), 0.0);\n"
    "        float spec = (diff > 0.0) ? pow(sa, max(shininess, 1.0)) : 0.0;\n"
    "        vec3 amb = lights[i].ambient  * lights[i].color;\n"
    "        vec3 dif = lights[i].diffuse  * diff * lights[i].color;\n"
    "        vec3 spc = lights[i].specular * matSpec * spec * lights[i].color;\n"
    "        lit += amb + (dif + spc) * atten * intensity;\n"
    "    }\n"
    "    FragColor = vec4(lit * albedo, 1.0);\n"
    "}\n";

static unsigned int s_geo = 0, s_light = 0;
static unsigned int s_vao = 0, s_vbo = 0;

static void EnsureResources(void)
{
    if (s_geo != 0)
        return;
    s_geo = MgeGL_CreateShaderProgram(
        MgeGL_LoadShader(geoVert, GL_VERTEX_SHADER, "deferred geometry vertex"),
        MgeGL_LoadShader(geoFrag, GL_FRAGMENT_SHADER, "deferred geometry fragment"));
    s_light = MgeGL_CreateShaderProgram(
        MgeGL_LoadShader(lightVert, GL_VERTEX_SHADER, "deferred lighting vertex"),
        MgeGL_LoadShader(lightFrag, GL_FRAGMENT_SHADER, "deferred lighting fragment"));

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

static unsigned int MakeTarget(int w, int h, GLint internalFmt, GLenum fmt, GLenum type)
{
    unsigned int id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, w, h, 0, fmt, type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return id;
}

GBuffer Mge_LoadGBuffer(int width, int height)
{
    GBuffer g = { 0 };
    g.width = width;
    g.height = height;

    glGenFramebuffers(1, &g.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g.fbo);

    g.position.id = MakeTarget(width, height, GL_RGB16F, GL_RGB, GL_FLOAT);
    g.normal.id = MakeTarget(width, height, GL_RGB16F, GL_RGB, GL_FLOAT);
    g.albedoSpec.id = MakeTarget(width, height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g.position.id, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g.normal.id, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, g.albedoSpec.id, 0);
    const GLenum bufs[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, bufs);

    glGenRenderbuffers(1, &g.depth);
    glBindRenderbuffer(GL_RENDERBUFFER, g.depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, g.depth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        TRACE_LOG(LOG_WARNING, "GBUFFER: [%dx%d] framebuffer is not complete", width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    g.position.width = g.normal.width = g.albedoSpec.width = width;
    g.position.height = g.normal.height = g.albedoSpec.height = height;
    return g;
}

void Mge_UnloadGBuffer(GBuffer* g)
{
    if (g == NULL)
        return;
    if (g->position.id) glDeleteTextures(1, &g->position.id);
    if (g->normal.id) glDeleteTextures(1, &g->normal.id);
    if (g->albedoSpec.id) glDeleteTextures(1, &g->albedoSpec.id);
    if (g->depth) glDeleteRenderbuffers(1, &g->depth);
    if (g->fbo) glDeleteFramebuffers(1, &g->fbo);
    *g = (GBuffer){ 0 };
}

void Mge_BeginGeometryPass(GBuffer* g, Camera3D camera)
{
    (void)camera; // matrices come from Mge_BeginMode3D via the batcher
    if (g == NULL || g->fbo == 0)
        return;
    EnsureResources();

    MgeGL_Draw();
    glBindFramebuffer(GL_FRAMEBUFFER, g->fbo);
    const GLenum bufs[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, bufs);
    MgeGL_Viewport(0, 0, g->width, g->height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    MgeGL_SetShader(s_geo);
    MgeGL_SetTexture(MgeGL_GetWhiteTexture());
    MgeGL_Uniform1i("sampleTex", 0);
    MgeGL_Uniform1f("matDiffuse", 1.0f);
    MgeGL_Uniform1f("matSpecular", 0.5f);
    Mge_BeginMaterialPass(); // let Mge_SetMaterial feed the G-buffer shader
}

void Mge_EndGeometryPass(void)
{
    MgeGL_Draw();
    Mge_EndMaterialPass();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    MgeGL_Viewport(0, 0, Mge_GetScreenWidth(), Mge_GetScreenHeight());
    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
}

void Mge_DeferredLighting(GBuffer g, const Light* lights, int count, Camera3D camera)
{
    if (g.fbo == 0)
        return;
    EnsureResources();

    MgeGL_Draw();
    MgeGL_SetShader(s_light);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g.position.id);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g.normal.id);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, g.albedoSpec.id);
    glActiveTexture(GL_TEXTURE0);
    MgeGL_Uniform1i("gPosition", 0);
    MgeGL_Uniform1i("gNormal", 1);
    MgeGL_Uniform1i("gAlbedoSpec", 2);

    MgeGL_Uniform3fv("viewPos", camera.position);
    MgeGL_Uniform1i("blinn", (Mge_GetLightingModel() == LIGHTING_PHONG) ? 0 : 1);
    MgeGL_Uniform1f("shininess", 32.0f);
    if (count < 0) count = 0;
    if (count > MGE_MAX_LIGHTS_DEFERRED) count = MGE_MAX_LIGHTS_DEFERRED;
    MgeGL_Uniform1i("lightCount", count);
    for (int i = 0; i < count; i++)
        Mge_UploadLightUniforms(&lights[i], i);

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(s_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    MgeGL_RegisterDrawCall();
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
}

void Mge_BlitGBufferDepth(GBuffer g)
{
    if (g.fbo == 0)
        return;
    GLint draw = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    int w = Mge_GetScreenWidth(), h = Mge_GetScreenHeight();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, g.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)draw);
    glBlitFramebuffer(0, 0, g.width, g.height, 0, 0, w, h, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)draw);
}
