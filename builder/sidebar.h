// The builder's left sidebar: mode, gizmo switch, entity list, inspector.
#pragma once

#include "scene.h"

// Draw the left panel. Call inside a Mge_GuiBeginFrame / Mge_GuiEndFrame pair
// (main.c owns the frame so the explorer panel shares it).
void Sidebar_Draw(Scene* s, bool editMode, int fps, int draws);
