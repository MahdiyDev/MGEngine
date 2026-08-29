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
    Mge_InitWindow(800, 600, "MGEngine - draw_rectangle");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    float rotation = 0.0f;

    while (!Mge_WindowShouldClose()) {
        rotation += 40.0f * (float)Mge_GetDeltaTime();

        Mge_BeginDrawing();
        Mge_ClearBackground(GRAY);

        Draw_Rectangle(80, 80, 120, 90, RED);
        Draw_RectangleRec((Rectangle){ 240, 120, 160, 100 }, GREEN);
        Draw_RectangleLines(460, 100, 140, 140, BLUE);

        // a spinning rectangle around its own centre
        Draw_RectanglePro((Rectangle){ 600, 400, 120, 80 },
            (Vector2){ 60, 40 }, rotation, YELLOW);

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
