#include "mge_gl.h"
#include "mge.h"

typedef struct MgeGL_RendererBatch {
	int mode;
	int vertexCount;
    float vertices[MAX_VERTICES];
	float currentDepth;
	int drawCounter;

    Shader* current_shader;
} MgeGL_RendererBatch;

typedef struct MgeGL_Data {
	MgeGL_RendererBatch batch;

    struct {
        unsigned char colorr, colorg, colorb, colora;
        Matrix model;
        Matrix view;
        Matrix projection;
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
	for (int i = 0; i < MAX_VERTICES; i++) {
		glData.batch.vertices[i] = 0.0f;
	}
	glData.batch.currentDepth = -1.0f;         // Reset depth value
	glData.batch.drawCounter = 0;
	TRACE_LOG(LOG_INFO, "MGE_GL: initialized");
}

void MgeGL_Close(void)
{
	glData.batch.current_shader->CleanUp();
	TRACE_LOG(LOG_INFO, "MGE_GL: clean up");
}

void MgeGL_Begin(int mode)
{
	glData.batch.mode = mode;
	glData.batch.vertexCount = 0;
	if (glData.batch.drawCounter >= MAX_DRAW_COUNT)
	{
		glData.batch.current_shader->DrawArrays(glData.batch.mode, MAX_VERTICES/3);
		glData.batch.currentDepth = -1.0f;         // Reset depth value
		glData.batch.drawCounter = 0;
	}
}

void MgeGL_End(void)
{
	glData.batch.current_shader->Set_Position_Buffer(glData.batch.vertices, MAX_VERTICES);
	glData.batch.current_shader->Use();
	glData.batch.current_shader->Set_Mat4("model", glData.State.model);
	glData.batch.current_shader->Set_Mat4("view", glData.State.view);
	glData.batch.current_shader->Set_Mat4("projection", glData.State.projection);
	glData.batch.current_shader->Set_Vec4("color",
		CLITERAL(Vector4) { 
			(float)glData.State.colorr, (float)glData.State.colorg, 
			(float)glData.State.colorb, (float)glData.State.colora
		}
	);
	glData.batch.current_shader->DrawArrays(glData.batch.mode, MAX_VERTICES/3);
	glData.batch.currentDepth += (1.0f/20000.0f);
	glData.batch.vertexCount = 0;
	glData.batch.drawCounter++;
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
	glData.batch.vertices[3*glData.batch.vertexCount+0] = x;
	glData.batch.vertices[3*glData.batch.vertexCount+1] = y;
	glData.batch.vertices[3*glData.batch.vertexCount+2] = z;
	glData.batch.vertexCount++;
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
