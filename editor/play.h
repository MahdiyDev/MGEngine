// Play mode: compile the active scene's code, load it as a module, run
// MgeScene_Update each frame, hot-reload on source change. Owns the build log
// + the console panel.
#pragma once

#include <mge.h>
#include "scene.h"
#include "project.h"
#include "editor_camera.h"
#include "topbar.h"
#include "scene_build.h"
#include "scene_runtime.h"

enum { JOB_NONE = 0, JOB_BUILD, JOB_PLAY, JOB_RELOAD };

typedef struct Play {
    SceneRuntime rt;
    BuildLog log;
    bool playing;
    bool showConsole;
    Scene snapshot; // whole-Scene copy taken on Play; restored (minus GPU handles) on Stop

    SceneBuildJob job;   // in-flight compile (runs as a separate process)
    int  jobPurpose;     // JOB_* -- what to do when the compile finishes
    long jobDigest;      // source digest captured when a hot-reload build started
} Play;

void Play_Init(Play* p);
void Play_Shutdown(Play* p, Scene* s, EditorCamera* cam);

// Handle a top-bar action. Returns true if it consumed the action
// (TOPBAR_BUILD / _PLAY / _STOP), false otherwise.
bool Play_Action(Play* p, TopbarAction a, Project* proj, Scene* s, EditorCamera* cam);

// Per frame: while playing, hot-reload on source change then run MgeScene_Update.
void Play_Frame(Play* p, Project* proj, Scene* s, EditorCamera* cam);

// Draw the console panel (call when p->showConsole is set).
void Play_DrawConsole(Play* p, Rectangle rect);
