#include "mge_math.h"
#include <math.h>

float Clamp(float value, float min, float max)
{
    float result = (value < min) ? min : value;

    if (result > max)
        result = max;

    return result;
}

float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

Vector2 Vector2_Lerp(Vector2 a, Vector2 b, float t)
{
    return (Vector2){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

Vector2 Vector2_Rotate(Vector2 v, float angle)
{
    Vector2 result = { 0 };

    float cosres = cosf(angle);
    float sinres = sinf(angle);

    result.x = v.x * cosres - v.y * sinres;
    result.y = v.x * sinres + v.y * cosres;

    return result;
}

Vector3 Vector3_Add(Vector3 a, Vector3 b)
{
    return (Vector3){ a.x + b.x, a.y + b.y, a.z + b.z };
}

Vector3 Vector3_Subtract(Vector3 a, Vector3 b)
{
    return (Vector3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

Vector3 Vector3_Scale(Vector3 v, float scalar)
{
    return (Vector3){ v.x * scalar, v.y * scalar, v.z * scalar };
}

Vector3 Vector3_Multiply(Vector3 a, Vector3 b)
{
    return (Vector3){ a.x * b.x, a.y * b.y, a.z * b.z };
}

Vector3 Vector3_Lerp(Vector3 a, Vector3 b, float t)
{
    return (Vector3){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}

float Vector3_DotProduct(Vector3 a, Vector3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float Vector3_Length(Vector3 v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vector3 Vector3Cross(Vector3 v1, Vector3 v2)
{
    Vector3 result = {
        v1.y * v2.z - v1.z * v2.y,
        v1.z * v2.x - v1.x * v2.z,
        v1.x * v2.y - v1.y * v2.x
    };

    return result;
}

Vector3 Vector3Normalize(Vector3 v)
{
    Vector3 result = v;

    float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length != 0.0f) {
        float ilength = 1.0f / length;

        result.x *= ilength;
        result.y *= ilength;
        result.z *= ilength;
    }

    return result;
}

Vector4 Vector4_Transform(Vector4 v, Matrix mat)
{
    Vector4 r;
    r.x = mat.m0 * v.x + mat.m4 * v.y + mat.m8 * v.z + mat.m12 * v.w;
    r.y = mat.m1 * v.x + mat.m5 * v.y + mat.m9 * v.z + mat.m13 * v.w;
    r.z = mat.m2 * v.x + mat.m6 * v.y + mat.m10 * v.z + mat.m14 * v.w;
    r.w = mat.m3 * v.x + mat.m7 * v.y + mat.m11 * v.z + mat.m15 * v.w;
    return r;
}

Matrix Matrix_Identity(void)
{
    Matrix result = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    return result;
}

Matrix Matrix_Multiply(Matrix left, Matrix right)
{
    Matrix result = { 0 };

    result.m0 = left.m0 * right.m0 + left.m1 * right.m4 + left.m2 * right.m8 + left.m3 * right.m12;
    result.m1 = left.m0 * right.m1 + left.m1 * right.m5 + left.m2 * right.m9 + left.m3 * right.m13;
    result.m2 = left.m0 * right.m2 + left.m1 * right.m6 + left.m2 * right.m10 + left.m3 * right.m14;
    result.m3 = left.m0 * right.m3 + left.m1 * right.m7 + left.m2 * right.m11 + left.m3 * right.m15;
    result.m4 = left.m4 * right.m0 + left.m5 * right.m4 + left.m6 * right.m8 + left.m7 * right.m12;
    result.m5 = left.m4 * right.m1 + left.m5 * right.m5 + left.m6 * right.m9 + left.m7 * right.m13;
    result.m6 = left.m4 * right.m2 + left.m5 * right.m6 + left.m6 * right.m10 + left.m7 * right.m14;
    result.m7 = left.m4 * right.m3 + left.m5 * right.m7 + left.m6 * right.m11 + left.m7 * right.m15;
    result.m8 = left.m8 * right.m0 + left.m9 * right.m4 + left.m10 * right.m8 + left.m11 * right.m12;
    result.m9 = left.m8 * right.m1 + left.m9 * right.m5 + left.m10 * right.m9 + left.m11 * right.m13;
    result.m10 = left.m8 * right.m2 + left.m9 * right.m6 + left.m10 * right.m10 + left.m11 * right.m14;
    result.m11 = left.m8 * right.m3 + left.m9 * right.m7 + left.m10 * right.m11 + left.m11 * right.m15;
    result.m12 = left.m12 * right.m0 + left.m13 * right.m4 + left.m14 * right.m8 + left.m15 * right.m12;
    result.m13 = left.m12 * right.m1 + left.m13 * right.m5 + left.m14 * right.m9 + left.m15 * right.m13;
    result.m14 = left.m12 * right.m2 + left.m13 * right.m6 + left.m14 * right.m10 + left.m15 * right.m14;
    result.m15 = left.m12 * right.m3 + left.m13 * right.m7 + left.m14 * right.m11 + left.m15 * right.m15;

    return result;
}

Matrix Matrix_Translate(float x, float y, float z)
{
    Matrix result = {
        1.0f, 0.0f, 0.0f, x,
        0.0f, 1.0f, 0.0f, y,
        0.0f, 0.0f, 1.0f, z,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    return result;
}

Matrix Matrix_Scale(float x, float y, float z)
{
    Matrix result = {
        x,    0.0f, 0.0f, 0.0f,
        0.0f, y,    0.0f, 0.0f,
        0.0f, 0.0f, z,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    return result;
}

Matrix Matrix_RotateXYZ(Vector3 eulerRad)
{
    Matrix rx = Matrix_Rotate((Vector3){ 1.0f, 0.0f, 0.0f }, eulerRad.x);
    Matrix ry = Matrix_Rotate((Vector3){ 0.0f, 1.0f, 0.0f }, eulerRad.y);
    Matrix rz = Matrix_Rotate((Vector3){ 0.0f, 0.0f, 1.0f }, eulerRad.z);
    // Matrix_Multiply(A, B) == "apply A, then B"
    return Matrix_Multiply(rx, Matrix_Multiply(ry, rz));
}

Vector3 Matrix_ToEulerXYZ(Matrix m)
{
    // for R = Matrix_RotateXYZ({x,y,z}): m2 = -sin(y), and the rest follow
    Vector3 e;
    float sy = m.m2;
    sy = (sy > 1.0f) ? 1.0f : (sy < -1.0f) ? -1.0f : sy;
    e.y = asinf(-sy);

    if (fabsf(cosf(e.y)) > 1e-4f) {
        e.x = atan2f(m.m6, m.m10);
        e.z = atan2f(m.m1, m.m0);
    } else { // gimbal lock: pin z, fold everything into x
        e.x = atan2f(-m.m9, m.m5);
        e.z = 0.0f;
    }
    return e;
}

Vector3 Vector3_RotateAround(Vector3 p, Vector3 pivot, Matrix rot)
{
    Vector3 d = Vector3_Subtract(p, pivot);
    Vector4 r = Vector4_Transform((Vector4){ d.x, d.y, d.z, 0.0f }, rot);
    return (Vector3){ pivot.x + r.x, pivot.y + r.y, pivot.z + r.z };
}

Matrix Matrix_Rotate(Vector3 axis, float angle)
{
    Matrix result = { 0 };

    float x = axis.x, y = axis.y, z = axis.z;

    float lengthSquared = x * x + y * y + z * z;

    if ((lengthSquared != 1.0f) && (lengthSquared != 0.0f)) {
        float ilength = 1.0f / sqrtf(lengthSquared);
        x *= ilength;
        y *= ilength;
        z *= ilength;
    }

    float sinres = sinf(angle);
    float cosres = cosf(angle);
    float t = 1.0f - cosres;

    result.m0 = x * x * t + cosres;
    result.m1 = y * x * t + z * sinres;
    result.m2 = z * x * t - y * sinres;
    result.m3 = 0.0f;

    result.m4 = x * y * t - z * sinres;
    result.m5 = y * y * t + cosres;
    result.m6 = z * y * t + x * sinres;
    result.m7 = 0.0f;

    result.m8 = x * z * t + y * sinres;
    result.m9 = y * z * t - x * sinres;
    result.m10 = z * z * t + cosres;
    result.m11 = 0.0f;

    result.m12 = 0.0f;
    result.m13 = 0.0f;
    result.m14 = 0.0f;
    result.m15 = 1.0f;

    return result;
}

Matrix MatrixOrtho(double left, double right, double bottom, double top, double nearPlane, double farPlane)
{
    Matrix result = { 0 };

    float rl = (float)(right - left);
    float tb = (float)(top - bottom);
    float fn = (float)(farPlane - nearPlane);

    result.m0 = 2.0f / rl;
    result.m1 = 0.0f;
    result.m2 = 0.0f;
    result.m3 = 0.0f;
    result.m4 = 0.0f;
    result.m5 = 2.0f / tb;
    result.m6 = 0.0f;
    result.m7 = 0.0f;
    result.m8 = 0.0f;
    result.m9 = 0.0f;
    result.m10 = -2.0f / fn;
    result.m11 = 0.0f;
    result.m12 = -((float)left + (float)right) / rl;
    result.m13 = -((float)top + (float)bottom) / tb;
    result.m14 = -((float)farPlane + (float)nearPlane) / fn;
    result.m15 = 1.0f;

    return result;
}

Matrix MatrixPerspective(double fovY, double aspect, double nearPlane, double farPlane)
{
    Matrix result = { 0 };

    double top = nearPlane * tan(fovY * 0.5);
    double bottom = -top;
    double right = top * aspect;
    double left = -right;

    float rl = (float)(right - left);
    float tb = (float)(top - bottom);
    float fn = (float)(farPlane - nearPlane);

    result.m0 = ((float)nearPlane * 2.0f) / rl;
    result.m5 = ((float)nearPlane * 2.0f) / tb;
    result.m8 = ((float)right + (float)left) / rl;
    result.m9 = ((float)top + (float)bottom) / tb;
    result.m10 = -((float)farPlane + (float)nearPlane) / fn;
    result.m11 = -1.0f;
    result.m14 = -((float)farPlane * (float)nearPlane * 2.0f) / fn;

    return result;
}

float16 MatrixToFloatV(Matrix mat)
{
    float16 result = { 0 };

    result.v[0] = mat.m0;
    result.v[1] = mat.m1;
    result.v[2] = mat.m2;
    result.v[3] = mat.m3;
    result.v[4] = mat.m4;
    result.v[5] = mat.m5;
    result.v[6] = mat.m6;
    result.v[7] = mat.m7;
    result.v[8] = mat.m8;
    result.v[9] = mat.m9;
    result.v[10] = mat.m10;
    result.v[11] = mat.m11;
    result.v[12] = mat.m12;
    result.v[13] = mat.m13;
    result.v[14] = mat.m14;
    result.v[15] = mat.m15;

    return result;
}

Matrix MatrixLookAt(Vector3 eye, Vector3 target, Vector3 up)
{
    Matrix result = { 0 };

    float length = 0.0f;
    float ilength = 0.0f;

    // vz = normalize(eye - target)
    Vector3 vz = { eye.x - target.x, eye.y - target.y, eye.z - target.z };
    length = sqrtf(vz.x * vz.x + vz.y * vz.y + vz.z * vz.z);
    if (length == 0.0f)
        length = 1.0f;
    ilength = 1.0f / length;
    vz.x *= ilength;
    vz.y *= ilength;
    vz.z *= ilength;

    // vx = normalize(cross(up, vz))
    Vector3 vx = { up.y * vz.z - up.z * vz.y, up.z * vz.x - up.x * vz.z, up.x * vz.y - up.y * vz.x };
    length = sqrtf(vx.x * vx.x + vx.y * vx.y + vx.z * vx.z);
    if (length == 0.0f)
        length = 1.0f;
    ilength = 1.0f / length;
    vx.x *= ilength;
    vx.y *= ilength;
    vx.z *= ilength;

    // vy = cross(vz, vx)
    Vector3 vy = { vz.y * vx.z - vz.z * vx.y, vz.z * vx.x - vz.x * vx.z, vz.x * vx.y - vz.y * vx.x };

    result.m0 = vx.x;
    result.m1 = vy.x;
    result.m2 = vz.x;
    result.m3 = 0.0f;
    result.m4 = vx.y;
    result.m5 = vy.y;
    result.m6 = vz.y;
    result.m7 = 0.0f;
    result.m8 = vx.z;
    result.m9 = vy.z;
    result.m10 = vz.z;
    result.m11 = 0.0f;
    result.m12 = -(vx.x * eye.x + vx.y * eye.y + vx.z * eye.z); // dot(vx, eye)
    result.m13 = -(vy.x * eye.x + vy.y * eye.y + vy.z * eye.z); // dot(vy, eye)
    result.m14 = -(vz.x * eye.x + vz.y * eye.y + vz.z * eye.z); // dot(vz, eye)
    result.m15 = 1.0f;

    return result;
}

// ---------------------------------------------------------------- Quaternions

Quaternion Quaternion_Identity(void)
{
    return (Quaternion){ 0.0f, 0.0f, 0.0f, 1.0f };
}

Quaternion Quaternion_Normalize(Quaternion q)
{
    float len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len < 1e-8f)
        return Quaternion_Identity();
    float il = 1.0f / len;
    return (Quaternion){ q.x * il, q.y * il, q.z * il, q.w * il };
}

Quaternion Quaternion_Conjugate(Quaternion q)
{
    return (Quaternion){ -q.x, -q.y, -q.z, q.w };
}

// Hamilton product p (x) q -- rotates by q first, then p.
static Quaternion hamilton(Quaternion p, Quaternion q)
{
    return (Quaternion){
        p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y,
        p.w * q.y - p.x * q.z + p.y * q.w + p.z * q.x,
        p.w * q.z + p.x * q.y - p.y * q.x + p.z * q.w,
        p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z,
    };
}

Quaternion Quaternion_Multiply(Quaternion a, Quaternion b)
{
    return hamilton(b, a); // "apply a, then b"
}

Quaternion Quaternion_FromAxisAngle(Vector3 axis, float angleRad)
{
    Vector3 a = Vector3Normalize(axis);
    float s = sinf(angleRad * 0.5f);
    return (Quaternion){ a.x * s, a.y * s, a.z * s, cosf(angleRad * 0.5f) };
}

Matrix Quaternion_ToMatrix(Quaternion q)
{
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;

    Matrix m = { 0 };
    m.m0 = 1.0f - 2.0f * (yy + zz);
    m.m4 = 2.0f * (xy - wz);
    m.m8 = 2.0f * (xz + wy);
    m.m1 = 2.0f * (xy + wz);
    m.m5 = 1.0f - 2.0f * (xx + zz);
    m.m9 = 2.0f * (yz - wx);
    m.m2 = 2.0f * (xz - wy);
    m.m6 = 2.0f * (yz + wx);
    m.m10 = 1.0f - 2.0f * (xx + yy);
    m.m15 = 1.0f;
    return m;
}

Quaternion Quaternion_FromMatrix(Matrix m)
{
    // Shepperd's method. Engine layout: R[row][col] -> m0 m4 m8 / m1 m5 m9 / m2 m6 m10.
    float trace = m.m0 + m.m5 + m.m10;
    Quaternion q;
    if (trace > 0.0f) {
        float s = sqrtf(trace + 1.0f) * 2.0f;
        q.w = 0.25f * s;
        q.x = (m.m6 - m.m9) / s;
        q.y = (m.m8 - m.m2) / s;
        q.z = (m.m1 - m.m4) / s;
    } else if (m.m0 > m.m5 && m.m0 > m.m10) {
        float s = sqrtf(1.0f + m.m0 - m.m5 - m.m10) * 2.0f;
        q.w = (m.m6 - m.m9) / s;
        q.x = 0.25f * s;
        q.y = (m.m4 + m.m1) / s;
        q.z = (m.m8 + m.m2) / s;
    } else if (m.m5 > m.m10) {
        float s = sqrtf(1.0f + m.m5 - m.m0 - m.m10) * 2.0f;
        q.w = (m.m8 - m.m2) / s;
        q.x = (m.m4 + m.m1) / s;
        q.y = 0.25f * s;
        q.z = (m.m9 + m.m6) / s;
    } else {
        float s = sqrtf(1.0f + m.m10 - m.m0 - m.m5) * 2.0f;
        q.w = (m.m1 - m.m4) / s;
        q.x = (m.m8 + m.m2) / s;
        q.y = (m.m9 + m.m6) / s;
        q.z = 0.25f * s;
    }
    return Quaternion_Normalize(q);
}

Quaternion Quaternion_FromEuler(Vector3 eulerRad)
{
    return Quaternion_FromMatrix(Matrix_RotateXYZ(eulerRad)); // consistent with the matrix path
}

Vector3 Quaternion_ToEuler(Quaternion q)
{
    return Matrix_ToEulerXYZ(Quaternion_ToMatrix(q));
}

Vector3 Quaternion_RotateVector3(Quaternion q, Vector3 v)
{
    Vector3 u = { q.x, q.y, q.z };
    Vector3 t = Vector3_Scale(Vector3Cross(u, v), 2.0f);
    Vector3 r = Vector3_Add(v, Vector3_Scale(t, q.w));
    return Vector3_Add(r, Vector3Cross(u, t));
}

Quaternion Quaternion_Slerp(Quaternion a, Quaternion b, float t)
{
    float d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (d < 0.0f) {
        b = (Quaternion){ -b.x, -b.y, -b.z, -b.w };
        d = -d;
    }
    if (d > 0.9995f) { // nearly parallel -> lerp
        Quaternion r = { a.x + t * (b.x - a.x), a.y + t * (b.y - a.y),
            a.z + t * (b.z - a.z), a.w + t * (b.w - a.w) };
        return Quaternion_Normalize(r);
    }
    float th0 = acosf(d);
    float th = th0 * t;
    float s0 = cosf(th) - d * sinf(th) / sinf(th0);
    float s1 = sinf(th) / sinf(th0);
    return (Quaternion){ a.x * s0 + b.x * s1, a.y * s0 + b.y * s1,
        a.z * s0 + b.z * s1, a.w * s0 + b.w * s1 };
}

Quaternion Quaternion_LookRotation(Vector3 forward, Vector3 up)
{
    Vector3 f = Vector3Normalize(forward);
    Vector3 zAxis = Vector3_Scale(f, -1.0f);           // camera looks down local -Z
    Vector3 xAxis = Vector3Normalize(Vector3Cross(up, zAxis));
    Vector3 yAxis = Vector3Cross(zAxis, xAxis);

    Matrix r = { 0 };
    r.m0 = xAxis.x; r.m4 = yAxis.x; r.m8 = zAxis.x;
    r.m1 = xAxis.y; r.m5 = yAxis.y; r.m9 = zAxis.y;
    r.m2 = xAxis.z; r.m6 = yAxis.z; r.m10 = zAxis.z;
    r.m15 = 1.0f;
    return Quaternion_FromMatrix(r);
}

bool Quaternion_Approx(Quaternion a, Quaternion b)
{
    float d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    return fabsf(d) > 0.9999f;
}
