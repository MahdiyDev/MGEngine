#include "scene_runtime.h"
#include "pathutil.h"

#include <stdio.h>
#include <string.h>

bool SceneRuntime_Loaded(const SceneRuntime* rt) { return rt->loaded; }

void SceneRuntime_Unload(SceneRuntime* rt)
{
    if (rt->handle != NULL)
        Mge_FreeLibrary(rt->handle);
    if (rt->liveDll[0] != '\0')
        remove(rt->liveDll);

    rt->handle = NULL;
    rt->initFn = NULL;
    rt->updateFn = NULL;
    rt->shutdownFn = NULL;
    rt->drawFn = NULL;
    rt->liveDll[0] = '\0';
    rt->loaded = false;
    rt->inited = false;
}

bool SceneRuntime_Load(SceneRuntime* rt, const char* dllPath, char* err, int errSize)
{
    if (rt->loaded)
        SceneRuntime_Unload(rt);

    char dir[480], stem[64], ext_stem[64];
    Path_Dir(dllPath, dir, sizeof(dir));
    Path_Base(dllPath, stem, sizeof(stem));
    snprintf(ext_stem, sizeof(ext_stem), "%s", stem);
    Path_StripExt(ext_stem);

    rt->liveN++;
    const char* ext = strrchr(stem, '.');
    snprintf(rt->liveDll, sizeof(rt->liveDll), "%s/%s_live_%d%s",
        dir, ext_stem, rt->liveN, ext ? ext : "");

    if (!Path_CopyFile(dllPath, rt->liveDll)) {
        snprintf(err, (size_t)errSize, "couldn't copy %s", dllPath);
        rt->liveDll[0] = '\0';
        return false;
    }

    rt->handle = Mge_LoadLibrary(rt->liveDll);
    if (rt->handle == NULL) {
        snprintf(err, (size_t)errSize, "%s", Mge_GetDylibError());
        remove(rt->liveDll);
        rt->liveDll[0] = '\0';
        return false;
    }

    rt->initFn = (MgeSceneInitFn)Mge_GetSymbol(rt->handle, "MgeScene_Init");
    rt->updateFn = (MgeSceneUpdateFn)Mge_GetSymbol(rt->handle, "MgeScene_Update");
    rt->shutdownFn = (MgeSceneShutdownFn)Mge_GetSymbol(rt->handle, "MgeScene_Shutdown");
    rt->drawFn = (MgeSceneDrawFn)Mge_GetSymbol(rt->handle, "MgeScene_Draw"); // optional
    if (rt->updateFn == NULL) {
        snprintf(err, (size_t)errSize, "module has no MgeScene_Update");
        SceneRuntime_Unload(rt);
        return false;
    }

    rt->loaded = true;
    rt->inited = false;
    return true;
}

void SceneRuntime_Init(SceneRuntime* rt, MgeSceneCtx* ctx)
{
    if (rt->loaded && !rt->inited) {
        if (rt->initFn != NULL)
            rt->initFn(ctx);
        rt->inited = true;
    }
}

void SceneRuntime_Update(SceneRuntime* rt, MgeSceneCtx* ctx, float dt)
{
    if (rt->loaded && rt->inited && rt->updateFn != NULL)
        rt->updateFn(ctx, dt);
}

void SceneRuntime_Draw(SceneRuntime* rt, MgeSceneCtx* ctx, Camera3D camera)
{
    if (rt->loaded && rt->inited && rt->drawFn != NULL)
        rt->drawFn(ctx, camera);
}

void SceneRuntime_Shutdown(SceneRuntime* rt, MgeSceneCtx* ctx)
{
    if (rt->loaded && rt->inited) {
        if (rt->shutdownFn != NULL)
            rt->shutdownFn(ctx);
        rt->inited = false;
    }
}

long SceneRuntime_SourceDigest(const char* sceneDir)
{
    char names[64][128];
    int nc = Path_List(sceneDir, ".c", false, names, 64);
    if (nc < 0)
        return 0;

    long sum = (long)nc * 1000003L;
    for (int i = 0; i < nc; i++) {
        char full[700];
        Path_Join(sceneDir, names[i], full, sizeof(full));
        sum += Path_MTime(full);
    }
    return sum;
}
