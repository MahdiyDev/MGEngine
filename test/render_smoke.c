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

#include <math.h>

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

// the three 3D primitive kinds -- cube, sphere, plane -- lit in one frame
static void scene_primitives(void)
{
    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 55.0f, .projection = CAMERA_PERSPECTIVE };
    cam.position = (Vector3){ 4.5f, 3.5f, 6.0f };
    cam.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, cam.position));
    Light sun = Mge_MakeDirectionalLight((Vector3){ -0.4f, -1.0f, -0.4f }, (Vector3){ 1, 1, 1 });
    sun.ambient = 0.35f;

    Object floor = Mge_MakeShape3D(PRIM_PLANE, (Vector3){ 0, -1.2f, 0 }, (Vector3){ 10, 0.2f, 10 }, (Color){ 90, 95, 105, 255 });
    Object cube = Mge_MakeObject3D((Vector3){ -2.2f, 0, 0 }, (Vector3){ 1.6f, 1.6f, 1.6f }, (Color){ 200, 90, 90, 255 });
    Object ball = Mge_MakeShape3D(PRIM_SPHERE, (Vector3){ 1.8f, 0, 0 }, (Vector3){ 2, 2, 2 }, (Color){ 90, 170, 220, 255 });

    Mge_BeginDrawing();
    Mge_ClearBackground((Color){ 22, 24, 30, 255 });
    Mge_BeginMode3D(cam);
    Mge_BeginLighting3D(sun, cam);
    Mge_DrawObject(floor);
    Mge_DrawObject(cube);
    Mge_DrawObject(ball);
    Mge_EndLighting3D();
    Mge_EndMode3D();
    check("primitives");
    Mge_EndDrawing();
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

// a scripted mouse drag on the rotate gizmo -- Mge_SetMouseOverride feeds fake
// press / drag / release across three frames; the object must end up rotated
static void scene_scripted_rotate(void)
{
    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 50.0f, .projection = CAMERA_PERSPECTIVE };
    cam.position = (Vector3){ 3.5f, 3.0f, 6.0f };
    cam.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, cam.position));
    Light sun = Mge_MakeDirectionalLight((Vector3){ -0.4f, -1.0f, -0.4f }, (Vector3){ 1, 1, 1 });
    sun.ambient = 0.4f;
    Object o = Mge_MakeObject3D((Vector3){ 0, 0, 0 }, (Vector3){ 1.8f, 1.8f, 1.8f }, (Color){ 150, 130, 110, 255 });
    Mge_SetGizmoMode(GIZMO_ROTATE);
    Mge_SetGizmoSpace(GIZMO_WORLD);

    Vector2 gc = Mge_GetWorldToScreen((Vector3){ 0, 0, 0 }, cam);
    Vector2 grab = Mge_GetWorldToScreen((Vector3){ 1.7f, 0.9f, 0.0f }, cam); // a point on a ring
    Vector2 radial = { grab.x - gc.x, grab.y - gc.y };
    Vector2 dragTo = { grab.x - radial.y * 0.5f, grab.y + radial.x * 0.5f }; // ~tangential nudge

    for (int f = 0; f < 3; f++) {
        Mge_SetMouseOverride(f == 0 ? grab : dragTo, f < 2); // press, drag, release
        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 24, 26, 32, 255 });
        Mge_BeginMode3D(cam);
        Mge_BeginLighting3D(sun, cam);
        Mge_DrawObject(o);
        Mge_EndLighting3D();
        Mge_Gizmo3D(&o.position, &o.rotation, &o.size, cam, 1.9f);
        Mge_EndMode3D();
        if (f == 2)
            check("scripted_rotate");
        Mge_EndDrawing();
    }
    Mge_ClearMouseOverride();

    printf("    scripted drag -> object euler (%.0f, %.0f, %.0f)\n",
        (double)o.rotation.x, (double)o.rotation.y, (double)o.rotation.z);
    if (fabsf(o.rotation.x) + fabsf(o.rotation.y) + fabsf(o.rotation.z) < 1.0f) {
        printf("    scripted_rotate  FAIL  -- the drag did not rotate the object\n");
        g_fails++;
    }
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
    wall.maps[MATERIAL_MAP_NORMAL].value = 1.8f; // exaggerate the relief (strength control)

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

