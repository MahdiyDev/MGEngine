// RenderTexture / PostFX definitions. The framebuffer code itself is raw GL
// (like the model uploader) and is only exercised with a live context.

#include "mge.h"
#include "test.h"

TEST(postfx_enum_is_contiguous)
{
    CHECK(POSTFX_NONE == 0);
    CHECK(POSTFX_INVERT == 1);
    CHECK(POSTFX_GRAYSCALE == 2);
    CHECK(POSTFX_SHARPEN == 3);
    CHECK(POSTFX_BLUR == 4);
    CHECK(POSTFX_EDGE == 5); // the shader branches on effect >= 3 for the kernels
}

TEST(tonemap_enum_is_contiguous)
{
    CHECK(TONEMAP_REINHARD == 0);
    CHECK(TONEMAP_EXPOSURE == 1);
    CHECK(TONEMAP_ACES == 2); // the shader branches: 1 = exposure, 2 = ACES, else Reinhard
}

TEST(render_texture_zero_value_is_inert)
{
    RenderTexture rt = { 0 };
    CHECK(rt.fbo == 0 && rt.texture.id == 0 && rt.depthStencil == 0);
    CHECK(rt.width == 0 && rt.height == 0);
    CHECK(!rt.hdr); // a plain RenderTexture is LDR (RGBA8)
    // Mge_UnloadRenderTexture on this must be a no-op (guards on the ids) --
    // exercised by the examples where a GL context exists.
}

int main(void)
{
    RUN(postfx_enum_is_contiguous);
    RUN(tonemap_enum_is_contiguous);
    RUN(render_texture_zero_value_is_inert);
    return test_summary();
}
