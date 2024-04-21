#pragma once

#include <string>
#include "mge.h"
#include "mge_math.h"

// Matrix modes (equivalent to OpenGL)
#define MGEGL_MODELVIEW		0x1700 // GL_MODELVIEW
#define MGEGL_PROJECTION	0x1701 // GL_PROJECTION
#define MGEGL_TEXTURE		0x1702 // GL_TEXTURE

// Draws
#define MGEGL_LINES			0x0001 // GL_LINES
#define MGEGL_TRIANGLES		0x0004 // GL_TRIANGLES
#define MGEGL_QUADS			0x0007 // GL_QUADS

#define MGEGL_MAX_MATRIX_STACK_SIZE 32

void MgeGL_Init(int width, int height);
void MgeGL_Close();
void MgeGL_Draw();
void MgeGL_Begin(int mode);
void MgeGL_End();
void MgeGL_ClearScreenBuffers(void);
void MgeGL_Viewport(int x, int y, int width, int height);
void MgeGL_Load_Extensions(void* loader);
void MgeGL_ClearColor(Color color);
void MgeGL_MatrixMode(int mode);
void MgeGL_LoadIdentity(void);
void MgeGL_EnableDepthTest(void);
void MgeGL_DisableDepthTest(void);

std::string MgeGL_ReadShaderFromFile(const char* file_path);
unsigned int MgeGL_LoadShader(const char* code, unsigned int shaderType, std::string typeName);
unsigned int MgeGL_CreateShaderProgram(unsigned int vertex, unsigned int fragment);
int MgeGL_GetAttribLoc(const char* name);
void MgeGL_Uniform1i(int programID, const char* name, const int value);
void MgeGL_Uniform3fv(int programID, const char* name, const Vector3& value);
void MgeGL_Uniform4fv(int programID, const char* name, const Vector4& value);
void MgeGL_UniformMatrix4fv(int programID, const char* name, const Matrix& value);

void MgeGL_Frustum(double left, double right, double bottom, double top, double znear, double zfar);
void MgeGL_Ortho(double left, double right, double bottom, double top, double znear, double zfar);
void MgeGL_Translatef(float x, float y, float z);
void MgeGL_Rotatef(float angle, float x, float y, float z);
void MgeGL_MultMatrixf(const float *matf);
void MgeGL_PushMatrix(void);
void MgeGL_PopMatrix(void);

void MgeGL_Color4ub(unsigned char x, unsigned char y, unsigned char z, unsigned char w);
void MgeGL_Vertex2i(int x, int y);
void MgeGL_Vertex2f(float x, float y);
void MgeGL_Vertex3f(float x, float y, float z);
void MgeGL_TexCoord2f(float x, float y);
int MgeGL_LoadTexture(const void *data, int width, int height, int format, int mipmapCount);
