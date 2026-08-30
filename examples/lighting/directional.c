// Directional light -- the sun.
//
// A directional light has only a *direction*: its rays are parallel and never
// fall off with distance, so every cube here is lit the same amount regardless
// of how far it sits from the camera. Only the facing of each face matters.
// The direction slowly rotates so you can watch the lit side travel around.
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
    Mge_InitWindow(1000, 700, "MGEngine - directional light");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Object cubes[5];
    for (int i = 0; i < 5; i++)
        cubes[i] = Mge_MakeObject3D((Vector3){ (float)(i - 2) * 2.4f, 0.0f, 0.0f },
            (Vector3){ 1.5f, 1.5f, 1.5f }, (Color){ 200, 200, 210, 255 });

    Light sun = Mge_MakeDirectionalLight((Vector3){ -1.0f, -1.0f, -0.3f }, (Vector3){ 1.0f, 0.97f, 0.9f });
    sun.ambient = 0.08f;

    Camera3D camera = {
        .position = { 0.0f, 3.5f, 10.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, camera.position));

    while (!Mge_WindowShouldClose()) {
        double t = Mge_GetTime();
        sun.direction = (Vector3){ (float)cos(t) * 1.0f, -0.7f, (float)sin(t) * 1.0f };

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 15, 16, 20, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(sun, camera); // one light -> the simple form
        for (int i = 0; i < 5; i++)
            Mge_DrawObject(cubes[i]);
        Mge_EndLighting3D();
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
