// Batch rendering in the backend (mge_gl).
//
// Every Draw_* below funnels through the same immediate-mode batcher: vertices
// pile into one CPU buffer, same-mode primitives merge into one draw-call entry,
// and nothing hits the GPU until Mge_EndDrawing (or a shader / state change).
// So this whole grid -- 400 filled rects + 400 triangle outlines -- costs about
// TWO glDraw* calls per frame, not 800. Mge_GetDrawCalls() reports last frame's
// count; watch it stay flat as GRID grows.
#include "mge.h"
#include "mge_math.h"

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static void signal_handler(int sig)
{
    Mge_CloseWindow();
    exit(sig);
}

#define GRID 20 // GRID*GRID rectangles + as many triangle outlines

int main(void)
{
    Mge_InitWindow(900, 700, "MGEngine - batch rendering");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    double reportAt = 0.0;

    while (!Mge_WindowShouldClose()) {
        float t = (float)Mge_GetTime();

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 24, 26, 32, 255 });

        for (int y = 0; y < GRID; y++) {
            for (int x = 0; x < GRID; x++) {
                float px = 40.0f + x * 40.0f;
                float py = 40.0f + y * 32.0f;
                float wob = 6.0f * sinf(t * 2.0f + x * 0.3f + y * 0.2f);
                Color c = { (unsigned char)(120 + x * 6), (unsigned char)(90 + y * 7), 200, 255 };
                Draw_RectangleRec((Rectangle){ px, py, 26.0f + wob, 20.0f }, c);
                Draw_TriangleLines(
                    (Vector2){ px + 13.0f, py - 10.0f },
                    (Vector2){ px, py },
                    (Vector2){ px + 26.0f, py },
                    (Color){ 240, 220, 120, 255 });
            }
        }

        Mge_EndDrawing();

        if (t - reportAt >= 1.0) {
            reportAt = t;
            printf("shapes this frame: %d   |   GL draw calls: %d\n",
                GRID * GRID * 2, Mge_GetDrawCalls());
        }
    }

    Mge_CloseWindow();
    return 0;
}
