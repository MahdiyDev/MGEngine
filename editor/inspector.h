// The editor's right panel: a type-aware inspector for the current selection
// (Object: active, primitive, transform, material slots. Light: type, colour,
// attenuation / direction).
#pragma once

#include <mge.h>
#include "scene.h"

void Inspector_Draw(Rectangle rect, Scene* s);
