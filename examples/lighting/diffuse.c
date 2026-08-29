// Diffuse (Lambert) lighting.
//
// The diffuse term is proportional to how directly a surface faces the light:
// `max(dot(N, L), 0)`. A face pointing straight at the light is fully lit; a
// face at a grazing angle is dark; a face pointing away gets nothing. A small
// ambient term keeps the unlit side from being pure black. There is no
// highlight -- specular is switched off here.
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
    Mge_InitWindow(1000, 700, "MGEngine - diffuse lighting");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Object cubes[3] = {
        Mge_MakeObject3D((Vector3){ -2.5f, 0.0f, 0.0f }, (Vector3){ 1.5f, 1.5f, 1.5f }, RED),
        Mge_MakeObject3D((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 1.5f, 1.5f, 1.5f }, GREEN),
        Mge_MakeObject3D((Vector3){ 2.5f, 0.0f, 0.0f }, (Vector3){ 1.5f, 1.5f, 1.5f }, BLUE),
    };

    Light light = Mge_MakeLight((Vector3){ 0.0f, 5.0f, 0.0f }, (Vector3){ 1.0f, 1.0f, 1.0f });
    light.ambient = 0.1f;
    light.diffuse = 1.0f;
    light.specular = 0.0f; // no highlight -- diffuse only

    Camera3D camera = {
        .position = { 0.0f, 3.0f, 9.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, camera.position));

    while (!Mge_WindowShouldClose()) {
        // sweep the light left-right so faces light up as it passes them
        double t = Mge_GetTime();
        light.position = (Vector3){ (float)sin(t) * 5.0f, 3.5f, 3.0f };

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 15, 15, 18, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(light, camera);
        for (int i = 0; i < 3; i++)
            Mge_DrawObject(cubes[i]);
        Mge_EndLighting3D();
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
