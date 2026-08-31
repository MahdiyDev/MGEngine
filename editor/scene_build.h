// Compiling a scene's `*.c` into a hot-reloadable shared library.
#pragma once

#include <stdbool.h>
#include "project.h"

// A growable text buffer for compiler output, shown in the editor console.
typedef struct BuildLog {
    char* text;
    int len, cap;
} BuildLog;

void BuildLog_Reset(BuildLog* b);           // clear (keeps the allocation)
void BuildLog_Free(BuildLog* b);
void BuildLog_Append(BuildLog* b, const char* s);
void BuildLog_Line(BuildLog* b, const char* fmt, ...);

// Locate the engine SDK (a directory with `source/mge.h` + `build/libmgengine`):
// env `MGE_ENGINE` first, then a search upward from the working directory.
// Returns true and fills `out` on success.
bool SceneBuild_FindSDK(char* out, int outSize);

// Compile every `*.c` in `<projectRoot>/scenes/<sceneName>/` into
// `<sceneDir>/build/<sceneName>_(debug|release).dll`, linking `libmgengine` with
// the project's cflags. The command and all compiler output go into `log`; on
// success `outDll` gets the produced library's path. Returns true on success.
bool SceneBuild_Compile(const Project* proj, const char* sceneName, bool release,
    BuildLog* log, char* outDll, int outDllSize);
