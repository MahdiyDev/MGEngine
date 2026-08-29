#include "mge.h"
#include "mge_math.h"
#include "mge_utils.h"

#include <signal.h>
#include <stdlib.h>

static void signal_handler(int sig)
{
    Mge_CloseWindow();
    if (sig == SIGINT)
        TRACE_LOG(LOG_INFO, "<Ctrl-C> received. exiting...");
    exit(sig);
}

int main(void)
{
    Mge_InitWindow(800, 600, "MGEngine - draw_line");
    Mge_SetTargetFPS(60);

    signal(SIGINT, signal_handler);

    Vector2 hand_start = { 400, 300 };
    Vector2 hand_end = { 400, 120 };

    while (!Mge_WindowShouldClose()) {
        // rotate the "clock hand" a little each frame
        float angle = 0.6f * (float)Mge_GetDeltaTime();
        Vector2 rel = { hand_end.x - hand_start.x, hand_end.y - hand_start.y };
        rel = Vector2_Rotate(rel, angle);
        hand_end = (Vector2){ hand_start.x + rel.x, hand_start.y + rel.y };

        Mge_BeginDrawing();
        Mge_ClearBackground(GRAY);

        Draw_LineV(hand_start, hand_end, RED);

        // a rectangle drawn from four lines
        Draw_Line(100, 100, 200, 100, BLUE);
        Draw_Line(200, 100, 200, 200, BLUE);
        Draw_Line(200, 200, 100, 200, BLUE);
        Draw_Line(100, 200, 100, 100, BLUE);

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
