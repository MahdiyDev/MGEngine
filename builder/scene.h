// The builder's scene: entities, selection, and the render passes. No UI here.
#pragma once

#include <mge.h>
#include <stdbool.h>

enum { SEL_NONE = 0, SEL_OBJECT, SEL_LIGHT };

#define SCENE_MAX_OBJECTS 8
#define SCENE_MAX_LIGHTS  4

typedef struct Scene {
    Object objects[SCENE_MAX_OBJECTS];
    char objectNames[SCENE_MAX_OBJECTS][24]; // mutable: the explorer names spawned shapes
    unsigned char texWrap[SCENE_MAX_OBJECTS][MATERIAL_MAP_COUNT]; // TextureWrap per map slot (0 = REPEAT)
    int objectCount;

    Light lights[SCENE_MAX_LIGHTS]; // [0] = sun (directional, casts shadow), [1] = lamp (point)
    const char* lightNames[SCENE_MAX_LIGHTS];
    int lightCount;

    int selKind;  // SEL_*
    int selIndex;

    ShadowMap shadow;
    bool shadowsOn;
    Vector3 shadowCenter;
    float shadowRadius;

    Cubemap sky;

    RenderTexture hdrRT;  // the lit pass renders here when hdrOn
    bool hdrOn;
    int toneMap;          // ToneMap
    float exposure;
} Scene;

void Scene_Init(Scene* s, int width, int height);
void Scene_Shutdown(Scene* s);

// Spawn a primitive at the origin, name it, and select it. No-op when the scene
// is full (SCENE_MAX_OBJECTS).
void Scene_AddShape(Scene* s, PrimitiveKind primitive);

// Left-click picking: nearest object centre, or the lamp. Click on empty space
// deselects. Call only when the gizmo is not being dragged.
void Scene_Pick(Scene* s, Camera3D camera);

// The selected entity's live transform, or NULL. rot / scale are NULL for lights
// and for the (unmovable) directional sun.
Vector3* Scene_SelPosition(Scene* s);
Vector3* Scene_SelRotation(Scene* s);
Vector3* Scene_SelScale(Scene* s);

// Shadow pass + lit pass + lamp marker + gizmo + skybox. When `interact` is true
// the gizmo also handles the mouse. Returns true while a gizmo handle is dragged.
bool Scene_Draw(Scene* s, Camera3D camera, bool interact);
