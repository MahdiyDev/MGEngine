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
| `scene.c` / `scene.h` | `Scene`: name / path / dirty flag, objects (incl. `OBJECT_CAMERA`), lights, selection (`SEL_OBJECT` / `SEL_LIGHT` / `SEL_ENV`), `skyDir` + `mainCamera`, picking, `Scene_AddShape` / `Scene_AddLight` / `Scene_AddCamera` / `Scene_Delete*` / `Scene_New`, `Scene_LoadMaterialTextures` / `Scene_LoadSkybox` / `Scene_MainCamera`, and the render passes (shadow + lit + skybox + editor markers + gizmo) |
| `scene_io.c` / `.h` | `.mgscene` read / write (`Scene_Save` / `Scene_Load`) — a flat, diffable text format, **data only** (no GL) |
| `pathutil.c` / `.h` | `Path_Dir` / `Base` / `Join` / `IsAbsolute` / `Equal` / `MakeDirs` / `CopyFile` / `List` / `MTime` — small path + fs helpers |
| `fileops.c` / `.h` | executes the Project + Scene menu actions (New / Open / Save project; New / Add / Save / switch scene; New Script; Quit) with the unsaved-changes confirm modal + the name-entry modal |
| `scene_build.c` / `.h` | finds the engine SDK, globs a scene's `*.c`, runs the compiler into a hot-reloadable `.dll`, captures the output in a `BuildLog`. `SceneBuild_Compile` blocks; `SceneBuild_Start` / `_Poll` / `_Clear` (`SceneBuildJob`) run the compiler as a detached process the editor polls each frame |
| `scene_runtime.c` / `.h` | `SceneRuntime`: loads the built module (via a `_live_<n>` copy), resolves `MgeScene_Init/Update/Shutdown`, tracks the scene dir's `.c` mtimes for hot reload |
| `history.c` / `.h` | `History`: undo / redo as whole-`Scene` snapshots. `History_Record` at each mutation site (coalesced per edit burst), `History_Rest` refreshes the baseline when idle, `Scene_RestoreSnapshot` puts a snapshot back — reusing already-loaded material textures / the skybox by source path so an undo re-reads no files |
| `play.c` / `.h` | Play / Stop / Build: snapshot the scene, compile + load, run `MgeScene_Update` each frame, hot-reload on change, restore on Stop; draws the console panel |
| `release.c` / `.h` | **Build Release**: compile every scene `-O2 -DNDEBUG -s`, `Mge_PakWrite` the data, stage `<root>/dist/` with a copy of the standalone player |
| `topbar.c` / `.h` | the **top** strip: a **Project** menu, a **Scene** dropdown (switch / new / add / save / new script), **Play** / **Build** / **Console**, VIEW/EDIT, gizmo Move/Rot/Scl, World/Local space, a **Render** dropdown (MSAA / shadows / HDR / tone map / bloom) |
| `hierarchy.c` / `.h` | the **left** panel: a fixed **Environment** row, then objects + lights. `+ add` menu, per-row select (ctrl-click = multi), **double-click to rename**, active toggle, `x` to delete, **drag to reorder** / Shift-drop to parent (children shown indented) |
| `inspector.c` / `.h` | the **right** panel: type-aware inspector — Environment (sun + skybox + main camera), Object (active, primitive, transform, **parent** combo, material slots — drop an image on a thumbnail to assign it), Camera, Light. A multi-selection edits the primary + notes "group move only" |
| `resources.c` / `.h` | the **bottom** panel: the project `res/` tree — Import / New Folder / Rename / Delete / **Copy** / **Paste**, ctrl-click multi-select, right-click context menu, **drag rows** onto folders (or the `res/` header) to move. Image thumbnails; drag one onto an inspector thumbnail to assign it |

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
+--------------------------------------------------+  bottom BOTTOM_H (172)
| Resources                                        |
+--------------------------------------------------+
```

## Controls

| | |
| --- | --- |
| **TAB** / top-bar button | switch between **VIEW** mode (fly-camera, cursor locked) and **EDIT** mode (cursor free) |
| VIEW / fly-camera | **WASD** move, mouse look |
| EDIT — camera | hold **RIGHT mouse** to look; **WASD** flies while it is held |
| EDIT — select | **left-click** an object or a light; **Shift-click** adds to the selection; click empty space to deselect |
| EDIT — gizmo | drag a handle to **move / rotate / scale** (switch mode in the top bar). Hold **Ctrl** to snap to the increments in the top-bar **Gizmo** menu. A multi-selection moves as a group about its centroid |
| **Ctrl+S** | save the active scene (prompts for a location if the project is new) |
| **Ctrl+Z** / **Ctrl+Y** / **Ctrl+Shift+Z** | undo / redo / redo |
| **Ctrl+D** | duplicate the selected object(s) just off the originals |
| **Delete** | remove the selected object(s) — asks first |
| **Play** / **Stop** / **Build** | build + run the active scene's code. The compile runs as a **separate process** — the editor keeps drawing while it works; the result lands in the Console. Editing is paused while playing and the scene is restored on Stop |
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
  res/
    skybox/             6 cubemap faces, seeded from the engine on New Project
```

**Project** menu:

| item | |
| --- | --- |
| **New Project...** | native save dialog → choose a location + name; creates a folder `<name>/` and writes `project.mgproject` + `res/` + `scenes/untitled/` inside it |
| **Open Project...** | pick a `project.mgproject`; loads it and opens its `startupScene` |
| **Save Project** | write `project.mgproject` + the active scene's `scene.mgscene` |
| **Build Release...** | compile every scene with the project's release cflags, pack the data, stage a runnable `<projectRoot>/dist/` — see below |

