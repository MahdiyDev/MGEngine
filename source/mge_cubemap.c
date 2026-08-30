// Cube maps: skybox, static environment mapping (reflection / refraction), and
// dynamic environment probes (render the scene into a cubemap, reflect it).

#include "mge.h"
#include "mge_gl.h"
#include "mge_math.h"
#include "mge_utils.h"

#include <glad/glad.h>
#include <stdio.h>
#include <string.h>

// ---- a unit cube, positions only (used by the skybox) ----

static const float s_cubeVerts[108] = {
    -1,  1, -1,  -1, -1, -1,   1, -1, -1,   1, -1, -1,   1,  1, -1,  -1,  1, -1,
    -1, -1,  1,  -1, -1, -1,  -1,  1, -1,  -1,  1, -1,  -1,  1,  1,  -1, -1,  1,
     1, -1, -1,   1, -1,  1,   1,  1,  1,   1,  1,  1,   1,  1, -1,   1, -1, -1,
    -1, -1,  1,  -1,  1,  1,   1,  1,  1,   1,  1,  1,   1, -1,  1,  -1, -1,  1,
    -1,  1, -1,   1,  1, -1,   1,  1,  1,   1,  1,  1,  -1,  1,  1,  -1,  1, -1,
    -1, -1, -1,  -1, -1,  1,   1, -1, -1,   1, -1, -1,  -1, -1,  1,   1, -1,  1,
};

static unsigned int s_cubeVao = 0, s_cubeVbo = 0;
static unsigned int s_skyProgram = 0;
static unsigned int s_envProgram = 0;

static const Vector3 s_faceDir[6] = {
    { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }
};
static const Vector3 s_faceUp[6] = {
    { 0, -1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }, { 0, -1, 0 }, { 0, -1, 0 }
};

// ---- shaders ----

static const char* skyVert =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "out vec3 vDir;\n"
    "uniform mat4 view;\n"        // rotation only
    "uniform mat4 projection;\n"
    "void main() { vDir = aPos; vec4 p = projection * view * vec4(aPos, 1.0); gl_Position = p.xyww; }\n";

static const char* skyFrag =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 vDir;\n"
    "uniform samplerCube skybox;\n"
    "void main() { FragColor = texture(skybox, vDir); }\n";

static const char* envVert =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 3) in vec3 aNormal;\n"
    "out vec3 vPos;\n"
    "out vec3 vN;\n"
    "uniform mat4 modelview;\n"
    "uniform mat4 projection;\n"
    "void main() { vPos = aPos; vN = aNormal; gl_Position = projection * modelview * vec4(aPos, 1.0); }\n";

static const char* envFrag =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 vPos;\n"
    "in vec3 vN;\n"
    "uniform vec3 viewPos;\n"
    "uniform samplerCube envMap;\n"
    "uniform int mode;\n"          // 0 reflect, 1 refract
    "uniform float ratio;\n"
    "void main()\n"
    "{\n"
    "    vec3 I = normalize(vPos - viewPos);\n"
    "    vec3 N = normalize(vN);\n"
    "    vec3 d = (mode == 0) ? reflect(I, N) : refract(I, N, ratio);\n"
    "    FragColor = vec4(texture(envMap, d).rgb, 1.0);\n"
    "}\n";

static void EnsureResources(void)
{
    if (s_cubeVao != 0)
        return;

    glGenVertexArrays(1, &s_cubeVao);
    glGenBuffers(1, &s_cubeVbo);
    glBindVertexArray(s_cubeVao);
    glBindBuffer(GL_ARRAY_BUFFER, s_cubeVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_cubeVerts), s_cubeVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    s_skyProgram = MgeGL_CreateShaderProgram(
        MgeGL_LoadShader(skyVert, GL_VERTEX_SHADER, "skybox vertex"),
        MgeGL_LoadShader(skyFrag, GL_FRAGMENT_SHADER, "skybox fragment"));
    s_envProgram = MgeGL_CreateShaderProgram(
        MgeGL_LoadShader(envVert, GL_VERTEX_SHADER, "envmap vertex"),
        MgeGL_LoadShader(envFrag, GL_FRAGMENT_SHADER, "envmap fragment"));
}

static GLenum GlFormat(int pixelFormat)
{
    switch (pixelFormat) {
    case PIXELFORMAT_UNCOMPRESSED_GRAYSCALE:  return GL_RED;
    case PIXELFORMAT_UNCOMPRESSED_R8G8B8A8:   return GL_RGBA;
    default:                                  return GL_RGB;
    }
}

// ---- cubemap loading ----

