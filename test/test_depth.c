// Depth-testing controls: clip planes, depth func / mask, polygon offset, and
// the depth-preview shader wiring. The MgeGL_* backend is stubbed.

#include <stdbool.h>
#include <string.h>

#include "mge.h"
#include "mge_gl.h"
#include "test.h"

// ---- recording stubs ----

static int g_enableCalls, g_disableCalls;
static int g_lastDepthFunc = -1;
static int g_depthMask = -1; // -1 unset, 0 false, 1 true
static struct {
    int enabled;
    float factor, units;
} g_poly;
static unsigned int g_lastShader = 12345;
static float g_nearUniform = -1.0f, g_farUniform = -1.0f;

void MgeGL_EnableDepthTest(void) { g_enableCalls++; }
void MgeGL_DisableDepthTest(void) { g_disableCalls++; }
void MgeGL_SetDepthFunc(int depthFunc) { g_lastDepthFunc = depthFunc; }
void MgeGL_SetDepthMask(bool writeEnabled) { g_depthMask = writeEnabled ? 1 : 0; }
void MgeGL_SetPolygonOffset(bool enabled, float factor, float units)
{
    g_poly.enabled = enabled ? 1 : 0;
    g_poly.factor = factor;
    g_poly.units = units;
}
void MgeGL_SetShader(unsigned int id) { g_lastShader = id; }
unsigned int MgeGL_GetDefaultShaderId(void) { return 0; }
void MgeGL_Uniform1f(const char* name, float value)
{
    if (strcmp(name, "nearPlane") == 0)
        g_nearUniform = value;
    else if (strcmp(name, "farPlane") == 0)
        g_farUniform = value;
}
Shader Mge_LoadShaderFromMemory(const char* vs, const char* fs)
{
    (void)vs;
    (void)fs;
    return (Shader){ .id = 99, .locs = NULL };
}

static bool deq(double a, double b) { return (a - b < 1e-9) && (b - a < 1e-9); }

// ---- tests ----

TEST(default_clip_planes)
{
    CHECK(deq(Mge_GetClipNear(), MGE_CULL_DISTANCE_NEAR));
    CHECK(deq(Mge_GetClipFar(), MGE_CULL_DISTANCE_FAR));
}

TEST(set_clip_planes_valid)
{
    Mge_SetClipPlanes(0.5, 200.0);
    CHECK(deq(Mge_GetClipNear(), 0.5));
    CHECK(deq(Mge_GetClipFar(), 200.0));

    Mge_SetClipPlanes(MGE_CULL_DISTANCE_NEAR, MGE_CULL_DISTANCE_FAR); // restore
}

TEST(set_clip_planes_rejects_bad_input)
{
    Mge_SetClipPlanes(2.0, 50.0);

    Mge_SetClipPlanes(0.0, 100.0);   // near must be > 0
    Mge_SetClipPlanes(-1.0, 100.0);
    Mge_SetClipPlanes(10.0, 5.0);    // far must be > near
    Mge_SetClipPlanes(10.0, 10.0);

    CHECK(deq(Mge_GetClipNear(), 2.0));
    CHECK(deq(Mge_GetClipFar(), 50.0));

    Mge_SetClipPlanes(MGE_CULL_DISTANCE_NEAR, MGE_CULL_DISTANCE_FAR);
}

TEST(depth_func_and_mask_forwarded)
{
    Mge_SetDepthFunc(DEPTH_LEQUAL);
    CHECK(g_lastDepthFunc == DEPTH_LEQUAL);
    Mge_SetDepthFunc(DEPTH_ALWAYS);
    CHECK(g_lastDepthFunc == DEPTH_ALWAYS);

    Mge_SetDepthMask(false);
    CHECK(g_depthMask == 0);
    Mge_SetDepthMask(true);
    CHECK(g_depthMask == 1);
}

TEST(enable_disable_depth_test_forwarded)
{
    int e = g_enableCalls, d = g_disableCalls;
    Mge_EnableDepthTest();
    Mge_DisableDepthTest();
    CHECK(g_enableCalls == e + 1);
    CHECK(g_disableCalls == d + 1);
}

TEST(polygon_offset)
{
    Mge_SetPolygonOffset(-1.5f, -2.0f);
    CHECK(g_poly.enabled == 1);
    CHECK(g_poly.factor == -1.5f && g_poly.units == -2.0f);

    Mge_DisablePolygonOffset();
    CHECK(g_poly.enabled == 0);
}

TEST(depth_preview_binds_shader_and_planes)
{
    Mge_SetClipPlanes(0.25, 80.0);

    Mge_BeginDepthPreview();
    CHECK(g_lastShader == 99);                 // the loaded depth shader
    CHECK(g_nearUniform == 0.25f);
    CHECK(g_farUniform == 80.0f);

    Mge_EndDepthPreview();
    CHECK(g_lastShader == 0);                  // back to the default shader

    Mge_SetClipPlanes(MGE_CULL_DISTANCE_NEAR, MGE_CULL_DISTANCE_FAR);
}

int main(void)
{
    RUN(default_clip_planes);
    RUN(set_clip_planes_valid);
    RUN(set_clip_planes_rejects_bad_input);
    RUN(depth_func_and_mask_forwarded);
    RUN(enable_disable_depth_test_forwarded);
    RUN(polygon_offset);
    RUN(depth_preview_binds_shader_and_planes);
    return test_summary();
}
