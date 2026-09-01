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

TEST(lerp)
{
    CHECK_F(Lerp(0.0f, 10.0f, 0.0f), 0.0f);
    CHECK_F(Lerp(0.0f, 10.0f, 1.0f), 10.0f);
    CHECK_F(Lerp(2.0f, 4.0f, 0.5f), 3.0f);
    CHECK_F(Lerp(0.0f, 10.0f, 1.5f), 15.0f); // unclamped

    Vector2 v2 = Vector2_Lerp((Vector2){ 0, 0 }, (Vector2){ 4, 8 }, 0.25f);
    CHECK_F(v2.x, 1.0f);
    CHECK_F(v2.y, 2.0f);

    Vector3 v3 = Vector3_Lerp((Vector3){ 1, 2, 3 }, (Vector3){ 3, 6, 9 }, 0.5f);
    CHECK_F(v3.x, 2.0f);
    CHECK_F(v3.y, 4.0f);
    CHECK_F(v3.z, 6.0f);
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

static bool v3eq(Vector3 a, Vector3 b)
{
    return feq(a.x, b.x) && feq(a.y, b.y) && feq(a.z, b.z);
}
static bool mat_rot_eq(Matrix a, Matrix b)
{
    return feq(a.m0, b.m0) && feq(a.m4, b.m4) && feq(a.m8, b.m8) &&
        feq(a.m1, b.m1) && feq(a.m5, b.m5) && feq(a.m9, b.m9) &&
        feq(a.m2, b.m2) && feq(a.m6, b.m6) && feq(a.m10, b.m10);
}

TEST(quaternion_basics)
{
    Quaternion id = Quaternion_Identity();
    CHECK_F(id.w, 1.0f);
    CHECK(v3eq(Quaternion_RotateVector3(id, (Vector3){ 3, -2, 5 }), (Vector3){ 3, -2, 5 }));

    // axis-angle: 90deg about +Y sends +X -> -Z
    Quaternion qy = Quaternion_FromAxisAngle((Vector3){ 0, 1, 0 }, (float)PI / 2.0f);
    CHECK(v3eq(Quaternion_RotateVector3(qy, (Vector3){ 1, 0, 0 }), (Vector3){ 0, 0, -1 }));

    // rotating a vector by q must match rotating it by q's matrix
    Quaternion q = Quaternion_FromEuler((Vector3){ 0.3f, -0.7f, 1.1f });
    Matrix m = Quaternion_ToMatrix(q);
    Vector3 v = { 1.0f, 2.0f, -3.0f };
    Vector4 mv = Vector4_Transform((Vector4){ v.x, v.y, v.z, 0.0f }, m);
    CHECK(v3eq(Quaternion_RotateVector3(q, v), (Vector3){ mv.x, mv.y, mv.z }));
}

TEST(quaternion_matches_matrix_path)
{
    Vector3 angles[4] = {
        { 0.4f, 0.0f, 0.0f }, { 0.0f, 1.2f, 0.0f },
        { 0.5f, -0.9f, 0.3f }, { -1.4f, 0.7f, -0.2f },
    };
    for (int i = 0; i < 4; i++) {
        // Quaternion_FromEuler <-> Matrix_RotateXYZ agree
        CHECK(mat_rot_eq(Quaternion_ToMatrix(Quaternion_FromEuler(angles[i])),
            Matrix_RotateXYZ(angles[i])));
        // euler round-trip
        Quaternion q = Quaternion_FromEuler(angles[i]);
        CHECK(Quaternion_Approx(Quaternion_FromEuler(Quaternion_ToEuler(q)), q));
        // matrix round-trip
        CHECK(Quaternion_Approx(Quaternion_FromMatrix(Quaternion_ToMatrix(q)), q));
    }

    // compose: "apply a then b" == Matrix_Multiply(A, B)
    Quaternion a = Quaternion_FromEuler((Vector3){ 0.3f, 0.0f, 0.0f });
    Quaternion b = Quaternion_FromEuler((Vector3){ 0.0f, 0.8f, 0.0f });
    CHECK(mat_rot_eq(Quaternion_ToMatrix(Quaternion_Multiply(a, b)),
        Matrix_Multiply(Quaternion_ToMatrix(a), Quaternion_ToMatrix(b))));
}

TEST(quaternion_look_and_slerp)
{
    // identity look: local -Z is the forward
    Quaternion look = Quaternion_LookRotation((Vector3){ 0, 0, -1 }, (Vector3){ 0, 1, 0 });
    CHECK(Quaternion_Approx(look, Quaternion_Identity()));

    Quaternion l2 = Quaternion_LookRotation((Vector3){ 1, 0, 0 }, (Vector3){ 0, 1, 0 });
    CHECK(v3eq(Quaternion_RotateVector3(l2, (Vector3){ 0, 0, -1 }), (Vector3){ 1, 0, 0 }));

    Quaternion s0 = Quaternion_Slerp(Quaternion_Identity(), l2, 0.0f);
    Quaternion s1 = Quaternion_Slerp(Quaternion_Identity(), l2, 1.0f);
    CHECK(Quaternion_Approx(s0, Quaternion_Identity()));
    CHECK(Quaternion_Approx(s1, l2));
}

int main(void)
{
    RUN(clamp);
    RUN(lerp);
    RUN(vector3_arithmetic);
    RUN(vector3_cross_and_normalize);
    RUN(vector2_rotate);
    RUN(matrix_identity_and_multiply);
    RUN(matrix_ortho);
    RUN(matrix_perspective);
    RUN(matrix_look_at);
    RUN(quaternion_basics);
    RUN(quaternion_matches_matrix_path);
    RUN(quaternion_look_and_slerp);
    return test_summary();
}
