# Overview & building

## Layout

```
source/                THE ENGINE -- every *.c here is compiled into the library
  mge.h            public types + core / shapes / texture / input API
  mge_gl.h  mge_gl.c     immediate-mode-ish batched GL renderer (MgeGL_*)
  mge_math.h mge_math.c  Vector2/3/4, Matrix, Quaternion, projections (replaces glm)
  mge_core.c            window, timing, input, shaders, camera
  mge_shapes.c          Draw_Line / Draw_Rectangle[Rounded] / Draw_Triangle / Draw_Arrow / Draw_Cube / Draw_Sphere / Draw_Plane ...
  mge_text.c            Font + Draw_Text / Draw_Text3D / Mge_MeasureText -- stb_truetype atlas + a built-in bitmap font
  mge_ui.c              Mge_Ui* retained widget GUI (Flutter-shaped box model; games)
  mge_object.c          Object struct (Transform + components, active flag) + 3D picking
  mge_component.c       Object components (Shape / Material / Collider / RigidBody) + accessors
  mge_body.c            linear rigid-body step + box/sphere collider overlap + resolution
  mge_gizmo.c           switchable translate / rotate / scale manipulation gizmo
  mge_dialog.c          native dialogs (Mge_OpenFileDialog / Image / Save / Folder)
  mge_light.c          Blinn-Phong lighting; directional / point / spot; normal maps
  mge_pbr.c            physically-based rendering -- Cook-Torrance BRDF + material
  mge_ibl.c            image-based lighting precompute (irradiance / prefilter / BRDF LUT)
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
  mge_gui.h  mge_gui.cpp   Mge_Gui* immediate-mode UI (Dear ImGui backend; the one C++ unit; editor)
  mge_ui.h   mge_ui.c      Mge_Ui* retained widget GUI (pure C; game menus / HUD)
  mge_texture.c         Mge_LoadImage / Mge_LoadTexture / ...Ex (sRGB) / ...HDR (float) / Mge_UnloadTexture / Mge_SetTextureWrap (stb_image)
  mge_screenshot.c     Mge_TakeScreenshot / MgeGL_SaveScreenshot -- framebuffer -> PNG (stb_image_write)
  mge_dylib.c          Mge_LoadLibrary / GetSymbol / FreeLibrary -- host side of the hot-reload scene-module contract
  mge_pak.c mge_pak.h  .pak archives (Mge_PakWrite / Open / Read) + Mge_MountPak; Mge_LoadFileData falls back to a mounted pak
  mge_utils.h mge_utils.c   Trace_Log, file loading
  platforms/mge_code_desktop.c   GLFW backend (#included by mge_core.c)
editor/                THE APP -- project / scene editor (docked panel shell around a viewport)
  main.c               window, loop, panel-rectangle layout, close guard; owns the Project + Scene
  editor_camera.c/.h   the yaw/pitch fly-cam (VIEW = always fly, EDIT = fly on RIGHT mouse)
  project.c/.h         Project struct: global config + scene list + path helpers
  project_io.c/.h      project.mgproject read/write -- flat text, data only (reads via Mge_LoadFileText, so a pak works)
  scene.c/.h           entities, selection, picking, add/delete/new, the render passes
  scene_io.c/.h        .mgscene read/write (Scene_Save / Scene_Load) -- flat text, data only
  pathutil.c/.h        path + fs helpers (dir/base/join/equal/mkdirs/copyfile/list/remove/nextline)
  scene_build.c/.h     compile a scene's *.c -> hot-reloadable .dll; BuildLog
  scene_runtime.c/.h   load / hot-reload a compiled scene module (SceneRuntime)
  play.c/.h            Play / Stop / Build + the build console
  release.c/.h         "Build Bundle": compile every scene + pak all data (project.mgproject too) -> dist/packs/data.pak.NNN; scene modules -> dist/scenes/scene.N.dll
  fileops.c/.h         Project + Scene menu actions + the unsaved-changes / name modals
  topbar.c/.h          top strip: Project menu, Scene dropdown, Play/Build/Console, mode, gizmo, Render
  hierarchy.c/.h       left panel: entity list, + add menu, rename / toggle / delete
  inspector.c/.h       right panel: the type-aware inspector (+ texture slots)
  resources.c/.h       bottom panel: project res/ browser (import / rename / delete / assign)
  USAGE.md             editor docs
docs/                  engine docs (this folder), split by area
runtime/
  player.c             standalone project runner -- reuses the editor data layer; what Build Bundle ships
vendor/
  glad/                glad GL loader -- include/ + glad.c (compiled into the engine)
  stb/                 stb_image.h, stb_image_write.h, stb_truetype.h (single-header, public domain)
  mlib/                MahdiyDev/mlib (containers, test harness)
  imgui/               Dear ImGui 1.90.5 source (compiled straight into the engine)
  glfw/                GLFW -- vendored source; `make vendor-glfw` builds lib/ + include/
  assimp/              Assimp OBJ/glTF2/FBX importers -- pruned source under
                       source/; `make vendor-assimp` builds lib/ + include/
test/                  unit tests (no window/GL); test/glstub/ = a fake glad so mge_gl.c itself is testable
examples/shapes/       draw_line, draw_rectangle, draw_triangle, mixed
examples/objects/      gizmo_2d, gizmo_3d
examples/lighting/     ambient, diffuse, specular, directional, point, spotlight, blinn_phong, gamma_correction, hdr, bloom, deferred_shading, ssao, shadow_mapping, point_shadows, normal_mapping, parallax_mapping
examples/pbr/          spheres (Cook-Torrance + IBL, metallic x roughness grid)
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
make               # debug   -> build/{libmgengine.(dll|so),editor,mgeplayer}
make release       # release -> build/release/{...}  (PRODUCTION build, see below)
make lib           # -> just the library (of the current config)
```

