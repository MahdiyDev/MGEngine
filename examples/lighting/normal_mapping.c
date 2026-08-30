// Advanced lighting: normal mapping.
//
// A normal map stores a per-texel surface normal (in tangent space) so a flat
// quad picks up the lighting response of bumpy geometry. The engine builds the
// TBN frame in the fragment shader from screen-space derivatives, so no tangent
// vertex attribute is needed -- just drop the map into MATERIAL_MAP_NORMAL.
//
// Press SPACE to toggle the normal map; the light orbits so the raking angle
// makes the bricks pop.
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

static Texture2D load_tex(const char* rel)
{
    char a[256], b[256];
    snprintf(a, sizeof(a), "assets/brickwall/%s", rel);
    snprintf(b, sizeof(b), "../../assets/brickwall/%s", rel);
    Texture2D t = Mge_LoadTexture(a);      // normal maps are linear data -> not ...Ex(true)
    if (t.id == 0)
        t = Mge_LoadTexture(b);
    return t;
}

int main(void)
{
    Mge_InitWindow(960, 720, "MGEngine - normal mapping");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Texture2D brick = load_tex("brickwall.jpg");
    Texture2D brickN = load_tex("brickwall_normal.jpg");

    Material wall = Mge_DefaultMaterial();
    Mge_SetMaterialTexture(&wall, MATERIAL_MAP_DIFFUSE, brick);
    Mge_SetMaterialTexture(&wall, MATERIAL_MAP_NORMAL, brickN);
    wall.shininess = 32.0f;

    Light lamp = Mge_MakePointLight((Vector3){ 0.0f, 0.0f, 2.0f }, (Vector3){ 1.0f, 1.0f, 1.0f });
    lamp.ambient = 0.08f;
    lamp.linear = 0.09f;
    lamp.quadratic = 0.0f;

    Camera3D camera = {
        .position = { 0.0f, 0.0f, 4.5f },
        .target = { 0.0f, 0.0f, -1.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 50.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    bool useMap = true;

    while (!Mge_WindowShouldClose()) {
        float t = (float)Mge_GetTime();

        if (IsKeyPressed(KEY_SPACE)) {
            useMap = !useMap;
            Mge_SetMaterialTexture(&wall, MATERIAL_MAP_NORMAL, useMap ? brickN : (Texture2D){ 0 });
            printf("normal map: %s\n", useMap ? "ON" : "OFF");
        }

        // orbit the light in the plane just in front of the wall
        lamp.position = (Vector3){ cosf(t * 0.9f) * 1.8f, sinf(t * 0.9f) * 1.4f, 1.4f };

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 12, 12, 15, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(lamp, camera);
        Mge_SetMaterial(wall);
        Draw_Cube((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 5.0f, 5.0f, 0.2f }, WHITE); // the wall
        Mge_EndLighting3D();
        Draw_Cube(lamp.position, (Vector3){ 0.1f, 0.1f, 0.1f }, (Color){ 255, 245, 210, 255 });
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
