#include "mge.h"
#include "mge_gl.h"
#include "mge_math.h"
#include <cstdio>
#include <math.h>

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
	0.0f, 1.0f,  -0.5f,  0.5f, -0.5f,
};

int width = 800 * 2, height = 600 * 2;
float firstMouse = true;
float lastX = width / 2.0;
float lastY = height / 2.0;
float yaw = -90.0f;
float pitch = 0.0f;

void HandleCameraMovement(Camera3D& camera)
{
	const float cameraSpeed = 5.0f * Mge_GetDeltaTime();
	if (IsKeyDown(KEY_W))
		camera.position += camera.target * cameraSpeed;
	else if (IsKeyDown(KEY_S))
		camera.position -= camera.target * cameraSpeed;
	else if (IsKeyDown(KEY_A))
		camera.position -= Vector3Normalize(Vector3Cross(camera.target, camera.up)) * cameraSpeed;
	else if (IsKeyDown(KEY_D))
		camera.position += Vector3Normalize(Vector3Cross(camera.target, camera.up)) * cameraSpeed;

	float xpos = GetMouseX();
	float ypos = GetMouseY();

	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f; // change this value to your liking
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	// make sure that when pitch is out of bounds, screen doesn't get flipped
	pitch = Clamp(pitch, -89.0f, 89.0f);

	Vector3 front;
	front.x = cos(yaw * DEG2RAD) * cos(pitch * DEG2RAD);
	front.y = sin(pitch * DEG2RAD);
	front.z = sin(yaw * DEG2RAD) * cos(pitch * DEG2RAD);
	camera.target = Vector3Normalize(front);
}

int main(int argc, char** argv)
{
	Mge_InitWindow(width, height, "OpenGL engine v1.0");
	Mge_SetTargetFPS(60);
	DisableCursor();

	Camera3D camera = { 0 };
	camera.position = Vector3 { 0.0f, 0.0f, 10.0f };
	camera.target = Vector3 { 0.0f, 0.0f, -1.0f };
	camera.up = Vector3 { 0.0f, 1.0f, 0.0f };
	camera.fovy = 60.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	// Texture2D cubeTexture = Mge_LoadTexture("assets/wall.jpg");
	// Texture2D cube2Texture = Mge_LoadTexture("assets/smile_face.jpg");
	Shader lightShader = Mge_LoadShader("shaders/light_shader.vert", "shaders/light_shader.frag");

	while (!Mge_WindowShouldClose()) {
		Mge_BeginDrawing();
		{
			Mge_ClearBackground(DARKGREEN);
			HandleCameraMovement(camera);

			Mge_BeginMode3D(camera);
			{
				Mge_BeginShaderMode(lightShader);
				{
					// MgeGL_SetTexture(cubeTexture.id);
					MgeGL_Uniform4fv("lightColor", Vector4 { 1.0f, 1.0f, (float)sin(Mge_GetTime()) * 2.0f, 1.0f });

					MgeGL_Begin(MGEGL_TRIANGLES);
					{
						MgeGL_Color4ub(255, 255, 255, 255);
						for (size_t i = 0; i < sizeof(vertices) / sizeof(float); i++) {
							MgeGL_TexCoord2f(vertices[i + 0], vertices[i + 1]);
							MgeGL_Vertex3f(vertices[i + 2], vertices[i + 3], vertices[i + 4]);
							i += 4;
						}
					}
					MgeGL_End();

					MgeGL_Begin(MGEGL_TRIANGLES);
					{
						MgeGL_Color4ub(255, 0, 0, 255);
						for (size_t i = 0; i < sizeof(vertices) / sizeof(float); i++) {
							MgeGL_TexCoord2f(vertices[i + 0], vertices[i + 1]);
							MgeGL_Vertex3f(vertices[i + 2], vertices[i + 3], vertices[i + 4] + 1.5f);
							i += 4;
						}
					}
					MgeGL_End();

					MgeGL_Begin(MGEGL_TRIANGLES);
					{
						MgeGL_Color4ub(0, 255, 0, 255);
						for (size_t i = 0; i < sizeof(vertices) / sizeof(float); i++) {
							MgeGL_TexCoord2f(vertices[i + 0], vertices[i + 1]);
							MgeGL_Vertex3f(vertices[i + 2], vertices[i + 3], vertices[i + 4] - 1.5f);
							i += 4;
						}
					}
					MgeGL_End();

					MgeGL_Begin(MGEGL_TRIANGLES);
					{
						MgeGL_Color4ub(0, 0, 255, 255);
						for (size_t i = 0; i < sizeof(vertices) / sizeof(float); i++) {
							MgeGL_TexCoord2f(vertices[i + 0], vertices[i + 1]);
							MgeGL_Vertex3f(vertices[i + 2] + 1.5f, vertices[i + 3], vertices[i + 4]);
							i += 4;
						}
					}
					MgeGL_End();

					MgeGL_Begin(MGEGL_TRIANGLES);
					{
						MgeGL_Color4ub(0, 255, 255, 255);
						for (size_t i = 0; i < sizeof(vertices) / sizeof(float); i++) {
							MgeGL_TexCoord2f(vertices[i + 0], vertices[i + 1]);
							MgeGL_Vertex3f(vertices[i + 2], vertices[i + 3] + 1.5f, vertices[i + 4]);
							i += 4;
						}
					}
					MgeGL_End();

					MgeGL_Begin(MGEGL_TRIANGLES);
					{
						MgeGL_Color4ub(255, 255, 0, 255);
						for (size_t i = 0; i < sizeof(vertices) / sizeof(float); i++) {
							MgeGL_TexCoord2f(vertices[i + 0], vertices[i + 1]);
							MgeGL_Vertex3f(vertices[i + 2] - 1.5f, vertices[i + 3], vertices[i + 4]);
							i += 4;
						}
					}
					MgeGL_End();

					MgeGL_Begin(MGEGL_TRIANGLES);
					{
						MgeGL_Color4ub(255, 0, 255, 255);
						for (size_t i = 0; i < sizeof(vertices) / sizeof(float); i++) {
							MgeGL_TexCoord2f(vertices[i + 0], vertices[i + 1]);
							MgeGL_Vertex3f(vertices[i + 2], vertices[i + 3] - 1.5f, vertices[i + 4]);
							i += 4;
						}
					}
					MgeGL_End();

					MgeGL_Begin(MGEGL_TRIANGLES);
					{
						MgeGL_Color4ub(255, 255, 255, 255);
						for (size_t i = 0; i < sizeof(vertices) / sizeof(float); i++) {
							MgeGL_TexCoord2f(vertices[i + 0], vertices[i + 1]);
							MgeGL_Vertex3f(vertices[i + 2] + 3.0f, vertices[i + 3] + 3.0f, vertices[i + 4] + 3.0f);
							i += 4;
						}
					}
					MgeGL_End();
				}
				Mge_EndShaderMode();
			}
			Mge_EndMode3D();
		}
		Mge_EndDrawing();
	}

	Mge_UnloadShader(lightShader);
	Mge_CloseWindow();
	return 0;
}
