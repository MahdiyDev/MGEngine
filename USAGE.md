# MGEngine

A small raylib-style 2D/3D rendering engine. The engine builds as a **shared
library** (`libmgengine.dll` / `.so`); `builder/` is a separate app that links
against it through the headers in `source/` (see
[builder/USAGE.md](builder/USAGE.md), and [README.md](README.md) for the map of
the whole repo). Engine code is **C11**; OpenGL 4.4 core via GLFW + glad, image
loading via stb_image, model loading via
[Assimp](https://github.com/assimp/assimp), UI via
[Dear ImGui](https://github.com/ocornut/imgui) behind a C abstraction. The two
C++ dependencies mean the library is linked with `g++` — it bakes in the C/C++
runtimes, so consumers stay pure C.

## Layout

```
source/                THE ENGINE -- every *.c here is compiled into the library
  mge.h            public types + core / shapes / texture / input API
  mge_gl.h  mge_gl.c     immediate-mode-ish batched GL renderer (MgeGL_*)
  mge_math.h mge_math.c  Vector2/3/4, Matrix, projections (replaces glm)
  mge_core.c            window, timing, input, shaders, camera
  mge_shapes.c          Draw_Line / Draw_Rectangle / Draw_Triangle / Draw_Arrow / Draw_Cube / Draw_Sphere / Draw_Plane ...
  mge_object.c          Object struct (primitive/position/rotation/size/material) + 3D picking
  mge_gizmo.c           switchable translate / rotate / scale manipulation gizmo
  mge_dialog.c          native "open file" dialog (Mge_OpenFileDialog / Mge_OpenImageDialog)
  mge_light.c          Blinn-Phong lighting; directional / point / spot; normal maps
  mge_shadow.c         shadow mapping: directional depth map + point-light depth cube
  mge_material.c        Material / MaterialMap construction helpers
  mge_mesh.c           Mesh: vertices + indices + textures, own GPU buffers
  mge_model.c          Mge_LoadModel -- Assimp file -> list of meshes
  mge_depth.c          depth test / clip planes / polygon offset / depth preview
  mge_stencil.c        stencil test + Mge_DrawObjectOutline
  mge_cull.c           face culling on/off + cull face / winding
  mge_framebuffer.c    RenderTexture + full-screen post-processing + HDR tone mapping
  mge_bloom.c          bloom -- bright-pass + separable Gaussian + tone-mapped composite
  mge_deferred.c       deferred shading -- G-buffer geometry pass + full-screen lighting pass
  mge_ssao.c           screen-space ambient occlusion (hemisphere kernel + noise + blur)
  mge_cubemap.c        cube maps: skybox, environment mapping, dynamic probes
  mge_geometry.c       geometry-shader effects: explode, normal visualization
  mge_instancing.c     ModelBatch: many copies of a Model in one instanced draw
  mge_msaa.c           MSAA request (Mge_SetMSAA / Mge_GetMSAA)
  mge_gamma.c          gamma correction toggle (Mge_SetGammaCorrection)
  mge_debug.c          GL debug-output callback (Mge_SetDebugOutput)
  mge_gui.h  mge_gui.cpp   Mge_Gui* immediate-mode UI (Dear ImGui backend; the one C++ unit)
  mge_texture.c         Mge_LoadImage / Mge_LoadTexture / ...Ex (sRGB) / Mge_UnloadTexture / Mge_SetTextureWrap (stb_image)
  mge_utils.h mge_utils.c   Trace_Log, file loading
  platforms/mge_code_desktop.c   GLFW backend (#included by mge_core.c)
builder/               THE APP -- scene editor (fly-camera + TAB edit mode + gizmo + panels)
  main.c               window, loop, camera, the shared Mge_Gui frame
  scene.c/.h           entities, selection, picking, spawn (Scene_AddShape), the render passes
  sidebar.c/.h         left panel: mode, gizmo switch, entity list, inspector (+ texture slots)
  explorer.c/.h        right panel: the shape palette (spawn cube / sphere / plane)
  USAGE.md             builder docs
vendor/
  glad/                glad GL loader -- include/ + glad.c (compiled into the engine)
  stb/                 stb_image.h
  mlib/                MahdiyDev/mlib (containers, test harness)
  imgui/               Dear ImGui 1.90.5 source (compiled straight into the engine)
  glfw/                GLFW -- vendored source; `make vendor-glfw` builds lib/ + include/
  assimp/              Assimp OBJ/glTF2/FBX importers -- pruned source under
                       source/; `make vendor-assimp` builds lib/ + include/
test/                  unit tests (no window/GL); test/glstub/ = a fake glad so mge_gl.c itself is testable
examples/shapes/       draw_line, draw_rectangle, draw_triangle, mixed
examples/objects/      gizmo_2d, gizmo_3d
examples/lighting/     ambient, diffuse, specular, directional, point, spotlight, blinn_phong, gamma_correction, hdr, bloom, deferred_shading, ssao, shadow_mapping, point_shadows, normal_mapping, parallax_mapping
examples/materials/    textured_cube, tiling_triplanar
examples/meshes/       textured_quad, batched_attributes
examples/models/       load_melon
examples/depth/        depth_buffer
examples/stencil/      object_outline
examples/culling/      backface_cull
examples/framebuffer/  post_process
examples/cubemap/      skybox_reflect, dynamic_envmap
examples/geometry/     geometry_shader
examples/instancing/   melon_field
examples/antialiasing/ msaa
examples/batching/     draw_calls
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
make release       # same, but a PRODUCTION build (see below)
make lib           # -> just the library
```

`make` is a **debug** build: `-O0 -g`, assertions on — handy while developing,
but noticeably slow. `make release` rebuilds every object with `-O2 -DNDEBUG`,
strips symbols and lets the linker drop unused code — this is the one to run for
real (typically several times the frame rate of the debug build, ~40 % smaller
binaries). The object cache doesn't track flags, so `make release` wipes
`build/obj` first; a plain `make` afterwards puts the debug objects back.
Override per-invocation instead with e.g. `make CFLAGS="-O3 -march=native"`.

The compiler writes `.d` dep files (`-MMD -MP`), so editing a header —
`source/mge.h` especially, since it fixes struct sizes — rebuilds every
dependent object. `make -C test render` / `make -C examples` still link the
objects the **root** `make` produced, so run `make` at the root first.

`make vendor-glfw` / `make vendor-assimp` build just one; `make vendor-clean`
deletes everything they produced (the committed source trees stay). The Assimp build
enables only the OBJ / glTF2 / FBX importers (no exporters, tools or tests) for a
small static lib; adjust the `-DASSIMP_BUILD_*` flags in the `vendor-assimp`
recipe to add formats.

`make` compiles `source/*.c` with `gcc -std=c11` and `source/mge_gui.cpp` with
`g++ -std=c++17` (the desktop platform file is `#include`d by `mge_core.c`, not
compiled on its own), links them into `build/libmgengine.dll` with `g++`
(`-static-libgcc -static-libstdc++ -static`, so the DLL carries the C/C++ runtime
and GLFW / Assimp / Dear ImGui are already inside), then builds `builder/main.c`
against it with plain `gcc -Isource -lmgengine`.
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
`Mge_GetFileExtension` and the file loaders; `test_object` covers 2D drag +
3D picking, `test_gizmo` the mode switch + translate/scale drag + rotation
maths; `test_material` covers `Material` / `MaterialMap`
construction; `test_light` covers the light constructors, the uniform wiring in
`Mge_BeginLighting3D(Ex)` and the Blinn/Phong toggle; `test_mesh` covers the `Mesh` struct
handling; `test_depth` covers the clip planes, depth-state forwarding and
depth-preview wiring; `test_stencil` covers the stencil forwarding and the
outline state sequence; `test_cull` covers face-culling forwarding;
`test_framebuffer` / `test_cubemap` cover their enums; `test_geometry` covers
the explode / normals wrappers; `test_instancing` covers the `ModelBatch`
contract and the `Matrix_Scale` / composition math behind the transforms;
`test_msaa` covers the `Mge_SetMSAA` request clamping; `test_gamma` covers the
`Mge_SetGammaCorrection` state + forwarding; `test_shadow` covers the `ShadowMap` /
`PointShadowMap` struct contract; `test_debug` covers the `Mge_SetDebugOutput`
toggle and callback registration.

`test_gl` is the odd one out: it compiles `source/mge_gl.c` itself against a fake
`<glad/glad.h>` (`test/glstub/`) that records every GL call, and checks the
renderer backend's own logic — the matrix stack, draw-call merging and alignment,
vertex accumulation, the triple-buffer ring, the draw-call counter, and every
engine-enum → GL-enum mapping in the state setters.

All suites use a stubbed GL backend — none open a window.

`test_model` is separate (`cd test && make model`) because it links the
vendored Assimp: it runs `Mge_LoadModel` for real against a generated OBJ and,
if present, `assets/sliced_musk_melon/scene.gltf`. Run `make vendor` first.

`make render` is the one test that touches a real GPU: it opens a **hidden**
GLFW window, renders ~20 engine features (2D shapes, a lit cube, a shadow map,
a post-fx pass, the skybox, a normal-mapped wall, a parallax-mapped wall, a
mirror-repeat wrapped quad, a tiled plane + a triplanar box, an HDR scene
tone-mapped vs clamped, a bloom glow, a deferred-shaded scene, an SSAO scene, the
cube/sphere/plane primitives, a rotated cube with each gizmo mode, the rotate
gizmo head-on, a scripted rotate drag) one frame
each, reads the
framebuffer back, and fails on a GL error or a blank frame. Every frame is also
written to `test/render_out/*.tga` so you can eyeball what actually rendered —
this is how you catch *valid-but-wrong* output that the stub tests can't see.
Needs the root `make` + `make vendor`, a GPU and a desktop session (not part of
`make test`).

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
`Mge_EndMode3D`; draw with `Draw_Cube` / `Draw_CubeWires` / `Draw_Sphere` /
`Draw_Plane` / `Draw_Arrow3D` (and the `*Ex` variants) or the
low-level `MgeGL_Begin(MGEGL_TRIANGLES)` … `MgeGL_Vertex3f` … `MgeGL_End` immediate
calls. `builder/` shows a fly-camera plus TAB-toggled edit mode with the
translate / rotate / scale gizmo.

### How the renderer batches

`mge_gl.c` is a retained-nothing, `rlgl`-style batcher. Every `Draw_*` shape and
every `MgeGL_Begin`/`MgeGL_Vertex*`/`MgeGL_End` block appends into **one** CPU
vertex buffer; consecutive primitives of the same kind (`LINES` / `TRIANGLES` /
quads) merge into a single draw-call entry. Nothing reaches the GPU until a
**flush** — `MgeGL_Draw()` — which uploads the whole buffer once and issues one
`glDraw*` per merged entry.

A flush happens on `Mge_EndDrawing`, a shader change (`Mge_BeginLighting3D`,
geometry/post-fx passes), a render-state change (`Mge_BeginMode3D`, depth /
stencil / cull toggles, matrix mode), a retained `Mge_DrawMesh` / `Mge_DrawModel`
/ `Mge_DrawModelBatch`, a texture change (`Mge_SetMaterial`), or the buffer
filling (~5 k vertices). So a frame of same-shader 2D UI + wireframe shapes is
usually **1 upload + 1–2 draw calls** regardless of shape count.

```c
int n = Mge_GetDrawCalls();   // GL draw calls in the previous frame; lower = better batching
```

The dynamic vertex buffers are **triple-buffered** (`MGEGL_BATCH_BUFFERS`, default
3): each flush uploads to and draws from the next set in the ring, so a
`glBufferSubData` never blocks on a buffer the GPU is still reading from an
earlier draw. Raise it if you flush many times per frame; drop it to 1 to save
VRAM.

What is *not* merged: each retained `Mesh` has its own VAO and its own
`glDrawElements`; different models are separate calls (use a `ModelBatch` for
many copies of one). `Draw_*` shapes across a shader/texture/state change land in
different batches. The builder shows the live count next to the FPS.

Demo: `examples/batching/draw_calls.c` — an 800-shape grid that stays at ~2 draw
calls a frame.

### Anti-aliasing (MSAA)

The window is created with a **4x multisampled** default framebuffer, so every
edge the renderer rasterizes — shapes, objects, meshes, models — comes out
smoothed with nothing extra per draw. Change it *before* `Mge_InitWindow`:

```c
Mge_SetMSAA(8);           // 2 / 4 / 8 ... ; 0 (or 1) turns MSAA off
Mge_InitWindow(800, 600, "hello");

int got = Mge_GetMSAA();  // sample count the driver actually granted (0 = none)
```

`Mge_SetMSAA` only records the request; it must be called first because the
sample count is fixed at window creation (`glfwWindowHint(GLFW_SAMPLES, …)`).
`builder/main.c` calls `Mge_SetMSAA(4)` explicitly. This covers the window's
framebuffer only — a `RenderTexture` from `Mge_LoadRenderTexture` is still
single-sampled, so post-processed passes don't get MSAA.

Demo: `examples/antialiasing/msaa.c` — orbiting cube + a thin rotating triangle
outline; set `Mge_SetMSAA(0)` to bring the jaggies back.

### Gamma correction

A display darkens whatever it's given by roughly a 2.2 power. Lighting maths are
linear, so their result reaches the eye too dark unless it's sRGB-encoded first.

```c
Mge_SetGammaCorrection(true);   // GL_FRAMEBUFFER_SRGB on the window; call after Mge_InitWindow
bool on = Mge_GetGammaCorrection();
```

This encodes the window's **final** pixels only. Intermediate `RenderTexture`s
stay linear, so `Mge_DrawRenderTextureFX` kernels still run on linear data and
the encode happens once, when the result is blitted out. The ImGui pass is drawn
with the encode off, so the UI is unaffected.

**Off by default** — the engine's shape, vertex and light colours are authored in
sRGB-ish space, not linear, so turning it on shifts their look. Use it when you
work in linear space: load colour maps with `Mge_LoadTextureEx(path, true)` (the
GPU then linearizes them on sample; `Mge_LoadModel` already does this for diffuse
maps and leaves specular linear) and treat `Light.color` as linear.

Demo: `examples/lighting/gamma_correction.c` — a lit scene + a black→white ramp,
toggling correction every 3 s (or SPACE).

### HDR & tone mapping

A normal `RenderTexture` is `RGBA8` — a value above `1.0` is clamped, so a bright
light flattens to featureless white. `Mge_LoadRenderTextureHDR` gives an
`RGBA16F` colour attachment instead: the lit scene is stored at its true
intensity, then a full-screen **tone-map** pass squeezes that range back to
`[0,1]` for the display, keeping the highlight roll-off.

```c
RenderTexture hdr = Mge_LoadRenderTextureHDR(w, h);
...
Mge_BeginTextureMode(hdr);
    Mge_ClearBackground(...); Mge_BeginMode3D(cam); ...lit scene...; Mge_EndMode3D();
Mge_EndTextureMode();
Mge_DrawRenderTextureHDR(hdr, TONEMAP_ACES, exposure);   // -> the window
```

| `ToneMap` | curve |
| --- | --- |
| `TONEMAP_REINHARD` | `c / (c + 1)` — `exposure` ignored |
| `TONEMAP_EXPOSURE` | `1 - exp(-c · exposure)` — LearnOpenGL's exposure control |
| `TONEMAP_ACES` | Narkowicz filmic ACES approximation, input scaled by `exposure` |

`exposure` shifts which range lands in view, like a camera stop — raise it to pull
detail out of dim areas, lower it to keep bright areas from clipping. The pass
also applies gamma, **unless** `Mge_SetGammaCorrection(true)` is on (then
`GL_FRAMEBUFFER_SRGB` does it) — use one or the other, not both. Tone mapping
darkens an LDR skybox along with everything else; a true HDR pipeline would use an
HDR environment map.

Demo: `examples/lighting/hdr.c` — a corridor with one very bright light; SPACE
toggles tone map vs raw clamp, T cycles the operator, UP/DOWN adjust exposure.
The builder's sidebar has an **HDR** toggle + tone-map + exposure.

### Bloom

The bright parts of an HDR image bleed a soft glow. Given the HDR scene texture,
`Mge_DrawBloom` extracts pixels above a luminance `threshold`, Gaussian-blurs
them (separable, ping-pong, `iterations` H+V rounds at half resolution), then
composites `scene + blur · intensity` **and** tone-maps — it replaces the
`Mge_DrawRenderTextureHDR` step.

```c
BloomFX bloom = Mge_LoadBloom(w, h);      // w,h = the HDR scene's size
bloom.threshold = 1.0f;                   // >1 = only genuinely over-bright pixels
bloom.intensity = 0.6f;
bloom.iterations = 5;
...
Mge_BeginTextureMode(hdr); /* ...lit scene... */ Mge_EndTextureMode();
Mge_DrawBloom(hdr, &bloom, TONEMAP_ACES, exposure);   // -> the window
...
Mge_UnloadBloom(&bloom);
```

The bright pass has a soft knee, so `threshold` below `1.0` lets bright-but-not-
HDR surfaces glow a little too. Same gamma rule as HDR (the composite does it
unless `GL_FRAMEBUFFER_SRGB` is on).

Demo: `examples/lighting/bloom.c` — coloured lamps in a dark room; SPACE toggles
bloom, `[` `]` the threshold, `-` `=` the intensity. The builder's sidebar has a
**bloom** toggle (under HDR) with threshold + intensity sliders.

### Deferred shading

The forward path shades each fragment against every light — cost is
`fragments × lights`, and overdraw multiplies it. **Deferred shading** draws the
scene once into a **G-buffer** (world position, world normal, albedo + specular),
then a single full-screen pass shades every *pixel* against every light — so
`MGE_MAX_LIGHTS_DEFERRED` (32) small point lights stay cheap regardless of how
much geometry piles up.

```c
GBuffer g = Mge_LoadGBuffer(w, h);
...
Mge_BeginMode3D(cam);
    Mge_BeginGeometryPass(&g, cam);
        Mge_DrawObject(obj);  Draw_Cube(...);      // the same draw calls as forward
    Mge_EndGeometryPass();
Mge_EndMode3D();

Mge_DeferredLighting(g, lights, count, cam);        // shades into the bound framebuffer
Mge_BlitGBufferDepth(g);                            // then forward-draw lamp cubes / a skybox
...
Mge_UnloadGBuffer(&g);
```

Wrap `Mge_DeferredLighting` in `Mge_BeginTextureMode(hdrRT)` to feed it into the
HDR / bloom path. The deferred path has **no shadows and no normal / parallax /
triplanar maps** — it's the "lots of little lights" pipeline; keep the forward
path (and the builder) for the rest. `g.position` / `g.normal` / `g.albedoSpec`
are plain `Texture2D`s you can blit for debugging.

Demo: `examples/lighting/deferred_shading.c` — a 5×5 field under 24 drifting
coloured point lights; **G** cycles the final image and the raw G-buffer channels.

### SSAO

Screen-space ambient occlusion. From the deferred G-buffer, for each pixel it
samples a hemisphere of points around the surface (oriented by the normal,
jittered by a 4×4 noise texture), counts how many are buried behind nearby
geometry, blurs the result 4×4, and folds it into the **ambient** term — so
creases and contact points pick up soft shadowing no light computes.

```c
SSAO ao = Mge_LoadSSAO(w, h);
ao.radius = 0.5f;  ao.bias = 0.025f;  ao.power = 2.5f;  ao.kernelSize = 32; // <= 64
...
Mge_BeginMode3D(cam);
    Mge_BeginGeometryPass(&g, cam); ...draw...; Mge_EndGeometryPass();
Mge_EndMode3D();

Mge_ComputeSSAO(&ao, g, cam);                                  // fills ao.aoBlur
Mge_DeferredLightingAO(g, lights, count, cam, ao.aoBlur.texture.id);
...
Mge_UnloadSSAO(&ao);
```

`radius` is in world units — scale it to your scene. `Mge_DeferredLighting`
(no AO arg) still works unchanged. `ao.aoRaw` / `ao.aoBlur` are plain
`RenderTexture`s you can blit to inspect.

Demo: `examples/lighting/ssao.c` — the sliced-melon model; SPACE toggles SSAO,
**B** shows the raw AO buffer, `[` `]` the radius, `-` `=` the power.

### GL debug output

The driver can report invalid API use, undefined behaviour and performance
warnings through a callback the instant they happen — so a broken draw is a loud
`GL DEBUG [...]` log line, not a silently wrong frame.

```c
Mge_SetDebugOutput(false);   // before Mge_InitWindow; default: on unless built -DNDEBUG
```

On in normal builds, off in `make release`. It requests a debug GL context and
registers a synchronous `glDebugMessageCallback`; the `SEVERITY_NOTIFICATION`
chatter is muted. It catches *invalid* GL, not *valid-but-wrong* rendering — a
screenshot check is what catches that.

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

### Objects & the manipulation gizmo

An `Object` is a movable rectangle (`OBJECT_2D`) or a 3D primitive (`OBJECT_3D`:
`PRIM_CUBE` / `PRIM_SPHERE` / `PRIM_PLANE`, in `obj.primitive`) with a `position`
(centre), `size`, `rotation` (XYZ euler degrees, 3D only), a `material`, an `id`
and a `selected` flag. There is **no `Object.color`** — the base colour is the
diffuse map's tint (`obj.material.maps[MATERIAL_MAP_DIFFUSE].color`); the
`Mge_MakeObject*` constructors still take a `Color` and store it there.
`Mge_MakeObject3D` makes a cube; `Mge_MakeShape3D(primitive, pos, size, color)`
makes any of the three. `Mge_DrawObject` renders the primitive rotated (cube
corners + normals are rotated on the CPU — there is no per-object model matrix),
with a stencil outline when `selected`. `Mge_DrawPrimitive(obj, color)` draws just
the geometry (used by the shadow pass and the outline).

**Picking** (3D): `Mge_PickObject3D(objects, count, camera)` selects the object
whose screen-projected centre is nearest the cursor on left-click (returns the
index, or `-1`). `Mge_SetSelectedObject` / `Mge_ClearSelection` /
`Mge_GetSelectedObject` do it from code. `objects[i].selected` drives the outline.

**Gizmo** (3D): one switchable handle set for a single target, drawn on top of
the scene (depth test off) with the hovered handle over-stroked white.

```c
typedef enum { GIZMO_TRANSLATE, GIZMO_ROTATE, GIZMO_SCALE } GizmoMode;
typedef enum { GIZMO_WORLD, GIZMO_LOCAL } GizmoSpace;
void       Mge_SetGizmoMode(GizmoMode mode);
GizmoMode  Mge_GetGizmoMode(void);
void       Mge_SetGizmoSpace(GizmoSpace space); // WORLD = global axes; LOCAL = the object's own
GizmoSpace Mge_GetGizmoSpace(void);
// rotation / scale may be NULL. Returns true while a handle is being dragged.
bool Mge_Gizmo3D(Vector3* position, Vector3* rotation, Vector3* scale, Camera3D camera, float size);
```

- **translate** — three axis arrows + a **centre ball**; drag an arrow → move
  along that axis, drag the ball → move on the view plane.
- **rotate** — three thin rings (Unreal-style): each is a full circle but only
  the segments facing the camera are drawn, so an edge-on ring shows its near
  arc and a face-on ring shows the whole circle. Drag → spin the matching euler
  component (counter-clockwise on screen = positive).
- **scale** — axes with cube tips + a centre cube; drag an axis tip → scale that
  `size` component, drag the centre → uniform scale.

**Space** — `GIZMO_WORLD` keeps the axes on global X/Y/Z; `GIZMO_LOCAL` aligns
them to the object's `rotation` (translate moves along a local axis, rotate spins
about it — composed via matrix, so it doesn't drift). Scale is always local.
Lights (`rotation == NULL`) ignore the setting.

The gizmo is a **fixed on-screen size** (`size` param) regardless of the object.
Its translucent parts use `MgeGL_SetBlend` (straight alpha).

```c
Mge_BeginMode3D(camera);
    for (...) Mge_DrawObject(objs[i]);
    bool busy = false;
    if (sel >= 0)
        busy = Mge_Gizmo3D(&objs[sel].position, &objs[sel].rotation, &objs[sel].size, camera, 2.0f);
Mge_EndMode3D();
if (!busy) Mge_PickObject3D(objs, n, camera); // don't re-pick mid-drag
```

2D keeps the old translate-only helper: `Mge_ManipulateObjects2D` (pick + drag)
plus `Mge_DrawObjectGizmo2D` for the X/Y arrows.

Supporting pieces: mouse buttons (`IsMouseButtonPressed/Down/Released`,
`GetMouseDelta`), `Mge_GetScreenWidth/Height`, `Draw_CubeEx` / `Draw_CubeWiresEx`,
`Matrix_RotateXYZ` / `Matrix_ToEulerXYZ` / `Vector3_RotateAround`, and world→screen
projection (`Mge_GetWorldToScreen[Ex]`, `Mge_GetCameraViewMatrix`,
`Mge_GetCameraProjectionMatrix`). Demos: `examples/objects/gizmo_2d.c` and
`gizmo_3d.c` (1/2/3 switch modes); `builder/` uses it in full.

**Testing a drag without a mouse** — `Mge_SetMouseOverride(pos, leftDown)` feeds a
fake cursor to `GetMousePosition` / `GetMouseDelta` / `IsMouseButton*(LEFT)`.
Each call is one frame, so call it with `leftDown = true` to press, again (moved)
to drag, then `false` to release; `Mge_ClearMouseOverride()` restores the real
mouse. The `make render` harness uses this to script a rotate drag and screenshot
the result, and `test_gizmo` stubs the same functions for headless drag tests.

### Lighting

Blinn-Phong shading with the three classic terms:

| term | what it is |
| --- | --- |
| **ambient** | a flat, constant fill added everywhere (nothing is fully black) |
| **diffuse** | Lambert: brightness ∝ `max(dot(surfaceNormal, dirToLight), 0)` |
| **specular** | a highlight where the surface reflects the light toward the camera; `material.shininess` sets its tightness |

The specular term is **Blinn-Phong** (`dot(normal, halfway)`) by default — unlike
classic Phong (`dot(view, reflect)`) it has no hard cutoff at grazing angles, so
low-`shininess` highlights stay smooth. Switch models with
`Mge_SetLightingModel(LIGHTING_PHONG)` / `LIGHTING_BLINN_PHONG` (Phong wants a
`shininess` ~2–4x lower for a similar highlight). The toggle drives
`Mge_BeginLighting3D[Ex]`; the instanced-model shader (`Mge_DrawModelBatch`) is
always Blinn-Phong.

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
                                   .maps[MATERIAL_MAP_DIFFUSE].value = 1.0f,   // gain -- a raw literal must set it
                                   .maps[MATERIAL_MAP_SPECULAR].value = 1.0f, .shininess = 8 });
        Draw_Cube((Vector3){ 0, -1, 0 }, (Vector3){ 24, 0.1f, 24 }, GRAY); // lit floor
    Mge_EndLighting3D();
    if (sel >= 0) Mge_Gizmo3D(&box.position, &box.rotation, &box.size, camera, 2.0f);
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
(spotlight shows a hard vs. a soft cone side by side); `blinn_phong` toggles the
two specular models over a low-shininess floor; `gamma_correction` toggles sRGB
output; `shadow_mapping` casts a directional shadow; `builder/main.c` combines a
directional fill with an orbiting point light.

