// Rigid-body physics: linear integration + box/sphere collision detection and
// resolution (positional correction + a restitution impulse). Boxes use their
// axis-aligned bounds -- rotation is not yet fed into the contact solver.

#include "mge.h"
#include "mge_math.h"

#include <math.h>
#include <stddef.h>

static Vector3 s_gravity = { 0.0f, -9.81f, 0.0f };

void    Mge_SetGravity(Vector3 g) { s_gravity = g; }
Vector3 Mge_GetGravity(void) { return s_gravity; }

// --- collider world bounds ------------------------------------------------

typedef struct { Vector3 lo, hi; } AABB;

static bool collider_bounds(const Object* o, AABB* box, Vector3* sphereC, float* sphereR)
{
    const Collider* col = Mge_GetColliderComponent((Object*)o);
    if (col == NULL)
        return false;
    Vector3 c = Vector3_Add(o->transform.position, col->offset);
    if (col->kind == COLLIDER_SPHERE) {
        float r = col->size.x;
        if (sphereC) *sphereC = c;
        if (sphereR) *sphereR = r;
        if (box) { box->lo = (Vector3){ c.x - r, c.y - r, c.z - r }; box->hi = (Vector3){ c.x + r, c.y + r, c.z + r }; }
    } else {
        Vector3 h = Vector3_Scale(col->size, 0.5f);
        if (box) { box->lo = Vector3_Subtract(c, h); box->hi = Vector3_Add(c, h); }
        if (sphereC) *sphereC = c;
        if (sphereR) *sphereR = 0.0f;
    }
    return true;
}

static Vector3 clampv(Vector3 v, Vector3 lo, Vector3 hi)
{
    return (Vector3){
        v.x < lo.x ? lo.x : (v.x > hi.x ? hi.x : v.x),
        v.y < lo.y ? lo.y : (v.y > hi.y ? hi.y : v.y),
        v.z < lo.z ? lo.z : (v.z > hi.z ? hi.z : v.z),
    };
}

// --- overlap tests. `mtv` = translation applied to `a` to separate it from `b`.

static bool overlap_box_box(AABB a, AABB b, Vector3* mtv)
{
    float ox = fminf(a.hi.x, b.hi.x) - fmaxf(a.lo.x, b.lo.x);
    float oy = fminf(a.hi.y, b.hi.y) - fmaxf(a.lo.y, b.lo.y);
    float oz = fminf(a.hi.z, b.hi.z) - fmaxf(a.lo.z, b.lo.z);
    if (ox <= 0.0f || oy <= 0.0f || oz <= 0.0f)
        return false;
    if (mtv) {
        float ac, bc;
        if (ox <= oy && ox <= oz) {
            ac = (a.lo.x + a.hi.x) * 0.5f; bc = (b.lo.x + b.hi.x) * 0.5f;
            *mtv = (Vector3){ ac < bc ? -ox : ox, 0.0f, 0.0f };
        } else if (oy <= oz) {
            ac = (a.lo.y + a.hi.y) * 0.5f; bc = (b.lo.y + b.hi.y) * 0.5f;
            *mtv = (Vector3){ 0.0f, ac < bc ? -oy : oy, 0.0f };
        } else {
            ac = (a.lo.z + a.hi.z) * 0.5f; bc = (b.lo.z + b.hi.z) * 0.5f;
            *mtv = (Vector3){ 0.0f, 0.0f, ac < bc ? -oz : oz };
        }
    }
    return true;
}

static bool overlap_sphere_sphere(Vector3 ca, float ra, Vector3 cb, float rb, Vector3* mtv)
{
    Vector3 d = Vector3_Subtract(ca, cb);
    float dist = Vector3_Length(d);
    float pen = ra + rb - dist;
    if (pen <= 0.0f)
        return false;
    if (mtv) {
        Vector3 n = dist > 1e-6f ? Vector3_Scale(d, 1.0f / dist) : (Vector3){ 0.0f, 1.0f, 0.0f };
        *mtv = Vector3_Scale(n, pen);
    }
    return true;
}

// sphere `a` vs box `b`
static bool overlap_sphere_box(Vector3 ca, float ra, AABB b, Vector3* mtv)
{
    Vector3 closest = clampv(ca, b.lo, b.hi);
    Vector3 d = Vector3_Subtract(ca, closest);
    float dist = Vector3_Length(d);
    if (dist >= ra)
        return false;
    if (mtv) {
        Vector3 n = dist > 1e-6f ? Vector3_Scale(d, 1.0f / dist) : (Vector3){ 0.0f, 1.0f, 0.0f };
        *mtv = Vector3_Scale(n, ra - dist);
    }
    return true;
}

