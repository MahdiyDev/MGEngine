# MGEngine editor

A scene editor built on top of the engine library — see
[../USAGE.md](../USAGE.md) for the engine itself. `editor/` is a plain-C consumer
(`#include <mge.h>` + link `-lmgengine`), split into one unit per concern:

| file | contents |
| --- | --- |
| `main.c` | the window, the frame loop, the docked-panel layout (top / left / right / bottom rectangles), the **TAB** mode toggle, **F12** screenshot, the close-button guard. Owns the `Project` + the active `Scene` |
| `editor_camera.c` / `.h` | `EditorCamera`: the yaw/pitch fly-cam. VIEW mode always flies; EDIT mode flies only while **RIGHT mouse** is held. `EditorCamera_SetPose` jumps it to a loaded scene's camera |
| `project.c` / `project.h` | `Project`: global config (window / build settings) + the scene list. `Project_Default` / `Project_AddScene` / `Project_RemoveScene`, and `Project_Root` / `Project_SceneDir` / `Project_SceneFile` path helpers |
| `project_io.c` / `.h` | `project.mgproject` read / write (`Project_Save` / `Project_Load`) — flat text, **data only** |
| `scene.c` / `scene.h` | `Scene`: name / path / dirty flag, objects, lights, selection, picking, `Scene_AddShape` / `Scene_AddLight` / `Scene_Delete*` / `Scene_New`, `Scene_LoadMaterialTextures`, and the render passes (shadow + lit + gizmo + skybox) |
| `scene_io.c` / `.h` | `.mgscene` read / write (`Scene_Save` / `Scene_Load`) — a flat, diffable text format, **data only** (no GL) |
| `pathutil.c` / `.h` | `Path_Dir` / `Base` / `Join` / `IsAbsolute` / `Equal` / `MakeDirs` / `CopyFile` / `List` / `MTime` — small path + fs helpers |
| `fileops.c` / `.h` | executes the Project + Scene menu actions (New / Open / Save project; New / Add / Save / switch scene; New Script; Quit) with the unsaved-changes confirm modal + the name-entry modal |
| `scene_build.c` / `.h` | finds the engine SDK, globs a scene's `*.c`, runs the compiler into a hot-reloadable `.dll`, captures the output in a `BuildLog` |
| `scene_runtime.c` / `.h` | `SceneRuntime`: loads the built module (via a `_live_<n>` copy), resolves `MgeScene_Init/Update/Shutdown`, tracks the scene dir's `.c` mtimes for hot reload |
| `play.c` / `.h` | Play / Stop / Build: snapshot the scene, compile + load, run `MgeScene_Update` each frame, hot-reload on change, restore on Stop; draws the console panel |
| `topbar.c` / `.h` | the **top** strip: a **Project** menu, a **Scene** dropdown (switch / new / add / save / new script), **Play** / **Build** / **Console**, VIEW/EDIT, gizmo Move/Rot/Scl, World/Local space, a **Render** dropdown (MSAA / shadows / HDR / tone map / bloom) |
| `hierarchy.c` / `.h` | the **left** panel: objects + lights as a flat list. `+ add` menu (Cube / Sphere / Plane / Light), per-row select, **double-click to rename**, active/enabled checkbox, `x` to delete |
| `inspector.c` / `.h` | the **right** panel: a type-aware inspector for the selection (Object: active, primitive, transform, material slots. Light: type, colour, attenuation / direction) |
| `resources.c` / `.h` | the **bottom** panel: the project resource explorer (`<root>/res/`) — a stub until Phase 5 |

```sh
make            # from the repo root -> build/editor(.exe)
```

`build/libmgengine.dll` sits next to the executable, so run it from `build/`.
The editor calls `Mge_SetMSAA(4)` before `Mge_InitWindow` (4x anti-aliasing).

## The docked shell

`main.c` computes four screen rectangles every frame and hands each to a panel
via `Mge_GuiBeginPanel` (a title-bar-less window pinned to an exact rect):

```
+--------------------------------------------------+  top    TOPBAR_H (46)
| Hierarchy |          viewport         | Inspector |  left   LEFT_W  (240)
|           |    (the 3D scene fills     |           |  right  RIGHT_W (320)
|           |     the whole window;      |           |
|           |     panels draw on top)    |           |
+--------------------------------------------------+  bottom BOTTOM_H (120)
| Resources                                        |
+--------------------------------------------------+
```

## Controls

