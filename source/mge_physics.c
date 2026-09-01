// Physics: raycasting against primitives and scene objects, plus debug draw.
//
// A ray is an origin + a direction; every Mge_Raycast* normalises the direction
// internally and reports the nearest FORWARD hit (distance >= 0) as a RayHit
// whose `normal` faces back toward the ray. Screen->ray unprojection here is
// self-contained (camera basis + fov) so it needs only mge_math + libm.
//
//   Ray r = Mge_GetMouseRay(camera);
//   RayHit h = Mge_RaycastObjects(r, objects, count);
//   if (h.hit) objects[h.index].selected = true;

#include "mge.h"
#include "mge_math.h"

#include <math.h>

// --- small vector helpers (keep the intersection code readable) ------------
static Vector3 v3add(Vector3 a, Vector3 b) { return Vector3_Add(a, b); }
static Vector3 v3sub(Vector3 a, Vector3 b) { return Vector3_Subtract(a, b); }
static Vector3 v3scale(Vector3 v, float s) { return Vector3_Scale(v, s); }
static float   v3dot(Vector3 a, Vector3 b) { return Vector3_DotProduct(a, b); }
static Vector3 v3cross(Vector3 a, Vector3 b) { return Vector3Cross(a, b); }
static Vector3 v3norm(Vector3 v) { return Vector3Normalize(v); }

static const RayHit RAYHIT_MISS = { false, 0.0f, { 0, 0, 0 }, { 0, 0, 0 }, -1 };

// flip `n` so it faces back toward a ray travelling along `dir`
static Vector3 face_ray(Vector3 n, Vector3 dir)
{
    return (v3dot(n, dir) > 0.0f) ? v3scale(n, -1.0f) : n;
}

static bool has_rotation(Quaternion q)
{
    return q.x != 0.0f || q.y != 0.0f || q.z != 0.0f;
}

// -------------------------------------------------------------------------
// primitives
// -------------------------------------------------------------------------

RayHit Mge_RaycastSphere(Ray ray, Vector3 center, float radius)
{
    Vector3 d = v3norm(ray.direction);
    Vector3 oc = v3sub(ray.position, center);

    float b = v3dot(oc, d);
    float c = v3dot(oc, oc) - radius * radius;
    float disc = b * b - c;
    if (disc < 0.0f)
        return RAYHIT_MISS;

    float s = sqrtf(disc);
    float t = -b - s;
    if (t < 0.0f)
        t = -b + s; // origin is inside the sphere
    if (t < 0.0f)
        return RAYHIT_MISS;

    RayHit h = RAYHIT_MISS;
    h.hit = true;
    h.distance = t;
    h.point = v3add(ray.position, v3scale(d, t));
    h.normal = face_ray(v3norm(v3sub(h.point, center)), d);
    return h;
}

RayHit Mge_RaycastAABB(Ray ray, Vector3 min, Vector3 max)
{
    Vector3 d = v3norm(ray.direction);
    const float o[3] = { ray.position.x, ray.position.y, ray.position.z };
    const float dv[3] = { d.x, d.y, d.z };
    const float lo[3] = { min.x, min.y, min.z };
    const float hi[3] = { max.x, max.y, max.z };

    float tmin = -INFINITY, tmax = INFINITY;
    int axis = 0;
    float minSign = 1.0f;

    for (int i = 0; i < 3; i++) {
        float invD = 1.0f / dv[i]; // dv[i] == 0 -> +/-inf, handled by the checks below
        float t0 = (lo[i] - o[i]) * invD;
        float t1 = (hi[i] - o[i]) * invD;
        float sign = -1.0f;
        if (t0 > t1) {
            float tmp = t0;
            t0 = t1;
            t1 = tmp;
            sign = 1.0f;
        }
        if (t0 > tmin) {
            tmin = t0;
            axis = i;
            minSign = sign;
        }
        if (t1 < tmax)
            tmax = t1;
        if (tmax < tmin)
            return RAYHIT_MISS;
    }

    float t = (tmin >= 0.0f) ? tmin : tmax; // start inside the box -> use the exit
    if (t < 0.0f)
        return RAYHIT_MISS;

    RayHit h = RAYHIT_MISS;
    h.hit = true;
    h.distance = t;
    h.point = v3add(ray.position, v3scale(d, t));
    Vector3 n = { 0, 0, 0 };
    float ns = (t == tmin) ? minSign : -minSign;
    if (axis == 0) n.x = ns;
    else if (axis == 1) n.y = ns;
    else n.z = ns;
    h.normal = face_ray(n, d);
    return h;
}

