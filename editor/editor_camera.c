#include "editor_camera.h"

#include <mge_math.h>
#include <math.h>

static Vector3 FrontFromYawPitch(float yaw, float pitch)
{
    return (Vector3){
        cosf(yaw * DEG2RAD) * cosf(pitch * DEG2RAD),
        sinf(pitch * DEG2RAD),
        sinf(yaw * DEG2RAD) * cosf(pitch * DEG2RAD),
    };
}

static void ApplyLook(EditorCamera* c, float sensitivity)
{
    Vector2 d = GetMouseDelta();
    c->yaw += d.x * sensitivity;
    c->pitch = Clamp(c->pitch - d.y * sensitivity, -89.0f, 89.0f);
    c->cam.target = Vector3Normalize(FrontFromYawPitch(c->yaw, c->pitch));
}

static void MoveWASD(EditorCamera* c)
{
    const float speed = 6.0f * (float)Mge_GetDeltaTime();
    float f = (IsKeyDown(KEY_W) ? speed : 0.0f) - (IsKeyDown(KEY_S) ? speed : 0.0f);
    float s = (IsKeyDown(KEY_D) ? speed : 0.0f) - (IsKeyDown(KEY_A) ? speed : 0.0f);
    if (f == 0.0f && s == 0.0f)
        return;
    Vector3 right = Vector3Normalize(Vector3Cross(c->cam.target, c->cam.up));
    c->cam.position = Vector3_Add(c->cam.position, Vector3_Scale(c->cam.target, f));
    c->cam.position = Vector3_Add(c->cam.position, Vector3_Scale(right, s));
}

void EditorCamera_Init(EditorCamera* c)
{
    *c = (EditorCamera){ 0 };
    c->cam = (Camera3D){
        .position = { 0.0f, 3.5f, 13.0f },
        .target = { 0.0f, 0.0f, -1.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 60.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    c->yaw = -90.0f;
    c->pitch = 0.0f;
    c->looking = false;
}

void EditorCamera_Update(EditorCamera* c, bool editMode, bool guiMouse)
{
    if (!editMode) {
        c->looking = false;
        MoveWASD(c);
        ApplyLook(c, 0.1f);
        return;
    }

    if (!c->looking && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !guiMouse) {
        c->looking = true;
        DisableCursor();
    }
    if (c->looking && IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
        c->looking = false;
        EnableCursor();
    }
    if (c->looking) {
        ApplyLook(c, 0.15f);
        MoveWASD(c);
    }
}

bool EditorCamera_IsLooking(const EditorCamera* c) { return c->looking; }
