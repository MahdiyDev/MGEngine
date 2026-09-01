// Object component system: add / remove / has / get, typed vs generic accessors,
// the seeded defaults, and the components that Mge_MakeShape3D / Mge_MakeObject2D
// attach. Hermetic -- the draw + input backend is stubbed.

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "mge.h"
#include "mge_math.h"
#include "test.h"

static bool feq(float a, float b) { return fabsf(a - b) < 1e-3f; }
#define CHECK_F(a, b) CHECK(feq((a), (b)))

// ---- draw / input backend stubs (so mge_object.c + mge_physics.c link) ----
void Draw_RectangleRec(Rectangle r, Color c) { (void)r; (void)c; }
void Draw_RectangleLines(int a, int b, int w, int h, Color c) { (void)a; (void)b; (void)w; (void)h; (void)c; }
void Draw_Cube(Vector3 a, Vector3 b, Color c) { (void)a; (void)b; (void)c; }
void Draw_CubeEx(Vector3 a, Vector3 b, Quaternion r, Color c) { (void)a; (void)b; (void)r; (void)c; }
void Draw_CubeWires(Vector3 a, Vector3 b, Color c) { (void)a; (void)b; (void)c; }
void Draw_CubeWiresEx(Vector3 a, Vector3 b, Quaternion q, Color c) { (void)a; (void)b; (void)q; (void)c; }
void Draw_SphereEx(Vector3 a, float r, int ri, int sl, Color c) { (void)a; (void)r; (void)ri; (void)sl; (void)c; }
void Draw_SphereWiresEx(Vector3 a, float r, int ri, int sl, Color c) { (void)a; (void)r; (void)ri; (void)sl; (void)c; }
void Draw_Plane(Vector3 a, float w, float l, Color c) { (void)a; (void)w; (void)l; (void)c; }
void Draw_Arrow(Vector2 a, Vector2 b, float s, Color c) { (void)a; (void)b; (void)s; (void)c; }
void Draw_Arrow3D(Vector3 a, Vector3 b, Color c) { (void)a; (void)b; (void)c; }
void Draw_CameraFrustum(Camera3D cam, float a, float n, float f, Color c) { (void)cam; (void)a; (void)n; (void)f; (void)c; }
void Draw_Line3D(Vector3 a, Vector3 b, Color c) { (void)a; (void)b; (void)c; }
void Draw_Quad3D(Vector3 a, Vector2 s, Quaternion q, Color c) { (void)a; (void)s; (void)q; (void)c; }
void Draw_Quad3DWires(Vector3 a, Vector2 s, Quaternion q, Color c) { (void)a; (void)s; (void)q; (void)c; }
void Draw_Polygon3D(const Vector3* p, int n, bool st, Color c) { (void)p; (void)n; (void)st; (void)c; }
void Draw_Polygon3DWires(const Vector3* p, int n, bool cl, Color c) { (void)p; (void)n; (void)cl; (void)c; }
void Mge_DrawObjectOutline(Object o, float t, Color c) { (void)o; (void)t; (void)c; }
void Mge_SetMaterial(Material m) { (void)m; }

static Vector2 g_mouse;
Vector2 GetMousePosition(void) { return g_mouse; }
bool IsMouseButtonPressed(int b) { (void)b; return false; }
bool IsMouseButtonDown(int b) { (void)b; return false; }
bool IsMouseButtonReleased(int b) { (void)b; return false; }
int Mge_GetScreenWidth(void) { return 800; }
int Mge_GetScreenHeight(void) { return 600; }

// ---- tests ----

TEST(bare_object_has_no_components)
{
    Object o = { 0 };
    for (int t = 0; t < COMPONENT_TYPE_COUNT; t++) {
        CHECK(!Mge_HasComponent(&o, (ComponentType)t));
        CHECK(Mge_GetComponent(&o, (ComponentType)t) == NULL);
    }
    CHECK(Mge_GetShapeComponent(&o) == NULL);
    CHECK(Mge_GetMaterialComponent(&o) == NULL);
    CHECK(Mge_GetColliderComponent(&o) == NULL);
    CHECK(Mge_GetRigidBodyComponent(&o) == NULL);
}

TEST(null_object_is_safe)
{
    CHECK(!Mge_HasComponent(NULL, COMPONENT_SHAPE));
    CHECK(Mge_GetComponent(NULL, COMPONENT_SHAPE) == NULL);
    CHECK(Mge_AddComponent(NULL, COMPONENT_SHAPE) == NULL);
    Mge_RemoveComponent(NULL, COMPONENT_SHAPE); // no crash
    CHECK(true);
}

TEST(add_then_has_then_remove)
{
    Object o = { 0 };
    o.transform.scale = (Vector3){ 1, 1, 1 };

    void* p = Mge_AddComponent(&o, COMPONENT_COLLIDER);
    CHECK(p != NULL);
    CHECK(Mge_HasComponent(&o, COMPONENT_COLLIDER));
    CHECK(Mge_GetComponent(&o, COMPONENT_COLLIDER) == p); // generic accessor == the add result
    CHECK((void*)Mge_GetColliderComponent(&o) == p);      // typed accessor agrees

    Mge_RemoveComponent(&o, COMPONENT_COLLIDER);
    CHECK(!Mge_HasComponent(&o, COMPONENT_COLLIDER));
    CHECK(Mge_GetColliderComponent(&o) == NULL);
}

