// Model loading via Assimp: a file on disk -> a flat list of GPU-ready Mesh.
//
// The node hierarchy is walked once; each node's transform is accumulated and
// baked into its meshes' vertex positions/normals (the engine has no per-object
// model matrix). Textures are resolved relative to the model's directory and
// de-duplicated within a single load.

#include "mge.h"
#include "mge_gl.h"
#include "mge_utils.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------- per-load texture cache ----------

#define MODEL_MAX_TEXTURES 16

typedef struct {
    char path[1024]; // fits an Assimp aiString (AI_MAXLEN)
    Texture2D texture;
} CachedTexture;

typedef struct {
    CachedTexture items[MODEL_MAX_TEXTURES];
    int count;
} TextureCache;

static Texture2D cache_get(TextureCache* cache, const char* dir, const char* rel, bool sRGB)
{
    for (int i = 0; i < cache->count; i++) {
        if (strcmp(cache->items[i].path, rel) == 0)
            return cache->items[i].texture;
    }

    char full[sizeof(((CachedTexture*)0)->path) + 520];
    snprintf(full, sizeof(full), "%s/%s", dir, rel);
    // colour/albedo maps are authored in sRGB; specular and other data maps aren't
    Texture2D tex = Mge_LoadTextureEx(full, sRGB);

    if (cache->count < MODEL_MAX_TEXTURES) {
        snprintf(cache->items[cache->count].path, sizeof(cache->items[0].path), "%s", rel);
        cache->items[cache->count].texture = tex;
        cache->count++;
    }
    return tex;
}

// ---------- material -> MeshTexture[] ----------

static int collect_textures(const struct aiMaterial* mat, enum aiTextureType aiType,
    MeshTextureType meshType, const char* dir, TextureCache* cache,
    MeshTexture* out, int outCap)
{
    int n = 0;
    unsigned int total = aiGetMaterialTextureCount(mat, aiType);

    for (unsigned int i = 0; i < total && n < outCap; i++) {
        struct aiString path;
        if (aiGetMaterialTexture(mat, aiType, i, &path, NULL, NULL, NULL, NULL, NULL, NULL) != aiReturn_SUCCESS)
            continue;
        out[n].texture = cache_get(cache, dir, path.data, meshType == MESH_TEXTURE_DIFFUSE);
        out[n].type = meshType;
        n++;
    }
    return n;
}

// ---------- aiMesh -> Mesh ----------

static Vector3 xform_point(const struct aiMatrix4x4* m, struct aiVector3D v)
{
    aiTransformVecByMatrix4(&v, m);
    return (Vector3){ v.x, v.y, v.z };
}

static Vector3 xform_normal(const struct aiMatrix4x4* m, struct aiVector3D n)
{
    // rotate/scale only -- ignore translation (fine for the rigid/uniform-scale
    // node transforms glTF/OBJ/FBX use here)
    return (Vector3){
        m->a1 * n.x + m->a2 * n.y + m->a3 * n.z,
        m->b1 * n.x + m->b2 * n.y + m->b3 * n.z,
        m->c1 * n.x + m->c2 * n.y + m->c3 * n.z,
    };
}

static Mesh process_mesh(const struct aiMesh* am, const struct aiScene* scene,
    const char* dir, TextureCache* cache, const struct aiMatrix4x4* xform)
{
    const int vcount = (int)am->mNumVertices;

    Vertex* verts = (Vertex*)calloc((size_t)(vcount > 0 ? vcount : 1), sizeof(Vertex));
    for (int i = 0; i < vcount; i++) {
        verts[i].position = xform_point(xform, am->mVertices[i]);
        if (am->mNormals != NULL)
            verts[i].normal = xform_normal(xform, am->mNormals[i]);
        if (am->mTextureCoords[0] != NULL) {
            // Assimp normalises every format's UVs to a bottom-left V origin,
            // but the engine uploads textures top row first (V grows downward),
            // so flip V back here.
            verts[i].texcoord.x = am->mTextureCoords[0][i].x;
            verts[i].texcoord.y = 1.0f - am->mTextureCoords[0][i].y;
        }
    }

    int icount = 0;
    for (unsigned int f = 0; f < am->mNumFaces; f++)
        icount += (int)am->mFaces[f].mNumIndices;

    unsigned int* indices = (unsigned int*)malloc((size_t)(icount > 0 ? icount : 1) * sizeof(unsigned int));
    int k = 0;
    for (unsigned int f = 0; f < am->mNumFaces; f++) {
        const struct aiFace* face = &am->mFaces[f];
        for (unsigned int j = 0; j < face->mNumIndices; j++)
            indices[k++] = face->mIndices[j];
    }

    MeshTexture tex[8];
    int tcount = 0;
    if (am->mMaterialIndex < scene->mNumMaterials) {
        const struct aiMaterial* mat = scene->mMaterials[am->mMaterialIndex];
        tcount += collect_textures(mat, aiTextureType_BASE_COLOR, MESH_TEXTURE_DIFFUSE, dir, cache, tex + tcount, 8 - tcount);
        if (tcount == 0)
            tcount += collect_textures(mat, aiTextureType_DIFFUSE, MESH_TEXTURE_DIFFUSE, dir, cache, tex + tcount, 8 - tcount);
        tcount += collect_textures(mat, aiTextureType_SPECULAR, MESH_TEXTURE_SPECULAR, dir, cache, tex + tcount, 8 - tcount);
    }

    Mesh mesh = Mge_MakeMesh(verts, vcount, indices, icount, tex, tcount);
    free(verts);
    free(indices);
    return mesh;
}

