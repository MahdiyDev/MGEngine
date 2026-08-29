// Mixed 2D shapes (was test.cpp).
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
    Mge_InitWindow(800, 600, "MGEngine - mixed shapes");
    signal(SIGINT, signal_handler);
    Mge_SetTargetFPS(60);

    while (!Mge_WindowShouldClose()) {
        Mge_BeginDrawing();
        Mge_ClearBackground(GRAY);

        Draw_RectangleRec((Rectangle){ 150, 150, 100, 100 }, RED);
        Draw_RectangleRec((Rectangle){ 200, 200, 100, 100 }, GREEN);
        Draw_TriangleLines((Vector2){ 100, 100 }, (Vector2){ 150, 200 }, (Vector2){ 50, 200 }, GREEN);

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