TEST(add_is_idempotent_and_keeps_data)
{
    Object o = { 0 };
    RigidBody* a = Mge_AddComponent(&o, COMPONENT_RIGIDBODY);
    a->mass = 9.0f;
    RigidBody* b = Mge_AddComponent(&o, COMPONENT_RIGIDBODY); // already present
    CHECK(a == b);
    CHECK_F(b->mass, 9.0f); // not re-seeded
}

TEST(seeded_defaults)
{
    Object o = { 0 };
    o.transform.scale = (Vector3){ 3, 4, 5 };

    Shape* sh = Mge_AddComponent(&o, COMPONENT_SHAPE);
    CHECK(sh->primitive == PRIM_CUBE);

    Material* m = Mge_AddComponent(&o, COMPONENT_MATERIAL);
    CHECK_F(m->shininess, Mge_DefaultMaterial().shininess);

    Collider* c = Mge_AddComponent(&o, COMPONENT_COLLIDER);
    CHECK(c->kind == COLLIDER_BOX);
    CHECK_F(c->size.x, 3.0f); // auto-fit to transform.scale
    CHECK_F(c->size.z, 5.0f);
    CHECK(!c->isTrigger);

    RigidBody* rb = Mge_AddComponent(&o, COMPONENT_RIGIDBODY);
    CHECK_F(rb->mass, 1.0f);
    CHECK_F(rb->restitution, 0.3f);
    CHECK(rb->useGravity);
}

TEST(collider_autofit_falls_back_when_scale_is_zero)
{
    Object o = { 0 }; // scale all-zero
    Collider* c = Mge_AddComponent(&o, COMPONENT_COLLIDER);
    CHECK_F(c->size.x, 1.0f);
    CHECK_F(c->size.y, 1.0f);
    CHECK_F(c->size.z, 1.0f);
}

TEST(components_are_independent)
{
    Object o = { 0 };
    Mge_AddComponent(&o, COMPONENT_SHAPE);
    Mge_AddComponent(&o, COMPONENT_RIGIDBODY);
    CHECK(Mge_HasComponent(&o, COMPONENT_SHAPE));
    CHECK(Mge_HasComponent(&o, COMPONENT_RIGIDBODY));
    CHECK(!Mge_HasComponent(&o, COMPONENT_MATERIAL));
    CHECK(!Mge_HasComponent(&o, COMPONENT_COLLIDER));

    Mge_RemoveComponent(&o, COMPONENT_SHAPE);
    CHECK(!Mge_HasComponent(&o, COMPONENT_SHAPE));
    CHECK(Mge_HasComponent(&o, COMPONENT_RIGIDBODY)); // untouched
}

TEST(component_name)
{
    CHECK(strcmp(Mge_ComponentName(COMPONENT_SHAPE), "Shape") == 0);
    CHECK(strcmp(Mge_ComponentName(COMPONENT_MATERIAL), "Material") == 0);
    CHECK(strcmp(Mge_ComponentName(COMPONENT_COLLIDER), "Collider") == 0);
    CHECK(strcmp(Mge_ComponentName(COMPONENT_RIGIDBODY), "RigidBody") == 0);
}

TEST(make_shape3d_attaches_shape_and_material)
{
    Object o = Mge_MakeShape3D(PRIM_SPHERE, (Vector3){ 0, 0, 0 }, (Vector3){ 2, 2, 2 }, RED);
    CHECK(Mge_HasComponent(&o, COMPONENT_SHAPE));
    CHECK(Mge_HasComponent(&o, COMPONENT_MATERIAL));
    CHECK(!Mge_HasComponent(&o, COMPONENT_COLLIDER));
    CHECK(!Mge_HasComponent(&o, COMPONENT_RIGIDBODY));
    CHECK(Mge_GetShapeComponent(&o)->primitive == PRIM_SPHERE);
    CHECK(Mge_GetMaterialComponent(&o)->maps[MATERIAL_MAP_DIFFUSE].color.r == 255);
}

TEST(make_object2d_attaches_material_only)
{
    Object o = Mge_MakeObject2D(0, 0, 10, 10, GREEN);
    CHECK(!Mge_HasComponent(&o, COMPONENT_SHAPE));
    CHECK(Mge_HasComponent(&o, COMPONENT_MATERIAL));
    CHECK(Mge_GetMaterialComponent(&o)->maps[MATERIAL_MAP_DIFFUSE].color.g == 255);
}

int main(void)
{
    RUN(bare_object_has_no_components);
    RUN(null_object_is_safe);
    RUN(add_then_has_then_remove);
    RUN(add_is_idempotent_and_keeps_data);
    RUN(seeded_defaults);
    RUN(collider_autofit_falls_back_when_scale_is_zero);
    RUN(components_are_independent);
    RUN(component_name);
    RUN(make_shape3d_attaches_shape_and_material);
    RUN(make_object2d_attaches_material_only);
    return test_summary();
}
