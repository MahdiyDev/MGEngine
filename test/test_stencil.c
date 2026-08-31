// Stencil-test forwarding and the object-outline state sequence.
// The MgeGL_* backend and the shape draws are stubbed.

#include <stdbool.h>
#include <string.h>

#include "mge.h"
#include "mge_gl.h"
#include "test.h"

// ---- recording stubs ----

static int g_stencilEnabled = -1;   // 1 on, 0 off
static int g_clearCalls;
static int g_lastFunc = -1, g_lastRef = -1;
static unsigned int g_lastFuncMask;
static int g_opFail = -1, g_opDepthFail = -1, g_opPass = -1;
static unsigned int g_lastMask;
static int g_colorMask = -1;
static int g_depthEnabled = 1;       // tracked; starts "on"
static int g_lastDepthFunc = -1;
static unsigned int g_curShader = 7; // pretend the lighting shader is active
static unsigned int g_lastSetShader;
static int g_cubeDraws;
static Vector3 g_lastCubeSize;

void MgeGL_EnableStencilTest(void) { g_stencilEnabled = 1; }
void MgeGL_DisableStencilTest(void) { g_stencilEnabled = 0; }
void MgeGL_SetStencilFunc(int func, int ref, unsigned int mask)
{
    g_lastFunc = func;
    g_lastRef = ref;
    g_lastFuncMask = mask;
}
void MgeGL_SetStencilOp(int f, int df, int p) { g_opFail = f; g_opDepthFail = df; g_opPass = p; }
void MgeGL_SetStencilMask(unsigned int mask) { g_lastMask = mask; }
void MgeGL_SetColorMask(bool enabled) { g_colorMask = enabled ? 1 : 0; }
void MgeGL_ClearStencil(void) { g_clearCalls++; }
bool MgeGL_IsDepthTestEnabled(void) { return g_depthEnabled == 1; }
void MgeGL_EnableDepthTest(void) { g_depthEnabled = 1; }
void MgeGL_DisableDepthTest(void) { g_depthEnabled = 0; }
void MgeGL_SetDepthFunc(int f) { g_lastDepthFunc = f; }
unsigned int MgeGL_GetDefaultShaderId(void) { return 0; }
unsigned int MgeGL_GetCurrentShaderId(void) { return g_curShader; }
void MgeGL_SetShader(unsigned int id) { g_lastSetShader = id; g_curShader = id; }

void Draw_Cube(Vector3 pos, Vector3 size, Color c)
{
    (void)pos; (void)c;
    g_cubeDraws++;
    g_lastCubeSize = size;
}
void Draw_CubeEx(Vector3 pos, Vector3 size, Vector3 rot, Color c)
{
    (void)rot;
    Draw_Cube(pos, size, c);
}
void Draw_RectangleRec(Rectangle r, Color c) { (void)r; (void)c; }

// lives in mge_object.c (not linked here); the outline only needs the cube path
void Mge_DrawPrimitive(Object o, Color c) { Draw_CubeEx(o.position, o.size, o.rotation, c); }

// ---- tests ----

TEST(raw_stencil_forwarding)
{
    Mge_EnableStencilTest();
    CHECK(g_stencilEnabled == 1);
    Mge_DisableStencilTest();
    CHECK(g_stencilEnabled == 0);

    Mge_SetStencilFunc(STENCIL_EQUAL, 2, 0x0Fu);
    CHECK(g_lastFunc == STENCIL_EQUAL && g_lastRef == 2 && g_lastFuncMask == 0x0Fu);

    Mge_SetStencilOp(STENCIL_KEEP, STENCIL_INCR, STENCIL_REPLACE);
    CHECK(g_opFail == STENCIL_KEEP && g_opDepthFail == STENCIL_INCR && g_opPass == STENCIL_REPLACE);

    Mge_SetStencilMask(0xABu);
    CHECK(g_lastMask == 0xABu);

    int c = g_clearCalls;
    Mge_ClearStencil();
    CHECK(g_clearCalls == c + 1);
}

TEST(begin_stencil_mask_sets_stamp_state)
{
    g_depthEnabled = 1;
    Mge_BeginStencilMask();

    CHECK(g_stencilEnabled == 1);
    CHECK(g_opFail == STENCIL_KEEP && g_opDepthFail == STENCIL_KEEP && g_opPass == STENCIL_REPLACE);
    CHECK(g_lastFunc == STENCIL_ALWAYS && g_lastRef == 1 && g_lastFuncMask == 0xFFu);
    CHECK(g_lastMask == 0xFFu);
    CHECK(g_colorMask == 0);      // colour writes off
    CHECK(g_depthEnabled == 0);   // depth test off -> full silhouette
}

TEST(begin_stencil_outside_flips_to_border)
{
    g_curShader = 7; // "lighting" shader active
    Mge_BeginStencilMask();
    Mge_BeginStencilOutside();
    CHECK(g_lastFunc == STENCIL_NOTEQUAL && g_lastRef == 1 && g_lastFuncMask == 0xFFu);
    CHECK(g_lastMask == 0x00u);   // border pass writes no stencil
    CHECK(g_colorMask == 1);
    CHECK(g_lastSetShader == 0);      // flat/unlit shader for the border
    CHECK(g_depthEnabled == 1);       // depth back on so the border writes depth
    CHECK(g_lastDepthFunc == DEPTH_ALWAYS); // ...but over everything
    Mge_EndStencil();
}

TEST(end_stencil_restores)
{
    g_depthEnabled = 1;
    g_curShader = 7;
    Mge_BeginStencilMask();   // turns depth test off, remembers it was on
    Mge_BeginStencilOutside();
    Mge_EndStencil();

    CHECK(g_lastSetShader == 7);         // previous shader restored
    CHECK(g_lastDepthFunc == DEPTH_LESS);

    CHECK(g_stencilEnabled == 0);
    CHECK(g_lastMask == 0xFFu);
    CHECK(g_lastFunc == STENCIL_ALWAYS && g_lastRef == 0);
    CHECK(g_colorMask == 1);
    CHECK(g_depthEnabled == 1);   // restored to what it was before the mask
}

TEST(end_stencil_keeps_depth_off_when_it_was_off)
{
    g_depthEnabled = 0;          // e.g. outlining during 2D drawing
    Mge_BeginStencilMask();
    Mge_EndStencil();
    CHECK(g_depthEnabled == 0);
}

TEST(draw_object_outline_stamps_then_borders)
{
    Object box = { .kind = OBJECT_3D, .primitive = PRIM_CUBE, .position = { 0, 0, 0 }, .size = { 2, 2, 2 } };
    g_cubeDraws = 0;

    Mge_DrawObjectOutline(box, 0.5f, WHITE);

    CHECK(g_cubeDraws == 2);                 // stamp + border
    CHECK(g_lastCubeSize.x == 2.5f);         // border is size + thickness
    CHECK(g_stencilEnabled == 0);            // ended cleanly
}

int main(void)
{
    RUN(raw_stencil_forwarding);
    RUN(begin_stencil_mask_sets_stamp_state);
    RUN(begin_stencil_outside_flips_to_border);
    RUN(end_stencil_restores);
    RUN(end_stencil_keeps_depth_off_when_it_was_off);
    RUN(draw_object_outline_stamps_then_borders);
    return test_summary();
}
