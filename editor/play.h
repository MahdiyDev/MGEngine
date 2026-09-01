// Play mode: compile the active scene's code, load it as a module, run
// MgeScene_Update each frame with real input, hot-reload on source change. The
// scene is snapshotted on Play and restored on Stop. While `playing` the editor
// hides its panels and views the scene through its main camera (see main.c).
#pragma once

#include <mge.h>
#include "scene.h"
#include "project.h"
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

    Camera3D viewCam;    // the camera the module sees as ctx.camera -- set each frame by main.c
    bool releaseCfg;     // Build Bundle config (set each frame by main.c): false = debug

    SceneBuildJob job;   // in-flight compile (runs as a separate process)
    int  jobPurpose;     // JOB_* -- what to do when the compile finishes
    long jobDigest;      // source digest captured when a hot-reload build started
} Play;

void Play_Init(Play* p);
void Play_Shutdown(Play* p, Scene* s);

// Handle a top-bar action. Returns true if it consumed the action
// (TOPBAR_BUILD / _PLAY / _STOP / _BUILD_RELEASE), false otherwise.
bool Play_Action(Play* p, TopbarAction a, Project* proj, Scene* s);

// Per frame: advance an async compile; while playing, hot-reload on source
// change then run MgeScene_Update with `p->viewCam` as the module's camera.
void Play_Frame(Play* p, Project* proj, Scene* s);

// The play-mode overlay strip: Stop + Console toggle + FPS. Returns true the
// frame Stop is pressed.
bool Play_DrawOverlay(Play* p, float screenW, int fps);

// Draw the console panel (call when p->showConsole is set).
void Play_DrawConsole(Play* p, Rectangle rect);
