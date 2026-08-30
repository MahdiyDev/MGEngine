# MGEngine builder

A minimal scene editor built on top of the engine library — see
[../USAGE.md](../USAGE.md) for the engine itself. `builder/` is a plain-C
consumer (`#include <mge.h>` + link `-lmgengine`), now split into three units:

| file | contents |
| --- | --- |
| `main.c` | the window, the frame loop, the camera (fly + edit look/move), the **TAB** toggle |
| `scene.c` / `scene.h` | `Scene`: objects, lights, selection, picking, and the render passes (shadow + lit + gizmo + skybox) |
| `sidebar.c` / `sidebar.h` | every `Mge_Gui*` call — the mode label, gizmo switch, entity list and inspector |

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

The sidebar is only clickable in EDIT mode. Engine input is suppressed while a
widget has focus (`Mge_GuiWantsMouse` / `Mge_GuiWantsKeyboard`).

## The sidebar

- **MODE** — `EDIT` or `VIEW`, at the very top.
- **FPS / draws** — refreshed once a second (`Mge_GetFps` / `Mge_GetDrawCalls`).
- **shadows** — checkbox, on by default. The Sun (first light) casts a 2048²
  shadow map each frame; editing its direction moves the shadows.
- **GIZMO** — Move / Rotate / Scale, one active (`Mge_SetGizmoMode`); and
  **SPACE** World / Local (`Mge_SetGizmoSpace`), shown as `SPACE: World|Local`.
  Local aligns the handles to a rotated object's own axes.
- **OBJECTS / LIGHTS** — every entity as a selectable row; picking in the
  viewport updates it and vice-versa. The lamp (point light) is movable; the Sun
  (directional) is not.
- **INSPECTOR** — the current selection's fields, live:

  | selection | fields |
  | --- | --- |
  | **Object** | position, **rotation** (euler °), size, colour; material diffuse colour, specular, shininess |
  | **Light** | kind, enabled, colour, ambient / diffuse / specular; position + attenuation (point/spot); direction (directional/spot) |

## Extending it

Scene data + rendering: `scene.c`. UI: `sidebar.c` (add a row + an `inspect_*`
for a new kind). A new movable kind also needs a `Scene_Sel*` accessor so
`Mge_Gizmo3D` can reach its transform.