RayHit Mge_RaycastBox(Ray ray, Vector3 center, Vector3 size, Quaternion rotation)
{
    Vector3 half = { size.x * 0.5f, size.y * 0.5f, size.z * 0.5f };

    if (!has_rotation(rotation)) {
        return Mge_RaycastAABB(ray, v3sub(center, half), v3add(center, half));
    }

    Quaternion q = Quaternion_Normalize(rotation);
    Quaternion inv = Quaternion_Conjugate(q);

    Ray local;
    local.position = Quaternion_RotateVector3(inv, v3sub(ray.position, center));
    local.direction = Quaternion_RotateVector3(inv, v3norm(ray.direction));

    RayHit h = Mge_RaycastAABB(local, v3scale(half, -1.0f), half);
    if (!h.hit)
        return h;

    // back to world space
    h.point = v3add(center, Quaternion_RotateVector3(q, h.point));
    h.normal = Quaternion_RotateVector3(q, h.normal);
    return h;
}

RayHit Mge_RaycastPlane(Ray ray, Vector3 point, Vector3 normal)
{
    Vector3 d = v3norm(ray.direction);
    Vector3 n = v3norm(normal);

    float denom = v3dot(d, n);
    if (fabsf(denom) < 1e-6f)
        return RAYHIT_MISS; // parallel

    float t = v3dot(v3sub(point, ray.position), n) / denom;
    if (t < 0.0f)
        return RAYHIT_MISS;

    RayHit h = RAYHIT_MISS;
    h.hit = true;
    h.distance = t;
    h.point = v3add(ray.position, v3scale(d, t));
    h.normal = face_ray(n, d);
    return h;
}

RayHit Mge_RaycastTriangle(Ray ray, Vector3 v0, Vector3 v1, Vector3 v2)
{
    Vector3 d = v3norm(ray.direction);
    Vector3 e1 = v3sub(v1, v0);
    Vector3 e2 = v3sub(v2, v0);

    Vector3 p = v3cross(d, e2);
    float det = v3dot(e1, p);
    if (fabsf(det) < 1e-8f)
        return RAYHIT_MISS; // ray parallel to the triangle

    float invDet = 1.0f / det;
    Vector3 tv = v3sub(ray.position, v0);
    float u = v3dot(tv, p) * invDet;
    if (u < 0.0f || u > 1.0f)
        return RAYHIT_MISS;

    Vector3 qv = v3cross(tv, e1);
    float v = v3dot(d, qv) * invDet;
    if (v < 0.0f || u + v > 1.0f)
        return RAYHIT_MISS;

    float t = v3dot(e2, qv) * invDet;
    if (t < 0.0f)
        return RAYHIT_MISS;

    RayHit h = RAYHIT_MISS;
    h.hit = true;
    h.distance = t;
    h.point = v3add(ray.position, v3scale(d, t));
    h.normal = face_ray(v3norm(v3cross(e1, e2)), d);
    return h;
}

// -------------------------------------------------------------------------
// scene objects
// -------------------------------------------------------------------------

