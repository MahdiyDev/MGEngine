// Face culling: skip triangles that face away from the camera.
//
// The scene cycles every ~2.5 s:
//   OFF        every triangle is drawn
//   CULL_BACK  back faces skipped -- the normal, faster setting; looks identical
//   CULL_FRONT front faces skipped -- the cubes turn "inside out", you see their
//              far interior walls (handy for debugging winding)
//
// `Draw_Cube` and imported meshes wind counter-clockwise (the default front
// face), so CULL_BACK just works. The engine never enables culling itself.
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

int main(void)
{
    Mge_InitWindow(1000, 700, "MGEngine - face culling");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Object cubes[6];
    for (int i = 0; i < 6; i++) {
        float a = (float)i / 6.0f * 2.0f * (float)PI;
        cubes[i] = Mge_MakeObject3D((Vector3){ cosf(a) * 3.2f, 0.0f, sinf(a) * 3.2f },
            (Vector3){ 1.6f, 1.6f, 1.6f }, (Color){ 200, 160, 110, 255 });
    }

    Light light = Mge_MakeDirectionalLight((Vector3){ -0.5f, -1.0f, -0.4f }, (Vector3){ 1.0f, 1.0f, 1.0f });
    light.ambient = 0.25f;

    Camera3D camera = {
        .position = { 0.0f, 4.0f, 9.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, camera.position));

    while (!Mge_WindowShouldClose()) {
        int mode = (int)(Mge_GetTime() / 2.5) % 3;

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 16, 18, 22, 255 });

        Mge_BeginMode3D(camera);

        if (mode == 0) {
            Mge_DisableFaceCulling();
        } else {
            Mge_EnableFaceCulling();
            Mge_SetCullFace((mode == 1) ? CULL_BACK : CULL_FRONT);
        }

        Mge_BeginLighting3DEx(&light, 1, camera);
        for (int i = 0; i < 6; i++)
            Mge_DrawObject(cubes[i]);
        Mge_EndLighting3D();

        Mge_DisableFaceCulling(); // don't leak into anything drawn after
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