### Shadow mapping

A directional or spot light casts shadows in two passes over the same geometry:

```c
ShadowMap sm = Mge_LoadShadowMap(2048);          // once; Mge_UnloadShadowMap(&sm) at the end
...
Mge_BeginShadowPass(&sm, sun, sceneCenter, sceneRadius); // pass 1: depth from the light
    DrawOccluders();                                     // world-space geometry only
Mge_EndShadowPass();

Mge_ClearBackground(bg);
Mge_BeginMode3D(camera);
    Mge_BeginLighting3DShadowed(&sun, 1, camera, sm);    // pass 2: lit, shadowed by lights[0]
        Mge_SetMaterial(mat);
        DrawScene();
    Mge_EndLighting3D();
Mge_EndMode3D();

Mge_DrawShadowMap(sm, 12, 12, 220);              // optional: blit the depth texture to debug
```

`center` / `radius` frame the light's view volume — pass the scene's bounding
sphere (too large softens the shadow, too small clips it). Only `lights[0]`
casts. The compare uses a 3×3 PCF filter and a slope-scaled bias; the depth
texture lands on texture unit 1, so material textures (unit 0) are unaffected.
`Mge_BeginShadowPass` must run before `Mge_ClearBackground` — it redirects
rendering to its own framebuffer and restores the window on `Mge_EndShadowPass`.

