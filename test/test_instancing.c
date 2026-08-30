// ModelBatch definitions + the CPU-side matrix packing. The instanced draw path
// (mge_instancing.c) is raw GL, exercised only with a live context by the
// examples; here we check the struct contract and Matrix_Scale.

#include "mge.h"
#include "mge_math.h"
#include "test.h"

TEST(model_batch_zero_value_is_inert)
{
    ModelBatch b = { 0 };
    CHECK(b.instanceVBO == 0);
    CHECK(b.count == 0);
    CHECK(b.model.meshCount == 0);
    // Mge_UnloadModelBatch(&b) and Mge_DrawModelBatch(b, ...) must be no-ops
    // (they guard on instanceVBO / count) -- checked by the examples.
}

TEST(matrix_scale_builds_a_diagonal)
{
    Matrix m = Matrix_Scale(2.0f, 3.0f, 4.0f);
    CHECK(m.m0 == 2.0f);
    CHECK(m.m5 == 3.0f);
    CHECK(m.m10 == 4.0f);
    CHECK(m.m15 == 1.0f);
    CHECK(m.m12 == 0.0f && m.m13 == 0.0f && m.m14 == 0.0f);
}

TEST(scale_then_translate_composes_in_call_order)
{
    // Matrix_Multiply(A, B) applies A first, then B -> S then T
    Matrix xf = Matrix_Multiply(Matrix_Scale(10.0f, 10.0f, 10.0f),
                                Matrix_Translate(5.0f, 0.0f, 0.0f));
    // a unit-x point ends at 10*1 + 5 = 15
    Vector4 p = Vector4_Transform((Vector4){ 1.0f, 0.0f, 0.0f, 1.0f }, xf);
    CHECK(p.x > 14.99f && p.x < 15.01f);
}

int main(void)
{
    RUN(model_batch_zero_value_is_inert);
    RUN(matrix_scale_builds_a_diagonal);
    RUN(scale_then_translate_composes_in_call_order);
    return test_summary();
}
