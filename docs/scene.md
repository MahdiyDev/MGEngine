# Scene: objects, components, physics & modules

## Objects & the manipulation gizmo

An `Object` is a movable rectangle (`OBJECT_2D`), a 3D object (`OBJECT_3D`), or a
camera marker (`OBJECT_CAMERA` — a transform only, drawn as a wireframe box + a
representative view frustum, never lit; `Mge_CameraObjectForward(rotation)`
applies the orientation to local −Z). Its placement lives in `obj.transform` — a
`Transform { Vector3 position, Quaternion rotation, Vector3 scale; int parent; }`
where `rotation` is `{0,0,0,1}` (identity) on a fresh object — the constructors
set it, and a zero-initialised `{0,0,0,0}` is also treated as identity when
drawn. `scale` is the full extents (a cube of scale `{2,2,2}` is 2 units across;
a sphere's diameter is `scale.x`); `parent` is reserved for hierarchy and is
`-1` on a fresh object. Beyond the transform, an object carries an `id`, an
`active` flag (false → not drawn / not outlined), a `selected` flag, and a fixed
array of **components** (see **Components** below) — everything else (shape,
material, collider, rigid body) is an optional component.

`Mge_MakeObject3D(pos, size, color)` makes a cube;
`Mge_MakeShape3D(primitive, pos, size, color)` makes any `PrimitiveKind`
(`PRIM_CUBE` / `PRIM_SPHERE` / `PRIM_PLANE` / `PRIM_ARROW` / `PRIM_POLYGON`) —
both attach a **Shape** + **Material** component; `Mge_MakeObject2D` attaches
only a Material. There is **no `Object.color`** — the base colour is the diffuse
map's tint (`Mge_GetMaterialComponent(&obj)->maps[MATERIAL_MAP_DIFFUSE].color`),
which the constructors set from their `Color` argument.
`Mge_DrawObject` renders the Shape rotated (cube corners + normals are rotated on
the CPU — there is no per-object model matrix), lit with the Material component
(or a default when absent), with a stencil outline when `selected`, and draws
nothing when `!active` or when there is no Shape.
`Mge_DrawPrimitive(obj, color)` draws just the geometry (used by the shadow pass
and the outline).

## Components

Optional, trivially-copyable data attached to an `Object`, held in a fixed array
indexed by `ComponentType` (`COMPONENT_SHAPE`, `COMPONENT_MATERIAL`,
`COMPONENT_COLLIDER`, `COMPONENT_RIGIDBODY`). Never touch `obj.components[...]`
directly — go through the accessors:

```c
bool        Mge_HasComponent(const Object* o, ComponentType t);
void*       Mge_GetComponent(Object* o, ComponentType t);   // &data, or NULL if absent
void*       Mge_AddComponent(Object* o, ComponentType t);   // adds (seeded default) or returns the existing
void        Mge_RemoveComponent(Object* o, ComponentType t);
const char* Mge_ComponentName(ComponentType t);             // "Shape" / "Material" / ...
Shape*      Mge_GetShapeComponent(Object* o);               // typed wrappers over Mge_GetComponent
Material*   Mge_GetMaterialComponent(Object* o);
Collider*   Mge_GetColliderComponent(Object* o);
RigidBody*  Mge_GetRigidBodyComponent(Object* o);
```

`Shape { PrimitiveKind primitive; Vector3 poly[MGE_MAX_POLY_POINTS]; int polyCount;
bool polyStrip, wireframe; }` — `wireframe` draws the primitive as an outline;
`PRIM_ARROW` runs along local +X for `scale.x`; `PRIM_POLYGON` draws
`poly[0..polyCount)` (local points through the transform) as a segment / triangle
/ fan / `polyStrip`. `Mge_AddComponent` seeds Shape → `PRIM_CUBE`, Material →
`Mge_DefaultMaterial()`, Collider → box auto-fitted to `transform.scale`,
RigidBody → `{ mass 1, restitution 0.3, useGravity true }`.

Because components are plain values inside `Object`, the whole struct still
`memcpy`s — snapshot-based undo and Play/Stop keep working unchanged.

**Picking** (3D): `Mge_PickObject3D(objects, count, camera)` casts a ray through
the cursor on left-click and selects the nearest object whose geometry it hits
(a miss clears the selection); returns the index, or `-1`. It is a thin wrapper
over `Mge_GetMouseRay` + `Mge_RaycastObjects` (see **Physics: raycasting**).
`Mge_SetSelectedObject` / `Mge_ClearSelection` / `Mge_GetSelectedObject` do it
from code. `objects[i].selected` drives the outline.

**Gizmo** (3D): one switchable handle set for a single target, drawn on top of
the scene (depth test off) with the hovered handle over-stroked white.

```c
typedef enum { GIZMO_TRANSLATE, GIZMO_ROTATE, GIZMO_SCALE } GizmoMode;
typedef enum { GIZMO_WORLD, GIZMO_LOCAL } GizmoSpace;
void       Mge_SetGizmoMode(GizmoMode mode);
GizmoMode  Mge_GetGizmoMode(void);
void       Mge_SetGizmoSpace(GizmoSpace space); // WORLD = global axes; LOCAL = the object's own
GizmoSpace Mge_GetGizmoSpace(void);
void Mge_SetGizmoSnap(float move, float rotateDeg, float scale); // <=0 disables a channel
void Mge_GetGizmoSnap(float* move, float* rotateDeg, float* scale);
// rotation / scale may be NULL; if BOTH are NULL the gizmo is move-only whatever
// the mode. Hold Ctrl while dragging to snap. Returns true while a handle is dragged.
bool Mge_Gizmo3D(Vector3* position, Quaternion* rotation, Vector3* scale, Camera3D camera, float size);
```

- **translate** — three axis arrows + a **centre ball**; drag an arrow → move
  along that axis, drag the ball → move on the view plane.
- **rotate** — three concentric rings (X/Y/Z), full circles so they read as one
  gyroscope; the half facing away from the camera is dimmed, not culled. Drag →
  compose a world-space rotation about that axis onto `*rotation` (a `Quaternion`
  — no gimbal drift; counter-clockwise on screen = positive).
- **scale** — axes with cube tips + a centre cube; drag an axis tip → scale that
  `size` component, drag the centre → uniform scale.

**Space** — `GIZMO_WORLD` keeps the axes on global X/Y/Z; `GIZMO_LOCAL` aligns
them to the object's `rotation` quaternion. Scale is always local. Lights and
multi-select pivots (`rotation == NULL && scale == NULL`) are move-only whatever
the mode.

The gizmo is a **fixed on-screen size** (`size` param) regardless of the object.
Its translucent parts use `MgeGL_SetBlend` (straight alpha).

```c
Mge_BeginMode3D(camera);
    for (...) Mge_DrawObject(objs[i]);
    bool busy = false;
    if (sel >= 0)
        busy = Mge_Gizmo3D(&objs[sel].transform.position, &objs[sel].transform.rotation,
                           &objs[sel].transform.scale, camera, 2.0f);
Mge_EndMode3D();
if (!busy) Mge_PickObject3D(objs, n, camera); // don't re-pick mid-drag
```

2D keeps the old translate-only helper: `Mge_ManipulateObjects2D` (pick + drag)
plus `Mge_DrawObjectGizmo2D` for the X/Y arrows.

Supporting pieces: mouse buttons (`IsMouseButtonPressed/Down/Released`,
`GetMouseDelta`), `Mge_GetScreenWidth/Height`, `Draw_CubeEx` / `Draw_CubeWiresEx`
(both take a `Quaternion`), `Quaternion_*` / `Matrix_RotateXYZ` /
`Vector3_RotateAround`, and world→screen
projection (`Mge_GetWorldToScreen[Ex]`, `Mge_GetCameraViewMatrix`,
`Mge_GetCameraProjectionMatrix`). Demos: `examples/objects/gizmo_2d.c` and
`gizmo_3d.c` (1/2/3 switch modes); `editor/` uses it in full.

**Testing a drag without a mouse** — `Mge_SetMouseOverride(pos, leftDown)` feeds a
fake cursor to `GetMousePosition` / `GetMouseDelta` / `IsMouseButton*(LEFT)`.
Each call is one frame, so call it with `leftDown = true` to press, again (moved)
to drag, then `false` to release; `Mge_ClearMouseOverride()` restores the real
mouse. The `make render` harness uses this to script a rotate drag and screenshot
the result, and `test_gizmo` stubs the same functions for headless drag tests.

## Physics: raycasting

`mge_physics.c` casts rays at primitives and scene objects. A `Ray` is an
`origin + direction`; the direction is normalised internally, so an unnormalised
one is fine. Every `Mge_Raycast*` returns a `RayHit` and only ever reports a
**forward** hit (`distance >= 0`):

```c
typedef struct Ray    { Vector3 position, direction; } Ray;
typedef struct RayHit {
    bool    hit;       // did the ray meet the primitive?
    float   distance;  // world units along the ray to `point`
    Vector3 point;     // world-space contact point
    Vector3 normal;    // unit surface normal, flipped to face the ray
    int     index;     // Mge_RaycastObjects: the object; -1 otherwise
} RayHit;

RayHit Mge_RaycastSphere(Ray ray, Vector3 center, float radius);
RayHit Mge_RaycastBox(Ray ray, Vector3 center, Vector3 size, Quaternion rotation); // OBB; identity/zero q -> AABB
RayHit Mge_RaycastAABB(Ray ray, Vector3 min, Vector3 max);
RayHit Mge_RaycastPlane(Ray ray, Vector3 point, Vector3 normal);                    // infinite plane
RayHit Mge_RaycastTriangle(Ray ray, Vector3 v0, Vector3 v1, Vector3 v2);            // Möller–Trumbore
```

`Mge_RaycastBox` takes `size` as full extents and applies `rotation` about
`center`; the identity (or a zero) quaternion takes an axis-aligned fast path.

**Scene objects.** `Mge_RaycastObjects(ray, objects, count)` tests each object as
its Shape component's `primitive` — `PRIM_SPHERE` (radius `scale.x/2`), `PRIM_CUBE` (an OBB from
`transform.scale` + `transform.rotation`), `PRIM_PLANE` (the finite XZ quad,
`scale.x` × `scale.z`, rotated); an `OBJECT_CAMERA` is tested as its marker-body
box. It returns the **nearest** forward hit, with `.index` set to that object (or
`-1`). Inactive objects and `OBJECT_2D` rects are skipped.

