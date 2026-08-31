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

static Material default_material(void)
{
    Material m = { 0 };
    m.shininess = 32.0f;
    m.tiling = (Vector2){ 1.0f, 1.0f };
    m.triplanarScale = 1.0f;
    for (int i = 0; i < MATERIAL_MAP_COUNT; i++) {
        m.maps[i].color = (Color){ 255, 255, 255, 255 };
        m.maps[i].value = (i == MATERIAL_MAP_NORMAL || i == MATERIAL_MAP_HEIGHT) ? 1.0f : 1.0f;
    }
    return m;
}

Object Mge_MakeShape3D(PrimitiveKind primitive, Vector3 position, Vector3 size, Color color)
{
    Object o = { 0 };
    o.kind = OBJECT_3D;
    o.active = true;
    o.primitive = primitive;
    o.transform.position = position;
    o.transform.scale = size;
    o.transform.parent = -1;
    o.material = default_material();
    o.material.maps[MATERIAL_MAP_DIFFUSE].color = color;
    return o;
}

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

static void build_scene(Scene* s)
{
    memset(s, 0, sizeof(*s));

    s->objects[0] = Mge_MakeShape3D(PRIM_PLANE, (Vector3){ 0, -1.1f, 0 }, (Vector3){ 24, 0.2f, 24 }, (Color){ 90, 95, 105, 255 });
    strcpy(s->objectNames[0], "Floor");

    s->objects[1] = Mge_MakeShape3D(PRIM_SPHERE, (Vector3){ 2.5f, 1.0f, -0.5f }, (Vector3){ 1.5f, 1.5f, 1.5f }, (Color){ 200, 80, 80, 255 });
    s->objects[1].transform.rotation = (Vector3){ 10, 20, 30 };
    s->objects[1].active = false;
    s->objects[1].material.shininess = 64.0f;
    s->objects[1].material.triplanar = true;
    s->objects[1].material.tiling = (Vector2){ 2, 3 };
    strcpy(s->objectNames[1], "Ball");
    strcpy(s->texPath[1][MATERIAL_MAP_DIFFUSE], "res/ball_albedo.png");
    s->texWrap[1][MATERIAL_MAP_DIFFUSE] = 2;

    s->objects[2] = Mge_MakeShape3D(PRIM_CUBE, (Vector3){ 1, 2, 3 }, (Vector3){ 1, 1, 1 }, (Color){ 255, 255, 255, 255 });
    s->objects[2].kind = OBJECT_CAMERA;
    s->objects[2].transform.rotation = (Vector3){ -5, -90, 15 };
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
    CHECK(b.objects[0].primitive == PRIM_PLANE);
    CHECK(b.objects[1].primitive == PRIM_SPHERE);

    CHECK(b.objects[0].kind == OBJECT_3D);
    CHECK(b.objects[2].kind == OBJECT_CAMERA);
    CHECK(strcmp(b.objectNames[2], "GameCam") == 0);
    CHECK_F(b.objects[2].transform.rotation.y, -90.0f);
    CHECK_F(b.objects[2].transform.rotation.z, 15.0f);
    CHECK(b.mainCamera == 2);
    CHECK(strcmp(b.skyDir, "res/sky_night") == 0);
    CHECK(b.objects[1].transform.parent == 0);  // parent link round-trips
    CHECK(b.objects[0].transform.parent == -1); // default when absent
    CHECK(b.objects[1].active == false);
    CHECK_F(b.objects[1].transform.position.x, 2.5f);
    CHECK_F(b.objects[1].transform.rotation.y, 20.0f);
    CHECK_F(b.objects[1].transform.scale.z, 1.5f);
    CHECK_F(b.objects[1].material.shininess, 64.0f);
    CHECK(b.objects[1].material.triplanar == true);
    CHECK_F(b.objects[1].material.tiling.y, 3.0f);
    CHECK(b.objects[0].material.maps[MATERIAL_MAP_DIFFUSE].color.r == 90);
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

int main(void)
{
    RUN(path_helpers);
    RUN(fs_list_rename_remove);
    RUN(scene_mge_round_trip);
    RUN(load_rejects_a_non_scene_file);
    RUN(canonical_scene_file_takes_its_name_from_the_folder);

    RMDIR("scene_io_tmp/res"); // best-effort tidy-up
    RMDIR("scene_io_tmp");
    return test_summary();
}
