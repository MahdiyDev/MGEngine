#include "project_io.h"
#include "pathutil.h"

#include <mge.h> // Mge_LoadFileText -- so a mounted pak resolves the path

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// text between the first and last '"' of `line` -> `out`
static bool quoted(const char* line, char* out, size_t outSize)
{
    const char* a = strchr(line, '"');
    const char* b = (a != NULL) ? strrchr(line, '"') : NULL;
    if (a == NULL || b == NULL || b <= a)
        return false;
    size_t n = (size_t)(b - a - 1);
    if (n >= outSize)
        n = outSize - 1;
    memcpy(out, a + 1, n);
    out[n] = '\0';
    return true;
}

// project name = the folder that holds project.mgproject, else the file stem
static void derive_name(const char* path, char* out, size_t outSize)
{
    char dir[512];
    Path_Dir(path, dir, sizeof(dir));
    if (dir[0] != '\0') {
        Path_Base(dir, out, outSize);
    } else {
        Path_Base(path, out, outSize);
        Path_StripExt(out);
    }
}

bool Project_Save(Project* p, const char* path)
{
    FILE* f = fopen(path, "wb");
    if (f == NULL)
        return false;

    derive_name(path, p->name, sizeof(p->name));

    fprintf(f, "mgeproject 1\n");
    fprintf(f, "name \"%s\"\n\n", p->name);

    fprintf(f, "settings\n");
    fprintf(f, "  window %d %d\n", p->windowW, p->windowH);
    fprintf(f, "  targetFps %d\n", p->targetFps);
    fprintf(f, "  msaa %d\n", p->msaa);
    fprintf(f, "  output \"%s\"\n", p->output);
    fprintf(f, "  cflagsDebug \"%s\"\n", p->cflagsDebug);
    fprintf(f, "  cflagsRelease \"%s\"\n", p->cflagsRelease);
    fprintf(f, "  startupScene \"%s\"\n\n", p->startupScene);

    for (int i = 0; i < p->sceneCount; i++)
        fprintf(f, "scene \"%s\"\n", p->scenes[i]);

    fclose(f);

    snprintf(p->path, sizeof(p->path), "%s", path);
    p->dirty = false;
    return true;
}

bool Project_Load(Project* p, const char* path)
{
    char* text = Mge_LoadFileText(path); // loose file, then any mounted pak
    if (text == NULL)
        return false;
    char* cur = text;

    char line[1024];
    if (!Path_NextLine(&cur, line, sizeof(line)) || strncmp(line, "mgeproject", 10) != 0) {
        Mge_UnLoadFileText(text);
        return false;
    }

    Project_Default(p);
    p->sceneCount = 0;
    p->activeScene = -1;

    int sec = 0; // 0 top, 1 settings

    while (Path_NextLine(&cur, line, sizeof(line))) {
        char* hash = strchr(line, '#');
        if (hash != NULL)
            *hash = '\0';

        char* a = line;
        while (*a == ' ' || *a == '\t' || *a == '\r' || *a == '\n')
            a++;
        size_t len = strlen(a);
        while (len > 0 && (a[len - 1] == ' ' || a[len - 1] == '\t' || a[len - 1] == '\r' || a[len - 1] == '\n'))
            a[--len] = '\0';
        if (a[0] == '\0')
            continue;

        char key[64] = { 0 };
        sscanf(a, "%63s", key);
        const char* rest = a + strlen(key);
        while (*rest == ' ' || *rest == '\t')
            rest++;

        if (strcmp(key, "name") == 0 && sec == 0) {
            char nm[64];
            if (quoted(a, nm, sizeof(nm)))
                snprintf(p->name, sizeof(p->name), "%s", nm);
            continue;
        }
        if (strcmp(key, "settings") == 0) { sec = 1; continue; }
        if (strcmp(key, "scene") == 0) {
            char nm[64];
            if (quoted(a, nm, sizeof(nm)) && p->sceneCount < PROJECT_MAX_SCENES)
                snprintf(p->scenes[p->sceneCount++], sizeof(p->scenes[0]), "%s", nm);
            continue;
        }

        if (sec == 1) {
            int x, y;
            if (strcmp(key, "window") == 0 && sscanf(rest, "%d %d", &x, &y) == 2) {
                p->windowW = x;
                p->windowH = y;
            } else if (strcmp(key, "targetFps") == 0) {
                p->targetFps = atoi(rest);
            } else if (strcmp(key, "msaa") == 0) {
                p->msaa = atoi(rest);
            } else if (strcmp(key, "output") == 0) {
                quoted(a, p->output, sizeof(p->output));
            } else if (strcmp(key, "cflagsDebug") == 0) {
                quoted(a, p->cflagsDebug, sizeof(p->cflagsDebug));
            } else if (strcmp(key, "cflagsRelease") == 0) {
                quoted(a, p->cflagsRelease, sizeof(p->cflagsRelease));
            } else if (strcmp(key, "startupScene") == 0) {
                quoted(a, p->startupScene, sizeof(p->startupScene));
            }
        }
    }

    Mge_UnLoadFileText(text);

    if (p->sceneCount == 0) {
        strcpy(p->scenes[0], "untitled");
        p->sceneCount = 1;
    }
    if (Project_FindScene(p, p->startupScene) < 0)
        snprintf(p->startupScene, sizeof(p->startupScene), "%s", p->scenes[0]);

    int start = Project_FindScene(p, p->startupScene);
    p->activeScene = (start >= 0) ? start : 0;

    snprintf(p->path, sizeof(p->path), "%s", path);
    char dir[512];
    Path_Dir(path, dir, sizeof(dir));
    if (dir[0] != '\0') // folder name is the project identity; else keep the parsed `name`
        Path_Base(dir, p->name, sizeof(p->name));
    p->dirty = false;
    return true;
}
