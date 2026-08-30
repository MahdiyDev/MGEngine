// Dynamic environment maps.
//
// Every frame the scene (skybox + the coloured cubes, but NOT the mirror itself)
// is rendered into a cube-map probe from the centre. The middle cube is then
// drawn with that probe as its environment map, so it reflects the moving
// coloured cubes around it in real time.
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

static const Vector3 MIRROR = { 0.0f, 0.5f, 0.0f };

static const Color ORBIT_COLORS[4] = {
    { 220, 70, 70, 255 }, { 70, 200, 100, 255 }, { 80, 130, 220, 255 }, { 220, 200, 90, 255 }
};

static void draw_scene(Camera3D cam, Light light, Vector3 orbiters[4], Cubemap sky)
{
    Mge_BeginLighting3DEx(&light, 1, cam);
    for (int i = 0; i < 4; i++) {
        Mge_SetMaterial((Material){ .maps[MATERIAL_MAP_DIFFUSE].color = ORBIT_COLORS[i],
            .maps[MATERIAL_MAP_SPECULAR].value = 1.0f, .shininess = 24.0f });
        Draw_Cube(orbiters[i], (Vector3){ 1.2f, 1.2f, 1.2f }, ORBIT_COLORS[i]);
    }
    Mge_EndLighting3D();
    Mge_DrawSkybox(sky, cam);
}

int main(void)
{
    Mge_InitWindow(1000, 700, "MGEngine - dynamic environment map");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Cubemap sky = load_sky();
    EnvProbe probe = Mge_LoadEnvProbe(256);

    Light light = Mge_MakeDirectionalLight((Vector3){ -0.4f, -1.0f, -0.5f }, (Vector3){ 1.0f, 1.0f, 1.0f });
    light.ambient = 0.3f;

    Camera3D camera = {
        .position = { 0.0f, 3.0f, 8.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    camera.target = Vector3Normalize(Vector3_Subtract(MIRROR, camera.position));

    while (!Mge_WindowShouldClose()) {
        double t = Mge_GetTime();
        Vector3 orbiters[4];
        for (int i = 0; i < 4; i++) {
            float a = (float)t * 0.6f + (float)i * 1.5708f;
            orbiters[i] = (Vector3){ cosf(a) * 3.4f, 0.4f, sinf(a) * 3.4f };
        }

        Mge_BeginDrawing();

        // 1. render the environment into the probe, from the mirror's position
        for (int f = 0; f < 6; f++) {
            Mge_BeginEnvProbeFace(probe, MIRROR, f);
            Mge_ClearBackground(BLACK);
            draw_scene(Mge_GetEnvProbeCamera(MIRROR, f), light, orbiters, sky);
            Mge_EndEnvProbeFace();
        }

        // 2. draw the real view, with the mirror using the fresh probe
        Mge_ClearBackground(BLACK);
        Mge_BeginMode3D(camera);
        draw_scene(camera, light, orbiters, sky);

        Mge_BeginEnvironmentMap(probe.cubemap, camera, ENVMAP_REFLECT, 0.0f);
        Draw_Cube(MIRROR, (Vector3){ 1.8f, 1.8f, 1.8f }, WHITE);
        Mge_EndEnvironmentMap();

        Mge_EndMode3D();
        Mge_EndDrawing();
    }

    Mge_UnloadEnvProbe(probe);
    Mge_UnloadCubemap(sky);
    Mge_CloseWindow();
    return 0;
}
