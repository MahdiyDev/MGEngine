// Mesh: an interleaved vertex array + 32-bit indices + a texture list, backed by
// its own GPU buffers. The struct handling lives here; the actual GL calls are
// in mge_gl.c (MgeGL_UploadMesh / MgeGL_DrawMesh / MgeGL_UnloadMesh).

#include "mge.h"
#include "mge_gl.h"

#include <stdlib.h>
#include <string.h>

// the interleaved layout MgeGL_UploadMesh expects: 3 + 3 + 2 floats, no padding
_Static_assert(sizeof(Vertex) == 8 * sizeof(float), "Vertex must be tightly packed");

static void* mem_dup(const void* src, size_t bytes)
{
    if (src == NULL || bytes == 0)
        return NULL;
    void* copy = malloc(bytes);
    if (copy != NULL)
        memcpy(copy, src, bytes);
    return copy;
}

static void copy_index_and_texture_arrays(Mesh* mesh, const unsigned int* indices, int indexCount,
    const MeshTexture* textures, int textureCount)
{
    if (indexCount > 0) {
        mesh->indices = mem_dup(indices, (size_t)indexCount * sizeof(unsigned int));
        mesh->indexCount = (mesh->indices != NULL) ? indexCount : 0;
    }
    if (textureCount > 0) {
        mesh->textures = mem_dup(textures, (size_t)textureCount * sizeof(MeshTexture));
        mesh->textureCount = (mesh->textures != NULL) ? textureCount : 0;
    }
}

Mesh Mge_MakeMesh(const Vertex* vertices, int vertexCount,
    const unsigned int* indices, int indexCount,
    const MeshTexture* textures, int textureCount)
{
    Mesh mesh = { 0 };

    if (vertexCount > 0) {
        mesh.vertices = mem_dup(vertices, (size_t)vertexCount * sizeof(Vertex));
        mesh.vertexCount = (mesh.vertices != NULL) ? vertexCount : 0;
    }
    copy_index_and_texture_arrays(&mesh, indices, indexCount, textures, textureCount);
    return mesh;
}

Mesh Mge_MakeMeshFromArrays(const Vector3* positions, const Vector3* normals,
    const Vector2* texcoords, int vertexCount,
    const unsigned int* indices, int indexCount,
    const MeshTexture* textures, int textureCount)
{
    Mesh mesh = { 0 };

    if (vertexCount > 0 && positions != NULL) {
        mesh.positions = mem_dup(positions, (size_t)vertexCount * sizeof(Vector3));
        mesh.normals = mem_dup(normals, (size_t)vertexCount * sizeof(Vector3));
        mesh.texcoords = mem_dup(texcoords, (size_t)vertexCount * sizeof(Vector2));
        mesh.vertexCount = (mesh.positions != NULL) ? vertexCount : 0;
    }
    copy_index_and_texture_arrays(&mesh, indices, indexCount, textures, textureCount);
    return mesh;
}

void Mge_UploadMesh(Mesh* mesh)
{
    if (mesh == NULL || mesh->vao != 0)
        return; // nothing to do / already on the GPU
    if (mesh->vertexCount <= 0 || mesh->indexCount <= 0)
        return;

    if (mesh->vertices != NULL) {
        MgeGL_UploadMesh(&mesh->vao, &mesh->vbo, &mesh->ebo,
            mesh->vertices, mesh->vertexCount, mesh->indices, mesh->indexCount);
    } else if (mesh->positions != NULL) {
        MgeGL_UploadMeshBatched(&mesh->vao, &mesh->vbo, &mesh->ebo,
            mesh->positions, mesh->normals, mesh->texcoords,
            mesh->vertexCount, mesh->indices, mesh->indexCount);
    }
}

static unsigned int diffuse_texture_id(Mesh mesh)
{
    for (int i = 0; i < mesh.textureCount; i++) {
        if (mesh.textures[i].type == MESH_TEXTURE_DIFFUSE)
            return mesh.textures[i].texture.id;
    }
    return 0; // -> white 1x1
}

void Mge_DrawMesh(Mesh mesh)
{
    if (mesh.vao == 0)
        return; // not uploaded

    MgeGL_DrawMesh(mesh.vao, mesh.indexCount, diffuse_texture_id(mesh));
}

void Mge_UnloadMesh(Mesh* mesh)
{
    if (mesh == NULL)
        return;

    MgeGL_UnloadMesh(mesh->vao, mesh->vbo, mesh->ebo);
    free(mesh->vertices);
    free(mesh->positions);
    free(mesh->normals);
    free(mesh->texcoords);
    free(mesh->indices);
    free(mesh->textures);

    *mesh = (Mesh){ 0 };
}
