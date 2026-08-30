// Spotlights -- a point light clipped to a cone.
//
// A spotlight aims along `direction` and is only bright inside its cone:
// full strength within `innerCutoff`, fading to nothing by `outerCutoff`.
//   - LEFT  cone: inner == outer  -> a hard, crisp circle of light
//   - RIGHT cone: inner < outer   -> a soft, feathered edge
// Both sweep back and forth over a tiled floor so the edges are easy to see.
//
// `Mge_MakeFlashlight(camera, colour)` builds the same thing pinned to the
// camera, pointing wherever you look.
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
    Mge_InitWindow(1100, 700, "MGEngine - spotlights (hard vs soft edge)");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    // a tiled floor: a grid of flat cubes
    Object tiles[8 * 8];
    int n = 0;
    for (int z = 0; z < 8; z++)
        for (int x = 0; x < 8; x++)
            tiles[n++] = Mge_MakeObject3D(
                (Vector3){ (float)(x - 4) * 1.5f + 0.75f, -1.0f, (float)(z - 4) * 1.5f + 0.75f },
                (Vector3){ 1.4f, 0.15f, 1.4f },
                ((x + z) & 1) ? (Color){ 180, 180, 190, 255 } : (Color){ 120, 120, 130, 255 });

    Vector3 down = { 0.0f, -1.0f, 0.0f };
    Light hard = Mge_MakeSpotLight((Vector3){ -3.0f, 4.0f, 0.0f }, down, (Vector3){ 1.0f, 0.95f, 0.8f }, 18.0f, 18.0f);
    Light soft = Mge_MakeSpotLight((Vector3){ 3.0f, 4.0f, 0.0f }, down, (Vector3){ 0.8f, 0.9f, 1.0f }, 10.0f, 26.0f);

    Camera3D camera = {
        .position = { 0.0f, 7.5f, 11.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 60.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, camera.position));

    while (!Mge_WindowShouldClose()) {
        double t = Mge_GetTime();
        float sweep = (float)sin(t) * 0.35f;
        hard.direction = (Vector3){ 0.0f, -1.0f, sweep };
        soft.direction = (Vector3){ 0.0f, -1.0f, sweep };

        Light lights[2] = { hard, soft };

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 8, 8, 10, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3DEx(lights, 2, camera);
        for (int i = 0; i < n; i++)
            Mge_DrawObject(tiles[i]);
        Mge_EndLighting3D();
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
