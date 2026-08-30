// Advanced lighting: shadow mapping (directional light).
//
// Pass 1: render occluders from the sun's point of view into a depth texture.
// Pass 2: render the scene lit; each fragment compares its light-space depth to
// the stored one -- farther means occluded, so it's shadowed (3x3 PCF).
//
// The depth texture is shown bottom-left. Press SPACE to freeze the sun.
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

// the occluders -- drawn identically in both passes
static void draw_scene(void)
{
    Draw_Cube((Vector3){ 0.0f, -0.05f, 0.0f }, (Vector3){ 18.0f, 0.1f, 18.0f }, (Color){ 170, 170, 175, 255 });
    Draw_Cube((Vector3){ -2.5f, 1.0f, 1.0f }, (Vector3){ 2.0f, 2.0f, 2.0f }, (Color){ 200, 120, 90, 255 });
    Draw_Cube((Vector3){ 2.5f, 0.75f, -1.5f }, (Vector3){ 1.5f, 1.5f, 1.5f }, (Color){ 90, 150, 200, 255 });
    Draw_Cube((Vector3){ 0.5f, 2.4f, -0.5f }, (Vector3){ 1.0f, 1.0f, 1.0f }, (Color){ 120, 200, 130, 255 });
}

int main(void)
{
    Mge_InitWindow(1000, 720, "MGEngine - shadow mapping");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    ShadowMap shadow = Mge_LoadShadowMap(2048);

    Light sun = Mge_MakeDirectionalLight((Vector3){ -0.5f, -1.0f, -0.35f }, (Vector3){ 1.0f, 0.97f, 0.9f });
    sun.ambient = 0.25f;
    sun.diffuse = 0.9f;
    sun.specular = 0.4f;

    Material mat = Mge_DefaultMaterial();
    mat.shininess = 24.0f;

    Camera3D camera = {
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    const Vector3 sceneCenter = { 0.0f, 1.0f, 0.0f };
    const float sceneRadius = 11.0f;

    bool sunFrozen = false;

    while (!Mge_WindowShouldClose()) {
        float t = (float)Mge_GetTime();

        if (IsKeyPressed(KEY_SPACE))
            sunFrozen = !sunFrozen;
        if (!sunFrozen) {
            float s = t * 0.4f;
            sun.direction = (Vector3){ sinf(s) * 0.6f, -1.0f, cosf(s) * 0.6f };
        }

        float a = t * 0.25f;
        camera.position = (Vector3){ sinf(a) * 16.0f, 8.0f, cosf(a) * 16.0f };
        camera.target = Vector3Normalize(Vector3_Subtract(sceneCenter, camera.position));

        Mge_BeginDrawing();

        // pass 1: depth from the light
        Mge_BeginShadowPass(&shadow, sun, sceneCenter, sceneRadius);
        draw_scene();
        Mge_EndShadowPass();

        // pass 2: lit + shadowed
        Mge_ClearBackground((Color){ 30, 33, 40, 255 });
        Mge_BeginMode3D(camera);
        Mge_BeginLighting3DShadowed(&sun, 1, camera, shadow);
        Mge_SetMaterial(mat);
        draw_scene();
        Mge_EndLighting3D();
        Mge_EndMode3D();

        Mge_DrawShadowMap(shadow, 12, 720 - 12 - 220, 220); // debug view, bottom-left

        Mge_EndDrawing();
    }

    Mge_UnloadShadowMap(&shadow);
    Mge_CloseWindow();
    return 0;
}
