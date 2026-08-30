// Geometry shaders.
//
//   left   -- normal object, plus Mge_BeginNormals3D: a yellow line grows from
//             every vertex along its normal
//   right  -- Mge_BeginExplode3D: each triangle is pushed out along its face
//             normal; the magnitude pulses, so the cube blows apart and reforms
#include "mge.h"
#include "mge_math.h"

#include <math.h>
#include <signal.h>
#include <stdlib.h>

static void signal_handler(int sig)
{
    Mge_CloseWindow();
    exit(sig);
}

int main(void)
{
    Mge_InitWindow(1000, 700, "MGEngine - geometry shaders");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Light light = Mge_MakeDirectionalLight((Vector3){ -0.5f, -1.0f, -0.4f }, (Vector3){ 1.0f, 1.0f, 1.0f });
    light.ambient = 0.3f;

    Camera3D camera = {
        .position = { 0.0f, 3.0f, 8.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, camera.position));

    const Vector3 LEFT = { -2.4f, 0.0f, 0.0f };
    const Vector3 RIGHT = { 2.4f, 0.0f, 0.0f };
    const Vector3 SIZE = { 1.8f, 1.8f, 1.8f };

    while (!Mge_WindowShouldClose()) {
        float t = (float)Mge_GetTime();
        float boom = (sinf(t * 1.5f) * 0.5f + 0.5f) * 0.7f;

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 16, 17, 21, 255 });

        Mge_BeginMode3D(camera);

        // left: lit cube + its normals
        Mge_BeginLighting3DEx(&light, 1, camera);
        Draw_Cube(LEFT, SIZE, (Color){ 120, 170, 220, 255 });
        Mge_EndLighting3D();
        Mge_BeginNormals3D(0.35f, (Color){ 255, 220, 80, 255 });
        Draw_Cube(LEFT, SIZE, WHITE);
        Mge_EndNormals3D();

        // right: exploding cube
        Mge_BeginExplode3D(boom);
        Draw_Cube(RIGHT, SIZE, (Color){ 220, 130, 110, 255 });
        Mge_EndExplode3D();

        Mge_EndMode3D();
        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
