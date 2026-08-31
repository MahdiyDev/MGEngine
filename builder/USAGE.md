# MGEngine builder

A minimal scene editor built on top of the engine library — see
[../USAGE.md](../USAGE.md) for the engine itself. `builder/` is a plain-C
consumer (`#include <mge.h>` + link `-lmgengine`), split into four units:

| file | contents |
| --- | --- |
| `main.c` | the window, the frame loop, the camera (fly + edit look/move), the **TAB** toggle, and the shared `Mge_GuiBeginFrame` / `Mge_GuiEndFrame` pair the two panels draw into |
| `scene.c` / `scene.h` | `Scene`: objects, lights, selection, picking, `Scene_AddShape` (spawn), and the render passes (shadow + lit + gizmo + skybox) |
| `sidebar.c` / `sidebar.h` | the **left** panel's `Mge_Gui*` calls — mode label, gizmo switch, entity list, inspector (incl. the material texture slots) |
| `explorer.c` / `explorer.h` | the **right** panel's `Mge_Gui*` calls — the shape palette (spawn cube / sphere / plane) |

```sh
make            # from the repo root -> build/mgengine(.exe)
```

`build/libmgengine.dll` sits next to the executable, so run it from `build/`.
The builder calls `Mge_SetMSAA(4)` before `Mge_InitWindow` (4x anti-aliasing).

## Controls

| | |
| --- | --- |
| **TAB** | switch between **VIEW** mode (fly-camera, cursor locked) and **EDIT** mode (cursor free) |
| VIEW / fly-camera | **WASD** move, mouse look |
| EDIT — camera | hold **RIGHT mouse** to look; **WASD** flies while it is held |
| EDIT — select | **left-click** an object or the lamp; click empty space to deselect |
| EDIT — gizmo | drag a handle to **move / rotate / scale** the selection (switch mode in the sidebar). Translate has axis arrows + a centre ball (view-plane move); rotate shows the camera-facing part of each ring (Unreal-style); scale has cube tips. The hovered handle highlights white; the gizmo is a fixed size and draws on top of everything |

Both panels are only clickable in EDIT mode. Engine input is suppressed while a
widget has focus (`Mge_GuiWantsMouse` / `Mge_GuiWantsKeyboard`).

## The Explorer (right panel)

A palette of primitives. Clicking **Cube**, **Sphere** or **Plane** spawns that
shape at the origin (`Scene_AddShape` → `Mge_MakeShape3D`), names it (`Cube 2`,
`Sphere 1`, …) and selects it, so the gizmo is on it immediately. The scene holds
up to `SCENE_MAX_OBJECTS` (8) — the panel shows the count and greys out when full.

## The Sidebar (left panel)

- **MODE** — `EDIT` or `VIEW`, at the very top.
- **FPS / draws** — refreshed once a second (`Mge_GetFps` / `Mge_GetDrawCalls`).
- **shadows** — checkbox, on by default. The Sun (first light) casts a 2048²
  shadow map each frame; editing its direction moves the shadows.
- **HDR** — checkbox, off by default. Renders the lit pass into an RGBA16F target
  and tone-maps it out; reveals a **tone map** dropdown (Reinhard / Exposure /
  ACES) and an **exposure** slider. Note it also tone-maps the (LDR) skybox.
- **GIZMO** — Move / Rotate / Scale, one active (`Mge_SetGizmoMode`); and
  **SPACE** World / Local (`Mge_SetGizmoSpace`), shown as `SPACE: World|Local`.
  Local aligns the handles to a rotated object's own axes.
- **OBJECTS / LIGHTS** — every entity as a selectable row; picking in the
  viewport updates it and vice-versa. The lamp (point light) is movable; the Sun
  (directional) is not.
- **INSPECTOR** — the current selection's fields, live:

  | selection | fields |
  | --- | --- |
  | **Object** | primitive label, position, **rotation** (euler °), size; `shininess`; then one **group per material map** — diffuse / specular / normal |
  | **Light** | kind, enabled, colour, ambient / diffuse / specular; position + attenuation (point/spot); direction (directional/spot) |

  Above the groups: **shininess**, **tiling** / **offset** (the per-material UV
  transform `uv' = uv*tiling + offset` — raise tiling to repeat a texture without
  stretching it), and a **triplanar** toggle (+ **triplanar scale**) that
  projects the diffuse, normal and height maps from world XYZ so a stretched
  object keeps square texels.

  Each **material-map group** is a square thumbnail (`Mge_GuiImageButton`) plus
  that slot's `color`, `value` and `wrap`:

  | group | thumbnail | color | value |
  | --- | --- | --- | --- |
  | diffuse | albedo texture (loaded sRGB) | tint over the texture — *this is the object's colour* | `gain` 0–2 |
  | specular | *(unused)* | tints the highlight | `strength` 0–1 |
  | normal | tangent-space normal map (loaded linear) | *(hidden — a tint is meaningless)* | `strength` 0–4 (0 = flat) |
  | height (parallax) | grayscale depth map, black = surface / white = groove (loaded linear) | *(hidden)* | `scale` 0–0.2 (0 = off); pair with a normal map |

  Click a thumbnail to open the OS file picker (`Mge_OpenImageDialog`); an `x`
  beside a filled slot clears it (`Mge_UnloadTexture`). There is no separate
  object colour. If a normal map looks flat, raise its `strength`.

  **wrap** — `Repeat` / `Clamp` / `Mirror` / `Mirror Once` (`Mge_SetTextureWrap`),
  how the texture samples past the UV edges. Both axes at once; per-axis is
  API-only (`Mge_SetTextureWrapEx`).

## Extending it

Scene data + rendering: `scene.c`. Left-panel UI: `sidebar.c` (add a row + an
`inspect_*` for a new kind); right-panel palette: `explorer.c`. A new movable
kind also needs a `Scene_Sel*` accessor so `Mge_Gizmo3D` can reach its transform.
Textures the scene loads are freed in `Scene_Shutdown`.
