// Project / scene File-menu actions with an unsaved-changes guard.
//
// New Project / Open Project / Scene switch / window-close, when the project or
// the active scene has unsaved edits, first pop a Save / Discard / Cancel modal.
// "New Scene" pops a name-entry modal.
#pragma once

#include "scene.h"
#include "project.h"
#include "editor_camera.h"
#include "topbar.h"

typedef struct FileOps {
    TopbarAction pending; // action deferred behind the confirm modal (TOPBAR_NONE = none)
    int pendingArg;
    bool quit;            // set true when the app should exit
    bool namePrompt;      // the name-entry modal is open
    int  promptKind;      // 0 = new scene, 1 = new script
    char nameBuf[64];
} FileOps;

// Handle a top-bar result. Runs it now, or defers it behind a modal.
void FileOps_Request(FileOps* ops, TopbarResult r, Project* proj, Scene* s, EditorCamera* cam);

// Draw the confirm + name-entry modals (call every frame inside the Mge_Gui
// frame). Runs the pending / new-scene action once the user commits.
void FileOps_Draw(FileOps* ops, Project* proj, Scene* s, EditorCamera* cam);
