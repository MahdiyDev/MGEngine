// .mgscene round-trip (editor/scene_io.c) + the path helpers (editor/pathutil.c).
// Hermetic: scene_io.c's few engine dependencies are stubbed here, so no GL and
// no other engine .o are linked.

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
    #include <direct.h>
    #define RMDIR(p) _rmdir(p)
#else
    #include <unistd.h>
    #define RMDIR(p) rmdir(p)
#endif

#include "mge.h"
#include "test.h"

#include "../editor/scene.h"
#include "../editor/scene_io.h"
#include "../editor/pathutil.h"

static bool feq(float a, float b) { return fabsf(a - b) < 1e-3f; }
#define CHECK_F(a, b) CHECK(feq((a), (b)))

// ---- stubs for the handful of engine calls scene_io.c makes ----

static bool g_msaa = true;
bool Mge_IsMSAAEnabled(void) { return g_msaa; }
void Mge_SetMSAAEnabled(bool e) { g_msaa = e; }

// the real component accessors + Mge_DefaultMaterial are linked in (mge_component.c
// + mge_material.c); only Mge_MakeShape3D itself is reimplemented here.
Object Mge_MakeShape3D(PrimitiveKind primitive, Vector3 position, Vector3 size, Color color)
{
    Object o = { 0 };
    o.kind = OBJECT_3D;
    o.active = true;
    o.transform.position = position;
    o.transform.rotation = Quaternion_Identity();
    o.transform.scale = size;
    o.transform.parent = -1;
    Shape* sh = Mge_AddComponent(&o, COMPONENT_SHAPE);
    sh->primitive = primitive;
    Material* m = Mge_AddComponent(&o, COMPONENT_MATERIAL);
    m->maps[MATERIAL_MAP_DIFFUSE].color = color;
    return o;
}

// shorthand for the round-trip assertions
static Shape* shp(Object* o) { return Mge_GetShapeComponent(o); }
static Material* mtl(Object* o) { return Mge_GetMaterialComponent(o); }

Light Mge_MakePointLight(Vector3 position, Vector3 color)
{
    Light l = { 0 };
    l.type = LIGHT_POINT;
    l.enabled = true;
    l.position = position;
    l.color = color;
    l.ambient = 0.05f;
    l.diffuse = 1.0f;
    l.specular = 1.0f;
    l.constant = 1.0f;
    l.linear = 0.09f;
    l.quadratic = 0.032f;
    return l;
}

Light Mge_MakeDirectionalLight(Vector3 direction, Vector3 color)
{
    Light l = { 0 };
    l.type = LIGHT_DIRECTIONAL;
    l.enabled = true;
    l.direction = direction;
    l.color = color;
    l.ambient = 0.05f;
    l.diffuse = 1.0f;
    l.specular = 1.0f;
    return l;
}

// ---- path helpers ----

TEST(path_helpers)
{
    char out[256];

    Path_Dir("a/b/c.mgscene", out, sizeof(out));
    CHECK(strcmp(out, "a/b") == 0);
    Path_Dir("c.mgscene", out, sizeof(out));
    CHECK(out[0] == '\0');
    Path_Dir("C:\\proj\\scene.mgscene", out, sizeof(out));
    CHECK(strcmp(out, "C:/proj") == 0);

    Path_Base("a/b/c.mgscene", out, sizeof(out));
    CHECK(strcmp(out, "c.mgscene") == 0);
    Path_Base("plain", out, sizeof(out));
    CHECK(strcmp(out, "plain") == 0);

    strcpy(out, "scene.tar.mgscene");
    Path_StripExt(out);
    CHECK(strcmp(out, "scene.tar") == 0);

    CHECK(Path_IsAbsolute("/x") && Path_IsAbsolute("C:\\x") && Path_IsAbsolute("C:/x"));
    CHECK(!Path_IsAbsolute("res/x.png") && !Path_IsAbsolute("x.png"));

    Path_Join("a/b", "c/d.png", out, sizeof(out));
    CHECK(strcmp(out, "a/b/c/d.png") == 0);
    Path_Join("a/b/", "c.png", out, sizeof(out));
    CHECK(strcmp(out, "a/b/c.png") == 0);
    Path_Join("a/b", "/abs/c.png", out, sizeof(out));
    CHECK(strcmp(out, "/abs/c.png") == 0);

    CHECK(Path_Equal("a/b/c", "a\\b\\c"));       // separator-agnostic
    CHECK(Path_Equal("a/b/", "a/b"));            // trailing slash ignored
    CHECK(!Path_Equal("a/b/c", "a/b"));
#if defined(_WIN32)
    CHECK(Path_Equal("C:/Proj", "c:\\proj"));    // case-insensitive on Windows
#endif
}