Demo: `examples/lighting/shadow_mapping.c` — a moving sun over a few blocks, with
the shadow map shown in the corner.

#### Point (omnidirectional) shadows

A point/spot light shadows in every direction, so pass 1 renders the occluders
into a depth **cubemap** — once per face — storing the distance from the light:

```c
PointShadowMap ps = Mge_LoadPointShadowMap(1024);
...
Mge_BeginPointShadowPass(&ps, lamp, 22.0f);   // farPlane = max shadow distance
    for (int f = 0; f < 6; f++) { Mge_SetPointShadowFace(f); DrawOccluders(); }
Mge_EndPointShadowPass();

Mge_BeginMode3D(camera);
    Mge_BeginLighting3DPointShadowed(&lamp, 1, camera, ps);  // lights[0] casts
        Mge_SetMaterial(mat);
        DrawScene();
    Mge_EndLighting3D();
Mge_EndMode3D();
```

The compare uses a 20-tap disk PCF. `Mge_BeginLighting3DShadowed` (2D map) and
`Mge_BeginLighting3DPointShadowed` (cube) are mutually exclusive — `lights[0]`
casts one kind or the other. The 2D map is on texture unit 1, the cube on unit 2.

Demo: `examples/lighting/point_shadows.c` — a lamp bobbing inside a room, cubes
casting onto the walls, floor and ceiling.

