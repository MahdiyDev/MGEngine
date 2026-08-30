// MSAA (multisample anti-aliasing).
//
// Requested once, before Mge_InitWindow -- the window then gets a multisampled
// default framebuffer and every edge the renderer rasterizes (shapes, objects,
// meshes, models) comes out smoothed, with nothing extra for the caller to do
// per draw. Mge_GetMSAA reports the sample count the driver actually granted.

#include "mge.h"
#include "mge_gl.h"

static int s_requestedSamples = 4; // default: 4x MSAA

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
    return MgeGL_GetSampleCount(); // 0 until the window (and its framebuffer) exist
}