**Mouse picking.** `Mge_GetScreenRay(pixel, camera, w, h)` unprojects a pixel to a
world ray (perspective *and* orthographic); `Mge_GetMouseRay(camera)` uses the
live cursor + window size. This is what `Mge_PickObject3D` and the editor's
click-to-select are built on — combine the two directly for custom picking:

```c
if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    RayHit h = Mge_RaycastObjects(Mge_GetMouseRay(camera), objects, n);
    for (int i = 0; i < n; i++) objects[i].selected = (i == h.index);
}
```

**Debug draw** (inside `Mge_BeginMode3D`): `Mge_DrawRay(ray, length, color)` draws
the ray as an arrow; `Mge_DrawRayHit(ray, hit, rayColor, hitColor)` draws it up to
the contact point (a long stub on a miss) and marks the point + surface normal.

Demo: `examples/physics/raycast_pick.c`. Tests: `test/test_physics.c` (hermetic —
the unprojection maths is self-contained, needing only `mge_math` + libm) and the
`raycast` scene in `make render`.

## Physics: colliders & the rigid-body step

`mge_body.c` adds linear rigid-body dynamics on top of the **Collider** and
**RigidBody** components. This pass is *linear only* — position, velocity, mass,
gravity, restitution and positional correction; boxes collide by their
axis-aligned bounds (rotation is not yet fed to the contact solver), and there is
no angular response, friction or sleeping.

