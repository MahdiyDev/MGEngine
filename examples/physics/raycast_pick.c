// Raycasting: pick objects with the mouse and draw the picking ray.
//   left-click            cast a ray through the cursor; nearest hit is selected
//   right-click           clear the selection
//   D                     toggle the debug ray / hit-point overlay
#include "mge.h"
#include "mge_math.h"

#include <signal.h>
#include <stdlib.h>

static void signal_handler(int sig)
{
    Mge_CloseWindow();
    exit(sig);
}

int main(void)
{
    Mge_InitWindow(1000, 700, "MGEngine - raycast pick");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    const int N = 4;
    Object objects[4] = {
        Mge_MakeShape3D(PRIM_CUBE, (Vector3){ -2.4f, 0.0f, 0.0f }, (Vector3){ 1.4f, 1.4f, 1.4f }, RED),
        Mge_MakeShape3D(PRIM_SPHERE, (Vector3){ 0.6f, 0.0f, -0.5f }, (Vector3){ 1.6f, 1.6f, 1.6f }, GREEN),
        Mge_MakeShape3D(PRIM_CUBE, (Vector3){ 2.6f, 0.4f, 1.2f }, (Vector3){ 1.0f, 1.8f, 1.0f }, BLUE),
        Mge_MakeShape3D(PRIM_PLANE, (Vector3){ 0.0f, -1.4f, 0.0f }, (Vector3){ 12.0f, 1.0f, 12.0f }, DARKGRAY),
    };
    objects[2].transform.rotation = Quaternion_FromAxisAngle((Vector3){ 0, 1, 0 }, 35.0f * DEG2RAD);

    Camera3D camera = {
        .position = { 6.0f, 5.0f, 9.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, camera.position));

    Light sun = Mge_MakeDirectionalLight((Vector3){ -0.5f, -1.0f, -0.4f }, (Vector3){ 1, 1, 1 });
    sun.ambient = 0.35f;

    Ray ray = { camera.position, camera.target };
    RayHit hit = { 0 };
    hit.index = -1;
    bool showDebug = true;

    while (!Mge_WindowShouldClose()) {
        if (IsKeyPressed(KEY_D))
            showDebug = !showDebug;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ray = Mge_GetMouseRay(camera);
            hit = Mge_RaycastObjects(ray, objects, N);
            for (int i = 0; i < N; i++)
                objects[i].selected = (i == hit.index);
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            hit = (RayHit){ 0 };
            hit.index = -1;
            for (int i = 0; i < N; i++)
                objects[i].selected = false;
        }

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 26, 30, 34, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(sun, camera);
        for (int i = 0; i < N; i++)
            Mge_DrawObject(objects[i]);
        Mge_EndLighting3D();

        if (showDebug)
            Mge_DrawRayHit(ray, hit, YELLOW, (Color){ 255, 120, 40, 255 });

        Mge_EndMode3D();

        // a small status swatch: green while something is picked
        Draw_RectangleRec((Rectangle){ 16, 16, 24, 24 }, hit.hit ? GREEN : GRAY);

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
