#include "scene.h"
#include "pathutil.h"

#include <mge_math.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A bare OBJECT_CAMERA: a transform (position + XYZ-euler look direction), no
// geometry or material. `rotDeg` {pitch, yaw, roll}; yaw -90 looks toward -Z.
static Object make_camera(Vector3 pos, Vector3 rotDeg)
{
    Object o = { 0 };
    o.kind = OBJECT_CAMERA;
    o.active = true;
    o.transform.position = pos;
    o.transform.rotation = rotDeg;
    o.transform.scale = (Vector3){ 1.0f, 1.0f, 1.0f };
    o.transform.parent = -1;
    return o;
}

// Load the 6-face cubemap in `dir`, or {0} if it isn't there. Peeks at
// `<dir>/right.jpg` first so a missing skybox doesn't spew FILEIO warnings while
// Scene_LoadSkybox walks its fallback chain. (Mge_LoadCubemapDir always hands
// back a live texture id; `.size` stays 0 when every face failed to decode.)
static Cubemap try_cubemap(const char* dir)
{
    char probe[1024];
    snprintf(probe, sizeof(probe), "%s/right.jpg", dir);
    FILE* pf = fopen(probe, "rb");
    if (pf != NULL) {
        fclose(pf);
    } else { // not loose -- maybe in a mounted pak (the built game)
        size_t n = 0;
        void* d = Mge_MountedRead(probe, &n);
        if (d == NULL)
            return (Cubemap){ 0 };
        free(d);
    }

    Cubemap c = Mge_LoadCubemapDir(dir);
    if (c.size == 0) {
        Mge_UnloadCubemap(c);
        return (Cubemap){ 0 };
    }
    return c;
}

// Reset the entity + settings fields to a fresh default scene, without touching
// the GL resources (shadow map / HDR target / bloom / sky). Shared by
// Scene_Init and Scene_New.
static void reset_data(Scene* s)
{
    for (int i = 0; i < s->objectCount; i++)
        for (int m = 0; m < MATERIAL_MAP_COUNT; m++)
            Mge_UnloadTexture(s->objects[i].material.maps[m].texture);

    memset(s->objects, 0, sizeof(s->objects));
    memset(s->objectNames, 0, sizeof(s->objectNames));
    memset(s->texWrap, 0, sizeof(s->texWrap));
    memset(s->texPath, 0, sizeof(s->texPath));
    memset(s->lights, 0, sizeof(s->lights));
    memset(s->lightNames, 0, sizeof(s->lightNames));

    s->objects[0] = Mge_MakeShape3D(PRIM_PLANE, (Vector3){ 0.0f, -1.1f, 0.0f }, (Vector3){ 24.0f, 0.2f, 24.0f }, (Color){ 90, 95, 105, 255 });
    s->objects[1] = Mge_MakeObject3D((Vector3){ -3.0f, 0.0f, 0.0f }, (Vector3){ 1.5f, 1.5f, 1.5f }, (Color){ 200, 80, 80, 255 });
    s->objects[2] = Mge_MakeShape3D(PRIM_SPHERE, (Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 1.5f, 1.5f, 1.5f }, (Color){ 90, 190, 110, 255 });
    s->objects[3] = Mge_MakeObject3D((Vector3){ 3.0f, 0.0f, 0.0f }, (Vector3){ 1.5f, 1.5f, 1.5f }, (Color){ 90, 130, 210, 255 });
    s->objects[4] = make_camera((Vector3){ 2.5f, 3.0f, 13.5f }, (Vector3){ -8.0f, -90.0f, 0.0f });
    strcpy(s->objectNames[0], "Floor");
    strcpy(s->objectNames[1], "Cube 1");
    strcpy(s->objectNames[2], "Sphere 1");
    strcpy(s->objectNames[3], "Cube 2");
    strcpy(s->objectNames[4], "Camera");
    s->objectCount = 5;

    s->lights[0] = Mge_MakeDirectionalLight((Vector3){ -0.5f, -1.0f, -0.4f }, (Vector3){ 0.7f, 0.7f, 0.8f });
    s->lights[0].ambient = 0.22f; // fill so shadowed faces aren't pitch black
    s->lights[1] = Mge_MakePointLight((Vector3){ 3.0f, 5.0f, 2.0f }, (Vector3){ 1.0f, 0.85f, 0.6f });
    strcpy(s->lightNames[0], "Sun");
    strcpy(s->lightNames[1], "Lamp");
    s->lightCount = 2;

    s->selKind = SEL_NONE;
    s->selIndex = 0;

    strcpy(s->skyDir, "res/skybox"); // project-relative; the editor falls back to assets/skybox
    s->mainCamera = 4;               // the "Camera" object above drives the built game's view

    strcpy(s->name, "untitled");
    s->path[0] = '\0';
    s->dirty = false;

    s->shadowsOn = true;
    s->shadowCenter = (Vector3){ 0.0f, 0.0f, 0.0f };
    s->shadowRadius = 14.0f;
    s->hdrOn = false; // opt-in -- tone mapping also affects the (LDR) skybox
    s->toneMap = TONEMAP_ACES;
    s->exposure = 1.0f;
    s->bloomOn = false;
}

