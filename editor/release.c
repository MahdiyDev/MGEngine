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

// dist/scenes/scene.<idx>.dll
static void staged_module(const ReleaseJob* j, int idx, char* out, size_t n)
{
    char name[32];
    snprintf(name, sizeof(name), "scene.%d.dll", idx);
    Path_Join(j->scenesDir, name, out, n);
}

bool Release_Start(ReleaseJob* j, const Project* proj, bool release, BuildLog* log)
{
    memset(j, 0, sizeof(*j));
    j->proj = *proj; // flat POD -- safe to snapshot
    j->release = release;
    j->log = log;
    j->stage = RJ_FAILED;

    const char* cfg = release ? "Release" : "Debug";

    if (proj->path[0] == '\0') {
        BuildLog_Line(log, "Build Bundle: save the project first");
        return false;
    }
    char sdk[1024];
    if (!SceneBuild_FindSDK(sdk, sizeof(sdk))) {
        BuildLog_Line(log, "Build Bundle: engine SDK not found (set MGE_ENGINE)");
        return false;
    }
    if (proj->sceneCount <= 0) {
        BuildLog_Line(log, "Build Bundle: the project has no scenes");
        return false;
    }
    if (proj->dirty)
        BuildLog_Line(log, "note: project has unsaved changes -- the bundle packs the last saved state");

    char root[512];
    Project_Root(proj, root, sizeof(root));
    Path_Join(root, "dist", j->dist, sizeof(j->dist));

    BuildLog_Reset(log);
    BuildLog_Line(log, "== Build Bundle (%s) -> %s ==", cfg, j->dist);

    char packsDir[760];
    Path_Remove(j->dist);
    Path_MakeDirs(j->dist);
    Path_Join(j->dist, "scenes", j->scenesDir, sizeof(j->scenesDir));
    Path_Join(j->dist, "packs", packsDir, sizeof(packsDir));
    Path_MakeDirs(j->scenesDir);
    Path_MakeDirs(packsDir);

    // scene 0's compile
    BuildLog_Line(log, "-- scene '%s' -> scene.0.dll --", j->proj.scenes[0]);
    if (!SceneBuild_Start(&j->compile, &j->proj, j->proj.scenes[0], release, log)) {
        SceneBuild_Clear(&j->compile);
        return false;
    }
    j->sceneIdx = 0;
    j->stage = RJ_COMPILE;
    return true;
}

// pak the project data + copy the runtime; runs inline in one poll.
static void stage_runtime(ReleaseJob* j)
{
    char root[512], sdk[1024];
    Project_Root(&j->proj, root, sizeof(root));
    SceneBuild_FindSDK(sdk, sizeof(sdk)); // already validated in Start

    // pak every project file (project.mgproject + scenes/*.mgscene + res/) --
    // the walker skips build/ and dist/ and .dll/.exe. Fixed name so the player
    // can mount it before reading anything. Nothing is staged loose.
    char pakStem[760];
    Path_Join(j->dist, "packs/data", pakStem, sizeof(pakStem));
    BuildLog_Line(j->log, "-- packing data -> %s.pak.NNN --", pakStem);
    if (!Mge_PakWrite(pakStem, root, PAK_SPLIT)) {
        BuildLog_Line(j->log, "  pak write failed");
        j->stage = RJ_FAILED;
        return;
    }

    const char* hint = j->release ? "run `make release` in the SDK first"
                                  : "run `make` in the SDK first";
    char src[1200], dst[700];
    sdk_artifact(sdk, j->release, "libmgengine.dll", src, sizeof(src));
    Path_Join(j->dist, "libmgengine.dll", dst, sizeof(dst));
    if (!Path_CopyFile(src, dst)) {
        BuildLog_Line(j->log, "  missing %s -- %s", src, hint);
        j->stage = RJ_FAILED;
        return;
    }

    char exeName[128];
    snprintf(exeName, sizeof(exeName), "%s.exe", j->proj.name);
    sdk_artifact(sdk, j->release, "mgeplayer.exe", src, sizeof(src));
    Path_Join(j->dist, exeName, dst, sizeof(dst));
    if (!Path_CopyFile(src, dst)) {
        sdk_artifact(sdk, j->release, "mgeplayer", src, sizeof(src)); // POSIX
        snprintf(exeName, sizeof(exeName), "%s", j->proj.name);
        Path_Join(j->dist, exeName, dst, sizeof(dst));
        if (!Path_CopyFile(src, dst)) {
            BuildLog_Line(j->log, "  missing the player (%s) -- %s", src, hint);
            j->stage = RJ_FAILED;
            return;
        }
    }

    BuildLog_Line(j->log, "== done. run: %s/%s ==", j->dist, exeName);
    j->stage = RJ_DONE;
}

bool Release_Poll(ReleaseJob* j)
{
    if (j->stage == RJ_DONE || j->stage == RJ_FAILED)
        return true;

    if (j->stage == RJ_STAGE) {
        stage_runtime(j);
        return true; // done or failed
    }

    // RJ_COMPILE
    if (!SceneBuild_Poll(&j->compile))
        return false;

    bool ok = j->compile.ok;
    char dll[768];
    snprintf(dll, sizeof(dll), "%s", j->compile.outDll);
    SceneBuild_Clear(&j->compile);

    if (!ok) {
        BuildLog_Line(j->log, "-- Build Bundle FAILED at scene '%s' --", j->proj.scenes[j->sceneIdx]);
        j->stage = RJ_FAILED;
        return true;
    }

    char dst[800];
    staged_module(j, j->sceneIdx, dst, sizeof(dst));
    if (!Path_CopyFile(dll, dst)) {
        BuildLog_Line(j->log, "  copy failed: %s", dst);
        j->stage = RJ_FAILED;
        return true;
    }

    j->sceneIdx++;
    if (j->sceneIdx < j->proj.sceneCount) {
        BuildLog_Line(j->log, "-- scene '%s' -> scene.%d.dll --",
            j->proj.scenes[j->sceneIdx], j->sceneIdx);
        if (!SceneBuild_Start(&j->compile, &j->proj, j->proj.scenes[j->sceneIdx],
                j->release, j->log)) {
            SceneBuild_Clear(&j->compile);
            j->stage = RJ_FAILED;
            return true;
        }
        return false; // keep compiling
    }

    j->stage = RJ_STAGE; // next poll paks + stages
    return false;
}

void Release_Clear(ReleaseJob* j)
{
    if (j->stage == RJ_COMPILE)
        SceneBuild_Clear(&j->compile);
    memset(j, 0, sizeof(*j));
}

bool Release_Build(const Project* proj, bool release, BuildLog* log)
{
    ReleaseJob j;
    if (!Release_Start(&j, proj, release, log))
        return false;
    while (!Release_Poll(&j)) { }
    bool done = (j.stage == RJ_DONE);
    Release_Clear(&j);
    return done;
}
