# MGEngine

A small raylib-style 2D/3D rendering engine. The engine builds as a **shared
library** (`libmgengine.dll` / `.so`); `builder/` is a separate app that links
against it through the headers in `source/`. Engine code is **C11** (no glm, no
Dear ImGui); OpenGL 4.4 core via GLFW + glad, image loading via stb_image, model
loading via [Assimp](https://github.com/assimp/assimp) (C++ — the library is
linked with `g++`, and bakes in the C/C++ runtimes so consumers stay pure C).

## Layout

```
source/                THE ENGINE -- every *.c here is compiled into the library
  mge.h            public types + core / shapes / texture / input API
  mge_gl.h  mge_gl.c     immediate-mode-ish batched GL renderer (MgeGL_*)
  mge_math.h mge_math.c  Vector2/3/4, Matrix, projections (replaces glm)
  mge_core.c            window, timing, input, shaders, camera
  mge_shapes.c          Draw_Line / Draw_Rectangle / Draw_Triangle / Draw_Arrow / Draw_Cube ...
  mge_object.c          Object struct + mouse-driven translation gizmo
  mge_light.c          Phong lighting; directional / point / spot lights
  mge_material.c        Material / MaterialMap construction helpers
  mge_mesh.c           Mesh: vertices + indices + textures, own GPU buffers
  mge_model.c          Mge_LoadModel -- Assimp file -> list of meshes
  mge_depth.c          depth test / clip planes / polygon offset / depth preview
  mge_stencil.c        stencil test + Mge_DrawObjectOutline
  mge_texture.c         Mge_LoadImage / Mge_LoadTexture (stb_image)
  mge_utils.h mge_utils.c   Trace_Log, file loading
  platforms/mge_code_desktop.c   GLFW backend (#included by mge_core.c)
builder/
  main.c               THE APP -- editor demo: fly-camera + TAB edit mode + lighting;
                       #include <mge.h>, links -lmgengine
vendor/
  glad/                glad GL loader -- include/ + glad.c (compiled into the engine)
  stb/                 stb_image.h
  mlib/                MahdiyDev/mlib (containers, test harness)
  glfw/                GLFW -- vendored source; `make vendor-glfw` builds lib/ + include/
  assimp/              Assimp OBJ/glTF2/FBX importers -- pruned source under
                       source/; `make vendor-assimp` builds lib/ + include/
test/                  unit tests for math / file utils / objects / materials / lights / mesh (no window/GL needed)
examples/shapes/       draw_line, draw_rectangle, draw_triangle, mixed
examples/objects/      gizmo_2d, gizmo_3d
examples/lighting/     ambient, diffuse, specular, directional, point, spotlight
examples/materials/    textured_cube
examples/meshes/       textured_quad
examples/models/       load_melon
examples/depth/        depth_buffer
examples/stencil/      object_outline
```

## Using mlib

