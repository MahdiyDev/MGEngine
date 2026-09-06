# HDR, bloom & deferred shading

## HDR & tone mapping

A normal `RenderTexture` is `RGBA8` — a value above `1.0` is clamped, so a bright
light flattens to featureless white. `Mge_LoadRenderTextureHDR` gives an
`RGBA16F` colour attachment instead: the lit scene is stored at its true
intensity, then a full-screen **tone-map** pass squeezes that range back to
`[0,1]` for the display, keeping the highlight roll-off.

```c
RenderTexture hdr = Mge_LoadRenderTextureHDR(w, h);
...
Mge_BeginTextureMode(hdr);
    Mge_ClearBackground(...); Mge_BeginMode3D(cam); ...lit scene...; Mge_EndMode3D();
Mge_EndTextureMode();
Mge_DrawRenderTextureHDR(hdr, TONEMAP_ACES, exposure);   // -> the window
```

| `ToneMap` | curve |
| --- | --- |
| `TONEMAP_REINHARD` | `c / (c + 1)` — `exposure` ignored |
| `TONEMAP_EXPOSURE` | `1 - exp(-c · exposure)` — LearnOpenGL's exposure control |
| `TONEMAP_ACES` | Narkowicz filmic ACES approximation, input scaled by `exposure` |

`exposure` shifts which range lands in view, like a camera stop — raise it to pull
detail out of dim areas, lower it to keep bright areas from clipping. The pass
also applies gamma, **unless** `Mge_SetGammaCorrection(true)` is on (then
`GL_FRAMEBUFFER_SRGB` does it) — use one or the other, not both. Tone mapping
darkens an LDR skybox along with everything else; a true HDR pipeline would use an
HDR environment map.

Demo: `examples/lighting/hdr.c` — a corridor with one very bright light; SPACE
toggles tone map vs raw clamp, T cycles the operator, UP/DOWN adjust exposure.
The editor's top-bar **Render** menu has an **HDR** toggle + tone-map + exposure.

## Bloom

The bright parts of an HDR image bleed a soft glow. Given the HDR scene texture,
`Mge_DrawBloom` extracts pixels above a luminance `threshold`, Gaussian-blurs
them (separable, ping-pong, `iterations` H+V rounds at half resolution), then
composites `scene + blur · intensity` **and** tone-maps — it replaces the
`Mge_DrawRenderTextureHDR` step.

```c
BloomFX bloom = Mge_LoadBloom(w, h);      // w,h = the HDR scene's size
bloom.threshold = 1.0f;                   // >1 = only genuinely over-bright pixels
bloom.intensity = 0.6f;
bloom.iterations = 5;
...
Mge_BeginTextureMode(hdr); /* ...lit scene... */ Mge_EndTextureMode();
Mge_DrawBloom(hdr, &bloom, TONEMAP_ACES, exposure);   // -> the window
...
Mge_UnloadBloom(&bloom);
```

The bright pass has a soft knee, so `threshold` below `1.0` lets bright-but-not-
HDR surfaces glow a little too. Same gamma rule as HDR (the composite does it
unless `GL_FRAMEBUFFER_SRGB` is on).

Demo: `examples/lighting/bloom.c` — coloured lamps in a dark room; SPACE toggles
bloom, `[` `]` the threshold, `-` `=` the intensity. The editor's top-bar
**Render** menu has a **bloom** toggle (under HDR) with threshold + intensity sliders.

## Deferred shading

The forward path shades each fragment against every light — cost is
`fragments × lights`, and overdraw multiplies it. **Deferred shading** draws the
scene once into a **G-buffer** (world position, world normal, albedo + specular),
then a single full-screen pass shades every *pixel* against every light — so
`MGE_MAX_LIGHTS_DEFERRED` (32) small point lights stay cheap regardless of how
much geometry piles up.

```c
GBuffer g = Mge_LoadGBuffer(w, h);
...
Mge_BeginMode3D(cam);
    Mge_BeginGeometryPass(&g, cam);
        Mge_DrawObject(obj);  Draw_Cube(...);      // the same draw calls as forward
    Mge_EndGeometryPass();
Mge_EndMode3D();

Mge_DeferredLighting(g, lights, count, cam);        // shades into the bound framebuffer
Mge_BlitGBufferDepth(g);                            // then forward-draw lamp cubes / a skybox
...
Mge_UnloadGBuffer(&g);
```

Wrap `Mge_DeferredLighting` in `Mge_BeginTextureMode(hdrRT)` to feed it into the
HDR / bloom path. The deferred path has **no shadows and no normal / parallax /
triplanar maps** — it's the "lots of little lights" pipeline; keep the forward
path (and the editor) for the rest. `g.position` / `g.normal` / `g.albedoSpec`
are plain `Texture2D`s you can blit for debugging.

Demo: `examples/lighting/deferred_shading.c` — a 5×5 field under 24 drifting
coloured point lights; **G** cycles the final image and the raw G-buffer channels.

## SSAO

Screen-space ambient occlusion. From the deferred G-buffer, for each pixel it
samples a hemisphere of points around the surface (oriented by the normal,
jittered by a 4×4 noise texture), counts how many are buried behind nearby
geometry, blurs the result 4×4, and folds it into the **ambient** term — so
creases and contact points pick up soft shadowing no light computes.

```c
SSAO ao = Mge_LoadSSAO(w, h);
ao.radius = 0.5f;  ao.bias = 0.025f;  ao.power = 2.5f;  ao.kernelSize = 32; // <= 64
...
Mge_BeginMode3D(cam);
    Mge_BeginGeometryPass(&g, cam); ...draw...; Mge_EndGeometryPass();
Mge_EndMode3D();

Mge_ComputeSSAO(&ao, g, cam);                                  // fills ao.aoBlur
Mge_DeferredLightingAO(g, lights, count, cam, ao.aoBlur.texture.id);
...
Mge_UnloadSSAO(&ao);
```

`radius` is in world units — scale it to your scene. `Mge_DeferredLighting`
(no AO arg) still works unchanged. `ao.aoRaw` / `ao.aoBlur` are plain
`RenderTexture`s you can blit to inspect.

Demo: `examples/lighting/ssao.c` — the sliced-melon model; SPACE toggles SSAO,
**B** shows the raw AO buffer, `[` `]` the radius, `-` `=` the power.

