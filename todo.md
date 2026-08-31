# TODO

- [x] draw line and triangle together.
- [x] draw check limit
- [x] set cursor hidden. (Hide/Show/Enable/DisableCursor, Mge_ToggleCursor, IsCursorHidden; TAB-bound in main.c)
- [x] add gizmo (2D, 3D)
- [x] lighting: ambient + diffuse + specular (Phong), Material on Object, Light as a scene entity
- [x] material maps: MaterialMap (texture + color + value) slots on Material (diffuse / specular)
- [x] light types: directional / point / spot (+ flashlight, soft cone edges), multi-light pass
- [x] Mesh: vertices + indices + textures, own VAO/VBO/EBO (Mge_MakeMesh/Upload/Draw/Unload)
- [x] Model: Mge_LoadModel via Assimp (glTF/OBJ/FBX) -> meshes + directory + bbox; node transforms baked in
- [x] depth testing: depth func/mask, clip planes, polygon offset (z-fighting), depth-buffer preview shader
- [x] stencil testing + object outlining; selected objects outline instead of wireframe
- [x] split engine (source/ -> libmgengine shared lib) from the builder app (builder/main.c links it)
- [x] Mge_Gui* UI abstraction (Dear ImGui backend); builder sidebar + type-aware inspector (Object / Light)
- [x] face culling: enable/disable, cull face + winding order (opt-in)
- [x] framebuffers + post-processing: RenderTexture, Mge_BeginTextureMode, invert/grayscale/sharpen/blur/edge
- [x] cubemaps: skybox, environment mapping (reflect/refract), dynamic env probes
- [x] batching vertex attributes: Mge_MakeMeshFromArrays -> one VBO, block per attribute
- [x] geometry shaders: Mge_BeginExplode3D, Mge_BeginNormals3D (+ MgeGL_CreateShaderProgramGeo)
- [x] instancing: ModelBatch -> Mge_LoadModelBatch / DrawModelBatch / UpdateModelBatch (one instanced draw per mesh)
- [x] anti-aliasing: MSAA on by default (Mge_SetMSAA / Mge_GetMSAA); GLFW_SAMPLES hint + glEnable(GL_MULTISAMPLE)
- [x] production build: `make release` -> -O2 -DNDEBUG, stripped, --gc-sections
- [x] batch rendering: rlgl-style vertex batcher in mge_gl (Begin/Vertex/End -> one buffer, merged draw calls); Mge_GetDrawCalls + auto-flush on texture change
- [x] multiple vertex rendering: triple-buffered batch VBOs (MGEGL_BATCH_BUFFERS), cycled per flush -> no upload stalls
- [x] advanced lighting: Blinn-Phong specular (default) + Mge_SetLightingModel toggle to classic Phong
- [x] gamma correction: Mge_SetGammaCorrection (GL_FRAMEBUFFER_SRGB, opt-in) + sRGB texture loading (Mge_LoadTextureEx)
- [x] shadow mapping: ShadowMap + Mge_BeginShadowPass / Mge_BeginLighting3DShadowed (depth pass + 3x3 PCF, directional/spot)
- [x] point shadows: PointShadowMap depth cubemap (6-face pass) + Mge_BeginLighting3DPointShadowed (20-tap PCF)
- [x] normal mapping: MATERIAL_MAP_NORMAL / MESH_TEXTURE_NORMAL; TBN from screen-space derivatives (no tangent attribute)
- [x] test_gl: unit-test mge_gl.c against a fake glad (test/glstub) -- matrix stack, batch merging, ring, enum mapping
- [x] GL debug output: Mge_SetDebugOutput -> glDebugMessageCallback (on in debug builds, off with -DNDEBUG)
- [x] `make render`: headless screenshot smoke test (hidden window, renders ~6 features, checks GL error + blank frame, dumps TGAs)
- [x] manipulation gizmo: Object.rotation + Draw_CubeEx; mge_gizmo.c translate/rotate/scale (Mge_SetGizmoMode / Mge_Gizmo3D), drawn on top, hot-handle highlight
- [x] builder split into main.c / scene.c / sidebar.c; sidebar shows EDIT/VIEW mode + gizmo switch; lamp movable by gizmo
- [x] gizmo world/local space (Mge_SetGizmoSpace, shown in sidebar); Mge_SetMouseOverride for headless drag tests
- [x] 3D primitives: Object.primitive (PRIM_CUBE/SPHERE/PLANE), Mge_MakeShape3D, Draw_Plane; removed Object.color (diffuse-map tint is the colour)
- [x] builder Explorer panel (right edge): spawn cube/sphere/plane into the scene (Scene_AddShape)
- [x] material texture slots in the inspector: Mge_GuiImageButton thumbnails -> native file picker (Mge_OpenImageDialog / mge_dialog.c) -> Mge_LoadTextureEx; Mge_UnloadTexture on clear/shutdown
- [x] per-map color + value in the inspector, one group per slot; wired to shader: matDiffuse (gain), matSpecularColor (tint), normalStrength (fixes flat normal maps -- MATERIAL_MAP_NORMAL.value now defaults to 1)
- [x] parallax mapping: MATERIAL_MAP_HEIGHT + parallax-occlusion in the lighting shader (LearnOpenGL Advanced-Lighting/Parallax); builder height-map slot; examples/lighting/parallax_mapping.c
- [x] Makefile: -MMD -MP header deps so editing mge.h rebuilds dependent objects (was: stale cache -> ABI mismatch -> segfault)
- [x] texture wrap modes: TextureWrap (Repeat/Clamp/MirrorRepeat/MirrorClamp), Mge_SetTextureWrap / ...Ex (per-axis U/V); builder per-slot wrap dropdown (Mge_GuiCombo)
- [x] material tiling/offset (uv' = uv*tiling + offset) + triplanar projection (world-XYZ diffuse, no stretch on scale); builder inspector fields; examples/materials/tiling_triplanar.c
- [x] triplanar normal map (whiteout blend) + height map (per-plane parallax-occlusion march, offset-limiting; view frame carries the face sign so it aligns on every face)
- [x] HDR: Mge_LoadRenderTextureHDR (RGBA16F) + Mge_DrawRenderTextureHDR (ToneMap: Reinhard/Exposure/ACES + exposure); builder sidebar HDR toggle; examples/lighting/hdr.c (LearnOpenGL Advanced-Lighting/HDR)
- [x] bloom: mge_bloom.c -- BloomFX (bright pass + ping-pong Gaussian, half res) + Mge_DrawBloom (composite scene+glow, tone-mapped); builder bloom toggle + threshold/intensity; examples/lighting/bloom.c (LearnOpenGL Advanced-Lighting/Bloom)
- [x] deferred shading: mge_deferred.c -- GBuffer (pos/normal/albedo+spec MRT) + Mge_BeginGeometryPass/EndGeometryPass + Mge_DeferredLighting (full-screen, up to 32 lights) + Mge_BlitGBufferDepth; examples/lighting/deferred_shading.c (LearnOpenGL Advanced-Lighting/Deferred-Shading). No shadows/normal maps in the deferred path; builder stays forward.
- [x] SSAO: mge_ssao.c -- Mge_LoadSSAO (hemisphere kernel + 4x4 noise) + Mge_ComputeSSAO (occlusion + box blur, from the deferred G-buffer) + Mge_DeferredLightingAO (ambient *= AO); examples/lighting/ssao.c uses the melon (LearnOpenGL Advanced-Lighting/SSAO)
- [x] PBR + IBL: mge_pbr.c (Cook-Torrance BRDF: GGX/Smith/Schlick, PBRMaterial albedo/normal/metallic/roughness/ao, Mge_BeginPBR3D / Mge_BeginPBR3DIBL / Mge_SetPBRMaterial) + mge_ibl.c (Mge_LoadEnvironment: equirect->cube, 32^2 irradiance, 5-mip prefilter, 512^2 BRDF LUT; Mge_DrawEnvironmentSkybox). Mge_LoadTextureHDR (stbi_loadf -> RGB16F). assets/hdr/newport_loft.hdr + assets/pbr/rusted_iron/; examples/pbr/spheres.c. (LearnOpenGL PBR/Theory + PBR/Lighting + PBR/IBL x2). Forward-only, no shadows in the PBR path.
- [x] builder starts in EDIT mode (was VIEW/fly)

---

# PLAN — Editor overhaul (`builder/` -> `editor/`)

Turn the demo `builder` into a real scene editor: panel UI, multi-scene
projects, a per-scene resource root, and hot-reloadable scene DLLs. Do it in
phases so the app keeps working the whole way.

## Terms / model

- **Project** = a directory with a `project.mgproject` at its root. Holds the global
  config (name, window w/h, target FPS, MSAA default, build output name, debug +
  release compiler flags, startup scene) **and** the list of scenes. The editor
  opens a *project*; everything else lives inside it.
  ```
  project.mgproject   global config + scene list
  scenes/<name>/      one subdirectory per scene
  res/                project-wide shared resources (optional)
  build/              generated build output (gitignored)
  ```
- **Scene** = `scenes/<name>/` containing:
  - `scene.mgscene`  -- the editor-authored objects / lights / camera (Phase 2)
  - `*.c`        -- scene logic. **Every** `.c` in the scene dir is compiled into
                    that scene's module; adding a new `.c` needs no registration
                    -- the build globs the directory and the editor watches for
                    new files.
  - `res/`       -- this scene's resource root (textures, models, hdr, ...);
                    resolves before the project `res/`.
- **Building the project** generates one build from `project.mgproject`: compiles each
  scene's globbed `*.c` -> a `<name>` module, links `libmgengine`, and produces
  the runnable app (runtime host + `libmgengine` + scene modules + packed `res/`).
  Debug = loose files + hot-reloadable per-scene DLLs; Release = one bundle.
- The **editor owns all object/light storage** (the in-editor `Scene` struct).
  Scene code only reads/writes it through a passed context, so rebuilding /
  reloading never loses live edits.
- Scene code contract (C, links `libmgengine` dynamically):
  `MgeScene_Init(MgeSceneCtx*)`, `MgeScene_Update(MgeSceneCtx*, float dt)`,
  `MgeScene_Shutdown(MgeSceneCtx*)` -- defined once across the scene's `.c`
  files. The template `Init` loads `scene.mgscene`; user code may also add objects
  imperatively via `ctx->add_object(...)`.

## Phase 0 -- engine prerequisites (do first, they ripple)

- [x] `Transform { Vector3 position, rotation (euler deg), scale; int parent; }`
      on `Object`; replaced the loose `position` / `size` / `rotation`. Updated
      `mge_object.c`, `mge_stencil.c`, picking, the gizmo call sites, `builder/`
      (`scene.c` + `sidebar.c`), and every example + test. `parent` is `-1` on a
      fresh object (hierarchy reserved for Phase 6).
- [x] `Object.active` (bool, default **true** -- set in `Mge_MakeObject*`).
      `Mge_DrawObject`, `Mge_DrawObjectOutline` and the builder shadow pass skip
      when `!active`. Inspector has an **active** checkbox.
- [x] Inspector writes `obj->primitive` directly (a **primitive** dropdown);
      sphere / plane draw + outline already switch on it. No setter needed.
- [ ] (moved to Phase 6) File API: `Mge_MountPak(path)` + make `Mge_LoadFileData`
      / `Mge_LoadImage` / `Mge_LoadModel` pak-aware -- doesn't ripple through
      examples/tests, so it belongs with the release bundle work.

## Phase 1 -- rename + panel layout   [DONE]

- [x] `builder/` -> `editor/`; `build/mgengine.exe` -> `build/editor.exe`;
      Makefile (`$(APP)`, `EDITOR_SRC`), `README.md`, `USAGE.md`,
      `editor/USAGE.md`, `examples/Makefile` comment. (No `.gitignore` changes
      needed -- `build/` was already ignored; the project `**/build/` + pak rules
      land in Phase 3.)
- [x] Split into: `main.c` (window/loop/layout), `editor_camera.c`, `topbar.c`,
      `hierarchy.c` (left), `inspector.c` (right), `resources.c` (bottom),
      `scene.c` (data). `scene_io.c` = Phase 2; `project_io.c` = Phase 3;
      `scene_build.c` = Phase 4.
- [x] Engine GUI additions: `Mge_GuiBeginPanel` (exact-rect, title-bar-less
      docked panel), `Mge_GuiInputText`, `Mge_GuiSelectableEx` (double-click),
      `Mge_GuiBeginMenu` / `MenuItem` / `EndMenu`, `Mge_GuiSetNextItemWidth`.
- [x] **Top bar** (`Mge_GuiBeginPanel`, 46px): scene-name field, Open / Save /
      Build (stubs), VIEW/EDIT toggle, gizmo Move/Rot/Scl, World/Local dropdown,
      **Render** menu (shadows / HDR / tone map / bloom). Icons still text
      placeholders -- swap when the icon set arrives.
- [x] **Left panel** (Hierarchy): objects + lights, flat list. `+ add` menu
      (Cube / Sphere / Plane / Light). Per-row: select, double-click rename,
      active/enabled checkbox, `x` delete (Sun protected). `Scene_AddLight` /
      `Scene_DeleteObject` / `Scene_DeleteLight` added.
- [x] **Right panel** (Inspector): the old `sidebar.c` inspector, unchanged
      (active toggle, primitive dropdown, transform vec3s, material groups).
- [x] **Bottom panel** (Resources): stub -- scene stats + FPS/draws readout.
- [x] Top-bar Render menu: MSAA on/off (`Mge_SetMSAAEnabled` -- toggles
      `GL_MULTISAMPLE` at runtime; count still fixed at window creation),
      shadows, HDR + tone map, bloom.
- [x] Added `Mge_TakeScreenshot` / `MgeGL_SaveScreenshot` (`mge_screenshot.c`,
      stb_image_write) -- editor F12, render-smoke round-trip, USAGE section.

## Phase 2 -- scene as data   [DONE]

- [x] `scene.mgscene` text format (flat, line-based, diffable, no JSON): `camera` +
      `render` sections, one `object` / `light` block per entity (primitive,
      transform, active, name, `m0..m3` material slots with `res/`-relative
      texture paths + colours/values/wrap). `#` comments.
- [x] `Scene_Save` / `Scene_Load` in `scene_io.c` -- **data only, no GL**.
      `Scene_LoadMaterialTextures` (in `scene.c`) brings textures onto the GPU
      afterwards. On Save, textures outside `res/` are copied in + paths rewritten.
      Path helpers in `pathutil.c`. Unit test: `test/test_scene_io.c` (round-trip
      + path helpers, hermetic).
- [x] File menu (New / Open / Save / Save As / Build) in `topbar.c`; actions +
      guard in `sceneops.c`. Engine additions: `Mge_SaveFileDialog`,
      `Mge_SetWindowShouldClose`, `Mge_GuiOpenPopup/BeginPopup/EndPopup/ClosePopup`.
- [x] Save As scaffolds `<dir>/res/` + a `<name>.c` scene-code template (Phase 4).
- [x] Unsaved-changes guard: `Scene.dirty` (set by every mutator / inspector edit
      / gizmo drag / rename); New / Open / window-close pop a Save/Discard/Cancel
      modal. Scene name in the top bar shows a `*` while dirty.
- Superseded by Phase 3: a `scenes/<name>/` convention isn't enforced yet -- Save
  As writes the `.mgscene` wherever the user picks and treats that folder as the
  scene root. Phase 3 puts scenes under a project and makes "New Scene" scaffold
  the full directory.

## Phase 3 -- project model & multi-scene

- [ ] `project.mgproject` text format (flat, like a `.mgscene`): a `[project]`
      section (name, window w/h, targetFps, msaa, output name, `cflags.debug`,
      `cflags.release`, `startupScene`) + one `scene "<name>"` line per scene
      (path relative to the project root). `editor/project_io.c` --
      `Project_Save` / `Project_Load` (data only), unit test like
      `test_scene_io`.
- [ ] The editor opens a **project**, not a bare `.mgscene`. File menu becomes:
      Project New / Open / Save; Scene New / Open (within the project) / Save /
      Save As. On launch with no project -> a default in-memory project holding
      one untitled scene (so the app still runs immediately).
- [ ] **New Scene** -> create `scenes/<name>/` with a template `<name>.c` (reuse
      the Phase 2 scaffold) + an empty `scene.mgscene` + `res/`, add a
      `scene "<name>"` line to `project.mgproject`, switch to it. **Add Scene** ->
      point at an existing `scenes/<name>/`.
- [ ] A **Scenes** list (a project panel, or a top-bar dropdown): click to switch
      the active scene; the unsaved-changes guard applies on switch.
- [ ] A scene is a *directory* -- its `.c` files are globbed, never enumerated in
      `project.mgproject`. The editor rescans the scene dir (on focus / a watch) and a
      newly-added `.c` just joins the next build. No registration step.
- [ ] **New Script** action (resources panel / a scene menu) scaffolds a
      `<name>.c` in the scene dir from a template.
- [ ] `.gitignore` `**/build/`, `*_live_*.dll`, `*.pak*`.

## Phase 4 -- scene as code + hot reload

- [ ] `MgeSceneCtx` (the callback struct scene code gets): add/remove/find
      object, get selection, spawn primitive, plus `dt`, input passthrough.
- [ ] `editor/scene_build.c`: from `project.mgproject` + the scene's globbed `*.c`,
      generate + run the compile (a generated command or a small Makefile) for
      one scene in **debug** or **release**, using the project's cflags; capture
      stdout/stderr into a build-log console panel.
- [ ] Per-scene DLL: compile the scene's `*.c` -> `<name>.dll` linking
      `libmgengine`. `LoadLibrary` a copy (`<name>_live_<n>.dll`, since Windows
      locks a loaded DLL).
- [ ] Hot reload: watch the scene dir's `*.c` + headers mtime (and new files);
      on change -> rebuild -> on success `FreeLibrary` old, load new, re-run
      `MgeScene_Init` against the editor-owned `Scene` (still holding live edits).
- [ ] "Build" in the File menu builds the whole **project** (every scene) in
      debug; a separate "Build Release" (Phase 6) does the bundle.

## Phase 5 -- resource explorer (bottom panel)

- [ ] File tree of the active scene's `res/` (folders expandable, file icons /
      thumbnails for images); the project `res/` shown alongside.
