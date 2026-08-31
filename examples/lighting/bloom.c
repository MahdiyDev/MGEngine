// Advanced lighting: bloom.
//
// The bright parts of an HDR image bleed a soft glow into their surroundings.
// The lit scene is rendered into a floating-point RenderTexture, then Mge_DrawBloom
// extracts the pixels above `threshold`, Gaussian-blurs them (ping-pong, half res)
// and composites `scene + blur * intensity` with tone mapping -- replacing the
// plain Mge_DrawRenderTextureHDR step. (LearnOpenGL Advanced-Lighting/Bloom.)
//
//   SPACE       bloom on / off
//   T           cycle tone map
//   UP / DOWN   exposure
//   [ / ]       bloom threshold
//   - / =       bloom intensity
#include "mge.h"
#include "mge_math.h"

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static const int WIDTH = 1100, HEIGHT = 700;

static void signal_handler(int sig)
{
    Mge_CloseWindow();
    exit(sig);
}

int main(void)
{
    Mge_InitWindow(WIDTH, HEIGHT, "MGEngine - bloom");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    RenderTexture hdr = Mge_LoadRenderTextureHDR(WIDTH, HEIGHT);
    BloomFX bloom = Mge_LoadBloom(WIDTH, HEIGHT);

    // point lights, each with a small marker cube sitting right on it -- so close
    // that the marker is lit far past 1.0 and glows
    Light lights[4] = {
        Mge_MakePointLight((Vector3){ -3.0f, 0.6f, 1.5f }, (Vector3){ 12.0f, 10.0f, 4.0f }),
        Mge_MakePointLight((Vector3){ 2.5f, 0.6f, -1.0f }, (Vector3){ 2.0f, 3.0f, 8.0f }),
        Mge_MakePointLight((Vector3){ 0.0f, 0.6f, -5.0f }, (Vector3){ 3.0f, 8.0f, 3.0f }),
        Mge_MakePointLight((Vector3){ 4.0f, 3.5f, 4.0f }, (Vector3){ 1.2f, 1.2f, 1.3f }),
    };
    for (int i = 0; i < 4; i++)
        lights[i].ambient = (i == 3) ? 0.06f : 0.0f;

    Object floorObj = Mge_MakeShape3D(PRIM_PLANE, (Vector3){ 0, -0.2f, -2 }, (Vector3){ 16, 0.2f, 16 },
        (Color){ 170, 165, 160, 255 });
    Object cubes[4];
    for (int i = 0; i < 4; i++)
        cubes[i] = Mge_MakeObject3D((Vector3){ (float)(i - 1) * 2.4f, 0.7f, -2.5f },
            (Vector3){ 1.2f, 1.2f, 1.2f }, (Color){ 150, 150, 155, 255 });

    Camera3D camera = {
        .position = { 0.0f, 3.2f, 8.0f },
        .target = { 0.0f, -0.2f, -1.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    bool bloomOn = true;
    int op = TONEMAP_ACES;
    float exposure = 1.0f;

    while (!Mge_WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) {
            bloomOn = !bloomOn;
            printf("bloom: %s\n", bloomOn ? "ON" : "OFF");
        }
        if (IsKeyPressed(KEY_T))
            op = (op + 1) % 3;
        if (IsKeyDown(KEY_UP))
            exposure += 0.9f * (float)Mge_GetDeltaTime();
        if (IsKeyDown(KEY_DOWN))
            exposure = fmaxf(0.05f, exposure - 0.9f * (float)Mge_GetDeltaTime());
        if (IsKeyDown(KEY_LEFT_BRACKET))
            bloom.threshold = fmaxf(0.0f, bloom.threshold - 0.6f * (float)Mge_GetDeltaTime());
        if (IsKeyDown(KEY_RIGHT_BRACKET))
            bloom.threshold += 0.6f * (float)Mge_GetDeltaTime();
        if (IsKeyDown(KEY_MINUS))
            bloom.intensity = fmaxf(0.0f, bloom.intensity - 0.8f * (float)Mge_GetDeltaTime());
        if (IsKeyDown(KEY_EQUAL))
            bloom.intensity += 0.8f * (float)Mge_GetDeltaTime();

        Mge_BeginDrawing();

        Mge_BeginTextureMode(hdr);
        Mge_ClearBackground((Color){ 3, 3, 5, 255 });
        Mge_BeginMode3D(camera);
        Mge_BeginLighting3DEx(lights, 4, camera);
        Mge_DrawObject(floorObj);
        for (int i = 0; i < 4; i++)
            Mge_DrawObject(cubes[i]);
        for (int i = 0; i < 3; i++) // the three coloured lamps sit in their own glow
            Draw_Cube(lights[i].position, (Vector3){ 0.14f, 0.14f, 0.14f }, WHITE);
        Mge_EndLighting3D();
        Mge_EndMode3D();
        Mge_EndTextureMode();

        if (bloomOn)
            Mge_DrawBloom(hdr, &bloom, op, exposure);
        else
            Mge_DrawRenderTextureHDR(hdr, op, exposure);

        Mge_EndDrawing();
    }

    Mge_UnloadBloom(&bloom);
    Mge_UnloadRenderTexture(hdr);
    Mge_CloseWindow();
    return 0;
}