// parallax-occlusion mapping: LearnOpenGL's bricks2 set on an angled wall
static void scene_parallax(void)
{
    Texture2D d = Mge_LoadTexture("../assets/bricks/bricks2.jpg");
    if (d.id == 0) {
        printf("  %-16s  (skipped -- assets/bricks not present)\n", "parallax");
        return;
    }
    Texture2D nrm = Mge_LoadTexture("../assets/bricks/bricks2_normal.jpg");
    Texture2D disp = Mge_LoadTexture("../assets/bricks/bricks2_disp.jpg"); // depth map

    Material wall = Mge_DefaultMaterial();
    Mge_SetMaterialTexture(&wall, MATERIAL_MAP_DIFFUSE, d);
    if (nrm.id != 0)
        Mge_SetMaterialTexture(&wall, MATERIAL_MAP_NORMAL, nrm);
    Mge_SetMaterialTexture(&wall, MATERIAL_MAP_HEIGHT, disp);
    wall.maps[MATERIAL_MAP_HEIGHT].value = 0.1f;

    Light lamp = Mge_MakePointLight((Vector3){ 1.4f, 0.6f, 2.0f }, (Vector3){ 1, 1, 1 });
    lamp.ambient = 0.12f;
    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 45.0f, .projection = CAMERA_PERSPECTIVE };
    cam.position = (Vector3){ 2.6f, 0.4f, 3.4f }; // off to the side -> grazing angle
    cam.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, cam.position));

    Mge_BeginDrawing();
    Mge_ClearBackground((Color){ 12, 12, 16, 255 });
    Mge_BeginMode3D(cam);
    Mge_BeginLighting3D(lamp, cam);
    Mge_SetMaterial(wall);
    Draw_Cube((Vector3){ 0, 0, 0 }, (Vector3){ 4, 4, 0.2f }, WHITE);
    Mge_EndLighting3D();
    Mge_EndMode3D();
    check("parallax");
    Mge_EndDrawing();
}

// texture wrap: one quad, UVs 0..3, mirror-repeat -> a tiled + flipped pattern
static void scene_texwrap(void)
{
    Texture2D t = Mge_LoadTexture("../assets/bricks/bricks2.jpg");
    if (t.id == 0) {
        printf("  %-16s  (skipped -- assets/bricks not present)\n", "texwrap");
        return;
    }
    Mge_SetTextureWrap(t, TEXTURE_WRAP_MIRROR_REPEAT);

    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 45.0f, .projection = CAMERA_PERSPECTIVE };
    cam.position = (Vector3){ 0, 0, 4.5f };
    cam.target = (Vector3){ 0, 0, -1 };
    Light sun = Mge_MakeDirectionalLight((Vector3){ 0, 0, -1 }, (Vector3){ 1, 1, 1 });
    sun.ambient = 0.9f;

    Material m = Mge_DefaultMaterial();
    Mge_SetMaterialTexture(&m, MATERIAL_MAP_DIFFUSE, t);

    Mge_BeginDrawing();
    Mge_ClearBackground((Color){ 10, 10, 14, 255 });
    Mge_BeginMode3D(cam);
    Mge_BeginLighting3D(sun, cam);
    Mge_SetMaterial(m);
    MgeGL_Begin(MGEGL_TRIANGLES);
    MgeGL_Color4ub(255, 255, 255, 255);
    MgeGL_Normal3f(0, 0, 1);
    const float q[4][5] = { // x, y, z, u, v  -- UV spans 0..3 so wrap shows
        { -2, -2, 0, 0, 0 }, { 2, -2, 0, 3, 0 }, { 2, 2, 0, 3, 3 }, { -2, 2, 0, 0, 3 },
    };
    const int tri[6] = { 0, 1, 2, 0, 2, 3 };
    for (int i = 0; i < 6; i++) {
        const float* v = q[tri[i]];
        MgeGL_TexCoord2f(v[3], v[4]);
        MgeGL_Vertex3f(v[0], v[1], v[2]);
    }
    MgeGL_End();
    Mge_EndLighting3D();
    Mge_EndMode3D();
    check("texwrap");
    Mge_EndDrawing();

    Mge_UnloadTexture(t);
}