### Normal mapping

Put a tangent-space normal map in `MATERIAL_MAP_NORMAL` and lit surfaces pick up
per-pixel bumps — no tangent vertex attribute needed: the lighting shader builds
the TBN frame from screen-space derivatives of position and UV.

```c
Material wall = Mge_DefaultMaterial();
Mge_SetMaterialTexture(&wall, MATERIAL_MAP_DIFFUSE, Mge_LoadTexture("brick.jpg"));
Mge_SetMaterialTexture(&wall, MATERIAL_MAP_NORMAL, Mge_LoadTexture("brick_normal.jpg"));
// ... Mge_SetMaterial(wall); Draw_Cube(...);   // inside Mge_BeginLighting3D
```

Load the normal map **linear** (`Mge_LoadTexture`, never `...Ex(path, true)`) — it
is vector data, not colour. OpenGL-convention maps (green = +Y) work as-is. It
binds to texture unit 3. `Mge_LoadModel` picks up `NORMALS` / `HEIGHT` textures
automatically, so imported models are normal-mapped without extra code.

Demo: `examples/lighting/normal_mapping.c` — `assets/brickwall/` on a flat quad,
SPACE toggles the map.

### Parallax mapping

Add a grayscale **depth map** to `MATERIAL_MAP_HEIGHT` and the lighting shader
does parallax-occlusion mapping: it marches the depth field in tangent space
along the view ray and shifts the sampled texture coordinates to the hit, so a
flat quad looks genuinely displaced — grooves hide behind ridges at grazing
angles, with a stepped silhouette. Same derivative-based tangent frame as the
normal map (no tangent attribute); binds to texture unit 4.