- [ ] Ops: **add** (import via file dialog -> copy into `res/`), **delete**,
      **rename**, **move** (drag between folders), **copy**, new folder.
- [ ] Drag a resource row onto an inspector texture slot to assign it.
- [ ] Thumbnails for image files (load small, cache; unload on panel close).

## Phase 6 -- build project (release bundle)

- [ ] `.pak` writer: TOC header (name, offset, size, crc) + concatenated blobs;
      split at ~1 GB into `<name>.pak.001`, `.002`, ... A reader that maps a
      logical path across the split files.
- [ ] Editor **Build Release**: for the whole project -- compile every scene
      module `-O2 -DNDEBUG -s`, pack each scene's `res/` (+ the project `res/`)
      into paks, and stage a runnable folder: a slim runtime host +
      `libmgengine.dll` + the scene modules + paks + `project.mgproject`.
- [ ] Debug build stays loose-file (fast iteration); release mounts the pak
      (`Mge_MountPak`, moved here from Phase 0).

## Phase 7 -- editor polish

- [ ] Undo / redo stack (transform edits, add/delete/rename, primitive change).
- [ ] Duplicate object (Ctrl+D), multi-select + group gizmo.
- [ ] Gizmo grid / increment snapping (hold a modifier).
- [ ] Delete confirmation; "revert scene" (reload `scene.mgscene`).

## Later / optional

- [ ] Object **parenting** + hierarchy transforms (`Transform.parent`, tree view
      in the left panel, world = parent-chain composition).
- [ ] **Play mode**: a third top-bar state that runs `MgeScene_Update` + real
      input, snapshotting the scene so Stop restores it.
- [ ] ImGui **docking** branch for freely arranged / resizable panels (currently
      panels are pinned to window edges).
- [ ] Editor preferences file (`~/.mgeeditor` or `editor.ini`): last scene,
      window size, panel widths, camera speed.