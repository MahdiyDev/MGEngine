# MGEngine

A small raylib-style 2D/3D rendering engine in C11, split into a **shared
library** and an **editor app**:

```
source/    the engine  -> build/libmgengine.(dll|so)
editor/    a project / scene editor that links the library (Dear ImGui docked panels)
runtime/   the standalone player  -> build/mgeplayer (what "Build Release" ships)
examples/  one focused demo per feature
test/      unit tests (stubbed GL backend -- no window needed)
vendor/    glad, stb, mlib, GLFW, Assimp, Dear ImGui
```

An editor **project** is a folder (`project.mgproject` + `scenes/<name>/` +
`res/`); each scene has editor-authored data (`.mgscene`) plus `.c` code compiled
into a hot-reloadable module (the compile runs as a separate process, so the
editor never freezes). A scene has an **Environment** (sun + skybox) and a
**main camera** object the built game views the world through. **Build Release**
compiles every scene, packs the data into a split `.pak`, and stages a runnable
`dist/` around the player.

## Build

```sh
make vendor      # once: builds GLFW + Assimp from vendored source (needs cmake + ninja)
make             # debug build -> libmgengine + build/editor + build/mgeplayer
make release     # production build: -O2 -DNDEBUG, stripped (use this for real runs)
make test        # unit tests (stubbed GL, no window)
make -C test render   # headless render smoke test: screenshots + GL-error check (needs a GPU)
```

On Windows use `mingw32-make`.

## Docs

- **[USAGE.md](USAGE.md)** — the engine: rendering, lighting, meshes, model
  loading, depth/stencil, the `Mge_Gui*` UI abstraction, and how to link the
  library from your own app.
- **[editor/USAGE.md](editor/USAGE.md)** — the editor: projects & scenes, the
  panels, scene code / Play / hot reload, Build Release + the player.
- **[todo.md](todo.md)** — what's done and what's next.
