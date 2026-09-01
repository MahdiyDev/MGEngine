// Loading + running a compiled scene module, with hot reload.
#pragma once

#include <mge.h>
#include <stdbool.h>

typedef struct SceneRuntime {
    void* handle;
    MgeSceneInitFn initFn;
    MgeSceneUpdateFn updateFn;
    MgeSceneShutdownFn shutdownFn;
    MgeSceneDrawFn drawFn;  // optional MgeScene_Draw export; NULL when absent
    int liveN;              // counter for the _live_<n> copies
    char liveDll[600];      // the loaded copy (deleted on unload)
    bool loaded;
    bool inited;            // Init called, Shutdown pending
    long sourceDigest;      // last-seen digest of the scene dir's *.c (hot-reload trigger)
} SceneRuntime;

// Copy `dllPath` to a fresh `<dir>/<stem>_live_<n>` (Windows locks the original)
// and load it; resolve MgeScene_Init / _Update / _Shutdown. On failure `err`
// (<= 256) gets a message. Any previously loaded module is unloaded first.
bool SceneRuntime_Load(SceneRuntime* rt, const char* dllPath, char* err, int errSize);

// Free the library and delete the copy. Call SceneRuntime_Shutdown first if the
// module was inited.
void SceneRuntime_Unload(SceneRuntime* rt);

void SceneRuntime_Init(SceneRuntime* rt, MgeSceneCtx* ctx);
void SceneRuntime_Update(SceneRuntime* rt, MgeSceneCtx* ctx, float dt);
// Run the module's optional MgeScene_Draw (no-op when it has none). Call inside
// Mge_BeginDrawing, after the host has drawn the scene.
void SceneRuntime_Draw(SceneRuntime* rt, MgeSceneCtx* ctx, Camera3D camera);
void SceneRuntime_Shutdown(SceneRuntime* rt, MgeSceneCtx* ctx);
bool SceneRuntime_Loaded(const SceneRuntime* rt);

// Digest of `sceneDir`'s `*.c` (sum of mtimes + a term for the file count, so a
// new or removed file also changes it). 0 if the dir can't be read.
long SceneRuntime_SourceDigest(const char* sceneDir);
