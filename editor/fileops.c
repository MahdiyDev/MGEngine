#include "fileops.h"
#include "scene_io.h"
#include "project_io.h"
#include "pathutil.h"

#include <mge.h>
#include <mge_gui.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIRM_ID "Unsaved changes"
#define NAME_ID "New scene"
#define PROJECT_EXT ".mgproject"

static bool dirty(const Project* p, const Scene* s) { return s->dirty || p->dirty; }

static bool is_guarded(TopbarAction a)
{
    return a == TOPBAR_PROJECT_NEW || a == TOPBAR_PROJECT_OPEN ||
        a == TOPBAR_SCENE_SWITCH || a == TOPBAR_QUIT;
}

// ---- scene <-> project.activeScene ----

static void save_active_scene(Project* p, Scene* s, Camera3D cam)
{
    if (p->path[0] == '\0' || p->activeScene < 0)
        return;
    char dir[512], file[512], root[512];
    Project_SceneDir(p, p->scenes[p->activeScene], dir, sizeof(dir));
    Project_SceneFile(p, p->scenes[p->activeScene], file, sizeof(file));
    Project_Root(p, root, sizeof(root));
    Path_MakeDirs(dir);
    if (!Scene_Save(s, file, cam, root))
        fprintf(stderr, "[editor] scene save failed: %s\n", file);
}

static void load_active_scene(Project* p, Scene* s, EditorCamera* cam, int index)
{
    p->activeScene = index;
    char file[512], root[512];
    Project_SceneFile(p, p->scenes[index], file, sizeof(file));
    Project_Root(p, root, sizeof(root));

    Camera3D c;
    if (file[0] != '\0' && Scene_Load(s, file, &c)) {
        Scene_LoadMaterialTextures(s, root);
        EditorCamera_SetPose(cam, c);
    } else {
        Scene_New(s); // listed but never saved yet -> a blank scene
        snprintf(s->path, sizeof(s->path), "%s", file);
    }
    s->dirty = false;
}

// ---- the actions ----

static void do_new_project(Project* p, Scene* s, EditorCamera* cam)
{
    char* pick = Mge_SaveFileDialog("New project (pick a location + name)",
        "MGEngine project", "*" PROJECT_EXT, "myproject" PROJECT_EXT);
    if (pick == NULL)
        return;

    // the project gets its own folder: <picked dir>/<picked stem>/
    char parent[400], stem[96];
    Path_Dir(pick, parent, sizeof(parent));
    Path_Base(pick, stem, sizeof(stem));
    Path_StripExt(stem);
    free(pick);
    if (stem[0] == '\0')
        snprintf(stem, sizeof(stem), "myproject");

    char root[400];
    if (parent[0] != '\0')
        Path_Join(parent, stem, root, sizeof(root));
    else
        snprintf(root, sizeof(root), "%s", stem);
    Path_MakeDirs(root);

    char path[420];
    Path_Join(root, "project" PROJECT_EXT, path, sizeof(path));

    Project_Default(p);
    if (!Project_Save(p, path)) {
        fprintf(stderr, "[editor] project save failed: %s\n", path);
        return;
    }
    char resDir[512];
    Project_ResDir(p, resDir, sizeof(resDir));
    Path_MakeDirs(resDir); // the project's single shared resource root

    Scene_New(s);
    save_active_scene(p, s, cam->cam); // writes scenes/untitled/{scene.mgscene,untitled.c}
    load_active_scene(p, s, cam, 0);
}

static void do_open_project(Project* p, Scene* s, EditorCamera* cam)
{
    char* pick = Mge_OpenFileDialog("Open project", "MGEngine project", "*" PROJECT_EXT);
    if (pick == NULL)
        return;

    if (Project_Load(p, pick))
        load_active_scene(p, s, cam, p->activeScene >= 0 ? p->activeScene : 0);
    else
        fprintf(stderr, "[editor] project load failed: %s\n", pick);
    free(pick);
}

static void do_save_project(Project* p, Scene* s, EditorCamera* cam)
{
    if (p->path[0] == '\0') {
        do_new_project(p, s, cam);
        return;
    }
    save_active_scene(p, s, cam->cam);
    if (!Project_Save(p, p->path))
        fprintf(stderr, "[editor] project save failed: %s\n", p->path);
}

static void do_add_scene(Project* p, Scene* s, EditorCamera* cam)
{
    char myRoot[512];
    Project_Root(p, myRoot, sizeof(myRoot));

    char* pick = Mge_OpenFileDialog("Add scene: pick scenes/<name>/scene.mgscene inside this project",
        "MGEngine scene", "*.mgscene");
    if (pick == NULL)
        return;

    // require exactly <root>/scenes/<name>/scene.mgscene
    char file[64], sceneDir[512], name[64], scenesDir[512], scenesName[64], root[512];
    Path_Base(pick, file, sizeof(file));
    Path_Dir(pick, sceneDir, sizeof(sceneDir));
    Path_Base(sceneDir, name, sizeof(name));
    Path_Dir(sceneDir, scenesDir, sizeof(scenesDir));
    Path_Base(scenesDir, scenesName, sizeof(scenesName));
    Path_Dir(scenesDir, root, sizeof(root));
    free(pick);

    if (strcmp(file, "scene.mgscene") != 0 || strcmp(scenesName, "scenes") != 0 ||
        !Path_Equal(root, myRoot)) {
        fprintf(stderr, "[editor] Add Scene: that isn't a 'scenes/<name>/scene.mgscene' inside\n"
                        "         this project (%s). Use New Scene to create one.\n", myRoot);
        return;
    }
    if (Project_FindScene(p, name) >= 0) {
        fprintf(stderr, "[editor] scene '%s' is already in the project\n", name);
        return;
    }
    if (!Project_AddScene(p, name)) {
        fprintf(stderr, "[editor] can't add scene '%s' (name reserved / project full)\n", name);
        return;
    }
    Project_Save(p, p->path);
    load_active_scene(p, s, cam, p->sceneCount - 1);
}

