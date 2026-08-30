// Mesh struct handling (Mge_MakeMesh / Upload / Draw / Unload).
// No GL context: the MgeGL_* mesh backend is stubbed and records its calls.

#include <stdbool.h>
#include <string.h>

#include "mge.h"
#include "mge_gl.h"
#include "test.h" // mlib repo-wide harness

// ---- recording stub backend ----

static int g_uploadCalls, g_drawCalls, g_unloadCalls;
static int g_lastUploadVerts, g_lastUploadIndices;
static int g_batchedUploadCalls, g_batchedHadNormals, g_batchedHadUV;
static unsigned int g_lastDrawVao, g_lastDrawTex, g_lastDrawNormalTex;
static int g_lastDrawIndexCount;
static unsigned int g_lastUnload[3];

static void backend_reset(void)
{
    g_uploadCalls = g_drawCalls = g_unloadCalls = 0;
    g_lastUploadVerts = g_lastUploadIndices = 0;
    g_batchedUploadCalls = g_batchedHadNormals = g_batchedHadUV = 0;
    g_lastDrawVao = g_lastDrawTex = g_lastDrawNormalTex = 0;
    g_lastDrawIndexCount = 0;
    memset(g_lastUnload, 0, sizeof(g_lastUnload));
}

void MgeGL_UploadMeshBatched(unsigned int* vao, unsigned int* vbo, unsigned int* ebo,
    const void* positions, const void* normals, const void* texcoords,
    int vertexCount, const unsigned int* indices, int indexCount)
{
    (void)positions;
    (void)indices;
    g_batchedUploadCalls++;
    g_batchedHadNormals = (normals != NULL);
    g_batchedHadUV = (texcoords != NULL);
    g_lastUploadVerts = vertexCount;
    g_lastUploadIndices = indexCount;
    *vao = 20;
    *vbo = 21;
    *ebo = 22;
}

void MgeGL_UploadMesh(unsigned int* vao, unsigned int* vbo, unsigned int* ebo,
    const void* vertices, int vertexCount, const unsigned int* indices, int indexCount)
{
    (void)vertices;
    (void)indices;
    g_uploadCalls++;
    g_lastUploadVerts = vertexCount;
    g_lastUploadIndices = indexCount;
    *vao = 10;
    *vbo = 11;
    *ebo = 12;
}

void MgeGL_DrawMesh(unsigned int vao, int indexCount, unsigned int textureId, unsigned int normalTextureId)
{
    g_drawCalls++;
    g_lastDrawVao = vao;
    g_lastDrawIndexCount = indexCount;
    g_lastDrawTex = textureId;
    g_lastDrawNormalTex = normalTextureId;
}

void MgeGL_UnloadMesh(unsigned int vao, unsigned int vbo, unsigned int ebo)
{
    g_unloadCalls++;
    g_lastUnload[0] = vao;
    g_lastUnload[1] = vbo;
    g_lastUnload[2] = ebo;
}

// ---- fixtures ----

static Vertex tri_verts[3] = {
    { { 0, 0, 0 }, { 0, 0, 1 }, { 0, 0 } },
    { { 1, 0, 0 }, { 0, 0, 1 }, { 1, 0 } },
    { { 0, 1, 0 }, { 0, 0, 1 }, { 0, 1 } },
};
static unsigned int tri_idx[3] = { 0, 1, 2 };

// ---- tests ----

