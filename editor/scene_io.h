// `.mgscene` (de)serialisation -- a flat, line-based, diffable text format. Data
// only: no GL. After Scene_Load, call Scene_LoadMaterialTextures to bring in the
// textures named by `texPath`.
#pragma once

#include <mge.h>
#include "scene.h"

// Write `s` + the editor `camera` to `path` (a `.mgscene` file). When
// `projectRoot` is non-empty, textures whose source lies outside `<projectRoot>/res/`
// are copied into it and the stored path is rewritten `res/<file>` (resolved
// against the project root); when empty, texture paths are left as-is. Also
// scaffolds a `<stem>.c` template beside the file if absent. Returns true on
// success and, on success, updates `s->path` / `s->name` and clears `s->dirty`.
bool Scene_Save(Scene* s, const char* path, Camera3D camera, const char* projectRoot);

// Parse `path` into `s` (entities + render settings) and, if `outCamera` is not
// NULL, the editor camera. Reads via Mge_LoadFileText so a mounted pak works.
// Resets `s`'s entity arrays first; leaves the GL resources untouched. Sets
// `s->path` / `s->name`, clears `s->dirty`. Returns true on success.
bool Scene_Load(Scene* s, const char* path, Camera3D* outCamera);
