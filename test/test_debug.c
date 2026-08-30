// GL debug-output toggle (mge_debug.c). The callback registration goes through
// the fake glad layer (test/glstub/), which records it.

#include <stdbool.h>

#include "mge.h"
#include "glstub.h"
#include "test.h"

TEST(default_follows_build_type)
{
    // this suite is compiled without -DNDEBUG, so debug output defaults on
    CHECK(Mge_GetDebugOutput() == true);
}

TEST(set_toggles_the_flag)
{
    Mge_SetDebugOutput(false);
    CHECK(Mge_GetDebugOutput() == false);
    Mge_SetDebugOutput(true);
    CHECK(Mge_GetDebugOutput() == true);
}

TEST(apply_registers_the_callback_only_when_enabled)
{
    Mge_SetDebugOutput(false);
    glstub_reset();
    glstub.debugCallback = NULL;
    Mge_ApplyDebugOutput();
    CHECK(glstub.debugCallback == NULL);            // disabled -> nothing
    CHECK(!glstub_is_enabled(GL_DEBUG_OUTPUT));

    Mge_SetDebugOutput(true);
    Mge_ApplyDebugOutput();
    CHECK(glstub.debugCallback != NULL);            // enabled -> callback set
    CHECK(glstub_is_enabled(GL_DEBUG_OUTPUT));
    CHECK(glstub_is_enabled(GL_DEBUG_OUTPUT_SYNCHRONOUS));
}

int main(void)
{
    RUN(default_follows_build_type);
    RUN(set_toggles_the_flag);
    RUN(apply_registers_the_callback_only_when_enabled);
    return test_summary();
}