// material tiling + triplanar: a tiled plane behind a stretched (1x1x3) box that
// keeps square texels because it's projected from world space
static void scene_tiling(void)
{
    Texture2D t = Mge_LoadTexture("../assets/bricks/bricks2.jpg");
    if (t.id == 0) {
        printf("  %-16s  (skipped -- assets/bricks not present)\n", "tiling");
        return;
    }

    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 55.0f, .projection = CAMERA_PERSPECTIVE };
    cam.position = (Vector3){ 3.5f, 2.5f, 6.0f };
    cam.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, cam.position));
    Light sun = Mge_MakeDirectionalLight((Vector3){ -0.4f, -1.0f, -0.5f }, (Vector3){ 1, 1, 1 });
    sun.ambient = 0.4f;

    Material floorMat = Mge_DefaultMaterial();
    Mge_SetMaterialTexture(&floorMat, MATERIAL_MAP_DIFFUSE, t);
    floorMat.tiling = (Vector2){ 4.0f, 4.0f }; // repeat 4x4 instead of stretching one copy

    Texture2D nrm = Mge_LoadTexture("../assets/bricks/bricks2_normal.jpg");
    Texture2D disp = Mge_LoadTexture("../assets/bricks/bricks2_disp.jpg");

    Material boxMat = Mge_DefaultMaterial();
    Mge_SetMaterialTexture(&boxMat, MATERIAL_MAP_DIFFUSE, t);
    if (nrm.id != 0)
        Mge_SetMaterialTexture(&boxMat, MATERIAL_MAP_NORMAL, nrm);
    if (disp.id != 0)
        Mge_SetMaterialTexture(&boxMat, MATERIAL_MAP_HEIGHT, disp);
    boxMat.triplanar = true;       // diffuse + normal + height all project from world XYZ
    boxMat.triplanarScale = 1.0f;  // 1 world unit per tile -> square texels on a 1x1x3 box
    boxMat.maps[MATERIAL_MAP_HEIGHT].value = 0.06f;

    Object floorObj = Mge_MakeShape3D(PRIM_PLANE, (Vector3){ 0, -1.6f, 0 }, (Vector3){ 12, 0.2f, 12 }, WHITE);
    Object box = Mge_MakeObject3D((Vector3){ 0, 0, 0 }, (Vector3){ 1, 1, 3 }, WHITE); // non-uniform scale

    Mge_BeginDrawing();
    Mge_ClearBackground((Color){ 14, 15, 20, 255 });
    Mge_BeginMode3D(cam);
    Mge_BeginLighting3D(sun, cam);
    floorObj.material = floorMat;
    box.material = boxMat;
    Mge_DrawObject(floorObj);
    Mge_DrawObject(box);
    Mge_EndLighting3D();
    Mge_EndMode3D();
    check("tiling");
    Mge_EndDrawing();

    Mge_UnloadTexture(t);
}

// HDR: a very bright light rendered into an RGBA16F target, then tone-mapped.
// Also checks the "raw clamp" path (no tone map) so both branches run.
static void scene_hdr(void)
{
    RenderTexture hdr = Mge_LoadRenderTextureHDR(W, H);

    Light bright = Mge_MakePointLight((Vector3){ 0, 1.5f, 2.0f }, (Vector3){ 20.0f, 20.0f, 18.0f });
    bright.ambient = 0.03f;
    Light fill = Mge_MakePointLight((Vector3){ -2.0f, 1.0f, -4.0f }, (Vector3){ 0.3f, 0.5f, 1.0f });

    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 55.0f, .projection = CAMERA_PERSPECTIVE };
    cam.position = (Vector3){ 0, 1.4f, 4.0f };
    cam.target = (Vector3){ 0, 0, -1 };
    Light lights[2] = { bright, fill };

    const char* names[2] = { "hdr_tonemap", "hdr_clamp" };
    for (int pass = 0; pass < 2; pass++) {
        Mge_BeginDrawing();
        Mge_BeginTextureMode(hdr);
        Mge_ClearBackground((Color){ 3, 3, 5, 255 });
        Mge_BeginMode3D(cam);
        Mge_BeginLighting3DEx(lights, 2, cam);
        Draw_Cube((Vector3){ 0, -1.0f, -4 }, (Vector3){ 6, 0.3f, 16 }, (Color){ 200, 195, 185, 255 });
        Draw_Cube((Vector3){ 0, 0.4f, -3 }, (Vector3){ 1.2f, 1.2f, 1.2f }, (Color){ 210, 120, 90, 255 });
        Mge_EndLighting3D();
        Mge_EndMode3D();
        Mge_EndTextureMode();

        if (pass == 0)
            Mge_DrawRenderTextureHDR(hdr, TONEMAP_ACES, 1.0f);
        else
            Mge_DrawRenderTextureFX(hdr, POSTFX_NONE);
        check(names[pass]);
        Mge_EndDrawing();
    }

    Mge_UnloadRenderTexture(hdr);
}

