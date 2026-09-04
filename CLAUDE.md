# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

MGEngine is a small raylib-style 2D/3D rendering engine in C11 (OpenGL 4.4 core,
GLFW, glad, stb_image, Assimp, Dear ImGui). It builds a shared library plus two
consumer apps. See `USAGE.md` (engine API) and `editor/USAGE.md` (editor) for
in-depth docs; `README.md` maps the repo.

## Commands

Windows: use `mingw32-make` everywhere below. `make vendor` needs `cmake` + `ninja`.

```sh
make vendor            # ONCE (and after a toolchain change): build GLFW + Assimp
                       #   from vendored source into vendor/*/lib(64)/ + include/
make                   # debug build  -> build/{libmgengine.so, editor, mgeplayer}
make release           # production build (-O2 -DNDEBUG, stripped) -> build/release/
                       #   separate object cache; run both to keep a debug + release set
make lib               # just the engine library for the current config
make clean-obj         # drop the current config's object cache
make clean             # remove build/ (keeps vendor/; use `make vendor-clean` for that)
```

Run the editor/player from `build/` (or `build/release/`) — `libmgengine` sits
next to the executable and the ELF rpath is `$ORIGIN`.

### Tests

```sh
make test              # build + run every windowless unit suite (stubbed GL, no context)
make -C test test_math # build + run ONE suite (target name == source file stem)
make -C test model     # test_model — needs `make vendor` (links real Assimp), not in `make test`
make -C test render    # headless render smoke test: screenshots + GL-error/blank-frame check.
                       #   Needs the root `make` first (it links build/obj/*.o), a GPU + a desktop session
```

Unit tests compile the engine `.c` files directly (no library link). The harness
is `vendor/mlib/test.h` (`TEST(name){ CHECK(cond); } … RUN(name); test_summary()`).
`test/glstub/` is a fake `<glad/glad.h>` that records GL calls, so `mge_gl.c` /
`mge_debug.c` are testable without a context.

### Examples

```sh
(cd .. && make) && make -C examples   # one binary per demo; reuses build/obj/*.o
```

## Git

- Do **not** run `git commit` until the user explicitly says "commit". Staging
  changes and describing them is fine; creating the commit is not.
- Commit messages carry **no attribution trailer** — omit `Co-Authored-By:` and
  `Claude-Session:` lines.
- Keep commit messages **brief** — a single concise subject line; add a short
  body only when the change genuinely needs explaining.

## Formatting

`.clang-format` is WebKit-based, `IndentWidth: 4`. `.editorconfig` sets
`indent_style = tab`, `tab_size = 4` — indent with tabs.

## Architecture

### One library, three outputs

`source/*.{c,cpp}` all compile into `libmgengine`. `mge_gui.cpp` is the **only**
C++ translation unit (the Dear ImGui backend behind the C `Mge_Gui*` API), which
is why the library is linked with `g++` and statically bakes in the C/C++
runtimes — consumers stay pure C. `editor/` and `runtime/` are plain-C apps that
`#include <mge.h>` and link `-lmgengine`. `source/platforms/mge_code_desktop.c`
(the GLFW backend) is `#include`d by `mge_core.c`, not compiled on its own.

The engine is raylib-shaped: an immediate-mode **batched GL renderer**
(`MgeGL_*` in `mge_gl.c`), its own math library (`mge_math.c`, replaces glm), and
one `.c` per feature (`mge_light.c`, `mge_pbr.c`, `mge_shadow.c`, `mge_bloom.c`,
`mge_deferred.c`, …). An `Object` is a `Transform` + a set of components
(Shape / Material / Collider / RigidBody, in `mge_component.c`); `mge_body.c` is
the linear rigid-body step + collider overlap/resolution.

### Editor project / scene model

The editor's document is a **project**: a folder with `project.mgproject` (flat
diffable text, data only) + a shared `res/` + `scenes/<name>/` per scene. Each
scene is `scene.mgscene` (editor-authored objects/lights/camera — flat text, no
GL) plus any number of `.c` files that compile together into **one
hot-reloadable shared module** exporting:

```c
void MgeScene_Init(MgeSceneCtx* ctx);
void MgeScene_Update(MgeSceneCtx* ctx, float dt);
void MgeScene_Shutdown(MgeSceneCtx* ctx);
void MgeScene_Draw(MgeSceneCtx* ctx, Camera3D camera);  // optional
```

- `scene_build.c` runs the compiler (`$CC`) as a **detached process** the editor
  polls each frame, so the UI never freezes.
- `scene_runtime.c` loads the built `.dll` via a `_live_<n>` copy (Windows locks
  the original) and watches the scene dir's `.c` mtimes for hot reload.
- `MgeSceneCtx` points at the editor's **live** object/light/camera storage, so a
  rebuild mid-edit keeps state. `ctx->requestedScene` triggers a scene switch (in
  the built player; in editor Play mode it only logs).
- The engine SDK is located via `$MGE_ENGINE`, else by searching upward for a
  dir with `source/mge.h` + a `build/` or `build/release/` engine.

`editor/` and `runtime/player.c` **share the data layer** — `scene.c`,
`scene_io.c`, `project*.c`, `pathutil.c`, `editor_camera.c`, `scene_runtime.c`.
`player.c` is the GUI-less runner that `Build Bundle` ships.

### Build Bundle & paks

`editor/release.c` compiles every scene, packs all project data
(`project.mgproject` + `*.mgscene` + `res/`) into `dist/packs/data.pak.NNN` via
`mge_pak.c`, and stages `dist/` — player + engine DLL at the root, scene modules
as **unnamed** `dist/scenes/scene.<index>.dll`. At runtime `Mge_LoadFileData` /
`Mge_Load*` fall back to a mounted pak, so the same code path reads loose files
in the editor and packed data in the shipped game.

### Build-system notes

- Debug lands in `build/`, release in `build/release/`, each with its **own
  object cache**. `make release` re-invokes make with `RELEASE=1`; the two
  configs coexist without a wipe.
- `-MMD -MP` dep files (`.d` next to each `.o`) make a header edit rebuild every
  dependent object. This matters: a stale object cache silently mixes struct
  layouts / ABIs → memory corruption. When in doubt, `make clean-obj`.
- GLFW and Assimp are built separately from vendored source; `-L` searches both
  `lib/` and `lib64/` (distros on GNUInstallDirs use `lib64/`).
