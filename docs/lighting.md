# Lighting, shadows & materials

## Lighting

Blinn-Phong shading with the three classic terms:

| term | what it is |
| --- | --- |
| **ambient** | a flat, constant fill added everywhere (nothing is fully black) |
| **diffuse** | Lambert: brightness ∝ `max(dot(surfaceNormal, dirToLight), 0)` |
| **specular** | a highlight where the surface reflects the light toward the camera; `material.shininess` sets its tightness |

The specular term is **Blinn-Phong** (`dot(normal, halfway)`) by default — unlike
classic Phong (`dot(view, reflect)`) it has no hard cutoff at grazing angles, so
low-`shininess` highlights stay smooth. Switch models with
`Mge_SetLightingModel(LIGHTING_PHONG)` / `LIGHTING_BLINN_PHONG` (Phong wants a
`shininess` ~2–4x lower for a similar highlight). The toggle drives
`Mge_BeginLighting3D[Ex]`; the instanced-model shader (`Mge_DrawModelBatch`) is
always Blinn-Phong.

**Where do the knobs live?** — the two halves of the equation split cleanly:

- a **`Light`** is a *scene entity*: its type, `color`, and the strength of each
  term. You can mix up to `MGE_MAX_LIGHTS` (8) in one pass.
- a **`Material`** is the *surface response* and is a field on `Object`
  (`obj.material`): a set of `MaterialMap` slots plus a `shininess`. The
  *surface* parameters live here; the *light* itself is separate.

#### Light types — one constructor each

```c
Light Mge_MakeDirectionalLight(Vector3 direction, Vector3 color); // the sun: parallel rays, no falloff
Light Mge_MakePointLight(Vector3 position, Vector3 color);        // a bulb: radiates + fades with distance
Light Mge_MakeSpotLight(Vector3 position, Vector3 direction, Vector3 color,
                        float innerAngleDeg, float outerAngleDeg); // a cone; inner < outer = soft edge
Light Mge_MakeFlashlight(Camera3D camera, Vector3 color);         // a tight spot at the camera, aimed where it looks
Light Mge_MakeLight(Vector3 position, Vector3 color);             // legacy: point light, no distance falloff
```

- **Directional** ignores position; set `.direction`. Attenuation never applies.
- **Point / spot** fade with distance via `.constant / .linear / .quadratic`
  (`Mge_MakePointLight` presets a ~50-unit reach).
- **Spot** adds a cone: full brightness inside `innerAngleDeg`, fading to nothing
  by `outerAngleDeg`. Equal angles → a hard edge; a gap → a **soft edge**.
  Stored as cosines in `.innerCutoff / .outerCutoff`.
- Every light has `.enabled` (skip it without removing it from the array) and
  per-term `.ambient / .diffuse / .specular` scalars.

#### Drawing with them

```c
Material Mge_DefaultMaterial(void);
void Mge_SetMaterialTexture(Material* m, int mapIndex, Texture2D texture);

void Mge_BeginLighting3D(Light light, Camera3D camera);                       // one light
void Mge_BeginLighting3DEx(const Light* lights, int count, Camera3D camera);  // up to MGE_MAX_LIGHTS
void Mge_SetMaterial(Material material);   // per-surface; no-op unless lighting is active
void Mge_EndLighting3D(void);              // restore the default (unlit) shader
```

Call inside `Mge_BeginMode3D`. `Mge_DrawObject` sets the object's own material
for you, so lit objects just work:

```c
Light sun   = Mge_MakeDirectionalLight((Vector3){ -1, -2, -1 }, (Vector3){ .6f, .6f, .7f });
Light lamp  = Mge_MakePointLight((Vector3){ 4, 6, 4 }, (Vector3){ 1, .9f, .7f });
Light torch = Mge_MakeFlashlight(camera, (Vector3){ 1, 1, 1 });
Light lights[3] = { sun, lamp, torch };

Object box = Mge_MakeObject3D((Vector3){ 0, 0, 0 }, (Vector3){ 1, 1, 1 }, RED);
box.material.shininess = 64.0f;

Mge_BeginMode3D(camera);
    Mge_BeginLighting3DEx(lights, 3, camera);
        Mge_DrawObject(box);                              // lit with box.material
        Mge_SetMaterial((Material){ .maps[MATERIAL_MAP_DIFFUSE].color = GRAY,
                                   .maps[MATERIAL_MAP_DIFFUSE].value = 1.0f,   // gain -- a raw literal must set it
                                   .maps[MATERIAL_MAP_SPECULAR].value = 1.0f, .shininess = 8 });
        Draw_Cube((Vector3){ 0, -1, 0 }, (Vector3){ 24, 0.1f, 24 }, GRAY); // lit floor
    Mge_EndLighting3D();
    if (sel >= 0) Mge_Gizmo3D(&box.transform.position, &box.transform.rotation, &box.transform.scale, camera, 2.0f);
Mge_EndMode3D();
```

