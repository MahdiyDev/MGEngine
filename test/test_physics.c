#include <math.h>
#include <stdbool.h>

#include "mge.h"
#include "mge_math.h"
#include "test.h" // mlib repo-wide harness

static bool feq(float a, float b) { return fabsf(a - b) < 1e-3f; }
#define CHECK_F(a, b) CHECK(feq((a), (b)))

// ---- draw / input backend stubs (so mge_physics.c links without a window) ----
void Draw_Arrow3D(Vector3 a, Vector3 b, Color c) { (void)a; (void)b; (void)c; }
void Draw_CubeWires(Vector3 a, Vector3 b, Color c) { (void)a; (void)b; (void)c; }

static Vector2 g_mouse = { 400.0f, 300.0f };
Vector2 GetMousePosition(void) { return g_mouse; }
int Mge_GetScreenWidth(void) { return 800; }
int Mge_GetScreenHeight(void) { return 600; }

static Ray ray_from(Vector3 o, Vector3 d) { return (Ray){ o, d }; }

TEST(sphere_hit_head_on)
{
    Ray r = ray_from((Vector3){ 0, 0, -5 }, (Vector3){ 0, 0, 1 });
    RayHit h = Mge_RaycastSphere(r, (Vector3){ 0, 0, 0 }, 1.0f);

    CHECK(h.hit);
    CHECK_F(h.distance, 4.0f);        // surface at z = -1
    CHECK_F(h.point.z, -1.0f);
    CHECK_F(h.normal.z, -1.0f);       // faces back toward the ray
}

TEST(sphere_miss)
{
    Ray r = ray_from((Vector3){ 0, 3, -5 }, (Vector3){ 0, 0, 1 });
    RayHit h = Mge_RaycastSphere(r, (Vector3){ 0, 0, 0 }, 1.0f);
    CHECK(!h.hit);
    CHECK(h.index == -1);
}

TEST(sphere_behind_is_a_miss)
{
    Ray r = ray_from((Vector3){ 0, 0, -5 }, (Vector3){ 0, 0, -1 });
    RayHit h = Mge_RaycastSphere(r, (Vector3){ 0, 0, 0 }, 1.0f);
    CHECK(!h.hit);
}

TEST(sphere_from_inside)
{
    Ray r = ray_from((Vector3){ 0, 0, 0 }, (Vector3){ 1, 0, 0 });
    RayHit h = Mge_RaycastSphere(r, (Vector3){ 0, 0, 0 }, 2.0f);
    CHECK(h.hit);
    CHECK_F(h.distance, 2.0f);
    CHECK_F(h.point.x, 2.0f);
    CHECK_F(h.normal.x, -1.0f); // flipped to face the ray
}

TEST(sphere_unnormalised_direction)
{
    Ray r = ray_from((Vector3){ 0, 0, -5 }, (Vector3){ 0, 0, 7.5f }); // not unit
    RayHit h = Mge_RaycastSphere(r, (Vector3){ 0, 0, 0 }, 1.0f);
    CHECK(h.hit);
    CHECK_F(h.distance, 4.0f); // distance is in world units, not direction lengths
}

TEST(aabb_hit_and_face_normal)
{
    Ray r = ray_from((Vector3){ -5, 0, 0 }, (Vector3){ 1, 0, 0 });
    RayHit h = Mge_RaycastAABB(r, (Vector3){ -1, -1, -1 }, (Vector3){ 1, 1, 1 });

    CHECK(h.hit);
    CHECK_F(h.distance, 4.0f);
    CHECK_F(h.point.x, -1.0f);
    CHECK_F(h.normal.x, -1.0f);
    CHECK_F(h.normal.y, 0.0f);
}

TEST(aabb_miss_alongside)
{
    Ray r = ray_from((Vector3){ -5, 5, 0 }, (Vector3){ 1, 0, 0 });
    RayHit h = Mge_RaycastAABB(r, (Vector3){ -1, -1, -1 }, (Vector3){ 1, 1, 1 });
    CHECK(!h.hit);
}

