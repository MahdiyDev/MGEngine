# MGEngine — engine docs

A small raylib-style 2D/3D rendering engine. The engine builds as a **shared
library** (`libmgengine.dll` / `.so`); `editor/` is a separate app that links
against it through the headers in `source/`. Engine code is **C11**; OpenGL 4.4
core via GLFW + glad, image loading via stb_image, model loading via
[Assimp](https://github.com/assimp/assimp), UI via
[Dear ImGui](https://github.com/ocornut/imgui) behind a C abstraction. The two
C++ dependencies mean the library is linked with `g++` — it bakes in the C/C++
runtimes, so consumers stay pure C.

See [../editor/USAGE.md](../editor/USAGE.md) for the editor, and
[../README.md](../README.md) for the map of the whole repo.

## Contents

| doc | topics |
| --- | --- |
| **[overview.md](overview.md)** | the repo layout, using `mlib`, building (debug / release / `make vendor`), the test + example suites, a minimal-app API sketch, notes / limitations, references |
| **[app.md](app.md)** | GL debug output, cursor modes, a resizable window, frame pacing / v-sync, screenshots |
| **[rendering.md](rendering.md)** | the batched `MgeGL_*` renderer, MSAA, gamma / sRGB, framebuffers & post-processing, geometry shaders, instancing, depth / stencil / face-culling state, the math library |
| **[hdr-bloom.md](hdr-bloom.md)** | HDR render targets & tone mapping, bloom, deferred shading, SSAO |
| **[lighting.md](lighting.md)** | Blinn-Phong lighting, PBR + image-based lighting, shadow mapping (directional / spot / point), normal & parallax mapping, materials & material maps |
| **[meshes.md](meshes.md)** | `Mesh` / `Model` (Assimp import), cube maps / skybox / environment mapping |
| **[scene.md](scene.md)** | `Object` + the manipulation gizmo, the component system, physics (raycasting, colliders, the rigid-body step), hot-reloadable scene modules, `.pak` archives |
| **[2d-ui.md](2d-ui.md)** | text rendering (`mge_text.c` — 2D + world-space), the `Mge_Gui*` ImGui abstraction, the `Mge_Ui*` retained widget GUI |