For a single light, `Mge_BeginLighting3D(sun, camera)` is the same as
`Mge_BeginLighting3DEx(&sun, 1, camera)`.

Only geometry with per-vertex normals is shaded correctly — `Draw_Cube` emits
them; `MgeGL_Normal3f(x, y, z)` sets the current normal for your own
`MgeGL_Vertex3f` calls. Lines (`Draw_Arrow3D`, `Draw_CubeWires`) have no normals,
so draw them outside the `Begin/EndLighting3D` pair.

Demos: `examples/lighting/` — `ambient` / `diffuse` / `specular` isolate the
three terms; `directional` / `point` / `spotlight` isolate the three light types
(spotlight shows a hard vs. a soft cone side by side); `blinn_phong` toggles the
two specular models over a low-shininess floor; `gamma_correction` toggles sRGB
output; `shadow_mapping` casts a directional shadow; `editor/main.c` combines a
directional fill with an orbiting point light.

## PBR & image-based lighting

A separate lighting path from Blinn-Phong: the **Cook-Torrance** microfacet BRDF
(GGX distribution, Smith geometry, Schlick Fresnel) with a metallic / roughness
`PBRMaterial`, plus an **image-based** ambient term from an environment map.

```c
Environment env = Mge_LoadEnvironment("assets/hdr/newport_loft.hdr"); // precompute, once

PBRMaterial m = Mge_DefaultPBRMaterial();
m.albedo = Mge_LoadTextureEx("albedo.png", true);   // sRGB
m.normal = ...; m.metallic = ...; m.roughness = ...; m.ao = ...; // rest linear
// or leave a map at id 0 and set m.albedoColor / m.metallicValue / m.roughnessValue

Mge_BeginTextureMode(hdrRT);                          // PBR outputs linear HDR
Mge_BeginMode3D(cam);
    Mge_BeginPBR3DIBL(lights, n, cam, env);           // or Mge_BeginPBR3D (direct only)
        Mge_SetPBRMaterial(m);  Draw_Sphere(...);     // or Mge_DrawModel(model)
    Mge_EndPBR3D();
    Mge_DrawEnvironmentSkybox(env, cam);              // the lit background
Mge_EndMode3D();
Mge_EndTextureMode();
Mge_DrawRenderTextureHDR(hdrRT, TONEMAP_ACES, exposure);  // tone-map on the way out
```

`Mge_LoadEnvironment` does the LearnOpenGL IBL precompute at load: equirect →
cubemap, convolve to a 32² **irradiance** cube (diffuse), **prefilter** to a
5-mip cube by roughness (specular), and bake the 512² **BRDF integration LUT**.
It renders several cube passes — needs a live GL context, do it once.

Lights use the same `Light` struct/constructors; PBR reads `color` and
`diffuse` as radiance and the attenuation terms as `1/(c + l·d + q·d²)` (set
`quadratic = 1`, `linear = 0` for physical `1/d²`). Load `albedo` **sRGB**, every
other map **linear**. The PBR shader takes up to `MGE_MAX_LIGHTS` (8) direct
lights; for many lights use the deferred path instead. It's forward-only —
no shadow maps yet.

Demo: `examples/pbr/spheres.c` — a metallic × roughness sphere grid under the
`assets/hdr/` environment, plus the downloaded `assets/pbr/rusted_iron/` texture
set on a few spheres. **SPACE** stops the orbit, **I** toggles IBL.

## Shadow mapping

A directional or spot light casts shadows in two passes over the same geometry:

