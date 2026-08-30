// Instancing: one draw call per mesh paints a whole field of melons.
//
//   Mge_LoadModelBatch(model, transforms, count)  uploads the per-instance
//   model matrices once; Mge_DrawModelBatch then renders every copy with a
//   single glDrawElementsInstanced per mesh -- the CPU submits nothing per
//   melon. Here 64 melons sit on a jittered grid, each at its own scale and
//   spin, while the camera orbits.
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

#define GRID  8
#define COUNT (GRID * GRID)

static float frand(void) { return (float)rand() / (float)RAND_MAX; }

int main(void)
{
    Mge_InitWindow(1100, 760, "MGEngine - instancing");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Model melon = load_melon();

    // per-instance placement: a grid on the ground, jittered, random tilt + scale
    const float spacing = 6.0f;
    const float origin = -0.5f * (GRID - 1) * spacing;
    Matrix xf[COUNT];
    float spin[COUNT], spinRate[COUNT];
    Vector3 pos[COUNT];
    float scl[COUNT];

    srand(1);
    for (int i = 0; i < COUNT; i++) {
        int gx = i % GRID, gz = i / GRID;
        pos[i] = (Vector3){
            origin + gx * spacing + (frand() - 0.5f) * 2.5f,
            0.0f,
            origin + gz * spacing + (frand() - 0.5f) * 2.5f,
        };
        scl[i] = 0.09f + frand() * 0.06f;
        spin[i] = frand() * 6.2831853f;
        spinRate[i] = (frand() - 0.5f) * 0.8f;
        xf[i] = Matrix_Multiply(
            Matrix_Multiply(Matrix_Scale(scl[i], scl[i], scl[i]),
                            Matrix_Rotate((Vector3){ 0.0f, 1.0f, 0.0f }, spin[i])),
            Matrix_Translate(pos[i].x, pos[i].y, pos[i].z));
    }

    ModelBatch field = Mge_LoadModelBatch(melon, xf, COUNT);

    Camera3D camera = {
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    Light sun = Mge_MakeDirectionalLight((Vector3){ -0.4f, -1.0f, -0.5f }, (Vector3){ 1.0f, 0.97f, 0.9f });
    sun.ambient = 0.35f;
    sun.diffuse = 0.95f;
    sun.specular = 0.3f;

    const float fieldRadius = 0.5f * GRID * spacing;

    while (!Mge_WindowShouldClose()) {
        float t = (float)Mge_GetTime();

        // keep the melons turning -- shows Mge_UpdateModelBatch re-uploading
        for (int i = 0; i < COUNT; i++) {
            float a = spin[i] + spinRate[i] * t;
            xf[i] = Matrix_Multiply(
                Matrix_Multiply(Matrix_Scale(scl[i], scl[i], scl[i]),
                                Matrix_Rotate((Vector3){ 0.0f, 1.0f, 0.0f }, a)),
                Matrix_Translate(pos[i].x, pos[i].y, pos[i].z));
        }
        Mge_UpdateModelBatch(&field, xf, COUNT);

        float orbit = t * 0.25f;
        camera.position = (Vector3){ sinf(orbit) * fieldRadius * 1.6f, fieldRadius * 0.8f, cosf(orbit) * fieldRadius * 1.6f };
        camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0.0f, 2.0f, 0.0f }, camera.position));

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 28, 30, 38, 255 });

        Mge_BeginMode3D(camera);
        Mge_DrawModelBatch(field, sun, camera);
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_UnloadModelBatch(&field);
    Mge_UnloadModel(&melon);
    Mge_CloseWindow();
    return 0;
}
