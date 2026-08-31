// The editor's top strip: scene name + file actions, VIEW/EDIT mode, the gizmo
// mode + space controls, and a Render dropdown for the post/lighting toggles.
#pragma once

#include <mge.h>
#include "scene.h"

// `editMode` is owned by main.c (it also drives the OS cursor); the bar flips it.
void Topbar_Draw(Rectangle rect, Scene* s, bool* editMode);
