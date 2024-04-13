#pragma once

#ifndef PI
    #define PI 3.14159265358979323846f
#endif
#ifndef DEG2RAD
    #define DEG2RAD (PI/180.0f)
#endif
#ifndef RAD2DEG
    #define RAD2DEG (180.0f/PI)
#endif

typedef struct Vector2 { float x; float y; } Vector2;
typedef struct Vector3 { float x; float y; float z; } Vector3;
typedef struct Vector4 { float x; float y; float z; float w; } Vector4;
typedef struct Matrix {
    float m0, m4, m8, m12;      // Matrix first row (4 components)
    float m1, m5, m9, m13;      // Matrix second row (4 components)
    float m2, m6, m10, m14;     // Matrix third row (4 components)
    float m3, m7, m11, m15;     // Matrix fourth row (4 components)
} Matrix;

Vector2 Vector2_Rotate(Vector2 v, float angle);
Matrix Matrix_Identity(void);
Matrix Matrix_Multiply(Matrix left, Matrix right);
Matrix Matrix_Translate(float x, float y, float z);
Matrix Matrix_Rotate(Vector3 axis, float angle);
Matrix MatrixOrtho(double left, double right, double bottom, double top, double nearPlane, double farPlane);
Matrix MatrixPerspective(double fovY, double aspect, double nearPlane, double farPlane);
