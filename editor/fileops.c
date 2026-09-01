#include "fileops.h"
#include "scene_io.h"
#include "project_io.h"
#include "scene_build.h"
#include "pathutil.h"

#include <mge.h>
#include <mge_gui.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIRM_ID "Unsaved changes"
#define NAME_ID "Name"
#define PROJECT_EXT ".mgproject"

enum { PROMPT_SCENE = 0, PROMPT_SCRIPT = 1 };

// set each call so the static scene-load path can reset undo history
static History* s_hist;

static bool dirty(const Project* p, const Scene* s) { return s->dirty || p->dirty; }

static bool is_guarded(TopbarAction a)
{
    return a == TOPBAR_PROJECT_NEW || a == TOPBAR_PROJECT_OPEN ||
        a == TOPBAR_SCENE_SWITCH || a == TOPBAR_SCENE_REVERT || a == TOPBAR_QUIT;
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
    Scene_LoadSkybox(s, root);
    s->dirty = false;

    if (s_hist != NULL) { // a fresh scene -> no history to undo into
        History_Reset(s_hist);
        History_SetRoot(s_hist, root);
    }
}

static long file_bytes(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (f == NULL)
        return -1;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fclose(f);
    return n;
}

// Seed <root>/res/skybox/ with the engine's bundled 6-face skybox so a new
// project ships its own copy (the player loads it from the project pak).
static void copy_default_skybox(const char* root)
{
    static const char* faces[6] = { "right", "left", "top", "bottom", "front", "back" };

    // where the bundled skybox might be: the located SDK, then the cwd-staged copy
    char sdk[1024];
    char srcBase[3][1100];
    int nbases = 0;
    if (SceneBuild_FindSDK(sdk, sizeof(sdk)))
        snprintf(srcBase[nbases++], sizeof(srcBase[0]), "%s/assets/skybox", sdk);
    snprintf(srcBase[nbases++], sizeof(srcBase[0]), "assets/skybox");
    snprintf(srcBase[nbases++], sizeof(srcBase[0]), "../assets/skybox");

    char dstDir[600];
    Path_Join(root, "res/skybox", dstDir, sizeof(dstDir));
    Path_MakeDirs(dstDir);

    int copied = 0;
    for (int i = 0; i < 6; i++) {
        char leaf[16], dst[700];
        snprintf(leaf, sizeof(leaf), "%s.jpg", faces[i]);
        Path_Join(dstDir, leaf, dst, sizeof(dst));
        for (int b = 0; b < nbases; b++) {
            char src[1200];
            snprintf(src, sizeof(src), "%.1100s/%.15s", srcBase[b], leaf);
            if (file_bytes(src) > 0 && Path_CopyFile(src, dst) && file_bytes(dst) > 0) {
                copied++;
                break;
            }
        }
    }
    if (copied != 6)
        fprintf(stderr, "[editor] couldn't seed the default skybox (%d/6 faces). "
                        "Set one with Environment > choose folder.\n", copied);
}

// Drop a compile_flags.txt at the project root so a language server (clangd,
// ccls) resolves <mge.h> & co. while the user edits scene scripts. `make` stages
// the public headers as <editorDir>/include/, so that's what we point at (an old
// build without them falls back to the SDK's source/). Refreshed on Open / Save /
// New Project so a moved editor self-heals. Kept comment-free for older clangd /
// ccls; the editor owns this file, so custom flags go in a sibling `.clangd`.
static void seed_compile_flags(const char* root)
{
    char inc[1100] = { 0 };

    char exeDir[1024];
    Path_ExeDir(exeDir, sizeof(exeDir));
    if (exeDir[0] != '\0') {
        Path_Join(exeDir, "include", inc, sizeof(inc));
        if (!Path_IsDir(inc))
            inc[0] = '\0';
    }
    if (inc[0] == '\0') { // headers not staged next to the editor -> the SDK tree
        char sdk[1024];
        if (!SceneBuild_FindSDK(sdk, sizeof(sdk)))
            return;
        Path_Join(sdk, "source", inc, sizeof(inc));
    }

    char want[1400];
    snprintf(want, sizeof(want), "-std=c11\n-DPLATFORM_DESKTOP\n-I%s\n", inc);

    char path[600];
    Path_Join(root, "compile_flags.txt", path, sizeof(path));

    // already current? leave it -- don't churn the mtime (clangd would reindex)
    FILE* f = fopen(path, "rb");
    if (f != NULL) {
        char cur[1500];
        size_t n = fread(cur, 1, sizeof(cur) - 1, f);
        fclose(f);
        cur[n] = '\0';
        if (strcmp(cur, want) == 0)
            return;
    }

    f = fopen(path, "wb");
    if (f != NULL) {
        fputs(want, f);
        fclose(f);
    }
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
    copy_default_skybox(root); // <root>/res/skybox/*.jpg
    seed_compile_flags(root);  // <root>/compile_flags.txt for scene-script editing

    Scene_New(s);
    save_active_scene(p, s, cam->cam); // writes scenes/untitled/{scene.mgscene,untitled.c}
    load_active_scene(p, s, cam, 0);
}

