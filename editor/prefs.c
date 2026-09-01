#include "prefs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PREFS_DEFAULT_W   1280
#define PREFS_DEFAULT_H   720
#define PREFS_LEFT        240.0f
#define PREFS_RIGHT       320.0f
#define PREFS_BOTTOM      172.0f

static void prefs_path(char* out, size_t n)
{
    const char* home = getenv("USERPROFILE");
    if (home == NULL || home[0] == '\0')
        home = getenv("HOME");
    if (home != NULL && home[0] != '\0')
        snprintf(out, n, "%s/.mgeeditor.ini", home);
    else
        snprintf(out, n, ".mgeeditor.ini");
}

void Prefs_Load(EditorPrefs* p)
{
    p->winW = PREFS_DEFAULT_W;
    p->winH = PREFS_DEFAULT_H;
    p->leftW = PREFS_LEFT;
    p->rightW = PREFS_RIGHT;
    p->bottomH = PREFS_BOTTOM;
    p->buildRelease = 0;

    char path[1024];
    prefs_path(path, sizeof(path));
    FILE* f = fopen(path, "rb");
    if (f == NULL)
        return;

    char line[256];
    while (fgets(line, sizeof(line), f) != NULL) {
        char key[64];
        float v;
        if (sscanf(line, " %63[^= ] = %f", key, &v) != 2)
            continue;
        if (strcmp(key, "winW") == 0) p->winW = (int)v;
        else if (strcmp(key, "winH") == 0) p->winH = (int)v;
        else if (strcmp(key, "leftW") == 0) p->leftW = v;
        else if (strcmp(key, "rightW") == 0) p->rightW = v;
        else if (strcmp(key, "bottomH") == 0) p->bottomH = v;
        else if (strcmp(key, "buildRelease") == 0) p->buildRelease = (v != 0.0f);
    }
    fclose(f);

    // clamp to something sane in case the file was hand-edited
    if (p->winW < 640) p->winW = 640;
    if (p->winH < 400) p->winH = 400;
    if (p->leftW < 140.0f) p->leftW = 140.0f;
    if (p->rightW < 160.0f) p->rightW = 160.0f;
    if (p->bottomH < 60.0f) p->bottomH = 60.0f;
}

void Prefs_Save(const EditorPrefs* p)
{
    char path[1024];
    prefs_path(path, sizeof(path));
    FILE* f = fopen(path, "wb");
    if (f == NULL)
        return;
    fprintf(f,
        "winW = %d\n"
        "winH = %d\n"
        "leftW = %.0f\n"
        "rightW = %.0f\n"
        "bottomH = %.0f\n"
        "buildRelease = %d\n",
        p->winW, p->winH, (double)p->leftW, (double)p->rightW, (double)p->bottomH,
        p->buildRelease);
    fclose(f);
}
