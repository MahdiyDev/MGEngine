// MSAA (multisample anti-aliasing).
//
// The sample COUNT is requested once, before Mge_InitWindow -- the window then
// gets a multisampled default framebuffer and every edge the renderer rasterizes
// (shapes, objects, meshes, models) comes out smoothed, with nothing extra to do
// per draw. Mge_GetMSAA reports the sample count the driver actually granted.
//
// The multisample resolve can then be turned off / back on at runtime with
// Mge_SetMSAAEnabled -- useful for an editor toggle. It cannot raise the count
// past what the window was created with; disabling it renders as if 1x.

#include "mge.h"
#include "mge_gl.h"

static int s_requestedSamples = 4; // default: 4x MSAA
static bool s_enabled = true;      // GL_MULTISAMPLE state (runtime toggle)

void Mge_SetMSAA(int samples)
{
    // GL needs >= 2 samples to multisample; treat anything less as "off"
    s_requestedSamples = (samples < 2) ? 0 : samples;
}

int Mge_GetRequestedMSAA(void)
{
    return s_requestedSamples; // read by the platform layer to hint the window
}

int Mge_GetMSAA(void)
{
    return s_enabled ? MgeGL_GetSampleCount() : 0; // 0 until the window exists, or when disabled
}

void Mge_SetMSAAEnabled(bool enabled)
{
    s_enabled = enabled;
    MgeGL_SetMultisample(enabled); // needs a live GL context -- call after Mge_InitWindow
}

bool Mge_IsMSAAEnabled(void)
{
    return s_enabled;
}
