// Cubemap / env-map definitions. The GL side (cubemap upload, skybox, probes)
// runs only with a live context and is exercised by examples/cubemap/*.

#include "mge.h"
#include "test.h"

TEST(envmap_mode_enum)
{
    CHECK(ENVMAP_REFLECT == 0); // the env shader branches: mode == 0 -> reflect
    CHECK(ENVMAP_REFRACT == 1); // otherwise refract
}

TEST(zeroed_handles_are_inert)
{
    Cubemap cm = { 0 };
    CHECK(cm.id == 0 && cm.size == 0);

    EnvProbe p = { 0 };
    CHECK(p.fbo == 0 && p.cubemap.id == 0 && p.depth == 0 && p.size == 0);
    // Mge_UnloadCubemap / Mge_UnloadEnvProbe guard on the ids -- no-ops on these.
}

int main(void)
{
    RUN(envmap_mode_enum);
    RUN(zeroed_handles_are_inert);
    return test_summary();
}