```c
typedef enum { COLLIDER_BOX, COLLIDER_SPHERE } ColliderKind;
typedef struct { ColliderKind kind; Vector3 offset; Vector3 size; bool isTrigger; } Collider;
// size: box = full extents; sphere = radius in .x. World centre = transform.position + offset.
typedef struct { Vector3 velocity; float mass; float restitution; bool useGravity; } RigidBody;
// mass 0 (or no RigidBody at all, just a Collider) = static / infinite mass.

void Mge_SetGravity(Vector3 g);   Vector3 Mge_GetGravity(void);   // default {0,-9.81,0}
bool Mge_ObjectsOverlap(const Object* a, const Object* b, Vector3* mtv); // mtv separates a from b
typedef struct { int a, b; Vector3 mtv; bool trigger; } MgeCollisionPair;
int  Mge_CheckCollisions(const Object* objects, int count, MgeCollisionPair* out, int maxOut);
void Mge_StepPhysics(Object* objects, int count, float dt);
void Draw_ColliderWires(Object obj, Color color);   // inside Mge_BeginMode3D
```

`Mge_StepPhysics` integrates each dynamic body (`velocity += gravity*dt` if
`useGravity`, then `position += velocity*dt`), then runs a couple of
detect/resolve passes: non-trigger pairs get inverse-mass-weighted positional
correction plus a restitution impulse. `e = min(restitutionA, restitutionB)`, and
a *static* collider (a Collider with no RigidBody — a floor or wall) counts as
`restitution 1` here, so a ball bounces off level geometry with its own
bounciness. Trigger pairs are reported by `Mge_CheckCollisions` but get no
response. Adding a Collider auto-fits its `size` to `transform.scale`.

