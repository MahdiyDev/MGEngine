#include "scene.h"

#include <mge_math.h>
#include <math.h>

void Scene_Init(Scene* s)
{
    *s = (Scene){ 0 };

    s->objects[0] = Mge_MakeObject3D((Vector3){ 0.0f, -1.1f, 0.0f }, (Vector3){ 24.0f, 0.2f, 24.0f }, (Color){ 90, 95, 105, 255 });
    s->objects[1] = Mge_MakeObject3D((Vector3){ -3.0f, 0.0f, 0.0f }, (Vector3){ 1.5f, 1.5f, 1.5f }, (Color){ 200, 80, 80, 255 });
    s->objects[2] = Mge_MakeObject3D((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 1.5f, 1.5f, 1.5f }, (Color){ 90, 190, 110, 255 });
    s->objects[3] = Mge_MakeObject3D((Vector3){ 3.0f, 0.0f, 0.0f }, (Vector3){ 1.5f, 1.5f, 1.5f }, (Color){ 90, 130, 210, 255 });
    s->objectNames[0] = "Floor";
    s->objectNames[1] = "Cube 1";
    s->objectNames[2] = "Cube 2";
    s->objectNames[3] = "Cube 3";
    s->objectCount = 4;

    s->lights[0] = Mge_MakeDirectionalLight((Vector3){ -0.5f, -1.0f, -0.4f }, (Vector3){ 0.7f, 0.7f, 0.8f });
    s->lights[0].ambient = 0.22f; // fill so shadowed faces aren't pitch black
    s->lights[1] = Mge_MakePointLight((Vector3){ 3.0f, 5.0f, 2.0f }, (Vector3){ 1.0f, 0.85f, 0.6f });
    s->lightNames[0] = "Sun";
    s->lightNames[1] = "Lamp";
    s->lightCount = 2;

    s->selKind = SEL_NONE;
    s->selIndex = 0;

    s->shadow = Mge_LoadShadowMap(2048);
    s->shadowsOn = true;
    s->shadowCenter = (Vector3){ 0.0f, 0.0f, 0.0f };
    s->shadowRadius = 14.0f;

    s->sky = Mge_LoadCubemapDir("assets/skybox");
    if (s->sky.id == 0)
        s->sky = Mge_LoadCubemapDir("../assets/skybox");
}

void Scene_Shutdown(Scene* s)
{
    Mge_UnloadShadowMap(&s->shadow);
    Mge_UnloadCubemap(s->sky);
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
        Vector2 sc = Mge_GetWorldToScreenEx(s->objects[i].position, camera, w, h);
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
        return &s->objects[s->selIndex].position;
    if (s->selKind == SEL_LIGHT && s->lights[s->selIndex].type != LIGHT_DIRECTIONAL)
        return &s->lights[s->selIndex].position;
    return NULL;
}

Vector3* Scene_SelRotation(Scene* s)
{
    return (s->selKind == SEL_OBJECT) ? &s->objects[s->selIndex].rotation : NULL;
}

Vector3* Scene_SelScale(Scene* s)
{
    return (s->selKind == SEL_OBJECT) ? &s->objects[s->selIndex].size : NULL;
}

// fixed -- the gizmo is a constant on-screen tool, it does not track object size
#define GIZMO_SIZE 1.7f

bool Scene_Draw(Scene* s, Camera3D camera, bool interact)
{
    // reflect selection into Object.selected so Mge_DrawObject outlines it
    for (int i = 0; i < s->objectCount; i++)
        s->objects[i].selected = (s->selKind == SEL_OBJECT && s->selIndex == i);

    // pass 1: shadow depth from the sun
    if (s->shadowsOn) {
        Mge_BeginShadowPass(&s->shadow, s->lights[0], s->shadowCenter, s->shadowRadius);
        for (int i = 0; i < s->objectCount; i++)
            Draw_CubeEx(s->objects[i].position, s->objects[i].size, s->objects[i].rotation, s->objects[i].color);
        Mge_EndShadowPass();
    }

    Mge_ClearBackground((Color){ 20, 21, 26, 255 });

    Mge_BeginMode3D(camera);

    if (s->shadowsOn)
        Mge_BeginLighting3DShadowed(s->lights, s->lightCount, camera, s->shadow);
    else
        Mge_BeginLighting3DEx(s->lights, s->lightCount, camera);
    for (int i = 0; i < s->objectCount; i++)
        Mge_DrawObject(s->objects[i]);
    Mge_EndLighting3D();

    // lamp position marker
    for (int i = 0; i < s->lightCount; i++)
        if (s->lights[i].type != LIGHT_DIRECTIONAL)
            Draw_Cube(s->lights[i].position, (Vector3){ 0.3f, 0.3f, 0.3f }, (Color){ 255, 235, 180, 255 });

    if (s->sky.id != 0)
        Mge_DrawSkybox(s->sky, camera); // fills whatever the scene didn't

    // gizmo last so its depth-disabled draw sits on top of the skybox too;
    // only in edit mode (in fly mode the cursor is locked, nothing to grab)
    bool busy = false;
    Vector3* pos = Scene_SelPosition(s);
    if (interact && pos != NULL)
        busy = Mge_Gizmo3D(pos, Scene_SelRotation(s), Scene_SelScale(s), camera, GIZMO_SIZE);

    Mge_EndMode3D();
    return busy;
}