TEST(make_mesh_deep_copies)
{
    Vertex v[3];
    memcpy(v, tri_verts, sizeof(v));
    unsigned int idx[3] = { 0, 1, 2 };
    MeshTexture tex[1] = { { .texture = { .id = 7 }, .type = MESH_TEXTURE_DIFFUSE } };

    Mesh m = Mge_MakeMesh(v, 3, idx, 3, tex, 1);

    CHECK(m.vertexCount == 3 && m.indexCount == 3 && m.textureCount == 1);
    CHECK(m.vertices != NULL && m.vertices != v);   // copied, not aliased
    CHECK(m.indices != NULL && m.indices != idx);
    CHECK(m.vao == 0 && m.vbo == 0 && m.ebo == 0);  // not on the GPU yet

    // mutating the caller's arrays must not touch the mesh
    v[0].position.x = 99.0f;
    idx[0] = 42;
    CHECK(m.vertices[0].position.x == 0.0f);
    CHECK(m.indices[0] == 0);
    CHECK(m.textures[0].texture.id == 7);

    Mge_UnloadMesh(&m);
}

TEST(make_mesh_handles_empty_parts)
{
    Mesh m = Mge_MakeMesh(tri_verts, 3, tri_idx, 3, NULL, 0);
    CHECK(m.textureCount == 0 && m.textures == NULL);
    CHECK(m.vertexCount == 3);

    Mesh z = Mge_MakeMesh(NULL, 0, NULL, 0, NULL, 0);
    CHECK(z.vertices == NULL && z.indices == NULL && z.vertexCount == 0);

    Mge_UnloadMesh(&m);
    Mge_UnloadMesh(&z);
}

TEST(upload_sets_handles_once)
{
    backend_reset();
    Mesh m = Mge_MakeMesh(tri_verts, 3, tri_idx, 3, NULL, 0);

    Mge_UploadMesh(&m);
    CHECK(g_uploadCalls == 1);
    CHECK(m.vao == 10 && m.vbo == 11 && m.ebo == 12);
    CHECK(g_lastUploadVerts == 3 && g_lastUploadIndices == 3);

    Mge_UploadMesh(&m); // already uploaded -> no second backend call
    CHECK(g_uploadCalls == 1);

    Mge_UnloadMesh(&m);
}

TEST(upload_skips_incomplete_mesh)
{
    backend_reset();
    Mesh m = Mge_MakeMesh(tri_verts, 3, NULL, 0, NULL, 0); // no indices

    Mge_UploadMesh(&m);
    CHECK(g_uploadCalls == 0);
    CHECK(m.vao == 0);

    Mge_UnloadMesh(&m);
}

TEST(draw_uses_the_diffuse_texture)
{
    backend_reset();
    MeshTexture tex[2] = {
        { .texture = { .id = 5 }, .type = MESH_TEXTURE_SPECULAR },
        { .texture = { .id = 9 }, .type = MESH_TEXTURE_DIFFUSE },
    };
    Mesh m = Mge_MakeMesh(tri_verts, 3, tri_idx, 3, tex, 2);
    Mge_UploadMesh(&m);

    Mge_DrawMesh(m);
    CHECK(g_drawCalls == 1);
    CHECK(g_lastDrawVao == 10);
    CHECK(g_lastDrawIndexCount == 3);
    CHECK(g_lastDrawTex == 9); // the diffuse one, not the specular
    CHECK(g_lastDrawNormalTex == 0); // no normal map on this mesh

    Mge_UnloadMesh(&m);
}

TEST(draw_passes_the_normal_map)
{
    backend_reset();
    MeshTexture tex[2] = {
        { .texture = { .id = 9 }, .type = MESH_TEXTURE_DIFFUSE },
        { .texture = { .id = 4 }, .type = MESH_TEXTURE_NORMAL },
    };
    Mesh m = Mge_MakeMesh(tri_verts, 3, tri_idx, 3, tex, 2);
    Mge_UploadMesh(&m);

    Mge_DrawMesh(m);
    CHECK(g_lastDrawTex == 9);
    CHECK(g_lastDrawNormalTex == 4);

    Mge_UnloadMesh(&m);
}

TEST(draw_without_diffuse_passes_zero)
{
    backend_reset();
    MeshTexture tex[1] = { { .texture = { .id = 5 }, .type = MESH_TEXTURE_SPECULAR } };
    Mesh m = Mge_MakeMesh(tri_verts, 3, tri_idx, 3, tex, 1);
    Mge_UploadMesh(&m);

    Mge_DrawMesh(m);
    CHECK(g_lastDrawTex == 0); // falls back to white

    Mge_UnloadMesh(&m);
}

