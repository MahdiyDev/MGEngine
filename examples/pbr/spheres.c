// Physically-based rendering + image-based lighting.
//
//   PBR/Theory  -- the Cook-Torrance BRDF: microfacet distribution (GGX),
//                  geometry (Smith) and Fresnel (Schlick), metallic/roughness.
//   PBR/Lighting -- direct point/directional lights through that BRDF.
//   PBR/IBL      -- the ambient term from an environment map: a convolved
//                  irradiance cube (diffuse) + a roughness-mip prefilter cube
//                  and a BRDF LUT (specular).
//
// Left block: a 6x6 grid of one material, metallic 0->1 up, roughness 0->1 right.
// Right: three spheres of the downloaded "rusted iron" texture set.
//
//   SPACE   rotate the camera on / off       I   toggle IBL
#include "mge.h"
#include "mge_math.h"

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static const int WIDTH = 1280, HEIGHT = 760;
#define ROWS 6
#define COLS 6

static void signal_handler(int sig)
{
    Mge_CloseWindow();
    exit(sig);
}

static Texture2D pbr_tex(const char* name, bool sRGB)
{
    char a[128], b[160];
    snprintf(a, sizeof(a), "assets/pbr/rusted_iron/%s", name);
    snprintf(b, sizeof(b), "../../assets/pbr/rusted_iron/%s", name);
    Texture2D t = Mge_LoadTextureEx(a, sRGB);
    if (t.id == 0)
        t = Mge_LoadTextureEx(b, sRGB);
    return t;
}

int main(void)
{
    Mge_InitWindow(WIDTH, HEIGHT, "MGEngine - PBR + IBL");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    Environment env = Mge_LoadEnvironment("assets/hdr/newport_loft.hdr");
    if (env.irradiance == 0)
        env = Mge_LoadEnvironment("../../assets/hdr/newport_loft.hdr");

    RenderTexture hdr = Mge_LoadRenderTextureHDR(WIDTH, HEIGHT);

    PBRMaterial iron = Mge_DefaultPBRMaterial();
    iron.albedo = pbr_tex("albedo.png", true);
    iron.normal = pbr_tex("normal.png", false);
    iron.metallic = pbr_tex("metallic.png", false);
    iron.roughness = pbr_tex("roughness.png", false);
    iron.ao = pbr_tex("ao.png", false);

    // four point lights, LearnOpenGL-style
    Light lights[4] = {
        Mge_MakePointLight((Vector3){ -10, 10, 10 }, (Vector3){ 200, 200, 200 }),
        Mge_MakePointLight((Vector3){ 10, 10, 10 }, (Vector3){ 200, 200, 200 }),
        Mge_MakePointLight((Vector3){ -10, -10, 10 }, (Vector3){ 120, 120, 130 }),
        Mge_MakePointLight((Vector3){ 10, -10, 10 }, (Vector3){ 120, 120, 130 }),
    };
    for (int i = 0; i < 4; i++) {
        lights[i].linear = 0.0f;
        lights[i].quadratic = 1.0f; // 1/d^2
    }

    Camera3D camera = { .up = { 0, 1, 0 }, .fovy = 55.0f, .projection = CAMERA_PERSPECTIVE };

    bool spin = true, ibl = true;
    float ang = 0.0f;

    while (!Mge_WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) spin = !spin;
        if (IsKeyPressed(KEY_I)) { ibl = !ibl; printf("IBL: %s\n", ibl ? "ON" : "OFF"); }
        if (spin) ang += 0.2f * (float)Mge_GetDeltaTime();

        camera.position = (Vector3){ sinf(ang) * 22.0f, 2.0f, cosf(ang) * 22.0f };
        camera.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, camera.position));

        Mge_BeginDrawing();
        Mge_BeginTextureMode(hdr);
        Mge_ClearBackground((Color){ 4, 5, 8, 255 });
        Mge_BeginMode3D(camera);

        if (ibl)
            Mge_BeginPBR3DIBL(lights, 4, camera, env);
        else
            Mge_BeginPBR3D(lights, 4, camera);

        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++) {
                PBRMaterial m = Mge_DefaultPBRMaterial();
                m.albedoColor = (Vector3){ 0.55f, 0.55f, 0.6f };
                m.metallicValue = (float)r / (ROWS - 1);
                m.roughnessValue = fmaxf(0.05f, (float)c / (COLS - 1));
                Mge_SetPBRMaterial(m);
                Draw_Sphere((Vector3){ (c - COLS / 2) * 2.5f - 3.0f, (r - ROWS / 2) * 2.5f, 0 }, 1.0f, WHITE);
            }

        Mge_SetPBRMaterial(iron);
        for (int i = 0; i < 3; i++)
            Draw_Sphere((Vector3){ 12.0f, (i - 1) * 3.2f, 0 }, 1.3f, WHITE);

        Mge_EndPBR3D();
        Mge_DrawEnvironmentSkybox(env, camera);
        Mge_EndMode3D();
        Mge_EndTextureMode();

        Mge_DrawRenderTextureHDR(hdr, TONEMAP_ACES, 1.0f);
        Mge_EndDrawing();
    }

    Mge_UnloadEnvironment(&env);
    Mge_UnloadRenderTexture(hdr);
    Mge_UnloadTexture(iron.albedo);
    Mge_UnloadTexture(iron.normal);
    Mge_UnloadTexture(iron.metallic);
    Mge_UnloadTexture(iron.roughness);
    Mge_UnloadTexture(iron.ao);
    Mge_CloseWindow();
    return 0;
}
