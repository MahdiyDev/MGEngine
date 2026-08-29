# MGEngine

A small raylib-style 2D/3D rendering engine. **Pure C11** — no C++, no glm, no
Dear ImGui. OpenGL 4.4 core via GLFW + glad, image loading via stb_image.

## Layout

```
source/
  mge.h            public types + core / shapes / texture / input API
  mge_gl.h  mge_gl.c     immediate-mode-ish batched GL renderer (MgeGL_*)
  mge_math.h mge_math.c  Vector2/3/4, Matrix, projections (replaces glm)
  mge_core.c            window, timing, input, shaders, camera
  mge_shapes.c          Draw_Line / Draw_Rectangle / Draw_Triangle / Draw_Arrow / Draw_Cube ...
  mge_object.c          Object struct + mouse-driven translation gizmo
  mge_light.c          Phong lighting (ambient + diffuse + specular)
  mge_texture.c         Mge_LoadImage / Mge_LoadTexture (stb_image)
  mge_utils.h mge_utils.c   Trace_Log, file loading
  platforms/mge_code_desktop.c   GLFW backend (#included by mge_core.c)
  main.c               demo: fly-camera + TAB edit mode + combined lighting
test/                  unit tests for math + file utils (no window/GL needed)
examples/shapes/       draw_line, draw_rectangle, draw_triangle, mixed
examples/objects/      gizmo_2d, gizmo_3d
examples/lighting/     ambient, diffuse, specular
```

## Using mlib

