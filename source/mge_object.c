// Scene objects + a mouse-driven translation gizmo.
//
// Call Mge_ManipulateObjects2D/3D() once per frame while the cursor is enabled:
// left-click an object to select it, then drag a gizmo arrow to move along that
// axis, or drag the body to move freely.

#include "mge.h"
#include "mge_math.h"

#include <math.h>
#include <stddef.h>

#ifndef MGE_GIZMO_GRAB_PX
    #define MGE_GIZMO_GRAB_PX 12.0f // pixel tolerance for grabbing an axis arrow
#endif

// selection outline drawn by Mge_DrawObject for selected objects
#ifndef MGE_SELECT_OUTLINE_COLOR
    #define MGE_SELECT_OUTLINE_COLOR CLITERAL(Color) { 255, 145, 40, 255 } // bold orange
#endif
#ifndef MGE_SELECT_OUTLINE_3D
    #define MGE_SELECT_OUTLINE_3D 0.16f  // world units added to the box extents
#endif
#ifndef MGE_SELECT_OUTLINE_2D
    #define MGE_SELECT_OUTLINE_2D 12.0f  // pixels added to the rect extents
#endif

// ----- construction -----

Object Mge_MakeObject2D(float x, float y, float w, float h, Color color)
{
    Object o = { 0 };
    o.kind = OBJECT_2D;
    o.active = true;
    o.transform.position = (Vector3){ x, y, 0.0f };
    o.transform.rotation = Quaternion_Identity();
    o.transform.scale = (Vector3){ w, h, 0.0f };
    o.transform.parent = -1;
    o.material = Mge_DefaultMaterial();
    o.material.maps[MATERIAL_MAP_DIFFUSE].color = color;
    return o;
}

Object Mge_MakeShape3D(PrimitiveKind primitive, Vector3 position, Vector3 size, Color color)
{
    Object o = { 0 };
    o.kind = OBJECT_3D;
    o.active = true;
    o.primitive = primitive;
    o.transform.position = position;
    o.transform.rotation = Quaternion_Identity();
    o.transform.scale = size;
    o.transform.parent = -1;
    o.material = Mge_DefaultMaterial();
    o.material.maps[MATERIAL_MAP_DIFFUSE].color = color;
    return o;
}

Object Mge_MakeObject3D(Vector3 position, Vector3 size, Color color)
{
    return Mge_MakeShape3D(PRIM_CUBE, position, size, color);
}

// ----- drawing -----

// PRIM_POLYGON local points -> world space (position + rotation*(point*scale)).
// Returns the point count actually written (clamped).
static int poly_world(const Object* o, Vector3 out[MGE_MAX_POLY_POINTS])
{
    int n = o->polyCount;
    if (n > MGE_MAX_POLY_POINTS)
        n = MGE_MAX_POLY_POINTS;
    for (int k = 0; k < n; k++) {
        Vector3 local = Vector3_Multiply(o->poly[k], o->transform.scale);
        out[k] = Vector3_Add(o->transform.position, Quaternion_RotateVector3(o->transform.rotation, local));
    }
    return n;
}

// true when the primitive draws as a filled surface (gets a shadow + a stencil
// selection shell); false for wireframe / arrow / line / polygon-outline.
bool Mge_PrimitiveIsSolid(Object obj)
{
    if (obj.kind != OBJECT_3D || obj.wireframe || obj.primitive == PRIM_ARROW)
        return false;
    if (obj.primitive == PRIM_POLYGON)
        return obj.polyCount >= 3;
    return true;
}