// a finite plane primitive: the XZ quad (width scale.x, length scale.z) centred
// at transform.position, rotated by transform.rotation, normal = local +Y
static RayHit raycast_object_plane(Ray ray, Vector3 center, Vector3 size, Quaternion rot)
{
    Quaternion q = has_rotation(rot) ? Quaternion_Normalize(rot) : Quaternion_Identity();
    Quaternion inv = Quaternion_Conjugate(q);

    Ray local;
    local.position = Quaternion_RotateVector3(inv, v3sub(ray.position, center));
    local.direction = Quaternion_RotateVector3(inv, v3norm(ray.direction));

    if (fabsf(local.direction.y) < 1e-6f)
        return RAYHIT_MISS;

    float t = -local.position.y / local.direction.y;
    if (t < 0.0f)
        return RAYHIT_MISS;

    float hx = local.position.x + local.direction.x * t;
    float hz = local.position.z + local.direction.z * t;
    if (fabsf(hx) > size.x * 0.5f || fabsf(hz) > size.z * 0.5f)
        return RAYHIT_MISS;

    RayHit h = RAYHIT_MISS;
    h.hit = true;
    h.distance = t;
    h.point = v3add(center, Quaternion_RotateVector3(q, (Vector3){ hx, 0.0f, hz }));
    Vector3 n = Quaternion_RotateVector3(q, (Vector3){ 0.0f, 1.0f, 0.0f });
    h.normal = face_ray(n, v3norm(ray.direction));
    return h;
}

// an arrow primitive: from `center` along local +X for `size.x`; picked as a
// slightly fat OBB along its axis
static RayHit raycast_object_arrow(Ray ray, Vector3 center, Vector3 size, Quaternion rot)
{
    Vector3 dir = Quaternion_RotateVector3(rot, (Vector3){ 1.0f, 0.0f, 0.0f });
    Vector3 mid = v3add(center, v3scale(dir, size.x * 0.5f));
    float th = 0.35f;
    return Mge_RaycastBox(ray, mid, (Vector3){ size.x, th, th }, rot);
}

// a polygon primitive: local points (Object.poly[] * scale), reoriented by
// rotation. >=3 points -> the fan/strip triangles; 2 points -> a fat AABB.
static RayHit raycast_object_polygon(Ray ray, const Object* o)
{
    Vector3 c = o->transform.position, s = o->transform.scale;
    Quaternion q = has_rotation(o->transform.rotation) ? Quaternion_Normalize(o->transform.rotation)
                                                       : Quaternion_Identity();
    Quaternion inv = Quaternion_Conjugate(q);

    int n = o->polyCount;
    if (n > MGE_MAX_POLY_POINTS)
        n = MGE_MAX_POLY_POINTS;
    Vector3 lp[MGE_MAX_POLY_POINTS];
    for (int k = 0; k < n; k++)
        lp[k] = (Vector3){ o->poly[k].x * s.x, o->poly[k].y * s.y, o->poly[k].z * s.z };

    Ray local;
    local.position = Quaternion_RotateVector3(inv, v3sub(ray.position, c));
    local.direction = Quaternion_RotateVector3(inv, v3norm(ray.direction));

    RayHit h = RAYHIT_MISS;
    if (n >= 3) {
        for (int i = 2; i < n; i++) {
            RayHit t = o->polyStrip ? Mge_RaycastTriangle(local, lp[i - 2], lp[i - 1], lp[i])
                                    : Mge_RaycastTriangle(local, lp[0], lp[i - 1], lp[i]);
            if (t.hit && (!h.hit || t.distance < h.distance))
                h = t;
        }
    } else if (n == 2) {
        Vector3 lo = { fminf(lp[0].x, lp[1].x) - 0.15f, fminf(lp[0].y, lp[1].y) - 0.15f, fminf(lp[0].z, lp[1].z) - 0.15f };
        Vector3 hi = { fmaxf(lp[0].x, lp[1].x) + 0.15f, fmaxf(lp[0].y, lp[1].y) + 0.15f, fmaxf(lp[0].z, lp[1].z) + 0.15f };
        h = Mge_RaycastAABB(local, lo, hi);
    }
    if (!h.hit)
        return h;
    h.point = v3add(c, Quaternion_RotateVector3(q, h.point));
    h.normal = Quaternion_RotateVector3(q, h.normal);
    return h;
}