static void do_new_scene(Project* p, Scene* s, EditorCamera* cam, const char* name)
{
    if (!Project_AddScene(p, name)) {
        fprintf(stderr, "[editor] can't create scene '%s'\n", name);
        return;
    }
    Scene_New(s);
    p->activeScene = p->sceneCount - 1;
    save_active_scene(p, s, cam->cam);
    load_active_scene(p, s, cam, p->activeScene);
    Project_Save(p, p->path);
}

// Run an action whose guard (if any) has already been cleared.
static void run(FileOps* ops, TopbarAction a, int arg, Project* p, Scene* s, EditorCamera* cam)
{
    switch (a) {
    case TOPBAR_PROJECT_NEW:  do_new_project(p, s, cam); break;
    case TOPBAR_PROJECT_OPEN: do_open_project(p, s, cam); break;
    case TOPBAR_PROJECT_SAVE: do_save_project(p, s, cam); break;
    case TOPBAR_SCENE_ADD:    do_add_scene(p, s, cam); break;
    case TOPBAR_SCENE_SAVE:
        if (p->path[0] != '\0')
            save_active_scene(p, s, cam->cam);
        else
            do_save_project(p, s, cam); // no project on disk yet -> prompt for a location
        break;
    case TOPBAR_SCENE_SWITCH:
        if (arg >= 0 && arg < p->sceneCount && arg != p->activeScene)
            load_active_scene(p, s, cam, arg);
        break;
    case TOPBAR_SCENE_NEW:
        ops->namePrompt = true;
        ops->nameBuf[0] = '\0';
        Mge_GuiOpenPopup(NAME_ID);
        break;
    case TOPBAR_BUILD:
        printf("[editor] Build: whole-project compile + hot reload lands in Phase 4\n");
        break;
    case TOPBAR_QUIT:
        ops->quit = true;
        break;
    default:
        break;
    }
}

void FileOps_Request(FileOps* ops, TopbarResult r, Project* proj, Scene* s, EditorCamera* cam)
{
    if (r.action == TOPBAR_NONE)
        return;

    if (is_guarded(r.action) && dirty(proj, s)) {
        ops->pending = r.action;
        ops->pendingArg = r.arg;
        Mge_GuiOpenPopup(CONFIRM_ID);
        return;
    }
    run(ops, r.action, r.arg, proj, s, cam);
}

void FileOps_Draw(FileOps* ops, Project* proj, Scene* s, EditorCamera* cam)
{
    // --- unsaved-changes confirm ---
    if (Mge_GuiBeginPopup(CONFIRM_ID)) {
        Mge_GuiLabel("The project or scene has unsaved changes.");
        Mge_GuiSpacing();

        if (Mge_GuiButton("Save")) {
            do_save_project(proj, s, cam);
            if (!dirty(proj, s)) {
                TopbarAction a = ops->pending;
                int arg = ops->pendingArg;
                ops->pending = TOPBAR_NONE;
                Mge_GuiClosePopup();
                run(ops, a, arg, proj, s, cam);
            }
        }
        Mge_GuiSameLine();
        if (Mge_GuiButton("Discard")) {
            s->dirty = false;
            proj->dirty = false;
            TopbarAction a = ops->pending;
            int arg = ops->pendingArg;
            ops->pending = TOPBAR_NONE;
            Mge_GuiClosePopup();
            run(ops, a, arg, proj, s, cam);
        }
        Mge_GuiSameLine();
        if (Mge_GuiButton("Cancel")) {
            ops->pending = TOPBAR_NONE;
            Mge_GuiClosePopup();
        }
        Mge_GuiEndPopup();
    }

    // --- new-scene name entry ---
    if (Mge_GuiBeginPopup(NAME_ID)) {
        Mge_GuiLabel("Scene name (letters, digits, _ - ):");
        Mge_GuiSetNextItemWidth(220.0f);
        Mge_GuiInputText("##scenename", ops->nameBuf, (int)sizeof(ops->nameBuf));

        bool nameOk = Project_ValidSceneName(ops->nameBuf);
        bool unique = Project_FindScene(proj, ops->nameBuf) < 0;
        bool valid = nameOk && unique;
        if (ops->nameBuf[0] && !valid)
            Mge_GuiLabel(!nameOk ? "(bad or reserved name)" : "(already a scene)");

        Mge_GuiSpacing();
        if (valid && Mge_GuiButton("Create")) {
            char name[64];
            snprintf(name, sizeof(name), "%s", ops->nameBuf);
            ops->namePrompt = false;
            Mge_GuiClosePopup();
            do_new_scene(proj, s, cam, name);
        }
        Mge_GuiSameLine();
        if (Mge_GuiButton("Cancel")) {
            ops->namePrompt = false;
            Mge_GuiClosePopup();
        }
        Mge_GuiEndPopup();
    }
}