The map follows LearnOpenGL's convention: **black = surface, white = deep groove**
(their `bricks2_disp.jpg`). If you have a *height* map (white = high), invert it
first.

```c
Material wall = Mge_DefaultMaterial();
Mge_SetMaterialTexture(&wall, MATERIAL_MAP_DIFFUSE, albedo);
Mge_SetMaterialTexture(&wall, MATERIAL_MAP_NORMAL, normal);   // pair them
Mge_SetMaterialTexture(&wall, MATERIAL_MAP_HEIGHT, depth);    // black = surface, white = deep
wall.maps[MATERIAL_MAP_HEIGHT].value = 0.1f;                  // displacement scale
// ... Mge_SetMaterial(wall); Draw_Cube(...);
```

Load the depth map **linear**. Keep `.value` small (`0.05`–`0.15`) — large scales
smear at oblique angles. Meshes (`Mge_DrawMesh`) don't do parallax.

Demo: `examples/lighting/parallax_mapping.c` — LearnOpenGL's `assets/bricks/`
set; SPACE toggles parallax, UP/DOWN the scale, N the normal map.

### Materials & material maps

A `Material` is a fixed set of `MaterialMap` slots (indexed by
`MaterialMapIndex`) plus a specular `shininess`. Each map carries a **texture**, a
**color** and a scalar **value**; what those mean depends on the slot:

