// ShadowMap struct contract. The depth-pass FBO and the PCF shader are raw GL,
// exercised only with a live context by the examples.

#include "mge.h"
#include "mge_math.h"
#include "test.h"

TEST(shadow_map_zero_value_is_inert)
{
    ShadowMap sm = { 0 };
    CHECK(sm.fbo == 0);
    CHECK(sm.depthTexture == 0);
    CHECK(sm.size == 0);
    // Mge_UnloadShadowMap(&sm) / Mge_BeginShadowPass(&sm, ...) must be no-ops
    // (they guard on fbo) -- checked by the example.
}

TEST(shadow_map_carries_a_light_space_matrix)
{
    ShadowMap sm = { 0 };
    sm.lightSpaceMatrix = Matrix_Identity();
    // the field the lighting shader reads back for the PCF compare
    CHECK(sm.lightSpaceMatrix.m0 == 1.0f);
    CHECK(sm.lightSpaceMatrix.m15 == 1.0f);
    CHECK(sm.lightSpaceMatrix.m1 == 0.0f);
}

int main(void)
{
    RUN(shadow_map_zero_value_is_inert);
    RUN(shadow_map_carries_a_light_space_matrix);
    return test_summary();
}