void Scene_Init(Scene* s, int width, int height)
{
    *s = (Scene){ 0 };

    s->shadow = Mge_LoadShadowMap(2048);
    s->hdrRT = Mge_LoadRenderTextureHDR(width, height);
    s->bloom = Mge_LoadBloom(width, height);

    reset_data(s);
    Scene_LoadSkybox(s, ""); // no project yet -> bundled assets/skybox
}

// (Re)load the skybox cubemap. Tries `<projectRoot>/<skyDir>`, then a cwd-relative
// `skyDir`, then the engine's bundled `assets/skybox` so the viewport is never
// blank. Safe to call every time the scene or the active project changes.
void Scene_LoadSkybox(Scene* s, const char* projectRoot)
{
    const char* root = (projectRoot != NULL) ? projectRoot : "";

    Mge_UnloadCubemap(s->sky);
    s->sky = (Cubemap){ 0 };

    if (s->skyDir[0] != '\0') {
        char full[1024];
        if (Path_IsAbsolute(s->skyDir) || root[0] == '\0')
            snprintf(full, sizeof(full), "%s", s->skyDir);
        else
            Path_Join(root, s->skyDir, full, sizeof(full));
        s->sky = try_cubemap(full);
        if (s->sky.size == 0 && !Path_IsAbsolute(s->skyDir))
            s->sky = try_cubemap(s->skyDir); // cwd-relative
    }
    if (s->sky.size == 0)
        s->sky = try_cubemap("assets/skybox");
    if (s->sky.size == 0)
        s->sky = try_cubemap("../assets/skybox");
}

// The Camera3D described by the `mainCamera` object, or false when there is none.
bool Scene_MainCamera(const Scene* s, Camera3D* out)
{
    int i = s->mainCamera;
    if (i < 0 || i >= s->objectCount || s->objects[i].kind != OBJECT_CAMERA)
        return false;
    if (out != NULL) {
        const Object* o = &s->objects[i];
        out->position = o->transform.position;
        out->target = Mge_CameraObjectForward(o->transform.rotation); // engine cams store a direction
        out->up = (Vector3){ 0.0f, 1.0f, 0.0f };
        out->fovy = 60.0f;
        out->projection = CAMERA_PERSPECTIVE;
    }
    return true;
}

void Scene_New(Scene* s)
{
    reset_data(s);
}

void Scene_LoadMaterialTextures(Scene* s, const char* projectRoot)
{
    const char* root = (projectRoot != NULL) ? projectRoot : "";

    for (int i = 0; i < s->objectCount; i++) {
        for (int m = 0; m < MATERIAL_MAP_COUNT; m++) {
            Mge_UnloadTexture(s->objects[i].material.maps[m].texture);
            s->objects[i].material.maps[m].texture = (Texture2D){ 0 };

            const char* rel = s->texPath[i][m];
            if (rel[0] == '\0')
                continue;

            char full[1024];
            if (Path_IsAbsolute(rel) || root[0] == '\0')
                snprintf(full, sizeof(full), "%s", rel);
            else
                Path_Join(root, rel, full, sizeof(full)); // paths are project-root-relative

            Texture2D t = Mge_LoadTextureEx(full, m == MATERIAL_MAP_DIFFUSE);
            if (t.id == 0 && !Path_IsAbsolute(rel)) // fall back to a cwd-relative path
                t = Mge_LoadTextureEx(rel, m == MATERIAL_MAP_DIFFUSE);
            s->objects[i].material.maps[m].texture = t;
            if (t.id != 0)
                Mge_SetTextureWrap(t, s->texWrap[i][m]);
        }
    }
}

void Scene_Shutdown(Scene* s)
{
    for (int i = 0; i < s->objectCount; i++)
        for (int m = 0; m < MATERIAL_MAP_COUNT; m++)
            Mge_UnloadTexture(s->objects[i].material.maps[m].texture);

    Mge_UnloadShadowMap(&s->shadow);
    Mge_UnloadCubemap(s->sky);
    Mge_UnloadRenderTexture(s->hdrRT);
    Mge_UnloadBloom(&s->bloom);
}

