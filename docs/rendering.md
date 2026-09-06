# Rendering & GL state

## How the renderer batches

`mge_gl.c` is a retained-nothing, `rlgl`-style batcher. Every `Draw_*` shape and
every `MgeGL_Begin`/`MgeGL_Vertex*`/`MgeGL_End` block appends into **one** CPU
vertex buffer; consecutive primitives of the same kind (`LINES` / `TRIANGLES` /
quads) merge into a single draw-call entry. Nothing reaches the GPU until a
**flush** — `MgeGL_Draw()` — which uploads the whole buffer once and issues one
`glDraw*` per merged entry.

A flush happens on `Mge_EndDrawing`, a shader change (`Mge_BeginLighting3D`,
geometry/post-fx passes), a render-state change (`Mge_BeginMode3D`, depth /
stencil / cull toggles, matrix mode), a retained `Mge_DrawMesh` / `Mge_DrawModel`
/ `Mge_DrawModelBatch`, a texture change (`Mge_SetMaterial`), or the buffer
filling (~5 k vertices). So a frame of same-shader 2D UI + wireframe shapes is
usually **1 upload + 1–2 draw calls** regardless of shape count.

```c
int n = Mge_GetDrawCalls();   // GL draw calls in the previous frame; lower = better batching
```

The dynamic vertex buffers are **triple-buffered** (`MGEGL_BATCH_BUFFERS`, default
3): each flush uploads to and draws from the next set in the ring, so a
`glBufferSubData` never blocks on a buffer the GPU is still reading from an
earlier draw. Raise it if you flush many times per frame; drop it to 1 to save
VRAM.

What is *not* merged: each retained `Mesh` has its own VAO and its own
`glDrawElements`; different models are separate calls (use a `ModelBatch` for
many copies of one). `Draw_*` shapes across a shader/texture/state change land in
different batches. The editor shows the live count next to the FPS.

Demo: `examples/batching/draw_calls.c` — an 800-shape grid that stays at ~2 draw
calls a frame.

## Anti-aliasing (MSAA)

The window is created with a **4x multisampled** default framebuffer, so every
edge the renderer rasterizes — shapes, objects, meshes, models — comes out
smoothed with nothing extra per draw. Change it *before* `Mge_InitWindow`:

```c
Mge_SetMSAA(8);           // 2 / 4 / 8 ... ; 0 (or 1) turns MSAA off
Mge_InitWindow(800, 600, "hello");

int got = Mge_GetMSAA();  // sample count the driver actually granted (0 = none)
```

`Mge_SetMSAA` only records the request; it must be called first because the
sample count is fixed at window creation (`glfwWindowHint(GLFW_SAMPLES, …)`).
`editor/main.c` calls `Mge_SetMSAA(4)` explicitly. This covers the window's
framebuffer only — a `RenderTexture` from `Mge_LoadRenderTexture` is still
single-sampled, so post-processed passes don't get MSAA.

The multisample *resolve* can be flipped at runtime with
`Mge_SetMSAAEnabled(bool)` (state via `Mge_IsMSAAEnabled`) — it toggles
`GL_MULTISAMPLE`, so it can't raise the count past what the window got, but it's
enough for an editor on/off switch. Disabled, `Mge_GetMSAA()` reports `0`. The
editor's top-bar **Render** menu has this toggle (labelled with the granted
count).

Demo: `examples/antialiasing/msaa.c` — orbiting cube + a thin rotating triangle
outline; set `Mge_SetMSAA(0)` to bring the jaggies back.

## Gamma correction

A display darkens whatever it's given by roughly a 2.2 power. Lighting maths are
linear, so their result reaches the eye too dark unless it's sRGB-encoded first.

```c
Mge_SetGammaCorrection(true);   // GL_FRAMEBUFFER_SRGB on the window; call after Mge_InitWindow
bool on = Mge_GetGammaCorrection();
```

This encodes the window's **final** pixels only. Intermediate `RenderTexture`s
stay linear, so `Mge_DrawRenderTextureFX` kernels still run on linear data and
the encode happens once, when the result is blitted out. The ImGui pass is drawn
with the encode off, so the UI is unaffected.

