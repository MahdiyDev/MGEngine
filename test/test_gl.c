// Unit tests for the renderer backend (source/mge_gl.c) with NO GL context:
// mge_gl.c is compiled against test/glstub (a fake <glad/glad.h>) and linked
// against test/glstub/glstub.c, which records every GL call for inspection.
//
// This covers the parts of the batcher that are pure CPU logic -- the matrix
// stack, draw-call merging, vertex accumulation, the buffer ring, the draw-call
// counter, and every engine-enum -> GL-enum mapping in the state setters.

#include <math.h>
#include <stdbool.h>

#include "mge_gl.h"
#include "glstub.h"
#include "test.h"

static bool feq(float a, float b) { return fabsf(a - b) < 1e-4f; }
static bool is_identity(Matrix m)
{
    return feq(m.m0, 1) && feq(m.m5, 1) && feq(m.m10, 1) && feq(m.m15, 1) &&
           feq(m.m1, 0) && feq(m.m2, 0) && feq(m.m3, 0) && feq(m.m4, 0) &&
           feq(m.m6, 0) && feq(m.m7, 0) && feq(m.m8, 0) && feq(m.m9, 0) &&
           feq(m.m11, 0) && feq(m.m12, 0) && feq(m.m13, 0) && feq(m.m14, 0);
}

// queue one triangle through the immediate-mode API
static void queue_triangle(void)
{
    MgeGL_Begin(MGEGL_TRIANGLES);
    MgeGL_Vertex3f(0.0f, 0.0f, 0.0f);
    MgeGL_Vertex3f(1.0f, 0.0f, 0.0f);
    MgeGL_Vertex3f(0.0f, 1.0f, 0.0f);
    MgeGL_End();
}

// ---- matrix stack ----

TEST(matrix_mode_load_identity)
{
    MgeGL_MatrixMode(MGEGL_PROJECTION);
    MgeGL_LoadIdentity();
    CHECK(is_identity(MgeGL_GetMatrixProjection()));

    MgeGL_MatrixMode(MGEGL_MODELVIEW);
    MgeGL_LoadIdentity();
    CHECK(is_identity(MgeGL_GetMatrixModelview()));
}

TEST(translatef_writes_the_translation_column)
{
    MgeGL_MatrixMode(MGEGL_MODELVIEW);
    MgeGL_LoadIdentity();
    MgeGL_Translatef(2.0f, 3.0f, 4.0f);

    Matrix m = MgeGL_GetMatrixModelview();
    CHECK(feq(m.m12, 2.0f));
    CHECK(feq(m.m13, 3.0f));
    CHECK(feq(m.m14, 4.0f));
    CHECK(feq(m.m0, 1.0f)); // rotation part untouched
}

TEST(ortho_builds_the_expected_projection)
{
    MgeGL_MatrixMode(MGEGL_PROJECTION);
    MgeGL_LoadIdentity();
    MgeGL_Ortho(-10.0, 10.0, -5.0, 5.0, 0.0, 100.0);

    Matrix o = MgeGL_GetMatrixProjection();
    CHECK(feq(o.m0, 2.0f / 20.0f));
    CHECK(feq(o.m5, 2.0f / 10.0f));
    CHECK(feq(o.m10, -2.0f / 100.0f));
    CHECK(feq(o.m15, 1.0f));
}

TEST(push_matrix_then_pop_restores_projection)
{
    MgeGL_MatrixMode(MGEGL_PROJECTION);
    MgeGL_LoadIdentity();

    MgeGL_PushMatrix();
    MgeGL_Ortho(0.0, 800.0, 600.0, 0.0, -1.0, 1.0);
    CHECK(feq(MgeGL_GetMatrixProjection().m0, 2.0f / 800.0f)); // ortho is live

    MgeGL_PopMatrix();
    CHECK(is_identity(MgeGL_GetMatrixProjection()));           // back to what was pushed
}

// ---- draw-call batching ----

TEST(same_mode_primitives_merge_into_one_draw)
{
    glstub_reset();
    queue_triangle();
    queue_triangle(); // second Begin(TRIANGLES) -> merged, not a new draw call
    MgeGL_Draw();

    CHECK(glstub.drawArraysCount == 1);
    CHECK(glstub.drawArrays[0].mode == MGEGL_TRIANGLES);
    CHECK(glstub.drawArrays[0].count == 6);
}

TEST(lines_and_triangles_become_two_draws_with_aligned_offsets)
{
    glstub_reset();
    MgeGL_Begin(MGEGL_LINES);
    MgeGL_Vertex3f(0, 0, 0);
    MgeGL_Vertex3f(1, 0, 0);
    MgeGL_End();
    MgeGL_Begin(MGEGL_TRIANGLES);
    MgeGL_Vertex3f(0, 0, 0);
    MgeGL_Vertex3f(1, 0, 0);
    MgeGL_Vertex3f(0, 1, 0);
    MgeGL_End();
    MgeGL_Draw();

    CHECK(glstub.drawArraysCount == 2);
    CHECK(glstub.drawArrays[0].mode == MGEGL_LINES);
    CHECK(glstub.drawArrays[0].count == 2);
    CHECK(glstub.drawArrays[0].first == 0);
    CHECK(glstub.drawArrays[1].mode == MGEGL_TRIANGLES);
    CHECK(glstub.drawArrays[1].count == 3);
    CHECK(glstub.drawArrays[1].first == 4); // 2 line verts + 2 alignment
}