void Scene_AddShape(Scene* s, PrimitiveKind primitive)
{
    if (s->objectCount >= SCENE_MAX_OBJECTS)
        return;

    static const char* nouns[3] = { "Cube", "Sphere", "Plane" };
    Vector3 size = (primitive == PRIM_PLANE) ? (Vector3){ 4.0f, 0.2f, 4.0f }
                                             : (Vector3){ 1.5f, 1.5f, 1.5f };

    int i = s->objectCount++;
    s->objects[i] = Mge_MakeShape3D(primitive, (Vector3){ 0.0f, 0.0f, 0.0f }, size, (Color){ 200, 200, 205, 255 });

    // number it after the existing shapes of the same kind
    int n = 1;
    for (int k = 0; k < i; k++)
        if (s->objects[k].kind == OBJECT_3D && s->objects[k].primitive == primitive)
            n++;
    snprintf(s->objectNames[i], sizeof(s->objectNames[i]), "%s %d", nouns[primitive], n);

    s->selKind = SEL_OBJECT;
    s->selIndex = i;
    s->dirty = true;
}

void Scene_AddCamera(Scene* s)
{
    if (s->objectCount >= SCENE_MAX_OBJECTS)
        return;

    int i = s->objectCount++;
    s->objects[i] = make_camera((Vector3){ 0.0f, 2.0f, 6.0f }, (Vector3){ -10.0f, -90.0f, 0.0f });

    int n = 1;
    for (int k = 0; k < i; k++)
        if (s->objects[k].kind == OBJECT_CAMERA)
            n++;
    if (n == 1)
        snprintf(s->objectNames[i], sizeof(s->objectNames[i]), "Camera");
    else
        snprintf(s->objectNames[i], sizeof(s->objectNames[i]), "Camera %d", n);

    if (s->mainCamera < 0)
        s->mainCamera = i; // first camera in the scene becomes the game view

    s->selKind = SEL_OBJECT;
    s->selIndex = i;
    s->dirty = true;
}

void Scene_AddLight(Scene* s)
{
    if (s->lightCount >= SCENE_MAX_LIGHTS)
        return;

    int i = s->lightCount++;
    s->lights[i] = Mge_MakePointLight((Vector3){ 0.0f, 4.0f, 0.0f }, (Vector3){ 1.0f, 1.0f, 1.0f });
    snprintf(s->lightNames[i], sizeof(s->lightNames[i]), "Light %d", i);

    s->selKind = SEL_LIGHT;
    s->selIndex = i;
    s->dirty = true;
}

void Scene_DeleteObject(Scene* s, int index)
{
    if (index < 0 || index >= s->objectCount)
        return;

    for (int m = 0; m < MATERIAL_MAP_COUNT; m++)
        Mge_UnloadTexture(s->objects[index].material.maps[m].texture);

    for (int i = index; i < s->objectCount - 1; i++) {
        s->objects[i] = s->objects[i + 1];
        memcpy(s->objectNames[i], s->objectNames[i + 1], sizeof(s->objectNames[i]));
        memcpy(s->texWrap[i], s->texWrap[i + 1], sizeof(s->texWrap[i]));
        memcpy(s->texPath[i], s->texPath[i + 1], sizeof(s->texPath[i]));
    }
    s->objectCount--;
    s->dirty = true;

    if (s->mainCamera == index)
        s->mainCamera = -1;
    else if (s->mainCamera > index)
        s->mainCamera--;

    if (s->selKind == SEL_OBJECT) {
        if (s->selIndex == index)
            s->selKind = SEL_NONE;
        else if (s->selIndex > index)
            s->selIndex--;
    }
}

void Scene_DeleteLight(Scene* s, int index)
{
    if (index <= 0 || index >= s->lightCount) // light 0 is the sun -- keep it
        return;

    for (int i = index; i < s->lightCount - 1; i++) {
        s->lights[i] = s->lights[i + 1];
        memcpy(s->lightNames[i], s->lightNames[i + 1], sizeof(s->lightNames[i]));
    }
    s->lightCount--;
    s->dirty = true;

    if (s->selKind == SEL_LIGHT) {
        if (s->selIndex == index)
            s->selKind = SEL_NONE;
        else if (s->selIndex > index)
            s->selIndex--;
    }
}

void Scene_Pick(Scene* s, Camera3D camera)
{
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        return;

    Vector2 m = GetMousePosition();
    int w = Mge_GetScreenWidth(), h = Mge_GetScreenHeight();

    int kind = SEL_NONE, index = 0;
    float best = 48.0f;

    for (int i = 0; i < s->objectCount; i++) {
        Vector2 sc = Mge_GetWorldToScreenEx(s->objects[i].transform.position, camera, w, h);
        float d = sqrtf((sc.x - m.x) * (sc.x - m.x) + (sc.y - m.y) * (sc.y - m.y));
        if (d < best) { best = d; kind = SEL_OBJECT; index = i; }
    }
    for (int i = 0; i < s->lightCount; i++) {
        if (s->lights[i].type == LIGHT_DIRECTIONAL)
            continue; // no world position
        Vector2 sc = Mge_GetWorldToScreenEx(s->lights[i].position, camera, w, h);
        float d = sqrtf((sc.x - m.x) * (sc.x - m.x) + (sc.y - m.y) * (sc.y - m.y));
        if (d < best) { best = d; kind = SEL_LIGHT; index = i; }
    }

    s->selKind = kind;
    s->selIndex = index;
}

