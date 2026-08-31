// Advanced lighting: screen-space ambient occlusion (SSAO).
//
// Built on the deferred G-buffer: Mge_ComputeSSAO reads world position + normal,
// samples a hemisphere around every pixel and measures how buried it is, then
// Mge_DeferredLightingAO folds that into the ambient term. Creases -- like the
// gaps between the melon slices -- pick up soft contact shadow that no light
// source actually produces. (LearnOpenGL Advanced-Lighting/SSAO.)
//
//   SPACE       SSAO on / off
//   B           show the raw AO buffer
//   [ / ]       radius        - / =   power
#include "mge.h"
#include "mge_math.h"

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static const int WIDTH = 1100, HEIGHT = 720;

static void signal_handler(int sig)
{
    Mge_CloseWindow();
    exit(sig);
}

static Model load_melon(void)
{
    Model m = Mge_LoadModel("assets/sliced_musk_melon/scene.gltf");
    if (m.meshCount == 0)
        m = Mge_LoadModel("../../assets/sliced_musk_melon/scene.gltf");
    return m;
}

int main(void)
{
    Mge_InitWindow(WIDTH, HEIGHT, "MGEngine - SSAO");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Model melon = load_melon();
    Vector3 c = Vector3_Scale(Vector3_Add(melon.bboxMin, melon.bboxMax), 0.5f);
    float rad = Vector3_Length(Vector3_Subtract(melon.bboxMax, melon.bboxMin)) * 0.5f;
    if (rad < 0.001f)
        rad = 2.0f;

    GBuffer gbuf = Mge_LoadGBuffer(WIDTH, HEIGHT);
    RenderTexture hdr = Mge_LoadRenderTextureHDR(WIDTH, HEIGHT);
    SSAO ssao = Mge_LoadSSAO(WIDTH, HEIGHT);
    ssao.radius = rad * 0.35f;
    ssao.power = 2.5f;

    // a soft key light + strong ambient so SSAO has something to bite into
    Light lights[2] = {
        Mge_MakeDirectionalLight((Vector3){ -0.5f, -1.0f, -0.4f }, (Vector3){ 0.9f, 0.9f, 0.95f }),
        Mge_MakePointLight((Vector3){ c.x + rad, c.y + rad, c.z + rad }, (Vector3){ 1.0f, 0.9f, 0.7f }),
    };
    lights[0].ambient = 0.55f; // <- SSAO darkens this
    lights[0].diffuse = 0.5f;
    lights[1].ambient = 0.0f;

    Camera3D camera = { .up = { 0, 1, 0 }, .fovy = 50.0f, .projection = CAMERA_PERSPECTIVE };

    bool on = true, showRaw = false;

    while (!Mge_WindowShouldClose()) {
        float t = (float)Mge_GetTime();
        if (IsKeyPressed(KEY_SPACE)) { on = !on; printf("SSAO: %s\n", on ? "ON" : "OFF"); }
        if (IsKeyPressed(KEY_B)) showRaw = !showRaw;
        if (IsKeyDown(KEY_LEFT_BRACKET))  ssao.radius = fmaxf(0.02f, ssao.radius - rad * 0.3f * (float)Mge_GetDeltaTime());
        if (IsKeyDown(KEY_RIGHT_BRACKET)) ssao.radius += rad * 0.3f * (float)Mge_GetDeltaTime();
        if (IsKeyDown(KEY_MINUS)) ssao.power = fmaxf(0.2f, ssao.power - 1.5f * (float)Mge_GetDeltaTime());
        if (IsKeyDown(KEY_EQUAL)) ssao.power += 1.5f * (float)Mge_GetDeltaTime();

        camera.position = (Vector3){ c.x + sinf(t * 0.3f) * rad * 2.4f, c.y + rad * 0.5f,
            c.z + cosf(t * 0.3f) * rad * 2.4f };
        camera.target = Vector3Normalize(Vector3_Subtract(c, camera.position));

        Mge_BeginDrawing();

        Mge_BeginMode3D(camera);
        Mge_BeginGeometryPass(&gbuf, camera);
        Draw_Cube((Vector3){ c.x, melon.bboxMin.y - rad * 0.05f, c.z },
            (Vector3){ rad * 6.0f, rad * 0.1f, rad * 6.0f }, (Color){ 180, 178, 172, 255 });
        Mge_DrawModel(melon);
        Mge_EndGeometryPass();
        Mge_EndMode3D();

        if (on)
            Mge_ComputeSSAO(&ssao, gbuf, camera);

        if (showRaw) {
            Mge_ClearBackground((Color){ 0, 0, 0, 255 });
            Mge_DrawRenderTextureFX(ssao.aoBlur, POSTFX_NONE);
        } else {
            Mge_BeginTextureMode(hdr);
            Mge_ClearBackground((Color){ 8, 9, 12, 255 });
            if (on)
                Mge_DeferredLightingAO(gbuf, lights, 2, camera, ssao.aoBlur.texture.id);
            else
                Mge_DeferredLighting(gbuf, lights, 2, camera);
            Mge_EndTextureMode();
            Mge_DrawRenderTextureHDR(hdr, TONEMAP_ACES, 1.0f);
        }

        Mge_EndDrawing();
    }

    Mge_UnloadSSAO(&ssao);
    Mge_UnloadGBuffer(&gbuf);
    Mge_UnloadRenderTexture(hdr);
    Mge_UnloadModel(&melon);
    Mge_CloseWindow();
    return 0;
}