TEST(fs_list_rename_remove)
{
    Path_MakeDirs("pu_tmp/a/b");
    FILE* f = fopen("pu_tmp/a/one.txt", "wb");
    CHECK(f != NULL);
    fputs("x", f);
    fclose(f);
    f = fopen("pu_tmp/a/two.md", "wb");
    fclose(f);

    CHECK(Path_IsDir("pu_tmp/a") && !Path_IsDir("pu_tmp/a/one.txt"));

    char names[16][128];
    int nd = Path_List("pu_tmp/a", NULL, true, names, 16);
    CHECK(nd == 1 && strcmp(names[0], "b") == 0);
    int nf = Path_List("pu_tmp/a", NULL, false, names, 16);
    CHECK(nf == 2);
    int txt = Path_List("pu_tmp/a", ".txt", false, names, 16);
    CHECK(txt == 1 && strcmp(names[0], "one.txt") == 0);
    CHECK(Path_List("pu_tmp/nope", NULL, false, names, 16) == -1);

    CHECK(Path_MTime("pu_tmp/a/one.txt") > 0);
    CHECK(Path_MTime("pu_tmp/nope") == 0);

    CHECK(Path_Rename("pu_tmp/a/one.txt", "pu_tmp/a/renamed.txt"));
    CHECK(Path_MTime("pu_tmp/a/renamed.txt") > 0 && Path_MTime("pu_tmp/a/one.txt") == 0);

    // copying a file onto itself must not truncate it to 0 bytes
    CHECK(Path_CopyFile("pu_tmp/a/renamed.txt", "pu_tmp/a/renamed.txt"));
    CHECK(Path_CopyFile("pu_tmp/a/renamed.txt", "pu_tmp\\a\\renamed.txt")); // same path, other sep
    f = fopen("pu_tmp/a/renamed.txt", "rb");
    CHECK(f != NULL);
    fseek(f, 0, SEEK_END);
    CHECK(ftell(f) == 1); // still "x"
    fclose(f);
    CHECK(Path_CopyFile("pu_tmp/a/renamed.txt", "pu_tmp/a/copy.txt")); // real copy still works
    CHECK(Path_MTime("pu_tmp/a/copy.txt") > 0);

    CHECK(Path_Remove("pu_tmp"));       // recursive
    CHECK(!Path_IsDir("pu_tmp"));
    CHECK(Path_Remove("pu_tmp"));       // already gone -> still true
}

// ---- .mgscene round-trip ----

// reference orientations for the round-trip check (euler degrees -> quaternion)
static Quaternion ball_rot(void)
{
    return Quaternion_FromEuler((Vector3){ 10 * DEG2RAD, 20 * DEG2RAD, 30 * DEG2RAD });
}
static Quaternion cam_rot(void)
{
    return Quaternion_FromEuler((Vector3){ -5 * DEG2RAD, -90 * DEG2RAD, 15 * DEG2RAD });
}

