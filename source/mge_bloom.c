// Bloom -- LearnOpenGL Advanced-Lighting/Bloom.
//
// Three full-screen passes over the HDR scene texture:
//   1. bright pass  -- keep only pixels whose luminance exceeds `threshold`
//   2. blur         -- separable Gaussian, ping-ponging H/V for `iterations` rounds
//   3. composite    -- scene + blurred-bright * intensity, then tone map + gamma
//
// Everything runs at half resolution except the final composite. This variant
// extracts the bright pixels from the finished HDR image rather than using a
// second MRT attachment on the lighting shader, so nothing about the scene draw
// changes -- just swap Mge_DrawRenderTextureHDR for Mge_DrawBloom.

#include "mge.h"
#include "mge_gl.h"

#include <glad/glad.h>
#include <stddef.h>

static const char* bloomVert =
    "#version 330 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "layout(location = 1) in vec2 aUV;\n"
    "out vec2 vUV;\n"
    "void main() { vUV = aUV; gl_Position = vec4(aPos, 0.0, 1.0); }\n";

static const char* brightFrag =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 vUV;\n"
    "uniform sampler2D scene;\n"
    "uniform float threshold;\n"
    "void main()\n"
    "{\n"
    "    vec3 c = texture(scene, vUV).rgb;\n"
    "    float l = dot(c, vec3(0.2126, 0.7152, 0.0722));\n"
    // soft knee: fade in over the [threshold, threshold+1] band instead of a hard cut
    "    float k = clamp((l - threshold) / max(l, 1e-4), 0.0, 1.0);\n"
    "    FragColor = vec4(c * k, 1.0);\n"
    "}\n";

static const char* blurFrag =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 vUV;\n"
    "uniform sampler2D image;\n"
    "uniform vec2 texelSize;\n"
    "uniform int horizontal;\n"
    "const float w[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);\n"
    "void main()\n"
    "{\n"
    "    vec2 dir = (horizontal == 1) ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);\n"
    "    vec3 c = texture(image, vUV).rgb * w[0];\n"
    "    for (int i = 1; i < 5; i++) {\n"
    "        c += texture(image, vUV + dir * float(i)).rgb * w[i];\n"
    "        c += texture(image, vUV - dir * float(i)).rgb * w[i];\n"
    "    }\n"
    "    FragColor = vec4(c, 1.0);\n"
    "}\n";

static const char* compositeFrag =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 vUV;\n"
    "uniform sampler2D scene;\n"
    "uniform sampler2D bloomTex;\n"
    "uniform float intensity;\n"
    "uniform int toneMap;\n"       // matches ToneMap
    "uniform float exposure;\n"
    "uniform int applyGamma;\n"
    "vec3 aces(vec3 x)\n"
    "{\n"
    "    const float a = 2.51, b = 0.03, cc = 2.43, d = 0.59, e = 0.14;\n"
    "    return clamp((x * (a * x + b)) / (x * (cc * x + d) + e), 0.0, 1.0);\n"
    "}\n"
    "void main()\n"
    "{\n"
    "    vec3 hdr = texture(scene, vUV).rgb + texture(bloomTex, vUV).rgb * intensity;\n"
    "    vec3 m;\n"
    "    if (toneMap == 1)      m = vec3(1.0) - exp(-hdr * exposure);\n"
    "    else if (toneMap == 2) m = aces(hdr * exposure);\n"
    "    else                   m = hdr / (hdr + vec3(1.0));\n"
    "    if (applyGamma == 1) m = pow(m, vec3(1.0 / 2.2));\n"
    "    FragColor = vec4(m, 1.0);\n"
    "}\n";

static unsigned int s_bright = 0, s_blur = 0, s_composite = 0;
static unsigned int s_vao = 0, s_vbo = 0;

