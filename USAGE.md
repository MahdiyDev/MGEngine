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
  mge_light.c          Phong lighting; directional / point / spot lights
  mge_material.c        Material / MaterialMap construction helpers
  mge_texture.c         Mge_LoadImage / Mge_LoadTexture (stb_image)
  mge_utils.h mge_utils.c   Trace_Log, file loading
  platforms/mge_code_desktop.c   GLFW backend (#included by mge_core.c)
  main.c               demo: fly-camera + TAB edit mode + combined lighting
test/                  unit tests for math / file utils / objects / materials / lights (no window/GL needed)
examples/shapes/       draw_line, draw_rectangle, draw_triangle, mixed
examples/objects/      gizmo_2d, gizmo_3d
examples/lighting/     ambient, diffuse, specular, directional, point, spotlight
examples/materials/    textured_cube
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
`Mge_GetFileExtension` and the file loaders; `test_object` covers the gizmo
picking/drag math; `test_material` covers `Material` / `MaterialMap`
construction; `test_light` covers the light constructors and the uniform
wiring in `Mge_BeginLighting3D(Ex)` (with a stubbed GL backend). None open a
window.

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
(spotlight shows a hard vs. a soft cone side by side); `source/main.c` combines
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
- `build/MGEngine` links once `3rdparty/glfw/lib/libglfw3.a` exists (`make 3rdparty`).
- Dear ImGui and `glm` are not used — `3rdparty/imgui/` is left in place but
  unlinked, `3rdparty/glm/` is gone.
