#pragma once

#include "mge.h"
#include "mge_utils.h"

#include <glad/glad.h>

void MgeGL_Clear_Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void MgeGL_Clear_Screen_Buffers(void);
void MgeGL_Load_Extensions(void* loader);
void MgeGL_Enable_Depth_Test(void);
void MgeGL_Viewport(int x, int y, int width, int height);

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
