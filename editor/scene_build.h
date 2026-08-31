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
// This form BLOCKS until the compiler exits -- used by Build Release, which
// compiles many scenes in a row. For the interactive Build / Play buttons use
// the async SceneBuildJob below so the editor keeps drawing.
bool SceneBuild_Compile(const Project* proj, const char* sceneName, bool release,
    BuildLog* log, char* outDll, int outDllSize);

// --- async compile: spawn the compiler as a separate process and poll it -------

typedef struct SceneBuildJob {
    bool  finished;       // child has exited (result is ready)
    bool  ok;             // valid once `finished`
    void* proc;           // OS process handle (Windows HANDLE box / POSIX pid box)
    char  logFile[768];   // temp file the child's stdout+stderr stream to
    long  logCopied;      // bytes already forwarded into `log`
    BuildLog* log;        // where compiler output is streamed, live
    char  outDll[768];    // the .dll the command produces (valid once ok)
} SceneBuildJob;

// Start the compile as a detached child writing to a temp file. Returns false if
// setup failed (no SDK / no sources / spawn error), writing why into `log`.
bool SceneBuild_Start(SceneBuildJob* job, const Project* proj, const char* sceneName,
    bool release, BuildLog* log);

// Forward any new compiler output into the job's log. Returns true once the child
// has exited; then `job->ok` and `job->outDll` hold the result.
bool SceneBuild_Poll(SceneBuildJob* job);

// Reap the child (terminating it if still running) and delete the temp file.
void SceneBuild_Clear(SceneBuildJob* job);
