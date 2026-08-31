#include "sceneops.h"
#include "scene_io.h"
#include "pathutil.h"

#include <mge.h>
#include <mge_gui.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIRM_ID "Unsaved changes"

static bool is_guarded(TopbarAction a)
{
    return a == TOPBAR_NEW || a == TOPBAR_OPEN || a == TOPBAR_QUIT;
}

static void do_save_as(Scene* s, EditorCamera* cam)
{
    char def[128];
    snprintf(def, sizeof(def), "%s.mge", s->name[0] ? s->name : "scene");

    char* p = Mge_SaveFileDialog("Save scene as", "MGEngine scene", "*.mge", def);
    if (p == NULL)
        return;

    char path[600];
    size_t n = strlen(p);
    if (n < 4 || strcmp(p + n - 4, ".mge") != 0)
        snprintf(path, sizeof(path), "%s.mge", p);
    else
        snprintf(path, sizeof(path), "%s", p);
    free(p);

    if (!Scene_Save(s, path, cam->cam))
        fprintf(stderr, "[editor] save failed: %s\n", path);
}

static void do_open(Scene* s, EditorCamera* cam)
{
    char* p = Mge_OpenFileDialog("Open scene", "MGEngine scene", "*.mge");
    if (p == NULL)
        return;

    Camera3D c;
    if (Scene_Load(s, p, &c)) {
        Scene_LoadMaterialTextures(s);
        EditorCamera_SetPose(cam, c);
    } else {
        fprintf(stderr, "[editor] load failed: %s\n", p);
    }
    free(p);
}

// Run an action unconditionally (the guard has already been cleared).
static void run(SceneOps* ops, TopbarAction a, Scene* s, EditorCamera* cam)
{
    switch (a) {
    case TOPBAR_NEW:
        Scene_New(s);
        break;
    case TOPBAR_OPEN:
        do_open(s, cam);
        break;
    case TOPBAR_SAVE:
        if (s->path[0] != '\0') {
            if (!Scene_Save(s, s->path, cam->cam))
                fprintf(stderr, "[editor] save failed: %s\n", s->path);
        } else {
            do_save_as(s, cam);
        }
        break;
    case TOPBAR_SAVE_AS:
        do_save_as(s, cam);
        break;
    case TOPBAR_BUILD:
        printf("[editor] Build: scene-as-code + hot reload lands in Phase 3\n");
        break;
    case TOPBAR_QUIT:
        ops->quit = true;
        break;
    default:
        break;
    }
}

void SceneOps_Request(SceneOps* ops, TopbarAction act, Scene* s, EditorCamera* cam)
{
    if (act == TOPBAR_NONE)
        return;

    if (is_guarded(act) && s->dirty) {
        ops->pending = act;
        Mge_GuiOpenPopup(CONFIRM_ID);
        return;
    }
    run(ops, act, s, cam);
}

void SceneOps_Draw(SceneOps* ops, Scene* s, EditorCamera* cam)
{
    if (!Mge_GuiBeginPopup(CONFIRM_ID))
        return;

    Mge_GuiLabel("This scene has unsaved changes.");
    Mge_GuiSpacing();

    if (Mge_GuiButton("Save")) {
        run(ops, TOPBAR_SAVE, s, cam);
        if (!s->dirty) { // save succeeded
            TopbarAction p = ops->pending;
            ops->pending = TOPBAR_NONE;
            Mge_GuiClosePopup();
            run(ops, p, s, cam);
        }
    }
    Mge_GuiSameLine();
    if (Mge_GuiButton("Discard")) {
        s->dirty = false;
        TopbarAction p = ops->pending;
        ops->pending = TOPBAR_NONE;
        Mge_GuiClosePopup();
        run(ops, p, s, cam);
    }
    Mge_GuiSameLine();
    if (Mge_GuiButton("Cancel")) {
        ops->pending = TOPBAR_NONE;
        Mge_GuiClosePopup();
    }

    Mge_GuiEndPopup();
}