TEST(aabb_from_inside_hits_exit_face)
{
    Ray r = ray_from((Vector3){ 0, 0, 0 }, (Vector3){ 0, 1, 0 });
    RayHit h = Mge_RaycastAABB(r, (Vector3){ -1, -1, -1 }, (Vector3){ 1, 1, 1 });
    CHECK(h.hit);
    CHECK_F(h.distance, 1.0f);
    CHECK_F(h.point.y, 1.0f);
    CHECK_F(h.normal.y, -1.0f); // exit face normal, flipped to face the ray
}

TEST(box_obb_rotated_45)
{
    // a unit cube spun 45 deg about Y: its corner now points at -Z, so a ray
    // down -Z from z=+5 meets it at z = sqrt(0.5) ~= 0.7071
    Quaternion q = Quaternion_FromAxisAngle((Vector3){ 0, 1, 0 }, 45.0f * DEG2RAD);
    Ray r = ray_from((Vector3){ 0, 0, 5 }, (Vector3){ 0, 0, -1 });
    RayHit h = Mge_RaycastBox(r, (Vector3){ 0, 0, 0 }, (Vector3){ 1, 1, 1 }, q);

    CHECK(h.hit);
    CHECK_F(h.point.z, 0.70710677f);
    CHECK_F(h.distance, 5.0f - 0.70710677f);
}

TEST(box_identity_matches_aabb)
{
    Ray r = ray_from((Vector3){ -5, 0.2f, 0.1f }, (Vector3){ 1, 0, 0 });
    RayHit a = Mge_RaycastAABB(r, (Vector3){ -1, -1, -1 }, (Vector3){ 1, 1, 1 });
    RayHit b = Mge_RaycastBox(r, (Vector3){ 0, 0, 0 }, (Vector3){ 2, 2, 2 }, Quaternion_Identity());
    CHECK(a.hit && b.hit);
    CHECK_F(a.distance, b.distance);
}

TEST(plane_hit)
{
    Ray r = ray_from((Vector3){ 0, 5, 0 }, (Vector3){ 0, -1, 0 });
    RayHit h = Mge_RaycastPlane(r, (Vector3){ 0, 0, 0 }, (Vector3){ 0, 1, 0 });
    CHECK(h.hit);
    CHECK_F(h.distance, 5.0f);
    CHECK_F(h.point.y, 0.0f);
    CHECK_F(h.normal.y, 1.0f);
}

TEST(plane_parallel_and_behind_miss)
{
    RayHit par = Mge_RaycastPlane(ray_from((Vector3){ 0, 5, 0 }, (Vector3){ 1, 0, 0 }),
        (Vector3){ 0, 0, 0 }, (Vector3){ 0, 1, 0 });
    CHECK(!par.hit);

    RayHit behind = Mge_RaycastPlane(ray_from((Vector3){ 0, 5, 0 }, (Vector3){ 0, 1, 0 }),
        (Vector3){ 0, 0, 0 }, (Vector3){ 0, 1, 0 });
    CHECK(!behind.hit);
}

TEST(triangle_hit_and_miss)
{
    Vector3 a = { -1, 0, 0 }, b = { 1, 0, 0 }, c = { 0, 2, 0 };
    RayHit hit = Mge_RaycastTriangle(ray_from((Vector3){ 0, 0.5f, -3 }, (Vector3){ 0, 0, 1 }), a, b, c);
    CHECK(hit.hit);
    CHECK_F(hit.distance, 3.0f);
    CHECK_F(fabsf(hit.normal.z), 1.0f);

    RayHit miss = Mge_RaycastTriangle(ray_from((Vector3){ 5, 0.5f, -3 }, (Vector3){ 0, 0, 1 }), a, b, c);
    CHECK(!miss.hit);
}

