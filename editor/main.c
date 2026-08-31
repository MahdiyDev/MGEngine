// MGEngine editor -- a scene editor built on the engine library.
//
//   TAB / top bar        VIEW mode (fly-camera, cursor locked)  <->  EDIT mode
//   fly-camera           WASD move, mouse look
//   edit mode
//     hold RIGHT mouse   look around; WASD flies while held
//     left-click         select an object / a light
//     drag a gizmo       move / rotate / scale it (switch mode in the top bar)
//   Ctrl+S (EDIT mode)   save the active scene       F12   screenshot
//   Play / Stop          build + run the scene's code (hot-reloads on .c change)
//   panels
//     top bar            Project + Scene menus, Play/Build/Console, mode, gizmo, Render
//     left (Hierarchy)   objects + lights; add / rename / toggle / delete
//     right (Inspector)  the selection's fields
//     bottom             project resource explorer (Phase 5 stub), or the build console
//
// This file: the window, the frame loop, the docked-panel layout, and the one
// Mge_Gui frame the panels share. The Project + active Scene are owned here.
// Scene data + rendering live in scene.c; the project in project.c; the camera
// in editor_camera.c; File actions in fileops.c; play/build in play.c; each
// panel in its own <name>.c.
#include <mge.h>
#include <mge_gui.h>

#include "scene.h"
#include "project.h"
#include "editor_camera.h"
#include "topbar.h"
#include "hierarchy.h"
#include "inspector.h"
#include "resources.h"
#include "fileops.h"
#include "play.h"

static const int width = 1280, height = 720;

// docked-shell metrics (px)
enum {
    TOPBAR_H = 46,
    LEFT_W = 240,
    RIGHT_W = 320,
    BOTTOM_H = 172,
};

int main(void)
{
    Mge_SetMSAA(4); // 4x anti-aliasing on every shape / object / model
    Mge_InitWindow(width, height, "MGEngine editor");
    Mge_SetTargetFPS(60);

    bool editMode = true; // start in EDIT mode (cursor free, panels clickable)
    EnableCursor();

    EditorCamera camera;
    EditorCamera_Init(&camera);

    Project project;
    Project_Default(&project); // in-memory: one scene "untitled", no files yet

    Scene scene;
    Scene_Init(&scene, width, height); // GPU resources + the default "untitled" scene data

    FileOps ops = { 0 };
    Play play;
    Play_Init(&play);
    Resources res;
    Resources_Init(&res);

    int fpsShown = 0, drawsShown = 0;
    double fpsAt = 0.0;
    bool prevEditMode = editMode;

    for (;;) {
        bool guiKeyboard = Mge_GuiWantsKeyboard();
        bool guiMouse = Mge_GuiWantsMouse();

        if (Mge_GetTime() - fpsAt >= 1.0) {
            fpsShown = Mge_GetFps();
            drawsShown = Mge_GetDrawCalls();
            fpsAt = Mge_GetTime();
        }

        if (IsKeyPressed(KEY_TAB) && !guiKeyboard)
            editMode = !editMode;

        // sync the OS cursor whenever the mode changes (keyboard or top bar)
        if (editMode != prevEditMode) {
            if (editMode)
                EnableCursor();
            else
                DisableCursor();
            prevEditMode = editMode;
        }

        EditorCamera_Update(&camera, editMode, guiMouse);

        // while playing, run the scene module + hot-reload; editing is paused
        Play_Frame(&play, &project, &scene, &camera);

        bool looking = EditorCamera_IsLooking(&camera);
        bool interact = editMode && !looking && !guiMouse && !play.playing;

        Mge_BeginDrawing();

        bool gizmoBusy = Scene_Draw(&scene, camera.cam, interact, true);
        if (gizmoBusy)
            scene.dirty = true;
        if (interact && !gizmoBusy)
            Scene_Pick(&scene, camera.cam);

        // docked-panel layout, recomputed each frame (window can be resized)
        float W = (float)Mge_GetScreenWidth(), H = (float)Mge_GetScreenHeight();
        float midH = H - TOPBAR_H - BOTTOM_H;
        Rectangle rTop = { 0, 0, W, TOPBAR_H };
        Rectangle rLeft = { 0, TOPBAR_H, LEFT_W, midH };
        Rectangle rRight = { W - RIGHT_W, TOPBAR_H, RIGHT_W, midH };
        Rectangle rBottom = { 0, H - BOTTOM_H, W, BOTTOM_H };

        Mge_GuiBeginFrame();

        // close button / ESC -> route through the unsaved-changes guard
        if (Mge_WindowShouldClose()) {
            Mge_SetWindowShouldClose(false);
            FileOps_Request(&ops, (TopbarResult){ TOPBAR_QUIT, 0 }, &project, &scene, &camera);
        }

        TopbarResult tr = Topbar_Draw(rTop, &project, &scene, &editMode, play.playing, &play.showConsole);
        Hierarchy_Draw(rLeft, &scene);
        Inspector_Draw(rRight, &scene, &project);
        if (play.showConsole)
            Play_DrawConsole(&play, rBottom);
        else
            Resources_Draw(&res, rBottom, &project, &scene, fpsShown, drawsShown);

        // Ctrl+S -> save the active scene (EDIT mode only; VIEW mode uses S to fly)
        bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        if (editMode && ctrl && IsKeyPressed(KEY_S) && !guiKeyboard && !play.playing)
            tr = (TopbarResult){ TOPBAR_SCENE_SAVE, 0 };

        if (Play_Action(&play, tr.action, &project, &scene, &camera))
            tr.action = TOPBAR_NONE;

        FileOps_Request(&ops, tr, &project, &scene, &camera);
        FileOps_Draw(&ops, &project, &scene, &camera);

        Mge_GuiEndFrame();

        if (IsKeyPressed(KEY_F12) && !guiKeyboard)
            Mge_TakeScreenshot("editor_screenshot.png");

        Mge_EndDrawing();

        if (ops.quit)
            break;
    }

    Play_Shutdown(&play, &scene, &camera);
    Resources_Shutdown(&res);
    Scene_Shutdown(&scene);
    Mge_GuiShutdown();
    Mge_CloseWindow();
    return 0;
}
