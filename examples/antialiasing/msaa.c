// Anti-aliasing (MSAA).
//
// Mge_SetMSAA(n) before Mge_InitWindow asks for an n-sample default framebuffer;
// after that every edge the renderer draws is smoothed automatically -- there is
// nothing per-shape to do. Mge_GetMSAA() reports what the driver actually gave.
//
// Run with `Mge_SetMSAA(0)` to see the jaggies come back.
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

int main(void)
{
    Mge_SetMSAA(4); // <-- the whole feature; try 0 to compare

    Mge_InitWindow(900, 640, "MGEngine - anti-aliasing");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    printf("MSAA: requested %dx, driver granted %d samples\n",
        Mge_GetRequestedMSAA(), Mge_GetMSAA());

    Light light = Mge_MakeDirectionalLight((Vector3){ -0.5f, -1.0f, -0.4f }, (Vector3){ 1.0f, 1.0f, 1.0f });
    light.ambient = 0.35f;

    Camera3D camera = {
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 50.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    const Vector3 CENTER = { 0.0f, 0.0f, 0.0f };
    const Vector3 SIZE = { 2.0f, 2.0f, 2.0f };

    while (!Mge_WindowShouldClose()) {
        // orbit so the cube's near-vertical edges sweep across the screen --
        // that is where aliasing is most visible
        float a = (float)Mge_GetTime() * 0.5f;
        camera.position = (Vector3){ sinf(a) * 6.0f, 2.2f, cosf(a) * 6.0f };
        camera.target = Vector3Normalize(Vector3_Subtract(CENTER, camera.position));

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 18, 20, 26, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(light, camera);
        Draw_Cube(CENTER, SIZE, (Color){ 120, 170, 220, 255 });
        Mge_EndLighting3D();
        Draw_CubeWires(CENTER, (Vector3){ 2.02f, 2.02f, 2.02f }, (Color){ 245, 245, 245, 255 });
        Mge_EndMode3D();

        // a thin rotating triangle outline -- the classic AA test case
        Vector2 c = { 700.0f, 480.0f };
        float r = 90.0f;
        Vector2 p1 = { c.x + cosf(a) * r, c.y + sinf(a) * r };
        Vector2 p2 = { c.x + cosf(a + 2.094f) * r, c.y + sinf(a + 2.094f) * r };
        Vector2 p3 = { c.x + cosf(a + 4.189f) * r, c.y + sinf(a + 4.189f) * r };
        Draw_TriangleLines(p1, p2, p3, (Color){ 255, 200, 90, 255 });

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