static Object shape(PrimitiveKind k, Vector3 pos, Vector3 scale)
{
    Object o = { 0 };
    o.kind = OBJECT_3D;
    o.primitive = k;
    o.transform.position = pos;
    o.transform.scale = scale;
    o.transform.rotation = Quaternion_Identity();
    o.transform.parent = -1;
    o.active = true;
    return o;
}

TEST(raycast_objects_returns_nearest)
{
    Object objs[3] = {
        shape(PRIM_SPHERE, (Vector3){ 0, 0, 0 }, (Vector3){ 2, 2, 2 }),   // near, r=1
        shape(PRIM_CUBE, (Vector3){ 0, 0, 10 }, (Vector3){ 2, 2, 2 }),    // far
        shape(PRIM_SPHERE, (Vector3){ 20, 0, 0 }, (Vector3){ 2, 2, 2 }),  // off to the side
    };
    Ray r = ray_from((Vector3){ 0, 0, -8 }, (Vector3){ 0, 0, 1 });
    RayHit h = Mge_RaycastObjects(r, objs, 3);

    CHECK(h.hit);
    CHECK(h.index == 0);
    CHECK_F(h.distance, 7.0f);
}

TEST(raycast_objects_skips_inactive)
{
    Object objs[2] = {
        shape(PRIM_SPHERE, (Vector3){ 0, 0, 0 }, (Vector3){ 2, 2, 2 }),
        shape(PRIM_CUBE, (Vector3){ 0, 0, 10 }, (Vector3){ 2, 2, 2 }),
    };
    objs[0].active = false;
    Ray r = ray_from((Vector3){ 0, 0, -8 }, (Vector3){ 0, 0, 1 });
    RayHit h = Mge_RaycastObjects(r, objs, 2);
    CHECK(h.hit);
    CHECK(h.index == 1);
}

TEST(raycast_objects_all_miss)
{
    Object objs[1] = { shape(PRIM_SPHERE, (Vector3){ 0, 50, 0 }, (Vector3){ 2, 2, 2 }) };
    Ray r = ray_from((Vector3){ 0, 0, -8 }, (Vector3){ 0, 0, 1 });
    RayHit h = Mge_RaycastObjects(r, objs, 1);
    CHECK(!h.hit);
    CHECK(h.index == -1);
}

TEST(raycast_objects_skips_2d_and_hits_camera_markers)
{
    Object rect = shape(PRIM_CUBE, (Vector3){ 0, 0, 0 }, (Vector3){ 4, 4, 4 });
    rect.kind = OBJECT_2D; // a 2D rect: no ray volume
    Object cam = shape(PRIM_CUBE, (Vector3){ 0, 0, 0 }, (Vector3){ 1, 1, 1 });
    cam.kind = OBJECT_CAMERA;

    Object objs[2] = { rect, cam };
    Ray r = ray_from((Vector3){ 0, 0, -8 }, (Vector3){ 0, 0, 1 });
    RayHit h = Mge_RaycastObjects(r, objs, 2);

    CHECK(h.hit);
    CHECK(h.index == 1);                // the 2D rect was skipped
    CHECK_F(h.distance, 8.0f - 0.25f);  // camera marker box is 0.7 x 0.5 x 0.5
}

TEST(raycast_objects_finite_plane)
{
    Object floor = shape(PRIM_PLANE, (Vector3){ 0, 0, 0 }, (Vector3){ 4, 1, 4 });
    // straight down onto the middle -> hit
    RayHit in = Mge_RaycastObjects(ray_from((Vector3){ 0, 5, 0 }, (Vector3){ 0, -1, 0 }), &floor, 1);
    CHECK(in.hit);
    CHECK_F(in.point.y, 0.0f);
    // down outside the 4x4 extent -> miss
    RayHit out = Mge_RaycastObjects(ray_from((Vector3){ 5, 5, 0 }, (Vector3){ 0, -1, 0 }), &floor, 1);
    CHECK(!out.hit);
}

