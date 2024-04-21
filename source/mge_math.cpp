#include "mge_math.h"
#include <math.h>

Vector2 Vector2_Rotate(Vector2 v, float angle)
{
	Vector2 result = { 0 };

	float cosres = cosf(angle);
	float sinres = sinf(angle);

	result.x = v.x*cosres - v.y*sinres;
	result.y = v.x*sinres + v.y*cosres;

	return result;
}

Vector3 Vector3Subtract(Vector3 v1, Vector3 v2)
{
	Vector3 result = { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };

	return result;
}

Vector3 Vector3Add(Vector3 v1, Vector3 v2)
{
	Vector3 result = { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };

	return result;
}

Vector3 Vector3Normalize(Vector3 v)
{
	Vector3 result = v;

	float length = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
	if (length != 0.0f)
	{
		float ilength = 1.0f/length;

		result.x *= ilength;
		result.y *= ilength;
		result.z *= ilength;
	}

	return result;
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

	result.m0 = left.m0*right.m0 + left.m1*right.m4 + left.m2*right.m8 + left.m3*right.m12;
	result.m1 = left.m0*right.m1 + left.m1*right.m5 + left.m2*right.m9 + left.m3*right.m13;
	result.m2 = left.m0*right.m2 + left.m1*right.m6 + left.m2*right.m10 + left.m3*right.m14;
	result.m3 = left.m0*right.m3 + left.m1*right.m7 + left.m2*right.m11 + left.m3*right.m15;
	result.m4 = left.m4*right.m0 + left.m5*right.m4 + left.m6*right.m8 + left.m7*right.m12;
	result.m5 = left.m4*right.m1 + left.m5*right.m5 + left.m6*right.m9 + left.m7*right.m13;
	result.m6 = left.m4*right.m2 + left.m5*right.m6 + left.m6*right.m10 + left.m7*right.m14;
	result.m7 = left.m4*right.m3 + left.m5*right.m7 + left.m6*right.m11 + left.m7*right.m15;
	result.m8 = left.m8*right.m0 + left.m9*right.m4 + left.m10*right.m8 + left.m11*right.m12;
	result.m9 = left.m8*right.m1 + left.m9*right.m5 + left.m10*right.m9 + left.m11*right.m13;
	result.m10 = left.m8*right.m2 + left.m9*right.m6 + left.m10*right.m10 + left.m11*right.m14;
	result.m11 = left.m8*right.m3 + left.m9*right.m7 + left.m10*right.m11 + left.m11*right.m15;
	result.m12 = left.m12*right.m0 + left.m13*right.m4 + left.m14*right.m8 + left.m15*right.m12;
	result.m13 = left.m12*right.m1 + left.m13*right.m5 + left.m14*right.m9 + left.m15*right.m13;
	result.m14 = left.m12*right.m2 + left.m13*right.m6 + left.m14*right.m10 + left.m15*right.m14;
	result.m15 = left.m12*right.m3 + left.m13*right.m7 + left.m14*right.m11 + left.m15*right.m15;

	return result;
}

Matrix Matrix_Translate(float x, float y, float z)
{
	Matrix result = { 1.0f, 0.0f, 0.0f, x,
					  0.0f, 1.0f, 0.0f, y,
					  0.0f, 0.0f, 1.0f, z,
					  0.0f, 0.0f, 0.0f, 1.0f };

	return result;
}

Matrix Matrix_Rotate(Vector3 axis, float angle)
{
	Matrix result = { 0 };

	float x = axis.x, y = axis.y, z = axis.z;

	float lengthSquared = x*x + y*y + z*z;

	if ((lengthSquared != 1.0f) && (lengthSquared != 0.0f))
	{
		float ilength = 1.0f/sqrtf(lengthSquared);
		x *= ilength;
		y *= ilength;
		z *= ilength;
	}

	float sinres = sinf(angle);
	float cosres = cosf(angle);
	float t = 1.0f - cosres;

	result.m0 = x*x*t + cosres;
	result.m1 = y*x*t + z*sinres;
	result.m2 = z*x*t - y*sinres;
	result.m3 = 0.0f;

	result.m4 = x*y*t - z*sinres;
	result.m5 = y*y*t + cosres;
	result.m6 = z*y*t + x*sinres;
	result.m7 = 0.0f;

	result.m8 = x*z*t + y*sinres;
	result.m9 = y*z*t - x*sinres;
	result.m10 = z*z*t + cosres;
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

	result.m0 = 2.0f/rl;
	result.m1 = 0.0f;
	result.m2 = 0.0f;
	result.m3 = 0.0f;
	result.m4 = 0.0f;
	result.m5 = 2.0f/tb;
	result.m6 = 0.0f;
	result.m7 = 0.0f;
	result.m8 = 0.0f;
	result.m9 = 0.0f;
	result.m10 = -2.0f/fn;
	result.m11 = 0.0f;
	result.m12 = -((float)left + (float)right)/rl;
	result.m13 = -((float)top + (float)bottom)/tb;
	result.m14 = -((float)farPlane + (float)nearPlane)/fn;
	result.m15 = 1.0f;

	return result;
}

Matrix MatrixPerspective(double fovY, double aspect, double nearPlane, double farPlane)
{
	Matrix result = { 0 };

	double top = nearPlane*tan(fovY*0.5);
	double bottom = -top;
	double right = top*aspect;
	double left = -right;

	// MatrixFrustum(-right, right, -top, top, near, far);
	float rl = (float)(right - left);
	float tb = (float)(top - bottom);
	float fn = (float)(farPlane - nearPlane);

	result.m0 = ((float)nearPlane*2.0f)/rl;
	result.m5 = ((float)nearPlane*2.0f)/tb;
	result.m8 = ((float)right + (float)left)/rl;
	result.m9 = ((float)top + (float)bottom)/tb;
	result.m10 = -((float)farPlane + (float)nearPlane)/fn;
	result.m11 = -1.0f;
	result.m14 = -((float)farPlane*(float)nearPlane*2.0f)/fn;

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

	// Vector3Subtract(eye, target)
	Vector3 vz = { eye.x - target.x, eye.y - target.y, eye.z - target.z };

	// Vector3Normalize(vz)
	Vector3 v = vz;
	length = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
	if (length == 0.0f) length = 1.0f;
	ilength = 1.0f/length;
	vz.x *= ilength;
	vz.y *= ilength;
	vz.z *= ilength;

	// Vector3CrossProduct(up, vz)
	Vector3 vx = { up.y*vz.z - up.z*vz.y, up.z*vz.x - up.x*vz.z, up.x*vz.y - up.y*vz.x };

	// Vector3Normalize(x)
	v = vx;
	length = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
	if (length == 0.0f) length = 1.0f;
	ilength = 1.0f/length;
	vx.x *= ilength;
	vx.y *= ilength;
	vx.z *= ilength;

	// Vector3CrossProduct(vz, vx)
	Vector3 vy = { vz.y*vx.z - vz.z*vx.y, vz.z*vx.x - vz.x*vx.z, vz.x*vx.y - vz.y*vx.x };

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
	result.m12 = -(vx.x*eye.x + vx.y*eye.y + vx.z*eye.z);   // Vector3DotProduct(vx, eye)
	result.m13 = -(vy.x*eye.x + vy.y*eye.y + vy.z*eye.z);   // Vector3DotProduct(vy, eye)
	result.m14 = -(vz.x*eye.x + vz.y*eye.y + vz.z*eye.z);   // Vector3DotProduct(vz, eye)
	result.m15 = 1.0f;

	return result;
}
