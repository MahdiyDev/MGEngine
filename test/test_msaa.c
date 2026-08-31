// MSAA request handling (mge_msaa.c). The GL side (GLFW_SAMPLES hint, GL_SAMPLES
// query) needs a live context; here MgeGL_GetSampleCount is stubbed.

#include "mge.h"
#include "mge_gl.h"
#include "test.h"

static int g_stubbedSamples;
static int g_multisampleCalls;
static bool g_multisampleState;

int MgeGL_GetSampleCount(void) { return g_stubbedSamples; }
void MgeGL_SetMultisample(bool enabled)
{
    g_multisampleCalls++;
    g_multisampleState = enabled;
}

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
    Mge_SetMSAAEnabled(true); // reset the runtime toggle
    g_stubbedSamples = 0;
    CHECK(Mge_GetMSAA() == 0);
    g_stubbedSamples = 4;
    CHECK(Mge_GetMSAA() == 4);
}

TEST(runtime_toggle_forwards_and_masks_the_count)
{
    g_stubbedSamples = 4;
    g_multisampleCalls = 0;

    Mge_SetMSAAEnabled(false);
    CHECK(g_multisampleCalls == 1 && g_multisampleState == false);
    CHECK(!Mge_IsMSAAEnabled());
    CHECK(Mge_GetMSAA() == 0); // disabled -> reports 0 even though the fb has 4

    Mge_SetMSAAEnabled(true);
    CHECK(g_multisampleState == true);
    CHECK(Mge_IsMSAAEnabled());
    CHECK(Mge_GetMSAA() == 4);
}

int main(void)
{
    RUN(default_is_4x);
    RUN(set_msaa_clamps_below_two_to_off);
    RUN(set_msaa_keeps_valid_sample_counts);
    RUN(get_msaa_reports_the_framebuffer_sample_count);
    RUN(runtime_toggle_forwards_and_masks_the_count);
    return test_summary();
}