[mlib](https://github.com/MahdiyDev/mlib) is vendored in `vendor/mlib/`
(add `-Ivendor/mlib -Ivendor/mlib/vec`). The engine uses two of its containers:

- `mge_utils.c` loads files into an mlib `string_builder` (`sb_read_file`).
- `mge_gl.c` keeps the render batch's **draw-call list** and **matrix stack** as
  `DEFINE_VEC(...)` vectors, so neither has a fixed cap any more.

## Building

The engine links against GLFW and Assimp, both vendored as source. Build them
once (needs `cmake` + `ninja`):

```sh
make vendor        # builds GLFW + Assimp -> vendor/*/lib + vendor/*/include
make               # -> build/libmgengine.(dll|so)  and  build/mgengine  (the app)
make lib           # -> just the library
```

`make vendor-glfw` / `make vendor-assimp` build just one; `make vendor-clean`
deletes everything they produced (the committed source trees stay). The Assimp build
enables only the OBJ / glTF2 / FBX importers (no exporters, tools or tests) for a
small static lib; adjust the `-DASSIMP_BUILD_*` flags in the `vendor-assimp`
recipe to add formats.

`make` compiles every `source/*.c` with `gcc -std=c11` (the desktop platform
file is `#include`d by `mge_core.c`, not compiled on its own), links them into
`build/libmgengine.dll` with `g++` (`-static-libgcc -static-libstdc++ -static`,
so the DLL carries the Assimp/C++ runtime and GLFW/OpenGL are already inside),
then builds `builder/main.c` against it with plain `gcc -Isource -lmgengine`.
`make_build_dir` stages `assets/` (and `shaders/`) plus, on Windows, the DLL
sits next to `build/mgengine.exe`, so the app runs from `build/`.

Your own app is the same one-liner: `gcc yours.c -Isource -Lbuild -lmgengine`
plus `libmgengine.dll` on the path (or beside the exe). The `examples/` still
link the engine object files directly (`build/obj/*.o`) so each example exe is
self-contained.

On Windows use `mingw32-make`. The Makefiles pin `SHELL := cmd.exe`, so the
recipes work whether or not an `sh`/Git-Bash shell is on `PATH`.

### Tests (no GLFW / GL context required)

```sh
make test            # or:  cd test && make
```

`test_math` covers the vector/matrix layer; `test_utils` covers
`Mge_GetFileExtension` and the file loaders; `test_object` covers the gizmo
picking/drag math; `test_material` covers `Material` / `MaterialMap`
construction; `test_light` covers the light constructors and the uniform
wiring in `Mge_BeginLighting3D(Ex)`; `test_mesh` covers the `Mesh` struct
handling; `test_depth` covers the clip planes, depth-state forwarding and
depth-preview wiring; `test_stencil` covers the stencil forwarding and the
outline state sequence. All use a stubbed GL backend -- none open a window.

`test_model` is separate (`cd test && make model`) because it links the
vendored Assimp: it runs `Mge_LoadModel` for real against a generated OBJ and,
if present, `assets/sliced_musk_melon/scene.gltf`. Run `make vendor` first.

On Windows the test Makefile links the C runtime statically (`LDFLAGS = -static`)
so that app-control policies (Device Guard / WDAC) don't block the freshly built
test binaries; override `LDFLAGS=` to get dynamic linking back.

### Examples

```sh
make                     # build the engine objects first
cd examples && make      # -> examples/shapes/*
```

## API sketch

```c
#include "mge.h"
#include "mge_gl.h"

int main(void)
{
    Mge_InitWindow(800, 600, "hello");
    Mge_SetTargetFPS(60);

    while (!Mge_WindowShouldClose()) {
        Mge_BeginDrawing();
        Mge_ClearBackground(DARKGREEN);

        Draw_RectangleRec((Rectangle){ 100, 100, 120, 80 }, RED);
        Draw_TriangleLines((Vector2){ 300, 80 }, (Vector2){ 260, 200 },
                           (Vector2){ 340, 200 }, GREEN);

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
```

3D uses a `Camera3D` (passed **by value**) between `Mge_BeginMode3D` /
`Mge_EndMode3D`; draw with `Draw_Cube` / `Draw_CubeWires` / `Draw_Arrow3D` or the
low-level `MgeGL_Begin(MGEGL_TRIANGLES)` … `MgeGL_Vertex3f` … `MgeGL_End` immediate
calls. `builder/main.c` shows a fly-camera plus TAB-toggled edit mode with the
move gizmo.

### Cursor

| Function | Effect |
| --- | --- |
| `ShowCursor()` / `HideCursor()` | toggle cursor **visibility** (the cursor still moves freely) |
| `EnableCursor()` | show **and** unlock the cursor |
| `DisableCursor()` | hide **and** lock the cursor to the window centre (FPS style, enables raw mouse motion) |
| `Mge_ToggleCursor()` | flip between `EnableCursor()` and `DisableCursor()` |
| `IsCursorHidden()` | `true` while the cursor is hidden / locked |

Bind it to a key in your loop — `builder/main.c` uses **TAB** to free and re-lock
the mouse, and only runs the fly-camera while it is locked:

```c
Mge_InitWindow(1280, 720, "demo");
DisableCursor();                        // start with a captured cursor

while (!Mge_WindowShouldClose()) {
    Mge_BeginDrawing();

    if (IsKeyPressed(KEY_TAB))
        Mge_ToggleCursor();

    if (IsCursorHidden())
        update_camera(&camera);         // mouse-look only while captured

    /* ... draw ... */
    Mge_EndDrawing();
}
```

### Objects & the move gizmo

An `Object` is a movable rectangle (`OBJECT_2D`) or axis-aligned box (`OBJECT_3D`)
with a `position` (centre), `size`, `color`, `id` and `selected` flag.

```c
Object Mge_MakeObject2D(float x, float y, float w, float h, Color color);
Object Mge_MakeObject3D(Vector3 position, Vector3 size, Color color);
void   Mge_DrawObject(Object obj);                        // filled shape (+ stencil outline when selected)
void   Mge_DrawObjectGizmo(Object obj, float axisLength); // X/Y (2D) or X/Y/Z (3D) arrows at the position
```

`Mge_ManipulateObjects2D/3D()` does mouse picking and dragging — call it **once
per frame while the cursor is enabled** (i.e. not in FPS mode). Left-click an
object to select it, then:

- drag a **gizmo arrow** → the object moves **along that axis only** (the drag is
  projected onto the arrow's screen direction);
- drag the object **body** (2D) → free move.

```c
Object objs[3] = { Mge_MakeObject2D(200, 200, 80, 60, RED), ... };
const float AXIS = 70.0f;

while (!Mge_WindowShouldClose()) {
    int sel = Mge_ManipulateObjects2D(objs, 3, AXIS);   // returns selected index or -1
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
        Mge_ClearSelection(objs, 3);

    Mge_BeginDrawing();
    Mge_ClearBackground(DARKGRAY);
    for (int i = 0; i < 3; i++) Mge_DrawObject(objs[i]);
    if (sel >= 0) Mge_DrawObjectGizmo(objs[sel], AXIS);
    Mge_EndDrawing();
}
```

3D is the same shape, called inside `Mge_BeginMode3D` for the drawing, with the
camera passed to the manipulator:

```c
int sel = Mge_ManipulateObjects3D(objs, n, camera, 1.6f);
Mge_BeginMode3D(camera);
    for (...) Mge_DrawObject(objs[i]);
    if (sel >= 0) Mge_DrawObjectGizmo(objs[sel], 1.6f);
Mge_EndMode3D();
```

Supporting pieces this adds: mouse buttons (`IsMouseButtonPressed/Down/Released`,
`GetMouseDelta`), window size (`Mge_GetScreenWidth/Height`), 3D shapes
(`Draw_Arrow`, `Draw_Arrow3D`, `Draw_Cube`, `Draw_CubeWires`), and world→screen
projection (`Mge_GetWorldToScreen[Ex]`, `Mge_GetCameraViewMatrix`,
`Mge_GetCameraProjectionMatrix`). Runnable demos: `examples/objects/gizmo_2d.c`
and `gizmo_3d.c`.

### Lighting

Phong shading with the three classic terms:

| term | what it is |
| --- | --- |
| **ambient** | a flat, constant fill added everywhere (nothing is fully black) |
| **diffuse** | Lambert: brightness ∝ `max(dot(surfaceNormal, dirToLight), 0)` |
| **specular** | a highlight where the surface reflects the light toward the camera; `material.shininess` sets its tightness |

**Where do the knobs live?** — the two halves of the equation split cleanly:

- a **`Light`** is a *scene entity*: its type, `color`, and the strength of each
  term. You can mix up to `MGE_MAX_LIGHTS` (8) in one pass.
- a **`Material`** is the *surface response* and is a field on `Object`
  (`obj.material`): a set of `MaterialMap` slots plus a `shininess`. The
  *surface* parameters live here; the *light* itself is separate.

#### Light types — one constructor each

```c
Light Mge_MakeDirectionalLight(Vector3 direction, Vector3 color); // the sun: parallel rays, no falloff
Light Mge_MakePointLight(Vector3 position, Vector3 color);        // a bulb: radiates + fades with distance
Light Mge_MakeSpotLight(Vector3 position, Vector3 direction, Vector3 color,
                        float innerAngleDeg, float outerAngleDeg); // a cone; inner < outer = soft edge
Light Mge_MakeFlashlight(Camera3D camera, Vector3 color);         // a tight spot at the camera, aimed where it looks
Light Mge_MakeLight(Vector3 position, Vector3 color);             // legacy: point light, no distance falloff
```

- **Directional** ignores position; set `.direction`. Attenuation never applies.
- **Point / spot** fade with distance via `.constant / .linear / .quadratic`
  (`Mge_MakePointLight` presets a ~50-unit reach).
- **Spot** adds a cone: full brightness inside `innerAngleDeg`, fading to nothing
  by `outerAngleDeg`. Equal angles → a hard edge; a gap → a **soft edge**.
  Stored as cosines in `.innerCutoff / .outerCutoff`.
- Every light has `.enabled` (skip it without removing it from the array) and
  per-term `.ambient / .diffuse / .specular` scalars.

#### Drawing with them

```c
Material Mge_DefaultMaterial(void);
void Mge_SetMaterialTexture(Material* m, int mapIndex, Texture2D texture);

void Mge_BeginLighting3D(Light light, Camera3D camera);                       // one light
void Mge_BeginLighting3DEx(const Light* lights, int count, Camera3D camera);  // up to MGE_MAX_LIGHTS
void Mge_SetMaterial(Material material);   // per-surface; no-op unless lighting is active
void Mge_EndLighting3D(void);              // restore the default (unlit) shader
```

Call inside `Mge_BeginMode3D`. `Mge_DrawObject` sets the object's own material
for you, so lit objects just work:

```c
Light sun   = Mge_MakeDirectionalLight((Vector3){ -1, -2, -1 }, (Vector3){ .6f, .6f, .7f });
Light lamp  = Mge_MakePointLight((Vector3){ 4, 6, 4 }, (Vector3){ 1, .9f, .7f });
Light torch = Mge_MakeFlashlight(camera, (Vector3){ 1, 1, 1 });
Light lights[3] = { sun, lamp, torch };

Object box = Mge_MakeObject3D((Vector3){ 0, 0, 0 }, (Vector3){ 1, 1, 1 }, RED);
box.material.shininess = 64.0f;

Mge_BeginMode3D(camera);
    Mge_BeginLighting3DEx(lights, 3, camera);
        Mge_DrawObject(box);                              // lit with box.material
        Mge_SetMaterial((Material){ .maps[MATERIAL_MAP_DIFFUSE].color = GRAY,
                                   .maps[MATERIAL_MAP_SPECULAR].value = 1.0f, .shininess = 8 });
        Draw_Cube((Vector3){ 0, -1, 0 }, (Vector3){ 24, 0.1f, 24 }, GRAY); // lit floor
    Mge_EndLighting3D();
    if (sel >= 0) Mge_DrawObjectGizmo(box, 1.6f);         // overlay lines stay unlit
Mge_EndMode3D();
```

For a single light, `Mge_BeginLighting3D(sun, camera)` is the same as
`Mge_BeginLighting3DEx(&sun, 1, camera)`.

Only geometry with per-vertex normals is shaded correctly — `Draw_Cube` emits
them; `MgeGL_Normal3f(x, y, z)` sets the current normal for your own
`MgeGL_Vertex3f` calls. Lines (`Draw_Arrow3D`, `Draw_CubeWires`) have no normals,
so draw them outside the `Begin/EndLighting3D` pair.

Demos: `examples/lighting/` — `ambient` / `diffuse` / `specular` isolate the
three terms; `directional` / `point` / `spotlight` isolate the three light types
(spotlight shows a hard vs. a soft cone side by side); `builder/main.c` combines
a directional fill with an orbiting point light.

### Materials & material maps

A `Material` is a fixed set of `MaterialMap` slots (indexed by
`MaterialMapIndex`) plus a Phong `shininess`. Each map carries a **texture**, a
**color** and a scalar **value**; what those mean depends on the slot:

| slot | `.texture` | `.color` | `.value` |
| --- | --- | --- | --- |
| `MATERIAL_MAP_DIFFUSE` | albedo image sampled across the surface (id `0` → a white 1×1, i.e. "untextured") | tint multiplied over the texture | unused |
| `MATERIAL_MAP_SPECULAR` | unused | unused (reserved) | highlight strength multiplier: `1` = as the light sets it, `0` = matte |

```c
typedef struct MaterialMap {
    Texture2D texture;   // id 0 -> engine's white 1x1
    Color     color;
    float     value;
} MaterialMap;

typedef struct Material {
    MaterialMap maps[MATERIAL_MAP_COUNT];  // [MATERIAL_MAP_DIFFUSE], [MATERIAL_MAP_SPECULAR]
    float       shininess;
} Material;
```

Start from `Mge_DefaultMaterial()` and edit the slots you care about:

```c
Texture2D wall = Mge_LoadTexture("assets/wall.jpg");

Material m = Mge_DefaultMaterial();
Mge_SetMaterialTexture(&m, MATERIAL_MAP_DIFFUSE, wall);       // or: m.maps[...].texture = wall;
m.maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 255, 210, 180, 255 }; // warm tint over the texture
m.maps[MATERIAL_MAP_SPECULAR].value = 0.0f;                   // matte
m.shininess = 48.0f;

Mge_BeginLighting3D(light, camera);
    Mge_SetMaterial(m);
    Draw_Cube(pos, size, m.maps[MATERIAL_MAP_DIFFUSE].color); // pass the diffuse tint as the vertex colour
Mge_EndLighting3D();
```

`Mge_SetMaterial` binds the diffuse texture and sets the specular / shininess
uniforms; the diffuse **color** reaches the shader through the drawn geometry's
per-vertex colour, so pass it to `Draw_Cube` (or `MgeGL_Color4ub`).
`Mge_DrawObject` does this for an `Object` automatically —
`obj.material.maps[MATERIAL_MAP_DIFFUSE].color` is seeded from the colour you
gave `Mge_MakeObject3D`. `Draw_Cube` emits per-face UVs so a texture maps one
full copy onto every face. A texture that fails to load has id `0` and falls
back to the flat colour.

Demo: `examples/materials/textured_cube.c` — textured/tinted/matte/plain cubes
side by side under a moving light.

### Mesh

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
first `MESH_TEXTURE_DIFFUSE` texture (or a white 1×1 if there is none) and draws
with whatever shader is active — the unlit default or the lighting shader. It has
no colour attribute, so the diffuse texture is shown untinted.

Demo: `examples/meshes/textured_quad.c` — a textured wall + an untextured floor,
both hand-built meshes, under a moving point light.

### Model

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

### Depth testing

`Mge_BeginMode3D` turns the depth test on (`DEPTH_LESS`) and `Mge_EndMode3D`
turns it off, so 2D drawing is always painter's-order and 3D is depth-sorted.
To tune it *within* a 3D block:

```c
void Mge_EnableDepthTest(void);  void Mge_DisableDepthTest(void);
void Mge_SetDepthFunc(int func);   // a DepthFunc: DEPTH_LESS (default) ... DEPTH_ALWAYS
void Mge_SetDepthMask(bool write); // false -> test against depth but leave it unchanged
```

**Visualizing the depth buffer.** Draw between `Mge_BeginDepthPreview()` /
`Mge_EndDepthPreview()` (in place of `Mge_BeginLighting3D`) to shade every
fragment by its linearized depth — near is black, far is white:

```c
Mge_BeginMode3D(camera);
    Mge_BeginDepthPreview();
        Draw_Cube(...); Mge_DrawModel(...);
    Mge_EndDepthPreview();
Mge_EndMode3D();
```

**Preventing z-fighting.** Two surfaces that land on almost the same depth value
flicker between each other. In order of effectiveness:

1. Don't make faces coplanar — offset the decal/marking slightly.
2. Push the **near** plane out. A near plane of `0.01` spends nearly all of the
   depth buffer's precision on the first few centimetres; `Mge_SetClipPlanes`
   lets you widen it to whatever the scene allows (this also affects
   `Mge_GetCameraProjectionMatrix`):

   ```c
   Mge_SetClipPlanes(0.2, 60.0);   // near, far -- rejected if near <= 0 or far <= near
   double n = Mge_GetClipNear();
   ```
3. When geometry *must* be coplanar (decals, outlines), bias its depth with a
   polygon offset — set it, draw, reset:

   ```c
   Mge_SetPolygonOffset(1.0f, 1.0f);   // positive = push away from the camera
   Draw_Cube(...);                     // this surface now loses ties
   Mge_DisablePolygonOffset();
   ```

(The framebuffer uses GLFW's default 24-bit depth buffer.)

Demo: `examples/depth/depth_buffer.c` — auto-flips between lit and depth-preview
views; shows a z-fighting cube pair beside a polygon-offset-fixed one.

### Stencil testing & object outlining

The framebuffer has an 8-bit stencil buffer (cleared with colour/depth by
`Mge_ClearBackground`). Raw controls mirror the depth ones:

```c
void Mge_EnableStencilTest(void);  void Mge_DisableStencilTest(void);
void Mge_SetStencilFunc(int func, int ref, unsigned mask);  // a StencilFunc
void Mge_SetStencilOp(int onStencilFail, int onDepthFail, int onPass); // StencilOp x3
void Mge_SetStencilMask(unsigned mask);   // stencil bits writes may change
void Mge_ClearStencil(void);
```

**Object outlining** is the built-in use. `Mge_DrawObject` already draws a
stencil outline (instead of a wireframe) around any `Object` whose `.selected`
flag is set. For anything else, three calls wrap the technique:

```c
Mge_BeginStencilMask();                        // stamp the silhouette:
    Draw_Cube(pos, size, col);                  //   colour + depth writes are off
Mge_BeginStencilOutside();                      // now draw only outside the stamp:
    Draw_Cube(pos, biggerSize, WHITE);          //   just the border survives
Mge_EndStencil();                              // restore normal drawing
```

`Mge_DrawObjectOutline(obj, thickness, color)` does exactly that for one
`Object` you have already drawn this frame (`thickness` is added to its
extents). The mask pass turns the depth test off, so a selected object's
outline shows even when it is partly behind something.

Demo: `examples/stencil/object_outline.c` — a walking selection outlines each
cube in turn, plus one hand-outlined pillar in a custom colour.

### Math

`glm` is gone. `mge_math.h` provides plain-C functions — no operator overloads:

| | |
| --- | --- |
| `Vector3_Add/Subtract/Scale/Multiply(a, b)` | `Vector3_DotProduct`, `Vector3_Length` |
| `Vector3Cross`, `Vector3Normalize` | `Vector2_Rotate(v, radians)`, `Clamp` |
| `Matrix_Identity/Multiply/Translate/Rotate` | `MatrixOrtho/Perspective/LookAt`, `MatrixToFloatV` |

Matrices are stored column-major so `MatrixToFloat(m)` feeds `glUniformMatrix4fv`
directly.

## Notes / limitations

- Pure C11: every translation unit builds under `gcc -std=c11 -Wall -Wextra`.
  The `test/` suite needs no window or GL context; the window / renderer itself
  needs a real GL context.
- `make` needs `vendor/{glfw,assimp}/lib` populated first (`make vendor`).
- Dear ImGui and `glm` are not used — `vendor/imgui/` is left in place but
  unlinked, `vendor/glm/` is gone.
