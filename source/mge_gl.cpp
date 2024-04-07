#include "mge.h"
#include "mge_gl.h"
#include <cstddef>
#include <cstdio>
#include <vector>

typedef struct MgeGL_VertexBuffer {
	int elementCount;
    float vertices[MAX_BUFFER_ELEMENTS*3];
	unsigned char colors[MAX_BUFFER_ELEMENTS*4];
} MgeGL_VertexBuffer;

typedef struct MgeGL_DrawCall {
	int mode;
	int vertexCount;
	int vertexAlignment;
} MgeGL_DrawCall;

typedef struct MgeGL_RendererBatch {
	float currentDepth;
	int drawCounter;
	int currentBuffer;
	MgeGL_VertexBuffer vertexBuffer[MAX_DRAW_COUNT];

	MgeGL_DrawCall draws[MAX_DRAW_COUNT];

    Shader* current_shader;
} MgeGL_RendererBatch;

typedef struct MgeGL_Data {
	MgeGL_RendererBatch batch;

    struct {
        unsigned char colorr, colorg, colorb, colora;
        Matrix model;
        Matrix view;
        Matrix projection;
		int vertexCounter;
    } State;
} MgeGL_Data;

MgeGL_Data glData = { 0 };

#define MAX_DRAW_COUNT  256

void MgeGL_Init(void)
{
	glData.batch.current_shader = new Shader("shaders/core_shader.vert", "shaders/core_shader.frag");
	glData.State.model		= Matrix_Identity();
	glData.State.view		= Matrix_Identity();
	glData.State.projection	= Matrix_Identity();
	for (int i = 0; i < MAX_DRAW_COUNT; i++)
	{
		glData.batch.vertexBuffer[i].elementCount = MAX_BUFFER_ELEMENTS;
		for (int v = 0; v < MAX_BUFFER_ELEMENTS*3; v++) glData.batch.vertexBuffer[i].vertices[v] = 0;
		for (int c = 0; c < MAX_BUFFER_ELEMENTS*4; c++) glData.batch.vertexBuffer[i].colors[c] = 0;
	}
	glData.batch.currentDepth = -1.0f;			// Reset depth value
	glData.batch.drawCounter = 1;				// Reste draw count
	TRACE_LOG(LOG_INFO, "MGE_GL: initialized");
}

void MgeGL_Close(void)
{
	glData.batch.current_shader->CleanUp();
	TRACE_LOG(LOG_INFO, "MGE_GL: clean up");
}

void MgeGL_Begin(int mode)
{
	MgeGL_DrawCall* drawCall = &glData.batch.draws[glData.batch.drawCounter - 1];
	if (drawCall->mode != mode)
	{
		if (drawCall->vertexCount > 0)
		{
			if (drawCall->mode == MGEGL_LINES) {
				drawCall->vertexAlignment = drawCall->vertexCount < 4 ? drawCall->vertexCount : drawCall->vertexCount%4;
			}
			else if (drawCall->mode == MGEGL_TRIANGLES)
			{
				drawCall->vertexAlignment = drawCall->vertexCount < 4 ? 1 : drawCall->vertexCount%4;
			}
			else
			{
				drawCall->vertexAlignment = 0;
			}

			if (!MgeGL_Check_RenderBatch_Limit(drawCall->vertexAlignment))
			{
				glData.State.vertexCounter += drawCall->vertexAlignment;
				glData.batch.drawCounter++;
			}
		}

		if (glData.batch.drawCounter >= MAX_DRAW_COUNT)
		{
			MgeGL_Draw_RenderBatch(glData.batch);
		}

		drawCall->mode = mode;
		drawCall->vertexCount = 0;
	}
}

void MgeGL_End(void)
{
	glData.batch.currentDepth += (1.0f/20000.0f);
}

void MgeGL_Color4ub(unsigned char x, unsigned char y, unsigned char z, unsigned char w)
{
	glData.State.colorr = x;
	glData.State.colorg = y;
	glData.State.colorb = z;
	glData.State.colora = w;
}

void MgeGL_Vertex2i(int x, int y)
{
	MgeGL_Vertex3f((float)x, (float)y, glData.batch.currentDepth);
}

void MgeGL_Vertex2f(float x, float y)
{
	MgeGL_Vertex3f(x, y, glData.batch.currentDepth);
}

void MgeGL_Vertex3f(float x, float y, float z)
{
	if (glData.State.vertexCounter > (glData.batch.vertexBuffer[glData.batch.currentBuffer].elementCount*4 - 4))
	{
		if ((glData.batch.draws[glData.batch.drawCounter - 1].mode == MGEGL_LINES) &&
			(glData.batch.draws[glData.batch.drawCounter - 1].vertexCount%2 == 0))
		{
			MgeGL_Check_RenderBatch_Limit(2 + 1);
		}
		else if ((glData.batch.draws[glData.batch.drawCounter - 1].mode == MGEGL_TRIANGLES) &&
			(glData.batch.draws[glData.batch.drawCounter - 1].vertexCount%3 == 0))
		{
			MgeGL_Check_RenderBatch_Limit(3 + 1);
		}
		else if ((glData.batch.draws[glData.batch.drawCounter - 1].mode == MGEGL_QUADS) &&
			(glData.batch.draws[glData.batch.drawCounter - 1].vertexCount%4 == 0))
		{
			MgeGL_Check_RenderBatch_Limit(4 + 1);
		}
	}

	glData.batch.vertexBuffer[glData.batch.currentBuffer].vertices[3*glData.State.vertexCounter+0] = x;
	glData.batch.vertexBuffer[glData.batch.currentBuffer].vertices[3*glData.State.vertexCounter+1] = y;
	glData.batch.vertexBuffer[glData.batch.currentBuffer].vertices[3*glData.State.vertexCounter+2] = z;

	glData.batch.vertexBuffer[glData.batch.drawCounter].colors[4*glData.State.vertexCounter+0] = glData.State.colorr;
	glData.batch.vertexBuffer[glData.batch.drawCounter].colors[4*glData.State.vertexCounter+1] = glData.State.colorg;
	glData.batch.vertexBuffer[glData.batch.drawCounter].colors[4*glData.State.vertexCounter+2] = glData.State.colorb;
	glData.batch.vertexBuffer[glData.batch.drawCounter].colors[4*glData.State.vertexCounter+3] = glData.State.colora;

	glData.State.vertexCounter++;
}