**Off by default** — the engine's shape, vertex and light colours are authored in
sRGB-ish space, not linear, so turning it on shifts their look. Use it when you
work in linear space: load colour maps with `Mge_LoadTextureEx(path, true)` (the
GPU then linearizes them on sample; `Mge_LoadModel` already does this for diffuse
maps and leaves specular linear) and treat `Light.color` as linear.

Demo: `examples/lighting/gamma_correction.c` — a lit scene + a black→white ramp,
toggling correction every 3 s (or SPACE).

## Depth testing

`Mge_BeginMode3D` turns the depth test on (`DEPTH_LESS`) and `Mge_EndMode3D`
turns it off, so 2D drawing is always painter's-order and 3D is depth-sorted.
To tune it *within* a 3D block:

```c
void Mge_EnableDepthTest(void);  void Mge_DisableDepthTest(void);
void Mge_SetDepthFunc(int func);   // a DepthFunc: DEPTH_LESS (default) ... DEPTH_ALWAYS
void Mge_SetDepthMask(bool write); // false -> test against depth but leave it unchanged
```

**Translucency.** `Mge_SetBlend(true)` turns on straight alpha blending
(`src.a` / `1-src.a`) for `Draw_*` calls with `color.a < 255`; turn it back off
when done. Pair it with `Mge_SetDepthMask(false)` for overlapping translucent
draws (glows, particles) so they blend instead of z-fighting. Each toggle flushes
the batch.

For a *bloom* glow instead of a translucent halo, don't fake it with alpha —
draw the object lit, with a local copy of the scene's lights boosted
(`ambient`/`diffuse` pushed up) so it genuinely exceeds 1.0 in the HDR target and
the engine's real bloom pass picks it up (`scene.mgscene` needs `hdr 1` /
`bloom 1`). See the `MgeScene_Draw` section above for the hook this composites
through.

**Visualizing the depth buffer.** Draw between `Mge_BeginDepthPreview()` /
`Mge_EndDepthPreview()` (in place of `Mge_BeginLighting3D`) to shade every
fragment by its linearized depth — near is black, far is white:

```c
Mge_BeginMode3D(camera);
    Mge_BeginDepthPreview();
        Draw_Cube(...); Mge_DrawModel(...);
    Mge_EndDepthPreview();
Mge_EndMode3D();
```

**Preventing z-fighting.** Two surfaces that land on almost the same depth value
flicker between each other. In order of effectiveness:

1. Don't make faces coplanar — offset the decal/marking slightly.
2. Push the **near** plane out. A near plane of `0.01` spends nearly all of the
   depth buffer's precision on the first few centimetres; `Mge_SetClipPlanes`
   lets you widen it to whatever the scene allows (this also affects
   `Mge_GetCameraProjectionMatrix`):

   ```c
   Mge_SetClipPlanes(0.2, 60.0);   // near, far -- rejected if near <= 0 or far <= near
   double n = Mge_GetClipNear();
   ```
3. When geometry *must* be coplanar (decals, outlines), bias its depth with a
   polygon offset — set it, draw, reset:

   ```c
   Mge_SetPolygonOffset(1.0f, 1.0f);   // positive = push away from the camera
   Draw_Cube(...);                     // this surface now loses ties
   Mge_DisablePolygonOffset();
   ```