TEST(vertex_count_flows_into_the_buffer_upload)
{
    glstub_reset();
    MgeGL_Begin(MGEGL_TRIANGLES);
    for (int i = 0; i < 9; i++)
        MgeGL_Vertex3f(0, 0, 0);
    MgeGL_End();
    MgeGL_Draw();

    CHECK(glstub.bufferSubDataCount >= 4); // pos / colour / uv / normal
    CHECK(glstub.bufferSubData[0] == (GLsizeiptr)(9 * 3 * sizeof(float)));
    CHECK(glstub.bufferSubData[1] == (GLsizeiptr)(9 * 4 * sizeof(unsigned char)));
}

TEST(draw_with_nothing_queued_is_a_noop)
{
    queue_triangle();
    MgeGL_Draw(); // drain
    glstub_reset();
    MgeGL_Draw();
    CHECK(glstub.drawArraysCount == 0);
    CHECK(glstub.bufferSubDataCount == 0);
}

TEST(flushes_cycle_through_the_buffer_ring)
{
    GLuint seen[4];
    for (int k = 0; k < 4; k++) {
        queue_triangle();
        glstub_reset();
        MgeGL_Draw();
        seen[k] = glstub.vaoBinds[0]; // the VAO bound at the top of this flush
    }
    CHECK(seen[0] != seen[1]);
    CHECK(seen[1] != seen[2]);
    CHECK(seen[0] != seen[2]);
    CHECK(seen[0] == seen[3]); // wrapped after MGEGL_BATCH_BUFFERS (3)
}

TEST(draw_call_counter_totals_the_frame)
{
    MgeGL_ResetDrawCalls();
    MgeGL_Begin(MGEGL_LINES);
    MgeGL_Vertex3f(0, 0, 0);
    MgeGL_Vertex3f(1, 0, 0);
    MgeGL_End();
    queue_triangle();
    MgeGL_Draw();              // 2 sub-draws
    MgeGL_RegisterDrawCall();  // + a feature-module draw
    MgeGL_ResetDrawCalls();    // snapshot -> "last frame"
    CHECK(MgeGL_GetDrawCalls() == 3);
}

TEST(set_shader_flushes_then_binds)
{
    unsigned int def = MgeGL_GetDefaultShaderId();
    glstub_reset();
    queue_triangle();
    MgeGL_SetShader(4242);

    CHECK(glstub.drawArraysCount == 1);  // the queued triangle was flushed first
    CHECK(glstub.usedProgram == 4242);
    CHECK(MgeGL_GetCurrentShaderId() == 4242);

    MgeGL_SetShader(def);
}

// ---- state setters: engine enum -> GL enum ----

TEST(depth_state_forwarding)
{
    MgeGL_EnableDepthTest();
    CHECK(glstub_is_enabled(GL_DEPTH_TEST));
    CHECK(MgeGL_IsDepthTestEnabled());

    MgeGL_SetDepthFunc(DEPTH_LEQUAL);
    CHECK(glstub.depthFunc == GL_LEQUAL);
    MgeGL_SetDepthFunc(DEPTH_ALWAYS);
    CHECK(glstub.depthFunc == GL_ALWAYS);

    MgeGL_SetDepthMask(false);
    CHECK(glstub.depthMask == GL_FALSE);
    MgeGL_SetDepthMask(true);
    CHECK(glstub.depthMask == GL_TRUE);

    MgeGL_SetPolygonOffset(true, 1.5f, 2.0f);
    CHECK(glstub_is_enabled(GL_POLYGON_OFFSET_FILL));
    CHECK(feq(glstub.polygonOffset.factor, 1.5f));
    CHECK(feq(glstub.polygonOffset.units, 2.0f));
    MgeGL_SetPolygonOffset(false, 0.0f, 0.0f);
    CHECK(!glstub_is_enabled(GL_POLYGON_OFFSET_FILL));

    MgeGL_DisableDepthTest();
    CHECK(!glstub_is_enabled(GL_DEPTH_TEST));
}

TEST(stencil_state_forwarding)
{
    MgeGL_EnableStencilTest();
    CHECK(glstub_is_enabled(GL_STENCIL_TEST));

    MgeGL_SetStencilFunc(STENCIL_NOTEQUAL, 1, 0xFF);
    CHECK(glstub.stencilFunc.func == GL_NOTEQUAL);
    CHECK(glstub.stencilFunc.ref == 1);
    CHECK(glstub.stencilFunc.mask == 0xFF);

    MgeGL_SetStencilOp(STENCIL_KEEP, STENCIL_KEEP, STENCIL_REPLACE);
    CHECK(glstub.stencilOp.sfail == GL_KEEP);
    CHECK(glstub.stencilOp.dppass == GL_REPLACE);

    MgeGL_SetStencilMask(0x00);
    CHECK(glstub.stencilMask == 0x00);

    MgeGL_SetColorMask(false);
    CHECK(glstub.colorMask == GL_FALSE);
    MgeGL_SetColorMask(true);
    CHECK(glstub.colorMask == GL_TRUE);

    MgeGL_DisableStencilTest();
    CHECK(!glstub_is_enabled(GL_STENCIL_TEST));
}