The host steps the physics — the editor's Play mode and the built player both
call `Mge_StepPhysics` each frame, after the scene module's `update`. A scene
module should **not** call it itself (that would double-step). In the editor Stop
restores the pre-Play scene, so the simulated motion is discarded. A selected
object's collider is drawn as a green wireframe, and **Render ▸ colliders** shows
every collider at once.

Tests: `test/test_physics.c` (overlap + one-step integration / bounce / trigger),
`test/test_component.c`, and the `physics` scene in `make render`.


## Hot-reloadable scene modules (`mge_dylib.c`)

A *scene module* is a shared library exporting `MgeScene_Init(MgeSceneCtx*)`,
`MgeScene_Update(MgeSceneCtx*, float dt)` and `MgeScene_Shutdown(MgeSceneCtx*)`,
plus an optional `MgeScene_Draw(MgeSceneCtx*, Camera3D camera)`. The host compiles
it, loads it, and calls it each frame with a pointer to its own object / light
storage (`MgeSceneCtx`), so recompiling + reloading keeps state.

```c
void* h = Mge_LoadLibrary("scene_live_3.dll");   // NULL -> Mge_GetDylibError()
MgeSceneUpdateFn up = (MgeSceneUpdateFn)Mge_GetSymbol(h, "MgeScene_Update");
up(&ctx, dt);
Mge_FreeLibrary(h);
```

**`MgeSceneCtx`** carries the host's `objects` / `objectCount` / `maxObjects`,
`lights` / `lightCount` / `maxLights`, the current `camera`, the `selected` index,
and two scene-control fields:

- `const char* sceneName` — the running scene's name (read-only). A module keyed
  on this can behave differently per scene (e.g. one game module, many levels).