Vector3* Scene_SelPosition(Scene* s)
{
    if (s->selKind == SEL_OBJECT)
        return &s->objects[s->selIndex].transform.position;
    if (s->selKind == SEL_LIGHT && s->lights[s->selIndex].type != LIGHT_DIRECTIONAL)
        return &s->lights[s->selIndex].position;
    return NULL;
}

Vector3* Scene_SelRotation(Scene* s)
{
    return (s->selKind == SEL_OBJECT) ? &s->objects[s->selIndex].transform.rotation : NULL;
}

Vector3* Scene_SelScale(Scene* s)
{
    if (s->selKind != SEL_OBJECT || s->objects[s->selIndex].kind == OBJECT_CAMERA)
        return NULL; // a camera has no size -- only move / rotate it
    return &s->objects[s->selIndex].transform.scale;
}

// fixed -- the gizmo is a constant on-screen tool, it does not track object size
#define GIZMO_SIZE 1.7f

bool Scene_Draw(Scene* s, Camera3D camera, bool interact, bool markers)
{
    // reflect selection into Object.selected so Mge_DrawObject outlines it
    for (int i = 0; i < s->objectCount; i++)
        s->objects[i].selected = (s->selKind == SEL_OBJECT && s->selIndex == i);

    // pass 1: shadow depth from the sun
    if (s->shadowsOn) {
        Mge_BeginShadowPass(&s->shadow, s->lights[0], s->shadowCenter, s->shadowRadius);
        for (int i = 0; i < s->objectCount; i++)
            if (s->objects[i].active)
                Mge_DrawPrimitive(s->objects[i], (Color){ 255, 255, 255, 255 });
        Mge_EndShadowPass();
    }

    if (s->hdrOn)
        Mge_BeginTextureMode(s->hdrRT); // lit pass -> floating-point target

    Mge_ClearBackground((Color){ 20, 21, 26, 255 });

    Mge_BeginMode3D(camera);

    if (s->shadowsOn)
        Mge_BeginLighting3DShadowed(s->lights, s->lightCount, camera, s->shadow);
    else
        Mge_BeginLighting3DEx(s->lights, s->lightCount, camera);
    for (int i = 0; i < s->objectCount; i++)
        if (s->objects[i].kind != OBJECT_CAMERA)
            Mge_DrawObject(s->objects[i]);
    Mge_EndLighting3D();

    // unlit editor markers, drawn after the lit pass: lamp positions + camera icons.
    // The built player passes markers=false so a game never shows its own gizmos.
    if (markers) {
        for (int i = 0; i < s->lightCount; i++)
            if (s->lights[i].type != LIGHT_DIRECTIONAL)
                Draw_Cube(s->lights[i].position, (Vector3){ 0.3f, 0.3f, 0.3f }, (Color){ 255, 235, 180, 255 });
        for (int i = 0; i < s->objectCount; i++)
            if (s->objects[i].kind == OBJECT_CAMERA)
                Mge_DrawObject(s->objects[i]);
    }

    if (s->sky.size != 0)
        Mge_DrawSkybox(s->sky, camera); // fills whatever the scene didn't

    // gizmo last so its depth-disabled draw sits on top of the skybox too;
    // only in edit mode (in fly mode the cursor is locked, nothing to grab)
    bool busy = false;
    Vector3* pos = Scene_SelPosition(s);
    if (interact && pos != NULL) {
        // skip it when the selection sits on / behind the eye (e.g. a camera
        // object where the fly-cam is) -- a depth-disabled gizmo at ~0 distance
        // smears across the whole viewport.
        Vector3 d = { pos->x - camera.position.x, pos->y - camera.position.y, pos->z - camera.position.z };
        float dist2 = d.x * d.x + d.y * d.y + d.z * d.z;
        float facing = d.x * camera.target.x + d.y * camera.target.y + d.z * camera.target.z;
        if (dist2 > 0.5f && facing > 0.0f)
            busy = Mge_Gizmo3D(pos, Scene_SelRotation(s), Scene_SelScale(s), camera, GIZMO_SIZE);
    }

    Mge_EndMode3D();

    if (s->hdrOn) {
        Mge_EndTextureMode();
        if (s->bloomOn)
            Mge_DrawBloom(s->hdrRT, &s->bloom, s->toneMap, s->exposure);
        else
            Mge_DrawRenderTextureHDR(s->hdrRT, s->toneMap, s->exposure);
    }

    return busy;
}
