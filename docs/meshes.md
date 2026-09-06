# Meshes, models & skyboxes

## Mesh

`Draw_Cube` & co. push vertices through the immediate-mode batch every frame. A
`Mesh` is the retained alternative: your own vertex + index arrays uploaded to a
GPU buffer once, then drawn with a single call.

```c
typedef struct Vertex {
    Vector3 position;   // world space -- there is no per-mesh transform
    Vector3 normal;     // for lighting; zero is fine unlit
    Vector2 texcoord;
} Vertex;

typedef struct MeshTexture { Texture2D texture; MeshTextureType type; } MeshTexture;
// MESH_TEXTURE_DIFFUSE -> sampled as the surface colour
// MESH_TEXTURE_SPECULAR -> stored on the mesh, not sampled by the built-in shader yet
// MESH_TEXTURE_NORMAL  -> tangent-space normal map, applied when the mesh is lit

Mesh Mge_MakeMesh(const Vertex* v, int vc, const unsigned int* idx, int ic,
                  const MeshTexture* tex, int tc);   // copies all three arrays
void Mge_UploadMesh(Mesh* m);   // create the GPU buffers (once)
void Mge_DrawMesh(Mesh m);      // inside Mge_BeginMode3D (+ Mge_BeginLighting3D for lighting)
void Mge_UnloadMesh(Mesh* m);   // free GPU + CPU, zero the struct
```

```c
Vertex verts[4] = {
    { { -1, 0, 0 }, { 0, 0, 1 }, { 0, 0 } }, { { 1, 0, 0 }, { 0, 0, 1 }, { 1, 0 } },
    { {  1, 2, 0 }, { 0, 0, 1 }, { 1, 1 } }, { { -1, 2, 0 }, { 0, 0, 1 }, { 0, 1 } },
};
unsigned int idx[6] = { 0, 1, 2, 0, 2, 3 };
MeshTexture tex[1] = { { Mge_LoadTexture("assets/wall.jpg"), MESH_TEXTURE_DIFFUSE } };

Mesh quad = Mge_MakeMesh(verts, 4, idx, 6, tex, 1);
Mge_UploadMesh(&quad);

while (!Mge_WindowShouldClose()) {
    Mge_BeginDrawing();
    Mge_BeginMode3D(camera);
        Mge_BeginLighting3D(light, camera);
            Mge_DrawMesh(quad);
        Mge_EndLighting3D();
    Mge_EndMode3D();
    Mge_EndDrawing();
}
Mge_UnloadMesh(&quad);
```

Indices are `unsigned int` (32-bit), 3 per triangle. `Mge_DrawMesh` binds the
first `MESH_TEXTURE_DIFFUSE` texture (or a white 1×1 if there is none), plus the
first `MESH_TEXTURE_NORMAL` if present, and draws with whatever shader is active
— the unlit default or the lighting shader. It has no colour attribute, so the
diffuse texture is shown untinted.

**Batched vertex attributes.** If you'd rather keep positions / normals /
texcoords in separate arrays than interleave them into `Vertex[]`, use

```c
Mesh Mge_MakeMeshFromArrays(const Vector3* positions, const Vector3* normals,
    const Vector2* texcoords, int vertexCount,
    const unsigned int* indices, int indexCount,
    const MeshTexture* textures, int textureCount);   // normals / texcoords may be NULL
```

On upload these become **one VBO** filled block-by-block (all positions, then
all normals, then all texcoords) with `glBufferSubData` — LearnOpenGL's
"batching vertex attributes". The VAO records the offsets, so `Mge_DrawMesh` /
`Mge_UnloadMesh` are unchanged.

Demos: `examples/meshes/textured_quad.c` (interleaved) and
`batched_attributes.c` (separate arrays).

## Model

