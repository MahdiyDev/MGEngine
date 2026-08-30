// 3D manipulation gizmo.
//   left-click an object   -> select it
//   1 / 2 / 3              -> translate / rotate / scale mode
//   drag a handle          -> move / rotate / scale the selected object
//   right-click            -> deselect
#include "mge.h"
#include "mge_math.h"
#include "mge_utils.h"

#include <signal.h>
#include <stdlib.h>

static void signal_handler(int sig)
{
    Mge_CloseWindow();
    exit(sig);
}

int main(void)
{
    Mge_InitWindow(1000, 700, "MGEngine - 3D gizmo");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    const int N = 3;
    Object objects[3] = {
        Mge_MakeObject3D((Vector3){ -2.0f, 0.0f, 0.0f }, (Vector3){ 1.0f, 1.0f, 1.0f }, RED),
        Mge_MakeObject3D((Vector3){ 1.5f, 0.0f, -1.0f }, (Vector3){ 1.4f, 0.8f, 1.0f }, GREEN),
        Mge_MakeObject3D((Vector3){ 0.0f, 1.6f, 2.0f }, (Vector3){ 0.9f, 0.9f, 0.9f }, BLUE),
    };

    Camera3D camera = {
        .position = { 5.0f, 5.0f, 9.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0.0f, 0.0f, 0.0f }, camera.position));

    Light sun = Mge_MakeDirectionalLight((Vector3){ -0.5f, -1.0f, -0.4f }, (Vector3){ 1.0f, 1.0f, 1.0f });
    sun.ambient = 0.35f;

    while (!Mge_WindowShouldClose()) {
        if (IsKeyPressed(KEY_ONE))
            Mge_SetGizmoMode(GIZMO_TRANSLATE);
        if (IsKeyPressed(KEY_TWO))
            Mge_SetGizmoMode(GIZMO_ROTATE);
        if (IsKeyPressed(KEY_THREE))
            Mge_SetGizmoMode(GIZMO_SCALE);
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            Mge_ClearSelection(objects, N);

        int sel = Mge_GetSelectedObject();

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 26, 30, 34, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(sun, camera);
        for (int i = 0; i < N; i++)
            Mge_DrawObject(objects[i]);
        Mge_EndLighting3D();

        bool busy = false;
        if (sel >= 0)
            busy = Mge_Gizmo3D(&objects[sel].position, &objects[sel].rotation, &objects[sel].size, camera, 2.0f);
        Mge_EndMode3D();

        if (!busy)
            Mge_PickObject3D(objects, N, camera);

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
