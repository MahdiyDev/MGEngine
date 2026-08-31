// The editor's viewport camera: a yaw/pitch fly-cam shared by VIEW mode
// (cursor locked, always flying) and EDIT mode (flies only while RIGHT mouse
// is held). No UI here.
#pragma once

#include <mge.h>
#include <stdbool.h>

typedef struct EditorCamera {
    Camera3D cam;
    float yaw, pitch;    // degrees; drive cam.target
    bool looking;        // EDIT mode: RIGHT mouse currently held
} EditorCamera;

void EditorCamera_Init(EditorCamera* c);

// Jump the camera to `cam` (position / target / up / fov), re-deriving yaw/pitch
// from the target direction so the fly-cam keeps working. Used after loading a
// scene.
void EditorCamera_SetPose(EditorCamera* c, Camera3D cam);

// Advance one frame. `editMode` false = always fly. `guiMouse` gates starting a
// look-drag so a click on a panel doesn't grab the camera. Toggles the OS cursor
// as needed.
void EditorCamera_Update(EditorCamera* c, bool editMode, bool guiMouse);

// True when the RIGHT-mouse look-drag is active (EDIT mode) -- gate picking on it.
bool EditorCamera_IsLooking(const EditorCamera* c);
