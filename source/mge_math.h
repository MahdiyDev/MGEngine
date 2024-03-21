#pragma once

#include <math.h>

typedef struct Vector2 { float x; float y; } Vector2;
typedef struct Vector3 { float x; float y; float z; } Vector3;

inline Vector2 Vector2_Rotate(Vector2 v, float angle)
{
	Vector2 result = { 0 };

    float cosres = cosf(angle);
    float sinres = sinf(angle);

    result.x = v.x*cosres - v.y*sinres;
    result.y = v.x*sinres + v.y*cosres;

    return result;
}