static void do_open_project(Project* p, Scene* s, EditorCamera* cam)
{
    char* pick = Mge_OpenFileDialog("Open project", "MGEngine project", "*" PROJECT_EXT);
    if (pick == NULL)
        return;

    if (Project_Load(p, pick)) {
        char root[512];
        Project_Root(p, root, sizeof(root));
        seed_compile_flags(root); // keep scene-script header paths resolvable
        load_active_scene(p, s, cam, p->activeScene >= 0 ? p->activeScene : 0);
    } else {
        fprintf(stderr, "[editor] project load failed: %s\n", pick);
    }
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
    char root[512];
    Project_Root(p, root, sizeof(root));
    seed_compile_flags(root);
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

// Scaffold `<activeSceneDir>/<name>.c` -- an extra source file that compiles into
// the scene's module alongside its main `.c`.
static void do_new_script(Project* p, const char* name)
{
    if (p->path[0] == '\0' || p->activeScene < 0) {
        fprintf(stderr, "[editor] save the project first\n");
        return;
    }
    char dir[600], file[700];
    Project_SceneDir(p, p->scenes[p->activeScene], dir, sizeof(dir));
    char leaf[80];
    snprintf(leaf, sizeof(leaf), "%s.c", name);
    Path_Join(dir, leaf, file, sizeof(file));

    FILE* f = fopen(file, "rb");
    if (f != NULL) {
        fclose(f);
        fprintf(stderr, "[editor] %s already exists\n", file);
        return;
    }
    f = fopen(file, "wb");
    if (f == NULL) {
        fprintf(stderr, "[editor] can't write %s\n", file);
        return;
    }
    fprintf(f,
        "// %s -- part of the \"%s\" scene module. Add helpers here; the editor\n"
        "// recompiles the whole folder and hot-reloads on save.\n"
        "#include <mge.h>\n",
        leaf, p->scenes[p->activeScene]);
    fclose(f);
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
    case TOPBAR_SCENE_REVERT:
        if (p->path[0] != '\0' && p->activeScene >= 0)
            load_active_scene(p, s, cam, p->activeScene); // reload from disk
        break;
    case TOPBAR_SCENE_NEW:
        ops->namePrompt = true;
        ops->promptKind = PROMPT_SCENE;
        ops->nameBuf[0] = '\0';
        Mge_GuiOpenPopup(NAME_ID);
        break;
    case TOPBAR_SCENE_NEWSCRIPT:
        ops->namePrompt = true;
        ops->promptKind = PROMPT_SCRIPT;
        ops->nameBuf[0] = '\0';
        Mge_GuiOpenPopup(NAME_ID);
        break;
    case TOPBAR_QUIT:
        ops->quit = true;
        break;
    default:
        break;
    }
}

void FileOps_Request(FileOps* ops, TopbarResult r, Project* proj, Scene* s, EditorCamera* cam, History* h)
{
    s_hist = h;
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

void FileOps_Draw(FileOps* ops, Project* proj, Scene* s, EditorCamera* cam, History* h)
{
    s_hist = h;
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

    // --- name entry (new scene / new script) ---
    if (Mge_GuiBeginPopup(NAME_ID)) {
        bool script = ops->promptKind == PROMPT_SCRIPT;
        Mge_GuiLabel(script ? "Script file name (no extension):" : "Scene name (letters, digits, _ - ):");
        Mge_GuiSetNextItemWidth(220.0f);
        Mge_GuiInputText("##name", ops->nameBuf, (int)sizeof(ops->nameBuf));

        bool nameOk = Project_ValidSceneName(ops->nameBuf); // same charset rules
        bool unique = script ? true : (Project_FindScene(proj, ops->nameBuf) < 0);
        bool valid = nameOk && unique;
        if (ops->nameBuf[0] && !valid)
            Mge_GuiLabel(!nameOk ? "(bad or reserved name)" : "(already a scene)");

        Mge_GuiSpacing();
        if (valid && Mge_GuiButton("Create")) {
            char name[64];
            snprintf(name, sizeof(name), "%s", ops->nameBuf);
            ops->namePrompt = false;
            Mge_GuiClosePopup();
            if (script)
                do_new_script(proj, name);
            else
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
