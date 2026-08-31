// Image-based lighting precompute -- LearnOpenGL PBR/IBL.
//
// Mge_LoadEnvironment(path):
//   1. load an equirectangular .hdr -> float texture
//   2. render it into a cubemap                        (equirect -> cube)
//   3. convolve that into a 32x32 irradiance cubemap   (diffuse IBL)
//   4. prefilter it into a mip-chained cubemap         (specular IBL, by roughness)
//   5. bake the 512x512 BRDF integration LUT           (split-sum second term)
//
// All one-time, at load, into GL_RGB16F targets.

#include "mge.h"
#include "mge_gl.h"
#include "mge_math.h"

#include <glad/glad.h>
#include <stddef.h>

// ---- geometry: a unit cube + a full-screen quad ----

static const float s_cube[108] = {
    -1,-1,-1,  1, 1,-1,  1,-1,-1,   1, 1,-1, -1,-1,-1, -1, 1,-1,
    -1,-1, 1,  1,-1, 1,  1, 1, 1,   1, 1, 1, -1, 1, 1, -1,-1, 1,
    -1, 1, 1, -1, 1,-1, -1,-1,-1,  -1,-1,-1, -1,-1, 1, -1, 1, 1,
     1, 1, 1,  1,-1,-1,  1, 1,-1,   1,-1,-1,  1, 1, 1,  1,-1, 1,
    -1,-1,-1,  1,-1, 1,  1,-1,-1,   1,-1, 1, -1,-1,-1, -1,-1, 1,
    -1, 1,-1,  1, 1,-1,  1, 1, 1,   1, 1, 1, -1, 1, 1, -1, 1,-1,
};
static const float s_quad[24] = {
    -1,-1, 0,0,  1,-1, 1,0,  1, 1, 1,1,  -1,-1, 0,0,  1, 1, 1,1,  -1, 1, 0,1,
};
static unsigned int s_cubeVao = 0, s_cubeVbo = 0, s_quadVao = 0, s_quadVbo = 0;

