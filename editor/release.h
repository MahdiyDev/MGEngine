// "Build Bundle" -- compile every scene module (with the project's debug or
// release cflags per `release`), pak the project data, and stage a runnable
// `<projectRoot>/dist/`: the player + engine DLL + project.mgproject at the
// root, scene modules in `dist/scenes/`, pak files in `dist/packs/`.
//
// Runs as a polled job so the editor keeps drawing: each scene compiles as a
// detached process (reusing SceneBuildJob), then the pak + runtime staging run
// inline in one poll (sub-second). Same shape as the Build / Play buttons.
#pragma once

#include <stdbool.h>

#include "project.h"
#include "scene_build.h" // BuildLog, SceneBuildJob

typedef enum {
    RJ_COMPILE = 0, // a per-scene compile is in flight
    RJ_STAGE,       // compiles done; pak + copy the runtime (inline, next poll)
    RJ_DONE,
    RJ_FAILED,
} ReleaseStage;

typedef struct ReleaseJob {
    ReleaseStage  stage;
    Project       proj;            // snapshot taken at Start
    bool          release;         // engine config: build/ vs build/release/
    int           sceneIdx;        // scene currently compiling
    char          dist[600];
    char          scenesDir[700];
    SceneBuildJob compile;         // the in-flight per-scene compile
    BuildLog*     log;
} ReleaseJob;

// Begin a bundle. Validates (saved project, SDK found), wipes + recreates
// `dist/`, and starts compiling the first scene. Returns false (reason in `log`)
// if setup failed -- then the job is inert and needs no Clear.
bool Release_Start(ReleaseJob* j, const Project* proj, bool release, BuildLog* log);

// Advance one step. Streams the current compile's output into `log`. Returns
// true once `j->stage` is RJ_DONE or RJ_FAILED.
bool Release_Poll(ReleaseJob* j);

// Reap a still-running per-scene compile and reset the job.
void Release_Clear(ReleaseJob* j);

// Synchronous convenience (headless harnesses / CI): drive a job to completion.
// Returns true if the bundle finished.
bool Release_Build(const Project* proj, bool release, BuildLog* log);
