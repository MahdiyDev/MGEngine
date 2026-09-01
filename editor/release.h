// "Build Bundle" -- compile every scene module (with the project's debug or
// release cflags per `release`), pak the project data, and stage a runnable
// `<projectRoot>/dist/`: the player + engine DLL + project.mgproject at the
// root, scene modules in `dist/scenes/`, pak files in `dist/packs/`.
#pragma once

#include <stdbool.h>

#include "project.h"
#include "scene_build.h" // BuildLog

// `release` picks the engine config (debug build/ vs build/release/) and the
// scene cflags. Everything reported into `log`; returns true if the bundle is
// complete.
bool Release_Build(const Project* proj, bool release, BuildLog* log);
