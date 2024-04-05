#pragma once

#include "mge.h"
#include "mge_utils.h"
#include "Shader.h"

#define MGEGL_LINES			0x0001
#define MGEGL_TRIANGLES		0x0004
#define MGEGL_QUADS			0x0007

#define MAX_VERTICES		128*3

typedef struct MgeGL_RendererBatch MgeGL_RendererBatch;
typedef struct MgeGL_Data MgeGL_Data;

void MgeGL_Clear_Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void MgeGL_Clear_Screen_Buffers(void);
void MgeGL_Load_Extensions(void* loader);
void MgeGL_Enable_Depth_Test(void);
void MgeGL_Viewport(int x, int y, int width, int height);
void MgeGL_Ortho(double left, double right, double bottom, double top, double znear, double zfar);

void MgeGL_Init(void);
void MgeGL_Close(void);
void MgeGL_Begin(int mode);
void MgeGL_End(void);
void MgeGL_Color4ub(unsigned char x, unsigned char y, unsigned char z, unsigned char w);
void MgeGL_Vertex2i(int x, int y);
void MgeGL_Vertex2f(float x, float y);
void MgeGL_Vertex3f(float x, float y, float z);
