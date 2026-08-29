#include "mge.h"
#include "mge_gl.h"
#include "mge_math.h"

#include <math.h>
#include <stdio.h>

// Interleaved [u, v, x, y, z] per vertex; 36 vertices = a textured cube.
static float vertices[] = {
    1.0f, 0.0f, 0.5f, -0.5f, -0.5f,
    0.0f, 0.0f, -0.5f, -0.5f, -0.5f,
    1.0f, 1.0f, 0.5f, 0.5f, -0.5f,
    1.0f, 1.0f, 0.5f, 0.5f, -0.5f,
    0.0f, 1.0f, -0.5f, 0.5f, -0.5f,
    0.0f, 0.0f, -0.5f, -0.5f, -0.5f,

    0.0f, 0.0f, -0.5f, -0.5f, 0.5f,
    1.0f, 0.0f, 0.5f, -0.5f, 0.5f,
    1.0f, 1.0f, 0.5f, 0.5f, 0.5f,
    1.0f, 1.0f, 0.5f, 0.5f, 0.5f,
    0.0f, 1.0f, -0.5f, 0.5f, 0.5f,
    0.0f, 0.0f, -0.5f, -0.5f, 0.5f,

    1.0f, 0.0f, -0.5f, 0.5f, 0.5f,
    1.0f, 1.0f, -0.5f, 0.5f, -0.5f,
    0.0f, 1.0f, -0.5f, -0.5f, -0.5f,
    0.0f, 1.0f, -0.5f, -0.5f, -0.5f,
    0.0f, 0.0f, -0.5f, -0.5f, 0.5f,
    1.0f, 0.0f, -0.5f, 0.5f, 0.5f,

    1.0f, 0.0f, 0.5f, 0.5f, 0.5f,
    1.0f, 1.0f, 0.5f, 0.5f, -0.5f,
    0.0f, 1.0f, 0.5f, -0.5f, -0.5f,
    0.0f, 1.0f, 0.5f, -0.5f, -0.5f,
    0.0f, 0.0f, 0.5f, -0.5f, 0.5f,
    1.0f, 0.0f, 0.5f, 0.5f, 0.5f,

    0.0f, 1.0f, -0.5f, -0.5f, -0.5f,
    1.0f, 1.0f, 0.5f, -0.5f, -0.5f,
    1.0f, 0.0f, 0.5f, -0.5f, 0.5f,
    1.0f, 0.0f, 0.5f, -0.5f, 0.5f,
    0.0f, 0.0f, -0.5f, -0.5f, 0.5f,
    0.0f, 1.0f, -0.5f, -0.5f, -0.5f,

    0.0f, 1.0f, -0.5f, 0.5f, -0.5f,
    1.0f, 1.0f, 0.5f, 0.5f, -0.5f,
    1.0f, 0.0f, 0.5f, 0.5f, 0.5f,
    1.0f, 0.0f, 0.5f, 0.5f, 0.5f,
    0.0f, 0.0f, -0.5f, 0.5f, 0.5f,
    0.0f, 1.0f, -0.5f, 0.5f, -0.5f,
};

static const int width = 800 * 2, height = 600 * 2;
static bool firstMouse = true;
static float lastX = 800.0f;
static float lastY = 600.0f;
static float yaw = -90.0f;
static float pitch = 0.0f;

static void DrawCube(Vector3 offset, Color color)
{
    MgeGL_Begin(MGEGL_TRIANGLES);
    MgeGL_Color4ub(color.r, color.g, color.b, color.a);
    for (size_t i = 0; i + 5 <= sizeof(vertices) / sizeof(float); i += 5) {
        MgeGL_TexCoord2f(vertices[i + 0], vertices[i + 1]);
        MgeGL_Vertex3f(vertices[i + 2] + offset.x, vertices[i + 3] + offset.y, vertices[i + 4] + offset.z);
    }
    MgeGL_End();
}

static void HandleCameraMovement(Camera3D* camera)
{
    const float cameraSpeed = 5.0f * (float)Mge_GetDeltaTime();
    if (IsKeyDown(KEY_W))
        camera->position = Vector3_Add(camera->position, Vector3_Scale(camera->target, cameraSpeed));
    else if (IsKeyDown(KEY_S))
        camera->position = Vector3_Subtract(camera->position, Vector3_Scale(camera->target, cameraSpeed));
    else if (IsKeyDown(KEY_A))
        camera->position = Vector3_Subtract(camera->position,
            Vector3_Scale(Vector3Normalize(Vector3Cross(camera->target, camera->up)), cameraSpeed));
    else if (IsKeyDown(KEY_D))
        camera->position = Vector3_Add(camera->position,
            Vector3_Scale(Vector3Normalize(Vector3Cross(camera->target, camera->up)), cameraSpeed));

    float xpos = GetMouseX();
    float ypos = GetMouseY();

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed: y goes bottom-to-top
    lastX = xpos;
    lastY = ypos;

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;
    pitch = Clamp(pitch, -89.0f, 89.0f);

    Vector3 front;
    front.x = cosf(yaw * DEG2RAD) * cosf(pitch * DEG2RAD);
    front.y = sinf(pitch * DEG2RAD);
    front.z = sinf(yaw * DEG2RAD) * cosf(pitch * DEG2RAD);
    camera->target = Vector3Normalize(front);
}

int main(void)
{
    Mge_InitWindow(width, height, "MGEngine v1.0");
    Mge_SetTargetFPS(60);
    DisableCursor();

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 0.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, -1.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Shader lightShader = Mge_LoadShader("shaders/light_shader.vert", "shaders/light_shader.frag");

    while (!Mge_WindowShouldClose()) {
        Mge_BeginDrawing();
        Mge_ClearBackground(DARKGREEN);

        // TAB frees / re-locks the mouse cursor
        if (IsKeyPressed(KEY_TAB)) {
            Mge_ToggleCursor();
            firstMouse = true; // avoid a camera jump when re-locking
        }

        // only steer the camera while the cursor is captured
        if (IsCursorHidden())
            HandleCameraMovement(&camera);

        Mge_BeginMode3D(camera);
        Mge_BeginShaderMode(lightShader);

        MgeGL_Uniform4fv("lightColor",
            (Vector4){ 1.0f, 1.0f, (float)sin(Mge_GetTime()) * 2.0f, 1.0f });

        DrawCube((Vector3){ 0.0f, 0.0f, 0.0f }, WHITE);
        DrawCube((Vector3){ 0.0f, 0.0f, 1.5f }, RED);
        DrawCube((Vector3){ 0.0f, 0.0f, -1.5f }, GREEN);
        DrawCube((Vector3){ 1.5f, 0.0f, 0.0f }, BLUE);
        DrawCube((Vector3){ 0.0f, 1.5f, 0.0f }, (Color){ 0, 255, 255, 255 });
        DrawCube((Vector3){ -1.5f, 0.0f, 0.0f }, YELLOW);
        DrawCube((Vector3){ 0.0f, -1.5f, 0.0f }, (Color){ 255, 0, 255, 255 });
        DrawCube((Vector3){ 3.0f, 3.0f, 3.0f }, WHITE);

        Mge_EndShaderMode();
        Mge_EndMode3D();
        Mge_EndDrawing();
    }

    Mge_UnloadShader(lightShader);
    Mge_CloseWindow();
    return 0;
}