- `char requestedScene[64]` — the scene the host should load after the frame.
  Set it with **`Mge_RequestScene(ctx, "name")`** rather than writing the field
  directly. Supported by `runtime/player.c` (the built game — every scene's
  module is pre-built into the bundle, or linked into the exe for a static-game
  build); the editor's Play mode only *logs* the request (it runs one scene at a
  time). Resolution is by name via an `mlib` hashmap (`vendor/mlib/hashmap`) of
  the project's scene list.

**`MgeScene_Draw`**, when exported, is composited straight into the scene's own
lit pass — `Scene_Draw` runs it right after the lit object loop, with
`Mge_BeginMode3D` already active and, when the scene has `hdr 1`, still inside
its HDR render target. **Do not call `Mge_BeginMode3D` / `Mge_EndMode3D`
yourself** (one is already open); you may wrap draws in your own
`Mge_BeginLighting3DEx` / `Mge_EndLighting3D`. Use it for game geometry the
module owns that isn't an editor Object (so the `SCENE_MAX_OBJECTS` cap doesn't
apply) — and because it shares the HDR pass, anything genuinely bright it draws
blooms like the rest of the scene:

```c
void MgeScene_Draw(MgeSceneCtx* ctx, Camera3D camera) {
    Mge_BeginLighting3DEx(ctx->lights, *ctx->lightCount, camera);
    Draw_Cube(pos, size, color);   // ... the module's own board / actors ...
    Mge_EndLighting3D();
}
```

**`MgeScene_DrawGui`**, also optional, runs *after* `MgeScene_Draw` and the
scene composite, in **2D screen space** (pixel coords, top-left origin). The
host has already called `Mge_UiNewFrame` and calls `Mge_UiRender` right after,
so build the game's HUD / menus here with the `Mge_Ui*` widget API
([the widget GUI section](2d-ui.md#widget-gui-mge_uih)) — not the `Mge_Gui*` ImGui shim. Both the built
player and the editor's Play mode call it.

```c
void MgeScene_DrawGui(MgeSceneCtx* ctx) {
    static MgeUiWidget hud = 0;
    if (hud == 0) { hud = Mge_UiContainer(...); /* ... build once ... */ }
    Mge_UiSetText(scoreLabel, buf);   // update from ctx / game state
    Mge_UiSetRoot(hud);
}
```

`Scene_Draw(s, camera, interact, markers, sceneHook, hookUser)` is how the host
wires this in — `sceneHook` is a `void (*)(void* user)` thunk the host calls at
that point (`runtime/player.c`'s `player_draw_hook`, the editor's
`editor_draw_hook` → `Play_Draw`, a no-op unless a module is playing).

Windows locks a loaded DLL, so copy it to a fresh name before loading (the editor
uses `<name>_live_<n>.dll`). The editor (`editor/scene_build.c` +
`scene_runtime.c` + `play.c`) and `runtime/player.c` are the worked examples —
see [../editor/USAGE.md](../editor/USAGE.md).

## `.pak` archives (`mge_pak.c`)

`Mge_PakWrite(stem, rootDir, splitBytes)` packs a directory tree into a single
logical stream (header + TOC + concatenated, CRC-32'd blobs), physically split
into `<stem>.pak.001`, `.002`, … at `splitBytes` (native code and build inputs —
`.dll` / `.exe` / `.c` / `.o` — plus `build/` and `dist/` dirs are excluded).

```c
Mge_MountPak("game/mygame");              // opens mygame.pak.001, reads the TOC
// ... now Mge_LoadFileData / Mge_LoadImage / Mge_LoadTexture / Scene_Load resolve
//     a missing loose file from the most-recently-mounted pak (loose always wins)
Mge_UnmountPaks();
```

`Mge_PakOpen` / `Mge_PakRead` / `Mge_PakClose` are the direct API (read returns a
malloc'd buffer, NUL-terminated past its size, crc-checked). The editor's **Build
Release** writes one and `runtime/player.c` mounts it.

