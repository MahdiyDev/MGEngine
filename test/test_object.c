#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "mge.h"
#include "mge_math.h"
#include "test.h" // mlib repo-wide harness

static bool feq(float a, float b) { return fabsf(a - b) < 1e-3f; }
#define CHECK_F(a, b) CHECK(feq((a), (b)))

// ---- fake input / draw backend (so mge_object.c links without a window) ----

void Draw_RectangleRec(Rectangle r, Color c) { (void)r; (void)c; }
void Draw_RectangleLines(int a, int b, int w, int h, Color c) { (void)a; (void)b; (void)w; (void)h; (void)c; }
void Draw_Cube(Vector3 a, Vector3 b, Color c) { (void)a; (void)b; (void)c; }
void Draw_CubeWires(Vector3 a, Vector3 b, Color c) { (void)a; (void)b; (void)c; }
void Draw_Arrow(Vector2 a, Vector2 b, float s, Color c) { (void)a; (void)b; (void)s; (void)c; }
void Draw_Arrow3D(Vector3 a, Vector3 b, Color c) { (void)a; (void)b; (void)c; }

static Vector2 g_mouse;
static int g_btn[8];
static int g_btnPrev[8];

Vector2 GetMousePosition(void) { return g_mouse; }
bool IsMouseButtonPressed(int b) { return b >= 0 && b < 8 && !g_btnPrev[b] && g_btn[b]; }
bool IsMouseButtonDown(int b) { return b >= 0 && b < 8 && g_btn[b]; }
bool IsMouseButtonReleased(int b) { return b >= 0 && b < 8 && g_btnPrev[b] && !g_btn[b]; }
int Mge_GetScreenWidth(void) { return 800; }
int Mge_GetScreenHeight(void) { return 600; }

// advance one "frame": current button state becomes previous
static void frame(void) { memcpy(g_btnPrev, g_btn, sizeof(g_btn)); }
static void press_at(float x, float y)
{
    frame();
    g_mouse = (Vector2){ x, y };
    g_btn[MOUSE_BUTTON_LEFT] = 1;
}
static void drag_to(float x, float y)
{
    frame();
    g_mouse = (Vector2){ x, y };
}
static void release(void)
{
    frame();
    g_btn[MOUSE_BUTTON_LEFT] = 0;
}

// ---- tests ----

TEST(make_objects)
{
    Object a = Mge_MakeObject2D(10.0f, 20.0f, 30.0f, 40.0f, RED);
    CHECK(a.kind == OBJECT_2D);
    CHECK_F(a.position.x, 10.0f);
    CHECK_F(a.position.y, 20.0f);
    CHECK_F(a.size.x, 30.0f);
    CHECK_F(a.size.y, 40.0f);
    CHECK(!a.selected);

    Object b = Mge_MakeObject3D((Vector3){ 1, 2, 3 }, (Vector3){ 4, 5, 6 }, GREEN);
    CHECK(b.kind == OBJECT_3D);
    CHECK_F(b.position.z, 3.0f);
    CHECK_F(b.size.z, 6.0f);
}

TEST(vector4_transform)
{
    Vector4 p = { 1, 2, 3, 1 };
    Vector4 id = Vector4_Transform(p, Matrix_Identity());
    CHECK_F(id.x, 1.0f);
    CHECK_F(id.y, 2.0f);
    CHECK_F(id.z, 3.0f);
    CHECK_F(id.w, 1.0f);

    Vector4 t = Vector4_Transform((Vector4){ 0, 0, 0, 1 }, Matrix_Translate(5, 6, 7));
    CHECK_F(t.x, 5.0f);
    CHECK_F(t.y, 6.0f);
    CHECK_F(t.z, 7.0f);
}

TEST(camera_projection_matrix)
{
    Camera3D c = { .position = { 0, 0, 5 }, .target = { 0, 0, -1 }, .up = { 0, 1, 0 },
        .fovy = 60.0f, .projection = CAMERA_PERSPECTIVE };
    Matrix p = Mge_GetCameraProjectionMatrix(c, 800.0f / 600.0f);
    CHECK_F(p.m11, -1.0f);
    CHECK(p.m0 > 0.0f && p.m5 > 0.0f);
}

TEST(world_to_screen)
{
    Camera3D c = { .position = { 0, 0, 5 }, .target = { 0, 0, -1 }, .up = { 0, 1, 0 },
        .fovy = 60.0f, .projection = CAMERA_PERSPECTIVE };

    // a point straight ahead projects to the screen centre
    Vector2 mid = Mge_GetWorldToScreenEx((Vector3){ 0, 0, 0 }, c, 800, 600);
    CHECK_F(mid.x, 400.0f);
    CHECK_F(mid.y, 300.0f);

    // a point above the target projects above the centre (smaller y)
    Vector2 up = Mge_GetWorldToScreenEx((Vector3){ 0, 1, 0 }, c, 800, 600);
    CHECK_F(up.x, 400.0f);
    CHECK(up.y < 300.0f);

    // a point to the right projects right of the centre
    Vector2 right = Mge_GetWorldToScreenEx((Vector3){ 1, 0, 0 }, c, 800, 600);
    CHECK(right.x > 400.0f);
    CHECK_F(right.y, 300.0f);
}