| slot | `.texture` | `.color` | `.value` |
| --- | --- | --- | --- |
| `MATERIAL_MAP_DIFFUSE` | albedo image sampled across the surface (id `0` → a white 1×1, i.e. "untextured") | tint multiplied over the texture | base-colour gain (`1` = as-is, `>1` brighter) |
| `MATERIAL_MAP_SPECULAR` | unused | tints the highlight (`WHITE` = untinted) | highlight strength multiplier: `1` = as the light sets it, `0` = matte |
| `MATERIAL_MAP_NORMAL` | tangent-space normal map (RGB = XYZ); load it linear. Unset → the vertex normal is used | unused | strength: `0` = flat, `1` = as authored, `>1` = exaggerated relief |
| `MATERIAL_MAP_HEIGHT` | grayscale depth map (black = surface, white = groove — `bricks2_disp.jpg`); load it linear. Set → parallax-occlusion mapping displaces the sampled UVs along the view ray | unused | height scale (`~0.05` subtle … `0.15` strong; `0` = off) |

`MATERIAL_MAP_DIFFUSE.value` defaults to `1` and `MATERIAL_MAP_HEIGHT.value` to
`~0.08` via `Mge_DefaultMaterial()` — a **raw `(Material){…}` literal** must set
the ones it uses (`0` = black diffuse / flat normal). Parallax and the normal map
share the shader's derivative-based tangent frame; pair them for the best result.

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
// with gamma correction on, load colour maps as sRGB instead:
// Texture2D wall = Mge_LoadTextureEx("assets/wall.jpg", true);

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

`Mge_SetMaterial` binds the diffuse + normal textures and sets the per-slot
uniforms (diffuse gain, specular strength + tint, normal strength, shininess);
the diffuse **color** reaches the shader through the drawn geometry's per-vertex
colour, so pass it to `Draw_Cube` (or `MgeGL_Color4ub`).
`Mge_DrawObject` does this for an `Object` automatically —
`obj.material.maps[MATERIAL_MAP_DIFFUSE].color` is seeded from the colour you
gave `Mge_MakeObject3D`. `Draw_Cube` emits per-face UVs so a texture maps one
full copy onto every face. A texture that fails to load has id `0` and falls
back to the flat colour.

Free a texture you loaded with `Mge_UnloadTexture(tex)` (id `0` and the shared
white texture are ignored) — do this before overwriting a slot so you don't leak
the old one.

Demo: `examples/materials/textured_cube.c` — textured/tinted/matte/plain cubes
side by side under a moving light.

#### Wrap mode

How a texture samples outside `0..1` UV. New textures are `TEXTURE_WRAP_REPEAT`;
change it once after loading:

```c
Mge_SetTextureWrap(tex, TEXTURE_WRAP_CLAMP);              // both axes
Mge_SetTextureWrapEx(tex, TEXTURE_WRAP_REPEAT, TEXTURE_WRAP_CLAMP); // U repeats, V clamps
```

| `TextureWrap` | GL | use |
| --- | --- | --- |
| `TEXTURE_WRAP_REPEAT` | `GL_REPEAT` | tiling — brick, grass, terrain (the default) |
| `TEXTURE_WRAP_CLAMP` | `GL_CLAMP_TO_EDGE` | decals, UI, spotlight cookies — no pattern loop |
| `TEXTURE_WRAP_MIRROR_REPEAT` | `GL_MIRRORED_REPEAT` | seamless-by-mirroring; flips at every integer boundary |
| `TEXTURE_WRAP_MIRROR_CLAMP` | `GL_MIRROR_CLAMP_TO_EDGE` | "mirror once" — mirror across one boundary, then clamp |

The state lives on the GL texture object, so set it once (it isn't stored in
`Material`). The builder's inspector has a per-slot **wrap** dropdown.

#### Tiling, offset & triplanar

Changing the wrap mode to `REPEAT` does **not** repeat a texture on its own — you
also have to scale the UVs. That's a per-material transform applied to every map:

```c
mat.tiling = (Vector2){ 4.0f, 4.0f }; // uv' = uv*tiling + offset -> 16 copies, no stretch
mat.offset = (Vector2){ 0.25f, 0.0f };
```

To keep texels **square when an object is scaled non-uniformly**, turn on
triplanar projection — it samples the maps from world-space XYZ (blending the
three axis planes by the normal) instead of the mesh UVs, so stretching the
geometry tiles the texture rather than smearing it:

```c
mat.triplanar = true;
mat.triplanarScale = 1.0f;            // world units per texture tile
```

The **normal map** (whiteout blend) and **height map** (a per-plane
parallax-occlusion march, offset-limiting) both follow the projection. `tiling`
does **not** apply under triplanar — scale it with `triplanarScale`.
`Mge_DefaultMaterial()` sets `tiling {1,1}`, `offset {0,0}`, `triplanar false`.
Demo: `examples/materials/tiling_triplanar.c`.

#### Picking a texture at runtime

`Mge_OpenImageDialog()` pops the OS "open file" dialog (Windows: comdlg32;
Linux: `zenity`, then `kdialog`) filtered to image extensions and returns a
**malloc'd** path you `free()`, or `NULL` on cancel / when no backend is present.
`Mge_OpenFileDialog(title, filterName, filterExts)` is the general form
(`filterExts` is `;`-separated, e.g. `"*.png;*.jpg"`). The dialog never changes
the process working directory. The builder's inspector uses it for the three
material-map thumbnails (`Mge_GuiImageButton`).

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

**Object outlining** is the built-in use. `Mge_DrawObject` draws a bold stencil
outline (a thick orange border, not a wireframe) around any `Object` whose
`.selected` flag is set. For anything else, three calls wrap the technique:

```c
Mge_BeginStencilMask();                        // stamp the silhouette:
    Draw_Cube(pos, size, col);                  //   colour + depth writes are off
Mge_BeginStencilOutside();                      // now draw only outside the stamp:
    Draw_Cube(pos, biggerSize, WHITE);          //   just the border survives
Mge_EndStencil();                              // restore normal drawing
```

`Mge_DrawObjectOutline(obj, thickness, color)` does exactly that for one
`Object` you have already drawn this frame (`thickness` is added to its
extents). The mask pass stamps the full silhouette (depth test off) so the
outline shows even when the object is partly occluded; the border pass draws it
flat/unlit with `glDepthFunc(GL_ALWAYS)` but still writes depth, so a later
"draw last" pass such as a skybox can't paint over it. Override the colour /
thickness with `-DMGE_SELECT_OUTLINE_COLOR` / `_3D` / `_2D`.

Demo: `examples/stencil/object_outline.c` — a walking selection outlines each
cube in turn, plus one hand-outlined pillar in a custom colour.

### Face culling

Off by default -- the engine never enables it, so 2D shapes and lines are
unaffected. Turn it on around 3D geometry to skip triangles pointing away from
the camera:

```c
void Mge_EnableFaceCulling(void);  void Mge_DisableFaceCulling(void);
void Mge_SetCullFace(int face);      // CULL_BACK (default) / CULL_FRONT / CULL_FRONT_AND_BACK
void Mge_SetFrontFace(int winding);  // WINDING_CCW (default) / WINDING_CW
```