```c
ShadowMap sm = Mge_LoadShadowMap(2048);          // once; Mge_UnloadShadowMap(&sm) at the end
...
Mge_BeginShadowPass(&sm, sun, sceneCenter, sceneRadius); // pass 1: depth from the light
    DrawOccluders();                                     // world-space geometry only
Mge_EndShadowPass();

Mge_ClearBackground(bg);
Mge_BeginMode3D(camera);
    Mge_BeginLighting3DShadowed(&sun, 1, camera, sm);    // pass 2: lit, shadowed by lights[0]
        Mge_SetMaterial(mat);
        DrawScene();
    Mge_EndLighting3D();
Mge_EndMode3D();

Mge_DrawShadowMap(sm, 12, 12, 220);              // optional: blit the depth texture to debug
```

`center` / `radius` frame the light's view volume — pass the scene's bounding
sphere (too large softens the shadow, too small clips it). Only `lights[0]`
casts. The compare uses a 3×3 PCF filter and a slope-scaled bias; the depth
texture lands on texture unit 1, so material textures (unit 0) are unaffected.
`Mge_BeginShadowPass` must run before `Mge_ClearBackground` — it redirects
rendering to its own framebuffer and restores the window on `Mge_EndShadowPass`.

Demo: `examples/lighting/shadow_mapping.c` — a moving sun over a few blocks, with
the shadow map shown in the corner.

#### Point (omnidirectional) shadows

A point/spot light shadows in every direction, so pass 1 renders the occluders
into a depth **cubemap** — once per face — storing the distance from the light:

```c
PointShadowMap ps = Mge_LoadPointShadowMap(1024);
...
Mge_BeginPointShadowPass(&ps, lamp, 22.0f);   // farPlane = max shadow distance
    for (int f = 0; f < 6; f++) { Mge_SetPointShadowFace(f); DrawOccluders(); }
Mge_EndPointShadowPass();

Mge_BeginMode3D(camera);
    Mge_BeginLighting3DPointShadowed(&lamp, 1, camera, ps);  // lights[0] casts
        Mge_SetMaterial(mat);
        DrawScene();
    Mge_EndLighting3D();
Mge_EndMode3D();
```

The compare uses a 20-tap disk PCF. `Mge_BeginLighting3DShadowed` (2D map) and
`Mge_BeginLighting3DPointShadowed` (cube) are mutually exclusive — `lights[0]`
casts one kind or the other. The 2D map is on texture unit 1, the cube on unit 2.

Demo: `examples/lighting/point_shadows.c` — a lamp bobbing inside a room, cubes
casting onto the walls, floor and ceiling.

## Normal mapping

Put a tangent-space normal map in `MATERIAL_MAP_NORMAL` and lit surfaces pick up
per-pixel bumps — no tangent vertex attribute needed: the lighting shader builds
the TBN frame from screen-space derivatives of position and UV.

```c
Material wall = Mge_DefaultMaterial();
Mge_SetMaterialTexture(&wall, MATERIAL_MAP_DIFFUSE, Mge_LoadTexture("brick.jpg"));
Mge_SetMaterialTexture(&wall, MATERIAL_MAP_NORMAL, Mge_LoadTexture("brick_normal.jpg"));
// ... Mge_SetMaterial(wall); Draw_Cube(...);   // inside Mge_BeginLighting3D
```

Load the normal map **linear** (`Mge_LoadTexture`, never `...Ex(path, true)`) — it
is vector data, not colour. OpenGL-convention maps (green = +Y) work as-is. It
binds to texture unit 3. `Mge_LoadModel` picks up `NORMALS` / `HEIGHT` textures
automatically, so imported models are normal-mapped without extra code.

Demo: `examples/lighting/normal_mapping.c` — `assets/brickwall/` on a flat quad,
SPACE toggles the map.

## Parallax mapping

Add a grayscale **depth map** to `MATERIAL_MAP_HEIGHT` and the lighting shader
does parallax-occlusion mapping: it marches the depth field in tangent space
along the view ray and shifts the sampled texture coordinates to the hit, so a
flat quad looks genuinely displaced — grooves hide behind ridges at grazing
angles, with a stepped silhouette. Same derivative-based tangent frame as the
normal map (no tangent attribute); binds to texture unit 4.