RayHit Mge_RaycastObjects(Ray ray, const Object* objects, int count)
{
    RayHit best = RAYHIT_MISS;

    for (int i = 0; i < count; i++) {
        const Object o = objects[i];
        if (!o.active || o.kind == OBJECT_2D)
            continue; // a 2D rect has no ray volume

        Vector3 c = o.transform.position;
        Vector3 s = o.transform.scale;
        Quaternion r = o.transform.rotation;

        RayHit h;
        if (o.kind == OBJECT_CAMERA) {
            h = Mge_RaycastBox(ray, c, (Vector3){ 0.7f, 0.5f, 0.5f }, r); // the marker body
        } else {
            switch (o.primitive) {
            case PRIM_SPHERE:
                h = Mge_RaycastSphere(ray, c, s.x * 0.5f);
                break;
            case PRIM_PLANE:
                h = raycast_object_plane(ray, c, s, r);
                break;
            case PRIM_ARROW:
                h = raycast_object_arrow(ray, c, s, r);
                break;
            case PRIM_POLYGON:
                h = raycast_object_polygon(ray, &o);
                break;
            case PRIM_CUBE:
            default:
                h = Mge_RaycastBox(ray, c, s, r);
                break;
            }
        }

        if (h.hit && (!best.hit || h.distance < best.distance)) {
            best = h;
            best.index = i;
        }
    }

    return best;
}

// -------------------------------------------------------------------------
// screen -> ray
// -------------------------------------------------------------------------

Ray Mge_GetScreenRay(Vector2 pixel, Camera3D camera, int screenWidth, int screenHeight)
{
    float w = (screenWidth > 0) ? (float)screenWidth : 1.0f;
    float h = (screenHeight > 0) ? (float)screenHeight : 1.0f;
    float aspect = w / h;

    Vector3 fwd = v3norm(camera.target); // Camera3D.target is a direction
    Vector3 right = v3norm(v3cross(fwd, camera.up));
    Vector3 up = v3cross(right, fwd);

    // pixel -> normalised device coords ([-1,1], y up)
    float ndcX = 2.0f * pixel.x / w - 1.0f;
    float ndcY = 1.0f - 2.0f * pixel.y / h;

    Ray ray;
    if (camera.projection == CAMERA_ORTHOGRAPHIC) {
        float top = camera.fovy * 0.5f;
        float rightExtent = top * aspect;
        ray.position = v3add(camera.position,
            v3add(v3scale(right, ndcX * rightExtent), v3scale(up, ndcY * top)));
        ray.direction = fwd;
    } else {
        float tanHalf = tanf(camera.fovy * 0.5f * DEG2RAD);
        Vector3 dir = v3add(fwd,
            v3add(v3scale(right, ndcX * tanHalf * aspect), v3scale(up, ndcY * tanHalf)));
        ray.position = camera.position;
        ray.direction = v3norm(dir);
    }
    return ray;
}

Ray Mge_GetMouseRay(Camera3D camera)
{
    Vector2 m = GetMousePosition();
    return Mge_GetScreenRay(m, camera, Mge_GetScreenWidth(), Mge_GetScreenHeight());
}

// -------------------------------------------------------------------------
// debug draw (inside Mge_BeginMode3D)
// -------------------------------------------------------------------------

void Mge_DrawRay(Ray ray, float length, Color color)
{
    Vector3 d = v3norm(ray.direction);
    Draw_Arrow3D(ray.position, v3add(ray.position, v3scale(d, length)), color);
}

void Mge_DrawRayHit(Ray ray, RayHit hit, Color rayColor, Color hitColor)
{
    Vector3 d = v3norm(ray.direction);
    float len = hit.hit ? hit.distance : 100.0f;
    Draw_Arrow3D(ray.position, v3add(ray.position, v3scale(d, len)), rayColor);

    if (hit.hit) {
        Draw_CubeWires(hit.point, (Vector3){ 0.15f, 0.15f, 0.15f }, hitColor);
        Draw_Arrow3D(hit.point, v3add(hit.point, v3scale(v3norm(hit.normal), 0.6f)), hitColor);
    }
}
