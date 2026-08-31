# MGEngine

A small raylib-style 2D/3D rendering engine in C11, split into a **shared
library** and an **editor app**:

```
source/    the engine  -> build/libmgengine.(dll|so)
editor/    a scene editor that links the library (Dear ImGui docked panels)
examples/  one focused demo per feature
test/      unit tests (stubbed GL backend -- no window needed)
vendor/    glad, stb, mlib, GLFW, Assimp, Dear ImGui
```

## Build

```sh
make vendor      # once: builds GLFW + Assimp from vendored source (needs cmake + ninja)
make             # debug build -> build/libmgengine.(dll|so) + build/editor (the editor)
make release     # production build: -O2 -DNDEBUG, stripped (use this for real runs)
make test        # unit tests (stubbed GL, no window)
make -C test render   # headless render smoke test: screenshots + GL-error check (needs a GPU)
```

On Windows use `mingw32-make`.

## Docs

- **[USAGE.md](USAGE.md)** — the engine: rendering, lighting, meshes, model
  loading, depth/stencil, the `Mge_Gui*` UI abstraction, and how to link the
  library from your own app.
- **[editor/USAGE.md](editor/USAGE.md)** — the editor app: the docked panel
  shell, controls, the hierarchy, the type-aware inspector.
- **[todo.md](todo.md)** — what's done and what's next.
