#include <math.h>
#include <stdbool.h>

#include "mge_math.h"
#include "test.h" // mlib repo-wide harness

static bool feq(float a, float b) { return fabsf(a - b) < 1e-4f; }
#define CHECK_F(a, b) CHECK(feq((a), (b)))

TEST(clamp)
{
    CHECK_F(Clamp(-5.0f, 0.0f, 10.0f), 0.0f);
    CHECK_F(Clamp(5.0f, 0.0f, 10.0f), 5.0f);
    CHECK_F(Clamp(50.0f, 0.0f, 10.0f), 10.0f);
}

TEST(vector3_arithmetic)
{
    Vector3 a = { 1.0f, 2.0f, 3.0f };
    Vector3 b = { 4.0f, 5.0f, 6.0f };

    Vector3 s = Vector3_Add(a, b);
    CHECK_F(s.x, 5.0f);
    CHECK_F(s.y, 7.0f);
    CHECK_F(s.z, 9.0f);

    Vector3 d = Vector3_Subtract(b, a);
    CHECK_F(d.x, 3.0f);
    CHECK_F(d.y, 3.0f);
    CHECK_F(d.z, 3.0f);

    Vector3 sc = Vector3_Scale(a, 2.0f);
    CHECK_F(sc.x, 2.0f);
    CHECK_F(sc.z, 6.0f);

    Vector3 m = Vector3_Multiply(a, b);
    CHECK_F(m.x, 4.0f);
    CHECK_F(m.y, 10.0f);
    CHECK_F(m.z, 18.0f);

    CHECK_F(Vector3_DotProduct(a, b), 32.0f);
    CHECK_F(Vector3_Length((Vector3){ 3.0f, 4.0f, 0.0f }), 5.0f);
}

TEST(vector3_cross_and_normalize)
{
    Vector3 x = { 1.0f, 0.0f, 0.0f };
    Vector3 y = { 0.0f, 1.0f, 0.0f };
    Vector3 z = Vector3Cross(x, y);
    CHECK_F(z.x, 0.0f);
    CHECK_F(z.y, 0.0f);
    CHECK_F(z.z, 1.0f);

    Vector3 n = Vector3Normalize((Vector3){ 0.0f, 3.0f, 4.0f });
    CHECK_F(Vector3_Length(n), 1.0f);
    CHECK_F(n.y, 0.6f);
    CHECK_F(n.z, 0.8f);

    Vector3 zero = Vector3Normalize((Vector3){ 0.0f, 0.0f, 0.0f });
    CHECK_F(zero.x, 0.0f); // no divide-by-zero
}

TEST(vector2_rotate)
{
    Vector2 r = Vector2_Rotate((Vector2){ 1.0f, 0.0f }, PI / 2.0f);
    CHECK_F(r.x, 0.0f);
    CHECK_F(r.y, 1.0f);
}

TEST(matrix_identity_and_multiply)
{
    Matrix id = Matrix_Identity();
    CHECK_F(id.m0, 1.0f);
    CHECK_F(id.m5, 1.0f);
    CHECK_F(id.m10, 1.0f);
    CHECK_F(id.m15, 1.0f);
    CHECK_F(id.m1, 0.0f);

    Matrix t = Matrix_Translate(2.0f, 3.0f, 4.0f);
    Matrix prod = Matrix_Multiply(id, t);
    float16 f = MatrixToFloatV(prod);
    // column-major: translation lives in v[12..14]
    CHECK_F(f.v[12], 2.0f);
    CHECK_F(f.v[13], 3.0f);
    CHECK_F(f.v[14], 4.0f);
    CHECK_F(f.v[0], 1.0f);
}

TEST(matrix_ortho)
{
    Matrix o = MatrixOrtho(0, 800, 600, 0, 0.0, 1.0);
    CHECK_F(o.m0, 2.0f / 800.0f);
    CHECK_F(o.m5, 2.0f / -600.0f);
    CHECK_F(o.m12, -1.0f); // -(0 + 800) / 800
    CHECK_F(o.m13, 1.0f);  // -(0 + 600) / -600
    CHECK_F(o.m15, 1.0f);
}

TEST(matrix_perspective)
{
    Matrix p = MatrixPerspective(60.0 * DEG2RAD, 16.0 / 9.0, 0.1, 100.0);
    CHECK_F(p.m11, -1.0f);
    CHECK(p.m0 > 0.0f);
    CHECK(p.m5 > 0.0f);
    CHECK(p.m10 < 0.0f);
}

TEST(matrix_look_at)
{
    // eye at origin, looking down -z, up +y  ->  rotation part is identity
    Matrix v = MatrixLookAt((Vector3){ 0, 0, 0 }, (Vector3){ 0, 0, -1 }, (Vector3){ 0, 1, 0 });
    CHECK_F(v.m0, 1.0f);
    CHECK_F(v.m5, 1.0f);
    CHECK_F(v.m10, 1.0f);
    CHECK_F(v.m15, 1.0f);

    // move the eye; translation column should reflect -dot(axis, eye)
    Matrix v2 = MatrixLookAt((Vector3){ 0, 0, 5 }, (Vector3){ 0, 0, 0 }, (Vector3){ 0, 1, 0 });
    CHECK_F(v2.m14, -5.0f);
}

int main(void)
{
    RUN(clamp);
    RUN(vector3_arithmetic);
    RUN(vector3_cross_and_normalize);
    RUN(vector2_rotate);
    RUN(matrix_identity_and_multiply);
    RUN(matrix_ortho);
    RUN(matrix_perspective);
    RUN(matrix_look_at);
    return test_summary();
}
