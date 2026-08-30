# MGEngine

A small raylib-style 2D/3D rendering engine in C11, split into a **shared
library** and a **builder app**:

```
source/    the engine  -> build/libmgengine.(dll|so)
builder/   a scene editor that links the library (Dear ImGui sidebar + inspector)
examples/  one focused demo per feature
test/      unit tests (stubbed GL backend -- no window needed)
vendor/    glad, stb, mlib, GLFW, Assimp, Dear ImGui
```

## Build

```sh
make vendor      # once: builds GLFW + Assimp from vendored source (needs cmake + ninja)
make             # debug build -> build/libmgengine.(dll|so) + build/mgengine (the builder)
make release     # production build: -O2 -DNDEBUG, stripped (use this for real runs)
make test        # unit tests
```

On Windows use `mingw32-make`.

## Docs

- **[USAGE.md](USAGE.md)** — the engine: rendering, lighting, meshes, model
  loading, depth/stencil, the `Mge_Gui*` UI abstraction, and how to link the
  library from your own app.
- **[builder/USAGE.md](builder/USAGE.md)** — the builder app: controls, the
  scene sidebar, the type-aware inspector.
- **[todo.md](todo.md)** — what's done and what's next.