| | |
| --- | --- |
| **TAB** / top-bar button | switch between **VIEW** mode (fly-camera, cursor locked) and **EDIT** mode (cursor free) |
| VIEW / fly-camera | **WASD** move, mouse look |
| EDIT — camera | hold **RIGHT mouse** to look; **WASD** flies while it is held |
| EDIT — select | **left-click** an object or a light; click empty space to deselect |
| EDIT — gizmo | drag a handle to **move / rotate / scale** the selection (switch mode in the top bar). Translate has axis arrows + a centre ball; rotate shows the camera-facing part of each ring; scale has cube tips. The hovered handle highlights white |
| **Ctrl+S** (EDIT mode) | save the active scene (same as Scene ▸ Save Scene; prompts for a project location if the project is new) |
| **Play** / **Stop** | build + run the active scene's code; editing is paused while playing and the scene is restored on Stop |
| **F12** | save `editor_screenshot.png` next to the executable (`Mge_TakeScreenshot`) |

Panels are only clickable in EDIT mode. Engine input is suppressed while a widget
has focus (`Mge_GuiWantsMouse` / `Mge_GuiWantsKeyboard`).

## Projects & scenes

The editor's document is a **project**: a directory with a `project.mgproject`
at its root, holding global config + a list of scenes, one shared `res/` for the
whole project, and a `scenes/<name>/` subdirectory per scene (its `scene.mgscene`
+ `.c` files). On launch you get an in-memory default project with one scene,
"untitled" — save it to put it on disk.

```
myproject/
  project.mgproject
  res/                  textures / models / hdr -- shared by every scene
  scenes/
    level1/
      scene.mgscene     editor-authored objects / lights / camera
      level1.c          scene logic (every .c here compiles into the module)
      build/            generated .dll + _live_ copies (gitignored)
```

**Project** menu:

| item | |
| --- | --- |
| **New Project...** | native save dialog → choose a location + name; creates a folder `<name>/` and writes `project.mgproject` + `res/` + `scenes/untitled/` inside it |
| **Open Project...** | pick a `project.mgproject`; loads it and opens its `startupScene` |
| **Save Project** | write `project.mgproject` + the active scene's `scene.mgscene` |

**Scene** dropdown (labelled `Scene: <active> *`):

| item | |
| --- | --- |
| *(scene list)* | click a name to switch to it (`>` marks the active one) |
| **New Scene...** | name-entry modal → creates `scenes/<name>/` (`scene.mgscene` + `<name>.c`), adds it to the project, switches |
| **Add Scene...** | pick an existing `scenes/<name>/scene.mgscene` *inside this project* to register it (rejected otherwise) |
| **Save Scene** | write just the active scene's `scene.mgscene` |
| **New Script...** | scaffold another `.c` in the active scene's folder (compiles into the same module) |

New Scene / Add Scene / Save Scene / New Script need the project saved first
(they write into its folder). Scene names are folder-safe (`[A-Za-z0-9_-]`) and
can't be `build`, `res`, `scenes`, `obj`, `bin`. The project + scene names each
show a trailing `*` while dirty.
**New / Open Project**, a scene **switch**, and the window close button — when
the project or scene is dirty — first pop a **Save / Discard / Cancel** modal.

### Scene code — Play / Build / hot reload

A scene's `.c` files compile into a **module** (a shared library) exporting:

```c
#include <mge.h>
void MgeScene_Init(MgeSceneCtx* ctx);
void MgeScene_Update(MgeSceneCtx* ctx, float dt);
void MgeScene_Shutdown(MgeSceneCtx* ctx);
```

`New Scene` scaffolds a starter `<name>.c` (a demo that spins every object).
`MgeSceneCtx` points at the editor's live storage — `ctx->objects` /
`*ctx->objectCount` (grow within `ctx->maxObjects`), `ctx->lights`,
`ctx->camera`, `ctx->selected` — so a rebuild never loses state. The module
links `libmgengine`, so it can also call `Draw_*`, `IsKeyDown`, `Mge_Load*`, etc.

| top-bar button | |
| --- | --- |
| **Build** | compile the active scene → `scenes/<name>/build/<name>_debug.dll`; output goes to the **Console** panel |
| **Play** / **Stop** | Build, then load + run: `MgeScene_Init` once, `MgeScene_Update(ctx, dt)` every frame. Editing / the gizmo are paused. **Stop** calls `MgeScene_Shutdown` and restores the scene to its pre-Play state (Play-mode changes are discarded) |
| **Console** | toggle the build-log panel (replaces Resources at the bottom) |

While **Play** is running, saving any `.c` in the scene folder triggers an
automatic **rebuild + reload** (`Shutdown` old, `Init` new) — a compile error
just shows in the console and the old module keeps running.

The compiler is `$CC` (default `gcc`) and must be on `PATH`. The editor finds the
engine SDK via `$MGE_ENGINE`, else by searching upward from the working directory
for a folder with `source/mge.h` + `build/libmgengine`.

