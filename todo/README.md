# TODO

Split by area. Each file is a running log — `[x]` done, `[ ]` planned.

- **[todo_engine.md](todo_engine.md)** — engine & rendering feature log: the
  batcher, lighting, shadows, normal/parallax maps, HDR, bloom, deferred, SSAO,
  PBR + IBL, text. Chronological, all shipped.
- **[todo_editor.md](todo_editor.md)** — the `builder/` → `editor/` overhaul:
  panel UI, multi-scene projects, scene-as-data + scene-as-code with hot reload,
  the resource explorer, Build Bundle + the player, quaternion rotation, Play
  mode, the component system + colliders + linear rigid-body physics. Phased,
  nearly all `[DONE]`; open items live under **Later / optional** at the end.
- **[todo_gui.md](todo_gui.md)** — planned: a retained, Flutter-shaped widget
  GUI (`Mge_Gui*`) for game UI, separate from the editor's Dear ImGui shim.
  Phases 0–9, nothing started.
