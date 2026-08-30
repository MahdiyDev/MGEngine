// Advanced lighting: point (omnidirectional) shadows.
//
// A point light shadows in every direction, so pass 1 renders the occluders
// into a depth CUBEMAP -- once per face -- storing the distance from the light.
// Pass 2 samples the cube along fragment->light and compares (20-tap PCF).
//
// Here a bobbing lamp sits inside a room; the cubes cast shadows onto the walls,
// floor and ceiling as it moves. Press SPACE to stop the bob.
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
    // the room -- culling is off, so its inside faces are visible from within
    Draw_Cube((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 16.0f, 16.0f, 16.0f }, (Color){ 150, 150, 155, 255 });
    Draw_Cube((Vector3){ -3.0f, -5.0f, 2.0f }, (Vector3){ 2.0f, 2.0f, 2.0f }, (Color){ 200, 110, 90, 255 });
    Draw_Cube((Vector3){ 3.5f, -2.0f, -2.5f }, (Vector3){ 1.6f, 1.6f, 1.6f }, (Color){ 90, 150, 200, 255 });
    Draw_Cube((Vector3){ 0.0f, 3.5f, 3.0f }, (Vector3){ 1.4f, 1.4f, 1.4f }, (Color){ 110, 200, 130, 255 });
    Draw_Cube((Vector3){ 2.0f, 1.0f, -4.0f }, (Vector3){ 1.2f, 1.2f, 1.2f }, (Color){ 210, 190, 110, 255 });
}

int main(void)
{
    Mge_InitWindow(1000, 720, "MGEngine - point shadows");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    PointShadowMap ps = Mge_LoadPointShadowMap(1024);

    Light lamp = Mge_MakePointLight((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 1.0f, 0.95f, 0.85f });
    lamp.ambient = 0.06f;
    lamp.diffuse = 1.0f;
    lamp.specular = 0.5f;
    lamp.linear = 0.022f;
    lamp.quadratic = 0.0019f;

    Material mat = Mge_DefaultMaterial();
    mat.shininess = 24.0f;

    Camera3D camera = {
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    bool bob = true;

    while (!Mge_WindowShouldClose()) {
        float t = (float)Mge_GetTime();

        if (IsKeyPressed(KEY_SPACE))
            bob = !bob;
        lamp.position = (Vector3){ 0.0f, bob ? sinf(t * 0.8f) * 3.0f : 0.0f, 0.0f };

        float a = t * 0.2f;
        camera.position = (Vector3){ sinf(a) * 6.5f, 2.0f, cosf(a) * 6.5f };
        camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0.0f, 0.0f, 0.0f }, camera.position));

        Mge_BeginDrawing();

        // pass 1: distance-from-light into the 6 cube faces
        Mge_BeginPointShadowPass(&ps, lamp, 22.0f);
        for (int f = 0; f < 6; f++) {
            Mge_SetPointShadowFace(f);
            draw_scene();
        }
        Mge_EndPointShadowPass();

        // pass 2: lit + shadowed
        Mge_ClearBackground((Color){ 8, 8, 10, 255 });
        Mge_BeginMode3D(camera);
        Mge_BeginLighting3DPointShadowed(&lamp, 1, camera, ps);
        Mge_SetMaterial(mat);
        draw_scene();
        Mge_EndLighting3D();
        Draw_Cube(lamp.position, (Vector3){ 0.25f, 0.25f, 0.25f }, (Color){ 255, 245, 210, 255 });
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_UnloadPointShadowMap(&ps);
    Mge_CloseWindow();
    return 0;
}
