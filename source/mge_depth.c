// Depth testing: tuning the depth test, the near/far clip planes (which set
// depth precision), polygon offset for coplanar geometry, and a shader that
// draws the linearized depth buffer as grayscale.

#include "mge.h"
#include "mge_gl.h"

// ---- near / far clip planes -------------------------------------------------
//
// These decide how the [0,1] depth buffer maps to view-space distance. A very
// small `near` packs almost all precision into the first fraction of the range,
// so distant surfaces share depth values and z-fight. Raising `near` is the
// single most effective fix.

static double s_near = MGE_CULL_DISTANCE_NEAR;
static double s_far = MGE_CULL_DISTANCE_FAR;

void Mge_SetClipPlanes(double near, double far)
{
    if (near > 0.0 && far > near) {
        s_near = near;
        s_far = far;
    }
}

double Mge_GetClipNear(void) { return s_near; }
double Mge_GetClipFar(void) { return s_far; }

// ---- depth test state ------------------------------------------------------

void Mge_EnableDepthTest(void) { MgeGL_EnableDepthTest(); }
void Mge_DisableDepthTest(void) { MgeGL_DisableDepthTest(); }
void Mge_SetDepthFunc(int func) { MgeGL_SetDepthFunc(func); }
void Mge_SetDepthMask(bool write) { MgeGL_SetDepthMask(write); }

// ---- polygon offset ------------------------------------------------------

void Mge_SetPolygonOffset(float factor, float units) { MgeGL_SetPolygonOffset(true, factor, units); }
void Mge_DisablePolygonOffset(void) { MgeGL_SetPolygonOffset(false, 0.0f, 0.0f); }

// ---- depth-buffer visualization ------------------------------------------

static const char* depthVertCode =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "uniform mat4 modelview;\n"
    "uniform mat4 projection;\n"
    "void main() { gl_Position = projection * modelview * vec4(aPos, 1.0); }\n";

static const char* depthFragCode =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform float nearPlane;\n"
    "uniform float farPlane;\n"
    "void main()\n"
    "{\n"
    "    float ndc = gl_FragCoord.z * 2.0 - 1.0;\n"
    "    float linear = (2.0 * nearPlane * farPlane) /\n"
    "                   (farPlane + nearPlane - ndc * (farPlane - nearPlane));\n"
    "    float g = linear / farPlane;\n"
    "    FragColor = vec4(vec3(g), 1.0);\n"
    "}\n";

static Shader s_depthShader = { 0 };
static bool s_depthLoaded = false;

void Mge_BeginDepthPreview(void)
{
    if (!s_depthLoaded) {
        s_depthShader = Mge_LoadShaderFromMemory(depthVertCode, depthFragCode);
        s_depthLoaded = true;
    }

    MgeGL_SetShader(s_depthShader.id);
    MgeGL_Uniform1f("nearPlane", (float)s_near);
    MgeGL_Uniform1f("farPlane", (float)s_far);
}

void Mge_EndDepthPreview(void)
{
    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
}
