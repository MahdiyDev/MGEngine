// The manipulation gizmo (mge_gizmo.c). No window: the GL draw + input backend
// is stubbed. Mge_GetWorldToScreenEx is faked as a simple 2D projection
// (screen.x grows with world +x, screen.y grows with world -y) so the axis
// picking / drag maths are exercised deterministically.

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "mge.h"
#include "mge_gl.h"
#include "mge_math.h"
#include "test.h"

static bool feq(float a, float b) { return fabsf(a - b) < 1e-3f; }
#define CHECK_F(a, b) CHECK(feq((a), (b)))

// ---- stub backend ----

#define VIEW_SCALE 40.0f
#define SCR_W 800
#define SCR_H 600

void Draw_Cube(Vector3 a, Vector3 b, Color c) { (void)a; (void)b; (void)c; }
void Draw_Sphere(Vector3 a, float r, Color c) { (void)a; (void)r; (void)c; }
void MgeGL_SetBlend(bool e) { (void)e; }
void MgeGL_Begin(int m) { (void)m; }
void MgeGL_Color4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a) { (void)r; (void)g; (void)b; (void)a; }
void MgeGL_Vertex3f(float x, float y, float z) { (void)x; (void)y; (void)z; }
void MgeGL_End(void) {}
void MgeGL_Draw(void) {}
void MgeGL_SetDepthFunc(int f) { (void)f; }
void MgeGL_SetDepthMask(bool e) { (void)e; }
int Mge_GetScreenWidth(void) { return SCR_W; }
int Mge_GetScreenHeight(void) { return SCR_H; }

Vector2 Mge_GetWorldToScreenEx(Vector3 p, Camera3D cam, int w, int h)
{
    (void)cam;
    return (Vector2){ w * 0.5f + p.x * VIEW_SCALE, h * 0.5f - p.y * VIEW_SCALE };
}

static Vector2 g_mouse;
static int g_btn, g_btnPrev;

Vector2 GetMousePosition(void) { return g_mouse; }
bool IsMouseButtonPressed(int b) { (void)b; return !g_btnPrev && g_btn; }
bool IsMouseButtonDown(int b) { (void)b; return g_btn != 0; }
bool IsMouseButtonReleased(int b) { (void)b; return g_btnPrev && !g_btn; }

static void frame(void) { g_btnPrev = g_btn; }
static void press_at(float x, float y) { frame(); g_mouse = (Vector2){ x, y }; g_btn = 1; }
static void drag_to(float x, float y) { frame(); g_mouse = (Vector2){ x, y }; }
static void release(void) { frame(); g_btn = 0; }

// ---- tests ----

TEST(gizmo_mode_round_trips)
{
    CHECK(Mge_GetGizmoMode() == GIZMO_TRANSLATE); // default
    Mge_SetGizmoMode(GIZMO_ROTATE);
    CHECK(Mge_GetGizmoMode() == GIZMO_ROTATE);
    Mge_SetGizmoMode(GIZMO_SCALE);
    CHECK(Mge_GetGizmoMode() == GIZMO_SCALE);
    Mge_SetGizmoMode(GIZMO_TRANSLATE);
}

TEST(rotate_xyz_90_about_z_maps_x_to_y)
{
    Matrix rz = Matrix_RotateXYZ((Vector3){ 0.0f, 0.0f, (float)PI / 2.0f });
    Vector3 v = Vector3_RotateAround((Vector3){ 1, 0, 0 }, (Vector3){ 0, 0, 0 }, rz);
    CHECK_F(v.x, 0.0f);
    CHECK_F(v.y, 1.0f);
    CHECK_F(v.z, 0.0f);
}