The map follows LearnOpenGL's convention: **black = surface, white = deep groove**
(their `bricks2_disp.jpg`). If you have a *height* map (white = high), invert it
first.

```c
Material wall = Mge_DefaultMaterial();
Mge_SetMaterialTexture(&wall, MATERIAL_MAP_DIFFUSE, albedo);
Mge_SetMaterialTexture(&wall, MATERIAL_MAP_NORMAL, normal);   // pair them
Mge_SetMaterialTexture(&wall, MATERIAL_MAP_HEIGHT, depth);    // black = surface, white = deep
wall.maps[MATERIAL_MAP_HEIGHT].value = 0.1f;                  // displacement scale
// ... Mge_SetMaterial(wall); Draw_Cube(...);
```

Load the depth map **linear**. Keep `.value` small (`0.05`–`0.15`) — large scales
smear at oblique angles. Meshes (`Mge_DrawMesh`) don't do parallax.

Demo: `examples/lighting/parallax_mapping.c` — LearnOpenGL's `assets/bricks/`
set; SPACE toggles parallax, UP/DOWN the scale, N the normal map.

## Materials & material maps

A `Material` is a fixed set of `MaterialMap` slots (indexed by
`MaterialMapIndex`) plus a specular `shininess`. Each map carries a **texture**, a
**color** and a scalar **value**; what those mean depends on the slot:

| slot | `.texture` | `.color` | `.value` |
| --- | --- | --- | --- |
| `MATERIAL_MAP_DIFFUSE` | albedo image sampled across the surface (id `0` → a white 1×1, i.e. "untextured") | tint multiplied over the texture | base-colour gain (`1` = as-is, `>1` brighter) |
| `MATERIAL_MAP_SPECULAR` | unused | tints the highlight (`WHITE` = untinted) | highlight strength multiplier: `1` = as the light sets it, `0` = matte |
| `MATERIAL_MAP_NORMAL` | tangent-space normal map (RGB = XYZ); load it linear. Unset → the vertex normal is used | unused | strength: `0` = flat, `1` = as authored, `>1` = exaggerated relief |
| `MATERIAL_MAP_HEIGHT` | grayscale depth map (black = surface, white = groove — `bricks2_disp.jpg`); load it linear. Set → parallax-occlusion mapping displaces the sampled UVs along the view ray | unused | height scale (`~0.05` subtle … `0.15` strong; `0` = off) |

`MATERIAL_MAP_DIFFUSE.value` defaults to `1` and `MATERIAL_MAP_HEIGHT.value` to
`~0.08` via `Mge_DefaultMaterial()` — a **raw `(Material){…}` literal** must set
the ones it uses (`0` = black diffuse / flat normal). Parallax and the normal map
share the shader's derivative-based tangent frame; pair them for the best result.

```c
typedef struct MaterialMap {
    Texture2D texture;   // id 0 -> engine's white 1x1
    Color     color;
    float     value;
} MaterialMap;

typedef struct Material {
    MaterialMap maps[MATERIAL_MAP_COUNT];  // [MATERIAL_MAP_DIFFUSE], [MATERIAL_MAP_SPECULAR]
    float       shininess;
} Material;
```

Start from `Mge_DefaultMaterial()` and edit the slots you care about:

```c
Texture2D wall = Mge_LoadTexture("assets/wall.jpg");
// with gamma correction on, load colour maps as sRGB instead:
// Texture2D wall = Mge_LoadTextureEx("assets/wall.jpg", true);

Material m = Mge_DefaultMaterial();
Mge_SetMaterialTexture(&m, MATERIAL_MAP_DIFFUSE, wall);       // or: m.maps[...].texture = wall;
m.maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 255, 210, 180, 255 }; // warm tint over the texture
m.maps[MATERIAL_MAP_SPECULAR].value = 0.0f;                   // matte
m.shininess = 48.0f;

Mge_BeginLighting3D(light, camera);
    Mge_SetMaterial(m);
    Draw_Cube(pos, size, m.maps[MATERIAL_MAP_DIFFUSE].color); // pass the diffuse tint as the vertex colour
Mge_EndLighting3D();
```

