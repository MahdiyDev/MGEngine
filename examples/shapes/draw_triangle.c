#include "mge.h"
#include "mge_math.h"
#include "mge_utils.h"

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
    Mge_InitWindow(800, 600, "MGEngine - draw_triangle");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    // a fan of points around a centre -> Draw_TriangleFan
    Vector2 center = { 600, 300 };
    float radius = 90.0f;
    Vector2 fan[13];
    fan[0] = center;
    for (int i = 0; i < 12; i++) {
        float a = (float)i / 11.0f * PI; // half turn
        fan[i + 1] = (Vector2){ center.x + radius * cosf(a), center.y - radius * sinf(a) };
    }

    while (!Mge_WindowShouldClose()) {
        Mge_BeginDrawing();
        Mge_ClearBackground(GRAY);

        Draw_Triangle((Vector2){ 150, 120 }, (Vector2){ 60, 300 }, (Vector2){ 240, 300 }, RED);
        Draw_TriangleLines((Vector2){ 150, 340 }, (Vector2){ 60, 520 }, (Vector2){ 240, 520 }, GREEN);

        Draw_TriangleFan(fan, 13, BLUE);

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
