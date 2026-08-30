// The builder's sidebar. All Mge_Gui* calls live here -- nothing else touches UI.
#pragma once

#include "scene.h"

// Draws the whole UI frame (BeginFrame -> sidebar -> EndFrame).
void Sidebar_Draw(Scene* s, bool editMode, int fps, int draws);