static void EnsureResources(void)
{
    if (s_vao != 0)
        return;

    unsigned int vs = MgeGL_LoadShader(bloomVert, GL_VERTEX_SHADER, "bloom vertex");
    s_bright = MgeGL_CreateShaderProgram(vs, MgeGL_LoadShader(brightFrag, GL_FRAGMENT_SHADER, "bloom bright"));
    s_blur = MgeGL_CreateShaderProgram(vs, MgeGL_LoadShader(blurFrag, GL_FRAGMENT_SHADER, "bloom blur"));
    s_composite = MgeGL_CreateShaderProgram(vs, MgeGL_LoadShader(compositeFrag, GL_FRAGMENT_SHADER, "bloom composite"));

    const float quad[24] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,
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

BloomFX Mge_LoadBloom(int sceneWidth, int sceneHeight)
{
    BloomFX b = { 0 };
    b.width = (sceneWidth > 1) ? sceneWidth / 2 : 1;
    b.height = (sceneHeight > 1) ? sceneHeight / 2 : 1;
    b.threshold = 1.0f;
    b.intensity = 0.6f;
    b.iterations = 5;

    b.bright = Mge_LoadRenderTextureHDR(b.width, b.height);
    b.pingpong[0] = Mge_LoadRenderTextureHDR(b.width, b.height);
    b.pingpong[1] = Mge_LoadRenderTextureHDR(b.width, b.height);
    return b;
}

void Mge_UnloadBloom(BloomFX* bloom)
{
    if (bloom == NULL)
        return;
    Mge_UnloadRenderTexture(bloom->bright);
    Mge_UnloadRenderTexture(bloom->pingpong[0]);
    Mge_UnloadRenderTexture(bloom->pingpong[1]);
    *bloom = (BloomFX){ 0 };
}

static void DrawQuad(void)
{
    glBindVertexArray(s_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    MgeGL_RegisterDrawCall();
    glBindVertexArray(0);
}

void Mge_DrawBloom(RenderTexture hdrScene, BloomFX* bloom, int toneMap, float exposure)
{
    if (bloom == NULL || bloom->bright.fbo == 0)
        return;

    MgeGL_Draw();
    EnsureResources();
    glDisable(GL_DEPTH_TEST);

    const float bw = 1.0f / (float)bloom->width;
    const float bh = 1.0f / (float)bloom->height;

    // 1. bright pass -> bloom->bright (half res)
    MgeGL_SetShader(s_bright);
    glBindFramebuffer(GL_FRAMEBUFFER, bloom->bright.fbo);
    MgeGL_Viewport(0, 0, bloom->width, bloom->height);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrScene.texture.id);
    glUniform1i(glGetUniformLocation(s_bright, "scene"), 0);
    glUniform1f(glGetUniformLocation(s_bright, "threshold"), bloom->threshold);
    DrawQuad();

    // 2. separable Gaussian, ping-ponging between pingpong[0] and [1]
    MgeGL_SetShader(s_blur);
    glUniform1i(glGetUniformLocation(s_blur, "image"), 0);
    glUniform2f(glGetUniformLocation(s_blur, "texelSize"), bw, bh);
    int passes = (bloom->iterations > 0 ? bloom->iterations : 1) * 2;
    int dst = 0;
    for (int i = 0; i < passes; i++, dst ^= 1) {
        glBindFramebuffer(GL_FRAMEBUFFER, bloom->pingpong[dst].fbo);
        glUniform1i(glGetUniformLocation(s_blur, "horizontal"), (i % 2 == 0) ? 1 : 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, (i == 0) ? bloom->bright.texture.id
                                              : bloom->pingpong[dst ^ 1].texture.id);
        DrawQuad();
    }
    unsigned int blurred = bloom->pingpong[dst ^ 1].texture.id; // last written

    // 3. composite scene + bloom, tone map, to the window
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    MgeGL_Viewport(0, 0, Mge_GetScreenWidth(), Mge_GetScreenHeight());
    MgeGL_SetShader(s_composite);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrScene.texture.id);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurred);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(s_composite, "scene"), 0);
    glUniform1i(glGetUniformLocation(s_composite, "bloomTex"), 1);
    glUniform1f(glGetUniformLocation(s_composite, "intensity"), bloom->intensity);
    glUniform1i(glGetUniformLocation(s_composite, "toneMap"), toneMap);
    glUniform1f(glGetUniformLocation(s_composite, "exposure"), (exposure > 0.0f) ? exposure : 1.0f);
    glUniform1i(glGetUniformLocation(s_composite, "applyGamma"), Mge_GetGammaCorrection() ? 0 : 1);
    DrawQuad();

    glBindTexture(GL_TEXTURE_2D, 0);
    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
}
