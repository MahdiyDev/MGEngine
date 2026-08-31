// The editor's left panel: the scene's objects + lights as a flat list, with an
// "add entity" menu, per-row select / rename (double-click) / active toggle /
// delete. Selection here drives the inspector and the gizmo.
#pragma once

#include <mge.h>
#include "scene.h"
#include "history.h"

// Draws the panel. Returns true when the user asked to delete the current
// selection (via a row's `x`) -- the caller confirms + records history.
bool Hierarchy_Draw(Rectangle rect, Scene* s, History* h);
