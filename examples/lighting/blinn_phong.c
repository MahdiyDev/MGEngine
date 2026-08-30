// Advanced lighting: Blinn-Phong vs classic Phong.
//
// A large floor at low shininess, lit by a point light hovering just above it.
// The model auto-switches every 3 s (or press SPACE):
//   PHONG  -- reflect(-L, N) . V : the highlight cuts hard to black once the
//             angle between view and reflection passes 90 deg -> a visible ring
//   BLINN  -- N . halfway(L, V)  : no such cutoff, the falloff stays smooth
// Blinn is the engine default; this example only calls Mge_SetLightingModel to
// show the difference.
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
    Mge_InitWindow(1000, 680, "MGEngine - Blinn-Phong");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    // point light close to the surface -> grazing view angles across the floor
    Light lamp = Mge_MakePointLight((Vector3){ 0.0f, 1.2f, 0.0f }, (Vector3){ 1.0f, 1.0f, 1.0f });
    lamp.ambient = 0.05f;
    lamp.diffuse = 0.8f;
    lamp.specular = 1.0f;
    lamp.linear = 0.09f;
    lamp.quadratic = 0.032f;

    Material floor = Mge_DefaultMaterial();
    floor.shininess = 8.0f; // low -> Phong's cutoff ring is obvious
    floor.maps[MATERIAL_MAP_SPECULAR].value = 1.0f;

    Camera3D camera = {
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    LightingModel model = LIGHTING_BLINN_PHONG;
    double switchedAt = 0.0;
    const char* names[] = { "BLINN-PHONG", "PHONG" };
    printf("lighting model: %s\n", names[model]);

    while (!Mge_WindowShouldClose()) {
        double t = Mge_GetTime();

        if (IsKeyPressed(KEY_SPACE) || (t - switchedAt) >= 3.0) {
            model = (model == LIGHTING_BLINN_PHONG) ? LIGHTING_PHONG : LIGHTING_BLINN_PHONG;
            Mge_SetLightingModel(model);
            switchedAt = t;
            printf("lighting model: %s\n", names[model]);
        }

        // low camera skimming across the floor toward the light
        float a = (float)t * 0.3f;
        camera.position = (Vector3){ sinf(a) * 7.0f, 1.6f, cosf(a) * 7.0f };
        camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0.0f, 0.4f, 0.0f }, camera.position));

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 10, 10, 14, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(lamp, camera);
        Mge_SetMaterial(floor);
        Draw_Cube((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 16.0f, 0.1f, 16.0f }, (Color){ 180, 180, 190, 255 });
        Mge_EndLighting3D();

        // mark the light position
        Draw_Cube((Vector3){ 0.0f, 1.2f, 0.0f }, (Vector3){ 0.15f, 0.15f, 0.15f }, (Color){ 255, 250, 210, 255 });
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
