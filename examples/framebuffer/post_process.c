// Framebuffers + post-processing.
//
// The lit scene is rendered into a RenderTexture, then that texture is drawn
// full-screen through an effect shader. The effect cycles every ~2 s:
//   none -> invert -> grayscale -> sharpen -> blur -> edge
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
    const int W = 1000, H = 700;
    Mge_InitWindow(W, H, "MGEngine - post-processing");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    RenderTexture scene = Mge_LoadRenderTexture(W, H);

    Object cubes[4] = {
        Mge_MakeObject3D((Vector3){ -3.0f, 0.0f, 0.0f }, (Vector3){ 1.6f, 1.6f, 1.6f }, (Color){ 210, 90, 90, 255 }),
        Mge_MakeObject3D((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 1.6f, 1.6f, 1.6f }, (Color){ 90, 200, 110, 255 }),
        Mge_MakeObject3D((Vector3){ 3.0f, 0.0f, 0.0f }, (Vector3){ 1.6f, 1.6f, 1.6f }, (Color){ 90, 140, 220, 255 }),
        Mge_MakeObject3D((Vector3){ 0.0f, -1.4f, 0.0f }, (Vector3){ 20.0f, 0.2f, 12.0f }, (Color){ 100, 100, 110, 255 }),
    };

    Light light = Mge_MakeDirectionalLight((Vector3){ -0.5f, -1.0f, -0.4f }, (Vector3){ 1.0f, 1.0f, 1.0f });
    light.ambient = 0.25f;

    Camera3D camera = {
        .position = { 0.0f, 3.0f, 9.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, camera.position));

    while (!Mge_WindowShouldClose()) {
        int effect = (int)(Mge_GetTime() / 2.0) % 6; // POSTFX_NONE .. POSTFX_EDGE

        Mge_BeginDrawing();

        // 1. render the scene into the texture
        Mge_BeginTextureMode(scene);
        Mge_ClearBackground((Color){ 18, 20, 24, 255 });
        Mge_BeginMode3D(camera);
        Mge_BeginLighting3DEx(&light, 1, camera);
        for (int i = 0; i < 4; i++)
            Mge_DrawObject(cubes[i]);
        Mge_EndLighting3D();
        Mge_EndMode3D();
        Mge_EndTextureMode();

        // 2. draw the texture over the window with the current effect
        Mge_ClearBackground(BLACK);
        Mge_DrawRenderTextureFX(scene, effect);

        Mge_EndDrawing();
    }

    Mge_UnloadRenderTexture(scene);
    Mge_CloseWindow();
    return 0;
}
