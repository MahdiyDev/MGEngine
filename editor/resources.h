// The editor's bottom panel: the per-scene resource explorer. A stub until
// Phase 5 (file tree of the active scene's res/, import, file ops).
#pragma once

#include <mge.h>
#include "scene.h"
#include "project.h"

void Resources_Draw(Rectangle rect, Project* proj, Scene* s, int fps, int draws);
