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
projects, a shared resource root, and hot-reloadable scene DLLs. Do it in
phases so the app keeps working the whole way.

## Terms / model

- **Project** = a directory with a `project.mgproject` at its root. Holds the global
  config (name, window w/h, target FPS, MSAA default, build output name, debug +
  release compiler flags, startup scene) **and** the list of scenes. The editor
  opens a *project*; everything else lives inside it.
  ```
  project.mgproject   global config + scene list
  res/                one shared resource root for the whole project
  scenes/<name>/      one subdirectory per scene
  build/              generated build output (gitignored)
  ```
- **Scene** = `scenes/<name>/` containing:
  - `scene.mgscene`  -- the editor-authored objects / lights / camera (Phase 2)
  - `*.c`        -- scene logic. **Every** `.c` in the scene dir is compiled into
                    that scene's module; adding a new `.c` needs no registration
                    -- the build globs the directory and the editor watches for
                    new files.
- Resources (textures / models / hdr) live in **one** `<root>/res/`; scene-file
  texture paths are stored `res/<file>` relative to the project root.
- **Building the project** generates one build from `project.mgproject`: compiles each
  scene's globbed `*.c` -> a `<name>` module, links `libmgengine`, and produces
  the runnable app (runtime host + `libmgengine` + scene modules + packed
  `res/`). Debug = loose files + hot-reloadable per-scene DLLs; Release = one bundle.
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
- [x] (done in Phase 6) File API: `Mge_MountPak` + `Mge_LoadFileData` / `Text`
      pak-aware, so `Mge_LoadImage` / `Mge_LoadTexture` follow. `Mge_LoadModel`
      (Assimp opens the file itself) is still loose-file only -- a scene that
      needs a packed model would extract it; noted for later.

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
      transform, active, name, `m0..m3` material slots with `res/<file>` texture
      paths relative to the project root + colours/values/wrap). `#` comments.
- [x] `Scene_Save(.., projectRoot)` / `Scene_Load` in `scene_io.c` -- **data only,
      no GL**. `Scene_LoadMaterialTextures(s, projectRoot)` (in `scene.c`) brings
      textures onto the GPU afterwards. On Save, textures outside `<root>/res/`
      are copied in + paths rewritten. Path helpers in `pathutil.c`. Unit test:
      `test/test_scene_io.c` (round-trip + path helpers, hermetic).
- [x] File menu (New / Open / Save / Save As / Build) in `topbar.c`; actions +
      guard in `sceneops.c`. Engine additions: `Mge_SaveFileDialog`,
      `Mge_SetWindowShouldClose`, `Mge_GuiOpenPopup/BeginPopup/EndPopup/ClosePopup`.
- [x] Save scaffolds `<dir>/<name>.c` (scene-code template) beside `scene.mgscene`.
- [x] Unsaved-changes guard: `Scene.dirty` (set by every mutator / inspector edit
      / gizmo drag / rename); New / Open / window-close pop a Save/Discard/Cancel
      modal. Scene name in the top bar shows a `*` while dirty.
- Superseded by Phase 3: a `scenes/<name>/` convention isn't enforced yet -- Save
  As writes the `.mgscene` wherever the user picks and treats that folder as the
  scene root. Phase 3 puts scenes under a project and makes "New Scene" scaffold
  the full directory.

## Phase 3 -- project model & multi-scene   [DONE]

- [x] `project.mgproject` text format (flat, `.mgscene`-style): a `settings`
      section (window w/h, targetFps, msaa, output, cflagsDebug/Release,
      startupScene) + one `scene "<name>"` line per scene. `editor/project.c`
      (Project struct + Default/Add/Remove/Find + Root/SceneDir/SceneFile path
      helpers) + `editor/project_io.c` (Project_Save / Project_Load, data only).
      Unit test `test/test_project_io.c` (round-trip + helpers, hermetic).
- [x] The editor opens a **project**. Top bar: **Project** menu (New / Open /
      Save Project) + **Scene** dropdown (scene list to switch, New / Add / Save
      Scene). Launch = an in-memory default project with one "untitled" scene, so
      the app still runs immediately; scene-file ops unlock once it's saved.