TEST(cull_state_forwarding)
{
    MgeGL_SetFaceCulling(true);
    CHECK(glstub_is_enabled(GL_CULL_FACE));

    MgeGL_SetCullFace(CULL_FRONT);
    CHECK(glstub.cullFace == GL_FRONT);
    MgeGL_SetCullFace(CULL_BACK);
    CHECK(glstub.cullFace == GL_BACK);

    MgeGL_SetFrontFace(WINDING_CW);
    CHECK(glstub.frontFace == GL_CW);
    MgeGL_SetFrontFace(WINDING_CCW);
    CHECK(glstub.frontFace == GL_CCW);

    MgeGL_SetFaceCulling(false);
    CHECK(!glstub_is_enabled(GL_CULL_FACE));
}

TEST(srgb_and_sample_count)
{
    MgeGL_SetFramebufferSRGB(true);
    CHECK(glstub_is_enabled(GL_FRAMEBUFFER_SRGB));
    MgeGL_SetFramebufferSRGB(false);
    CHECK(!glstub_is_enabled(GL_FRAMEBUFFER_SRGB));

    glstub.sampleCount = 4;
    CHECK(MgeGL_GetSampleCount() == 4);
    glstub.sampleCount = 0;
}

// ---- textures ----

TEST(load_texture_picks_the_sRGB_internal_format)
{
    unsigned char px[4] = { 0, 0, 0, 0 };

    MgeGL_LoadTexture(px, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1, 0);
    CHECK(glstub.texImage.internalFormat == GL_RGBA8);

    MgeGL_LoadTexture(px, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1, 1);
    CHECK(glstub.texImage.internalFormat == GL_SRGB8_ALPHA8);

    MgeGL_LoadTexture(px, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8, 1, 1);
    CHECK(glstub.texImage.internalFormat == GL_SRGB8);

    MgeGL_LoadTexture(px, 1, 1, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE, 1, 1);
    CHECK(glstub.texImage.internalFormat == GL_R8); // no sRGB form for 1-channel
}

TEST(set_texture_slot_targets_the_unit_then_restores)
{
    glstub_reset();
    MgeGL_SetTextureSlot(3, 77);
    CHECK(glstub.slotTexture[3] == 77);
    CHECK(glstub.activeTexture == GL_TEXTURE0); // left on unit 0 for the batcher
}

// ---- shaders ----

TEST(shader_and_program_creation)
{
    unsigned int v = MgeGL_LoadShader("void main(){}", GL_VERTEX_SHADER, "v");
    unsigned int f = MgeGL_LoadShader("void main(){}", GL_FRAGMENT_SHADER, "f");
    CHECK(v != 0 && f != 0 && v != f);
    CHECK(MgeGL_LoadShader(NULL, GL_VERTEX_SHADER, "bad") == 0);

    glstub_reset();
    unsigned int p = MgeGL_CreateShaderProgram(v, f);
    CHECK(p != 0);
    CHECK(glstub.attachCount == 2);

    glstub_reset();
    MgeGL_CreateShaderProgramGeo(1, 2, 3);
    CHECK(glstub.attachCount == 3); // vertex + geometry + fragment

    glstub_reset();
    MgeGL_CreateShaderProgramGeo(1, 0, 3);
    CHECK(glstub.attachCount == 2); // geometry id 0 -> skipped

    MgeGL_UnloadShaderProgram(p);
    CHECK(glstub.deletedProgram == p);
}

int main(void)
{
    MgeGL_Init(800, 600); // one batcher for the whole suite

    RUN(matrix_mode_load_identity);
    RUN(translatef_writes_the_translation_column);
    RUN(ortho_builds_the_expected_projection);
    RUN(push_matrix_then_pop_restores_projection);
    RUN(same_mode_primitives_merge_into_one_draw);
    RUN(lines_and_triangles_become_two_draws_with_aligned_offsets);
    RUN(vertex_count_flows_into_the_buffer_upload);
    RUN(draw_with_nothing_queued_is_a_noop);
    RUN(flushes_cycle_through_the_buffer_ring);
    RUN(draw_call_counter_totals_the_frame);
    RUN(set_shader_flushes_then_binds);
    RUN(depth_state_forwarding);
    RUN(stencil_state_forwarding);
    RUN(cull_state_forwarding);
    RUN(srgb_and_sample_count);
    RUN(load_texture_picks_the_sRGB_internal_format);
    RUN(set_texture_slot_targets_the_unit_then_restores);
    RUN(shader_and_program_creation);
    return test_summary();
}
