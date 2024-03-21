#pragma once

#include "Shader.h"
#include "mge.h"
#include "mge_utils.h"

#include <glad/glad.h>

typedef struct MgeGL_Data {
	Shader* lineShader;
	glm::vec3 lineColor;
	glm::mat4 MVP = glm::mat4(1.0f);
	std::vector<float> vertices;
} MgeGL_Data;


void MgeGL_Clear_Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void MgeGL_Clear_Screen_Buffers(void);
void MgeGL_Load_Extensions(void* loader);
void MgeGL_Enable_Depth_Test(void);
void MgeGL_Viewport(int x, int y, int width, int height);
void MgeGL_Init(void);
void MgeGL_Close(void);
void MgeGL_Begin(void);
void MgeGL_End(void);

// #define MGEGL_IMLEMENTATION
#ifdef MGEGL_IMLEMENTATION
MgeGL_Data glData = {0};

void MgeGL_Init(void){
	glData.lineShader = new Shader(
		"shaders/line_shader.vert",
		"shaders/line_shader.frag"
	);
}

void MgeGL_Close(void)
{
	glData.lineShader->CleanUp();
}

void MgeGL_Begin(void)
{
	glData.vertices = {};
}

void MgeGL_End(void)
{
}

inline void MgeGL_Clear_Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    float cr = (float)r / 255;
    float cg = (float)g / 255;
    float cb = (float)b / 255;
    float ca = (float)a / 255;

    glClearColor(cr, cg, cb, ca);
}

inline void MgeGL_Clear_Screen_Buffers(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

inline void MgeGL_Load_Extensions(void* loader)
{
    if (gladLoadGLLoader((GLADloadproc)loader) == 0) {
        TRACE_LOG(LOG_WARNING, "GLAD: Cannot load OpenGL extensions");
    } else {
        TRACE_LOG(LOG_INFO, "GLAD: OpenGL extensions loaded successfully");
    }

    MgeGL_Enable_Depth_Test();
}

inline void MgeGL_Enable_Depth_Test(void) { glEnable(GL_DEPTH_TEST); }

inline void MgeGL_Viewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}
#endif
