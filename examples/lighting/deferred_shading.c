// Advanced lighting: deferred shading.
//
// The scene is drawn ONCE into a G-buffer (world position, world normal, albedo
// + specular). A single full-screen pass then shades every pixel against every
// light -- so a field lit by two dozen small point lights costs one lighting
// evaluation per pixel, not per overlapping light per fragment.
//
//   G          cycle: final / position / normal / albedo view of the G-buffer
//   T          cycle tone map
//   UP / DOWN  exposure
#include "mge.h"
#include "mge_math.h"

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static const int WIDTH = 1100, HEIGHT = 700;
#define NLIGHTS 24
#define GRID 5

static void signal_handler(int sig)
{
    Mge_CloseWindow();
    exit(sig);
}

int main(void)
{
    Mge_InitWindow(WIDTH, HEIGHT, "MGEngine - deferred shading");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    GBuffer gbuf = Mge_LoadGBuffer(WIDTH, HEIGHT);
    RenderTexture hdr = Mge_LoadRenderTextureHDR(WIDTH, HEIGHT);

    Object floorObj = Mge_MakeShape3D(PRIM_PLANE, (Vector3){ 0, -0.6f, 0 }, (Vector3){ 26, 0.4f, 26 },
        (Color){ 150, 148, 145, 255 });
    Object items[GRID * GRID];
    for (int z = 0; z < GRID; z++)
        for (int x = 0; x < GRID; x++) {
            Vector3 p = { (x - GRID / 2) * 3.0f, 0.4f, (z - GRID / 2) * 3.0f };
            items[z * GRID + x] = ((x + z) & 1)
                ? Mge_MakeShape3D(PRIM_SPHERE, p, (Vector3){ 1.4f, 1.4f, 1.4f }, (Color){ 200, 205, 210, 255 })
                : Mge_MakeObject3D(p, (Vector3){ 1.3f, 1.3f, 1.3f }, (Color){ 205, 175, 150, 255 });
        }

    Light lights[NLIGHTS];
    Vector3 lightColor[NLIGHTS];
    for (int i = 0; i < NLIGHTS; i++) {
        float h = (float)i / NLIGHTS;
        lightColor[i] = (Vector3){ 0.5f + 0.5f * sinf(h * 6.28f),
            0.5f + 0.5f * sinf(h * 6.28f + 2.09f), 0.5f + 0.5f * sinf(h * 6.28f + 4.19f) };
        lights[i] = Mge_MakePointLight((Vector3){ 0, 0.7f, 0 },
            (Vector3){ lightColor[i].x * 3.0f, lightColor[i].y * 3.0f, lightColor[i].z * 3.0f });
        lights[i].linear = 0.7f;
        lights[i].quadratic = 1.8f; // tight falloff so each pool of light is small
    }

    Camera3D camera = { .up = { 0, 1, 0 }, .fovy = 55.0f, .projection = CAMERA_PERSPECTIVE };

    int view = 0; // 0 final, 1 position, 2 normal, 3 albedo
    int op = TONEMAP_ACES;
    float exposure = 1.0f;

    while (!Mge_WindowShouldClose()) {
        float t = (float)Mge_GetTime();
        if (IsKeyPressed(KEY_G)) {
            view = (view + 1) % 4;
            printf("view: %s\n", view == 0 ? "final" : view == 1 ? "position" : view == 2 ? "normal" : "albedo");
        }
        if (IsKeyPressed(KEY_T))
            op = (op + 1) % 3;
        if (IsKeyDown(KEY_UP))
            exposure += 0.9f * (float)Mge_GetDeltaTime();
        if (IsKeyDown(KEY_DOWN))
            exposure = fmaxf(0.05f, exposure - 0.9f * (float)Mge_GetDeltaTime());

        for (int i = 0; i < NLIGHTS; i++) {
            float a = t * 0.4f + (float)i / NLIGHTS * 6.28f;
            float r = 3.0f + 5.0f * (float)((i * 37) % 100) / 100.0f;
            lights[i].position = (Vector3){ cosf(a) * r, 0.7f, sinf(a) * r };
        }

        camera.position = (Vector3){ sinf(t * 0.15f) * 14.0f, 8.0f, cosf(t * 0.15f) * 14.0f };
        camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, camera.position));

        Mge_BeginDrawing();

        // 1. geometry pass -> the G-buffer
        Mge_BeginMode3D(camera);
        Mge_BeginGeometryPass(&gbuf, camera);
        Mge_DrawObject(floorObj);
        for (int i = 0; i < GRID * GRID; i++)
            Mge_DrawObject(items[i]);
        Mge_EndGeometryPass();
        Mge_EndMode3D();

        if (view == 0) {
            // 2. lighting pass -> HDR, then tone-map out
            Mge_BeginTextureMode(hdr);
            Mge_ClearBackground((Color){ 6, 7, 10, 255 });
            Mge_DeferredLighting(gbuf, lights, NLIGHTS, camera);
            Mge_BlitGBufferDepth(gbuf); // so the lamp markers depth-test
            Mge_BeginMode3D(camera);
            for (int i = 0; i < NLIGHTS; i++)
                Draw_Cube(lights[i].position, (Vector3){ 0.12f, 0.12f, 0.12f }, WHITE);
            Mge_EndMode3D();
            Mge_EndTextureMode();
            Mge_DrawRenderTextureHDR(hdr, op, exposure);
        } else {
            // raw G-buffer channels for inspection
            Mge_ClearBackground((Color){ 0, 0, 0, 255 });
            RenderTexture fake = { .texture = (view == 1) ? gbuf.position
                                            : (view == 2) ? gbuf.normal
                                                          : gbuf.albedoSpec,
                .width = WIDTH, .height = HEIGHT };
            Mge_DrawRenderTextureFX(fake, POSTFX_NONE);
        }

        Mge_EndDrawing();
    }

    Mge_UnloadGBuffer(&gbuf);
    Mge_UnloadRenderTexture(hdr);
    Mge_CloseWindow();
    return 0;
}