bool Mge_ObjectsOverlap(const Object* a, const Object* b, Vector3* mtv)
{
    AABB ba, bb;
    Vector3 sca, scb;
    float sra, srb;
    if (!collider_bounds(a, &ba, &sca, &sra) || !collider_bounds(b, &bb, &scb, &srb))
        return false;

    const Collider* ca = Mge_GetColliderComponent((Object*)a);
    const Collider* cb = Mge_GetColliderComponent((Object*)b);
    bool sa = ca->kind == COLLIDER_SPHERE, sb = cb->kind == COLLIDER_SPHERE;

    if (sa && sb)
        return overlap_sphere_sphere(sca, sra, scb, srb, mtv);
    if (sa && !sb)
        return overlap_sphere_box(sca, sra, bb, mtv);
    if (!sa && sb) {
        Vector3 m;
        bool hit = overlap_sphere_box(scb, srb, ba, mtv ? &m : NULL);
        if (hit && mtv) *mtv = Vector3_Scale(m, -1.0f); // computed b<-a; flip to a<-b
        return hit;
    }
    return overlap_box_box(ba, bb, mtv);
}

int Mge_CheckCollisions(const Object* objects, int count, MgeCollisionPair* out, int maxOut)
{
    int n = 0;
    for (int i = 0; i < count && n < maxOut; i++) {
        if (!objects[i].active || !Mge_HasComponent(&objects[i], COMPONENT_COLLIDER))
            continue;
        for (int j = i + 1; j < count && n < maxOut; j++) {
            if (!objects[j].active || !Mge_HasComponent(&objects[j], COMPONENT_COLLIDER))
                continue;
            Vector3 mtv;
            if (!Mge_ObjectsOverlap(&objects[i], &objects[j], &mtv))
                continue;
            const Collider* a = Mge_GetColliderComponent((Object*)&objects[i]);
            const Collider* b = Mge_GetColliderComponent((Object*)&objects[j]);
            out[n].a = i;
            out[n].b = j;
            out[n].mtv = mtv;
            out[n].trigger = a->isTrigger || b->isTrigger;
            n++;
        }
    }
    return n;
}

// --- the step -----------------------------------------------------------

void Mge_StepPhysics(Object* objects, int count, float dt)
{
    if (dt <= 0.0f)
        return;

    // 1. integrate
    for (int i = 0; i < count; i++) {
        if (!objects[i].active)
            continue;
        RigidBody* rb = Mge_GetRigidBodyComponent(&objects[i]);
        if (rb == NULL || rb->mass <= 0.0f)
            continue;
        if (rb->useGravity)
            rb->velocity = Vector3_Add(rb->velocity, Vector3_Scale(s_gravity, dt));
        objects[i].transform.position = Vector3_Add(objects[i].transform.position, Vector3_Scale(rb->velocity, dt));
    }

    // 2 + 3. detect + resolve (a couple of passes settle stacks)
    MgeCollisionPair pairs[64];
    for (int pass = 0; pass < 2; pass++) {
        int np = Mge_CheckCollisions(objects, count, pairs, 64);
        for (int k = 0; k < np; k++) {
            if (pairs[k].trigger)
                continue;
            Object* a = &objects[pairs[k].a];
            Object* b = &objects[pairs[k].b];
            RigidBody* ra = Mge_GetRigidBodyComponent(a);
            RigidBody* rb = Mge_GetRigidBodyComponent(b);
            float invA = (ra && ra->mass > 0.0f) ? 1.0f / ra->mass : 0.0f;
            float invB = (rb && rb->mass > 0.0f) ? 1.0f / rb->mass : 0.0f;
            float invSum = invA + invB;
            if (invSum <= 0.0f)
                continue;

            float pen = Vector3_Length(pairs[k].mtv);
            Vector3 nrm = pen > 1e-6f ? Vector3_Scale(pairs[k].mtv, 1.0f / pen) : (Vector3){ 0.0f, 1.0f, 0.0f };

            // positional correction (mtv points the way to push `a`)
            a->transform.position = Vector3_Add(a->transform.position, Vector3_Scale(nrm, pen * (invA / invSum)));
            b->transform.position = Vector3_Subtract(b->transform.position, Vector3_Scale(nrm, pen * (invB / invSum)));

            // restitution impulse along the normal
            Vector3 va = ra ? ra->velocity : (Vector3){ 0, 0, 0 };
            Vector3 vb = rb ? rb->velocity : (Vector3){ 0, 0, 0 };
            float vn = Vector3_DotProduct(Vector3_Subtract(va, vb), nrm);
            if (vn < 0.0f) { // approaching
                // a static collider (no RigidBody) has no restitution of its own;
                // default it to 1 so fminf yields the dynamic body's bounciness
                float e = fminf(ra ? ra->restitution : 1.0f, rb ? rb->restitution : 1.0f);
                float j = -(1.0f + e) * vn / invSum;
                Vector3 imp = Vector3_Scale(nrm, j);
                if (ra && invA > 0.0f) ra->velocity = Vector3_Add(va, Vector3_Scale(imp, invA));
                if (rb && invB > 0.0f) rb->velocity = Vector3_Subtract(vb, Vector3_Scale(imp, invB));
            }
        }
    }
}

// --- debug draw --------------------------------------------------------

void Draw_ColliderWires(Object obj, Color color)
{
    const Collider* col = Mge_GetColliderComponent(&obj);
    if (col == NULL)
        return;
    Vector3 c = Vector3_Add(obj.transform.position, col->offset);
    if (col->kind == COLLIDER_SPHERE)
        Draw_SphereWiresEx(c, col->size.x, 8, 12, color);
    else
        Draw_CubeWires(c, col->size, color);
}