- [x] **New Scene** -> name-entry modal -> `scenes/<name>/` with `scene.mgscene`
      + a `<name>.c` template, added to `project.mgproject`, switched to.
      **Add Scene** -> pick an existing `scenes/<name>/scene.mgscene`.
      `scene_io.c`: a canonical `scene.mgscene` takes its name from the folder.
- [x] Scene switch via the top-bar dropdown; unsaved-changes guard (now also
      `Project.dirty`) applies. `editor/fileops.c` (was `sceneops.c`) executes
      every Project + Scene action + the two modals. **Ctrl+S** (EDIT mode) =
      Save Scene (prompts for a location when the project is new).
- [x] One shared `<root>/res/` for the whole project (not per-scene). Scene-file
      texture paths are `res/<file>` relative to the project root;
      `Scene_Save(.., projectRoot)` / `Scene_LoadMaterialTextures(s, projectRoot)`.
      `Project_ResDir` helper.
- [x] A scene is a directory -- `.c` files are **not** listed in
      `project.mgproject`; Phase 4's build globs the folder. (The mtime watch for
      hot reload lands with Phase 4.)
- [x] `.gitignore` `**/build/`, `*_live_*.dll`, `*.pak` / `*.pak.*`, plus the
      `test_project_io` binary + tmp dir.
- Deferred to Phase 4: the **New Script** action (it's about code, and Phase 4
      owns the per-scene build + `.c` discovery).

## Phase 4 -- scene as code + hot reload   [DONE]

- [x] `MgeSceneCtx` in `mge.h` (objects / objectCount / maxObjects, lights, camera,
      selected) + `MgeScene_Init/Update/Shutdown` fn typedefs. `source/mge_dylib.c`
      -- `Mge_LoadLibrary` / `Mge_GetSymbol` / `Mge_FreeLibrary` / `Mge_GetDylibError`
      (Windows `LoadLibrary` / POSIX `dlopen`). Unit test `test/test_dylib.c`
      (compiles + loads a tiny .dll). `pathutil`: `Path_List`, `Path_MTime`.
