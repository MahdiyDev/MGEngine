// Model loading: Mge_LoadModel runs the file through Assimp and hands back a
// list of GPU-ready meshes (with textures resolved next to the model file).
//
//   Model m = Mge_LoadModel("assets/sliced_musk_melon/scene.gltf");
//   ... Mge_DrawModel(m); ...     // inside Mge_BeginMode3D / Mge_BeginLighting3D
//   Mge_UnloadModel(&m);
//
// The camera here is framed from the model's reported bounding box, and a point
// light orbits it.
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

static Model load_melon(void)
{
    Model m = Mge_LoadModel("assets/sliced_musk_melon/scene.gltf");
    if (m.meshCount == 0)
        m = Mge_LoadModel("../../assets/sliced_musk_melon/scene.gltf");
    return m;
}

int main(void)
{
    Mge_InitWindow(1000, 720, "MGEngine - model loading");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Model melon = load_melon();

    Vector3 center = Vector3_Scale(Vector3_Add(melon.bboxMin, melon.bboxMax), 0.5f);
    Vector3 extent = Vector3_Subtract(melon.bboxMax, melon.bboxMin);
    float radius = Vector3_Length(extent) * 0.5f;
    if (radius < 0.001f)
        radius = 2.0f; // model failed to load -- still show a lit empty scene

    const float camDist = radius * 2.2f;
    const float camHeight = center.y + radius * 0.6f;

    Camera3D camera = {
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 50.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    // fixed directional light (no distance falloff) -- as the view orbits, the
    // melon sweeps through its lit and shaded sides
    Light light = Mge_MakeDirectionalLight((Vector3){ -0.4f, -1.0f, -0.5f }, (Vector3){ 1.0f, 0.98f, 0.94f });
    light.ambient = 0.45f;
    light.diffuse = 0.9f;
    light.specular = 0.25f;

    while (!Mge_WindowShouldClose()) {
        // orbit the camera around the melon -> it appears to spin left-to-right
        // (there is no per-model matrix; flip the sign to reverse the spin)
        float angle = (float)Mge_GetTime() * 0.6f;
        camera.position = (Vector3){
            center.x + sinf(angle) * camDist,
            camHeight,
            center.z + cosf(angle) * camDist,
        };
        camera.target = Vector3Normalize(Vector3_Subtract(center, camera.position));

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 32, 34, 40, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(light, camera);
        Mge_DrawModel(melon);
        Mge_EndLighting3D();
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_UnloadModel(&melon);
    Mge_CloseWindow();
    return 0;
}
