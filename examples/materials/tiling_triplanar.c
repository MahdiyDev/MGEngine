// Material tiling / offset and triplanar projection.
//
// Repeating a texture without stretching it is a per-material UV transform:
//   uv' = uv * material.tiling + material.offset
// A tiling of {4,4} draws the texture 16 times across the same surface (needs a
// REPEAT / MIRROR wrap mode -- the default is REPEAT).
//
// Triplanar projection samples the maps from world-space XYZ, blended by the
// surface normal, instead of the mesh UVs -- so a non-uniformly scaled object
// tiles the texture instead of stretching it. The normal map (whiteout blend)
// and height map (per-plane parallax-occlusion march) follow; `tiling` does not
// (use `triplanarScale`).
//
//   LEFT   plane, tiling {1,1}  -- one stretched copy
//   MIDDLE plane, tiling {5,5}  -- repeated, no stretch
//   RIGHT  a tall box (1 x 3 x 1) with triplanar on -- square texels on every face
//
//   T   toggle triplanar on the box     UP/DOWN  tiling on the middle plane
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

static Texture2D load_bricks(const char* file)
{
    char a[128], b[160];
    snprintf(a, sizeof(a), "assets/bricks/%s", file);
    snprintf(b, sizeof(b), "../../assets/bricks/%s", file);
    Texture2D t = Mge_LoadTexture(a);
    if (t.id == 0)
        t = Mge_LoadTexture(b);
    return t;
}

int main(void)
{
    Mge_InitWindow(1100, 720, "MGEngine - tiling & triplanar");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Texture2D bricks = load_bricks("bricks2.jpg");
    Texture2D bricksN = load_bricks("bricks2_normal.jpg");
    Mge_SetTextureWrap(bricks, TEXTURE_WRAP_REPEAT);

    Material stretched = Mge_DefaultMaterial();
    Mge_SetMaterialTexture(&stretched, MATERIAL_MAP_DIFFUSE, bricks);

    Material tiled = Mge_DefaultMaterial();
    Mge_SetMaterialTexture(&tiled, MATERIAL_MAP_DIFFUSE, bricks);
    tiled.tiling = (Vector2){ 5.0f, 5.0f };

    Material projected = Mge_DefaultMaterial();
    Mge_SetMaterialTexture(&projected, MATERIAL_MAP_DIFFUSE, bricks);
    Mge_SetMaterialTexture(&projected, MATERIAL_MAP_NORMAL, bricksN); // follows the projection
    projected.triplanar = true;
    projected.triplanarScale = 1.0f;

    Object planeA = Mge_MakeShape3D(PRIM_PLANE, (Vector3){ -3.5f, -1.0f, 0 }, (Vector3){ 3, 0.2f, 3 }, WHITE);
    Object planeB = Mge_MakeShape3D(PRIM_PLANE, (Vector3){ 0.0f, -1.0f, 0 }, (Vector3){ 3, 0.2f, 3 }, WHITE);
    Object box = Mge_MakeObject3D((Vector3){ 3.5f, 0.0f, 0 }, (Vector3){ 1, 3, 1 }, WHITE);

    Light sun = Mge_MakeDirectionalLight((Vector3){ -0.4f, -1.0f, -0.5f }, (Vector3){ 1, 1, 1 });
    sun.ambient = 0.4f;

    Camera3D camera = {
        .position = { 0.0f, 3.0f, 9.0f },
        .target = { 0.0f, -0.2f, -1.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    while (!Mge_WindowShouldClose()) {
        if (IsKeyPressed(KEY_T)) {
            projected.triplanar = !projected.triplanar;
            printf("triplanar: %s\n", projected.triplanar ? "ON" : "OFF");
        }
        if (IsKeyDown(KEY_UP))
            tiled.tiling.x = tiled.tiling.y += 2.0f * (float)Mge_GetDeltaTime();
        if (IsKeyDown(KEY_DOWN))
            tiled.tiling.x = tiled.tiling.y = fmaxf(1.0f, tiled.tiling.y - 2.0f * (float)Mge_GetDeltaTime());

        float t = (float)Mge_GetTime();
        camera.position = (Vector3){ sinf(t * 0.25f) * 2.5f, 3.0f, 9.0f };

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 14, 15, 20, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(sun, camera);
        planeA.material = stretched;
        planeB.material = tiled;
        box.material = projected;
        Mge_DrawObject(planeA);
        Mge_DrawObject(planeB);
        Mge_DrawObject(box);
        Mge_EndLighting3D();
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
