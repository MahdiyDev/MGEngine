// MGEngine demo.
//   TAB           toggle between fly-camera (cursor locked) and edit mode (cursor free)
//   fly-camera    WASD to move, mouse to look
//   edit mode     left-click a box to select it, drag a gizmo arrow to move it
//                 along that axis; right-click to deselect
#include "mge.h"
#include "mge_gl.h"
#include "mge_math.h"

#include <math.h>

static const int width = 1280, height = 720;
static bool firstMouse = true;
static float lastX = 640.0f;
static float lastY = 360.0f;
static float yaw = -90.0f;
static float pitch = 0.0f;

static void HandleCameraMovement(Camera3D* camera)
{
    const float cameraSpeed = 5.0f * (float)Mge_GetDeltaTime();
    if (IsKeyDown(KEY_W))
        camera->position = Vector3_Add(camera->position, Vector3_Scale(camera->target, cameraSpeed));
    else if (IsKeyDown(KEY_S))
        camera->position = Vector3_Subtract(camera->position, Vector3_Scale(camera->target, cameraSpeed));
    else if (IsKeyDown(KEY_A))
        camera->position = Vector3_Subtract(camera->position,
            Vector3_Scale(Vector3Normalize(Vector3Cross(camera->target, camera->up)), cameraSpeed));
    else if (IsKeyDown(KEY_D))
        camera->position = Vector3_Add(camera->position,
            Vector3_Scale(Vector3Normalize(Vector3Cross(camera->target, camera->up)), cameraSpeed));

    float xpos = GetMouseX();
    float ypos = GetMouseY();

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed: y goes bottom-to-top
    lastX = xpos;
    lastY = ypos;

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;
    pitch = Clamp(pitch, -89.0f, 89.0f);

    Vector3 front;
    front.x = cosf(yaw * DEG2RAD) * cosf(pitch * DEG2RAD);
    front.y = sinf(pitch * DEG2RAD);
    front.z = sinf(yaw * DEG2RAD) * cosf(pitch * DEG2RAD);
    camera->target = Vector3Normalize(front);
}

int main(void)
{
    Mge_InitWindow(width, height, "MGEngine v1.0");
    Mge_SetTargetFPS(60);
    DisableCursor(); // start in fly-camera mode

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 3.0f, 12.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, -1.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // a dim directional fill + an orbiting point light
    Light sun = Mge_MakeDirectionalLight((Vector3){ -0.6f, -1.0f, -0.4f }, (Vector3){ 0.6f, 0.6f, 0.7f });
    Light lamp = Mge_MakePointLight((Vector3){ 4.0f, 6.0f, 4.0f }, (Vector3){ 1.0f, 0.9f, 0.7f });

    const int N = 4;
    const float AXIS = 1.6f;
    Object objects[4] = {
        Mge_MakeObject3D((Vector3){ -3.0f, 0.0f, 0.0f }, (Vector3){ 1.0f, 1.0f, 1.0f }, RED),
        Mge_MakeObject3D((Vector3){ 3.0f, 0.0f, 0.0f }, (Vector3){ 1.0f, 1.0f, 1.0f }, GREEN),
        Mge_MakeObject3D((Vector3){ 0.0f, 0.0f, -3.0f }, (Vector3){ 1.0f, 1.0f, 1.0f }, BLUE),
        Mge_MakeObject3D((Vector3){ 0.0f, 2.5f, 0.0f }, (Vector3){ 1.6f, 0.6f, 1.6f }, YELLOW),
    };

    while (!Mge_WindowShouldClose()) {
        if (IsKeyPressed(KEY_TAB)) {
            Mge_ToggleCursor();
            firstMouse = true; // avoid a camera jump when re-locking
        }

        bool editing = !IsCursorHidden();
        int selected = -1;

        if (editing) {
            selected = Mge_ManipulateObjects3D(objects, N, camera, AXIS);
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
                Mge_ClearSelection(objects, N);
        } else {
            HandleCameraMovement(&camera);
        }

        // orbit the point light so the specular highlight moves across the boxes
        double t = Mge_GetTime();
        lamp.position = (Vector3){ (float)cos(t) * 6.0f, 6.0f, (float)sin(t) * 6.0f };

        Light lights[2] = { sun, lamp };

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 18, 18, 22, 255 }); // dark room

        Mge_BeginMode3D(camera);

        // directional + point in one pass
        Mge_BeginLighting3DEx(lights, 2, camera);
        Mge_SetMaterial((Material){ .maps[MATERIAL_MAP_DIFFUSE].color = DARKGRAY,
            .maps[MATERIAL_MAP_SPECULAR].value = 1.0f, .shininess = 8.0f });
        Draw_Cube((Vector3){ 0.0f, -1.0f, 0.0f }, (Vector3){ 24.0f, 0.1f, 24.0f }, DARKGRAY);
        for (int i = 0; i < N; i++)
            Mge_DrawObject(objects[i]);
        Mge_EndLighting3D();

        // unlit markers so you can see where the lights are
        Draw_Cube(lamp.position, (Vector3){ 0.3f, 0.3f, 0.3f },
            (Color){ 255, 235, 180, 255 }); // the point light

        // the sun has no position -- park a marker up-light and point an arrow along its rays
        Vector3 sunDir = Vector3Normalize(sun.direction);
        Vector3 sunAt = Vector3_Subtract((Vector3){ 0.0f, 0.0f, 0.0f }, Vector3_Scale(sunDir, 10.0f));
        Draw_Cube(sunAt, (Vector3){ 0.5f, 0.5f, 0.5f }, (Color){ 180, 190, 255, 255 });
        Draw_Arrow3D(sunAt, Vector3_Add(sunAt, Vector3_Scale(sunDir, 3.0f)), (Color){ 180, 190, 255, 255 });

        // gizmo / selection wires are unlit overlay lines
        if (selected >= 0)
            Mge_DrawObjectGizmo(objects[selected], AXIS);

        Mge_EndMode3D();
        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
