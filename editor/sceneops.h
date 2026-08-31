// File-menu actions (New / Open / Save / Save As / Build / Quit) with an
// unsaved-changes guard: New / Open / Quit while the scene is dirty pop a confirm
// modal first, and the action runs only after Save or Discard.
#pragma once

#include "scene.h"
#include "editor_camera.h"
#include "topbar.h"

typedef struct SceneOps {
    TopbarAction pending; // action deferred behind the confirm modal (TOPBAR_NONE = none)
    bool quit;            // set true when the app should exit
} SceneOps;

// Handle a File-menu (or close-button) action. Runs it now, or defers it behind
// the confirm modal when it would discard unsaved edits.
void SceneOps_Request(SceneOps* ops, TopbarAction act, Scene* s, EditorCamera* cam);

// Draw the confirm modal (call every frame inside the Mge_Gui frame). Runs the
// pending action when the user picks Save or Discard.
void SceneOps_Draw(SceneOps* ops, Scene* s, EditorCamera* cam);