TEST(rotate_around_a_pivot_keeps_the_pivot_fixed)
{
    Matrix r = Matrix_RotateXYZ((Vector3){ 0.0f, (float)PI, 0.0f });
    Vector3 pivot = { 5, 1, -2 };
    Vector3 same = Vector3_RotateAround(pivot, pivot, r);
    CHECK_F(same.x, pivot.x);
    CHECK_F(same.y, pivot.y);
    CHECK_F(same.z, pivot.z);
    // a point 1 unit +x of the pivot lands 1 unit -x of it after 180 deg about y
    Vector3 p = Vector3_RotateAround((Vector3){ 6, 1, -2 }, pivot, r);
    CHECK_F(p.x, 4.0f);
}

TEST(translate_drag_moves_along_the_grabbed_axis)
{
    Mge_SetGizmoMode(GIZMO_TRANSLATE);
    Vector3 pos = { 0, 0, 0 };
    Camera3D cam = { 0 };
    float size = 2.0f;

    // +X handle runs from screen centre to (centre + size*VIEW_SCALE, centre)
    float cx = SCR_W * 0.5f, cy = SCR_H * 0.5f;
    press_at(cx + size * VIEW_SCALE * 0.5f, cy); // click mid-way along +X
    bool busy = Mge_Gizmo3D(&pos, NULL, NULL, cam, size);
    CHECK(busy); // grabbed a handle

    drag_to(g_mouse.x + 80.0f, g_mouse.y); // 80 px right = 2 world units
    Mge_Gizmo3D(&pos, NULL, NULL, cam, size);
    CHECK(pos.x > 1.5f && pos.x < 2.5f);
    CHECK_F(pos.y, 0.0f);
    CHECK_F(pos.z, 0.0f);

    release();
    CHECK(Mge_Gizmo3D(&pos, NULL, NULL, cam, size) == false);
}

TEST(centre_ball_drag_moves_on_the_view_plane)
{
    Mge_SetGizmoMode(GIZMO_TRANSLATE);
    Vector3 pos = { 0, 0, 0 };
    Camera3D cam = { .target = { 0, 0, -1 }, .up = { 0, 1, 0 } };
    float size = 2.0f;
    float cx = SCR_W * 0.5f, cy = SCR_H * 0.5f;

    press_at(cx, cy); // right on the centre ball
    bool busy = Mge_Gizmo3D(&pos, NULL, NULL, cam, size);
    CHECK(busy);

    drag_to(cx + 80.0f, cy); // 80 px right = 2 world units along camera-right (+x)
    Mge_Gizmo3D(&pos, NULL, NULL, cam, size);
    CHECK(pos.x > 1.5f && pos.x < 2.5f);
    CHECK_F(pos.z, 0.0f);
    release();
    Mge_Gizmo3D(&pos, NULL, NULL, cam, size);
}

TEST(scale_drag_grows_the_axis_component)
{
    Mge_SetGizmoMode(GIZMO_SCALE);
    Vector3 pos = { 0, 0, 0 };
    Vector3 scale = { 1, 1, 1 };
    Camera3D cam = { 0 };
    float size = 2.0f;
    float cx = SCR_W * 0.5f, cy = SCR_H * 0.5f;

    press_at(cx, cy - size * VIEW_SCALE * 0.5f); // +Y handle (screen up)
    Mge_Gizmo3D(&pos, NULL, &scale, cam, size);
    drag_to(g_mouse.x, g_mouse.y - 40.0f);       // drag further up
    Mge_Gizmo3D(&pos, NULL, &scale, cam, size);

    CHECK(scale.y > 1.05f);
    CHECK_F(scale.x, 1.0f);
    CHECK_F(scale.z, 1.0f);
    release();
    Mge_Gizmo3D(&pos, NULL, &scale, cam, size);
    Mge_SetGizmoMode(GIZMO_TRANSLATE);
}

int main(void)
{
    RUN(gizmo_mode_round_trips);
    RUN(rotate_xyz_90_about_z_maps_x_to_y);
    RUN(rotate_around_a_pivot_keeps_the_pivot_fixed);
    RUN(translate_drag_moves_along_the_grabbed_axis);
    RUN(centre_ball_drag_moves_on_the_view_plane);
    RUN(scale_drag_grows_the_axis_component);
    return test_summary();
}