static Camera3D test_cam(void)
{
    Camera3D c = { 0 };
    c.position = (Vector3){ 0, 0, 10 };
    c.target = (Vector3){ 0, 0, -1 }; // looking down -Z
    c.up = (Vector3){ 0, 1, 0 };
    c.fovy = 60.0f;
    c.projection = CAMERA_PERSPECTIVE;
    return c;
}

TEST(screen_ray_centre_points_forward)
{
    Camera3D cam = test_cam();
    Ray r = Mge_GetScreenRay((Vector2){ 400, 300 }, cam, 800, 600);

    CHECK_F(r.position.x, 0.0f);
    CHECK_F(r.position.z, 10.0f);
    CHECK_F(r.direction.x, 0.0f);
    CHECK_F(r.direction.y, 0.0f);
    CHECK_F(r.direction.z, -1.0f);
}

TEST(screen_ray_edges_tilt_correctly)
{
    Camera3D cam = test_cam();
    Ray right = Mge_GetScreenRay((Vector2){ 800, 300 }, cam, 800, 600);
    Ray up = Mge_GetScreenRay((Vector2){ 400, 0 }, cam, 800, 600);

    CHECK(right.direction.x > 0.0f); // right edge -> +X component
    CHECK(up.direction.y > 0.0f);    // top edge -> +Y component
    CHECK_F(Vector3_Length(right.direction), 1.0f);
}

TEST(screen_ray_hits_object_under_centre)
{
    Camera3D cam = test_cam();
    Object o = shape(PRIM_SPHERE, (Vector3){ 0, 0, 0 }, (Vector3){ 2, 2, 2 });
    Ray r = Mge_GetScreenRay((Vector2){ 400, 300 }, cam, 800, 600);
    RayHit h = Mge_RaycastObjects(r, &o, 1);
    CHECK(h.hit);
    CHECK(h.index == 0);
    CHECK_F(h.distance, 9.0f); // 10 - radius 1
}

TEST(mouse_ray_uses_cursor_and_screen_size)
{
    Camera3D cam = test_cam();
    Ray r = Mge_GetMouseRay(cam);          // g_mouse = (400,300), screen 800x600
    CHECK_F(r.direction.z, -1.0f);
    CHECK_F(r.direction.x, 0.0f);
}

TEST(draw_helpers_link_and_run)
{
    Ray r = ray_from((Vector3){ 0, 0, -5 }, (Vector3){ 0, 0, 1 });
    RayHit h = Mge_RaycastSphere(r, (Vector3){ 0, 0, 0 }, 1.0f);
    Mge_DrawRay(r, 5.0f, RED);
    Mge_DrawRayHit(r, h, GREEN, YELLOW);
    RayHit miss = { 0 };
    miss.index = -1;
    Mge_DrawRayHit(r, miss, GREEN, YELLOW); // miss path
    CHECK(true);
}

int main(void)
{
    RUN(sphere_hit_head_on);
    RUN(sphere_miss);
    RUN(sphere_behind_is_a_miss);
    RUN(sphere_from_inside);
    RUN(sphere_unnormalised_direction);
    RUN(aabb_hit_and_face_normal);
    RUN(aabb_miss_alongside);
    RUN(aabb_from_inside_hits_exit_face);
    RUN(box_obb_rotated_45);
    RUN(box_identity_matches_aabb);
    RUN(plane_hit);
    RUN(plane_parallel_and_behind_miss);
    RUN(triangle_hit_and_miss);
    RUN(raycast_objects_returns_nearest);
    RUN(raycast_objects_skips_inactive);
    RUN(raycast_objects_all_miss);
    RUN(raycast_objects_skips_2d_and_hits_camera_markers);
    RUN(raycast_objects_finite_plane);
    RUN(screen_ray_centre_points_forward);
    RUN(screen_ray_edges_tilt_correctly);
    RUN(screen_ray_hits_object_under_centre);
    RUN(mouse_ray_uses_cursor_and_screen_size);
    RUN(draw_helpers_link_and_run);
    return test_summary();
}
