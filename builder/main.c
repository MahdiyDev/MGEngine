// MGEngine builder -- a tiny scene editor built on the engine library.
//
//   TAB                     fly-camera (cursor locked)  <->  edit mode (cursor free)
//   fly-camera              WASD move, mouse look
//   edit mode
//     hold RIGHT mouse      look around; WASD flies while held
//     left-click a cube     select it, drag its gizmo arrow to move it
//     sidebar               pick any object/light and edit its fields
#include <mge.h>
#include <mge_gui.h>
#include <mge_math.h>

#include <math.h>
#include <stdio.h>

static const int width = 1280, height = 720;

// --- shared camera orientation ---
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

static void MoveAlongView(Camera3D* camera, float forward, float strafe)
{
    Vector3 right = Vector3Normalize(Vector3Cross(camera->target, camera->up));
    camera->position = Vector3_Add(camera->position, Vector3_Scale(camera->target, forward));
    camera->position = Vector3_Add(camera->position, Vector3_Scale(right, strafe));
}

// WASD along the current facing (W/S forward-back, A/D strafe)
static void MoveWASD(Camera3D* camera)
{
    const float speed = 6.0f * (float)Mge_GetDeltaTime();
    float f = (IsKeyDown(KEY_W) ? speed : 0.0f) - (IsKeyDown(KEY_S) ? speed : 0.0f);
    float s = (IsKeyDown(KEY_D) ? speed : 0.0f) - (IsKeyDown(KEY_A) ? speed : 0.0f);
    if (f != 0.0f || s != 0.0f)
        MoveAlongView(camera, f, s);
}

// fly mode: WASD + mouse look, cursor locked
static void FlyCamera(Camera3D* camera)
{
    MoveWASD(camera);
    ApplyLook(camera, 0.1f);
}

// --- selection ---
enum { SEL_NONE, SEL_OBJECT, SEL_LIGHT };
static int selKind = SEL_NONE;
static int selIndex = 0;

// --- inspectors: only "draw box / draw input" abstract calls, no ImGui here ---

static void InspectObject(Object* o)
{
    Mge_GuiLabel(o->kind == OBJECT_3D ? "box (3D)" : "rect (2D)");
    Mge_GuiInputVec3("position", &o->position);
    Mge_GuiInputVec3("size", &o->size);
    Mge_GuiInputColor("color", &o->color);
    Mge_GuiSeparator();
    Mge_GuiLabel("material");
    Mge_GuiInputColor("diffuse", &o->material.maps[MATERIAL_MAP_DIFFUSE].color);
    Mge_GuiSliderFloat("specular", &o->material.maps[MATERIAL_MAP_SPECULAR].value, 0.0f, 1.0f);
    Mge_GuiInputFloat("shininess", &o->material.shininess);
}

static void InspectLight(Light* l)
{
    static const char* kinds[3] = { "directional", "point", "spot" };
    Mge_GuiLabel(kinds[l->type]);
    Mge_GuiCheckbox("enabled", &l->enabled);
    Mge_GuiInputColorRGB("color", &l->color);
    Mge_GuiSliderFloat("ambient", &l->ambient, 0.0f, 1.0f);
    Mge_GuiSliderFloat("diffuse", &l->diffuse, 0.0f, 2.0f);
    Mge_GuiSliderFloat("specular", &l->specular, 0.0f, 2.0f);
    if (l->type != LIGHT_DIRECTIONAL) {
        Mge_GuiSeparator();
        Mge_GuiInputVec3("position", &l->position);
        Mge_GuiInputFloat("linear", &l->linear);
        Mge_GuiInputFloat("quadratic", &l->quadratic);
    }
    if (l->type != LIGHT_POINT) {
        Mge_GuiSeparator();
        Mge_GuiInputVec3("direction", &l->direction);
    }
}

