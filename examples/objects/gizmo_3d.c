// 3D object move gizmo.
//   left-click an object -> select it
//   drag a gizmo arrow   -> move along that axis (X red, Y green, Z blue)
//   right-click          -> deselect
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
    const float AXIS = 1.6f;
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
    // point the camera at the scene origin (Mge_BeginMode3D looks at position + target)
    camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0.0f, 0.0f, 0.0f }, camera.position));

    while (!Mge_WindowShouldClose()) {
        int selected = Mge_ManipulateObjects3D(objects, N, camera, AXIS);
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            Mge_ClearSelection(objects, N);

        Mge_BeginDrawing();
        Mge_ClearBackground(DARKGREEN);

        Mge_BeginMode3D(camera);
        for (int i = 0; i < N; i++)
            Mge_DrawObject(objects[i]);
        if (selected >= 0)
            Mge_DrawObjectGizmo(objects[selected], AXIS);
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
