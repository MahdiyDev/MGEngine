// Headless render smoke test.
//
//   (cd .. && make)          # build the engine objects first
//   make render              # build + run; writes TGAs to test/render_out/
//
// Opens a hidden GLFW window (real GL context / real driver), renders a handful
// of engine features one frame each, reads the framebuffer back, and checks:
//   (a) glGetError() == GL_NO_ERROR   -- no invalid GL
//   (b) the image is not a flat colour -- something actually drew
// Every frame is also saved as a .tga so a human can eyeball what rendered
// (the windowed examples can't be launched under this machine's app-control).
//
// Needs a GPU + a desktop session. On a truly headless box GLFW fails to make a
// context and the engine exits -- that's expected, run this where you have one.

#include "mge.h"
#include "mge_gl.h"
#include "mge_math.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
    #include <direct.h>
    #define MKDIR(p) _mkdir(p)
#else
    #include <sys/stat.h>
    #define MKDIR(p) mkdir(p, 0755)
#endif

#define W 320
#define H 240
#define OUT_DIR "render_out"

static unsigned char g_px[W * H * 4];
static int g_fails;
static int g_scenes;

static void write_tga(const char* name, const unsigned char* rgba)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/%s.tga", OUT_DIR, name);
    FILE* f = fopen(path, "wb");
    if (f == NULL)
        return;

    unsigned char hdr[18] = { 0 };
    hdr[2] = 2;               // uncompressed true-colour
    hdr[12] = (unsigned char)(W & 0xFF);
    hdr[13] = (unsigned char)((W >> 8) & 0xFF);
    hdr[14] = (unsigned char)(H & 0xFF);
    hdr[15] = (unsigned char)((H >> 8) & 0xFF);
    hdr[16] = 32;             // bpp
    hdr[17] = 0x20;           // top-left origin
    fwrite(hdr, 1, sizeof(hdr), f);

    for (int i = 0; i < W * H; i++) {
        const unsigned char* p = &rgba[i * 4];
        unsigned char bgra[4] = { p[2], p[1], p[0], p[3] }; // TGA is BGRA
        fwrite(bgra, 1, 4, f);
    }
    fclose(f);
}

// fraction of pixels that differ noticeably from the top-left pixel
static double content_fraction(const unsigned char* p)
{
    int diff = 0;
    for (int i = 0; i < W * H; i++) {
        int d = abs(p[i * 4 + 0] - p[0]) + abs(p[i * 4 + 1] - p[1]) + abs(p[i * 4 + 2] - p[2]);
        if (d > 16)
            diff++;
    }
    return (double)diff / (W * H);
}

// call right before Mge_EndDrawing: flush, read back, save, judge
static void check(const char* name)
{
    MgeGL_Draw();
    glFinish();
    GLenum err = glGetError();
    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, g_px);
    write_tga(name, g_px);

    double frac = content_fraction(g_px);
    int ok = (err == GL_NO_ERROR) && (frac > 0.01);
    g_scenes++;
    if (!ok)
        g_fails++;
    printf("  %-16s  glError=0x%04X  content=%5.1f%%   %s\n",
        name, (unsigned)err, frac * 100.0, ok ? "ok" : "FAIL");
}

// ---- scenes ----

static void scene_shapes(void)
{
    Mge_BeginDrawing();
    Mge_ClearBackground((Color){ 28, 30, 40, 255 });
    Draw_RectangleRec((Rectangle){ 40, 40, 120, 90 }, (Color){ 220, 80, 80, 255 });
    Draw_TriangleLines((Vector2){ 220, 40 }, (Vector2){ 190, 160 }, (Vector2){ 280, 160 },
        (Color){ 90, 220, 130, 255 });
    check("shapes_2d");
    Mge_EndDrawing();
}

static void scene_cube_lit(void)
{
    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 50.0f, .projection = CAMERA_PERSPECTIVE };
    cam.position = (Vector3){ 3.5f, 3.0f, 5.0f };
    cam.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, cam.position));
    Light sun = Mge_MakeDirectionalLight((Vector3){ -0.4f, -1.0f, -0.4f }, (Vector3){ 1, 1, 1 });
    sun.ambient = 0.3f;

    Mge_BeginDrawing();
    Mge_ClearBackground((Color){ 18, 20, 26, 255 });
    Mge_BeginMode3D(cam);
    Mge_BeginLighting3D(sun, cam);
    Draw_Cube((Vector3){ 0, 0, 0 }, (Vector3){ 2, 2, 2 }, (Color){ 120, 170, 220, 255 });
    Mge_EndLighting3D();
    Mge_EndMode3D();
    check("cube_lit_3d");
    Mge_EndDrawing();
}