TEST(draw_before_upload_is_noop)
{
    backend_reset();
    Mesh m = Mge_MakeMesh(tri_verts, 3, tri_idx, 3, NULL, 0);

    Mge_DrawMesh(m); // vao == 0
    CHECK(g_drawCalls == 0);

    Mge_UnloadMesh(&m);
}

TEST(unload_frees_and_zeros)
{
    backend_reset();
    Mesh m = Mge_MakeMesh(tri_verts, 3, tri_idx, 3, NULL, 0);
    Mge_UploadMesh(&m);

    Mge_UnloadMesh(&m);
    CHECK(g_unloadCalls == 1);
    CHECK(g_lastUnload[0] == 10 && g_lastUnload[1] == 11 && g_lastUnload[2] == 12);
    CHECK(m.vertices == NULL && m.indices == NULL && m.textures == NULL);
    CHECK(m.vertexCount == 0 && m.indexCount == 0 && m.textureCount == 0);
    CHECK(m.vao == 0 && m.vbo == 0 && m.ebo == 0);

    Mge_UnloadMesh(&m); // second unload on a zeroed mesh is safe
    CHECK(g_unloadCalls == 2); // called again, but with zero handles (backend guards)
}

TEST(make_mesh_from_arrays_uses_the_batched_upload)
{
    Vector3 pos[3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 } };
    Vector3 nrm[3] = { { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 } };
    Vector2 uv[3] = { { 0, 0 }, { 1, 0 }, { 0, 1 } };
    unsigned int idx[3] = { 0, 1, 2 };
    backend_reset();

    Mesh m = Mge_MakeMeshFromArrays(pos, nrm, uv, 3, idx, 3, NULL, 0);
    CHECK(m.vertices == NULL);                 // not the interleaved form
    CHECK(m.positions != NULL && m.positions != pos);
    CHECK(m.normals != NULL && m.texcoords != NULL);
    CHECK(m.vertexCount == 3 && m.indexCount == 3);

    pos[0].x = 42.0f; // deep copy
    CHECK(m.positions[0].x == 0.0f);

    Mge_UploadMesh(&m);
    CHECK(g_batchedUploadCalls == 1 && g_uploadCalls == 0); // the batched path
    CHECK(g_batchedHadNormals && g_batchedHadUV);
    CHECK(m.vao == 20);

    Mge_UnloadMesh(&m);
    CHECK(m.positions == NULL && m.normals == NULL && m.texcoords == NULL);
}

TEST(make_mesh_from_arrays_optional_normals_texcoords)
{
    Vector3 pos[3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 } };
    unsigned int idx[3] = { 0, 1, 2 };
    backend_reset();

    Mesh m = Mge_MakeMeshFromArrays(pos, NULL, NULL, 3, idx, 3, NULL, 0);
    CHECK(m.positions != NULL && m.normals == NULL && m.texcoords == NULL);

    Mge_UploadMesh(&m);
    CHECK(g_batchedUploadCalls == 1);
    CHECK(!g_batchedHadNormals && !g_batchedHadUV);

    Mge_UnloadMesh(&m);
}

int main(void)
{
    RUN(make_mesh_deep_copies);
    RUN(make_mesh_handles_empty_parts);
    RUN(make_mesh_from_arrays_uses_the_batched_upload);
    RUN(make_mesh_from_arrays_optional_normals_texcoords);
    RUN(upload_sets_handles_once);
    RUN(upload_skips_incomplete_mesh);
    RUN(draw_uses_the_diffuse_texture);
    RUN(draw_passes_the_normal_map);
    RUN(draw_without_diffuse_passes_zero);
    RUN(draw_before_upload_is_noop);
    RUN(unload_frees_and_zeros);
    return test_summary();
}