// The geometry for a 3D object's primitive, in a chosen colour. Shared by the
// lit draw and (via Mge_DrawObjectOutline) the stencil outline.
void Mge_DrawPrimitive(Object obj, Color color)
{
    if (obj.kind == OBJECT_CAMERA)
        return; // a camera marker has no solid geometry (no shadow, no lit pass)
    const Vector3 p = obj.transform.position, s = obj.transform.scale;
    const Quaternion r = obj.transform.rotation;
    const bool wire = obj.wireframe;
    switch (obj.primitive) {
    case PRIM_SPHERE:
        if (wire)
            Draw_SphereWiresEx(p, s.x * 0.5f, 8, 14, color);
        else
            Draw_SphereEx(p, s.x * 0.5f, 16, 24, color);
        break;
    case PRIM_PLANE:
        if (wire)
            Draw_Quad3DWires(p, (Vector2){ s.x, s.z }, r, color);
        else
            Draw_Quad3D(p, (Vector2){ s.x, s.z }, r, color);
        break;
    case PRIM_ARROW: {
        Vector3 dir = Quaternion_RotateVector3(r, (Vector3){ 1.0f, 0.0f, 0.0f });
        Draw_Arrow3D(p, Vector3_Add(p, Vector3_Scale(dir, s.x)), color);
        break;
    }
    case PRIM_POLYGON: {
        Vector3 w[MGE_MAX_POLY_POINTS];
        int n = poly_world(&obj, w);
        if (wire || n < 3)
            Draw_Polygon3DWires(w, n, n >= 3, color);
        else
            Draw_Polygon3D(w, n, obj.polyStrip, color);
        break;
    }
    case PRIM_CUBE:
    default:
        if (wire)
            Draw_CubeWiresEx(p, s, r, color);
        else
            Draw_CubeEx(p, s, r, color);
        break;
    }
}

// Unit forward vector for an OBJECT_CAMERA: its orientation applied to local -Z
// (identity looks toward -Z, the standard camera convention).
Vector3 Mge_CameraObjectForward(Quaternion rotation)
{
    Vector3 f = Quaternion_RotateVector3(rotation, (Vector3){ 0.0f, 0.0f, -1.0f });
    float len = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
    if (len > 1e-6f) { f.x /= len; f.y /= len; f.z /= len; }
    return f;
}

void Mge_DrawObject(Object obj)
{
    if (!obj.active)
        return;

    if (obj.kind == OBJECT_CAMERA) {
        const Vector3 p = obj.transform.position;
        Vector3 fwd = Mge_CameraObjectForward(obj.transform.rotation);
        Color body = obj.selected ? MGE_SELECT_OUTLINE_COLOR : CLITERAL(Color){ 150, 200, 255, 255 };
        Draw_CubeWires(p, (Vector3){ 0.7f, 0.5f, 0.5f }, body);
        Draw_CubeWires((Vector3){ p.x + fwd.x * 0.45f, p.y + fwd.y * 0.45f, p.z + fwd.z * 0.45f },
            (Vector3){ 0.35f, 0.35f, 0.2f }, body);
        // a representative view frustum (60 deg / 16:9, as Scene_MainCamera builds it)
        Camera3D cam = { .position = p, .target = fwd, .up = { 0.0f, 1.0f, 0.0f },
            .fovy = 60.0f, .projection = CAMERA_PERSPECTIVE };
        Draw_CameraFrustum(cam, 16.0f / 9.0f, 0.12f, 2.4f, body);
        return;
    }

    const Vector3 p = obj.transform.position, s = obj.transform.scale;
    if (obj.kind == OBJECT_2D) {
        Rectangle r = { p.x - s.x * 0.5f, p.y - s.y * 0.5f, s.x, s.y };
        Draw_RectangleRec(r, obj.material.maps[MATERIAL_MAP_DIFFUSE].color);
        if (obj.selected)
            Mge_DrawObjectOutline(obj, MGE_SELECT_OUTLINE_2D, MGE_SELECT_OUTLINE_COLOR);
    } else {
        Mge_SetMaterial(obj.material); // no-op unless Mge_BeginLighting3D is active
        bool solid = Mge_PrimitiveIsSolid(obj);
        // wire / arrow / line have no fillable interior for a stencil shell --
        // just recolour them when selected (like the camera marker)
        Color c = (obj.selected && !solid) ? MGE_SELECT_OUTLINE_COLOR
                                           : obj.material.maps[MATERIAL_MAP_DIFFUSE].color;
        Mge_DrawPrimitive(obj, c);
        if (obj.selected && solid)
            Mge_DrawObjectOutline(obj, MGE_SELECT_OUTLINE_3D, MGE_SELECT_OUTLINE_COLOR);
    }
}