// ---------- node recursion ----------

typedef struct {
    Mesh* items;
    int count, cap;
} MeshList;

static void list_push(MeshList* l, Mesh m)
{
    if (l->count == l->cap) {
        l->cap = (l->cap != 0) ? l->cap * 2 : 8;
        l->items = (Mesh*)realloc(l->items, (size_t)l->cap * sizeof(Mesh));
    }
    l->items[l->count++] = m;
}

static void process_node(MeshList* list, const struct aiNode* node, const struct aiScene* scene,
    const char* dir, TextureCache* cache, struct aiMatrix4x4 parent)
{
    struct aiMatrix4x4 global = parent;
    aiMultiplyMatrix4(&global, &node->mTransformation); // global = parent * local

    for (unsigned int i = 0; i < node->mNumMeshes; i++)
        list_push(list, process_mesh(scene->mMeshes[node->mMeshes[i]], scene, dir, cache, &global));

    for (unsigned int i = 0; i < node->mNumChildren; i++)
        process_node(list, node->mChildren[i], scene, dir, cache, global);
}

// ---------- public ----------

static void extract_directory(const char* path, char* out, size_t outSize)
{
    const char* fwd = strrchr(path, '/');
    const char* bwd = strrchr(path, '\\');
    const char* cut = (fwd > bwd) ? fwd : bwd;

    if (cut != NULL) {
        size_t n = (size_t)(cut - path);
        if (n >= outSize)
            n = outSize - 1;
        memcpy(out, path, n);
        out[n] = '\0';
    } else {
        snprintf(out, outSize, ".");
    }
}

Model Mge_LoadModel(const char* path)
{
    Model model = { 0 };

    const struct aiScene* scene = aiImportFile(path,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);

    if (scene == NULL || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || scene->mRootNode == NULL) {
        TRACE_LOG(LOG_WARNING, "MODEL: [%s] load failed: %s", path, aiGetErrorString());
        if (scene != NULL)
            aiReleaseImport(scene);
        return model;
    }

    extract_directory(path, model.directory, sizeof(model.directory));

    TextureCache cache = { 0 };
    MeshList list = { 0 };
    struct aiMatrix4x4 identity;
    aiIdentityMatrix4(&identity);
    process_node(&list, scene->mRootNode, scene, model.directory, &cache, identity);

    aiReleaseImport(scene);

    model.meshes = list.items;
    model.meshCount = list.count;

    // bounds + GPU upload
    int first = 1;
    for (int m = 0; m < model.meshCount; m++) {
        for (int v = 0; v < model.meshes[m].vertexCount; v++) {
            Vector3 p = model.meshes[m].vertices[v].position;
            if (first) {
                model.bboxMin = model.bboxMax = p;
                first = 0;
            } else {
                if (p.x < model.bboxMin.x) model.bboxMin.x = p.x;
                if (p.y < model.bboxMin.y) model.bboxMin.y = p.y;
                if (p.z < model.bboxMin.z) model.bboxMin.z = p.z;
                if (p.x > model.bboxMax.x) model.bboxMax.x = p.x;
                if (p.y > model.bboxMax.y) model.bboxMax.y = p.y;
                if (p.z > model.bboxMax.z) model.bboxMax.z = p.z;
            }
        }
        Mge_UploadMesh(&model.meshes[m]);
    }

    TRACE_LOG(LOG_INFO, "MODEL: [%s] loaded %d mesh(es)", path, model.meshCount);
    return model;
}

void Mge_DrawModel(Model model)
{
    for (int i = 0; i < model.meshCount; i++)
        Mge_DrawMesh(model.meshes[i]);
}

void Mge_UnloadModel(Model* model)
{
    if (model == NULL)
        return;

    for (int i = 0; i < model->meshCount; i++)
        Mge_UnloadMesh(&model->meshes[i]);
    free(model->meshes);

    *model = (Model){ 0 };
}
