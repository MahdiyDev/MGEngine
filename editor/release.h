// "Build Release" -- compile every scene module with the project's release
// cflags, pak the project data, and stage a runnable `<projectRoot>/dist/`.
#pragma once

#include "project.h"
#include "scene_build.h" // BuildLog

// Everything reported into `log`; returns true if the bundle is complete.
bool Release_Build(const Project* proj, BuildLog* log);