`make` is a **debug** build: `-O0 -g`, assertions on — handy while developing,
but noticeably slow. `make release` rebuilds every object with `-O2 -DNDEBUG`,
strips symbols and lets the linker drop unused code — this is the one to run for
real (typically several times the frame rate of the debug build, ~40 % smaller
binaries). The two configs have **separate object caches and output dirs**
(`build/` vs `build/release/`), so `make` and `make release` coexist — run both
to keep a debug and a release set side by side. Override flags per-invocation
with e.g. `make CFLAGS="-O3 -march=native"` (lands in `build/`).

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
and GLFW / Assimp / Dear ImGui are already inside), then builds `editor/*.c`
against it with plain `gcc -Isource -lmgengine`.
`make_build_dir` stages `assets/` (and `shaders/`) plus the public headers
(`source/*.h` -> `<conf>/include/`) into the config's own dir, and the DLL sits
next to `editor.exe` / `mgeplayer.exe` there, so each config runs from its own
folder (`build/` or `build/release/`). The staged headers let an editor project
point its `compile_flags.txt` at `<conf>/include` for scene-script IntelliSense.

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
toggle and callback registration; `test_dylib` compiles a tiny shared library
with the C compiler and loads / calls / frees it through `Mge_LoadLibrary`;
`test_pak` writes + reads a `.pak` (crc, split-file spanning, mount stack);
`test_scene_io` / `test_project_io` round-trip the editor's `.mgscene` /
`.mgproject` text formats; `test_physics` covers the raycast primitives, the
nearest-hit object sweep, screen→ray unprojection, box/sphere collider overlap
and one linear rigid-body step; `test_component` covers the component array —
add / remove / has / get, typed vs generic accessors, the seeded defaults and
what the `Mge_Make*` constructors attach; `test_text` covers the built-in font,
glyph metrics, `Mge_MeasureText`'s cursor walk and the batch emission of
`Draw_Text` / `Draw_Text3D` (also against the fake glad).

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
GLFW window, renders ~27 engine features (2D shapes, a lit cube, a shadow map,
a post-fx pass, the skybox, a normal-mapped wall, a parallax-mapped wall, a
mirror-repeat wrapped quad, a tiled plane + a triplanar box, an HDR scene
tone-mapped vs clamped, a bloom glow, a deferred-shaded scene, an SSAO scene,
a PBR + IBL sphere grid, the cube/sphere/plane primitives, a rotated cube with
each gizmo mode, the rotate gizmo head-on, a scripted rotate drag, a raycast
against two shapes with the hit marker drawn, a camera marker's view frustum,
a wire box / wire sphere / rotated plane / arrow / triangle gallery, a
`MgeGL_SaveScreenshot` round-trip) one frame each, reads the
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
        Draw_Text(Mge_GetDefaultFont(), "hello", (Vector2){ 20, 20 }, 24, WHITE);

        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
```

`Mge_WindowShouldClose()` latches true on the window's X button or **ESC**.
`Mge_SetWindowShouldClose(false)` clears it — call it to cancel a quit and show a
"save first?" prompt, then exit the loop yourself once the user confirms (the
editor's close guard does this).

3D uses a `Camera3D` (passed **by value**) between `Mge_BeginMode3D` /
`Mge_EndMode3D`; draw with `Draw_Cube` / `Draw_CubeWires` / `Draw_Sphere[WiresEx]` /
`Draw_Plane` / `Draw_Quad3D[Wires]` / `Draw_Line3D` / `Draw_Polygon3D[Wires]` /
`Draw_Arrow3D` / `Draw_CameraFrustum` (and the `*Ex` variants) or the
low-level `MgeGL_Begin(MGEGL_TRIANGLES)` … `MgeGL_Vertex3f` … `MgeGL_End` immediate
calls. `editor/` shows a fly-camera plus TAB-toggled edit mode with the
translate / rotate / scale gizmo.

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
| LearnOpenGL — [PBR: Theory](https://learnopengl.com/PBR/Theory) / [Lighting](https://learnopengl.com/PBR/Lighting) / [IBL: Diffuse irradiance](https://learnopengl.com/PBR/IBL/Diffuse-irradiance) / [Specular IBL](https://learnopengl.com/PBR/IBL/Specular-IBL) | the Cook-Torrance BRDF + metallic/roughness material (`mge_pbr.c`) and the irradiance / prefilter / BRDF-LUT precompute (`mge_ibl.c`); `assets/hdr/newport_loft.hdr` and `assets/pbr/rusted_iron/` are LearnOpenGL's resources |
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