void MgeGL_Draw_RenderBatch(MgeGL_RendererBatch batch)
{
	
	// if (glData.State.vertexCounter > 0)
	// {
	// 	glData.batch.current_shader->Set_Position_Buffer(
	// 		batch.vertexBuffer[batch.currentBuffer].vertices,
	// 		glData.State.vertexCounter*3
	// 	);
	// 	glData.batch.current_shader->Set_Color_Buffer(
	// 		batch.vertexBuffer[batch.currentBuffer].colors,
	// 		glData.State.vertexCounter*4
	// 	);
	// 	glData.batch.current_shader->Use();
	// 	glData.batch.current_shader->Set_Mat4("model", glData.State.model);
	// 	glData.batch.current_shader->Set_Mat4("view", glData.State.view);
	// 	glData.batch.current_shader->Set_Mat4("projection", glData.State.projection);
	// 	for (int i = 0, vertexOffset = 0; i < batch.drawCounter; i++)
	// 	{
	// 		glData.batch.current_shader->DrawArrays(
	// 			batch.draws[i].mode,
	// 			vertexOffset,
	// 			batch.draws[i].vertexCount
	// 		);
	// 		vertexOffset += (batch.draws[i].vertexCount + batch.draws[i].vertexAlignment);
	// 	}
	// }
}

bool MgeGL_Check_RenderBatch_Limit(int vCount)
{
	bool overflow = false;

	if ((glData.State.vertexCounter+vCount) >= (glData.batch.vertexBuffer[glData.batch.currentBuffer].elementCount*4))
	{
		overflow = true;

		int currentMode = glData.batch.draws[glData.batch.drawCounter - 1].mode;

		MgeGL_RendererBatch(glData.batch);

		glData.batch.draws[glData.batch.drawCounter - 1].mode = currentMode;
	}

	return overflow;
}

void MgeGL_Ortho(double left, double right, double bottom, double top, double znear, double zfar)
{
	// NOTE: If left-right and top-botton values are equal it could create a division by zero,
	// response to it is platform/compiler dependant
	Matrix matOrtho = { 0 };

	float rl = (float)(right - left);
	float tb = (float)(top - bottom);
	float fn = (float)(zfar - znear);

	matOrtho.m0 = 2.0f/rl;
	matOrtho.m1 = 0.0f;
	matOrtho.m2 = 0.0f;
	matOrtho.m3 = 0.0f;
	matOrtho.m4 = 0.0f;
	matOrtho.m5 = 2.0f/tb;
	matOrtho.m6 = 0.0f;
	matOrtho.m7 = 0.0f;
	matOrtho.m8 = 0.0f;
	matOrtho.m9 = 0.0f;
	matOrtho.m10 = -2.0f/fn;
	matOrtho.m11 = 0.0f;
	matOrtho.m12 = -((float)left + (float)right)/rl;
	matOrtho.m13 = -((float)top + (float)bottom)/tb;
	matOrtho.m14 = -((float)zfar + (float)znear)/fn;
	matOrtho.m15 = 1.0f;

	glData.State.projection = Matrix_Multiply(glData.State.projection, matOrtho);
}

void MgeGL_Clear_Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	float cr = (float)r / 255;
	float cg = (float)g / 255;
	float cb = (float)b / 255;
	float ca = (float)a / 255;

	glClearColor(cr, cg, cb, ca);
}

void MgeGL_Clear_Screen_Buffers(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void MgeGL_Load_Extensions(void* loader)
{
	if (gladLoadGLLoader((GLADloadproc)loader) == 0) {
		TRACE_LOG(LOG_WARNING, "GLAD: Cannot load OpenGL extensions");
	} else {
		TRACE_LOG(LOG_INFO, "GLAD: OpenGL extensions loaded successfully");
	}

	TRACE_LOG(LOG_INFO, "GL: OpenGL device information:");
	TRACE_LOG(LOG_INFO, "    > Vendor:   %s", glGetString(GL_VENDOR));
	TRACE_LOG(LOG_INFO, "    > Renderer: %s", glGetString(GL_RENDERER));
	TRACE_LOG(LOG_INFO, "    > Version:  %s", glGetString(GL_VERSION));
	TRACE_LOG(LOG_INFO, "    > GLSL:     %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    MgeGL_Enable_Depth_Test();
}

void MgeGL_Enable_Depth_Test(void) { glEnable(GL_DEPTH_TEST); }

void MgeGL_Viewport(int x, int y, int width, int height)
{
	glViewport(x, y, width, height);
}
