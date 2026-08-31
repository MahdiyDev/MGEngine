// MGEngine editor -- a scene editor built on the engine library.
//
//   TAB / top bar        VIEW mode (fly-camera, cursor locked)  <->  EDIT mode
//   fly-camera           WASD move, mouse look
//   edit mode
//     hold RIGHT mouse   look around; WASD flies while held
//     left-click         select an object / a light  (Shift-click = add to selection)
//     drag a gizmo       move / rotate / scale it (hold Ctrl to snap)
//   Ctrl+S  save scene   Ctrl+Z / Ctrl+Y  undo / redo   Ctrl+D  duplicate
//   Delete  remove the selection (asks first)          F12  screenshot
//   Play / Stop          a third mode: panels hide, the scene runs through its
//                        main camera with real input; Esc or Stop returns and
//                        restores the pre-Play scene
//   panels
//     top bar            Project + Scene menus, Play/Build/Console, mode, gizmo, Render
//     left (Hierarchy)   objects + lights; add / rename / toggle / delete / reorder
//     right (Inspector)  the selection's fields
//     bottom             project resource explorer, or the build console
//
// This file: the window, the frame loop, the docked-panel layout, and the one
// Mge_Gui frame the panels share. The Project + active Scene + undo History are
// owned here.
#include <mge.h>
#include <mge_gui.h>

#include <stdio.h>

#include "scene.h"
#include "project.h"
#include "editor_camera.h"
#include "topbar.h"
#include "hierarchy.h"
#include "inspector.h"
#include "resources.h"
#include "fileops.h"
#include "play.h"
#include "history.h"

static const int width = 1280, height = 720;

// docked-shell metrics (px)
enum {
    TOPBAR_H = 46,
    LEFT_W = 240,
    RIGHT_W = 320,
    BOTTOM_H = 172,
};