[mlib](https://github.com/MahdiyDev/mlib) is vendored in `3rdparty/mlib/`
(add `-I3rdparty/mlib -I3rdparty/mlib/vec`). The engine uses two of its containers:

- `mge_utils.c` loads files into an mlib `string_builder` (`sb_read_file`).
- `mge_gl.c` keeps the render batch's **draw-call list** and **matrix stack** as
  `DEFINE_VEC(...)` vectors, so neither has a fixed cap any more.

## Building

The engine links against GLFW, which is vendored as source only. Build it once:

```sh
make 3rdparty        # runs cmake on 3rdparty/glfw/source  (needs cmake)
make                 # -> build/MGEngine
```

`make` compiles every `source/*.c` (the desktop platform file is `#include`d by
`mge_core.c`, not compiled on its own). It first runs `make_build_dir`, which
creates `build/obj/` and copies `assets/` (and `shaders/`, if either exists) into
`build/` so the executable can be run from either the repo root or `build/`.

On Windows use `mingw32-make`. The Makefiles pin `SHELL := cmd.exe`, so the
recipes work whether or not an `sh`/Git-Bash shell is on `PATH`.

### Tests (no GLFW / GL context required)

```sh
make test            # or:  cd test && make
```

`test_math` covers the vector/matrix layer; `test_utils` covers
`Mge_GetFileExtension` and the file loaders.

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
calls. `source/main.c` shows a fly-camera plus TAB-toggled edit mode with the
move gizmo.

### Cursor

| Function | Effect |
| --- | --- |
| `ShowCursor()` / `HideCursor()` | toggle cursor **visibility** (the cursor still moves freely) |
| `EnableCursor()` | show **and** unlock the cursor |
| `DisableCursor()` | hide **and** lock the cursor to the window centre (FPS style, enables raw mouse motion) |
| `Mge_ToggleCursor()` | flip between `EnableCursor()` and `DisableCursor()` |
| `IsCursorHidden()` | `true` while the cursor is hidden / locked |

Bind it to a key in your loop — `source/main.c` uses **TAB** to free and re-lock
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
void   Mge_DrawObject(Object obj);                        // filled shape (+ outline when selected)
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

- a **`Light`** is a *scene entity*: its `position`, `color`, and the strength of
  each term (`ambient` / `diffuse` / `specular`). There can be several; you pass
  one to `Mge_BeginLighting3D`.
- a **`Material`** is the *surface response* and is a field on `Object`
  (`obj.material`): the surface `color` and its `shininess`. That is the answer
  to "should lighting be in the material, and attached to the object?" — the
  *surface* parameters are; the *light* itself is separate.

```c
Light    Mge_MakeLight(Vector3 position, Vector3 color); // defaults: ambient .15, diffuse 1, specular .5
Material Mge_DefaultMaterial(void);                      // WHITE, shininess 32
void     Mge_BeginLighting3D(Light light, Camera3D camera);
void     Mge_SetMaterial(Material material);             // per-surface; no-op unless lighting is active
void     Mge_EndLighting3D(void);                        // restore the default (unlit) shader
```

Call it inside `Mge_BeginMode3D`. `Mge_DrawObject` sets the object's own material
for you, so lit objects just work:

```c
Light light = Mge_MakeLight((Vector3){ 4, 6, 4 }, (Vector3){ 1, 1, 1 });
Object box  = Mge_MakeObject3D((Vector3){ 0, 0, 0 }, (Vector3){ 1, 1, 1 }, RED);
box.material.shininess = 64.0f;

Mge_BeginMode3D(camera);
    Mge_BeginLighting3D(light, camera);
        Mge_DrawObject(box);                              // lit with box.material
        Mge_SetMaterial((Material){ .color = GRAY, .shininess = 8 });
        Draw_Cube((Vector3){ 0, -1, 0 }, (Vector3){ 24, 0.1f, 24 }, GRAY); // lit floor
    Mge_EndLighting3D();
    if (sel >= 0) Mge_DrawObjectGizmo(box, 1.6f);         // overlay lines stay unlit
Mge_EndMode3D();
```

Only geometry with per-vertex normals is shaded correctly — `Draw_Cube` emits
them; `MgeGL_Normal3f(x, y, z)` sets the current normal for your own
`MgeGL_Vertex3f` calls. Lines (`Draw_Arrow3D`, `Draw_CubeWires`) have no normals,
so draw them outside the `Begin/EndLighting3D` pair.

Isolated demos: `examples/lighting/{ambient,diffuse,specular}.c` each switch off
the other terms so you can see one at a time; `source/main.c` uses all three.

### Math

`glm` is gone. `mge_math.h` provides plain-C functions — no operator overloads:

| | |
| --- | --- |
| `Vector3_Add/Subtract/Scale/Multiply(a, b)` | `Vector3_DotProduct`, `Vector3_Length` |
| `Vector3Cross`, `Vector3Normalize` | `Vector2_Rotate(v, radians)`, `Clamp` |
| `Matrix_Identity/Multiply/Translate/Rotate` | `MatrixOrtho/Perspective/LookAt`, `MatrixToFloatV` |

Matrices are stored column-major so `MatrixToFloat(m)` feeds `glUniformMatrix4fv`
directly.

## Bugs fixed during the C port

- **Heap overflow in the GL batch** — `MgeGL_Init` allocated the vertex / colour /
  texcoord buffers for `MAX_BUFFER_ELEMENTS` vertices, but every bounds check used
  `MAX_BUFFER_ELEMENTS * 4`, so a full batch wrote ~4× past each buffer. Buffers
  are now sized `MAX_BUFFER_ELEMENTS * 4` vertices.
- **Matrix-stack overflow** — `MgeGL_PushMatrix` logged an error on overflow then
  wrote past `stack[32]` anyway. The stack is now an mlib `vec` (grows).
- `Mge_LoadFileText` / `Mge_LoadFileData` called `fclose(NULL)` when the file
  could not be opened.
- `Mge_LoadFileData`'s partial-load log had a wrong format string / argument
  count and truncated the size to `int`.
- `Mge_GetFps` did `roundf(1.0f / average)` with `average == 0` → `(int)INFINITY`.
- `MgeGL_LoadShader` returned `EXIT_FAILURE` (a plausible shader id) on failure
  and passed a possibly-`NULL` source to `glShaderSource`; it now returns `0` and
  rejects `NULL`.
- `Draw_TriangleFan` was an empty stub — now implemented.
- Removed the dead `transform` global and the two duplicate `Draw_Triangle`
  prototypes.
- **`glUniform*` on the wrong program** — `MgeGL_Draw` uploaded the matrix
  uniforms before `glUseProgram`, and `MgeGL_SetShader` never called
  `glUseProgram` at all, so after a mid-frame shader switch the matrices landed
  on whichever program was last active and the geometry collapsed to the origin.
- **Shared VAO vs. custom shaders** — attribute locations were looked up per
  shader with `glGetAttribLocation`; a custom shader that ordered its inputs
  differently broke the one shared VAO. Locations are now fixed
  (`layout(location = N)`, `AttribLocations` enum): 0 pos, 1 color, 2 texcoord,
  3 normal.

## Notes / limitations

- Every translation unit compiles clean under `gcc -std=c11 -Wall -Wextra`, the
  `test/` suite passes (106 checks — math, file I/O, objects/gizmo/material), and `build/MGEngine` links once
  `3rdparty/glfw/lib/libglfw3.a` exists (`make 3rdparty`). Running the window /
  renderer needs a real GL context and was not exercised here.
- Dear ImGui was already fully commented out; its includes and the `-limgui` link
  flag were removed. `3rdparty/imgui/` is left in place but unused.
- `3rdparty/glm/` was deleted.
