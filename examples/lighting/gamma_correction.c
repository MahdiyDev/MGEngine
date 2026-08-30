// Advanced lighting: gamma correction.
//
// Mge_SetGammaCorrection(true) enables GL_FRAMEBUFFER_SRGB, so the GPU encodes
// the final image linear -> sRGB. Toggles every 3 s (or press SPACE):
//   OFF -- lighting output goes straight to the display, which then darkens it
//          with its ~2.2 gamma: shading looks muddy, the ramp below is too bright
//   ON  -- the output is sRGB-encoded first, so it survives the display: the lit
//          floor reads correctly and the ramp is perceptually even
//
// Off is the engine default (shape/light colours here are not authored linear).
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
    Mge_InitWindow(1000, 680, "MGEngine - gamma correction");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Light lamp = Mge_MakePointLight((Vector3){ 0.0f, 3.0f, 0.0f }, (Vector3){ 1.0f, 1.0f, 1.0f });
    lamp.ambient = 0.03f;
    lamp.diffuse = 0.9f;
    lamp.specular = 0.4f;

    Material floor = Mge_DefaultMaterial();
    floor.shininess = 16.0f;

    Camera3D camera = {
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    bool gamma = false;
    double toggledAt = 0.0;
    printf("gamma correction: OFF\n");

    while (!Mge_WindowShouldClose()) {
        double t = Mge_GetTime();

        if (IsKeyPressed(KEY_SPACE) || (t - toggledAt) >= 3.0) {
            gamma = !gamma;
            Mge_SetGammaCorrection(gamma);
            toggledAt = t;
            printf("gamma correction: %s\n", gamma ? "ON" : "OFF");
        }

        float a = (float)t * 0.3f;
        camera.position = (Vector3){ sinf(a) * 8.0f, 4.0f, cosf(a) * 8.0f };
        camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0.0f, 0.5f, 0.0f }, camera.position));

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 12, 12, 16, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(lamp, camera);
        Mge_SetMaterial(floor);
        Draw_Cube((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 14.0f, 0.1f, 14.0f }, (Color){ 170, 170, 175, 255 });
        Draw_Cube((Vector3){ -2.5f, 0.9f, 0.0f }, (Vector3){ 1.6f, 1.6f, 1.6f }, (Color){ 200, 120, 90, 255 });
        Draw_Cube((Vector3){ 2.5f, 0.9f, 1.0f }, (Vector3){ 1.6f, 1.6f, 1.6f }, (Color){ 90, 150, 200, 255 });
        Mge_EndLighting3D();
        Draw_Cube((Vector3){ 0.0f, 3.0f, 0.0f }, (Vector3){ 0.2f, 0.2f, 0.2f }, (Color){ 255, 250, 220, 255 });
        Mge_EndMode3D();

        // 24-step black->white ramp: linear steps look perceptually even only
        // once the output is gamma-encoded
        const int steps = 24;
        for (int i = 0; i < steps; i++) {
            unsigned char v = (unsigned char)(i * 255 / (steps - 1));
            Draw_RectangleRec((Rectangle){ 40.0f + i * 38.0f, 610.0f, 38.0f, 40.0f }, (Color){ v, v, v, 255 });
        }

        Mge_EndDrawing();
    }

    Mge_SetGammaCorrection(false);
    Mge_CloseWindow();
    return 0;
}
