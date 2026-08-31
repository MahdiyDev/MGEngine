// project.mgproject (de)serialisation -- a flat, line-based, diffable text
// format (same style as .mgscene). Data only, no GL.
#pragma once

#include "project.h"

// Write `p` to `path`. On success sets `p->path` / `p->name` (from the file
// stem's directory) and clears `p->dirty`. Returns true on success.
bool Project_Save(Project* p, const char* path);

// Parse `path` into `p`. Sets `p->path` / `p->name`, clears `p->dirty`,
// `activeScene` = the startup scene (or 0). Returns true on success.
bool Project_Load(Project* p, const char* path);