`Mge_LoadModel` runs a file through the vendored [Assimp](https://github.com/assimp/assimp)
(OBJ / glTF2 / FBX in this build) and returns a flat list of GPU-ready meshes.

```c
typedef struct Model {
    Mesh* meshes;   int meshCount;
    char  directory[512];       // where the file (and its textures) live
    Vector3 bboxMin, bboxMax;   // bounds over every vertex
} Model;

Model Mge_LoadModel(const char* path);
void  Mge_DrawModel(Model model);    // draws every mesh; inside Mge_BeginMode3D
void  Mge_UnloadModel(Model* model);
```

```c
Model melon = Mge_LoadModel("assets/sliced_musk_melon/scene.gltf");

Vector3 c = Vector3_Scale(Vector3_Add(melon.bboxMin, melon.bboxMax), 0.5f);
float r = Vector3_Length(Vector3_Subtract(melon.bboxMax, melon.bboxMin)) * 0.5f;
// ... position the camera at c + (0, r*0.25, r*2.2), looking at c ...

Mge_BeginMode3D(camera);
    Mge_BeginLighting3D(light, camera);
        Mge_DrawModel(melon);
    Mge_EndLighting3D();
Mge_EndMode3D();

Mge_UnloadModel(&melon);
```

The private processor (in `mge_model.c`) walks the Assimp node tree once,
**bakes each node's transform into its meshes' vertices** (the engine has no
per-object matrix), copies positions / normals / the first UV set into `Vertex`,
flattens the faces into a 32-bit index array, and loads each material's
base-colour / diffuse texture from `directory` (de-duplicated per load). Every
mesh is uploaded to the GPU before `Mge_LoadModel` returns.

Demo: `examples/models/load_melon.c` — loads `assets/sliced_musk_melon/`, frames
it from its bounding box, orbits a point light around it. Needs `make vendor`.


## Cube maps, skybox & environment mapping

A `Cubemap` is six square textures sampled by a 3D direction.

```c
Cubemap Mge_LoadCubemap(const char* facePaths[6]);  // GL order: +X -X +Y -Y +Z -Z
Cubemap Mge_LoadCubemapDir(const char* dir);         // dir/{right,left,top,bottom,front,back}.jpg
void    Mge_UnloadCubemap(Cubemap);
```

**Skybox** — `Mge_DrawSkybox(cubemap, camera)` draws a camera-locked cube of the
map. Call it **last** inside `Mge_BeginMode3D` (its depth is forced to 1.0, so it
only fills pixels the scene didn't cover).

**Environment mapping** — geometry drawn between these samples the cube map by
the reflected or refracted view direction (needs per-vertex normals, so `Draw_Cube`
and meshes work, 2D shapes don't):

```c
Mge_BeginEnvironmentMap(cubemap, camera, ENVMAP_REFLECT, 0.0f);   // chrome
    Draw_Cube(pos, size, WHITE);
Mge_EndEnvironmentMap();

Mge_BeginEnvironmentMap(cubemap, camera, ENVMAP_REFRACT, 1.0f / 1.52f); // glass
    Draw_Cube(pos, size, WHITE);
Mge_EndEnvironmentMap();
```

**Dynamic environment maps** — render the live scene into a probe's cube map,
then reflect it:

```c
EnvProbe probe = Mge_LoadEnvProbe(256);
...
for (int f = 0; f < 6; f++) {
    Mge_BeginEnvProbeFace(probe, mirrorPos, f);
        Mge_ClearBackground(BLACK);
        Camera3D fc = Mge_GetEnvProbeCamera(mirrorPos, f);
        /* draw the scene (skybox + everything except the mirror) with `fc` */
    Mge_EndEnvProbeFace();
}
Mge_BeginMode3D(camera);
    Mge_BeginEnvironmentMap(probe.cubemap, camera, ENVMAP_REFLECT, 0.0f);
        Draw_Cube(mirrorPos, size, WHITE);   // reflects the real-time surroundings
    Mge_EndEnvironmentMap();
Mge_EndMode3D();
```

Demos: `examples/cubemap/skybox_reflect.c` (static sky, reflect + refract) and
`dynamic_envmap.c` (a mirror cube reflecting orbiting cubes each frame). Both
use `assets/skybox/`.

