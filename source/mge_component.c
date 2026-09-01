// Object components: a fixed array on Object indexed by ComponentType, reached
// only through these accessors (never obj.components[...] directly).

#include "mge.h"
#include "mge_math.h"

#include <stddef.h>

bool Mge_HasComponent(const Object* o, ComponentType t)
{
    return o != NULL && (unsigned)t < COMPONENT_TYPE_COUNT && o->components[t].present;
}

void* Mge_GetComponent(Object* o, ComponentType t)
{
    if (o == NULL || (unsigned)t >= COMPONENT_TYPE_COUNT || !o->components[t].present)
        return NULL;
    // every union member starts at the same offset -- return the first one
    return &o->components[t].shape;
}

Shape*     Mge_GetShapeComponent(Object* o)     { return (Shape*)Mge_GetComponent(o, COMPONENT_SHAPE); }
Material*  Mge_GetMaterialComponent(Object* o)  { return (Material*)Mge_GetComponent(o, COMPONENT_MATERIAL); }
Collider*  Mge_GetColliderComponent(Object* o)  { return (Collider*)Mge_GetComponent(o, COMPONENT_COLLIDER); }
RigidBody* Mge_GetRigidBodyComponent(Object* o) { return (RigidBody*)Mge_GetComponent(o, COMPONENT_RIGIDBODY); }

const char* Mge_ComponentName(ComponentType t)
{
    switch (t) {
    case COMPONENT_SHAPE:     return "Shape";
    case COMPONENT_MATERIAL:  return "Material";
    case COMPONENT_COLLIDER:  return "Collider";
    case COMPONENT_RIGIDBODY: return "RigidBody";
    default:                  return "?";
    }
}

void* Mge_AddComponent(Object* o, ComponentType t)
{
    if (o == NULL || (unsigned)t >= COMPONENT_TYPE_COUNT)
        return NULL;

    Component* c = &o->components[t];
    if (!c->present) {
        Component fresh = { 0 };
        fresh.present = true;
        switch (t) {
        case COMPONENT_SHAPE:
            fresh.shape.primitive = PRIM_CUBE;
            break;
        case COMPONENT_MATERIAL:
            fresh.material = Mge_DefaultMaterial();
            break;
        case COMPONENT_COLLIDER:
            fresh.collider.kind = COLLIDER_BOX;
            fresh.collider.size = o->transform.scale; // auto-fit to the object's extents
            if (fresh.collider.size.x == 0.0f && fresh.collider.size.y == 0.0f && fresh.collider.size.z == 0.0f)
                fresh.collider.size = (Vector3){ 1.0f, 1.0f, 1.0f };
            break;
        case COMPONENT_RIGIDBODY:
            fresh.body.mass = 1.0f;
            fresh.body.restitution = 0.3f;
            fresh.body.useGravity = true;
            break;
        default:
            break;
        }
        *c = fresh;
    }
    return &c->shape;
}

void Mge_RemoveComponent(Object* o, ComponentType t)
{
    if (o != NULL && (unsigned)t < COMPONENT_TYPE_COUNT)
        o->components[t].present = false; // data left in place, ignored
}