(The framebuffer uses GLFW's default 24-bit depth buffer.)

Demo: `examples/depth/depth_buffer.c` — auto-flips between lit and depth-preview
views; shows a z-fighting cube pair beside a polygon-offset-fixed one.

## Stencil testing & object outlining

The framebuffer has an 8-bit stencil buffer (cleared with colour/depth by
`Mge_ClearBackground`). Raw controls mirror the depth ones:

```c
void Mge_EnableStencilTest(void);  void Mge_DisableStencilTest(void);
void Mge_SetStencilFunc(int func, int ref, unsigned mask);  // a StencilFunc
void Mge_SetStencilOp(int onStencilFail, int onDepthFail, int onPass); // StencilOp x3
void Mge_SetStencilMask(unsigned mask);   // stencil bits writes may change
void Mge_ClearStencil(void);
```

**Object outlining** is the built-in use. `Mge_DrawObject` draws a bold stencil
outline (a thick orange border, not a wireframe) around any `Object` whose
`.selected` flag is set. For anything else, three calls wrap the technique:

```c
Mge_BeginStencilMask();                        // stamp the silhouette:
    Draw_Cube(pos, size, col);                  //   colour + depth writes are off
Mge_BeginStencilOutside();                      // now draw only outside the stamp:
    Draw_Cube(pos, biggerSize, WHITE);          //   just the border survives
Mge_EndStencil();                              // restore normal drawing
```

`Mge_DrawObjectOutline(obj, thickness, color)` does exactly that for one
`Object` you have already drawn this frame (`thickness` is added to its
extents). The mask pass stamps the full silhouette (depth test off) so the
outline shows even when the object is partly occluded; the border pass draws it
flat/unlit with `glDepthFunc(GL_ALWAYS)` but still writes depth, so a later
"draw last" pass such as a skybox can't paint over it. Override the colour /
thickness with `-DMGE_SELECT_OUTLINE_COLOR` / `_3D` / `_2D`.

Demo: `examples/stencil/object_outline.c` — a walking selection outlines each
cube in turn, plus one hand-outlined pillar in a custom colour.

## Face culling

Off by default -- the engine never enables it, so 2D shapes and lines are
unaffected. Turn it on around 3D geometry to skip triangles pointing away from
the camera:

```c
void Mge_EnableFaceCulling(void);  void Mge_DisableFaceCulling(void);
void Mge_SetCullFace(int face);      // CULL_BACK (default) / CULL_FRONT / CULL_FRONT_AND_BACK
void Mge_SetFrontFace(int winding);  // WINDING_CCW (default) / WINDING_CW
```

`Draw_Cube` and imported meshes wind counter-clockwise, so `CULL_BACK` "just
works". 2D shapes have mixed winding — disable culling before drawing them (or
only enable it inside `Mge_BeginMode3D`). Demo:
`examples/culling/backface_cull.c` cycles off / back / front.


## Framebuffers & post-processing

Render the scene into a `RenderTexture` (an FBO with a colour texture + a
depth/stencil renderbuffer), then draw that texture full-screen through an
effect shader.

```c
RenderTexture Mge_LoadRenderTexture(int width, int height); // usually the window size
void Mge_UnloadRenderTexture(RenderTexture target);
void Mge_BeginTextureMode(RenderTexture target);   // drawing now goes into target
void Mge_EndTextureMode(void);                     // back to the window
void Mge_DrawRenderTextureFX(RenderTexture target, int effect); // a PostFX
```

```c
RenderTexture rt = Mge_LoadRenderTexture(w, h);

Mge_BeginDrawing();
    Mge_BeginTextureMode(rt);
        Mge_ClearBackground(DARKGRAY);
        Mge_BeginMode3D(cam);
            Mge_BeginLighting3DEx(&light, 1, cam); /* ... */ Mge_EndLighting3D();
        Mge_EndMode3D();
    Mge_EndTextureMode();

    Mge_ClearBackground(BLACK);
    Mge_DrawRenderTextureFX(rt, POSTFX_EDGE);
Mge_EndDrawing();
```

`PostFX`: `POSTFX_NONE` (blit), `POSTFX_INVERT`, `POSTFX_GRAYSCALE`,
`POSTFX_SHARPEN`, `POSTFX_BLUR`, `POSTFX_EDGE` — the last three are 3×3 kernels
(`texelSize` = 1 / render-texture size). One shader program handles all of them,
switched by the `effect` uniform. Keep the render texture the same size as the
window so 2D coordinates and 3D aspect line up.

Demo: `examples/framebuffer/post_process.c` cycles through every effect.

For a **floating-point** target (values above `1.0` survive), use
`Mge_LoadRenderTextureHDR` + `Mge_DrawRenderTextureHDR(target, TONEMAP_*, exposure)`
— see [HDR & tone mapping](hdr-bloom.md#hdr--tone-mapping) and [Bloom](hdr-bloom.md#bloom).


## Geometry shaders

Two built-in effects run a geometry stage over the batcher's triangles (works
with `Draw_Cube`, `Mge_DrawMesh`, `Mge_DrawModel`) — call inside `Mge_BeginMode3D`:

