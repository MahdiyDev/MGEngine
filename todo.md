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

Turn the demo `builder` into a real scene editor: panel UI, scene files, a
per-scene resource root, and hot-reloadable scene DLLs. Do it in phases so the
app keeps working the whole way.

## Terms / model

- **Scene** = a directory `scenes/<name>/` containing:
  - `<name>.c`   -- scene logic, compiled to `<name>.dll`
  - `scene.mge`  -- the editor-authored objects/lights/camera, serialised (text)
  - `res/`       -- this scene's resource root (textures, models, hdr, ...)
- The **editor owns all object/light storage** (an in-editor `Scene` struct).
  The scene DLL only reads/writes it through a passed context, so reloading the
  DLL never loses live edits.
- Scene DLL contract (C only, links `libmgengine`, not static):
  `MgeScene_Init(MgeSceneCtx*)`, `MgeScene_Update(MgeSceneCtx*, float dt)`,
  `MgeScene_Shutdown(MgeSceneCtx*)`. The template `Init` loads `scene.mge`; user
  code may also add objects imperatively via `ctx->add_object(...)`.

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
- [ ] (moved to Phase 5) File API: `Mge_MountPak(path)` + make `Mge_LoadFileData`
      / `Mge_LoadImage` / `Mge_LoadModel` pak-aware -- doesn't ripple through
      examples/tests, so it belongs with the release bundle work.

## Phase 1 -- rename + panel layout

- [ ] `builder/` -> `editor/`; `build/mgengine.exe` -> `build/editor.exe`;
      update Makefile (`$(APP)`, `BUILDER_SRC`), `USAGE.md`, `builder/USAGE.md`,
      session/run-skill references, `.gitignore`.
- [ ] Split into: `main.c` (window/loop/camera), `topbar.c`, `hierarchy.c`
      (left), `inspector.c` (right), `resources.c` (bottom), `scene.c` (data),
      `scene_io.c`, `scene_build.c`, `editor_camera.c`.
- [ ] **Top bar** -- narrow strip at the top: View/Edit mode toggle (icon
      buttons -- icons TBD), World/Local space **dropdown**, gizmo Move/Rotate/
      Scale, scene name + Open / Save / Build buttons. (`Mge_GuiBeginBox` docked
      to the top edge, or extend `Mge_GuiBeginSidebar` with a top/bottom edge.)
- [ ] **Left sidebar** -- the scene's object + light list (flat for now). A row
      per entity; click selects (drives the inspector + gizmo). A **"+" add-object
      button** at the top (plus icon TBD) -> menu: Cube / Sphere / Plane / Light.
      Per-row: rename (double-click), an **active** toggle, delete.
- [ ] **Right sidebar** -- the inspector (move the current `sidebar.c` inspector
      here). Object: `transform` (pos/rot/scale vec3s), **primitive-type
      dropdown**, `active` checkbox, material groups (already built). Light: as
      today.
- [ ] **Bottom panel** -- the resource explorer (Phase 4). For now, a stub.
- [ ] Keep MSAA / HDR / bloom / shadows toggles somewhere sensible (a top-bar
      "Render" dropdown, or a collapsible section).

## Phase 2 -- scene as data

- [ ] `scene.mge` text format (line/section based, diffable -- no JSON dep):
      one block per object (`primitive`, `transform`, `active`, `name`, material
      slots with `res/`-relative texture paths + colours/values) and per light;
      plus the editor camera (pos/target/fov).
- [ ] `Scene_Save` / `Scene_Load` in `scene_io.c`. Texture paths are stored
      relative to the scene's `res/`; loading resolves against it.
- [ ] Open-scene flow (file dialog -> pick `scenes/<name>/`), Save, Save As.
- [ ] New-scene scaffold: create `scenes/<name>/` + template `<name>.c` +
      empty `scene.mge` + `res/`.
- [ ] Unsaved-changes guard on scene switch / editor exit.

## Phase 3 -- scene as code + hot reload

- [ ] `MgeSceneCtx` (the callback struct the DLL gets): add/remove/find object,
      get selection, spawn primitive, plus `dt`, input passthrough.
- [ ] `scene_build.c`: run `mingw32-make` for `scenes/<name>/` (a small
      per-scene Makefile or a generated command) in **debug** or **release**;
      capture stdout/stderr into a build-log console panel.
- [ ] DLL load: `LoadLibrary` a copy of `<name>.dll` (Windows won't let you
      overwrite a loaded one -> copy to `<name>_live_<n>.dll`, load that).
- [ ] Hot reload: watch `<name>.c` (and its headers) mtime; on change -> rebuild
      -> on success `FreeLibrary` old, load new, re-run `MgeScene_Init` against
      the editor-owned `Scene` (which still holds the live objects).
- [ ] `.gitignore` `scenes/**/build/`, `*_live_*.dll`, `*.pak*`.

## Phase 4 -- resource explorer (bottom panel)

- [ ] File tree of the active scene's `res/` (folders expandable, file icons /
      thumbnails for images).
- [ ] Ops: **add** (import via file dialog -> copy into `res/`), **delete**,
      **rename**, **move** (drag between folders), **copy**, new folder.
- [ ] Drag a resource row onto an inspector texture slot to assign it.
- [ ] Thumbnails for image files (load small, cache; unload on panel close).

## Phase 5 -- release bundle

- [ ] `.pak` writer: TOC header (name, offset, size, crc) + concatenated blobs;
      split at ~1 GB into `<name>.pak.001`, `.002`, ... A reader that maps a
      logical path across the split files.
- [ ] Editor "Build Release": compile the scene DLL `-O2 -DNDEBUG -s`, pack
      `res/` into `<name>.pak.NNN`, and stage a runnable folder:
      `editor.exe`(or a slim runtime) + `libmgengine.dll` + `<name>.dll` + paks.
- [ ] Debug build stays loose-file (fast iteration); release mounts the pak.

## Phase 6 -- editor polish

- [ ] Undo / redo stack (transform edits, add/delete/rename, primitive change).
- [ ] Duplicate object (Ctrl+D), multi-select + group gizmo.
- [ ] Gizmo grid / increment snapping (hold a modifier).
- [ ] Delete confirmation; "revert scene" (reload `scene.mge`).

## Later / optional

- [ ] Object **parenting** + hierarchy transforms (`Transform.parent`, tree view
      in the left panel, world = parent-chain composition).
- [ ] **Play mode**: a third top-bar state that runs `MgeScene_Update` + real
      input, snapshotting the scene so Stop restores it.
- [ ] ImGui **docking** branch for freely arranged / resizable panels (currently
      panels are pinned to window edges).
- [ ] Editor preferences file (`~/.mgeeditor` or `editor.ini`): last scene,
      window size, panel widths, camera speed.