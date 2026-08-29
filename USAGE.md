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
  mge_shapes.c          Draw_Line / Draw_Rectangle / Draw_Triangle ...
  mge_texture.c         Mge_LoadImage / Mge_LoadTexture (stb_image)
  mge_utils.h mge_utils.c   Trace_Log, file loading
  platforms/mge_code_desktop.c   GLFW backend (#included by mge_core.c)
  main.c               demo: a grid of spinning coloured cubes
test/                  unit tests for math + file utils (no window/GL needed)
examples/shapes/       draw_line, draw_rectangle, draw_triangle, mixed
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
creates `build/obj/` and copies `shaders/` + `assets/` into `build/` so the
executable can be run from either the repo root or `build/`.

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
`Mge_EndMode3D`, with the low-level `MgeGL_Begin(MGEGL_TRIANGLES)` …
`MgeGL_Vertex3f` … `MgeGL_End` immediate calls inside. See `source/main.c`.

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

## Notes / limitations

- Every translation unit compiles clean under `gcc -std=c11 -Wall -Wextra`, the
  `test/` suite passes (67 checks), and `build/MGEngine` links once
  `3rdparty/glfw/lib/libglfw3.a` exists (`make 3rdparty`). Running the window /
  renderer needs a real GL context and was not exercised here.
- Dear ImGui was already fully commented out; its includes and the `-limgui` link
  flag were removed. `3rdparty/imgui/` is left in place but unused.
- `3rdparty/glm/` was deleted.
