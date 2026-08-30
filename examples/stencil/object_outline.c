// Stencil testing: outlining selected objects.
//
// Mge_DrawObject() draws a stencil outline (not a wireframe) around any Object
// whose `.selected` flag is set -- here the selection walks along the row. The
// far-left cube is outlined by hand with Mge_DrawObjectOutline() to show a
// custom colour / thickness.
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
    Mge_InitWindow(1000, 700, "MGEngine - object outlining");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    const int N = 4;
    Object cubes[4];
    for (int i = 0; i < N; i++)
        cubes[i] = Mge_MakeObject3D((Vector3){ (float)(i - 1) * 2.4f, 0.0f, 0.0f },
            (Vector3){ 1.4f, 1.4f, 1.4f }, (Color){ 200, 120, 90, 255 });

    Object pillar = Mge_MakeObject3D((Vector3){ -5.5f, 0.4f, 0.0f }, (Vector3){ 1.2f, 2.4f, 1.2f },
        (Color){ 110, 150, 200, 255 });

    Light light = Mge_MakeDirectionalLight((Vector3){ -0.4f, -1.0f, -0.5f }, (Vector3){ 1.0f, 1.0f, 1.0f });
    light.ambient = 0.3f;

    Camera3D camera = {
        .position = { 0.0f, 3.5f, 10.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, camera.position));

    while (!Mge_WindowShouldClose()) {
        int sel = (int)(Mge_GetTime() / 1.2) % N;
        for (int i = 0; i < N; i++)
            cubes[i].selected = (i == sel);

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 18, 19, 24, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(light, camera);
        Draw_Cube((Vector3){ -1.0f, -1.4f, 0.0f }, (Vector3){ 22.0f, 0.2f, 10.0f }, (Color){ 80, 84, 92, 255 });
        for (int i = 0; i < N; i++)
            Mge_DrawObject(cubes[i]);        // selected one gets a white outline
        Mge_DrawObject(pillar);
        Mge_DrawObjectOutline(pillar, 0.12f, (Color){ 120, 230, 255, 255 }); // manual, custom colour
        Mge_EndLighting3D();
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