static void EnsureGeometry(void)
{
    if (s_cubeVao != 0)
        return;
    glGenVertexArrays(1, &s_cubeVao);
    glGenBuffers(1, &s_cubeVbo);
    glBindVertexArray(s_cubeVao);
    glBindBuffer(GL_ARRAY_BUFFER, s_cubeVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_cube), s_cube, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glGenVertexArrays(1, &s_quadVao);
    glGenBuffers(1, &s_quadVbo);
    glBindVertexArray(s_quadVao);
    glBindBuffer(GL_ARRAY_BUFFER, s_quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_quad), s_quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ---- shaders ----

static const char* cubeVert =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "out vec3 vDir;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main() { vDir = aPos; gl_Position = projection * view * vec4(aPos, 1.0); }\n";

static const char* equirectFrag =
    "#version 330 core\n"
    "out vec4 FragColor; in vec3 vDir;\n"
    "uniform sampler2D equirect;\n"
    "const vec2 invAtan = vec2(0.1591, 0.3183);\n"
    "void main()\n"
    "{\n"
    "    vec3 v = normalize(vDir);\n"
    // -asin(v.y): stb loads the .hdr top-row-first, so flip V here (equivalent to
    // LearnOpenGL's stbi_set_flip_vertically_on_load) or the sky is upside down.
    "    vec2 uv = vec2(atan(v.z, v.x), -asin(v.y)) * invAtan + 0.5;\n"
    "    FragColor = vec4(texture(equirect, uv).rgb, 1.0);\n"
    "}\n";

static const char* irradianceFrag =
    "#version 330 core\n"
    "out vec4 FragColor; in vec3 vDir;\n"
    "uniform samplerCube envMap;\n"
    "const float PI = 3.14159265359;\n"
    "void main()\n"
    "{\n"
    "    vec3 N = normalize(vDir);\n"
    "    vec3 up = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);\n"
    "    vec3 right = normalize(cross(up, N));\n"
    "    up = normalize(cross(N, right));\n"
    "    vec3 irr = vec3(0.0); float n = 0.0;\n"
    "    for (float phi = 0.0; phi < 2.0 * PI; phi += 0.025)\n"
    "        for (float theta = 0.0; theta < 0.5 * PI; theta += 0.025) {\n"
    "            vec3 t = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));\n"
    "            vec3 s = t.x * right + t.y * up + t.z * N;\n"
    "            irr += texture(envMap, s).rgb * cos(theta) * sin(theta);\n"
    "            n += 1.0;\n"
    "        }\n"
    "    FragColor = vec4(PI * irr / n, 1.0);\n"
    "}\n";

// Hammersley + GGX importance sampling -- standard split-sum helpers
static const char* iblHelpers =
    "const float PI = 3.14159265359;\n"
    "float RadicalInverse_VdC(uint bits) {\n"
    "    bits = (bits << 16u) | (bits >> 16u);\n"
    "    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);\n"
    "    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);\n"
    "    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);\n"
    "    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);\n"
    "    return float(bits) * 2.3283064365386963e-10;\n"
    "}\n"
    "vec2 Hammersley(uint i, uint N) { return vec2(float(i) / float(N), RadicalInverse_VdC(i)); }\n"
    "vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float rough) {\n"
    "    float a = rough * rough;\n"
    "    float phi = 2.0 * PI * Xi.x;\n"
    "    float ct = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));\n"
    "    float st = sqrt(1.0 - ct * ct);\n"
    "    vec3 H = vec3(cos(phi) * st, sin(phi) * st, ct);\n"
    "    vec3 up = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);\n"
    "    vec3 tx = normalize(cross(up, N));\n"
    "    vec3 ty = cross(N, tx);\n"
    "    return normalize(tx * H.x + ty * H.y + N * H.z);\n"
    "}\n";

static char s_prefilterSrc[4096];
static const char* prefilterFragBody =
    "out vec4 FragColor; in vec3 vDir;\n"
    "uniform samplerCube envMap;\n"
    "uniform float roughness;\n"
    "void main()\n"
    "{\n"
    "    vec3 N = normalize(vDir); vec3 V = N;\n"
    "    const uint SAMPLES = 1024u;\n"
    "    vec3 col = vec3(0.0); float w = 0.0;\n"
    "    for (uint i = 0u; i < SAMPLES; ++i) {\n"
    "        vec2 Xi = Hammersley(i, SAMPLES);\n"
    "        vec3 H = ImportanceSampleGGX(Xi, N, roughness);\n"
    "        vec3 L = normalize(2.0 * dot(V, H) * H - V);\n"
    "        float ndl = max(dot(N, L), 0.0);\n"
    "        if (ndl > 0.0) { col += texture(envMap, L).rgb * ndl; w += ndl; }\n"
    "    }\n"
    "    FragColor = vec4(col / max(w, 1e-4), 1.0);\n"
    "}\n";

static const char* brdfVert =
    "#version 330 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "layout(location = 1) in vec2 aUV;\n"
    "out vec2 vUV;\n"
    "void main() { vUV = aUV; gl_Position = vec4(aPos, 0.0, 1.0); }\n";

static char s_brdfSrc[4096];
static const char* brdfFragBody =
    "out vec4 FragColor; in vec2 vUV;\n"
    "float GeometrySchlickGGX(float ndv, float rough) { float k = rough * rough / 2.0; return ndv / (ndv * (1.0 - k) + k); }\n"
    "float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough) {\n"
    "    return GeometrySchlickGGX(max(dot(N, V), 0.0), rough) * GeometrySchlickGGX(max(dot(N, L), 0.0), rough);\n"
    "}\n"
    "vec2 IntegrateBRDF(float ndv, float rough) {\n"
    "    vec3 V = vec3(sqrt(1.0 - ndv * ndv), 0.0, ndv);\n"
    "    vec3 N = vec3(0.0, 0.0, 1.0);\n"
    "    float A = 0.0, B = 0.0;\n"
    "    const uint SAMPLES = 1024u;\n"
    "    for (uint i = 0u; i < SAMPLES; ++i) {\n"
    "        vec2 Xi = Hammersley(i, SAMPLES);\n"
    "        vec3 H = ImportanceSampleGGX(Xi, N, rough);\n"
    "        vec3 L = normalize(2.0 * dot(V, H) * H - V);\n"
    "        float ndl = max(L.z, 0.0), ndh = max(H.z, 0.0), vdh = max(dot(V, H), 0.0);\n"
    "        if (ndl > 0.0) {\n"
    "            float G = GeometrySmith(N, V, L, rough);\n"
    "            float gv = (G * vdh) / (ndh * ndv);\n"
    "            float fc = pow(1.0 - vdh, 5.0);\n"
    "            A += (1.0 - fc) * gv; B += fc * gv;\n"
    "        }\n"
    "    }\n"
    "    return vec2(A, B) / float(SAMPLES);\n"
    "}\n"
    "void main() { FragColor = vec4(IntegrateBRDF(vUV.x, vUV.y), 0.0, 1.0); }\n";

// skybox uses the same cube but forces depth to 1.0 (draw behind everything)
static const char* skyVert =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "out vec3 vDir;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main() { vDir = aPos; vec4 p = projection * view * vec4(aPos, 1.0); gl_Position = p.xyww; }\n";

static const char* skyFrag =
    "#version 330 core\n"
    "out vec4 FragColor; in vec3 vDir;\n"
    "uniform samplerCube envMap;\n"
    "void main() { FragColor = vec4(texture(envMap, vDir).rgb, 1.0); }\n"; // linear HDR

static unsigned int s_equirect = 0, s_irradiance = 0, s_prefilter = 0, s_brdf = 0, s_sky = 0;

static unsigned int cat3(const char* a, const char* b, const char* c, char* out, size_t n)
{
    size_t i = 0;
    for (const char* p = a; *p && i < n - 1; p++) out[i++] = *p;
    for (const char* p = b; *p && i < n - 1; p++) out[i++] = *p;
    for (const char* p = c; *p && i < n - 1; p++) out[i++] = *p;
    out[i] = '\0';
    return (unsigned int)i;
}

static void EnsureShaders(void)
{
    if (s_equirect != 0)
        return;
    EnsureGeometry();
    unsigned int cv = MgeGL_LoadShader(cubeVert, GL_VERTEX_SHADER, "ibl cube vertex");
    s_equirect = MgeGL_CreateShaderProgram(cv, MgeGL_LoadShader(equirectFrag, GL_FRAGMENT_SHADER, "ibl equirect"));
    s_irradiance = MgeGL_CreateShaderProgram(cv, MgeGL_LoadShader(irradianceFrag, GL_FRAGMENT_SHADER, "ibl irradiance"));
    s_sky = MgeGL_CreateShaderProgram(MgeGL_LoadShader(skyVert, GL_VERTEX_SHADER, "ibl skybox vertex"),
        MgeGL_LoadShader(skyFrag, GL_FRAGMENT_SHADER, "ibl skybox"));

    cat3("#version 330 core\n", iblHelpers, prefilterFragBody, s_prefilterSrc, sizeof(s_prefilterSrc));
    s_prefilter = MgeGL_CreateShaderProgram(cv, MgeGL_LoadShader(s_prefilterSrc, GL_FRAGMENT_SHADER, "ibl prefilter"));

    cat3("#version 330 core\n", iblHelpers, brdfFragBody, s_brdfSrc, sizeof(s_brdfSrc));
    unsigned int bv = MgeGL_LoadShader(brdfVert, GL_VERTEX_SHADER, "ibl brdf vertex");
    s_brdf = MgeGL_CreateShaderProgram(bv, MgeGL_LoadShader(s_brdfSrc, GL_FRAGMENT_SHADER, "ibl brdf"));
}

// six 90-degree views from the origin, one per cube face
static Matrix faceView(int f)
{
    static const Vector3 t[6] = { { 1,0,0 }, { -1,0,0 }, { 0,1,0 }, { 0,-1,0 }, { 0,0,1 }, { 0,0,-1 } };
    static const Vector3 u[6] = { { 0,-1,0 }, { 0,-1,0 }, { 0,0,1 }, { 0,0,-1 }, { 0,-1,0 }, { 0,-1,0 } };
    return MatrixLookAt((Vector3){ 0, 0, 0 }, t[f], u[f]);
}

static unsigned int NewCube(int size, int mips)
{
    unsigned int id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    for (int f = 0; f < 6; f++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, mips > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (mips > 1)
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return id;
}

static void RenderCube(void)
{
    glBindVertexArray(s_cubeVao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    MgeGL_RegisterDrawCall();
    glBindVertexArray(0);
}

Environment Mge_LoadEnvironment(const char* hdrPath)
{
    Environment env = { 0 };
    env.prefilterMips = 5;

    Texture2D hdr = Mge_LoadTextureHDR(hdrPath);
    if (hdr.id == 0)
        return env;

    EnsureShaders();
    MgeGL_Draw();

    // save GL state we clobber
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    unsigned int fbo = 0, rbo = 0;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);

    Matrix proj = MatrixPerspective(90.0 * DEG2RAD, 1.0, 0.1, 10.0);

    // 1. equirect -> cubemap (512)
    env.cubemap = NewCube(512, 1);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
    MgeGL_SetShader(s_equirect);
    MgeGL_UniformMatrix4fv("projection", proj);
    MgeGL_Uniform1i("equirect", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdr.id);
    glViewport(0, 0, 512, 512);
    for (int f = 0; f < 6; f++) {
        MgeGL_UniformMatrix4fv("view", faceView(f));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, env.cubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderCube();
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, env.cubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP); // for the prefilter's trilinear sampling

    // 2. irradiance (32)
    env.irradiance = NewCube(32, 1);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
    MgeGL_SetShader(s_irradiance);
    MgeGL_UniformMatrix4fv("projection", proj);
    MgeGL_Uniform1i("envMap", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env.cubemap);
    glViewport(0, 0, 32, 32);
    for (int f = 0; f < 6; f++) {
        MgeGL_UniformMatrix4fv("view", faceView(f));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, env.irradiance, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderCube();
    }

    // 3. prefilter (128 base, 5 mips by roughness)
    env.prefilter = NewCube(128, env.prefilterMips);
    MgeGL_SetShader(s_prefilter);
    MgeGL_UniformMatrix4fv("projection", proj);
    MgeGL_Uniform1i("envMap", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env.cubemap);
    for (int mip = 0; mip < env.prefilterMips; mip++) {
        int msize = 128 >> mip;
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, msize, msize);
        glViewport(0, 0, msize, msize);
        MgeGL_Uniform1f("roughness", (float)mip / (float)(env.prefilterMips - 1));
        for (int f = 0; f < 6; f++) {
            MgeGL_UniformMatrix4fv("view", faceView(f));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, env.prefilter, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            RenderCube();
        }
    }

    // 4. BRDF LUT (512, RG16F 2D)
    glGenTextures(1, &env.brdfLUT);
    glBindTexture(GL_TEXTURE_2D, env.brdfLUT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, env.brdfLUT, 0);
    glViewport(0, 0, 512, 512);
    MgeGL_SetShader(s_brdf);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(s_quadVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    MgeGL_RegisterDrawCall();
    glBindVertexArray(0);

    // restore
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &rbo);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
    Mge_UnloadTexture(hdr);

    return env;
}

void Mge_UnloadEnvironment(Environment* env)
{
    if (env == NULL)
        return;
    if (env->cubemap) glDeleteTextures(1, &env->cubemap);
    if (env->irradiance) glDeleteTextures(1, &env->irradiance);
    if (env->prefilter) glDeleteTextures(1, &env->prefilter);
    if (env->brdfLUT) glDeleteTextures(1, &env->brdfLUT);
    *env = (Environment){ 0 };
}

void Mge_DrawEnvironmentSkybox(Environment env, Camera3D camera)
{
    if (env.cubemap == 0)
        return;
    MgeGL_Draw();
    EnsureShaders();

    Matrix view = MatrixLookAt(camera.position, Vector3_Add(camera.position, camera.target), camera.up);
    view.m12 = view.m13 = view.m14 = 0.0f; // rotation only
    int w = Mge_GetScreenWidth(), h = Mge_GetScreenHeight();
    Matrix proj = MatrixPerspective((double)camera.fovy * DEG2RAD,
        (h != 0) ? (double)w / (double)h : 1.0, Mge_GetClipNear(), Mge_GetClipFar());

    MgeGL_SetShader(s_sky);
    MgeGL_UniformMatrix4fv("view", view);
    MgeGL_UniformMatrix4fv("projection", proj);
    MgeGL_Uniform1i("envMap", 0);
    MgeGL_SetDepthFunc(DEPTH_LEQUAL);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env.cubemap);
    RenderCube();
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    MgeGL_SetDepthFunc(DEPTH_LESS);
    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
}