`Draw_Cube` and imported meshes wind counter-clockwise, so `CULL_BACK` "just
works". 2D shapes have mixed winding — disable culling before drawing them (or
only enable it inside `Mge_BeginMode3D`). Demo:
`examples/culling/backface_cull.c` cycles off / back / front.

### Framebuffers & post-processing

Render the scene into a `RenderTexture` (an FBO with a colour texture + a
depth/stencil renderbuffer), then draw that texture full-screen through an
effect shader.

```c
RenderTexture Mge_LoadRenderTexture(int width, int height); // usually the window size
void Mge_UnloadRenderTexture(RenderTexture target);
void Mge_BeginTextureMode(RenderTexture target);   // drawing now goes into target
void Mge_EndTextureMode(void);                     // back to the window
void Mge_DrawRenderTextureFX(RenderTexture target, int effect); // a PostFX
```

```c
RenderTexture rt = Mge_LoadRenderTexture(w, h);

Mge_BeginDrawing();
    Mge_BeginTextureMode(rt);
        Mge_ClearBackground(DARKGRAY);
        Mge_BeginMode3D(cam);
            Mge_BeginLighting3DEx(&light, 1, cam); /* ... */ Mge_EndLighting3D();
        Mge_EndMode3D();
    Mge_EndTextureMode();

    Mge_ClearBackground(BLACK);
    Mge_DrawRenderTextureFX(rt, POSTFX_EDGE);
Mge_EndDrawing();
```

`PostFX`: `POSTFX_NONE` (blit), `POSTFX_INVERT`, `POSTFX_GRAYSCALE`,
`POSTFX_SHARPEN`, `POSTFX_BLUR`, `POSTFX_EDGE` — the last three are 3×3 kernels
(`texelSize` = 1 / render-texture size). One shader program handles all of them,
switched by the `effect` uniform. Keep the render texture the same size as the
window so 2D coordinates and 3D aspect line up.

Demo: `examples/framebuffer/post_process.c` cycles through every effect.

For a **floating-point** target (values above `1.0` survive), use
`Mge_LoadRenderTextureHDR` + `Mge_DrawRenderTextureHDR(target, TONEMAP_*, exposure)`
— see [HDR & tone mapping](#hdr--tone-mapping) and [Bloom](#bloom).

### Cube maps, skybox & environment mapping

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

### Geometry shaders

Two built-in effects run a geometry stage over the batcher's triangles (works
with `Draw_Cube`, `Mge_DrawMesh`, `Mge_DrawModel`) — call inside `Mge_BeginMode3D`:

```c
Mge_BeginExplode3D(0.5f);             // push each triangle out along its face normal
    Draw_Cube(pos, size, RED);
Mge_EndExplode3D();

Mge_BeginNormals3D(0.2f, YELLOW);     // a short line along every vertex normal
    Draw_Cube(pos, size, WHITE);
Mge_EndNormals3D();
```

The explode magnitude is a plain float — animate it yourself. To roll your own
geometry-shader pass, `MgeGL_LoadShader(src, GL_GEOMETRY_SHADER, name)` +
`MgeGL_CreateShaderProgramGeo(vs, gs, fs)`; the vertex shader gets `modelview`
and the geometry shader `projection` from the batcher.

Demo: `examples/geometry/geometry_shader.c` — a cube showing its normals beside
one that pulses apart and back.

### Instancing

`ModelBatch` draws hundreds of copies of one `Model` with a single
`glDrawElementsInstanced` per mesh — the CPU submits nothing per copy. The
per-instance model matrices are packed into one GPU buffer that is bound onto
the model's mesh VAOs as a `mat4` vertex attribute (locations 4–7).

```c
Model melon = Mge_LoadModel("assets/sliced_musk_melon/scene.gltf");

Matrix xf[200];
for (int i = 0; i < 200; i++)
    xf[i] = Matrix_Multiply(                       // Matrix_Multiply(A, B) = A then B
        Matrix_Multiply(Matrix_Scale(s, s, s),
                        Matrix_Rotate((Vector3){ 0, 1, 0 }, angle)),
        Matrix_Translate(x, y, z));                // -> scale, then rotate, then translate

ModelBatch field = Mge_LoadModelBatch(melon, xf, 200);

// each frame, inside Mge_BeginMode3D:
Mge_DrawModelBatch(field, sun, camera);           // lit by one light (directional or point)

// optional: recompute xf[] and re-upload (count clamped to the original)
Mge_UpdateModelBatch(&field, xf, 200);

Mge_UnloadModelBatch(&field);                      // frees the instance buffer only
Mge_UnloadModel(&melon);                           // the model is not owned by the batch
```

`Mge_DrawModelBatch` uses its own shader (not the `Mge_BeginLighting3D` one), so
call it on its own — not between `Mge_BeginLighting3D` / `Mge_EndLighting3D`. One
live batch per `Model` at a time: the instance attributes are bound onto the
shared mesh VAOs, so a second batch over the same model overwrites the first.

Demo: `examples/instancing/melon_field.c` — 64 spinning melons on a jittered
grid, one draw call per mesh, camera orbiting.

### GUI (`mge_gui.h`)

An immediate-mode UI abstracted over Dear ImGui — the backend is baked into
`libmgengine`, so consumers include `<mge_gui.h>` and call plain C. No ImGui
types leak out.

```c
#include <mge_gui.h>

Mge_GuiBeginFrame();                         // after the 3D/2D scene
    if (Mge_GuiBeginSidebar("Scene", 300, false)) {   // full-height dock on the left
        if (Mge_GuiSelectable("Cube 0", sel == 0)) sel = 0;
        Mge_GuiSeparator();
        Mge_GuiInputVec3("position", &obj.position);   // "draw input" for a Vector3
        Mge_GuiInputColor("diffuse", &mat.maps[MATERIAL_MAP_DIFFUSE].color); // 8-bit RGBA swatch
        Mge_GuiSliderFloat("shininess", &mat.shininess, 1, 128);
        if (Mge_GuiImageButton("albedo", tex.id, 56.0f)) { /* open a file picker */ }
    }
    Mge_GuiEndSidebar();
Mge_GuiEndFrame();                           // renders on top of the framebuffer
```

| kind | calls |
| --- | --- |
| frame | `Mge_GuiBeginFrame` / `Mge_GuiEndFrame`, `Mge_GuiShutdown` |
| boxes | `Mge_GuiBeginBox` (floating panel) / `Mge_GuiBeginSidebar` (edge dock) + matching `End*` |
| widgets | `Mge_GuiLabel`, `Mge_GuiSeparator`, `Mge_GuiSpacing`, `Mge_GuiSameLine`, `Mge_GuiButton`, `Mge_GuiSelectable`, `Mge_GuiImageButton` (square texture thumbnail; id `0` → a "+" placeholder) |
| inputs | `Mge_GuiCheckbox`, `Mge_GuiCombo` (dropdown), `Mge_GuiInputInt/Float`, `Mge_GuiSliderFloat`, `Mge_GuiInputVec2/Vec3`, `Mge_GuiInputColor` (8-bit RGBA), `Mge_GuiInputColorRGB` (0..1 linear, e.g. `Light.color`) |

