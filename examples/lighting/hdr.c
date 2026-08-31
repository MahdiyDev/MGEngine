// Advanced lighting: HDR + tone mapping.
//
// The lit scene is rendered into a floating-point RenderTexture, so a bright
// light is stored at its true intensity instead of being clamped to white. A
// full-screen tone-map pass then squeezes that range back into [0,1] for the
// display -- keeping highlight detail and letting `exposure` pick which range
// you see, like a camera stop. (LearnOpenGL Advanced-Lighting/HDR.)
//
//   SPACE      tone map  <->  raw clamp (the "no HDR" look)
//   T          cycle Reinhard / Exposure / ACES
//   UP / DOWN  exposure
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
    Mge_InitWindow(WIDTH, HEIGHT, "MGEngine - HDR");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    RenderTexture hdr = Mge_LoadRenderTextureHDR(WIDTH, HEIGHT);

    const Color slab = { 190, 185, 175, 255 }; // the corridor's surfaces

    // point lights down the tunnel -- the first is *very* bright (HDR territory)
    Light lights[4] = {
        Mge_MakePointLight((Vector3){ 0.0f, 3.2f, 1.0f }, (Vector3){ 24.0f, 24.0f, 22.0f }),
        Mge_MakePointLight((Vector3){ -1.6f, 1.0f, -6.0f }, (Vector3){ 1.6f, 0.35f, 0.35f }),
        Mge_MakePointLight((Vector3){ 1.4f, 1.6f, -12.0f }, (Vector3){ 0.2f, 0.7f, 0.25f }),
        Mge_MakePointLight((Vector3){ 0.0f, 2.6f, -18.0f }, (Vector3){ 0.25f, 0.3f, 1.1f }),
    };
    for (int i = 0; i < 4; i++)
        lights[i].ambient = (i == 0) ? 0.05f : 0.0f;

    Camera3D camera = {
        .position = { 0.0f, 1.6f, 3.0f },
        .target = { 0.0f, 0.0f, -1.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 60.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    bool toneMapped = true;
    int op = TONEMAP_ACES;
    float exposure = 1.0f;

    while (!Mge_WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) {
            toneMapped = !toneMapped;
            printf("output: %s\n", toneMapped ? "tone mapped" : "raw clamp");
        }
        if (IsKeyPressed(KEY_T)) {
            op = (op + 1) % 3;
            printf("tone map: %s\n", op == 0 ? "Reinhard" : op == 1 ? "Exposure" : "ACES");
        }
        if (IsKeyDown(KEY_UP))
            exposure += 0.9f * (float)Mge_GetDeltaTime();
        if (IsKeyDown(KEY_DOWN))
            exposure = fmaxf(0.05f, exposure - 0.9f * (float)Mge_GetDeltaTime());

        Mge_BeginDrawing();

        Mge_BeginTextureMode(hdr);
        Mge_ClearBackground((Color){ 4, 4, 6, 255 });
        Mge_BeginMode3D(camera);
        Mge_BeginLighting3DEx(lights, 4, camera);
        Draw_Cube((Vector3){ 0, -1.0f, -8 }, (Vector3){ 7, 0.3f, 34 }, slab);   // floor
        Draw_Cube((Vector3){ 0, 4.3f, -8 }, (Vector3){ 7, 0.3f, 34 }, slab);    // ceiling
        Draw_Cube((Vector3){ -3.4f, 1.6f, -8 }, (Vector3){ 0.3f, 5.6f, 34 }, slab); // left
        Draw_Cube((Vector3){ 3.4f, 1.6f, -8 }, (Vector3){ 0.3f, 5.6f, 34 }, slab);  // right
        Draw_Cube((Vector3){ 0, 1.6f, -25 }, (Vector3){ 7, 5.6f, 0.3f }, slab);     // back
        Mge_EndLighting3D();
        for (int i = 0; i < 4; i++)
            Draw_Cube(lights[i].position, (Vector3){ 0.15f, 0.15f, 0.15f }, WHITE);
        Mge_EndMode3D();
        Mge_EndTextureMode();

        if (toneMapped)
            Mge_DrawRenderTextureHDR(hdr, op, exposure);
        else
            Mge_DrawRenderTextureFX(hdr, POSTFX_NONE); // straight blit -> GL clamps to [0,1]

        Mge_EndDrawing();
    }

    Mge_UnloadRenderTexture(hdr);
    Mge_CloseWindow();
    return 0;
}
