// Offscreen framebuffers + full-screen post-processing.
//
// Mge_BeginTextureMode / Mge_EndTextureMode redirect all drawing into a
// RenderTexture; Mge_DrawRenderTextureFX then draws that colour texture over the
// window through one of the built-in effect shaders (inversion, grayscale, and
// the sharpen / blur / edge 3x3 kernels).

#include "mge.h"
#include "mge_gl.h"
#include "mge_utils.h"

#include <glad/glad.h>
#include <stddef.h>

// ---- the full-screen effect shader (one program, `effect` picks the maths) ----

static const char* fxVert =
    "#version 330 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "layout(location = 1) in vec2 aUV;\n"
    "out vec2 vUV;\n"
    "void main() { vUV = aUV; gl_Position = vec4(aPos, 0.0, 1.0); }\n";

static const char* fxFrag =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 vUV;\n"
    "uniform sampler2D screenTex;\n"
    "uniform vec2 texelSize;\n"   // 1 / render-texture size
    "uniform int effect;\n"       // matches the PostFX enum
    "\n"
    "vec3 kernel(float k0,float k1,float k2,float k3,float k4,float k5,float k6,float k7,float k8)\n"
    "{\n"
    "    vec2 t = texelSize;\n"
    "    vec3 s = vec3(0.0);\n"
    "    s += texture(screenTex, vUV + vec2(-t.x,  t.y)).rgb * k0;\n"
    "    s += texture(screenTex, vUV + vec2( 0.0,  t.y)).rgb * k1;\n"
    "    s += texture(screenTex, vUV + vec2( t.x,  t.y)).rgb * k2;\n"
    "    s += texture(screenTex, vUV + vec2(-t.x,  0.0)).rgb * k3;\n"
    "    s += texture(screenTex, vUV                   ).rgb * k4;\n"
    "    s += texture(screenTex, vUV + vec2( t.x,  0.0)).rgb * k5;\n"
    "    s += texture(screenTex, vUV + vec2(-t.x, -t.y)).rgb * k6;\n"
    "    s += texture(screenTex, vUV + vec2( 0.0, -t.y)).rgb * k7;\n"
    "    s += texture(screenTex, vUV + vec2( t.x, -t.y)).rgb * k8;\n"
    "    return s;\n"
    "}\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec3 c = texture(screenTex, vUV).rgb;\n"
    "    if (effect == 1) c = 1.0 - c;\n"
    "    else if (effect == 2) c = vec3(dot(c, vec3(0.2126, 0.7152, 0.0722)));\n"
    "    else if (effect == 3) c = kernel(-1.,-1.,-1., -1., 9.,-1., -1.,-1.,-1.);\n"          // sharpen
    "    else if (effect == 4) c = kernel(.0625,.125,.0625, .125,.25,.125, .0625,.125,.0625);\n" // blur
    "    else if (effect == 5) c = kernel(1.,1.,1., 1.,-8.,1., 1.,1.,1.);\n"                  // edge
    "    FragColor = vec4(c, 1.0);\n"
    "}\n";

static unsigned int s_fxProgram = 0;
static unsigned int s_quadVao = 0;
static unsigned int s_quadVbo = 0;

static void EnsureFxResources(void)
{
    if (s_fxProgram != 0)
        return;

    unsigned int vs = MgeGL_LoadShader(fxVert, GL_VERTEX_SHADER, "postfx vertex");
    unsigned int fs = MgeGL_LoadShader(fxFrag, GL_FRAGMENT_SHADER, "postfx fragment");
    s_fxProgram = MgeGL_CreateShaderProgram(vs, fs);

    // a full-screen triangle pair in clip space: [pos.xy][uv]
    const float quad[24] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
    };
    glGenVertexArrays(1, &s_quadVao);
    glGenBuffers(1, &s_quadVbo);
    glBindVertexArray(s_quadVao);
    glBindBuffer(GL_ARRAY_BUFFER, s_quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ---- render textures ----

static void SetOrtho(int w, int h)
{
    MgeGL_MatrixMode(MGEGL_PROJECTION);
    MgeGL_LoadIdentity();
    MgeGL_Ortho(0.0, (double)w, (double)h, 0.0, 0.0, 1.0);
    MgeGL_MatrixMode(MGEGL_MODELVIEW);
    MgeGL_LoadIdentity();
}

RenderTexture Mge_LoadRenderTexture(int width, int height)
{
    RenderTexture rt = { 0 };
    rt.width = width;
    rt.height = height;

    glGenFramebuffers(1, &rt.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, rt.fbo);

    glGenTextures(1, &rt.texture.id);
    glBindTexture(GL_TEXTURE_2D, rt.texture.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt.texture.id, 0);
    rt.texture.width = width;
    rt.texture.height = height;
    rt.texture.mipmaps = 1;
    rt.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    glGenRenderbuffers(1, &rt.depthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, rt.depthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rt.depthStencil);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        TRACE_LOG(LOG_WARNING, "FBO: [%ux%u] framebuffer is not complete", width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    return rt;
}

void Mge_UnloadRenderTexture(RenderTexture target)
{
    if (target.depthStencil != 0)
        glDeleteRenderbuffers(1, &target.depthStencil);
    if (target.texture.id != 0)
        glDeleteTextures(1, &target.texture.id);
    if (target.fbo != 0)
        glDeleteFramebuffers(1, &target.fbo);
}

void Mge_BeginTextureMode(RenderTexture target)
{
    MgeGL_Draw(); // flush whatever was going to the window
    glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);
    MgeGL_Viewport(0, 0, target.width, target.height);
    SetOrtho(target.width, target.height);
}

void Mge_EndTextureMode(void)
{
    MgeGL_Draw();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    int w = Mge_GetScreenWidth();
    int h = Mge_GetScreenHeight();
    MgeGL_Viewport(0, 0, w, h);
    SetOrtho(w, h);
}

void Mge_DrawRenderTextureFX(RenderTexture target, int effect)
{
    MgeGL_Draw(); // flush any 2D batch first
    EnsureFxResources();

    MgeGL_SetShader(s_fxProgram); // keeps MgeGL's tracked program in sync

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, target.texture.id);
    glUniform1i(glGetUniformLocation(s_fxProgram, "screenTex"), 0);
    glUniform2f(glGetUniformLocation(s_fxProgram, "texelSize"),
        (target.width > 0) ? 1.0f / (float)target.width : 0.0f,
        (target.height > 0) ? 1.0f / (float)target.height : 0.0f);
    glUniform1i(glGetUniformLocation(s_fxProgram, "effect"), effect);

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(s_quadVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    MgeGL_SetShader(MgeGL_GetDefaultShaderId()); // restore for later batcher draws
}
