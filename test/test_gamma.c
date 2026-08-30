// Gamma-correction state (mge_gamma.c). GL_FRAMEBUFFER_SRGB needs a context, so
// MgeGL_SetFramebufferSRGB is stubbed and records the last value it was given.

#include <stdbool.h>

#include "mge.h"
#include "mge_gl.h"
#include "test.h"

static int g_srgbCalls;
static bool g_srgbLast;

void MgeGL_SetFramebufferSRGB(bool enabled)
{
    g_srgbCalls++;
    g_srgbLast = enabled;
}

TEST(gamma_is_off_by_default)
{
    CHECK(Mge_GetGammaCorrection() == false);
}

TEST(set_gamma_toggles_and_forwards_to_gl)
{
    Mge_SetGammaCorrection(true);
    CHECK(Mge_GetGammaCorrection() == true);
    CHECK(g_srgbLast == true);

    Mge_SetGammaCorrection(false);
    CHECK(Mge_GetGammaCorrection() == false);
    CHECK(g_srgbLast == false);
}

TEST(apply_gamma_state_reasserts_current_flag)
{
    Mge_SetGammaCorrection(true);
    int before = g_srgbCalls;
    Mge_ApplyGammaState();
    CHECK(g_srgbCalls == before + 1);
    CHECK(g_srgbLast == true);

    Mge_SetGammaCorrection(false); // leave the default put back
}

int main(void)
{
    RUN(gamma_is_off_by_default);
    RUN(set_gamma_toggles_and_forwards_to_gl);
    RUN(apply_gamma_state_reasserts_current_flag);
    return test_summary();
}
