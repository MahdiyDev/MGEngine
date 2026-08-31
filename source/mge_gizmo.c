// Manipulation gizmo: one switchable handle set (translate / rotate / scale) for
// a single target, drawn on top of the scene.
//
//   Mge_SetGizmoMode(GIZMO_ROTATE);
//   bool busy = Mge_Gizmo3D(&obj.position, &obj.rotation, &obj.size, camera, s);
//
// Drawn depth-test-disabled so it is always visible; the handle under the cursor
// gets a white over-stroke. `rotation` / `scale` may be NULL.

#include "mge.h"
#include "mge_gl.h"
#include "mge_math.h"

#include <math.h>
#include <stddef.h>

#ifndef GZ_GRAB_PX
    #define GZ_GRAB_PX 15.0f
#endif

#define GZ_X   CLITERAL(Color){ 232, 72, 72, 255 }
#define GZ_Y   CLITERAL(Color){ 76, 200, 100, 255 }
#define GZ_Z   CLITERAL(Color){ 72, 140, 236, 255 }
#define GZ_HOT CLITERAL(Color){ 255, 255, 255, 255 }

enum { H_NONE = -1, H_X = 0, H_Y = 1, H_Z = 2, H_UNIFORM = 3, H_CENTER = 4 };

static GizmoMode s_mode = GIZMO_TRANSLATE;
static GizmoSpace s_space = GIZMO_WORLD;
static Vector3 s_snap = { 0.5f, 15.0f, 0.25f }; // move units, rotate degrees, scale step
static int s_drag = H_NONE;
static Vector3 s_startPos, s_startRot, s_startScale;
static Vector3 s_dragAxis;      // world direction of the grabbed axis, captured at drag start
static Vector2 s_startMouse;
static float s_startAngle;

// gizmo axis frame for this call -- world axes, or the object's own (local space)
static Vector3 s_ax[3], s_pu[3], s_pv[3];

void Mge_SetGizmoMode(GizmoMode mode) { s_mode = mode; }
GizmoMode Mge_GetGizmoMode(void) { return s_mode; }
void Mge_SetGizmoSpace(GizmoSpace space) { s_space = space; }
GizmoSpace Mge_GetGizmoSpace(void) { return s_space; }

void Mge_SetGizmoSnap(float move, float rotateDeg, float scale)
{
    s_snap = (Vector3){ move, rotateDeg, scale };
}
void Mge_GetGizmoSnap(float* move, float* rotateDeg, float* scale)
{
    if (move != NULL) *move = s_snap.x;
    if (rotateDeg != NULL) *rotateDeg = s_snap.y;
    if (scale != NULL) *scale = s_snap.z;
}

// ---- small helpers ----

