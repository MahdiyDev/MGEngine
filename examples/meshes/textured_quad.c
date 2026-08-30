// Mesh: hand-built vertex + index + texture data, uploaded once and drawn each
// frame under a moving point light.
//
//   Mesh m = Mge_MakeMesh(verts, nverts, indices, nindices, textures, ntex);
//   Mge_UploadMesh(&m);            // once
//   ... Mge_DrawMesh(m); ...       // per frame, inside Mge_BeginLighting3D
//   Mge_UnloadMesh(&m);            // at shutdown
//
// Positions are world-space -- there is no per-mesh transform, so the wall and
// the floor are just built where they sit.
#include "mge.h"
#include "mge_math.h"

#include <math.h>
#include <signal.h>
#include <stdlib.h>

static void signal_handler(int sig)
{
    Mge_CloseWindow();
    exit(sig);
}

// a flat quad (two triangles) at `center`, facing `normal`, `size` across,
// with the texture mapped once corner-to-corner
static Mesh make_quad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector3 n, Texture2D tex)
{
    Vertex v[4] = {
        { a, n, { 0.0f, 0.0f } },
        { b, n, { 1.0f, 0.0f } },
        { c, n, { 1.0f, 1.0f } },
        { d, n, { 0.0f, 1.0f } },
    };
    unsigned int idx[6] = { 0, 1, 2, 0, 2, 3 };
    MeshTexture mt[1] = { { .texture = tex, .type = MESH_TEXTURE_DIFFUSE } };
    int ntex = (tex.id != 0) ? 1 : 0;
    return Mge_MakeMesh(v, 4, idx, 6, mt, ntex);
}

int main(void)
{
    Mge_InitWindow(1000, 700, "MGEngine - mesh");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Texture2D wall = Mge_LoadTexture("assets/wall.jpg");
    if (wall.id == 0)
        wall = Mge_LoadTexture("../../assets/wall.jpg");

    // upright wall, 4x3, centred at the origin
    Mesh wallMesh = make_quad(
        (Vector3){ -2.0f, 0.0f, 0.0f }, (Vector3){ 2.0f, 0.0f, 0.0f },
        (Vector3){ 2.0f, 3.0f, 0.0f }, (Vector3){ -2.0f, 3.0f, 0.0f },
        (Vector3){ 0.0f, 0.0f, 1.0f }, wall);

    // floor quad, untextured
    Texture2D none = { 0 };
    Mesh floorMesh = make_quad(
        (Vector3){ -6.0f, 0.0f, 6.0f }, (Vector3){ 6.0f, 0.0f, 6.0f },
        (Vector3){ 6.0f, 0.0f, -6.0f }, (Vector3){ -6.0f, 0.0f, -6.0f },
        (Vector3){ 0.0f, 1.0f, 0.0f }, none);

    Mge_UploadMesh(&wallMesh);
    Mge_UploadMesh(&floorMesh);

    Light light = Mge_MakePointLight((Vector3){ 0, 3, 4 }, (Vector3){ 1.0f, 0.95f, 0.85f });
    light.ambient = 0.12f;

    Camera3D camera = {
        .position = { 0.0f, 3.0f, 8.5f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 1.5f, 0 }, camera.position));

    while (!Mge_WindowShouldClose()) {
        double t = Mge_GetTime();
        light.position = (Vector3){ (float)sin(t) * 4.0f, 3.0f, 3.0f };

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 14, 14, 18, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(light, camera);
        Mge_DrawMesh(floorMesh);
        Mge_DrawMesh(wallMesh);
        Mge_EndLighting3D();
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_UnloadMesh(&wallMesh);
    Mge_UnloadMesh(&floorMesh);
    Mge_CloseWindow();
    return 0;
}
