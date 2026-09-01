#pragma once

// Small linear-algebra layer (replaces glm). Plain C11, no operator overloads.

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PI
    #define PI 3.14159265358979323846f
#endif
#ifndef DEG2RAD
    #define DEG2RAD (PI / 180.0f)
#endif
#ifndef RAD2DEG
    #define RAD2DEG (180.0f / PI)
#endif

typedef struct float16 {
    float v[16];
} float16;

#ifndef MatrixToFloat
    #define MatrixToFloat(mat) (MatrixToFloatV(mat).v)
#endif

typedef struct Vector2 {
    float x;
    float y;
} Vector2;

typedef struct Vector3 {
    float x;
    float y;
    float z;
} Vector3;

typedef struct Vector4 {
    float x;
    float y;
    float z;
    float w;
} Vector4;

// Orientation. {0,0,0,1} = identity; keep it unit-length. See the Quaternion_*
// helpers below. Multiplication / Euler conversion match Matrix_Multiply /
// Matrix_RotateXYZ so the two paths agree.
typedef struct Quaternion {
    float x;
    float y;
    float z;
    float w;
} Quaternion;

// NOTE: stored column-major so MatrixToFloatV() feeds OpenGL directly.
typedef struct Matrix {
    float m0, m4, m8, m12;  // Matrix first column
    float m1, m5, m9, m13;  // Matrix second column
    float m2, m6, m10, m14; // Matrix third column
    float m3, m7, m11, m15; // Matrix fourth column
} Matrix;

float Clamp(float value, float min, float max);
float Lerp(float a, float b, float t);            // a + (b - a) * t  (t is not clamped)

Vector2 Vector2_Rotate(Vector2 v, float angle);
Vector2 Vector2_Lerp(Vector2 a, Vector2 b, float t);

Vector3 Vector3_Add(Vector3 a, Vector3 b);
Vector3 Vector3_Subtract(Vector3 a, Vector3 b);
Vector3 Vector3_Scale(Vector3 v, float scalar);   // v * scalar
Vector3 Vector3_Multiply(Vector3 a, Vector3 b);   // component-wise
float   Vector3_DotProduct(Vector3 a, Vector3 b);
float   Vector3_Length(Vector3 v);
Vector3 Vector3Cross(Vector3 v1, Vector3 v2);
Vector3 Vector3Normalize(Vector3 v);
Vector3 Vector3_Lerp(Vector3 a, Vector3 b, float t); // per-component a + (b - a) * t

Vector4 Vector4_Transform(Vector4 v, Matrix mat); // column-major mat * v
// rotate `p` about `pivot` by `rot` (a Matrix_Rotate / Matrix_RotateXYZ result)
Vector3 Vector3_RotateAround(Vector3 p, Vector3 pivot, Matrix rot);

Matrix Matrix_Identity(void);
Matrix Matrix_Multiply(Matrix left, Matrix right);
Matrix Matrix_Translate(float x, float y, float z);
Matrix Matrix_Scale(float x, float y, float z);
Matrix Matrix_Rotate(Vector3 axis, float angle);
Matrix Matrix_RotateXYZ(Vector3 eulerRad); // apply X, then Y, then Z
Vector3 Matrix_ToEulerXYZ(Matrix m);       // inverse of Matrix_RotateXYZ (radians)

// --- Quaternions ---
Quaternion Quaternion_Identity(void);
Quaternion Quaternion_Normalize(Quaternion q);
Quaternion Quaternion_Conjugate(Quaternion q);              // inverse for a unit quaternion
// Compose: the result applies rotation `a` first, then `b` (matches Matrix_Multiply).
Quaternion Quaternion_Multiply(Quaternion a, Quaternion b);
Quaternion Quaternion_FromAxisAngle(Vector3 axis, float angleRad);
Quaternion Quaternion_FromEuler(Vector3 eulerRad);          // XYZ order, == Matrix_RotateXYZ
Vector3    Quaternion_ToEuler(Quaternion q);                // inverse of Quaternion_FromEuler (radians)
Matrix     Quaternion_ToMatrix(Quaternion q);
Quaternion Quaternion_FromMatrix(Matrix m);                 // rotation part only
Vector3    Quaternion_RotateVector3(Quaternion q, Vector3 v);
Quaternion Quaternion_Slerp(Quaternion a, Quaternion b, float t);
// Orientation whose local -Z looks along `forward` and local +Y is near `up`.
Quaternion Quaternion_LookRotation(Vector3 forward, Vector3 up);
bool       Quaternion_Approx(Quaternion a, Quaternion b);   // same rotation (q and -q equal)
Matrix MatrixOrtho(double left, double right, double bottom, double top, double nearPlane, double farPlane);
Matrix MatrixPerspective(double fovY, double aspect, double nearPlane, double farPlane);
Matrix MatrixLookAt(Vector3 eye, Vector3 target, Vector3 up);
float16 MatrixToFloatV(Matrix mat);

#ifdef __cplusplus
}
#endif