`Mge_SetMaterial` binds the diffuse + normal textures and sets the per-slot
uniforms (diffuse gain, specular strength + tint, normal strength, shininess);
the diffuse **color** reaches the shader through the drawn geometry's per-vertex
colour, so pass it to `Draw_Cube` (or `MgeGL_Color4ub`).
`Mge_DrawObject` does this for an `Object` automatically —
`obj.material.maps[MATERIAL_MAP_DIFFUSE].color` is seeded from the colour you
gave `Mge_MakeObject3D`. `Draw_Cube` emits per-face UVs so a texture maps one
full copy onto every face. A texture that fails to load has id `0` and falls
back to the flat colour.

Free a texture you loaded with `Mge_UnloadTexture(tex)` (id `0` and the shared
white texture are ignored) — do this before overwriting a slot so you don't leak
the old one.

Demo: `examples/materials/textured_cube.c` — textured/tinted/matte/plain cubes
side by side under a moving light.

#### Wrap mode

How a texture samples outside `0..1` UV. New textures are `TEXTURE_WRAP_REPEAT`;
change it once after loading:

```c
Mge_SetTextureWrap(tex, TEXTURE_WRAP_CLAMP);              // both axes
Mge_SetTextureWrapEx(tex, TEXTURE_WRAP_REPEAT, TEXTURE_WRAP_CLAMP); // U repeats, V clamps
```

| `TextureWrap` | GL | use |
| --- | --- | --- |
| `TEXTURE_WRAP_REPEAT` | `GL_REPEAT` | tiling — brick, grass, terrain (the default) |
| `TEXTURE_WRAP_CLAMP` | `GL_CLAMP_TO_EDGE` | decals, UI, spotlight cookies — no pattern loop |
| `TEXTURE_WRAP_MIRROR_REPEAT` | `GL_MIRRORED_REPEAT` | seamless-by-mirroring; flips at every integer boundary |
| `TEXTURE_WRAP_MIRROR_CLAMP` | `GL_MIRROR_CLAMP_TO_EDGE` | "mirror once" — mirror across one boundary, then clamp |

The state lives on the GL texture object, so set it once (it isn't stored in
`Material`). The editor's inspector has a per-slot **wrap** dropdown.

#### Tiling, offset & triplanar

Changing the wrap mode to `REPEAT` does **not** repeat a texture on its own — you
also have to scale the UVs. That's a per-material transform applied to every map:

```c
mat.tiling = (Vector2){ 4.0f, 4.0f }; // uv' = uv*tiling + offset -> 16 copies, no stretch
mat.offset = (Vector2){ 0.25f, 0.0f };
```

To keep texels **square when an object is scaled non-uniformly**, turn on
triplanar projection — it samples the maps from world-space XYZ (blending the
three axis planes by the normal) instead of the mesh UVs, so stretching the
geometry tiles the texture rather than smearing it:

```c
mat.triplanar = true;
mat.triplanarScale = 1.0f;            // world units per texture tile
```

The **normal map** (whiteout blend) and **height map** (a per-plane
parallax-occlusion march, offset-limiting) both follow the projection. `tiling`
does **not** apply under triplanar — scale it with `triplanarScale`.
`Mge_DefaultMaterial()` sets `tiling {1,1}`, `offset {0,0}`, `triplanar false`.
Demo: `examples/materials/tiling_triplanar.c`.

#### Picking a texture at runtime

`Mge_OpenImageDialog()` pops the OS "open file" dialog (Windows: comdlg32;
Linux: `zenity`, then `kdialog`) filtered to image extensions and returns a
**malloc'd** path you `free()`, or `NULL` on cancel / when no backend is present.
`Mge_OpenFileDialog(title, filterName, filterExts)` is the general form
(`filterExts` is `;`-separated, e.g. `"*.png;*.jpg"`), and
`Mge_SaveFileDialog(title, filterName, filterExts, defaultName)` is the save
counterpart (overwrite prompt, pre-filled name), and
`Mge_OpenFolderDialog(title)` picks an existing directory (Windows:
`SHBrowseForFolder`; Linux: `zenity --directory`). The dialogs never change the
process working directory. The editor's inspector uses the open dialog for the
material-map thumbnails; the editor's Project / Scene menus use both for
`project.mgproject` and `scene.mgscene` open / save.