// Bloom: the bright light should bleed a soft glow (composite = scene + blur).
static void scene_bloom(void)
{
    RenderTexture hdr = Mge_LoadRenderTextureHDR(W, H);
    BloomFX bloom = Mge_LoadBloom(W, H);
    bloom.intensity = 1.0f;

    Light bright = Mge_MakePointLight((Vector3){ 0, 1.2f, 1.0f }, (Vector3){ 30.0f, 26.0f, 12.0f });
    bright.ambient = 0.02f;
    Light lights[1] = { bright };

    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 55.0f, .projection = CAMERA_PERSPECTIVE };
    cam.position = (Vector3){ 0, 1.2f, 4.5f };
    cam.target = (Vector3){ 0, 0, -1 };

    Mge_BeginDrawing();
    Mge_BeginTextureMode(hdr);
    Mge_ClearBackground((Color){ 2, 2, 4, 255 });
    Mge_BeginMode3D(cam);
    Mge_BeginLighting3DEx(lights, 1, cam);
    Draw_Cube((Vector3){ 0, -1.0f, -4 }, (Vector3){ 8, 0.3f, 18 }, (Color){ 200, 195, 185, 255 });
    Draw_Cube((Vector3){ -2.0f, 0.2f, -3 }, (Vector3){ 1, 1, 1 }, (Color){ 200, 120, 90, 255 });
    Mge_EndLighting3D();
    Draw_Cube(bright.position, (Vector3){ 0.25f, 0.25f, 0.25f }, WHITE); // the glowing bulb
    Mge_EndMode3D();
    Mge_EndTextureMode();

    Mge_DrawBloom(hdr, &bloom, TONEMAP_ACES, 1.0f);
    check("bloom");
    Mge_EndDrawing();

    Mge_UnloadBloom(&bloom);
    Mge_UnloadRenderTexture(hdr);
}

// Deferred shading: geometry pass -> G-buffer -> one full-screen lighting pass.
static void scene_deferred(void)
{
    GBuffer g = Mge_LoadGBuffer(W, H);

    Light lights[6];
    for (int i = 0; i < 6; i++) {
        float a = (float)i / 6.0f * 6.28318f;
        lights[i] = Mge_MakePointLight((Vector3){ cosf(a) * 3.0f, 0.6f, sinf(a) * 3.0f - 3.0f },
            (Vector3){ (i % 3 == 0) ? 3.0f : 0.4f, (i % 3 == 1) ? 3.0f : 0.4f, (i % 3 == 2) ? 3.0f : 0.4f });
        lights[i].ambient = (i == 0) ? 0.08f : 0.0f;
    }

    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 55.0f, .projection = CAMERA_PERSPECTIVE };
    cam.position = (Vector3){ 0, 3.5f, 5.0f };
    cam.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, -3 }, cam.position));

    Object floorObj = Mge_MakeShape3D(PRIM_PLANE, (Vector3){ 0, -1.0f, -3 }, (Vector3){ 14, 0.2f, 14 },
        (Color){ 180, 175, 170, 255 });
    Object cube = Mge_MakeObject3D((Vector3){ 0, 0, -3 }, (Vector3){ 1.6f, 1.6f, 1.6f }, (Color){ 200, 120, 90, 255 });
    Object ball = Mge_MakeShape3D(PRIM_SPHERE, (Vector3){ -2.4f, 0, -3 }, (Vector3){ 2, 2, 2 }, (Color){ 90, 170, 220, 255 });

    Mge_BeginDrawing();
    Mge_ClearBackground((Color){ 6, 7, 10, 255 });
    Mge_BeginMode3D(cam);
    Mge_BeginGeometryPass(&g, cam);
    Mge_DrawObject(floorObj);
    Mge_DrawObject(cube);
    Mge_DrawObject(ball);
    Mge_EndGeometryPass();
    Mge_EndMode3D();

    Mge_DeferredLighting(g, lights, 6, cam);
    check("deferred");
    Mge_EndDrawing();

    Mge_UnloadGBuffer(&g);
}