static void build_scene(Scene* s)
{
    memset(s, 0, sizeof(*s));

    s->objects[0] = Mge_MakeShape3D(PRIM_PLANE, (Vector3){ 0, -1.1f, 0 }, (Vector3){ 24, 0.2f, 24 }, (Color){ 90, 95, 105, 255 });
    strcpy(s->objectNames[0], "Floor");

    s->objects[1] = Mge_MakeShape3D(PRIM_SPHERE, (Vector3){ 2.5f, 1.0f, -0.5f }, (Vector3){ 1.5f, 1.5f, 1.5f }, (Color){ 200, 80, 80, 255 });
    s->objects[1].transform.rotation = ball_rot();
    s->objects[1].active = false;
    mtl(&s->objects[1])->shininess = 64.0f;
    mtl(&s->objects[1])->triplanar = true;
    mtl(&s->objects[1])->tiling = (Vector2){ 2, 3 };
    strcpy(s->objectNames[1], "Ball");
    strcpy(s->texPath[1][MATERIAL_MAP_DIFFUSE], "res/ball_albedo.png");
    s->texWrap[1][MATERIAL_MAP_DIFFUSE] = 2;

    s->objects[2] = Mge_MakeShape3D(PRIM_CUBE, (Vector3){ 1, 2, 3 }, (Vector3){ 1, 1, 1 }, (Color){ 255, 255, 255, 255 });
    s->objects[2].kind = OBJECT_CAMERA;
    s->objects[2].transform.rotation = cam_rot();
    strcpy(s->objectNames[2], "GameCam");
    s->objects[1].transform.parent = 0; // Ball parented to Floor (grouping)
    s->objectCount = 3;

    strcpy(s->skyDir, "res/sky_night");
    s->mainCamera = 2;

    s->lights[0] = Mge_MakeDirectionalLight((Vector3){ -0.5f, -1.0f, -0.4f }, (Vector3){ 0.7f, 0.7f, 0.8f });
    s->lights[0].ambient = 0.22f;
    strcpy(s->lightNames[0], "Sun");
    s->lights[1] = Mge_MakePointLight((Vector3){ 3, 5, 2 }, (Vector3){ 1.0f, 0.85f, 0.6f });
    s->lights[1].linear = 0.14f;
    strcpy(s->lightNames[1], "Lamp");
    s->lightCount = 2;

    s->shadowsOn = true;
    s->shadowRadius = 14.0f;
    s->hdrOn = true;
    s->toneMap = 2;
    s->exposure = 1.5f;
    s->bloomOn = true;
    s->bloom.threshold = 1.2f;
    s->bloom.intensity = 0.8f;
}

TEST(scene_mge_round_trip)
{
    Path_MakeDirs("scene_io_tmp");
    const char* path = "scene_io_tmp/myscene.mgscene";

    Scene a;
    build_scene(&a);
    Camera3D camA = { .position = { 1, 2, 3 }, .target = { 0, 0, -1 }, .up = { 0, 1, 0 }, .fovy = 55.0f };

    CHECK(Scene_Save(&a, path, camA, "scene_io_tmp")); // project root = the tmp dir
    CHECK(strcmp(a.name, "myscene") == 0); // name follows the file stem
    CHECK(!a.dirty);
    CHECK(strcmp(a.texPath[1][MATERIAL_MAP_DIFFUSE], "res/ball_albedo.png") == 0);

    Scene b;
    Camera3D camB = { 0 };
    CHECK(Scene_Load(&b, path, &camB));

    CHECK(b.objectCount == 3);
    CHECK(strcmp(b.objectNames[0], "Floor") == 0);
    CHECK(strcmp(b.objectNames[1], "Ball") == 0);
    CHECK(shp(&b.objects[0])->primitive == PRIM_PLANE);
    CHECK(shp(&b.objects[1])->primitive == PRIM_SPHERE);

    CHECK(b.objects[0].kind == OBJECT_3D);
    CHECK(b.objects[2].kind == OBJECT_CAMERA);
    CHECK(strcmp(b.objectNames[2], "GameCam") == 0);
    CHECK(Quaternion_Approx(b.objects[2].transform.rotation, cam_rot())); // quaternion round-trips
    CHECK(b.mainCamera == 2);
    CHECK(strcmp(b.skyDir, "res/sky_night") == 0);
    CHECK(b.objects[1].transform.parent == 0);  // parent link round-trips
    CHECK(b.objects[0].transform.parent == -1); // default when absent
    CHECK(b.objects[1].active == false);
    CHECK_F(b.objects[1].transform.position.x, 2.5f);
    CHECK(Quaternion_Approx(b.objects[1].transform.rotation, ball_rot()));
    CHECK(Quaternion_Approx(b.objects[0].transform.rotation, Quaternion_Identity())); // no line -> identity
    CHECK_F(b.objects[1].transform.scale.z, 1.5f);
    CHECK_F(mtl(&b.objects[1])->shininess, 64.0f);
    CHECK(mtl(&b.objects[1])->triplanar == true);
    CHECK_F(mtl(&b.objects[1])->tiling.y, 3.0f);
    CHECK(mtl(&b.objects[0])->maps[MATERIAL_MAP_DIFFUSE].color.r == 90);
    CHECK(strcmp(b.texPath[1][MATERIAL_MAP_DIFFUSE], "res/ball_albedo.png") == 0);
    CHECK(b.texWrap[1][MATERIAL_MAP_DIFFUSE] == 2);

    CHECK(b.lightCount == 2);
    CHECK(b.lights[0].type == LIGHT_DIRECTIONAL);
    CHECK(strcmp(b.lightNames[0], "Sun") == 0);
    CHECK_F(b.lights[0].ambient, 0.22f);
    CHECK_F(b.lights[0].direction.y, -1.0f);
    CHECK(b.lights[1].type == LIGHT_POINT);
    CHECK_F(b.lights[1].position.x, 3.0f);
    CHECK_F(b.lights[1].linear, 0.14f);
    CHECK_F(b.lights[1].color.z, 0.6f);

    CHECK(b.shadowsOn && b.hdrOn && b.bloomOn);
    CHECK(b.toneMap == 2);
    CHECK_F(b.exposure, 1.5f);
    CHECK_F(b.bloom.threshold, 1.2f);

    CHECK_F(camB.position.z, 3.0f);
    CHECK_F(camB.fovy, 55.0f);

    CHECK(strcmp(b.path, path) == 0);
    CHECK(!b.dirty);

    remove(path);
    remove("scene_io_tmp/myscene.c"); // template scaffolded by Scene_Save
}

