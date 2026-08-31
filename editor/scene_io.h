// scene.mge (de)serialisation -- a flat, line-based, diffable text format. Data
// only: no GL. After Scene_Load, call Scene_LoadMaterialTextures to bring in the
// textures named by `texPath`.
#pragma once

#include <mge.h>
#include "scene.h"

// Write `s` + the editor `camera` to `path` (a `.mge` file). Textures whose
// source lies outside the scene directory are copied into `<dir>/res/` and the
// stored path is rewritten relative. Also (re)creates `<dir>/res/` and, if it is
// absent, a template `<stem>.c` for Phase 3. Returns true on success and, on
// success, updates `s->path` / `s->name` and clears `s->dirty`.
bool Scene_Save(Scene* s, const char* path, Camera3D camera);

// Parse `path` into `s` (entities + render settings) and, if `outCamera` is not
// NULL, the editor camera. Resets `s`'s entity arrays first; leaves the GL
// resources (shadow map, HDR target, bloom, sky) untouched. Sets `s->path` /
// `s->name`, clears `s->dirty`. Returns true on success.
bool Scene_Load(Scene* s, const char* path, Camera3D* outCamera);