A `.mgscene` file is a flat, indentation-cosmetic, line-based text format —
diffable, no JSON dependency. One `object` / `light` block per entity, plus
`camera` and `render` sections (the leading `mgescene 1` line is the format id):

```
mgescene 1
name "my scene"

camera
  position 0 3.5 13
  target 0 0 -1
  up 0 1 0
  fov 60

render
  shadows 1
  hdr 0
  tonemap 2
  exposure 1
  bloom 0
  msaa 1

object "Floor"
  primitive plane          # cube | sphere | plane
  active 1
  position 0 -1.1 0
  rotation 0 0 0            # XYZ euler degrees
  scale 24 0.2 24
  shininess 32
  tiling 1 1
  offset 0 0
  triplanar 0
  triplanarScale 1
  m0.color 90 95 105 255    # m0..m3 = diffuse / specular / normal / height
  m0.value 1
  m0.wrap 0
  m0.texture "res/floor.png"   # relative to the project root; absolute also works

light "Sun"
  type directional         # directional | point | spot
  enabled 1
  direction -0.5 -1 -0.4
  color 0.7 0.7 0.8
  ambient 0.22
  ...
```

Texture references are stored as `res/<file>`, **relative to the project root** —
on save, any texture whose source lies outside the project `res/` is copied in
and the path rewritten. `Scene_Load` fills the data only;
`Scene_LoadMaterialTextures(s, projectRoot)` then brings the textures onto the
GPU. `#` starts a comment.

`project.mgproject` is the same style — a `settings` section (window size,
target FPS, MSAA, output name, debug / release cflags, `startupScene`) then one
`scene "<name>"` line per scene. `.c` files are **not** listed; the build
(Phase 4) globs each scene directory, so a new `.c` needs no registration.

## The Hierarchy (left panel)

Every object and light as a row. The leading checkbox is the object's `active`
flag (inactive = not drawn / not outlined) or the light's `enabled` flag.
Single-click a row to select it (drives the inspector + gizmo); **double-click**
the name to rename it in place (`ok` commits). `x` deletes — except the
directional **Sun** (light 0), which the shadow pass depends on.

`+ add` opens a menu: **Cube / Sphere / Plane** (`Scene_AddShape`) or **Light**
(`Scene_AddLight`), spawned at the origin, named, and selected. The scene holds
up to `SCENE_MAX_OBJECTS` (8) and `SCENE_MAX_LIGHTS` (4).

## The Inspector (right panel)

The current selection's fields, live:

| selection | fields |
| --- | --- |
| **Object** | **active** toggle, **primitive** dropdown (cube / sphere / plane), the `Transform` — position, **rotation** (euler °), size (= `transform.scale`); `shininess`; **tiling** / **offset** (`uv' = uv*tiling + offset`); a **triplanar** toggle (+ scale); then one **group per material map** |
| **Light** | kind, enabled, colour, ambient / diffuse / specular; position + attenuation (point/spot); direction (directional/spot) |

Each **material-map group** is a square thumbnail (`Mge_GuiImageButton`) plus that
slot's `color`, `value` and `wrap`:

| group | thumbnail | color | value |
| --- | --- | --- | --- |
| diffuse | albedo texture (loaded sRGB) | tint over the texture — *this is the object's colour* | `gain` 0–2 |
| specular | *(unused)* | tints the highlight | `strength` 0–1 |
| normal | tangent-space normal map (loaded linear) | *(hidden)* | `strength` 0–4 (0 = flat) |
| height (parallax) | grayscale depth map (loaded linear) | *(hidden)* | `scale` 0–0.2 (0 = off); pair with a normal map |

Click a thumbnail to open the OS file picker (`Mge_OpenImageDialog`); an `x`
beside a filled slot clears it. There is no separate object colour.

**wrap** — `Repeat` / `Clamp` / `Mirror` / `Mirror Once` (`Mge_SetTextureWrap`),
how the texture samples past the UV edges.

## Extending it

Scene data + rendering: `scene.c`; project structure: `project.c`; the file/menu
actions: `fileops.c`; Play / Build: `play.c` (compile in `scene_build.c`, load +
hot-reload in `scene_runtime.c`). A panel: its own `*.c` (they Begin/End their
own `Mge_GuiBeginPanel`). A new inspectable kind needs an `inspect_*` in
`inspector.c` and a hierarchy row; a new movable kind also needs a `Scene_Sel*`
accessor so `Mge_Gizmo3D` can reach its transform. New serialised fields go in
`scene_io.c` (writer + parser) or `project_io.c`, with a matching `test_*_io.c`
check. New `MgeSceneCtx` fields go in `mge.h` + `play.c`'s `make_ctx`. Textures
the scene loads are freed in `Scene_Shutdown` / `Scene_DeleteObject`.