// SSAO: a box wedged into a corner -- the creases should darken.
static void scene_ssao(void)
{
    GBuffer g = Mge_LoadGBuffer(W, H);
    SSAO ao = Mge_LoadSSAO(W, H);
    ao.radius = 0.8f;
    ao.power = 2.5f;

    Light sun = Mge_MakeDirectionalLight((Vector3){ -0.4f, -0.9f, -0.5f }, (Vector3){ 1, 1, 1 });
    sun.ambient = 0.6f; // SSAO bites into this
    sun.diffuse = 0.4f;
    Light lights[1] = { sun };

    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 55.0f, .projection = CAMERA_PERSPECTIVE };
    cam.position = (Vector3){ 4, 3.5f, 4 };
    cam.target = Vector3Normalize(Vector3_Subtract((Vector3){ 0, 0, 0 }, cam.position));

    Object floorObj = Mge_MakeShape3D(PRIM_PLANE, (Vector3){ 0, -1, 0 }, (Vector3){ 12, 0.2f, 12 }, (Color){ 190, 188, 185, 255 });
    Object wall = Mge_MakeObject3D((Vector3){ 0, 1, -2.5f }, (Vector3){ 12, 5, 0.4f }, (Color){ 185, 183, 180, 255 });
    Object box = Mge_MakeObject3D((Vector3){ 0, 0, -1 }, (Vector3){ 2, 2, 2 }, (Color){ 200, 140, 110, 255 });
    Object ball = Mge_MakeShape3D(PRIM_SPHERE, (Vector3){ -2.4f, -0.2f, 0 }, (Vector3){ 1.6f, 1.6f, 1.6f }, (Color){ 130, 170, 210, 255 });

    Mge_BeginDrawing();
    Mge_ClearBackground((Color){ 10, 11, 15, 255 });
    Mge_BeginMode3D(cam);
    Mge_BeginGeometryPass(&g, cam);
    Mge_DrawObject(floorObj);
    Mge_DrawObject(wall);
    Mge_DrawObject(box);
    Mge_DrawObject(ball);
    Mge_EndGeometryPass();
    Mge_EndMode3D();

    Mge_ComputeSSAO(&ao, g, cam);
    Mge_DeferredLightingAO(g, lights, 1, cam, ao.aoBlur.texture.id);
    check("ssao");

    // also exercise the raw-AO view
    Mge_ClearBackground((Color){ 0, 0, 0, 255 });
    Mge_DrawRenderTextureFX(ao.aoBlur, POSTFX_NONE);
    check("ssao_raw");
    Mge_EndDrawing();

    Mge_UnloadSSAO(&ao);
    Mge_UnloadGBuffer(&g);
}

// PBR + IBL: a metallic/rough sphere grid under an environment map.
static void scene_pbr(void)
{
    Environment env = Mge_LoadEnvironment("../assets/hdr/newport_loft.hdr");
    if (env.irradiance == 0) {
        printf("  %-16s  (skipped -- assets/hdr not present)\n", "pbr");
        return;
    }
    RenderTexture hdr = Mge_LoadRenderTextureHDR(W, H);

    Light lights[2] = {
        Mge_MakePointLight((Vector3){ -6, 6, 8 }, (Vector3){ 12, 12, 12 }),
        Mge_MakePointLight((Vector3){ 6, -3, 8 }, (Vector3){ 6, 6, 7 }),
    };

    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 55.0f, .projection = CAMERA_PERSPECTIVE };
    cam.position = (Vector3){ 0, 0, 10 };
    cam.target = (Vector3){ 0, 0, -1 };

    Mge_BeginDrawing();
    Mge_BeginTextureMode(hdr);
    Mge_ClearBackground((Color){ 4, 4, 6, 255 });
    Mge_BeginMode3D(cam);
    Mge_BeginPBR3DIBL(lights, 2, cam, env);
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            PBRMaterial m = Mge_DefaultPBRMaterial();
            m.albedoColor = (Vector3){ 0.6f, 0.16f, 0.13f };
            m.metallicValue = row / 4.0f;
            m.roughnessValue = (col / 4.0f < 0.05f) ? 0.05f : col / 4.0f;
            Mge_SetPBRMaterial(m);
            Draw_Sphere((Vector3){ (col - 2) * 2.2f, (row - 2) * 2.2f, 0 }, 0.9f, WHITE);
        }
    }
    Mge_EndPBR3D();
    Mge_DrawEnvironmentSkybox(env, cam);
    Mge_EndMode3D();
    Mge_EndTextureMode();

    Mge_DrawRenderTextureHDR(hdr, TONEMAP_ACES, 1.0f);
    check("pbr");
    Mge_EndDrawing();

    Mge_UnloadRenderTexture(hdr);
    Mge_UnloadEnvironment(&env);
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
    scene_parallax();
    scene_texwrap();
    scene_tiling();
    scene_hdr();
    scene_bloom();
    scene_deferred();
    scene_ssao();
    scene_pbr();
    scene_primitives();
    scene_gizmo(GIZMO_TRANSLATE, "gizmo_translate");
    scene_gizmo(GIZMO_ROTATE, "gizmo_rotate");
    scene_gizmo(GIZMO_SCALE, "gizmo_scale");
    scene_rotate_gizmo((Vector3){ 0.2f, 0.3f, 6.0f }, "rot_facing_z"); // face-on -> full ring
    scene_rotate_gizmo((Vector3){ 6.0f, 0.3f, 0.2f }, "rot_facing_x");
    scene_scripted_rotate();

    Mge_CloseWindow();

    printf("\n%d/%d scenes ok%s\n", g_scenes - g_fails, g_scenes,
        g_fails ? "  -- FAILURES above" : "");
    return g_fails ? 1 : 0;
}
