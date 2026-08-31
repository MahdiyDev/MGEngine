#include "project.h"
#include "pathutil.h"

#include <stdio.h>
#include <string.h>

void Project_Default(Project* p)
{
    memset(p, 0, sizeof(*p));
    strcpy(p->name, "untitled");
    p->windowW = 1280;
    p->windowH = 720;
    p->targetFps = 60;
    p->msaa = 4;
    strcpy(p->output, "game");
    strcpy(p->cflagsDebug, "-O0 -g");
    strcpy(p->cflagsRelease, "-O2 -DNDEBUG -s");
    strcpy(p->startupScene, "untitled");
    strcpy(p->scenes[0], "untitled");
    p->sceneCount = 1;
    p->activeScene = 0;
}

int Project_FindScene(const Project* p, const char* name)
{
    for (int i = 0; i < p->sceneCount; i++)
        if (strcmp(p->scenes[i], name) == 0)
            return i;
    return -1;
}

bool Project_ValidSceneName(const char* name)
{
    if (name == NULL || name[0] == '\0' || strlen(name) >= 64)
        return false;
    for (const char* c = name; *c != '\0'; c++) {
        bool ok = (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') ||
            (*c >= '0' && *c <= '9') || *c == '_' || *c == '-';
        if (!ok)
            return false;
    }
    // names that collide with the project layout
    static const char* reserved[] = { "build", "res", "scenes", "obj", "bin" };
    for (size_t i = 0; i < sizeof(reserved) / sizeof(reserved[0]); i++)
        if (strcmp(name, reserved[i]) == 0)
            return false;
    return true;
}

bool Project_AddScene(Project* p, const char* name)
{
    if (p->sceneCount >= PROJECT_MAX_SCENES)
        return false;
    if (!Project_ValidSceneName(name))
        return false;
    if (Project_FindScene(p, name) >= 0)
        return false;

    snprintf(p->scenes[p->sceneCount], sizeof(p->scenes[0]), "%s", name);
    p->sceneCount++;
    p->dirty = true;
    return true;
}

void Project_RemoveScene(Project* p, int index)
{
    if (index < 0 || index >= p->sceneCount || p->sceneCount <= 1)
        return; // keep at least one scene

    for (int i = index; i < p->sceneCount - 1; i++)
        memcpy(p->scenes[i], p->scenes[i + 1], sizeof(p->scenes[0]));
    p->sceneCount--;

    if (p->activeScene == index)
        p->activeScene = (index < p->sceneCount) ? index : p->sceneCount - 1;
    else if (p->activeScene > index)
        p->activeScene--;

    if (Project_FindScene(p, p->startupScene) < 0)
        snprintf(p->startupScene, sizeof(p->startupScene), "%s", p->scenes[0]);

    p->dirty = true;
}

void Project_Root(const Project* p, char* out, size_t outSize)
{
    if (p->path[0] == '\0')
        out[0] = '\0';
    else
        Path_Dir(p->path, out, outSize);
}

void Project_ResDir(const Project* p, char* out, size_t outSize)
{
    char root[512];
    Project_Root(p, root, sizeof(root));
    if (root[0] == '\0')
        out[0] = '\0';
    else
        Path_Join(root, "res", out, outSize);
}

void Project_SceneDir(const Project* p, const char* sceneName, char* out, size_t outSize)
{
    char root[512];
    Project_Root(p, root, sizeof(root));
    if (root[0] == '\0') {
        out[0] = '\0';
        return;
    }
    char scenes[600];
    Path_Join(root, "scenes", scenes, sizeof(scenes));
    Path_Join(scenes, sceneName, out, outSize);
}

void Project_SceneFile(const Project* p, const char* sceneName, char* out, size_t outSize)
{
    char dir[600];
    Project_SceneDir(p, sceneName, dir, sizeof(dir));
    if (dir[0] == '\0') {
        out[0] = '\0';
        return;
    }
    Path_Join(dir, "scene.mgscene", out, outSize);
}