**Scene** dropdown (labelled `Scene: <active> *`):

| item | |
| --- | --- |
| *(scene list)* | click a name to switch to it (`>` marks the active one) |
| **New Scene...** | name-entry modal → creates `scenes/<name>/` (`scene.mgscene` + `<name>.c`), adds it to the project, switches |
| **Add Scene...** | pick an existing `scenes/<name>/scene.mgscene` *inside this project* to register it (rejected otherwise) |
| **Save Scene** | write just the active scene's `scene.mgscene` |
| **Revert Scene** | reload `scene.mgscene` from disk, discarding unsaved edits (guarded by the unsaved-changes modal) |
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

### Build Release

**Project ▸ Build Release** stages `<projectRoot>/dist/`:

```
dist/
  <project>.exe        a copy of the standalone player (runtime/player.c)
  libmgengine.dll
  <scene>.dll ...      each scene's module, compiled -O2 -DNDEBUG -s
  <project>.pak.001…   project.mgproject + scenes/*.mgscene + res/, one archive
  project.mgproject    also loose (the player reads it to learn the pak name)
```

Run `dist/<project>.exe`: it `chdir`s to its own folder, loads the project,
mounts the pak, opens the `startupScene` (data + textures from the pak), loads
that scene's `.dll`, and runs `MgeScene_Init` + `MgeScene_Update` each frame. The
view comes from the scene's **main camera** object (`Scene.mainCamera`) — a scene
module moves that object to move the camera. There is no fly-camera or cursor
grab in the shipped game; the debug fly-cam only appears when a scene has no main
camera. Debug iteration stays loose-file; the pak is a release concern.

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
  skybox "res/skybox"      # project-root-relative folder of 6 faces
  mainCamera 4             # index of the OBJECT_CAMERA that runs the game, or -1

object "GameCam"
  kind camera               # transform-only view camera (omit for a normal 3d object)
  position 0 3.5 13
  rotation -8 -90 0         # pitch, yaw, roll -> look direction

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

## Resources (bottom panel)

A tree of the project's shared `<root>/res/`. Click a folder's arrow to expand
it; click a name to select it (the target for the next operation); **ctrl-click**
to build a multi-selection.

| button | |
| --- | --- |
| **Import** | file dialog → copies the file into the selected folder (or `res/`) |
| **New Folder** | name modal → `mkdir` under the selected folder |
| **Rename** | rename the selection |
| **Copy** / **Paste** | copy the selection (Ctrl+C); paste into the target folder (Ctrl+V), suffixing " copy" on a name clash. Recursive for folders |
| **Delete** | remove the selection — asks first; folders go recursively |

Right-click a row for the same actions as a context menu.

**Drag** a row onto a folder (or onto the **`res/`** header for the top level) to
**move** it (`Path_Rename`). Drag an image row onto a material thumbnail in the
Inspector to assign that slot — the path is stored `res/…`-relative and the scene
is marked dirty.

Image files (`png` / `jpg` / `bmp` / `tga` / …) show a small thumbnail, cached
until the panel's project changes. The panel swaps with the build **Console**
while that's open.

## The Hierarchy (left panel)

Every object and light as a row. The leading checkbox is the object's `active`
flag (inactive = not drawn / not outlined) or the light's `enabled` flag.
Single-click a row to select it, **ctrl-click** to multi-select; **double-click**
the name to rename it in place (`ok` commits). `x` deletes (asks first) — except the
directional **Sun** (light 0), which the shadow pass depends on.

`+ add` opens a menu: **Cube / Sphere / Plane** (`Scene_AddShape`), **Light**
(`Scene_AddLight`), or **Camera** (`Scene_AddCamera`) — spawned, named, and
selected. Objects (shapes *and* cameras) share `SCENE_MAX_OBJECTS` (8); lights
`SCENE_MAX_LIGHTS` (4).

**Ctrl-click** a row to build a multi-selection. **Drag** a row onto another to
**reorder** it there (`Scene_MoveObject` fixes up every stored object index);
**Shift-drop** to make it a **child** of the target (`Transform.parent`) — child
rows show indented. Parenting is grouping only for now; child transforms are not
yet composed down the chain.

The fixed **Environment** row at the top can't be deleted. Selecting it shows the
scene's sun (`lights[0]`), the **skybox**, and the **main camera** — which
`OBJECT_CAMERA` the built game views the scene through. A camera object draws as a
wireframe box + forward arrow; its gizmo is hidden while it sits exactly where the
editor fly-cam is.

Skybox buttons: **choose folder...** (pick a folder holding
`right/left/top/bottom/front/back.jpg`), **use engine default** (copy the engine's
bundled skybox), **reload**. Either import copies the 6 faces into
`<root>/res/skybox/` so they go in the pak; a status line reports how many landed.
New Project seeds `res/skybox/` from the engine automatically.

## The Inspector (right panel)

The current selection's fields, live:

| selection | fields |
| --- | --- |
| **Environment** | sun (`lights[0]`) direction / colour / ambient / diffuse / specular; skybox (`choose folder...` / `use engine default` / `reload`); **main camera** combo |
| **Object** | **active** toggle, **primitive** dropdown, the `Transform` — position, **rotation** (euler °), size (= `transform.scale`), a **parent** combo; `shininess`; **tiling** / **offset**; a **triplanar** toggle (+ scale); then one **group per material map** (drop an image from Resources on the thumbnail to assign it) |
| **Camera** | active, position, rotation (pitch / yaw / roll); **main camera** toggle. fov is fixed at 60°; the editor always uses its own fly-cam |
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
