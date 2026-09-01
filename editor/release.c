#include "release.h"
#include "pathutil.h"

#include <mge.h> // Mge_PakWrite

#include <stdio.h>
#include <string.h>

#define PAK_SPLIT (1024u * 1024u * 1024u) // ~1 GB per .pak.NNN

// join <sdk>/build[/release]/<leaf> into out
static void sdk_artifact(const char* sdk, bool release, const char* leaf, char* out, size_t n)
{
    char buildDir[1040];
    Path_Join(sdk, release ? "build/release" : "build", buildDir, sizeof(buildDir));
    Path_Join(buildDir, leaf, out, n);
}

bool Release_Build(const Project* proj, bool release, BuildLog* log)
{
    const char* cfg = release ? "Release" : "Debug";

    if (proj->path[0] == '\0') {
        BuildLog_Line(log, "Build Bundle: save the project first");
        return false;
    }

    char root[512], sdk[1024];
    Project_Root(proj, root, sizeof(root));
    if (!SceneBuild_FindSDK(sdk, sizeof(sdk))) {
        BuildLog_Line(log, "Build Bundle: engine SDK not found (set MGE_ENGINE)");
        return false;
    }

    // the pak captures project.mgproject + scenes/*.mgscene straight off disk
    if (proj->dirty)
        BuildLog_Line(log, "note: project has unsaved changes -- the bundle packs the last saved state");

    char dist[600], scenesDir[700], packsDir[700];
    Path_Join(root, "dist", dist, sizeof(dist));
    BuildLog_Reset(log);
    BuildLog_Line(log, "== Build Bundle (%s) -> %s ==", cfg, dist);
    Path_Remove(dist);
    Path_MakeDirs(dist);
    Path_Join(dist, "scenes", scenesDir, sizeof(scenesDir));
    Path_Join(dist, "packs", packsDir, sizeof(packsDir));
    Path_MakeDirs(scenesDir);
    Path_MakeDirs(packsDir);

    // 1. compile every scene module, stage as dist/scenes/scene.<index>.dll --
    //    the player addresses it by its index into project.scenes[], so scene
    //    names never reach the shipped folder.
    for (int i = 0; i < proj->sceneCount; i++) {
        char dll[700], dst[800], modName[32];
        BuildLog_Line(log, "-- scene '%s' -> scene.%d.dll --", proj->scenes[i], i);
        if (!SceneBuild_Compile(proj, proj->scenes[i], release, log, dll, sizeof(dll)))
            return false;
        snprintf(modName, sizeof(modName), "scene.%d", i);
        Path_Join(scenesDir, modName, dst, sizeof(dst));
        strncat(dst, ".dll", sizeof(dst) - strlen(dst) - 1);
        if (!Path_CopyFile(dll, dst)) {
            BuildLog_Line(log, "  copy failed: %s", dst);
            return false;
        }
    }

    // 2. pak every project file (project.mgproject + scenes/*.mgscene + res/)
    //    into dist/packs/data.pak.NNN. Fixed name so the player can mount it
    //    before it has read anything. The walker skips build/ and dist/ and
    //    .dll/.exe, so this is safe. Nothing is staged loose -- the shipped
    //    game has no editable project file.
    char pakStem[800];
    Path_Join(packsDir, "data", pakStem, sizeof(pakStem));
    BuildLog_Line(log, "-- packing data -> %s.pak.NNN --", pakStem);
    if (!Mge_PakWrite(pakStem, root, PAK_SPLIT)) {
        BuildLog_Line(log, "  pak write failed");
        return false;
    }

    // 3. runtime: libmgengine + the player (renamed to the project), from the
    //    matching engine config.
    const char* hint = release ? "run `make release` in the SDK first"
                               : "run `make` in the SDK first";
    char src[1200], dst[700];
    sdk_artifact(sdk, release, "libmgengine.dll", src, sizeof(src));
    Path_Join(dist, "libmgengine.dll", dst, sizeof(dst));
    if (!Path_CopyFile(src, dst)) {
        BuildLog_Line(log, "  missing %s -- %s", src, hint);
        return false;
    }
    sdk_artifact(sdk, release, "mgeplayer.exe", src, sizeof(src));
    char exeName[128];
    snprintf(exeName, sizeof(exeName), "%s.exe", proj->name);
    Path_Join(dist, exeName, dst, sizeof(dst));
    if (!Path_CopyFile(src, dst)) {
        sdk_artifact(sdk, release, "mgeplayer", src, sizeof(src)); // POSIX
        snprintf(exeName, sizeof(exeName), "%s", proj->name);
        Path_Join(dist, exeName, dst, sizeof(dst));
        if (!Path_CopyFile(src, dst)) {
            BuildLog_Line(log, "  missing the player (%s) -- %s", src, hint);
            return false;
        }
    }

    BuildLog_Line(log, "== done. run: %s/%s ==", dist, exeName);
    return true;
}
