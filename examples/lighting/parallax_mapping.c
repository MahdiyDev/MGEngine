// Advanced lighting: parallax (occlusion) mapping.
//
// A depth map displaces the sampled texture coordinates along the view ray, so
// a flat quad looks genuinely embossed -- mortar grooves hide behind bricks at
// grazing angles, with a stepped silhouette. The engine builds the tangent frame
// in the fragment shader (screen-space derivatives, no tangent attribute),
// marches the depth field in tangent space and interpolates the hit -- exactly
// LearnOpenGL's Parallax Occlusion Mapping. Drop the depth map into
// MATERIAL_MAP_HEIGHT (black = surface, white = deep); .value is the scale.
//
//   SPACE      toggle parallax
//   UP / DOWN  height scale
//   N          toggle the normal map
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

static Texture2D load_tex(const char* file, bool sRGB)
{
    char a[256], b[256];
    snprintf(a, sizeof(a), "assets/bricks/%s", file);
    snprintf(b, sizeof(b), "../../assets/bricks/%s", file);
    Texture2D t = Mge_LoadTextureEx(a, sRGB);
    if (t.id == 0)
        t = Mge_LoadTextureEx(b, sRGB);
    return t;
}

int main(void)
{
    Mge_InitWindow(960, 720, "MGEngine - parallax mapping");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Texture2D albedo = load_tex("bricks2.jpg", false);
    Texture2D normal = load_tex("bricks2_normal.jpg", false); // vector data -> linear
    Texture2D depth = load_tex("bricks2_disp.jpg", false);   // depth map -> linear
    if (albedo.id == 0) {
        printf("assets/bricks not found\n");
        Mge_CloseWindow();
        return 1;
    }

    Material wall = Mge_DefaultMaterial();
    Mge_SetMaterialTexture(&wall, MATERIAL_MAP_DIFFUSE, albedo);
    Mge_SetMaterialTexture(&wall, MATERIAL_MAP_NORMAL, normal);
    Mge_SetMaterialTexture(&wall, MATERIAL_MAP_HEIGHT, depth);
    wall.maps[MATERIAL_MAP_HEIGHT].value = 0.1f; // LearnOpenGL's height_scale
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

    bool parallax = true, useNormal = true;

    while (!Mge_WindowShouldClose()) {
        float t = (float)Mge_GetTime();

        if (IsKeyPressed(KEY_SPACE)) {
            parallax = !parallax;
            Mge_SetMaterialTexture(&wall, MATERIAL_MAP_HEIGHT, parallax ? depth : (Texture2D){ 0 });
            printf("parallax: %s\n", parallax ? "ON" : "OFF");
        }
        if (IsKeyPressed(KEY_N)) {
            useNormal = !useNormal;
            wall.maps[MATERIAL_MAP_NORMAL].value = useNormal ? 1.0f : 0.0f;
        }
        if (IsKeyDown(KEY_UP))
            wall.maps[MATERIAL_MAP_HEIGHT].value += 0.4f * (float)Mge_GetDeltaTime();
        if (IsKeyDown(KEY_DOWN))
            wall.maps[MATERIAL_MAP_HEIGHT].value = fmaxf(0.0f,
                wall.maps[MATERIAL_MAP_HEIGHT].value - 0.4f * (float)Mge_GetDeltaTime());

        // sweep the camera side to side so the displacement is obvious at angles
        camera.position = (Vector3){ sinf(t * 0.4f) * 2.6f, 0.6f, 4.2f };
        camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, camera.position));

        // and orbit the light across the bricks for raking highlights
        lamp.position = (Vector3){ cosf(t * 0.8f) * 1.6f, sinf(t * 0.8f) * 1.2f, 1.6f };

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 10, 10, 13, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(lamp, camera);
        Mge_SetMaterial(wall);
        Draw_Cube((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 4.0f, 4.0f, 0.15f }, WHITE); // the wall
        Mge_EndLighting3D();
        Draw_Cube(lamp.position, (Vector3){ 0.08f, 0.08f, 0.08f }, (Color){ 255, 245, 210, 255 });
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
