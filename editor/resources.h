// The editor's bottom panel: a browser for the project's shared `res/` folder --
// tree view, import / new folder / rename / delete, image thumbnails, and
// one-click assign to the selected object's material slots.
#pragma once

#include <mge.h>
#include "scene.h"
#include "project.h"

#define RES_PATH_LEN 512
#define RES_MAX_THUMBS 48

typedef struct ResThumb {
    char rel[RES_PATH_LEN]; // project-root-relative ("res/...")
    Texture2D tex;
} ResThumb;

typedef struct Resources {
    char sel[RES_PATH_LEN]; // selected entry, root-relative ("" = none)
    bool selIsDir;

    ResThumb thumbs[RES_MAX_THUMBS];
    int thumbCount;

    int modal; // 0 none, 1 rename, 2 new folder, 3 delete
    char nameBuf[128];

    char projectPath[RES_PATH_LEN]; // last project seen -> flush thumbs on change
} Resources;

void Resources_Init(Resources* r);
void Resources_Shutdown(Resources* r);
void Resources_Draw(Resources* r, Rectangle rect, Project* proj, Scene* s, int fps, int draws);