static void scene_shadow(void)
{
    ShadowMap sm = Mge_LoadShadowMap(1024);
    Light sun = Mge_MakeDirectionalLight((Vector3){ -0.5f, -1.0f, -0.35f }, (Vector3){ 1, 1, 1 });
    sun.ambient = 0.28f;
    Material mat = Mge_DefaultMaterial();
    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 45.0f, .projection = CAMERA_PERSPECTIVE };
    cam.position = (Vector3){ 5.0f, 5.0f, 7.0f };
    cam.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0.5f, 0 }, cam.position));

    const Vector3 fp = { 0, -0.05f, 0 }, fs = { 10, 0.1f, 10 };
    const Vector3 bp = { 0, 1.0f, 0 }, bs = { 2, 2, 2 };

    Mge_BeginDrawing();
    Mge_BeginShadowPass(&sm, sun, (Vector3){ 0, 0.5f, 0 }, 7.0f);
    Draw_Cube(fp, fs, (Color){ 180, 180, 185, 255 });
    Draw_Cube(bp, bs, (Color){ 200, 120, 90, 255 });
    Mge_EndShadowPass();

    Mge_ClearBackground((Color){ 30, 33, 40, 255 });
    Mge_BeginMode3D(cam);
    Mge_BeginLighting3DShadowed(&sun, 1, cam, sm);
    Mge_SetMaterial(mat);
    Draw_Cube(fp, fs, (Color){ 180, 180, 185, 255 });
    Draw_Cube(bp, bs, (Color){ 200, 120, 90, 255 });
    Mge_EndLighting3D();
    Mge_EndMode3D();
    check("shadow_map");
    Mge_EndDrawing();

    Mge_UnloadShadowMap(&sm);
}

static void scene_postfx(void)
{
    RenderTexture rt = Mge_LoadRenderTexture(W, H);

    Mge_BeginDrawing();
    Mge_BeginTextureMode(rt);
    Mge_ClearBackground((Color){ 40, 60, 80, 255 });
    for (int i = 0; i < 5; i++)
        Draw_RectangleRec((Rectangle){ 20.0f + i * 55.0f, 30.0f + i * 20.0f, 70, 90 },
            (Color){ (unsigned char)(60 + i * 40), 200, (unsigned char)(220 - i * 30), 255 });
    Mge_EndTextureMode();

    Mge_ClearBackground((Color){ 0, 0, 0, 255 });
    Mge_DrawRenderTextureFX(rt, POSTFX_EDGE);
    check("postfx_edge");
    Mge_EndDrawing();

    Mge_UnloadRenderTexture(rt);
}

static void scene_skybox(void)
{
    Cubemap sky = Mge_LoadCubemapDir("../assets/skybox");
    if (sky.id == 0) {
        printf("  %-16s  (skipped -- assets/skybox not loadable)\n", "skybox");
        return;
    }

    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 60.0f, .projection = CAMERA_PERSPECTIVE };
    cam.position = (Vector3){ 0, 0, 0 };
    cam.target = (Vector3){ 0.6f, 0.1f, -0.8f };

    Mge_BeginDrawing();
    Mge_ClearBackground((Color){ 0, 0, 0, 255 });
    Mge_BeginMode3D(cam);
    Mge_DrawSkybox(sky, cam);
    Mge_EndMode3D();
    check("skybox");
    Mge_EndDrawing();

    Mge_UnloadCubemap(sky);
}

static void scene_gizmo(GizmoMode mode, const char* name)
{
    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 50.0f, .projection = CAMERA_PERSPECTIVE };
    cam.position = (Vector3){ 4.0f, 3.5f, 5.5f };
    cam.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, cam.position));
    Light sun = Mge_MakeDirectionalLight((Vector3){ -0.4f, -1.0f, -0.4f }, (Vector3){ 1, 1, 1 });
    sun.ambient = 0.35f;

    Object o = Mge_MakeObject3D((Vector3){ 0, 0, 0 }, (Vector3){ 2, 2, 2 }, (Color){ 130, 170, 210, 255 });
    o.rotation = (Vector3){ 20.0f, 35.0f, 0.0f }; // exercises Draw_CubeEx

    Mge_SetGizmoMode(mode);

    Mge_BeginDrawing();
    Mge_ClearBackground((Color){ 22, 24, 30, 255 });
    Mge_BeginMode3D(cam);
    Mge_BeginLighting3D(sun, cam);
    Mge_DrawObject(o);
    Mge_EndLighting3D();
    Mge_Gizmo3D(&o.position, &o.rotation, &o.size, cam, 2.4f); // no mouse -> just draws
    Mge_EndMode3D();
    check(name);
    Mge_EndDrawing();
}