#define DELETE_ID "Delete selection"

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
    History hist;
    History_Init(&hist);

    int fpsShown = 0, drawsShown = 0;
    double fpsAt = 0.0;
    bool prevEditMode = editMode;
    bool prevPlaying = false;
    bool wantDeletePopup = false;

    for (;;) {
        bool playing = play.playing;
        bool guiKeyboard = Mge_GuiWantsKeyboard();
        bool guiMouse = Mge_GuiWantsMouse();

        if (Mge_GetTime() - fpsAt >= 1.0) {
            fpsShown = Mge_GetFps();
            drawsShown = Mge_GetDrawCalls();
            fpsAt = Mge_GetTime();
        }

        if (IsKeyPressed(KEY_TAB) && !guiKeyboard && !playing)
            editMode = !editMode;

        // cursor: Play mode captures it (the module may re-enable it); leaving Play
        // restores the EDIT/VIEW state
        if (playing != prevPlaying) {
            if (playing)
                DisableCursor();
            else if (editMode)
                EnableCursor();
            else
                DisableCursor();
            prevPlaying = playing;
            prevEditMode = editMode;
        } else if (!playing && editMode != prevEditMode) {
            if (editMode)
                EnableCursor();
            else
                DisableCursor();
            prevEditMode = editMode;
        }

        // --- the view camera ---
        Camera3D view;
        bool gameCam = playing && Scene_MainCamera(&scene, &view);
        if (!gameCam) {
            EditorCamera_Update(&camera, playing ? false : editMode, guiMouse && !playing);
            view = camera.cam;
        }
        play.viewCam = view;

        Play_Frame(&play, &project, &scene); // runs the module; may move the game camera

        if (playing && Scene_MainCamera(&scene, &view))
            play.viewCam = view;

        bool looking = EditorCamera_IsLooking(&camera);
        bool interact = editMode && !looking && !guiMouse && !playing;

        Mge_BeginDrawing();

        bool gizmoBusy = Scene_Draw(&scene, view, interact, !playing); // no editor markers in Play
        if (gizmoBusy) {
            History_Record(&hist);
            scene.dirty = true;
        }
        if (interact && !gizmoBusy)
            Scene_Pick(&scene, view);

        // docked-panel layout, recomputed each frame (window can be resized)
        float W = (float)Mge_GetScreenWidth(), H = (float)Mge_GetScreenHeight();
        float midH = H - TOPBAR_H - BOTTOM_H;
        Rectangle rTop = { 0, 0, W, TOPBAR_H };
        Rectangle rLeft = { 0, TOPBAR_H, LEFT_W, midH };
        Rectangle rRight = { W - RIGHT_W, TOPBAR_H, RIGHT_W, midH };
        Rectangle rBottom = { 0, H - BOTTOM_H, W, BOTTOM_H };

        Mge_GuiBeginFrame();

        // close button / Esc
        if (Mge_WindowShouldClose()) {
            Mge_SetWindowShouldClose(false);
            if (playing)
                Play_Action(&play, TOPBAR_STOP, &project, &scene); // Esc -> stop Play
            else
                FileOps_Request(&ops, (TopbarResult){ TOPBAR_QUIT, 0 }, &project, &scene, &camera, &hist);
        }

        TopbarResult tr = { TOPBAR_NONE, 0 };

        if (playing) {
            if (Play_DrawOverlay(&play, W, fpsShown))
                tr.action = TOPBAR_STOP;
            if (play.showConsole)
                Play_DrawConsole(&play, rBottom);
        } else {
            tr = Topbar_Draw(rTop, &project, &scene, &editMode, play.playing, &play.showConsole);
            if (Hierarchy_Draw(rLeft, &scene, &hist))
                wantDeletePopup = true;
            Inspector_Draw(rRight, &scene, &project, &hist);
            if (play.showConsole)
                Play_DrawConsole(&play, rBottom);
            else
                Resources_Draw(&res, rBottom, &project, &scene, fpsShown, drawsShown);

            // --- editing hotkeys (EDIT mode, not while a field has focus) ---
            bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
            bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            bool keysFree = editMode && !guiKeyboard;

            if (keysFree && ctrl && IsKeyPressed(KEY_S))
                tr = (TopbarResult){ TOPBAR_SCENE_SAVE, 0 };

            if (keysFree && ctrl && IsKeyPressed(KEY_Z)) {
                if (shift)
                    History_Redo(&hist, &scene);
                else
                    History_Undo(&hist, &scene);
            }
            if (keysFree && ctrl && IsKeyPressed(KEY_Y))
                History_Redo(&hist, &scene);

            if (keysFree && ctrl && IsKeyPressed(KEY_D) && scene.selKind == SEL_OBJECT) {
                History_Record(&hist);
                if (Scene_DuplicateSelectedObjects(&scene) > 0) {
                    char root[512];
                    Project_Root(&project, root, sizeof(root));
                    Scene_LoadMaterialTextures(&scene, root);
                }
            }
            if (keysFree && IsKeyPressed(KEY_DELETE) && scene.selKind == SEL_OBJECT)
                wantDeletePopup = true;

            if (wantDeletePopup) {
                Mge_GuiOpenPopup(DELETE_ID);
                wantDeletePopup = false;
            }
            if (Mge_GuiBeginPopup(DELETE_ID)) {
                int n = (scene.selKind == SEL_OBJECT) ? 1 + scene.selExtraCount : 0;
                char msg[64];
                snprintf(msg, sizeof(msg), "Delete %d object%s?", n, n == 1 ? "" : "s");
                Mge_GuiLabel(msg);
                Mge_GuiSpacing();
                if (Mge_GuiButton("Delete")) {
                    History_Record(&hist);
                    Scene_DeleteSelectedObjects(&scene);
                    Mge_GuiClosePopup();
                }
                Mge_GuiSameLine();
                if (Mge_GuiButton("Cancel"))
                    Mge_GuiClosePopup();
                Mge_GuiEndPopup();
            }
        }

        if (Play_Action(&play, tr.action, &project, &scene))
            tr.action = TOPBAR_NONE;

        FileOps_Request(&ops, tr, &project, &scene, &camera, &hist);
        FileOps_Draw(&ops, &project, &scene, &camera, &hist);

        Mge_GuiEndFrame();

        if (IsKeyPressed(KEY_F12) && !guiKeyboard)
            Mge_TakeScreenshot("editor_screenshot.png");

        Mge_EndDrawing();

        // refresh the undo baseline once the scene settles (edit mode only)
        if (!playing && !gizmoBusy && !guiKeyboard && !hist.touched)
            History_Rest(&hist, &scene);
        History_EndFrame(&hist);

        if (ops.quit)
            break;
    }

    History_Free(&hist);
    Play_Shutdown(&play, &scene);
    Resources_Shutdown(&res);
    Scene_Shutdown(&scene);
    Mge_GuiShutdown();
    Mge_CloseWindow();
    return 0;
}
