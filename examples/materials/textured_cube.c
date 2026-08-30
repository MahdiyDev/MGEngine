// Material maps: a textured, tinted, lit cube.
//
// A `Material` holds one `MaterialMap` per slot. This example uses two:
//
//   MATERIAL_MAP_DIFFUSE   .texture  an image sampled across each face
//                          .color    a tint multiplied over the texture
//   MATERIAL_MAP_SPECULAR  .value    how strong the highlight is (0 = matte)
//
// Left cube:  texture + white tint, glossy.
// Middle cube: same texture, warm tint, matte (specular value 0).
// Right cube: no texture (id 0 falls back to white), plain colour, glossy.
//
// Run from the repo root or this folder; a missing texture file just falls
// back to the colour tint.
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

static Texture2D load_wall(void)
{
    Texture2D t = Mge_LoadTexture("assets/wall.jpg");
    if (t.id == 0)
        t = Mge_LoadTexture("../../assets/wall.jpg");
    return t;
}

int main(void)
{
    Mge_InitWindow(1000, 700, "MGEngine - material maps");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Texture2D wall = load_wall();

    // start from the default material, then edit individual maps
    Material glossyTextured = Mge_DefaultMaterial();
    Mge_SetMaterialTexture(&glossyTextured, MATERIAL_MAP_DIFFUSE, wall);
    glossyTextured.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    glossyTextured.maps[MATERIAL_MAP_SPECULAR].value = 1.0f;
    glossyTextured.shininess = 64.0f;

    Material matteTinted = Mge_DefaultMaterial();
    Mge_SetMaterialTexture(&matteTinted, MATERIAL_MAP_DIFFUSE, wall);
    matteTinted.maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 255, 170, 120, 255 }; // warm tint
    matteTinted.maps[MATERIAL_MAP_SPECULAR].value = 0.0f;                         // no highlight

    Material plainGlossy = Mge_DefaultMaterial();
    plainGlossy.maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 120, 180, 255, 255 }; // no texture -> flat blue
    plainGlossy.maps[MATERIAL_MAP_SPECULAR].value = 1.0f;
    plainGlossy.shininess = 96.0f;

    const Vector3 pos[3] = {
        { -2.6f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 2.6f, 0.0f, 0.0f }
    };
    const Material* mats[3] = { &glossyTextured, &matteTinted, &plainGlossy };
    const Color tints[3] = {
        WHITE, { 255, 170, 120, 255 }, { 120, 180, 255, 255 }
    };

    Light light = Mge_MakeLight((Vector3){ 0.0f, 5.0f, 4.0f }, (Vector3){ 1.0f, 1.0f, 1.0f });

    Camera3D camera = {
        .position = { 0.0f, 2.5f, 8.5f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, camera.position));

    while (!Mge_WindowShouldClose()) {
        double t = Mge_GetTime();
        light.position = (Vector3){ (float)sin(t) * 5.0f, 4.0f, 3.5f };

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 15, 15, 18, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(light, camera);
        for (int i = 0; i < 3; i++) {
            Mge_SetMaterial(*mats[i]);
            // the diffuse map's colour is carried by the geometry's vertex colour
            Draw_Cube(pos[i], (Vector3){ 1.6f, 1.6f, 1.6f }, tints[i]);
        }
        Mge_EndLighting3D();
        Mge_EndMode3D();

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