int main(void)
{
    Mge_InitWindow(width, height, "MGEngine builder");
    Mge_SetTargetFPS(60);

    bool flyMode = true;
    bool looking = false; // dragging with RIGHT mouse in edit mode
    DisableCursor();       // start in fly-camera mode

    Camera3D camera = {
        .position = { 0.0f, 3.5f, 13.0f },
        .target = { 0.0f, 0.0f, -1.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 60.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    Light sun = Mge_MakeDirectionalLight((Vector3){ -0.5f, -1.0f, -0.4f }, (Vector3){ 0.7f, 0.7f, 0.8f });
    Light lamp = Mge_MakePointLight((Vector3){ 3.0f, 5.0f, 2.0f }, (Vector3){ 1.0f, 0.85f, 0.6f });

    const int N = 4;
    const float AXIS = 1.6f; // object move-gizmo arrow length
    Object objects[4] = {
        Mge_MakeObject3D((Vector3){ 0.0f, -1.1f, 0.0f }, (Vector3){ 24.0f, 0.2f, 24.0f }, (Color){ 90, 95, 105, 255 }),
        Mge_MakeObject3D((Vector3){ -3.0f, 0.0f, 0.0f }, (Vector3){ 1.5f, 1.5f, 1.5f }, (Color){ 200, 80, 80, 255 }),
        Mge_MakeObject3D((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 1.5f, 1.5f, 1.5f }, (Color){ 90, 190, 110, 255 }),
        Mge_MakeObject3D((Vector3){ 3.0f, 0.0f, 0.0f }, (Vector3){ 1.5f, 1.5f, 1.5f }, (Color){ 90, 130, 210, 255 }),
    };

    while (!Mge_WindowShouldClose()) {
        bool guiKeyboard = Mge_GuiWantsKeyboard();
        bool guiMouse = Mge_GuiWantsMouse();

        if (IsKeyPressed(KEY_TAB) && !guiKeyboard) {
            flyMode = !flyMode;
            looking = false;
            if (flyMode)
                DisableCursor();
            else
                EnableCursor();
        }

        if (flyMode) {
            FlyCamera(&camera);
        } else {
            // hold RIGHT mouse to look around; lock the cursor while dragging
            if (!looking && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !guiMouse) {
                looking = true;
                DisableCursor();
            }
            if (looking && IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
                looking = false;
                EnableCursor();
            }

            if (looking) {
                // hold RIGHT mouse: look with the mouse, fly with WASD
                ApplyLook(&camera, 0.15f);
                MoveWASD(&camera);
            } else if (!guiMouse) {
                int hit = Mge_ManipulateObjects3D(objects, N, camera, AXIS);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (hit >= 0) {
                        selKind = SEL_OBJECT;
                        selIndex = hit;
                    } else if (selKind == SEL_OBJECT) {
                        selKind = SEL_NONE; // clicked empty space
                    }
                }
            }
        }

        for (int i = 0; i < N; i++)
            objects[i].selected = (selKind == SEL_OBJECT && selIndex == i);

        Light lights[2] = { sun, lamp };

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 20, 21, 26, 255 });

        Mge_BeginMode3D(camera);
        Mge_BeginLighting3DEx(lights, 2, camera);
        for (int i = 0; i < N; i++)
            Mge_DrawObject(objects[i]);
        Mge_EndLighting3D();

        Draw_Cube(lamp.position, (Vector3){ 0.3f, 0.3f, 0.3f }, (Color){ 255, 235, 180, 255 });
        if (selKind == SEL_OBJECT)
            Mge_DrawObjectGizmo(objects[selIndex], AXIS);
        Mge_EndMode3D();

        // --- sidebar ---
        Mge_GuiBeginFrame();
        if (Mge_GuiBeginSidebar("Scene", 300.0f, false)) {
            Mge_GuiLabel("OBJECTS");
            for (int i = 0; i < N; i++) {
                char row[32];
                snprintf(row, sizeof(row), "%s %d", (i == 0) ? "Floor" : "Cube", i);
                if (Mge_GuiSelectable(row, selKind == SEL_OBJECT && selIndex == i)) {
                    // same as clicking the object -> its gizmo is now draggable
                    Mge_SetSelectedObject(objects, N, i);
                    selKind = SEL_OBJECT;
                    selIndex = i;
                }
            }
            Mge_GuiSeparator();
            Mge_GuiLabel("LIGHTS");
            const char* lightRows[2] = { "Sun", "Lamp" };
            for (int i = 0; i < 2; i++)
                if (Mge_GuiSelectable(lightRows[i], selKind == SEL_LIGHT && selIndex == i)) {
                    Mge_ClearSelection(objects, N); // no object gizmo for a light
                    selKind = SEL_LIGHT;
                    selIndex = i;
                }

            Mge_GuiSeparator();
            Mge_GuiLabel("INSPECTOR");
            Mge_GuiSpacing();
            if (selKind == SEL_OBJECT)
                InspectObject(&objects[selIndex]);
            else if (selKind == SEL_LIGHT)
                InspectLight((selIndex == 0) ? &sun : &lamp);
            else
                Mge_GuiLabel("(nothing selected)");
        }
        Mge_GuiEndSidebar();
        Mge_GuiEndFrame();

        Mge_EndDrawing();
    }

    Mge_GuiShutdown();
    Mge_CloseWindow();
    return 0;
}
