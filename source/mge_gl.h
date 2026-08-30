#pragma once

#include "mge.h"
#include "mge_math.h"

#ifdef __cplusplus
extern "C" {
#endif

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
	#define MAX_ATTRIB_LOCATION		4
#endif

#ifndef MAX_BUFFER_ELEMENTS
	#define MAX_BUFFER_ELEMENTS		256*5
#endif

// The immediate-mode batch keeps this many copies of its dynamic vertex buffers
// and cycles to the next one on every flush, so a glBufferSubData never lands on
// a buffer the GPU is still reading from the previous draw (no pipeline stall).
#ifndef MGEGL_BATCH_BUFFERS
	#define MGEGL_BATCH_BUFFERS		3
#endif

#ifndef MGEGL_DEFAULT_DRAWCALLS
	#define MGEGL_DEFAULT_DRAWCALLS 256
#endif

// Fixed attribute locations -- every shader (default and custom) must use
// `layout(location = N)` matching these so the one shared VAO stays valid.
typedef enum {
	VERTICE_LOCATION = 0,
	COLOR_LOCATION = 1,
	TEXTURE_LOCATION = 2,
	NORMAL_LOCATION = 3,
} AttribLocations;

typedef struct VertexData {
	float* vertices;
	unsigned char* colors;
	float* texcoords;
	float* normals;
	unsigned int* indices;

	int elementCount;
} VertexData;

void MgeGL_Init(int width, int height);
void MgeGL_Close(void);
void MgeGL_Draw(void);
void MgeGL_Begin(int mode);
void MgeGL_End(void);
void MgeGL_ClearScreenBuffers(void);
void MgeGL_Viewport(int x, int y, int width, int height);
void MgeGL_Load_Extensions(void* loader);
void MgeGL_ClearColor(Color color);
void MgeGL_MatrixMode(int mode);
void MgeGL_LoadIdentity(void);
void MgeGL_EnableDepthTest(void);
void MgeGL_DisableDepthTest(void);
bool MgeGL_IsDepthTestEnabled(void);
void MgeGL_SetDepthFunc(int depthFunc);   // DepthFunc enum
void MgeGL_SetDepthMask(bool writeEnabled);
void MgeGL_SetPolygonOffset(bool enabled, float factor, float units);

void MgeGL_EnableStencilTest(void);
void MgeGL_DisableStencilTest(void);
void MgeGL_SetStencilFunc(int func, int ref, unsigned int mask);       // StencilFunc
void MgeGL_SetStencilOp(int onStencilFail, int onDepthFail, int onPass); // StencilOp
void MgeGL_SetStencilMask(unsigned int mask);
void MgeGL_SetColorMask(bool enabled);   // all channels
void MgeGL_ClearStencil(void);

void MgeGL_SetFaceCulling(bool enabled);
void MgeGL_SetCullFace(int face);      // CullFace
void MgeGL_SetFrontFace(int winding);  // FrontFace

int MgeGL_GetSampleCount(void);        // GL_SAMPLES of the current framebuffer (0 = not multisampled)
bool MgeGL_CheckRenderBatchLimit(int vCount);

void MgeGL_RegisterDrawCall(void);     // feature modules issuing their own gl draw* call this
int  MgeGL_GetDrawCalls(void);         // GL draws since the last reset
void MgeGL_ResetDrawCalls(void);       // called by Mge_BeginDrawing

unsigned int MgeGL_GetDefaultShaderId(void);
unsigned int MgeGL_GetCurrentShaderId(void);
void MgeGL_SetShader(unsigned int id);
unsigned int MgeGL_LoadShader(const char* code, unsigned int shaderType, const char* typeName);
unsigned int MgeGL_CreateShaderProgram(unsigned int vertex, unsigned int fragment);
unsigned int MgeGL_CreateShaderProgramGeo(unsigned int vertex, unsigned int geometry, unsigned int fragment);
void MgeGL_UnloadShaderProgram(unsigned int id);
void MgeGL_SetTexture(unsigned int id);
unsigned int MgeGL_GetWhiteTexture(void); // the 1x1 white texture created at init

// Retained indexed meshes (own VAO/VBO/EBO). Interleaved vertex layout:
//   location 0 vec3 position, location 3 vec3 normal, location 2 vec2 texcoord.
void MgeGL_UploadMesh(unsigned int* vao, unsigned int* vbo, unsigned int* ebo,
	const void* vertices, int vertexCount, const unsigned int* indices, int indexCount);
// same, but the attributes are batched (one VBO, block per attribute) not interleaved
void MgeGL_UploadMeshBatched(unsigned int* vao, unsigned int* vbo, unsigned int* ebo,
	const void* positions, const void* normals, const void* texcoords,
	int vertexCount, const unsigned int* indices, int indexCount);
void MgeGL_DrawMesh(unsigned int vao, int indexCount, unsigned int textureId); // uses the current shader
void MgeGL_UnloadMesh(unsigned int vao, unsigned int vbo, unsigned int ebo);
int MgeGL_GetAttribLoc(const char* name);
void MgeGL_Uniform1i(const char* name, const int value);
void MgeGL_Uniform1f(const char* name, float value);
void MgeGL_Uniform3fv(const char* name, Vector3 value);
void MgeGL_Uniform4fv(const char* name, Vector4 value);
void MgeGL_UniformMatrix4fv(const char* name, Matrix value);

Matrix MgeGL_GetMatrixModelview(void);   // current view matrix (set by Mge_BeginMode3D)
Matrix MgeGL_GetMatrixProjection(void);

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
void MgeGL_Normal3f(float x, float y, float z);
int MgeGL_LoadTexture(const void *data, int width, int height, int format, int mipmapCount);

#ifdef __cplusplus
}
#endif