// a legacy `rotation <deg> <deg> <deg>` line (3 values) reads as euler degrees
TEST(legacy_euler_rotation_line)
{
    Path_MakeDirs("scene_io_tmp");
    const char* path = "scene_io_tmp/legacy.mgscene";
    FILE* f = fopen(path, "wb");
    CHECK(f != NULL);
    fprintf(f,
        "mgescene 1\n"
        "name \"legacy\"\n"
        "object \"Box\"\n"
        "  primitive cube\n"
        "  rotation 0 90 0\n"
        "  scale 1 1 1\n");
    fclose(f);

    Scene s;
    CHECK(Scene_Load(&s, path, NULL));
    CHECK(s.objectCount == 1);
    Quaternion want = Quaternion_FromEuler((Vector3){ 0.0f, 90.0f * DEG2RAD, 0.0f });
    CHECK(Quaternion_Approx(s.objects[0].transform.rotation, want));
    CHECK(shp(&s.objects[0]) != NULL && shp(&s.objects[0])->primitive == PRIM_CUBE); // legacy `primitive` => Shape

    remove(path);
    remove("scene_io_tmp/legacy.c");
}

// a legacy camera used a spherical yaw/pitch euler; it must still aim the same
// after being read as a quaternion
TEST(legacy_camera_euler_keeps_its_aim)
{
    Path_MakeDirs("scene_io_tmp");
    const char* path = "scene_io_tmp/legacycam.mgscene";
    FILE* f = fopen(path, "wb");
    CHECK(f != NULL);
    fprintf(f,
        "mgescene 1\n"
        "name \"lc\"\n"
        "object \"Cam\"\n"
        "  kind camera\n"
        "  rotation -8 -90 0\n"); // pitch -8, yaw -90 (old convention)
    fclose(f);

    Scene s;
    CHECK(Scene_Load(&s, path, NULL));
    CHECK(s.objects[0].kind == OBJECT_CAMERA);

    // old Mge_CameraObjectForward for {-8, -90, 0}
    float yaw = -90.0f * DEG2RAD, pitch = -8.0f * DEG2RAD;
    Vector3 want = { cosf(yaw) * cosf(pitch), sinf(pitch), sinf(yaw) * cosf(pitch) };
    // Mge_CameraObjectForward == the orientation applied to local -Z
    Vector3 got = Quaternion_RotateVector3(s.objects[0].transform.rotation, (Vector3){ 0, 0, -1 });
    CHECK(feq(got.x, want.x) && feq(got.y, want.y) && feq(got.z, want.z));

    remove(path);
    remove("scene_io_tmp/lc.c");
}

TEST(load_rejects_a_non_scene_file)
{
    FILE* f = fopen("scene_io_tmp/bogus.mgscene", "wb");
    CHECK(f != NULL);
    fprintf(f, "not a scene\n");
    fclose(f);

    Scene s;
    CHECK(!Scene_Load(&s, "scene_io_tmp/bogus.mgscene", NULL));
    remove("scene_io_tmp/bogus.mgscene");
}

