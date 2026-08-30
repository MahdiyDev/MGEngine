// Gamma correction.
//
// Displays are ~2.2-gamma: they darken their input. Lighting maths, though, are
// linear, so their output looks too dark on screen unless it is encoded to sRGB
// on the way out. Mge_SetGammaCorrection(true) turns on GL_FRAMEBUFFER_SRGB, so
// the GPU does that encode on every write to the window's framebuffer.
//
// Off by default: the engine's shape / vertex / light colours are authored in
// sRGB-ish space, not linear, so flipping this changes their look. Turn it on
// when you work in linear space -- load colour textures with
// Mge_LoadTextureEx(path, true) and treat light colours as linear.

#include "mge.h"
#include "mge_gl.h"

static bool s_enabled = false;

void Mge_SetGammaCorrection(bool enabled)
{
    s_enabled = enabled;
    MgeGL_SetFramebufferSRGB(enabled);
}

bool Mge_GetGammaCorrection(void)
{
    return s_enabled;
}

void Mge_ApplyGammaState(void)
{
    MgeGL_SetFramebufferSRGB(s_enabled); // after a context (re)exists
}