static Vector3 axis_vec(int a) { return s_ax[a]; }
static void axis_perp(int a, Vector3* u, Vector3* v) { *u = s_pu[a]; *v = s_pv[a]; }
static Color axis_col(int a) { return (a == H_X) ? GZ_X : (a == H_Y) ? GZ_Y : GZ_Z; }
static Vector3 add3(Vector3 a, Vector3 b) { return (Vector3){ a.x + b.x, a.y + b.y, a.z + b.z }; }
static Vector3 mul3(Vector3 a, float s) { return (Vector3){ a.x * s, a.y * s, a.z * s }; }
static float dot3(Vector3 a, Vector3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static Vector3 rot_dir(Vector3 v, Matrix R)
{
    Vector4 r = Vector4_Transform((Vector4){ v.x, v.y, v.z, 0.0f }, R);
    return (Vector3){ r.x, r.y, r.z };
}
static void perp_basis(Vector3 n, Vector3* u, Vector3* v)
{
    Vector3 ref = (fabsf(n.y) < 0.9f) ? (Vector3){ 0, 1, 0 } : (Vector3){ 1, 0, 0 };
    *u = Vector3Normalize(Vector3Cross(ref, n));
    *v = Vector3Cross(n, *u);
}

// set s_ax / s_pu / s_pv for this frame (world, or object-local when in local
// space -- and scale is always local since world non-uniform scale is not
// representable by `size`).
static void set_axes(const Vector3* rotationDeg)
{
    static const Vector3 world[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
    bool local = rotationDeg != NULL && (s_space == GIZMO_LOCAL || s_mode == GIZMO_SCALE)
        && (rotationDeg->x != 0.0f || rotationDeg->y != 0.0f || rotationDeg->z != 0.0f);
    Matrix R = local
        ? Matrix_RotateXYZ((Vector3){ rotationDeg->x * DEG2RAD, rotationDeg->y * DEG2RAD, rotationDeg->z * DEG2RAD })
        : Matrix_Identity();
    for (int a = 0; a < 3; a++) {
        s_ax[a] = local ? rot_dir(world[a], R) : world[a];
        perp_basis(s_ax[a], &s_pu[a], &s_pv[a]);
    }
}

static Vector3 to_camera(Vector3 from, Camera3D cam)
{
    return Vector3Normalize((Vector3){ cam.position.x - from.x, cam.position.y - from.y, cam.position.z - from.z });
}

static float dist_seg(Vector2 p, Vector2 a, Vector2 b)
{
    Vector2 ab = { b.x - a.x, b.y - a.y };
    Vector2 ap = { p.x - a.x, p.y - a.y };
    float len2 = ab.x * ab.x + ab.y * ab.y;
    float t = (len2 > 0.0f) ? (ap.x * ab.x + ap.y * ab.y) / len2 : 0.0f;
    t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;
    Vector2 c = { a.x + ab.x * t, a.y + ab.y * t };
    return sqrtf((p.x - c.x) * (p.x - c.x) + (p.y - c.y) * (p.y - c.y));
}

// ---- drawing (all inside a depth-disabled overlay) ----

// a thin box from `c` along axis `a` -- oriented, so it follows local axes too
static void axis_box(Vector3 c, int a, float len, float half, Color col)
{
    Vector3 A = s_ax[a], U = s_pu[a], V = s_pv[a];
    Vector3 e = add3(c, mul3(A, len));
    Vector3 q0[4] = {
        add3(add3(c, mul3(U, half)), mul3(V, half)),
        add3(add3(c, mul3(U, -half)), mul3(V, half)),
        add3(add3(c, mul3(U, -half)), mul3(V, -half)),
        add3(add3(c, mul3(U, half)), mul3(V, -half)),
    };
    Vector3 q1[4];
    for (int i = 0; i < 4; i++)
        q1[i] = add3(q0[i], mul3(A, len));

    MgeGL_Begin(MGEGL_TRIANGLES);
    MgeGL_Color4ub(col.r, col.g, col.b, col.a);
    for (int i = 0; i < 4; i++) {
        int j = (i + 1) % 4;
        Vector3 p[6] = { q0[i], q0[j], q1[j], q0[i], q1[j], q1[i] };
        for (int k = 0; k < 6; k++)
            MgeGL_Vertex3f(p[k].x, p[k].y, p[k].z);
    }
    // end caps
    Vector3 cap[12] = { c, q0[0], q0[1], c, q0[1], q0[2], c, q0[2], q0[3], c, q0[3], q0[0] };
    for (int k = 0; k < 12; k++)
        MgeGL_Vertex3f(cap[k].x, cap[k].y, cap[k].z);
    Vector3 cap2[12] = { e, q1[0], q1[1], e, q1[1], q1[2], e, q1[2], q1[3], e, q1[3], q1[0] };
    for (int k = 0; k < 12; k++)
        MgeGL_Vertex3f(cap2[k].x, cap2[k].y, cap2[k].z);
    MgeGL_End();
}

// `base` sits at the end of the shaft; the tip points `h` further out
static void arrow_head(Vector3 base, int a, float h, float r, Color col)
{
    Vector3 A = axis_vec(a), U, V;
    axis_perp(a, &U, &V);
    Vector3 apex = add3(base, mul3(A, h));
    Vector3 bc[4] = {
        add3(add3(base, mul3(U, r)), mul3(V, r)),
        add3(add3(base, mul3(U, -r)), mul3(V, r)),
        add3(add3(base, mul3(U, -r)), mul3(V, -r)),
        add3(add3(base, mul3(U, r)), mul3(V, -r)),
    };
    MgeGL_Begin(MGEGL_TRIANGLES);
    MgeGL_Color4ub(col.r, col.g, col.b, col.a);
    for (int i = 0; i < 4; i++) {
        Vector3 p0 = bc[i], p1 = bc[(i + 1) % 4];
        MgeGL_Vertex3f(apex.x, apex.y, apex.z);
        MgeGL_Vertex3f(p0.x, p0.y, p0.z);
        MgeGL_Vertex3f(p1.x, p1.y, p1.z);
    }
    MgeGL_End();
}

// A full-circle band, but only the segments whose outward direction faces the
// camera are drawn -- so an edge-on ring shows the near arc and a face-on ring
// shows the whole circle (like Unreal's rotate gizmo).
static void ring(Vector3 c, int a, float radius, Camera3D cam, float bandFrac, Color col)
{
    Vector3 U, V;
    axis_perp(a, &U, &V);
    Vector3 toCam = to_camera(c, cam);
    const int N = 64;
    const float band = radius * bandFrac;
    const float inner = radius - band, outer = radius + band;

    MgeGL_Begin(MGEGL_TRIANGLES);
    MgeGL_Color4ub(col.r, col.g, col.b, col.a);
    for (int i = 0; i < N; i++) {
        float a0 = (float)i / N * 6.2831853f, a1 = (float)(i + 1) / N * 6.2831853f;
        float am = (a0 + a1) * 0.5f;
        Vector3 dm = add3(mul3(U, cosf(am)), mul3(V, sinf(am))); // segment's outward normal
        if (dot3(dm, toCam) < -0.12f)
            continue; // faces away from the camera
        Vector3 i0 = add3(c, add3(mul3(U, cosf(a0) * inner), mul3(V, sinf(a0) * inner)));
        Vector3 i1 = add3(c, add3(mul3(U, cosf(a1) * inner), mul3(V, sinf(a1) * inner)));
        Vector3 o0 = add3(c, add3(mul3(U, cosf(a0) * outer), mul3(V, sinf(a0) * outer)));
        Vector3 o1 = add3(c, add3(mul3(U, cosf(a1) * outer), mul3(V, sinf(a1) * outer)));
        MgeGL_Vertex3f(i0.x, i0.y, i0.z);
        MgeGL_Vertex3f(o0.x, o0.y, o0.z);
        MgeGL_Vertex3f(o1.x, o1.y, o1.z);
        MgeGL_Vertex3f(i0.x, i0.y, i0.z);
        MgeGL_Vertex3f(o1.x, o1.y, o1.z);
        MgeGL_Vertex3f(i1.x, i1.y, i1.z);
    }
    MgeGL_End();
}

// ---- hot-handle picking ----

static int hot_translate_or_scale(Vector3 c, float len, Camera3D cam, int w, int h, Vector2 m)
{
    Vector2 s0 = Mge_GetWorldToScreenEx(c, cam, w, h);
    float best = GZ_GRAB_PX;
    int hot = H_NONE;
    for (int a = 0; a < 3; a++) {
        // reach past the shaft end to cover the arrow / cube tip
        Vector2 se = Mge_GetWorldToScreenEx(add3(c, mul3(axis_vec(a), len * 1.22f)), cam, w, h);
        float d = dist_seg(m, s0, se);
        if (d < best) { best = d; hot = a; }
    }
    // the centre handle: uniform scale, or a screen-plane move in translate mode
    float dc = sqrtf((s0.x - m.x) * (s0.x - m.x) + (s0.y - m.y) * (s0.y - m.y));
    if (dc < GZ_GRAB_PX && dc < best)
        hot = (s_mode == GIZMO_SCALE) ? H_UNIFORM : H_CENTER;
    return hot;
}

static int hot_rotate(Vector3 c, float radius, Camera3D cam, int w, int h, Vector2 m)
{
    float best = GZ_GRAB_PX;
    int hot = H_NONE;
    for (int a = 0; a < 3; a++) {
        Vector3 U, V;
        axis_perp(a, &U, &V);
        Vector2 prev = { 0, 0 };
        for (int i = 0; i <= 48; i++) {
            float ang = (float)i / 48 * 6.2831853f;
            Vector3 wp = add3(c, add3(mul3(U, cosf(ang) * radius), mul3(V, sinf(ang) * radius)));
            Vector2 sp = Mge_GetWorldToScreenEx(wp, cam, w, h);
            if (i > 0) {
                float d = dist_seg(m, prev, sp);
                if (d < best) { best = d; hot = a; }
            }
            prev = sp;
        }
    }
    return hot;
}

// ---- public ----

bool Mge_Gizmo3D(Vector3* position, Vector3* rotation, Vector3* scale, Camera3D camera, float size)
{
    if (position == NULL)
        return false;
    if (size < 0.01f)
        size = 1.0f;

    Vector2 m = GetMousePosition();
    int w = Mge_GetScreenWidth();
    int h = Mge_GetScreenHeight();
    Vector3 c = *position;
    float len = size;
    float thick = size * 0.045f;

    // only a position given (a light, or a multi-select group pivot) -> move-only,
    // whatever the toolbar mode is. Restored before returning.
    GizmoMode savedMode = s_mode;
    if (rotation == NULL && scale == NULL)
        s_mode = GIZMO_TRANSLATE;

    // hold Ctrl while dragging to snap to the configured increments
    bool snap = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

    set_axes(rotation); // world axes, or the object's own (local space / scale)

    // which handle is under the cursor this frame
    int hot = (s_drag != H_NONE) ? s_drag
        : (s_mode == GIZMO_ROTATE) ? hot_rotate(c, len, camera, w, h, m)
                                   : hot_translate_or_scale(c, len, camera, w, h, m);

    // start / stop a drag
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hot != H_NONE) {
        s_drag = hot;
        s_startPos = *position;
        s_startRot = rotation ? *rotation : (Vector3){ 0, 0, 0 };
        s_startScale = scale ? *scale : (Vector3){ 1, 1, 1 };
        s_startMouse = m;
        s_dragAxis = (hot >= 0 && hot < 3) ? s_ax[hot] : (Vector3){ 1, 0, 0 };
        Vector2 sc = Mge_GetWorldToScreenEx(c, camera, w, h);
        s_startAngle = atan2f(sc.y - m.y, m.x - sc.x);
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        s_drag = H_NONE;

    // apply the drag
    if (s_drag != H_NONE && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (s_mode == GIZMO_ROTATE && rotation) {
            Vector2 sc = Mge_GetWorldToScreenEx(s_startPos, camera, w, h);
            // screen y is down -> flip it so dragging counter-clockwise reads as +
            float now = atan2f(sc.y - m.y, m.x - sc.x);
            float rad = now - s_startAngle;
            // consistent sense whichever side of the ring you view it from
            if (dot3(s_dragAxis, to_camera(s_startPos, camera)) < 0.0f)
                rad = -rad;
            if (snap && s_snap.y > 0.0f) {
                float step = s_snap.y * (float)DEG2RAD;
                rad = roundf(rad / step) * step;
            }
            Matrix newR = Matrix_Multiply(
                Matrix_RotateXYZ((Vector3){ s_startRot.x * DEG2RAD, s_startRot.y * DEG2RAD, s_startRot.z * DEG2RAD }),
                Matrix_Rotate(s_dragAxis, rad));
            Vector3 e = Matrix_ToEulerXYZ(newR);
            *rotation = (Vector3){ e.x * (float)RAD2DEG, e.y * (float)RAD2DEG, e.z * (float)RAD2DEG };
        } else if (s_drag == H_CENTER) {
            // screen-plane move: solve for the world offset that matches the drag
            Vector3 fwd = Vector3Normalize(camera.target);
            Vector3 right = Vector3Normalize(Vector3Cross(fwd, camera.up));
            Vector3 up = Vector3Cross(right, fwd);
            Vector2 o = Mge_GetWorldToScreenEx(s_startPos, camera, w, h);
            Vector2 sr = Mge_GetWorldToScreenEx(add3(s_startPos, right), camera, w, h);
            Vector2 su = Mge_GetWorldToScreenEx(add3(s_startPos, up), camera, w, h);
            sr = (Vector2){ sr.x - o.x, sr.y - o.y };
            su = (Vector2){ su.x - o.x, su.y - o.y };
            float det = sr.x * su.y - sr.y * su.x;
            if (fabsf(det) > 1e-4f) {
                Vector2 tot = { m.x - s_startMouse.x, m.y - s_startMouse.y };
                float dr = (tot.x * su.y - tot.y * su.x) / det;
                float du = (sr.x * tot.y - sr.y * tot.x) / det;
                if (snap && s_snap.x > 0.0f) {
                    dr = roundf(dr / s_snap.x) * s_snap.x;
                    du = roundf(du / s_snap.x) * s_snap.x;
                }
                *position = add3(s_startPos, add3(mul3(right, dr), mul3(up, du)));
            }
        } else {
            // translate / scale: project the mouse motion onto the axis' screen direction
            Vector3 axis = (s_drag == H_UNIFORM) ? (Vector3){ 1, 1, 1 } : axis_vec(s_drag);
            Vector3 refAxis = (s_drag == H_UNIFORM) ? (Vector3){ 1, 0, 0 } : axis;
            Vector2 a0 = Mge_GetWorldToScreenEx(s_startPos, camera, w, h);
            Vector2 a1 = Mge_GetWorldToScreenEx(add3(s_startPos, mul3(refAxis, len)), camera, w, h);
            Vector2 sd = { a1.x - a0.x, a1.y - a0.y };
            float sd2 = sd.x * sd.x + sd.y * sd.y;
            if (sd2 > 0.001f) {
                Vector2 tot = { m.x - s_startMouse.x, m.y - s_startMouse.y };
                float t = (tot.x * sd.x + tot.y * sd.y) / sd2; // fraction of `len`

                if (s_mode == GIZMO_SCALE && scale) {
                    float f = 1.0f + t * 1.5f;
                    if (f < 0.05f) f = 0.05f;
                    float step = s_snap.z;
                    if (s_drag == H_UNIFORM) {
                        Vector3 v = mul3(s_startScale, f);
                        if (snap && step > 0.0f) {
                            v.x = roundf(v.x / step) * step; if (v.x < step) v.x = step;
                            v.y = roundf(v.y / step) * step; if (v.y < step) v.y = step;
                            v.z = roundf(v.z / step) * step; if (v.z < step) v.z = step;
                        }
                        *scale = v;
                    } else {
                        *scale = s_startScale;
                        float* comp = (s_drag == H_X) ? &scale->x : (s_drag == H_Y) ? &scale->y : &scale->z;
                        float start = (s_drag == H_X) ? s_startScale.x : (s_drag == H_Y) ? s_startScale.y : s_startScale.z;
                        float val = start * f;
                        if (snap && step > 0.0f) { val = roundf(val / step) * step; if (val < step) val = step; }
                        *comp = val;
                    }
                } else { // translate
                    float step = t * len;
                    if (snap && s_snap.x > 0.0f)
                        step = roundf(step / s_snap.x) * s_snap.x;
                    *position = add3(s_startPos, mul3(axis, step));
                }
            }
        }
    }

    // ---- draw, on top of everything ----
    MgeGL_Draw();
    MgeGL_SetDepthFunc(DEPTH_ALWAYS);
    MgeGL_SetDepthMask(false);
    MgeGL_SetBlend(true);

    c = *position;
    if (s_mode == GIZMO_ROTATE) {
        for (int a = 0; a < 3; a++) {
            Color col = (hot == a) ? GZ_HOT : axis_col(a);
            col.a = (hot == a) ? 255 : 210;
            ring(c, a, len, camera, (hot == a) ? 0.055f : 0.032f, col);
        }
    } else {
        for (int a = 0; a < 3; a++) {
            Color col = (hot == a) ? GZ_HOT : axis_col(a);
            Vector3 end = add3(c, mul3(axis_vec(a), len)); // shaft end
            axis_box(c, a, len, (hot == a) ? thick * 0.8f : thick * 0.5f, col); // half-width
            if (s_mode == GIZMO_SCALE) {
                Vector3 tip = add3(end, mul3(axis_vec(a), size * 0.09f)); // clear of the shaft
                Draw_Cube(tip, (Vector3){ size * 0.15f, size * 0.15f, size * 0.15f }, col);
            } else {
                arrow_head(end, a, size * 0.30f, thick * 3.2f, col); // base at end, tip further out
            }
        }
        if (s_mode == GIZMO_SCALE) {
            Color col = (hot == H_UNIFORM) ? GZ_HOT : (Color){ 210, 210, 210, 255 };
            Draw_Cube(c, (Vector3){ size * 0.15f, size * 0.15f, size * 0.15f }, col);
        } else { // translate: a centre ball -- drag it to move on the view plane
            Color col = (hot == H_CENTER) ? GZ_HOT : (Color){ 225, 225, 230, 255 };
            Draw_Sphere(c, size * 0.10f, col);
        }
    }

    MgeGL_SetBlend(false);
    MgeGL_Draw();
    MgeGL_SetDepthFunc(DEPTH_LESS);
    MgeGL_SetDepthMask(true);

    s_mode = savedMode;
    return s_drag != H_NONE;
}