Every input returns `true` the frame its value changes; `Mge_GuiSelectable` /
`Mge_GuiButton` return `true` on click. Gate your own picking and camera on
`Mge_GuiWantsMouse()` / `Mge_GuiWantsKeyboard()` so widgets don't fight the
viewport. The backend boots lazily on the first `Mge_GuiBeginFrame` after
`Mge_InitWindow`; apps that never call it pay nothing.

`builder/` is the worked example — a left sidebar with a type-aware inspector
(Object vs Light) plus a right-edge shape palette, both drawn in one
`Mge_GuiBeginFrame` / `Mge_GuiEndFrame` pair. See
[builder/USAGE.md](builder/USAGE.md).

### Math

`glm` is gone. `mge_math.h` provides plain-C functions — no operator overloads:

| | |
| --- | --- |
| `Vector3_Add/Subtract/Scale/Multiply(a, b)` | `Vector3_DotProduct`, `Vector3_Length` |
| `Vector3Cross`, `Vector3Normalize` | `Vector2_Rotate(v, radians)`, `Clamp` |
| `Matrix_Identity/Multiply/Translate/Scale/Rotate` | `MatrixOrtho/Perspective/LookAt`, `MatrixToFloatV` |

Matrices are stored column-major so `MatrixToFloat(m)` feeds `glUniformMatrix4fv`
directly.

## Notes / limitations

- Engine sources are C11 (`mge_gui.cpp` is the lone C++ unit); everything builds
  under `-Wall -Wextra`. The `test/` suite needs no window or GL context; the
  window / renderer itself does.
- `make` needs `vendor/{glfw,assimp}/lib` populated first (`make vendor`).
  Dear ImGui (`vendor/imgui/`, v1.90.5) is vendored as source and compiled into
  the DLL — no separate build step, and header/binary versions can't drift.
- `glm` is gone; `vendor/glm/` was deleted.

## References

External material this engine's design and shaders are based on.

| Source | Used for |
| --- | --- |
| [LearnOpenGL](https://learnopengl.com/) — Getting Started / Lighting | core renderer, camera, the Phong → Blinn-Phong lighting model (`mge_light.c`), material maps |
| LearnOpenGL — [Depth testing](https://learnopengl.com/Advanced-OpenGL/Depth-testing) / [Stencil testing](https://learnopengl.com/Advanced-OpenGL/Stencil-testing) / [Face culling](https://learnopengl.com/Advanced-OpenGL/Face-culling) | `mge_depth.c`, `mge_stencil.c` + object outlining, `mge_cull.c` |
| LearnOpenGL — [Framebuffers](https://learnopengl.com/Advanced-OpenGL/Framebuffers) / [Cubemaps](https://learnopengl.com/Advanced-OpenGL/Cubemaps) | `RenderTexture` + post-processing kernels, skybox + environment mapping (`mge_cubemap.c`) |
| LearnOpenGL — [Instancing](https://learnopengl.com/Advanced-OpenGL/Instancing) / [Anti-Aliasing](https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing) / [Geometry Shader](https://learnopengl.com/Advanced-OpenGL/Geometry-Shader) | `mge_instancing.c` (`ModelBatch`), MSAA (`mge_msaa.c`), explode / normal-viz (`mge_geometry.c`) |
| LearnOpenGL — [Advanced Lighting](https://learnopengl.com/Advanced-Lighting/Advanced-Lighting) / [Gamma Correction](https://learnopengl.com/Advanced-Lighting/Gamma-Correction) / [HDR](https://learnopengl.com/Advanced-Lighting/HDR) / [Bloom](https://learnopengl.com/Advanced-Lighting/Bloom) / [Deferred Shading](https://learnopengl.com/Advanced-Lighting/Deferred-Shading) / [SSAO](https://learnopengl.com/Advanced-Lighting/SSAO) | Blinn-Phong specular, `GL_FRAMEBUFFER_SRGB` + sRGB texture loading (`mge_gamma.c`), RGBA16F render target + tone mapping (`mge_framebuffer.c`), bright-pass + Gaussian bloom (`mge_bloom.c`), G-buffer + full-screen lighting pass (`mge_deferred.c`), hemisphere-kernel ambient occlusion (`mge_ssao.c`) |
| LearnOpenGL — [Shadow Mapping](https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping) / [Point Shadows](https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows) | `ShadowMap` / `PointShadowMap`, the depth pass + PCF (`mge_shadow.c`) |
| LearnOpenGL — [Normal Mapping](https://learnopengl.com/Advanced-Lighting/Normal-Mapping) / [Parallax Mapping](https://learnopengl.com/Advanced-Lighting/Parallax-Mapping) | derivative-TBN normal maps, parallax-occlusion mapping (`MATERIAL_MAP_NORMAL` / `MATERIAL_MAP_HEIGHT`); `assets/bricks/` is LearnOpenGL's `bricks2` set |
| LearnOpenGL — [Advanced Data](https://learnopengl.com/Advanced-OpenGL/Advanced-Data) | batched vertex attributes (`Mge_MakeMeshFromArrays` — one VBO, block per attribute) |
| raylib / [rlgl](https://github.com/raysan5/raylib/blob/master/src/rlgl.h) | the immediate-mode batched renderer design (`mge_gl.c` — `MgeGL_Begin` / `Vertex` / `End` → merged draw calls, matrix stack) and the public API shape (`Draw_*`, `Color`, `Rectangle`, `Camera3D`, `Texture2D`, wrap modes) |
| [Ben Golus — "Normal Mapping for a Triplanar Shader"](https://bgolus.medium.com/normal-mapping-for-a-triplanar-shader-10bf39dca05a) | the whiteout-blend triplanar normal mapping in `mge_light.c` |
| [landow.dev — "Triplanar Mapping with Deep Parallax"](https://www.landow.dev/posts/triplanar/) | per-plane parallax-occlusion under triplanar (offset-limiting march) |
| Unreal Engine editor | the rotate gizmo — full-circle rings with only the camera-facing arc drawn (`mge_gizmo.c`) |
| [Dear ImGui](https://github.com/ocornut/imgui) | the `Mge_Gui*` UI backend (`mge_gui.cpp`) |
| [MahdiyDev/mlib](https://github.com/MahdiyDev/mlib) | the `test/` harness and small container helpers |

Vendored libraries: [GLFW](https://www.glfw.org/) (windowing/input), [glad](https://gen.glad.sh/) (GL loader), [Assimp](https://github.com/assimp/assimp) (model import), [stb_image](https://github.com/nothings/stb) (image decode).
