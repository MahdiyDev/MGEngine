// The right-edge "Explorer" panel: a palette of primitives to spawn into the
// scene. Every Mge_Gui* call for it lives in explorer.c.
#pragma once

#include "scene.h"

// Draw the panel. Call inside a Mge_GuiBeginFrame / Mge_GuiEndFrame pair.
void Explorer_Draw(Scene* s);
