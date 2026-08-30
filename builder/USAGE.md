# MGEngine builder

A minimal scene editor built on top of the engine library — see
[../USAGE.md](../USAGE.md) for the engine itself. `builder/main.c` is a plain-C
consumer: `#include <mge.h>` / `<mge_gui.h>` and link `-lmgengine`.

```sh
make            # from the repo root -> build/mgengine(.exe)
```

`build/libmgengine.dll` sits next to the executable, so run it from `build/`.

The builder calls `Mge_SetMSAA(4)` before `Mge_InitWindow`, so the viewport is
4x anti-aliased — every shape, object and model edge is smoothed. See the
Anti-aliasing section in [../USAGE.md](../USAGE.md).

## Controls

| | |
| --- | --- |
| **TAB** | switch between *fly-camera* (cursor locked) and *edit mode* (cursor free) |
| fly-camera | **WASD** move, mouse look |
| edit mode -- camera | hold **RIGHT mouse** to look; **WASD** flies while it is held |
| edit mode -- objects | **left-click** a cube to select it (its move gizmo appears); drag a **gizmo arrow** to move it; click empty space to deselect |

The sidebar is only clickable in edit mode. Keyboard/mouse input to the engine
is suppressed while a widget has focus (the app checks `Mge_GuiWantsMouse()` /
`Mge_GuiWantsKeyboard()`); the right-mouse rotate only starts when the press
lands outside the sidebar.

## The sidebar

A full-height panel on the left, drawn every frame with the engine's UI
abstraction (`Mge_GuiBeginSidebar`, `Mge_GuiSelectable`, `Mge_GuiInput*` — no
ImGui in the app's code).

- **FPS** and **draws** — frames per second and the previous frame's GL draw-call
  count (the batcher keeps it small) at the top, refreshed once a second.
- **OBJECTS / LIGHTS** — every scene entity as a selectable row. Selecting a
  cube here is exactly like clicking it in the viewport (via
  `Mge_SetSelectedObject`): it gets the outline **and** its move gizmo, so you
  can drag its arrows straight away. A viewport pick updates the sidebar too.
- **INSPECTOR** — the fields of the current selection, editable live. What it
  shows depends on *what* is selected:

  | selection | fields |
  | --- | --- |
  | **Object** (box / rect) | position, size, colour; material diffuse colour, specular, shininess |
  | **Light** | kind, enabled, colour, ambient / diffuse / specular; plus position + attenuation for point/spot, and direction for directional/spot |

Every edit writes straight into the live struct, so the scene updates the same
frame.

## Extending it

Add an entity to the `objects[]` / lights, give it a row in the sidebar loop,
and (for a new type) write an `Inspect<Type>()` that calls `Mge_GuiInput*` on
its fields. Adding a whole new inspectable kind is: a `SEL_*` enum value, a row
loop, and one inspector function.
