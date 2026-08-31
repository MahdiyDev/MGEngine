// The editor's Project: the top-level document. Holds global config (window /
// build settings) and the list of scenes. Each scene is a subdirectory
// `<root>/scenes/<name>/` with its own `scene.mgscene`, `res/` and `*.c`.
#pragma once

#include <stdbool.h>
#include <stddef.h>

#define PROJECT_MAX_SCENES 32

typedef struct Project {
    char name[64];   // = the project directory / file stem
    char path[512];  // absolute path to `project.mgproject` ("" = in-memory default)
    bool dirty;      // unsaved structural / settings edits

    // build + runtime settings (consumed in Phase 4)
    int windowW, windowH;
    int targetFps;
    int msaa;                  // 0 = off, else sample count
    char output[64];           // built app's base name
    char cflagsDebug[256];
    char cflagsRelease[256];
    char startupScene[64];     // which scene the built app / editor opens first

    char scenes[PROJECT_MAX_SCENES][64]; // subdirectory names under scenes/
    int sceneCount;
    int activeScene;           // index into scenes[], or -1
} Project;

// A blank in-memory project with one scene, "untitled". No files on disk.
void Project_Default(Project* p);

int  Project_FindScene(const Project* p, const char* name); // index, or -1
bool Project_AddScene(Project* p, const char* name);        // false if full / duplicate / bad name
void Project_RemoveScene(Project* p, int index);

// Path helpers (empty `out` when the project is in-memory / has no path):
//   root      = the directory holding project.mgproject
//   resDir    = <root>/res     (one shared resource root for the whole project)
//   sceneDir  = <root>/scenes/<name>
//   sceneFile = <root>/scenes/<name>/scene.mgscene
void Project_Root(const Project* p, char* out, size_t outSize);
void Project_ResDir(const Project* p, char* out, size_t outSize);
void Project_SceneDir(const Project* p, const char* sceneName, char* out, size_t outSize);
void Project_SceneFile(const Project* p, const char* sceneName, char* out, size_t outSize);

// true when `name` is a usable scene folder name (letters/digits/_/-, non-empty).
bool Project_ValidSceneName(const char* name);