void Mge_DrawObjectGizmo2D(Object obj, float axisLength)
{
    Vector2 o = { obj.transform.position.x, obj.transform.position.y };
    Draw_Arrow(o, (Vector2){ o.x + axisLength, o.y }, 12.0f, RED);   // +X (right)
    Draw_Arrow(o, (Vector2){ o.x, o.y - axisLength }, 12.0f, GREEN); // +Y (up on screen)
}

// ----- camera / projection -----

Matrix Mge_GetCameraViewMatrix(Camera3D camera)
{
    return MatrixLookAt(camera.position, Vector3_Add(camera.position, camera.target), camera.up);
}

Matrix Mge_GetCameraProjectionMatrix(Camera3D camera, float aspect)
{
    if (camera.projection == CAMERA_ORTHOGRAPHIC) {
        double top = camera.fovy / 2.0;
        double right = top * (double)aspect;
        return MatrixOrtho(-right, right, -top, top, MGE_CULL_DISTANCE_NEAR, MGE_CULL_DISTANCE_FAR);
    }
    return MatrixPerspective((double)camera.fovy * DEG2RAD, (double)aspect,
        MGE_CULL_DISTANCE_NEAR, MGE_CULL_DISTANCE_FAR);
}

Vector2 Mge_GetWorldToScreenEx(Vector3 position, Camera3D camera, int screenWidth, int screenHeight)
{
    float aspect = (screenHeight != 0) ? (float)screenWidth / (float)screenHeight : 1.0f;
    Matrix view = Mge_GetCameraViewMatrix(camera);
    Matrix proj = Mge_GetCameraProjectionMatrix(camera, aspect);

    Vector4 clip = Vector4_Transform((Vector4){ position.x, position.y, position.z, 1.0f }, view);
    clip = Vector4_Transform(clip, proj);

    if (fabsf(clip.w) < 1e-6f)
        return (Vector2){ -1.0f, -1.0f };

    Vector3 ndc = { clip.x / clip.w, clip.y / clip.w, clip.z / clip.w };
    Vector2 s;
    s.x = (ndc.x * 0.5f + 0.5f) * (float)screenWidth;
    s.y = (0.5f - ndc.y * 0.5f) * (float)screenHeight;
    return s;
}

Vector2 Mge_GetWorldToScreen(Vector3 position, Camera3D camera)
{
    return Mge_GetWorldToScreenEx(position, camera, Mge_GetScreenWidth(), Mge_GetScreenHeight());
}

// ----- picking helpers -----

static float DistPointSegment(Vector2 p, Vector2 a, Vector2 b)
{
    Vector2 ab = { b.x - a.x, b.y - a.y };
    Vector2 ap = { p.x - a.x, p.y - a.y };
    float len2 = ab.x * ab.x + ab.y * ab.y;
    float t = (len2 > 0.0f) ? (ap.x * ab.x + ap.y * ab.y) / len2 : 0.0f;
    if (t < 0.0f)
        t = 0.0f;
    if (t > 1.0f)
        t = 1.0f;
    Vector2 c = { a.x + ab.x * t, a.y + ab.y * t };
    float dx = p.x - c.x, dy = p.y - c.y;
    return sqrtf(dx * dx + dy * dy);
}

static bool PointInObject2D(Vector2 p, Object o)
{
    float hx = fabsf(o.transform.scale.x) * 0.5f, hy = fabsf(o.transform.scale.y) * 0.5f;
    return (p.x >= o.transform.position.x - hx) && (p.x <= o.transform.position.x + hx) &&
        (p.y >= o.transform.position.y - hy) && (p.y <= o.transform.position.y + hy);
}

