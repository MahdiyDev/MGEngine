#pragma once

// Small linear-algebra layer (replaces glm). Plain C11, no operator overloads.

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

// NOTE: stored column-major so MatrixToFloatV() feeds OpenGL directly.
typedef struct Matrix {
    float m0, m4, m8, m12;  // Matrix first column
    float m1, m5, m9, m13;  // Matrix second column
    float m2, m6, m10, m14; // Matrix third column
    float m3, m7, m11, m15; // Matrix fourth column
} Matrix;

float Clamp(float value, float min, float max);

Vector2 Vector2_Rotate(Vector2 v, float angle);

Vector3 Vector3_Add(Vector3 a, Vector3 b);
Vector3 Vector3_Subtract(Vector3 a, Vector3 b);
Vector3 Vector3_Scale(Vector3 v, float scalar);   // v * scalar
Vector3 Vector3_Multiply(Vector3 a, Vector3 b);   // component-wise
float   Vector3_DotProduct(Vector3 a, Vector3 b);
float   Vector3_Length(Vector3 v);
Vector3 Vector3Cross(Vector3 v1, Vector3 v2);
Vector3 Vector3Normalize(Vector3 v);

Vector4 Vector4_Transform(Vector4 v, Matrix mat); // column-major mat * v

Matrix Matrix_Identity(void);
Matrix Matrix_Multiply(Matrix left, Matrix right);
Matrix Matrix_Translate(float x, float y, float z);
Matrix Matrix_Rotate(Vector3 axis, float angle);
Matrix MatrixOrtho(double left, double right, double bottom, double top, double nearPlane, double farPlane);
Matrix MatrixPerspective(double fovY, double aspect, double nearPlane, double farPlane);
Matrix MatrixLookAt(Vector3 eye, Vector3 target, Vector3 up);
float16 MatrixToFloatV(Matrix mat);

#ifdef __cplusplus
}
#endif
