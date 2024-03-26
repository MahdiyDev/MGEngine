#pragma once

typedef struct Vector2 { float x; float y; } Vector2;
typedef struct Vector3 { float x; float y; float z; } Vector3;
typedef struct Matrix {
    float m0, m4, m8, m12;      // Matrix first row (4 components)
    float m1, m5, m9, m13;      // Matrix second row (4 components)
    float m2, m6, m10, m14;     // Matrix third row (4 components)
    float m3, m7, m11, m15;     // Matrix fourth row (4 components)
} Matrix;

Vector2 Vector2_Rotate(Vector2 v, float angle);
Matrix Matrix_Multiply(Matrix left, Matrix right);