// the rotate gizmo: a ring the camera faces head-on must draw as a full circle
static void scene_rotate_gizmo(Vector3 camPos, const char* name)
{
    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 50.0f, .projection = CAMERA_PERSPECTIVE };
    cam.position = camPos;
    cam.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, cam.position));
    Light sun = Mge_MakeDirectionalLight((Vector3){ -0.4f, -1.0f, -0.4f }, (Vector3){ 1, 1, 1 });
    sun.ambient = 0.4f;

    Object o = Mge_MakeObject3D((Vector3){ 0, 0, 0 }, (Vector3){ 1.6f, 1.6f, 1.6f }, (Color){ 120, 130, 140, 255 });
    Mge_SetGizmoMode(GIZMO_ROTATE);

    Mge_BeginDrawing();
    Mge_ClearBackground((Color){ 24, 26, 32, 255 });
    Mge_BeginMode3D(cam);
    Mge_BeginLighting3D(sun, cam);
    Mge_DrawObject(o);
    Mge_EndLighting3D();
    Mge_Gizmo3D(&o.position, &o.rotation, &o.size, cam, 1.7f);
    Mge_EndMode3D();
    check(name);
    Mge_EndDrawing();
}

static void scene_normal_map(void)
{
    Texture2D d = Mge_LoadTexture("../assets/brickwall/brickwall.jpg");
    Texture2D n = Mge_LoadTexture("../assets/brickwall/brickwall_normal.jpg");
    if (d.id == 0) {
        printf("  %-16s  (skipped -- assets/brickwall not present)\n", "normal_map");
        return;
    }
    Material wall = Mge_DefaultMaterial();
    Mge_SetMaterialTexture(&wall, MATERIAL_MAP_DIFFUSE, d);
    Mge_SetMaterialTexture(&wall, MATERIAL_MAP_NORMAL, n);

    Light lamp = Mge_MakePointLight((Vector3){ 1.2f, 0.8f, 2.0f }, (Vector3){ 1, 1, 1 });
    lamp.ambient = 0.1f;
    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 45.0f, .projection = CAMERA_PERSPECTIVE };
    cam.position = (Vector3){ 0, 0, 4.0f };
    cam.target = (Vector3){ 0, 0, -1 };

    Mge_BeginDrawing();
    Mge_ClearBackground((Color){ 12, 12, 16, 255 });
    Mge_BeginMode3D(cam);
    Mge_BeginLighting3D(lamp, cam);
    Mge_SetMaterial(wall);
    Draw_Cube((Vector3){ 0, 0, 0 }, (Vector3){ 4, 4, 0.2f }, WHITE);
    Mge_EndLighting3D();
    Mge_EndMode3D();
    check("normal_map");
    Mge_EndDrawing();
}

int main(void)
{
    Mge_SetDebugOutput(true); // loud GL errors in the log

    Mge_InitWindow(W, H, "mge render smoke");
    GLFWwindow* win = (GLFWwindow*)Mge_GetWindowHandle();
    if (win != NULL)
        glfwHideWindow(win);
    Mge_SetTargetFPS(0);

    MKDIR(OUT_DIR);
    printf("render smoke test  %dx%d  -> %s/\n", W, H, OUT_DIR);

    scene_shapes();
    scene_cube_lit();
    scene_shadow();
    scene_postfx();
    scene_skybox();
    scene_normal_map();
    scene_gizmo(GIZMO_TRANSLATE, "gizmo_translate");
    scene_gizmo(GIZMO_ROTATE, "gizmo_rotate");
    scene_gizmo(GIZMO_SCALE, "gizmo_scale");
    scene_rotate_gizmo((Vector3){ 0.2f, 0.3f, 6.0f }, "rot_facing_z"); // face-on -> full ring
    scene_rotate_gizmo((Vector3){ 6.0f, 0.3f, 0.2f }, "rot_facing_x");

    Mge_CloseWindow();

    printf("\n%d/%d scenes ok%s\n", g_scenes - g_fails, g_scenes,
        g_fails ? "  -- FAILURES above" : "");
    return g_fails ? 1 : 0;
}
