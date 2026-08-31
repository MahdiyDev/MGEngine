// The editor's scene: entities, selection, and the render passes. No UI here.
#pragma once

#include <mge.h>
#include <stdbool.h>

enum { SEL_NONE = 0, SEL_OBJECT, SEL_LIGHT, SEL_ENV };

#define SCENE_MAX_OBJECTS 8
#define SCENE_MAX_LIGHTS  4

#define SCENE_TEXPATH_LEN 128

typedef struct Scene {
    char name[64];  // shown in the top bar; the scene-file stem
    char path[512]; // absolute path to this scene's `.mgscene` file ("" = never saved)
    bool dirty;     // unsaved edits since the last load / save

    Object objects[SCENE_MAX_OBJECTS];
    char objectNames[SCENE_MAX_OBJECTS][24]; // mutable: the hierarchy names + renames spawned shapes
    unsigned char texWrap[SCENE_MAX_OBJECTS][MATERIAL_MAP_COUNT]; // TextureWrap per map slot (0 = REPEAT)
    // source path of each material-map texture, relative to the scene dir (or
    // absolute); "" = no texture. Kept in step with the GL ids so the scene can
    // be serialised and reloaded.
    char texPath[SCENE_MAX_OBJECTS][MATERIAL_MAP_COUNT][SCENE_TEXPATH_LEN];
    int objectCount;

    Light lights[SCENE_MAX_LIGHTS]; // [0] = sun (directional, casts shadow), [1] = lamp (point)
    char lightNames[SCENE_MAX_LIGHTS][24];
    int lightCount;

    int selKind;  // SEL_*
    int selIndex;

    // --- Environment: the sun is lights[0]; here the skybox + which camera drives
    //     the view when the project is run (the editor always uses its fly-cam).
    char skyDir[SCENE_TEXPATH_LEN]; // project-root-relative folder of 6 faces ("" = none)
    int mainCamera;                // index of an OBJECT_CAMERA object, or -1

    ShadowMap shadow;
    bool shadowsOn;
    Vector3 shadowCenter;
    float shadowRadius;

    Cubemap sky;

    RenderTexture hdrRT;  // the lit pass renders here when hdrOn
    bool hdrOn;
    int toneMap;          // ToneMap
    float exposure;

    BloomFX bloom;        // used when hdrOn && bloomOn
    bool bloomOn;
} Scene;

void Scene_Init(Scene* s, int width, int height);
void Scene_Shutdown(Scene* s);

// Reset to a fresh default scene (floor + sun + lamp), name "untitled", no path.
void Scene_New(Scene* s);

// (Re)load every material-map texture from `texPath` into the GL ids, resolving
// project-root-relative paths against `projectRoot` (may be "" / NULL for the
// in-memory default project). Call after Scene_Load.
void Scene_LoadMaterialTextures(Scene* s, const char* projectRoot);

// Spawn a primitive at the origin, name it, and select it. No-op when the scene
// is full (SCENE_MAX_OBJECTS).
void Scene_AddShape(Scene* s, PrimitiveKind primitive);

// Add a point light above the origin, name it, and select it. No-op when full.
void Scene_AddLight(Scene* s);

// Add an OBJECT_CAMERA (position + look direction), name it, select it. If the
// scene has no main camera yet, this one becomes it.
void Scene_AddCamera(Scene* s);

// (Re)load the skybox cubemap from `<projectRoot>/skyDir` (pak-aware). Falls back
// to the bundled `assets/skybox` when `skyDir` is empty.
void Scene_LoadSkybox(Scene* s, const char* projectRoot);

// The Camera3D that `mainCamera` describes (position + rotation -> look-at,
// fovy 60). Returns false when there is no valid main camera.
bool Scene_MainCamera(const Scene* s, Camera3D* out);

// Remove an entity and re-pack the arrays. Deleting the directional sun (light 0)
// is a no-op -- the shadow pass depends on it. Selection falls back sensibly.
void Scene_DeleteObject(Scene* s, int index);
void Scene_DeleteLight(Scene* s, int index);

// Left-click picking: nearest object centre, or the lamp. Click on empty space
// deselects. Call only when the gizmo is not being dragged.
void Scene_Pick(Scene* s, Camera3D camera);

// The selected entity's live transform, or NULL. rot / scale are NULL for lights
// and for the (unmovable) directional sun.
Vector3* Scene_SelPosition(Scene* s);
Vector3* Scene_SelRotation(Scene* s);
Vector3* Scene_SelScale(Scene* s);

// Shadow pass + lit pass + skybox + (when `markers`) the editor-only lamp / camera
// icons + (when `interact`) the mouse-driven gizmo. The built player passes
// markers=false. Returns true while a gizmo handle is dragged.
bool Scene_Draw(Scene* s, Camera3D camera, bool interact, bool markers);
