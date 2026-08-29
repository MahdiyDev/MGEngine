// 2D object move gizmo.
//   left-click an object   -> select it
//   drag a gizmo arrow     -> move along that axis (X = red, Y = green)
//   drag the object body   -> move freely
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
    Mge_InitWindow(1000, 700, "MGEngine - 2D gizmo");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    const int N = 3;
    const float AXIS = 70.0f;
    Object objects[3] = {
        Mge_MakeObject2D(260.0f, 260.0f, 90.0f, 60.0f, RED),
        Mge_MakeObject2D(520.0f, 360.0f, 70.0f, 70.0f, GREEN),
        Mge_MakeObject2D(760.0f, 200.0f, 120.0f, 40.0f, BLUE),
    };

    while (!Mge_WindowShouldClose()) {
        int selected = Mge_ManipulateObjects2D(objects, N, AXIS);
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            Mge_ClearSelection(objects, N);

        Mge_BeginDrawing();
        Mge_ClearBackground(DARKGRAY);

        for (int i = 0; i < N; i++)
            Mge_DrawObject(objects[i]);
        if (selected >= 0)
            Mge_DrawObjectGizmo(objects[selected], AXIS);

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
