// Face-culling state forwarding (mge_cull.c -> MgeGL_*, stubbed).

#include <stdbool.h>

#include "mge.h"
#include "mge_gl.h"
#include "test.h"

static int g_enabled = -1; // 1 on, 0 off
static int g_lastCullFace = -1;
static int g_lastFrontFace = -1;

void MgeGL_SetFaceCulling(bool enabled) { g_enabled = enabled ? 1 : 0; }
void MgeGL_SetCullFace(int face) { g_lastCullFace = face; }
void MgeGL_SetFrontFace(int winding) { g_lastFrontFace = winding; }

TEST(enum_values)
{
    CHECK(CULL_BACK == 0 && CULL_FRONT == 1 && CULL_FRONT_AND_BACK == 2);
    CHECK(WINDING_CCW == 0 && WINDING_CW == 1);
}

TEST(enable_disable_forwarded)
{
    Mge_EnableFaceCulling();
    CHECK(g_enabled == 1);
    Mge_DisableFaceCulling();
    CHECK(g_enabled == 0);
}

TEST(setters_forwarded)
{
    Mge_SetCullFace(CULL_FRONT);
    CHECK(g_lastCullFace == CULL_FRONT);
    Mge_SetCullFace(CULL_FRONT_AND_BACK);
    CHECK(g_lastCullFace == CULL_FRONT_AND_BACK);

    Mge_SetFrontFace(WINDING_CW);
    CHECK(g_lastFrontFace == WINDING_CW);
    Mge_SetFrontFace(WINDING_CCW);
    CHECK(g_lastFrontFace == WINDING_CCW);
}

int main(void)
{
    RUN(enum_values);
    RUN(enable_disable_forwarded);
    RUN(setters_forwarded);
    return test_summary();
}
