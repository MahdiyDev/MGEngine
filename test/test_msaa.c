// MSAA request handling (mge_msaa.c). The GL side (GLFW_SAMPLES hint, GL_SAMPLES
// query) needs a live context; here MgeGL_GetSampleCount is stubbed.

#include "mge.h"
#include "mge_gl.h"
#include "test.h"

static int g_stubbedSamples;

int MgeGL_GetSampleCount(void) { return g_stubbedSamples; }

TEST(default_is_4x)
{
    CHECK(Mge_GetRequestedMSAA() == 4);
}

TEST(set_msaa_clamps_below_two_to_off)
{
    Mge_SetMSAA(0);
    CHECK(Mge_GetRequestedMSAA() == 0);
    Mge_SetMSAA(1);
    CHECK(Mge_GetRequestedMSAA() == 0);
    Mge_SetMSAA(-8);
    CHECK(Mge_GetRequestedMSAA() == 0);
}

TEST(set_msaa_keeps_valid_sample_counts)
{
    Mge_SetMSAA(2);
    CHECK(Mge_GetRequestedMSAA() == 2);
    Mge_SetMSAA(4);
    CHECK(Mge_GetRequestedMSAA() == 4);
    Mge_SetMSAA(8);
    CHECK(Mge_GetRequestedMSAA() == 8);
}

TEST(get_msaa_reports_the_framebuffer_sample_count)
{
    g_stubbedSamples = 0;
    CHECK(Mge_GetMSAA() == 0);
    g_stubbedSamples = 4;
    CHECK(Mge_GetMSAA() == 4);
}

int main(void)
{
    RUN(default_is_4x);
    RUN(set_msaa_clamps_below_two_to_off);
    RUN(set_msaa_keeps_valid_sample_counts);
    RUN(get_msaa_reports_the_framebuffer_sample_count);
    return test_summary();
}