```c
Mge_BeginExplode3D(0.5f);             // push each triangle out along its face normal
    Draw_Cube(pos, size, RED);
Mge_EndExplode3D();

Mge_BeginNormals3D(0.2f, YELLOW);     // a short line along every vertex normal
    Draw_Cube(pos, size, WHITE);
Mge_EndNormals3D();
```

The explode magnitude is a plain float — animate it yourself. To roll your own
geometry-shader pass, `MgeGL_LoadShader(src, GL_GEOMETRY_SHADER, name)` +
`MgeGL_CreateShaderProgramGeo(vs, gs, fs)`; the vertex shader gets `modelview`
and the geometry shader `projection` from the batcher.

Demo: `examples/geometry/geometry_shader.c` — a cube showing its normals beside
one that pulses apart and back.

## Instancing

`ModelBatch` draws hundreds of copies of one `Model` with a single
`glDrawElementsInstanced` per mesh — the CPU submits nothing per copy. The
per-instance model matrices are packed into one GPU buffer that is bound onto
the model's mesh VAOs as a `mat4` vertex attribute (locations 4–7).

```c
Model melon = Mge_LoadModel("assets/sliced_musk_melon/scene.gltf");

Matrix xf[200];
for (int i = 0; i < 200; i++)
    xf[i] = Matrix_Multiply(                       // Matrix_Multiply(A, B) = A then B
        Matrix_Multiply(Matrix_Scale(s, s, s),
                        Matrix_Rotate((Vector3){ 0, 1, 0 }, angle)),
        Matrix_Translate(x, y, z));                // -> scale, then rotate, then translate

ModelBatch field = Mge_LoadModelBatch(melon, xf, 200);

// each frame, inside Mge_BeginMode3D:
Mge_DrawModelBatch(field, sun, camera);           // lit by one light (directional or point)

// optional: recompute xf[] and re-upload (count clamped to the original)
Mge_UpdateModelBatch(&field, xf, 200);

Mge_UnloadModelBatch(&field);                      // frees the instance buffer only
Mge_UnloadModel(&melon);                           // the model is not owned by the batch
```

`Mge_DrawModelBatch` uses its own shader (not the `Mge_BeginLighting3D` one), so
call it on its own — not between `Mge_BeginLighting3D` / `Mge_EndLighting3D`. One
live batch per `Model` at a time: the instance attributes are bound onto the
shared mesh VAOs, so a second batch over the same model overwrites the first.

Demo: `examples/instancing/melon_field.c` — 64 spinning melons on a jittered
grid, one draw call per mesh, camera orbiting.


## Math

`glm` is gone. `mge_math.h` provides plain-C functions — no operator overloads:

| | |
| --- | --- |
| `Vector3_Add/Subtract/Scale/Multiply(a, b)` | `Vector3_DotProduct`, `Vector3_Length` |
| `Vector3Cross`, `Vector3Normalize` | `Vector2_Rotate(v, radians)`, `Clamp` |
| `Matrix_Identity/Multiply/Translate/Scale/Rotate` | `Matrix_RotateXYZ` / `Matrix_ToEulerXYZ`, `MatrixOrtho/Perspective/LookAt`, `MatrixToFloatV` |
| `Quaternion_Identity/Normalize/Conjugate/Multiply` | `Quaternion_FromAxisAngle` / `_FromEuler` / `_ToEuler` (XYZ, == the matrix path) |
| `Quaternion_ToMatrix` / `_FromMatrix` | `Quaternion_RotateVector3`, `_Slerp`, `_LookRotation` (local −Z → forward), `_Approx` |

Matrices are stored column-major so `MatrixToFloat(m)` feeds `glUniformMatrix4fv`
directly. `Quaternion_Multiply(a, b)` composes "apply `a`, then `b`" (matches
`Matrix_Multiply`); it's the orientation type in `Transform`.