// in the project layout the file is always `scene.mgscene`; the name comes from
// the containing folder, not the stem.
TEST(canonical_scene_file_takes_its_name_from_the_folder)
{
    Path_MakeDirs("scene_io_tmp/scenes/level1");
    const char* path = "scene_io_tmp/scenes/level1/scene.mgscene";

    Scene a;
    build_scene(&a);
    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 60.0f };
    CHECK(Scene_Save(&a, path, cam, "scene_io_tmp"));
    CHECK(strcmp(a.name, "level1") == 0);
    // texture ends up under the PROJECT res/, stored root-relative
    CHECK(strcmp(a.texPath[1][MATERIAL_MAP_DIFFUSE], "res/ball_albedo.png") == 0);

    Scene b;
    CHECK(Scene_Load(&b, path, NULL));
    CHECK(strcmp(b.name, "level1") == 0);

    remove(path);
    remove("scene_io_tmp/scenes/level1/level1.c");
    RMDIR("scene_io_tmp/scenes/level1");
    RMDIR("scene_io_tmp/scenes");
}

// Textures the user assigns come either from the Resources panel (a
// "res/..."-relative path, subfolders allowed) or the file picker (a path
// outside res/). On save, only the latter is imported into res/ root; a path
// already under res/ is left exactly where it is -- no flatten, no copy.
TEST(save_keeps_res_paths_and_imports_only_outside_files)
{
    Path_MakeDirs("scene_io_tmp/res/textures");
    FILE* f = fopen("scene_io_tmp/res/textures/wall.png", "wb");
    CHECK(f != NULL); fputs("PNG", f); fclose(f);

    Path_MakeDirs("scene_io_tmp/loose");
    f = fopen("scene_io_tmp/loose/pick.png", "wb");
    CHECK(f != NULL); fputs("PNG", f); fclose(f);

    Scene a;
    build_scene(&a);
    strcpy(a.texPath[0][MATERIAL_MAP_DIFFUSE], "res/textures/wall.png"); // subfolder -> keep
    strcpy(a.texPath[2][MATERIAL_MAP_NORMAL], "loose/pick.png");         // outside res/ -> import

    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 60.0f };
    CHECK(Scene_Save(&a, "scene_io_tmp/keep.mgscene", cam, "scene_io_tmp"));

    CHECK(strcmp(a.texPath[0][MATERIAL_MAP_DIFFUSE], "res/textures/wall.png") == 0);
    CHECK(Path_MTime("scene_io_tmp/res/wall.png") == 0);  // NOT flattened into res/ root
    CHECK(strcmp(a.texPath[2][MATERIAL_MAP_NORMAL], "res/pick.png") == 0);
    CHECK(Path_MTime("scene_io_tmp/res/pick.png") != 0);  // imported

    remove("scene_io_tmp/keep.mgscene");
    remove("scene_io_tmp/keep.c");
    remove("scene_io_tmp/res/textures/wall.png");
    remove("scene_io_tmp/res/pick.png");
    remove("scene_io_tmp/loose/pick.png");
    RMDIR("scene_io_tmp/res/textures");
    RMDIR("scene_io_tmp/loose");
}

// wireframe flag, the new primitives and a polygon's point list round-trip
TEST(shape_variants_round_trip)
{
    Path_MakeDirs("scene_io_tmp");
    const char* path = "scene_io_tmp/shapes.mgscene";

    Scene a;
    memset(&a, 0, sizeof(a));
    a.objects[0] = Mge_MakeShape3D(PRIM_CUBE, (Vector3){ 0, 0, 0 }, (Vector3){ 1, 1, 1 }, WHITE);
    shp(&a.objects[0])->wireframe = true;
    strcpy(a.objectNames[0], "Box 1");

    a.objects[1] = Mge_MakeShape3D(PRIM_ARROW, (Vector3){ 2, 0, 0 }, (Vector3){ 2, 1, 1 }, WHITE);
    strcpy(a.objectNames[1], "Arrow 1");

    a.objects[2] = Mge_MakeShape3D(PRIM_POLYGON, (Vector3){ -2, 0, 0 }, (Vector3){ 1, 1, 1 }, WHITE);
    {
        Shape* p = shp(&a.objects[2]);
        p->polyStrip = true;
        p->poly[0] = (Vector3){ -1, -1, 0 };
        p->poly[1] = (Vector3){ 1, -1, 0 };
        p->poly[2] = (Vector3){ -1, 1, 0 };
        p->poly[3] = (Vector3){ 1, 1, 0.5f };
        p->polyCount = 4;
    }
    strcpy(a.objectNames[2], "Polygon 1");
    a.objectCount = 3;

    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 60.0f };
    CHECK(Scene_Save(&a, path, cam, NULL)); // no project root -> no texture import

    Scene b;
    CHECK(Scene_Load(&b, path, NULL));
    CHECK(b.objectCount == 3);
    CHECK(shp(&b.objects[0])->primitive == PRIM_CUBE && shp(&b.objects[0])->wireframe);
    CHECK(shp(&b.objects[1])->primitive == PRIM_ARROW && !shp(&b.objects[1])->wireframe);
    CHECK(shp(&b.objects[2])->primitive == PRIM_POLYGON);
    CHECK(shp(&b.objects[2])->polyStrip);
    CHECK(shp(&b.objects[2])->polyCount == 4);
    CHECK_F(shp(&b.objects[2])->poly[3].x, 1.0f);
    CHECK_F(shp(&b.objects[2])->poly[3].z, 0.5f);

    remove(path);
    remove("scene_io_tmp/shapes.c");
}

