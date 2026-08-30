// Batching vertex attributes: build a Mesh from separate (non-interleaved)
// position / normal / texcoord arrays. Mge_UploadMesh packs them into ONE VBO
// as contiguous blocks -- all positions, then all normals, then all texcoords
// -- instead of the usual interleaved layout. Drawing is identical.
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

int main(void)
{
    Mge_InitWindow(1000, 700, "MGEngine - batched vertex attributes");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Texture2D wall = Mge_LoadTexture("assets/wall.jpg");
    if (wall.id == 0)
        wall = Mge_LoadTexture("../../assets/wall.jpg");

    // a 6x6 quad in the y=0 plane, described one attribute at a time
    const Vector3 positions[4] = {
        { -3.0f, 0.0f, 3.0f }, { 3.0f, 0.0f, 3.0f }, { 3.0f, 0.0f, -3.0f }, { -3.0f, 0.0f, -3.0f }
    };
    const Vector3 normals[4] = {
        { 0, 1, 0 }, { 0, 1, 0 }, { 0, 1, 0 }, { 0, 1, 0 }
    };
    const Vector2 texcoords[4] = {
        { 0, 0 }, { 2, 0 }, { 2, 2 }, { 0, 2 }
    };
    const unsigned int indices[6] = { 0, 1, 2, 0, 2, 3 };
    MeshTexture tex[1] = { { wall, MESH_TEXTURE_DIFFUSE } };

    Mesh floor = Mge_MakeMeshFromArrays(positions, normals, texcoords, 4,
        indices, 6, tex, (wall.id != 0) ? 1 : 0);
    Mge_UploadMesh(&floor);

    Light light = Mge_MakePointLight((Vector3){ 0, 3, 2 }, (Vector3){ 1.0f, 0.95f, 0.85f });
    light.ambient = 0.2f;

    Camera3D camera = {
        .position = { 0.0f, 4.0f, 7.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, camera.position));

    while (!Mge_WindowShouldClose()) {
        double t = Mge_GetTime();
        light.position = (Vector3){ (float)sin(t) * 3.0f, 3.0f, 2.0f };

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 15, 16, 20, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(light, camera);
        Mge_DrawMesh(floor);
        Mge_EndLighting3D();
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_UnloadMesh(&floor);
    Mge_CloseWindow();
    return 0;
}
