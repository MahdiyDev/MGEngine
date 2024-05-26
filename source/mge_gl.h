#pragma once

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

#ifndef MGEGL_MAX_MATRIX_STACK_SIZE
	#define MGEGL_MAX_MATRIX_STACK_SIZE 32
#endif

// Shader limits
#ifndef MGEGL_MAX_SHADER_LOCATIONS
	#define MGEGL_MAX_SHADER_LOCATIONS                 32      // Maximum number of shader locations supported
#endif

#ifndef MAX_ATTRIB_LOCATION
	#define MAX_ATTRIB_LOCATION		3
#endif

#ifndef MAX_BUFFER_ELEMENTS
	#define MAX_BUFFER_ELEMENTS		256*5
#endif

#ifndef MGEGL_DEFAULT_DRAWCALLS
	#define MGEGL_DEFAULT_DRAWCALLS 256
#endif

typedef enum {
	VERTICE_LOCATION = 0,
	COLOR_LOCATION,
	TEXTURE_LOCATION,
} AttribLocations;

typedef struct VertexData {
	float* vertices;
	unsigned char* colors;
	float* texcoords;
	unsigned int* indices;

	int elementCount;
} VertexData;

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
bool MgeGL_CheckRenderBatchLimit(int vCount);

unsigned int MgeGL_GetDefaultShaderId();
void MgeGL_SetShader(unsigned int id);
unsigned int MgeGL_LoadShader(const char* code, unsigned int shaderType, const char* typeName);
unsigned int MgeGL_CreateShaderProgram(unsigned int vertex, unsigned int fragment);
void MgeGL_UnloadShaderProgram(unsigned int id);
void MgeGL_SetTexture(unsigned int id);
int MgeGL_GetAttribLoc(const char* name);
void MgeGL_Uniform1i(const char* name, const int value);
void MgeGL_Uniform3fv(const char* name, const Vector3& value);
void MgeGL_Uniform4fv(const char* name, const Vector4& value);
void MgeGL_UniformMatrix4fv(const char* name, const Matrix& value);

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
