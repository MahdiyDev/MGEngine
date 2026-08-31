// The editor's top strip: a Project menu, a Scene dropdown (switch / new / add /
// save), VIEW/EDIT mode, the gizmo mode + space controls, and a Render dropdown.
#pragma once

#include <mge.h>
#include "scene.h"
#include "project.h"

typedef enum {
    TOPBAR_NONE = 0,
    TOPBAR_PROJECT_NEW,
    TOPBAR_PROJECT_OPEN,
    TOPBAR_PROJECT_SAVE,
    TOPBAR_SCENE_NEW,
    TOPBAR_SCENE_ADD,
    TOPBAR_SCENE_SAVE,
    TOPBAR_SCENE_NEWSCRIPT, // scaffold another .c in the active scene's folder
    TOPBAR_SCENE_SWITCH,    // `arg` = target scene index
    TOPBAR_BUILD,        // compile the active scene's code
    TOPBAR_BUILD_RELEASE, // compile every scene release + pak + stage dist/
    TOPBAR_PLAY,         // build (if needed), load + run the scene module
    TOPBAR_STOP,         // shut down + unload the module, restore the scene
    TOPBAR_QUIT,         // never returned by Topbar_Draw; main.c uses it for the close guard
} TopbarAction;

typedef struct {
    TopbarAction action;
    int arg;
} TopbarResult;

// Draws the bar and returns the menu item the user clicked this frame (or
// TOPBAR_NONE). `editMode` and `showConsole` are owned by main.c (this toggles
// them); `playing` is read to label the Play/Stop button.
TopbarResult Topbar_Draw(Rectangle rect, Project* proj, Scene* s,
    bool* editMode, bool playing, bool* showConsole);