// ----- manipulation -----

enum { DRAG_NONE = -1, DRAG_X = 0, DRAG_Y = 1, DRAG_Z = 2, DRAG_BODY = 3 };

static int g_active = -1;
static int g_drag = DRAG_NONE;
static Vector3 g_dragStartPos;
static Vector2 g_dragStartMouse;

static void SelectOnly(Object* objs, int count, int idx)
{
    for (int i = 0; i < count; i++)
        objs[i].selected = (i == idx);
    g_active = idx;
}

void Mge_ClearSelection(Object* objects, int count)
{
    if (objects != NULL) {
        for (int i = 0; i < count; i++)
            objects[i].selected = false;
    }
    g_active = -1;
    g_drag = DRAG_NONE;
}

int Mge_GetSelectedObject(void)
{
    return g_active;
}

// Select an object programmatically -- same effect as clicking it, so the next
// Mge_ManipulateObjects3D() call will drag its gizmo. `index` out of range (or
// -1) clears the selection.
void Mge_SetSelectedObject(Object* objects, int count, int index)
{
    if (objects == NULL || count <= 0)
        return;

    g_drag = DRAG_NONE;
    if (index < 0 || index >= count)
        Mge_ClearSelection(objects, count);
    else
        SelectOnly(objects, count, index);
}

int Mge_ManipulateObjects2D(Object* objects, int count, float axisLength)
{
    if (objects == NULL || count <= 0)
        return -1;

    Vector2 m = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        g_drag = DRAG_NONE;

        // 1. axis arrows of the currently selected object
        if (g_active >= 0 && g_active < count) {
            Vector3 p = objects[g_active].transform.position;
            Vector2 origin = { p.x, p.y };
            Vector2 xEnd = { origin.x + axisLength, origin.y };
            Vector2 yEnd = { origin.x, origin.y - axisLength };
            if (DistPointSegment(m, origin, xEnd) <= MGE_GIZMO_GRAB_PX)
                g_drag = DRAG_X;
            else if (DistPointSegment(m, origin, yEnd) <= MGE_GIZMO_GRAB_PX)
                g_drag = DRAG_Y;
        }

        // 2. otherwise pick an object body (topmost = last in the array)
        if (g_drag == DRAG_NONE) {
            int hit = -1;
            for (int i = count - 1; i >= 0; i--) {
                if (objects[i].kind == OBJECT_2D && PointInObject2D(m, objects[i])) {
                    hit = i;
                    break;
                }
            }
            SelectOnly(objects, count, hit);
            if (hit >= 0)
                g_drag = DRAG_BODY;
        }

        if (g_drag != DRAG_NONE && g_active >= 0) {
            g_dragStartPos = objects[g_active].transform.position;
            g_dragStartMouse = m;
        }
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && g_drag != DRAG_NONE && g_active >= 0 && g_active < count) {
        Vector3* pos = &objects[g_active].transform.position;
        Vector2 d = { m.x - g_dragStartMouse.x, m.y - g_dragStartMouse.y };
        if (g_drag == DRAG_X) {
            pos->x = g_dragStartPos.x + d.x;
        } else if (g_drag == DRAG_Y) {
            pos->y = g_dragStartPos.y + d.y; // screen +y is down: dragging the up-arrow up (d.y < 0) moves the object up
        } else if (g_drag == DRAG_BODY) {
            pos->x = g_dragStartPos.x + d.x;
            pos->y = g_dragStartPos.y + d.y;
        }
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        g_drag = DRAG_NONE;

    return g_active;
}

// 3D picking: on left-click, cast a ray through the cursor and select the
// nearest object it actually hits (its primitive geometry), or clear on a miss.
int Mge_PickObject3D(Object* objects, int count, Camera3D camera)
{
    if (objects == NULL || count <= 0)
        return -1;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        RayHit h = Mge_RaycastObjects(Mge_GetMouseRay(camera), objects, count);
        SelectOnly(objects, count, h.index);
    }

    return g_active;
}
