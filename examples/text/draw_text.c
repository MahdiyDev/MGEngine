// Text rendering -- LearnOpenGL In-Practice/Text-Rendering.
//
// The built-in 8x8 font needs no asset; a scalable TrueType font is loaded if
// MGE_FONT points at a .ttf / .otf (else the demo just uses the built-in one).

#include "mge.h"
#include "mge_math.h"
#include "mge_utils.h"

#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

static void signal_handler(int sig)
{
    Mge_CloseWindow();
    exit(sig);
}

int main(void)
{
    Mge_InitWindow(900, 560, "MGEngine - text");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Font builtin = Mge_GetDefaultFont();

    const char* ttfPath = getenv("MGE_FONT");
    Font ttf = ttfPath ? Mge_LoadFont(ttfPath, 48) : (Font){ 0 };
    bool haveTTF = Mge_IsFontValid(ttf);

    while (!Mge_WindowShouldClose()) {
        double t = Mge_GetTime();

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 18, 18, 24, 255 });

        // built-in bitmap font at a few integer scales
        Draw_Text(builtin, "built-in 8x8 font", (Vector2){ 24, 24 }, 8, (Color){ 150, 150, 160, 255 });
        Draw_Text(builtin, "The quick brown fox 0123456789", (Vector2){ 24, 44 }, 16, WHITE);
        Draw_Text(builtin, "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", (Vector2){ 24, 72 }, 16, (Color){ 120, 200, 255, 255 });
        Draw_Text(builtin, "line one\nline two\nline three", (Vector2){ 24, 108 }, 20, (Color){ 200, 255, 200, 255 });

        // a label sized to its text with Mge_MeasureText
        const char* label = "measured box";
        Vector2 lp = { 24, 200 };
        Vector2 sz = Mge_MeasureText(builtin, label, 24);
        Draw_Rectangle((int)lp.x - 6, (int)lp.y - 4, (int)sz.x + 12, (int)sz.y + 8, (Color){ 40, 40, 60, 255 });
        Draw_Text(builtin, label, lp, 24, (Color){ 255, 220, 120, 255 });

        // world-space billboard text: Draw_Text3D inside Mge_BeginMode3D, always
        // facing the orbiting camera
        Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 45.0f, .projection = CAMERA_PERSPECTIVE };
        float a = (float)t * 0.7f;
        cam.position = (Vector3){ 3.5f * sinf(a), 1.2f, 3.5f * cosf(a) };
        cam.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, cam.position));
        Mge_BeginMode3D(cam);
        Draw_CubeWires((Vector3){ 0, 0, 0 }, (Vector3){ 1.4f, 1.4f, 1.4f }, (Color){ 70, 90, 130, 255 });
        Draw_Text3D(builtin, "Draw_Text3D", (Vector3){ 0.0f, 0.0f, 0.0f }, 0.4f,
            (Color){ 255, 240, 180, 255 });
        Mge_EndMode3D();

        // scalable TrueType, if provided
        if (haveTTF) {
            float pulse = 32.0f + 24.0f * (0.5f + 0.5f * sinf((float)t * 2.0f));
            Draw_Text(ttf, "TrueType via stb_truetype", (Vector2){ 24, 270 }, 40, WHITE);
            Draw_Text(ttf, "AVAWATa. kerning & scale", (Vector2){ 24, 330 }, pulse,
                (Color){ 255, 140, 160, 255 });
        } else {
            Draw_Text(builtin, "set MGE_FONT=path/to/font.ttf for a scalable font",
                (Vector2){ 24, 300 }, 16, (Color){ 130, 130, 140, 255 });
        }

        Mge_EndDrawing();
    }

    if (haveTTF)
        Mge_UnloadFont(ttf);
    Mge_CloseWindow();
    return 0;
}