Cubemap Mge_LoadCubemap(const char* facePaths[6])
{
    Cubemap cm = { 0 };

    glGenTextures(1, &cm.id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cm.id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (int i = 0; i < 6; i++) {
        Image img = Mge_LoadImage(facePaths[i]);
        if (img.data == NULL) {
            TRACE_LOG(LOG_WARNING, "CUBEMAP: face %d [%s] failed to load", i, facePaths[i]);
            continue;
        }
        GLenum fmt = GlFormat(img.format);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, (GLint)fmt,
            img.width, img.height, 0, fmt, GL_UNSIGNED_BYTE, img.data);
        cm.size = img.width;
        Mge_UnloadImage(img);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return cm;
}

Cubemap Mge_LoadCubemapDir(const char* dir)
{
    static const char* names[6] = { "right", "left", "top", "bottom", "front", "back" };
    char paths[6][512];
    const char* ptrs[6];
    for (int i = 0; i < 6; i++) {
        snprintf(paths[i], sizeof(paths[i]), "%s/%s.jpg", dir, names[i]);
        ptrs[i] = paths[i];
    }
    return Mge_LoadCubemap(ptrs);
}

void Mge_UnloadCubemap(Cubemap cubemap)
{
    if (cubemap.id != 0)
        glDeleteTextures(1, &cubemap.id);
}

// ---- skybox ----

static float ViewportAspect(void)
{
    GLint vp[4] = { 0, 0, 1, 1 };
    glGetIntegerv(GL_VIEWPORT, vp);
    return (vp[3] != 0) ? (float)vp[2] / (float)vp[3] : 1.0f;
}

void Mge_DrawSkybox(Cubemap cubemap, Camera3D camera)
{
    MgeGL_Draw();
    EnsureResources();

    Matrix view = MatrixLookAt(camera.position,
        Vector3_Add(camera.position, camera.target), camera.up);
    view.m12 = view.m13 = view.m14 = 0.0f; // rotation only -- the sky doesn't translate
    Matrix proj = MatrixPerspective((double)camera.fovy * DEG2RAD, (double)ViewportAspect(),
        Mge_GetClipNear(), Mge_GetClipFar());

    MgeGL_SetShader(s_skyProgram);
    MgeGL_UniformMatrix4fv("view", view);
    MgeGL_UniformMatrix4fv("projection", proj);
    MgeGL_Uniform1i("skybox", 0);

    MgeGL_SetDepthFunc(DEPTH_LEQUAL); // depth is forced to 1.0; pass against a cleared buffer
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.id);
    glBindVertexArray(s_cubeVao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    MgeGL_RegisterDrawCall();
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    MgeGL_SetDepthFunc(DEPTH_LESS);

    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
}

// ---- environment mapping ----

void Mge_BeginEnvironmentMap(Cubemap cubemap, Camera3D camera, int mode, float refractRatio)
{
    EnsureResources();
    MgeGL_SetShader(s_envProgram);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.id);
    glActiveTexture(GL_TEXTURE0);

    MgeGL_Uniform1i("envMap", 1);
    MgeGL_Uniform3fv("viewPos", camera.position);
    MgeGL_Uniform1i("mode", mode);
    MgeGL_Uniform1f("ratio", (refractRatio > 0.0f) ? refractRatio : (1.0f / 1.52f));
}

void Mge_EndEnvironmentMap(void)
{
    MgeGL_Draw();
    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glActiveTexture(GL_TEXTURE0);
}

// ---- dynamic environment probes ----

EnvProbe Mge_LoadEnvProbe(int size)
{
    EnvProbe p = { 0 };
    p.size = size;
    p.cubemap.size = size;

    glGenFramebuffers(1, &p.fbo);

    glGenTextures(1, &p.cubemap.id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, p.cubemap.id);
    for (int i = 0; i < 6; i++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB8, size, size, 0,
            GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    glGenRenderbuffers(1, &p.depth);
    glBindRenderbuffer(GL_RENDERBUFFER, p.depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    return p;
}

void Mge_UnloadEnvProbe(EnvProbe probe)
{
    if (probe.depth != 0)
        glDeleteRenderbuffers(1, &probe.depth);
    if (probe.cubemap.id != 0)
        glDeleteTextures(1, &probe.cubemap.id);
    if (probe.fbo != 0)
        glDeleteFramebuffers(1, &probe.fbo);
}

void Mge_BeginEnvProbeFace(EnvProbe probe, Vector3 position, int face)
{
    if (face < 0 || face > 5)
        return;

    MgeGL_Draw();
    glBindFramebuffer(GL_FRAMEBUFFER, probe.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, probe.cubemap.id, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, probe.depth);

    MgeGL_Viewport(0, 0, probe.size, probe.size);

    Matrix proj = MatrixPerspective(90.0 * DEG2RAD, 1.0, Mge_GetClipNear(), Mge_GetClipFar());
    Matrix view = MatrixLookAt(position, Vector3_Add(position, s_faceDir[face]), s_faceUp[face]);

    MgeGL_MatrixMode(MGEGL_PROJECTION);
    MgeGL_PushMatrix();
    MgeGL_LoadIdentity();
    MgeGL_MultMatrixf(MatrixToFloat(proj));

    MgeGL_MatrixMode(MGEGL_MODELVIEW);
    MgeGL_LoadIdentity();
    MgeGL_MultMatrixf(MatrixToFloat(view));

    MgeGL_EnableDepthTest();
}

Camera3D Mge_GetEnvProbeCamera(Vector3 position, int face)
{
    Camera3D c = { 0 };
    c.position = position;
    c.fovy = 90.0f;
    c.projection = CAMERA_PERSPECTIVE;
    if (face >= 0 && face <= 5) {
        c.target = s_faceDir[face];
        c.up = s_faceUp[face];
    } else {
        c.target = (Vector3){ 0, 0, -1 };
        c.up = (Vector3){ 0, 1, 0 };
    }
    return c;
}

void Mge_EndEnvProbeFace(void)
{
    MgeGL_Draw();

    MgeGL_MatrixMode(MGEGL_PROJECTION);
    MgeGL_PopMatrix();
    MgeGL_MatrixMode(MGEGL_MODELVIEW);
    MgeGL_LoadIdentity();
    MgeGL_DisableDepthTest();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    MgeGL_Viewport(0, 0, Mge_GetScreenWidth(), Mge_GetScreenHeight());
}
