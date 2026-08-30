// Mge_LoadModel + the private Assimp->Mesh processor, exercised for real against
// a generated OBJ and (when present) assets/sliced_musk_melon/scene.gltf.
//
// Links the vendored Assimp, so it is NOT part of `make test`; run it with
//   make -C test model      (needs `make vendor` first)
//
// The GL mesh backend and texture loader are stubbed -- no window required.

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "mge.h"
#include "mge_gl.h"
#include "test.h"

// ---- stubs ----

static int g_drawCalls, g_unloadCalls, g_uploadCalls;

void MgeGL_UploadMesh(unsigned int* vao, unsigned int* vbo, unsigned int* ebo,
    const void* vertices, int vertexCount, const unsigned int* indices, int indexCount)
{
    (void)vertices; (void)indices; (void)vertexCount; (void)indexCount;
    g_uploadCalls++;
    *vao = 1; *vbo = 2; *ebo = 3;
}
void MgeGL_DrawMesh(unsigned int vao, int indexCount, unsigned int textureId)
{
    (void)vao; (void)indexCount; (void)textureId;
    g_drawCalls++;
}
void MgeGL_UnloadMesh(unsigned int vao, unsigned int vbo, unsigned int ebo)
{
    (void)vao; (void)vbo; (void)ebo;
    g_unloadCalls++;
}
Texture2D Mge_LoadTexture(const char* fileName)
{
    (void)fileName;
    return (Texture2D){ 0 };
}
void Trace_Log(int level, const char* text, ...)
{
    (void)level; (void)text;
}

static void reset(void) { g_drawCalls = g_unloadCalls = g_uploadCalls = 0; }

// ---- fixtures ----

static const char* OBJ_QUAD =
    "v -1 0 -1\n"
    "v  1 0 -1\n"
    "v  1 0  1\n"
    "v -1 0  1\n"
    "vn 0 1 0\n"
    "f 1//1 2//1 3//1\n"
    "f 1//1 3//1 4//1\n";

static const char* MELON = "../assets/sliced_musk_melon/scene.gltf";

static bool write_file(const char* name, const char* text)
{
    FILE* f = fopen(name, "wb");
    if (f == NULL)
        return false;
    fwrite(text, 1, strlen(text), f);
    fclose(f);
    return true;
}

static bool file_exists(const char* name)
{
    FILE* f = fopen(name, "rb");
    if (f == NULL)
        return false;
    fclose(f);
    return true;
}

static int total_vertices(Model m)
{
    int n = 0;
    for (int i = 0; i < m.meshCount; i++)
        n += m.meshes[i].vertexCount;
    return n;
}

// ---- tests ----

TEST(load_missing_returns_empty_model)
{
    reset();
    Model m = Mge_LoadModel("no/such/file.obj");
    CHECK(m.meshCount == 0);
    CHECK(m.meshes == NULL);
    Mge_UnloadModel(&m); // safe on an empty model
}

TEST(load_obj_quad)
{
    CHECK(write_file("_model_test.obj", OBJ_QUAD));
    reset();

    Model m = Mge_LoadModel("_model_test.obj");
    CHECK(m.meshCount == 1);
    CHECK(m.meshes[0].vertexCount == 4);   // shared corners are joined
    CHECK(m.meshes[0].indexCount == 6);    // two triangles
    CHECK(g_uploadCalls == 1);             // uploaded during load
    CHECK(m.meshes[0].vao != 0);

    // bounding box of the 2x2 quad in the xz plane
    CHECK(m.bboxMin.x <= -1.0f && m.bboxMax.x >= 1.0f);
    CHECK(m.bboxMin.z <= -1.0f && m.bboxMax.z >= 1.0f);

    // no slash in the path -> directory is "."
    CHECK(strcmp(m.directory, ".") == 0);

    Mge_UnloadModel(&m);
    CHECK(g_unloadCalls == 1);
    CHECK(m.meshCount == 0 && m.meshes == NULL);

    remove("_model_test.obj");
}

TEST(directory_is_taken_from_the_path)
{
    CHECK(write_file("_model_test.obj", OBJ_QUAD));

    Model m = Mge_LoadModel("./_model_test.obj");
    CHECK(strcmp(m.directory, ".") == 0);
    Mge_UnloadModel(&m);

    remove("_model_test.obj");

    if (file_exists(MELON)) {
        Model g = Mge_LoadModel(MELON);
        CHECK(strcmp(g.directory, "../assets/sliced_musk_melon") == 0);
        Mge_UnloadModel(&g);
    }
}

TEST(load_sliced_musk_melon_gltf)
{
    if (!file_exists(MELON)) {
        CHECK(true); // asset not present in this checkout -- skip
        return;
    }
    reset();

    Model m = Mge_LoadModel(MELON);
    CHECK(m.meshCount >= 1);
    CHECK(total_vertices(m) > 10000);          // it's a dense scan
    CHECK(g_uploadCalls == m.meshCount);       // every mesh uploaded

    float width = m.bboxMax.x - m.bboxMin.x;   // ~24 units across in x
    CHECK(width > 15.0f && width < 30.0f);
    CHECK(m.bboxMax.y > m.bboxMin.y);

    Mge_DrawModel(m);
    CHECK(g_drawCalls == m.meshCount);

    Mge_UnloadModel(&m);
    CHECK(g_unloadCalls == m.meshCount);
    CHECK(m.meshCount == 0);
}

int main(void)
{
    RUN(load_missing_returns_empty_model);
    RUN(load_obj_quad);
    RUN(directory_is_taken_from_the_path);
    RUN(load_sliced_musk_melon_gltf);
    return test_summary();
}
