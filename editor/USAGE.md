# MGEngine editor

A scene editor built on top of the engine library — see
[../USAGE.md](../USAGE.md) for the engine itself. `editor/` is a plain-C consumer
(`#include <mge.h>` + link `-lmgengine`), split into one unit per concern:

| file | contents |
| --- | --- |
| `main.c` | the window, the frame loop, the docked-panel layout (top / left / right / bottom rectangles), the **TAB** mode toggle, **F12** screenshot, and the shared `Mge_GuiBeginFrame` / `Mge_GuiEndFrame` pair every panel draws into |
| `editor_camera.c` / `.h` | `EditorCamera`: the yaw/pitch fly-cam. VIEW mode always flies; EDIT mode flies only while **RIGHT mouse** is held |
| `scene.c` / `scene.h` | `Scene`: name, objects, lights, selection, picking, `Scene_AddShape` / `Scene_AddLight` / `Scene_DeleteObject` / `Scene_DeleteLight`, and the render passes (shadow + lit + gizmo + skybox) |
| `topbar.c` / `.h` | the **top** strip: scene name, Open / Save / Build (stubs until Phase 2/3), VIEW/EDIT, gizmo Move/Rot/Scl, World/Local space, a **Render** dropdown (shadows / HDR / tone map / bloom) |
| `hierarchy.c` / `.h` | the **left** panel: objects + lights as a flat list. `+ add` menu (Cube / Sphere / Plane / Light), per-row select, **double-click to rename**, active/enabled checkbox, `x` to delete |
| `inspector.c` / `.h` | the **right** panel: a type-aware inspector for the selection (Object: active, primitive, transform, material slots. Light: type, colour, attenuation / direction) |
| `resources.c` / `.h` | the **bottom** panel: the per-scene resource explorer — a stub until Phase 4 |

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
| **F12** | save `editor_screenshot.png` next to the executable (`Mge_TakeScreenshot`) |

Panels are only clickable in EDIT mode. Engine input is suppressed while a widget
has focus (`Mge_GuiWantsMouse` / `Mge_GuiWantsKeyboard`).

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

Scene data + rendering: `scene.c`. A panel: its own `*.c` (they take a
`Rectangle` + `Scene*` and Begin/End their own `Mge_GuiBeginPanel`). A new
inspectable kind needs an `inspect_*` in `inspector.c` and a hierarchy row; a new
movable kind also needs a `Scene_Sel*` accessor so `Mge_Gizmo3D` can reach its
transform. Textures the scene loads are freed in `Scene_Shutdown` /
`Scene_DeleteObject`.
