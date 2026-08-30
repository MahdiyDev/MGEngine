// Depth testing: the depth buffer, visualizing it, and stopping z-fighting.
//
//  - Every ~3s the scene flips between normal lighting and a grayscale view of
//    the linearized depth buffer (near = dark, far = light).
//  - LEFT:  a red and a blue cube occupy the exact same space -> they z-fight
//           and the surface flickers between the two colours.
//  - RIGHT: the same pair, but the blue cube is drawn with a polygon offset so
//           it always loses the depth test -> a clean, stable red cube.
//
// `Mge_SetClipPlanes` pushes the near plane out to 0.2, which is also what buys
// back most of the depth precision that a tiny near plane throws away.
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

static void draw_scene(void)
{
    // a ground slab + a few cubes marching into the distance
    Draw_Cube((Vector3){ 0.0f, -1.1f, -6.0f }, (Vector3){ 30.0f, 0.2f, 40.0f }, (Color){ 90, 95, 105, 255 });
    for (int i = 0; i < 6; i++)
        Draw_Cube((Vector3){ -4.0f, 0.0f, -2.0f - (float)i * 4.0f }, (Vector3){ 1.5f, 1.5f, 1.5f },
            (Color){ 200, 180, 120, 255 });

    // LEFT: coplanar cubes, no fix -> z-fighting
    Draw_Cube((Vector3){ 2.5f, 0.0f, -3.0f }, (Vector3){ 1.6f, 1.6f, 1.6f }, RED);
    Draw_Cube((Vector3){ 2.5f, 0.0f, -3.0f }, (Vector3){ 1.6f, 1.6f, 1.6f }, BLUE);

    // RIGHT: same pair, but the blue cube is pushed back in depth
    Draw_Cube((Vector3){ 5.5f, 0.0f, -3.0f }, (Vector3){ 1.6f, 1.6f, 1.6f }, RED);
    Mge_SetPolygonOffset(1.0f, 1.0f);
    Draw_Cube((Vector3){ 5.5f, 0.0f, -3.0f }, (Vector3){ 1.6f, 1.6f, 1.6f }, BLUE);
    Mge_DisablePolygonOffset();
}

int main(void)
{
    Mge_InitWindow(1000, 700, "MGEngine - depth buffer");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Mge_SetClipPlanes(0.2, 60.0); // wider near plane -> better depth precision

    Camera3D camera = {
        .position = { 4.0f, 4.5f, 8.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 1.0f, 0.0f, -6.0f }, camera.position));

    Light light = Mge_MakeDirectionalLight((Vector3){ -0.5f, -1.0f, -0.4f }, (Vector3){ 1.0f, 1.0f, 1.0f });
    light.ambient = 0.25f;

    while (!Mge_WindowShouldClose()) {
        bool showDepth = fmod(Mge_GetTime(), 6.0) >= 3.0;

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 20, 22, 26, 255 });

        Mge_BeginMode3D(camera);
        if (showDepth) {
            Mge_BeginDepthPreview();
            draw_scene();
            Mge_EndDepthPreview();
        } else {
            Mge_BeginLighting3D(light, camera);
            draw_scene();
            Mge_EndLighting3D();
        }
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
