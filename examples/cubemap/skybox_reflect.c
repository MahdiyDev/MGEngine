// Cube maps: a skybox, plus reflection and refraction environment mapping.
//
//   left cube  -- ENVMAP_REFLECT: a chrome mirror of the sky
//   right cube -- ENVMAP_REFRACT: glass, bends the sky through it (ratio 1/1.52)
//
// The camera orbits so the mirrored / refracted sky shifts as you'd expect.
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

static Cubemap load_sky(void)
{
    Cubemap c = Mge_LoadCubemapDir("assets/skybox");
    if (c.id == 0 || c.size == 0)
        c = Mge_LoadCubemapDir("../../assets/skybox");
    return c;
}

int main(void)
{
    Mge_InitWindow(1000, 700, "MGEngine - skybox + environment mapping");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Cubemap sky = load_sky();

    Light light = Mge_MakeDirectionalLight((Vector3){ -0.5f, -1.0f, -0.4f }, (Vector3){ 1.0f, 1.0f, 1.0f });
    light.ambient = 0.3f;

    Object pillars[2] = {
        Mge_MakeObject3D((Vector3){ 0.0f, -1.3f, 0.0f }, (Vector3){ 10.0f, 0.3f, 6.0f }, (Color){ 110, 110, 120, 255 }),
        Mge_MakeObject3D((Vector3){ 0.0f, 0.0f, -3.5f }, (Vector3){ 1.0f, 2.0f, 1.0f }, (Color){ 200, 150, 90, 255 }),
    };

    Camera3D camera = {
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    while (!Mge_WindowShouldClose()) {
        float t = (float)Mge_GetTime() * 0.4f;
        camera.position = (Vector3){ sinf(t) * 8.0f, 2.5f, cosf(t) * 8.0f };
        camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, camera.position));

        Mge_BeginDrawing();
        Mge_ClearBackground(BLACK);

        Mge_BeginMode3D(camera);

        Mge_BeginLighting3DEx(&light, 1, camera);
        for (int i = 0; i < 2; i++)
            Mge_DrawObject(pillars[i]);
        Mge_EndLighting3D();

        Mge_BeginEnvironmentMap(sky, camera, ENVMAP_REFLECT, 0.0f);
        Draw_Cube((Vector3){ -2.0f, 0.2f, 0.0f }, (Vector3){ 1.8f, 1.8f, 1.8f }, WHITE);
        Mge_EndEnvironmentMap();

        Mge_BeginEnvironmentMap(sky, camera, ENVMAP_REFRACT, 1.0f / 1.52f);
        Draw_Cube((Vector3){ 2.0f, 0.2f, 0.0f }, (Vector3){ 1.8f, 1.8f, 1.8f }, WHITE);
        Mge_EndEnvironmentMap();

        Mge_DrawSkybox(sky, camera); // draw last

        Mge_EndMode3D();
        Mge_EndDrawing();
    }

    Mge_UnloadCubemap(sky);
    Mge_CloseWindow();
    return 0;
}
