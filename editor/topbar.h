// The editor's top strip: a File menu, VIEW/EDIT mode, the gizmo mode + space
// controls, and a Render dropdown for the post/lighting toggles.
#pragma once

#include <mge.h>
#include "scene.h"

typedef enum {
    TOPBAR_NONE = 0,
    TOPBAR_NEW,
    TOPBAR_OPEN,
    TOPBAR_SAVE,
    TOPBAR_SAVE_AS,
    TOPBAR_BUILD,
    TOPBAR_QUIT, // never returned by Topbar_Draw; main.c uses it for the close guard
} TopbarAction;

// Draws the bar and returns the File-menu item the user clicked this frame (or
// TOPBAR_NONE). `editMode` is owned by main.c (it also drives the OS cursor).
TopbarAction Topbar_Draw(Rectangle rect, Scene* s, bool* editMode);