TEST(manipulate_2d_select_and_body_drag)
{
    Object objs[2] = {
        Mge_MakeObject2D(100.0f, 100.0f, 40.0f, 40.0f, RED),
        Mge_MakeObject2D(300.0f, 300.0f, 40.0f, 40.0f, GREEN),
    };
    Mge_ClearSelection(objs, 2);

    // click empty space -> nothing selected
    press_at(600.0f, 50.0f);
    CHECK(Mge_ManipulateObjects2D(objs, 2, 60.0f) == -1);
    release();
    Mge_ManipulateObjects2D(objs, 2, 60.0f);

    // click object 0's body -> selected
    press_at(100.0f, 100.0f);
    CHECK(Mge_ManipulateObjects2D(objs, 2, 60.0f) == 0);
    CHECK(objs[0].selected && !objs[1].selected);

    // drag the body by (+50, +30)
    drag_to(150.0f, 130.0f);
    Mge_ManipulateObjects2D(objs, 2, 60.0f);
    CHECK_F(objs[0].position.x, 150.0f);
    CHECK_F(objs[0].position.y, 130.0f);
    release();
    Mge_ManipulateObjects2D(objs, 2, 60.0f);

    Mge_ClearSelection(objs, 2);
}

TEST(manipulate_2d_axis_constrained_drag)
{
    Object objs[1] = { Mge_MakeObject2D(200.0f, 200.0f, 40.0f, 40.0f, BLUE) };
    Mge_ClearSelection(objs, 1);
    float axis = 60.0f;

    // select it
    press_at(200.0f, 200.0f);
    Mge_ManipulateObjects2D(objs, 1, axis);
    release();
    Mge_ManipulateObjects2D(objs, 1, axis);

    // grab the +X arrow (mid-point ~ (230, 200)) and drag by (+40, +25)
    press_at(230.0f, 200.0f);
    Mge_ManipulateObjects2D(objs, 1, axis);
    drag_to(270.0f, 225.0f);
    Mge_ManipulateObjects2D(objs, 1, axis);
    CHECK_F(objs[0].position.x, 240.0f); // moved along X only
    CHECK_F(objs[0].position.y, 200.0f); // Y unchanged
    release();
    Mge_ManipulateObjects2D(objs, 1, axis);

    // grab the +Y arrow (points up) at the object's *current* position, drag up 30
    press_at(objs[0].position.x, objs[0].position.y - 30.0f); // ~ (240, 170)
    Mge_ManipulateObjects2D(objs, 1, axis);
    drag_to(objs[0].position.x + 15.0f, objs[0].position.y - 60.0f); // ~ (255, 140)
    Mge_ManipulateObjects2D(objs, 1, axis);
    CHECK_F(objs[0].position.x, 240.0f); // X unchanged
    CHECK_F(objs[0].position.y, 170.0f); // moved up by 30
    release();
    Mge_ManipulateObjects2D(objs, 1, axis);

    Mge_ClearSelection(objs, 1);
}

TEST(manipulate_3d_axis_drag_moves_along_axis)
{
    Object objs[1] = { Mge_MakeObject3D((Vector3){ 0, 0, 0 }, (Vector3){ 1, 1, 1 }, RED) };
    Camera3D cam = { .position = { 0, 0, 10 }, .target = { 0, 0, -1 }, .up = { 0, 1, 0 },
        .fovy = 60.0f, .projection = CAMERA_PERSPECTIVE };
    Mge_ClearSelection(objs, 1);
    float axis = 2.0f;

    // select by clicking near the object's screen centre (~400,300)
    press_at(400.0f, 300.0f);
    CHECK(Mge_ManipulateObjects3D(objs, 1, cam, axis) == 0);
    release();
    Mge_ManipulateObjects3D(objs, 1, cam, axis);

    // the +X axis runs horizontally on screen; grab it just right of centre
    Vector2 xTip = Mge_GetWorldToScreenEx((Vector3){ axis, 0, 0 }, cam, 800, 600);
    Vector2 origin = Mge_GetWorldToScreenEx((Vector3){ 0, 0, 0 }, cam, 800, 600);
    press_at((origin.x + xTip.x) * 0.5f, (origin.y + xTip.y) * 0.5f);
    Mge_ManipulateObjects3D(objs, 1, cam, axis);

    // drag the mouse to the right -> object moves along +X, y/z barely change
    drag_to(g_mouse.x + 40.0f, g_mouse.y);
    Mge_ManipulateObjects3D(objs, 1, cam, axis);
    CHECK(objs[0].position.x > 0.05f);
    CHECK_F(objs[0].position.y, 0.0f);
    CHECK_F(objs[0].position.z, 0.0f);
    release();
    Mge_ManipulateObjects3D(objs, 1, cam, axis);

    Mge_ClearSelection(objs, 1);
}

int main(void)
{
    RUN(make_objects);
    RUN(vector4_transform);
    RUN(camera_projection_matrix);
    RUN(world_to_screen);
    RUN(manipulate_2d_select_and_body_drag);
    RUN(manipulate_2d_axis_constrained_drag);
    RUN(manipulate_3d_axis_drag_moves_along_axis);
    return test_summary();
}
