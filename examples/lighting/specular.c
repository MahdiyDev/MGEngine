// Specular (Phong) highlight.
//
// The specular term is a bright spot that appears where the surface reflects the
// light straight into the camera: `pow(max(dot(V, R), 0), shininess)`. It moves
// as the light or the viewer moves, and `material.shininess` controls its size
// -- a high exponent gives a small, sharp highlight (polished), a low exponent a
// broad, soft one (satin). Ambient + diffuse are also on so the shape reads.
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
    Mge_InitWindow(1000, 700, "MGEngine - specular highlight");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Object cubes[3] = {
        Mge_MakeObject3D((Vector3){ -2.5f, 0.0f, 0.0f }, (Vector3){ 1.6f, 1.6f, 1.6f }, LIGHTGRAY),
        Mge_MakeObject3D((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 1.6f, 1.6f, 1.6f }, LIGHTGRAY),
        Mge_MakeObject3D((Vector3){ 2.5f, 0.0f, 0.0f }, (Vector3){ 1.6f, 1.6f, 1.6f }, LIGHTGRAY),
    };
    cubes[0].material.shininess = 4.0f;   // broad, soft highlight
    cubes[1].material.shininess = 32.0f;  // medium
    cubes[2].material.shininess = 128.0f; // tight, sharp highlight

    Light light = Mge_MakeLight((Vector3){ 4.0f, 5.0f, 4.0f }, (Vector3){ 1.0f, 1.0f, 1.0f });
    light.ambient = 0.1f;
    light.diffuse = 0.6f;
    light.specular = 0.9f;

    Camera3D camera = {
        .position = { 0.0f, 2.0f, 8.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, camera.position));

    while (!Mge_WindowShouldClose()) {
        // orbit the light so the highlight slides across each cube
        double t = Mge_GetTime();
        light.position = (Vector3){ (float)cos(t) * 5.0f, 4.0f, (float)sin(t) * 5.0f + 3.0f };

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 15, 15, 18, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(light, camera);
        for (int i = 0; i < 3; i++)
            Mge_DrawObject(cubes[i]);
        Mge_EndLighting3D();
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
