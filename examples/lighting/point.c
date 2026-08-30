// Point lights -- bulbs with distance falloff.
//
// A point light sits at a position and radiates in every direction, fading with
// distance (1 / (constant + linear*d + quadratic*d^2)). Here two point lights,
// one warm and one cool, drift along a long row of cubes: a cube is bright only
// while a light is near it. A small marker cube rides along at each light.
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
    Mge_InitWindow(1100, 700, "MGEngine - point lights");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Object cubes[9];
    for (int i = 0; i < 9; i++)
        cubes[i] = Mge_MakeObject3D((Vector3){ (float)(i - 4) * 2.2f, 0.0f, 0.0f },
            (Vector3){ 1.4f, 1.4f, 1.4f }, (Color){ 190, 190, 195, 255 });

    Light warm = Mge_MakePointLight((Vector3){ 0, 2.0f, 2.0f }, (Vector3){ 1.0f, 0.6f, 0.25f });
    Light cool = Mge_MakePointLight((Vector3){ 0, 2.0f, 2.0f }, (Vector3){ 0.3f, 0.5f, 1.0f });

    Camera3D camera = {
        .position = { 0.0f, 4.0f, 13.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 60.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, camera.position));

    while (!Mge_WindowShouldClose()) {
        double t = Mge_GetTime();
        warm.position = (Vector3){ (float)sin(t) * 8.0f, 2.0f, 2.0f };
        cool.position = (Vector3){ (float)sin(t + 3.14159) * 8.0f, 2.0f, 2.0f };

        Light lights[2] = { warm, cool };

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 10, 10, 12, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3DEx(lights, 2, camera);
        for (int i = 0; i < 9; i++)
            Mge_DrawObject(cubes[i]);
        Mge_EndLighting3D();

        // unlit markers where the bulbs are
        Draw_Cube(warm.position, (Vector3){ 0.3f, 0.3f, 0.3f }, (Color){ 255, 150, 60, 255 });
        Draw_Cube(cool.position, (Vector3){ 0.3f, 0.3f, 0.3f }, (Color){ 80, 130, 255, 255 });
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