- [x] `editor/scene_build.c`: finds the engine SDK (`$MGE_ENGINE` / search up for
      `source/mge.h` + `build/libmgengine`), globs the scene's `*.c`, runs
      `$CC -shared` with the project's debug/release cflags -> `scenes/<name>/build/
      <name>_debug.dll`, captures the command + all output in a `BuildLog`.
- [x] `editor/scene_runtime.c`: copies the dll to `<name>_live_<n>` (Windows locks
      a loaded one), `Mge_LoadLibrary`, resolves the three symbols; `SourceDigest`
      = sum of `*.c` mtimes + file count for the hot-reload watch.
- [x] `editor/play.c` + top-bar **Play / Stop / Build / Console**: Play snapshots
      the `Scene`, builds, loads, `MgeScene_Init`, then `MgeScene_Update(ctx, dt)`
      each frame (gizmo + picking paused). While playing, a `.c` save triggers
      rebuild + `Shutdown`/`Init` reload; a compile error just shows in the
      console. Stop restores the snapshot (Play-mode edits discarded).
- [x] Console panel (`Mge_GuiLogBox`) replaces Resources at the bottom while open;
      auto-scrolls, auto-opens on build.
- [x] **New Script** (Scene menu, from Phase 3): name modal -> extra `.c` in the
      scene folder. `<name>.c` template updated to `MgeSceneCtx*` with a spin demo.
- Deferred: "Build whole project" (every scene at once) is a Phase 6 concern
      (release bundle); Build here compiles the active scene.

## Phase 5 -- resource explorer (bottom panel)   [DONE]

- [x] `editor/resources.c`: recursive tree of the project `<root>/res/`
      (`Mge_GuiTreeNode` folders + file rows). Image thumbnails (`Mge_GuiImage`),
      cached, flushed on project change / shutdown.
- [x] Ops: **Import** (file dialog -> `Path_CopyFile` into the selected folder),
      **New Folder** (name modal), **Rename**, **Delete** (confirm modal;
      recursive for folders). `pathutil`: `Path_IsDir` / `Rename` / `Remove` /
      `List` / `MTime`. Unit-covered in `test_scene_io` (`fs_list_rename_remove`).
- [x] Assign: with a 3D object selected, an image row shows **D S N H** buttons
      -> `Mge_LoadTextureEx` into that material slot + set `scene.texPath` +
      mark dirty.
- [x] GUI additions: `Mge_GuiTreeNode` / `TreePop`, `Mge_GuiImage`,
      `Mge_GuiIndent` / `Unindent`.
- Assign UI: a selected image + a selected object shows an `assign to:
      diffuse / specular / normal / height` bar above the tree (no per-row
      button clutter).
- Deferred to **Phase 7**: drag-and-drop (move / reparent / assign) and copy --
      our `Mge_Gui*` layer doesn't expose ImGui drag/drop yet.

## Phase 6 -- build project (release bundle)   [DONE]

- [x] `source/mge_pak.c`: `Mge_PakWrite(stem, root, splitBytes)` -- header + TOC
      (path[256], offset, size, crc32) + concatenated blobs, physically split into
      `<stem>.pak.001`, `.002`, ... at `splitBytes`. `Mge_PakOpen` / `Mge_PakRead`
      (crc-checked, spans split files) / `Mge_PakClose`. Skips `.dll` / `.exe` /
      `.c` / `.o` and `build/` / `dist/`. `test/test_pak.c`.
- [x] `Mge_MountPak` / `Mge_UnmountPaks` (mount stack) + `Mge_LoadFileText` /
      `Mge_LoadFileData` fall back to a mounted pak -- so `Mge_LoadImage` /
      `Mge_LoadTexture` and the `.mgscene` / `.mgproject` parsers (refactored to
      read via `Mge_LoadFileText`, line iterator `Path_NextLine`) all work
      unchanged. Loose file always wins.
- [x] `runtime/player.c` -- standalone runner. Reuses the editor data layer
      (`scene.c` / `scene_io.c` / `project*.c` / `editor_camera.c` /
      `scene_runtime.c`), no GUI. Loads the project, mounts `<name>.pak`, opens
      the startup scene from the pak + its `.dll` module, runs
      `MgeScene_Init` / `_Update` with a fly-cam. `make` builds `build/mgeplayer`.
      `Project_SceneDir` / `ResDir` now yield relative paths when the project has
      no path (the player runs with cwd at the project root).
- [x] `editor/release.c` + **Project ▸ Build Release**: compile every scene
      `-O2 -DNDEBUG -s`, `Mge_PakWrite` the project data, stage `<root>/dist/`
      (`<name>.exe` = the player, `libmgengine.dll`, `<scene>.dll`s, the pak,
      loose `project.mgproject`). Output to the build console.
- Verified end to end: Build Release -> run `dist/<name>.exe` -> renders the
      scene (data + textures from the pak) with its module running.

## Phase 6b -- environment, camera objects, async build   [DONE]

- [x] **Skybox is project data.** `Scene.skyDir` (project-root-relative folder of
      6 faces, default `res/skybox`), serialised in the scene's `render` block.
      `Scene_LoadSkybox(s, root)` loads it pak-aware, falling back to the engine's
      bundled `assets/skybox` so the editor viewport is never blank.
      **New Project** seeds `<root>/res/skybox/` from the engine (tries the located
      SDK, then a cwd-staged `assets/skybox`), so a built game ships its own skybox
      in the pak. Environment ▸ **choose folder...** / **use engine default**
      re-import the 6 faces; `Mge_OpenFolderDialog` is new
      (`SHBrowseForFolder` / `zenity --directory`). `Path_CopyFile` now no-ops on a
      src==dst copy (was truncating faces to 0 bytes when re-picking `res/skybox/`).
      The scaffolded scene script only spins `OBJECT_3D`, never the camera.
- [x] **`OBJECT_CAMERA`** -- a transform-only object (position + XYZ-euler look
      direction, fov fixed 60). Draws as a wireframe box + forward arrow in the
      editor (never lit, never in the built game). Add via *hierarchy ▸ + add ▸
      Camera* or `Scene_AddCamera`. `Mge_CameraObjectForward()` in `mge.h`.
- [x] **Environment pseudo-entity** -- a fixed "Environment" row at the top of the
      hierarchy (`SEL_ENV`). Its inspector edits the sun (`lights[0]` direction /
      colour / ambient / diffuse / specular), the skybox (`choose skybox...`
      copies the 6 faces into `res/skybox/`), and the **main camera** combo
      (`Scene.mainCamera` = index of an `OBJECT_CAMERA`, or -1).
- [x] **Player views through the main camera.** `Scene_MainCamera()` -> the built
      game renders from that camera object every frame (a scene module can move it
      to move the view). No fly-cam / cursor grab in the shipped game; the debug
      fly-cam only kicks in when a scene has no main camera.
- [x] **Interactive build runs as a separate process.** `SceneBuild_Start` /
      `_Poll` / `_Clear` (`SceneBuildJob`) spawn the compiler detached, streaming
      its output to a temp file the editor tails each frame -- **Build** / **Play**
      and hot-reload no longer freeze the window. Build Release stays synchronous
      (it ships every scene at once). `test_scene_io` covers the new scene fields.

## Phase 7 -- editor polish   [DONE]

- [x] **Drag and drop** -- `Mge_GuiDragSource` / `Mge_GuiDropTarget` (string
      payload) + `Mge_GuiBeginContextMenu` in `mge_gui`:
  - resources: drag a file/folder row onto a folder -> move (`Path_Rename`); onto
    the `res/` header -> top level. `Path_CopyTree` for recursive copies.
  - drag an image row onto an inspector material thumbnail -> assign that slot
    (the old resources `assign to:` bar is gone).
  - hierarchy: drag a row onto another -> reorder (`Scene_MoveObject`, re-maps
    every stored object index); Shift-drop -> set `Transform.parent`.
- [x] Resources: **Copy / Paste** (buttons, Ctrl+C / Ctrl+V, right-click menu),
      right-click **context menu** (New Folder / Rename / Copy / Paste / Delete),
      **multi-select** (ctrl-click); multi delete + a plural confirm.
- [x] **Undo / redo** (`editor/history.c`) -- whole-`Scene` snapshots, coalesced
      per edit "burst" (`History_Rest` refreshes the baseline when idle,
      `History_Record` at every mutation site). `Ctrl+Z` / `Ctrl+Y` /
      `Ctrl+Shift+Z`. `Scene_RestoreSnapshot` keeps live GPU handles and
      **salvages material textures / the skybox by source path** -- an undo that
      didn't touch materials re-reads nothing (a frame hitch there was also
      eating fast key taps, so redo could be missed). History resets on scene
      switch / new / revert.
- [x] **Duplicate** (`Ctrl+D`, `Scene_DuplicateSelectedObjects`) + **multi-select**
      (Shift-click in the viewport, ctrl-click in the hierarchy) + **group gizmo**
      (translate the whole selection about its centroid; rotate / scale act on
      the primary).
- [x] **Gizmo snapping** -- hold **Ctrl** while dragging to snap to the increments
      in the top-bar **Gizmo** menu (`Mge_SetGizmoSnap`, default 0.5 / 15deg /
      0.25). A gizmo given only a position (light / group) is move-only.
- [x] **Delete confirmation** (viewport `Delete` key + hierarchy `x` -> a
      "Delete N object(s)?" modal) + **Revert Scene** (Scene menu -> reload
      `scene.mgscene`, guarded by the unsaved-changes modal).
- [x] `parent` serialised in `.mgscene`; `test_scene_io` covers it. Reparent is
      grouping only -- transform composition down the chain stays in "Later".

## Later / optional

- [ ] Object **parenting** + hierarchy transforms (`Transform.parent`, tree view
      in the left panel, world = parent-chain composition).
- [ ] **Play mode**: a third top-bar state that runs `MgeScene_Update` + real
      input, snapshotting the scene so Stop restores it.
- [ ] ImGui **docking** branch for freely arranged / resizable panels (currently
      panels are pinned to window edges).
- [ ] Editor preferences file (`~/.mgeeditor` or `editor.ini`): last scene,
      window size, panel widths, camera speed.