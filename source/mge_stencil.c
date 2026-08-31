// Stencil testing + the object-outlining technique built on it.
//
// Outline flow (see Mge_BeginStencilMask / Outside / EndStencil in mge.h):
//   1. mask pass  -- stamp `1` into the stencil over the object's silhouette,
//                    with colour and depth writes off (so the already-rendered
//                    object is untouched).
//   2. border pass -- draw a slightly larger shell, but only where the stencil
//                     is NOT `1`, so just the border survives.
//   3. restore     -- stencil off, colour mask on, depth test back to what it was.

#include "mge.h"
#include "mge_gl.h"

// ---- raw stencil state ----

void Mge_EnableStencilTest(void) { MgeGL_EnableStencilTest(); }
void Mge_DisableStencilTest(void) { MgeGL_DisableStencilTest(); }
void Mge_SetStencilFunc(int func, int ref, unsigned int mask) { MgeGL_SetStencilFunc(func, ref, mask); }
void Mge_SetStencilOp(int onStencilFail, int onDepthFail, int onPass)
{
    MgeGL_SetStencilOp(onStencilFail, onDepthFail, onPass);
}
void Mge_SetStencilMask(unsigned int mask) { MgeGL_SetStencilMask(mask); }
void Mge_ClearStencil(void) { MgeGL_ClearStencil(); }

// ---- outlining ----

static bool s_depthTestWasOn = true;
static unsigned int s_prevShader = 0;

void Mge_BeginStencilMask(void)
{
    s_depthTestWasOn = MgeGL_IsDepthTestEnabled();
    s_prevShader = MgeGL_GetCurrentShaderId();

    MgeGL_EnableStencilTest();
    MgeGL_SetStencilOp(STENCIL_KEEP, STENCIL_KEEP, STENCIL_REPLACE);
    MgeGL_SetStencilFunc(STENCIL_ALWAYS, 1, 0xFF);
    MgeGL_SetStencilMask(0xFF);
    MgeGL_SetColorMask(false);   // stamp only -- don't touch the framebuffer
    MgeGL_DisableDepthTest();    // stamp the whole silhouette, even if occluded
}

void Mge_BeginStencilOutside(void)
{
    MgeGL_SetStencilFunc(STENCIL_NOTEQUAL, 1, 0xFF);
    MgeGL_SetStencilMask(0x00);  // border pass writes no stencil
    MgeGL_SetColorMask(true);
    MgeGL_SetShader(MgeGL_GetDefaultShaderId()); // flat, unlit border colour

    // draw the border over everything, but still WRITE depth -- otherwise a
    // later "draw last" pass (a skybox) paints straight over the outline
    MgeGL_EnableDepthTest();
    MgeGL_SetDepthFunc(DEPTH_ALWAYS);
}

void Mge_EndStencil(void)
{
    MgeGL_DisableStencilTest();
    MgeGL_SetStencilMask(0xFF);
    MgeGL_SetStencilFunc(STENCIL_ALWAYS, 0, 0xFF);
    MgeGL_SetColorMask(true);
    MgeGL_SetDepthFunc(DEPTH_LESS);
    MgeGL_SetShader(s_prevShader);

    if (s_depthTestWasOn)
        MgeGL_EnableDepthTest();
    else
        MgeGL_DisableDepthTest();
}

void Mge_DrawObjectOutline(Object obj, float thickness, Color color)
{
    if (!obj.active)
        return;

    const Vector3 p = obj.transform.position, s = obj.transform.scale;

    Mge_BeginStencilMask();
    if (obj.kind == OBJECT_3D) {
        Mge_DrawPrimitive(obj, color); // colour is irrelevant here -- colour mask is off
    } else {
        Draw_RectangleRec((Rectangle){ p.x - s.x * 0.5f, p.y - s.y * 0.5f, s.x, s.y }, color);
    }

    Mge_BeginStencilOutside();
    if (obj.kind == OBJECT_3D) {
        Object shell = obj;
        shell.transform.scale = (Vector3){ s.x + thickness, s.y + thickness, s.z + thickness };
        Mge_DrawPrimitive(shell, color);
    } else {
        float w = s.x + thickness, h = s.y + thickness;
        Draw_RectangleRec((Rectangle){ p.x - w * 0.5f, p.y - h * 0.5f, w, h }, color);
    }

    Mge_EndStencil();
}