// Collider + RigidBody components survive the .mgscene round-trip
TEST(physics_components_round_trip)
{
    Path_MakeDirs("scene_io_tmp");
    const char* path = "scene_io_tmp/phys.mgscene";

    Scene a;
    memset(&a, 0, sizeof(a));
    a.objects[0] = Mge_MakeShape3D(PRIM_CUBE, (Vector3){ 0, 3, 0 }, (Vector3){ 2, 2, 2 }, WHITE);
    strcpy(a.objectNames[0], "Crate");
    Collider* col = Mge_AddComponent(&a.objects[0], COMPONENT_COLLIDER);
    col->kind = COLLIDER_SPHERE;
    col->offset = (Vector3){ 0.1f, 0.2f, 0.3f };
    col->size = (Vector3){ 1.5f, 0, 0 };
    col->isTrigger = true;
    RigidBody* rb = Mge_AddComponent(&a.objects[0], COMPONENT_RIGIDBODY);
    rb->mass = 4.0f;
    rb->restitution = 0.25f;
    rb->useGravity = false;

    // a component-less object still round-trips as an empty node
    a.objects[1] = Mge_MakeShape3D(PRIM_CUBE, (Vector3){ 0, 0, 0 }, (Vector3){ 1, 1, 1 }, WHITE);
    Mge_RemoveComponent(&a.objects[1], COMPONENT_SHAPE);
    Mge_RemoveComponent(&a.objects[1], COMPONENT_MATERIAL);
    strcpy(a.objectNames[1], "Empty");
    a.objectCount = 2;
    a.showColliders = true;

    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 60.0f };
    CHECK(Scene_Save(&a, path, cam, NULL));

    Scene b;
    CHECK(Scene_Load(&b, path, NULL));
    CHECK(b.objectCount == 2);
    CHECK(b.showColliders);

    Collider* bc = Mge_GetColliderComponent(&b.objects[0]);
    RigidBody* br = Mge_GetRigidBodyComponent(&b.objects[0]);
    CHECK(bc != NULL && br != NULL);
    CHECK(bc->kind == COLLIDER_SPHERE);
    CHECK_F(bc->offset.z, 0.3f);
    CHECK_F(bc->size.x, 1.5f);
    CHECK(bc->isTrigger);
    CHECK_F(br->mass, 4.0f);
    CHECK_F(br->restitution, 0.25f);
    CHECK(!br->useGravity);

    CHECK(!Mge_HasComponent(&b.objects[1], COMPONENT_SHAPE));
    CHECK(!Mge_HasComponent(&b.objects[1], COMPONENT_MATERIAL));
    CHECK(strcmp(b.objectNames[1], "Empty") == 0);

    remove(path);
    remove("scene_io_tmp/phys.c");
}

int main(void)
{
    RUN(path_helpers);
    RUN(fs_list_rename_remove);
    RUN(scene_mge_round_trip);
    RUN(legacy_euler_rotation_line);
    RUN(legacy_camera_euler_keeps_its_aim);
    RUN(load_rejects_a_non_scene_file);
    RUN(canonical_scene_file_takes_its_name_from_the_folder);
    RUN(save_keeps_res_paths_and_imports_only_outside_files);
    RUN(shape_variants_round_trip);
    RUN(physics_components_round_trip);

    RMDIR("scene_io_tmp/res"); // best-effort tidy-up
    RMDIR("scene_io_tmp");
    return test_summary();
}
