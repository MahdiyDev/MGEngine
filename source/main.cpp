#include "mge.h"
#include "mge_gl.h"
#include <cstddef>
#include <cstdio>

float vertices[] = {
    1.0f, 0.0f,   0.5f, -0.5f, -0.5f,
    0.0f, 0.0f,  -0.5f, -0.5f, -0.5f,
    1.0f, 1.0f,   0.5f,  0.5f, -0.5f,
    1.0f, 1.0f,   0.5f,  0.5f, -0.5f,
    0.0f, 1.0f,  -0.5f,  0.5f, -0.5f,
    0.0f, 0.0f,  -0.5f, -0.5f, -0.5f,

    0.0f, 0.0f,  -0.5f, -0.5f,  0.5f,
    1.0f, 0.0f,   0.5f, -0.5f,  0.5f,
    1.0f, 1.0f,   0.5f,  0.5f,  0.5f,
    1.0f, 1.0f,   0.5f,  0.5f,  0.5f,
    0.0f, 1.0f,  -0.5f,  0.5f,  0.5f,
    0.0f, 0.0f,  -0.5f, -0.5f,  0.5f,

    1.0f, 0.0f,  -0.5f,  0.5f,  0.5f,
    1.0f, 1.0f,  -0.5f,  0.5f, -0.5f,
    0.0f, 1.0f,  -0.5f, -0.5f, -0.5f,
    0.0f, 1.0f,  -0.5f, -0.5f, -0.5f,
    0.0f, 0.0f,  -0.5f, -0.5f,  0.5f,
    1.0f, 0.0f,  -0.5f,  0.5f,  0.5f,

    1.0f, 0.0f,   0.5f,  0.5f,  0.5f,
    1.0f, 1.0f,   0.5f,  0.5f, -0.5f,
    0.0f, 1.0f,   0.5f, -0.5f, -0.5f,
    0.0f, 1.0f,   0.5f, -0.5f, -0.5f,
    0.0f, 0.0f,   0.5f, -0.5f,  0.5f,
    1.0f, 0.0f,   0.5f,  0.5f,  0.5f,

    0.0f, 1.0f,  -0.5f, -0.5f, -0.5f,
    1.0f, 1.0f,   0.5f, -0.5f, -0.5f,
    1.0f, 0.0f,   0.5f, -0.5f,  0.5f,
    1.0f, 0.0f,   0.5f, -0.5f,  0.5f,
    0.0f, 0.0f,  -0.5f, -0.5f,  0.5f,
    0.0f, 1.0f,  -0.5f, -0.5f, -0.5f,

    0.0f, 1.0f,  -0.5f,  0.5f, -0.5f,
    1.0f, 1.0f,   0.5f,  0.5f, -0.5f,
    1.0f, 0.0f,   0.5f,  0.5f,  0.5f,
    1.0f, 0.0f,   0.5f,  0.5f,  0.5f,
    0.0f, 0.0f,  -0.5f,  0.5f,  0.5f,
    0.0f, 1.0f,  -0.5f,  0.5f, -0.5f
};

int main(int argc, char** argv)
{
	Mge_InitWindow(800, 600, "OpenGL engine v1.0");
	Mge_SetTargetFPS(60);
	while(!Mge_WindowShouldClose())
	{
		Mge_BeginDrawing();
			Mge_ClearBackground(DARKGREEN);

			MgeGL_Begin(MGEGL_TRIANGLES);
				MgeGL_Color4ub(255, 255, 255, 255);
				for (size_t i = 0; i < sizeof(vertices) / sizeof(float); i++) {
					MgeGL_TexCoord2f(vertices[i+0], vertices[i+1]);
					MgeGL_Vertex3f(vertices[i+2],  vertices[i+3], vertices[i+4]);
					i+=4;
				}
			MgeGL_End();

			MgeGL_Begin(MGEGL_TRIANGLES);
				MgeGL_Color4ub(255, 0, 0, 255);
				for (size_t i = 0; i < sizeof(vertices) / sizeof(float); i++) {
					MgeGL_TexCoord2f(vertices[i+0], vertices[i+1]);
					MgeGL_Vertex3f(vertices[i+2],  vertices[i+3], vertices[i+4]+1.5f);
					i+=4;
				}
			MgeGL_End();

			MgeGL_Begin(MGEGL_TRIANGLES);
				MgeGL_Color4ub(0, 255, 0, 255);
				for (size_t i = 0; i < sizeof(vertices) / sizeof(float); i++) {
					MgeGL_TexCoord2f(vertices[i+0], vertices[i+1]);
					MgeGL_Vertex3f(vertices[i+2],  vertices[i+3], vertices[i+4]-1.5f);
					i+=4;
				}
			MgeGL_End();

			MgeGL_Begin(MGEGL_TRIANGLES);
				MgeGL_Color4ub(0, 0, 255, 255);
				for (size_t i = 0; i < sizeof(vertices) / sizeof(float); i++) {
					MgeGL_TexCoord2f(vertices[i+0], vertices[i+1]);
					MgeGL_Vertex3f(vertices[i+2]+1.5f,  vertices[i+3], vertices[i+4]);
					i+=4;
				}
			MgeGL_End();

			MgeGL_Begin(MGEGL_TRIANGLES);
				MgeGL_Color4ub(0, 255, 255, 255);
				for (size_t i = 0; i < sizeof(vertices) / sizeof(float); i++) {
					MgeGL_TexCoord2f(vertices[i+0], vertices[i+1]);
					MgeGL_Vertex3f(vertices[i+2],  vertices[i+3]+1.5f, vertices[i+4]);
					i+=4;
				}
			MgeGL_End();

			MgeGL_Begin(MGEGL_TRIANGLES);
				MgeGL_Color4ub(255, 255, 0, 255);
				for (size_t i = 0; i < sizeof(vertices) / sizeof(float); i++) {
					MgeGL_TexCoord2f(vertices[i+0], vertices[i+1]);
					MgeGL_Vertex3f(vertices[i+2]-1.5f,  vertices[i+3], vertices[i+4]);
					i+=4;
				}
			MgeGL_End();

			MgeGL_Begin(MGEGL_TRIANGLES);
				MgeGL_Color4ub(255, 0, 255, 255);
				for (size_t i = 0; i < sizeof(vertices) / sizeof(float); i++) {
					MgeGL_TexCoord2f(vertices[i+0], vertices[i+1]);
					MgeGL_Vertex3f(vertices[i+2],  vertices[i+3]-1.5f, vertices[i+4]);
					i+=4;
				}
			MgeGL_End();
		Mge_EndDrawing();
	}

	Mge_CloseWindow();
	return 0;
}
