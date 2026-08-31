#include "release.h"
#include "pathutil.h"

#include <mge.h> // Mge_PakWrite

#include <stdio.h>
#include <string.h>

#define PAK_SPLIT (1024u * 1024u * 1024u) // ~1 GB per .pak.NNN

bool Release_Build(const Project* proj, BuildLog* log)
{
    if (proj->path[0] == '\0') {
        BuildLog_Line(log, "Build Release: save the project first");
        return false;
    }

    char root[512], sdk[1024];
    Project_Root(proj, root, sizeof(root));
    if (!SceneBuild_FindSDK(sdk, sizeof(sdk))) {
        BuildLog_Line(log, "Build Release: engine SDK not found (set MGE_ENGINE)");
        return false;
    }

    char dist[600];
    Path_Join(root, "dist", dist, sizeof(dist));
    BuildLog_Reset(log);
    BuildLog_Line(log, "== Build Release -> %s ==", dist);
    Path_Remove(dist);
    Path_MakeDirs(dist);

    // 1. compile every scene module with release flags, stage as dist/<name>.dll
    for (int i = 0; i < proj->sceneCount; i++) {
        char dll[700], dst[700];
        BuildLog_Line(log, "-- scene '%s' --", proj->scenes[i]);
        if (!SceneBuild_Compile(proj, proj->scenes[i], true, log, dll, sizeof(dll)))
            return false;
        Path_Join(dist, proj->scenes[i], dst, sizeof(dst));
        strncat(dst, ".dll", sizeof(dst) - strlen(dst) - 1);
        if (!Path_CopyFile(dll, dst)) {
            BuildLog_Line(log, "  copy failed: %s", dst);
            return false;
        }
    }

    // 2. pak the project data (scenes/*.mgscene + res/). the walker skips build/
    //    and dist/ and .dll/.exe, so this is safe.
    char pakStem[700];
    Path_Join(dist, proj->name, pakStem, sizeof(pakStem));
    BuildLog_Line(log, "-- packing data -> %s.pak.NNN --", pakStem);
    if (!Mge_PakWrite(pakStem, root, PAK_SPLIT)) {
        BuildLog_Line(log, "  pak write failed");
        return false;
    }

    // project.mgproject also loose: the player reads it to learn the pak's name.
    char projDst[700];
    Path_Join(dist, "project.mgproject", projDst, sizeof(projDst));
    Path_CopyFile(proj->path, projDst);

    // 3. runtime: libmgengine + the player, renamed to the project.
    char src[1024], dst[700];
    Path_Join(sdk, "build/libmgengine.dll", src, sizeof(src));
    Path_Join(dist, "libmgengine.dll", dst, sizeof(dst));
    if (!Path_CopyFile(src, dst)) {
        BuildLog_Line(log, "  missing %s -- run `make` in the SDK first", src);
        return false;
    }
    Path_Join(sdk, "build/mgeplayer.exe", src, sizeof(src));
    char exeName[128];
    snprintf(exeName, sizeof(exeName), "%s.exe", proj->name);
    Path_Join(dist, exeName, dst, sizeof(dst));
    if (!Path_CopyFile(src, dst)) {
        Path_Join(sdk, "build/mgeplayer", src, sizeof(src)); // POSIX
        snprintf(exeName, sizeof(exeName), "%s", proj->name);
        Path_Join(dist, exeName, dst, sizeof(dst));
        if (!Path_CopyFile(src, dst)) {
            BuildLog_Line(log, "  missing the player (%s) -- run `make` in the SDK first", src);
            return false;
        }
    }

    BuildLog_Line(log, "== done. run: %s/%s ==", dist, exeName);
    return true;
}
