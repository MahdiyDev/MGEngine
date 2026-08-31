// MGEngine builder -- a tiny scene editor built on the engine library.
//
//   TAB                  VIEW mode (fly-camera, cursor locked)  <->  EDIT mode
//   fly-camera           WASD move, mouse look
//   edit mode
//     hold RIGHT mouse   look around; WASD flies while held
//     left-click         select an object / the lamp
//     drag a gizmo       move / rotate / scale it (switch mode in the sidebar)
//     left sidebar       mode, FPS, shadows, gizmo mode, entity list + inspector
//     right explorer     spawn a cube / sphere / plane into the scene
//
// This file: the window, the frame loop, the camera, and the one Mge_Gui frame
// the two panels share. Scene data + rendering live in scene.c; the panels'
// Mge_Gui* calls live in sidebar.c / explorer.c.
#include <mge.h>
#include <mge_gui.h> // only the input-gate queries + frame lifecycle; widgets are in the panels
#include <mge_math.h>

#include <math.h>

#include "scene.h"
#include "sidebar.h"
#include "explorer.h"

static const int width = 1280, height = 720;

// --- camera orientation shared by fly + edit look ---
static float yaw = -90.0f, pitch = 0.0f;

static Vector3 FrontFromYawPitch(void)
{
    return (Vector3){
        cosf(yaw * DEG2RAD) * cosf(pitch * DEG2RAD),
        sinf(pitch * DEG2RAD),
        sinf(yaw * DEG2RAD) * cosf(pitch * DEG2RAD),
    };
}

static void ApplyLook(Camera3D* camera, float sensitivity)
{
    Vector2 d = GetMouseDelta();
    yaw += d.x * sensitivity;
    pitch = Clamp(pitch - d.y * sensitivity, -89.0f, 89.0f);
    camera->target = Vector3Normalize(FrontFromYawPitch());
}

static void MoveWASD(Camera3D* camera)
{
    const float speed = 6.0f * (float)Mge_GetDeltaTime();
    float f = (IsKeyDown(KEY_W) ? speed : 0.0f) - (IsKeyDown(KEY_S) ? speed : 0.0f);
    float s = (IsKeyDown(KEY_D) ? speed : 0.0f) - (IsKeyDown(KEY_A) ? speed : 0.0f);
    if (f == 0.0f && s == 0.0f)
        return;
    Vector3 right = Vector3Normalize(Vector3Cross(camera->target, camera->up));
    camera->position = Vector3_Add(camera->position, Vector3_Scale(camera->target, f));
    camera->position = Vector3_Add(camera->position, Vector3_Scale(right, s));
}

int main(void)
{
    Mge_SetMSAA(4); // 4x anti-aliasing on every shape / object / model
    Mge_InitWindow(width, height, "MGEngine builder");
    Mge_SetTargetFPS(60);

    bool editMode = true;  // start in EDIT mode (cursor free, sidebar clickable)
    bool looking = false;   // holding RIGHT mouse in edit mode
    EnableCursor();

    Camera3D camera = {
        .position = { 0.0f, 3.5f, 13.0f },
        .target = { 0.0f, 0.0f, -1.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 60.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    Scene scene;
    Scene_Init(&scene, width, height);

    int fpsShown = 0, drawsShown = 0;
    double fpsAt = 0.0;

    while (!Mge_WindowShouldClose()) {
        bool guiKeyboard = Mge_GuiWantsKeyboard();
        bool guiMouse = Mge_GuiWantsMouse();

        if (Mge_GetTime() - fpsAt >= 1.0) {
            fpsShown = Mge_GetFps();
            drawsShown = Mge_GetDrawCalls();
            fpsAt = Mge_GetTime();
        }

        if (IsKeyPressed(KEY_TAB) && !guiKeyboard) {
            editMode = !editMode;
            looking = false;
            if (editMode)
                EnableCursor();
            else
                DisableCursor();
        }

        // --- camera ---
        if (!editMode) {
            MoveWASD(&camera);
            ApplyLook(&camera, 0.1f);
        } else {
            if (!looking && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !guiMouse) {
                looking = true;
                DisableCursor();
            }
            if (looking && IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
                looking = false;
                EnableCursor();
            }
            if (looking) {
                ApplyLook(&camera, 0.15f);
                MoveWASD(&camera);
            }
        }

        bool interact = editMode && !looking && !guiMouse;

        Mge_BeginDrawing();

        bool gizmoBusy = Scene_Draw(&scene, camera, interact);
        if (interact && !gizmoBusy)
            Scene_Pick(&scene, camera);

        Mge_GuiBeginFrame();
        Sidebar_Draw(&scene, editMode, fpsShown, drawsShown);
        Explorer_Draw(&scene);
        Mge_GuiEndFrame();

        Mge_EndDrawing();
    }

    Scene_Shutdown(&scene);
    Mge_GuiShutdown();
    Mge_CloseWindow();
    return 0;
}
