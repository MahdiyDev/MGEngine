// Material construction helpers.
//
// A `Material` is a small fixed set of `MaterialMap` slots (see mge.h); each map
// carries a texture, a colour and a scalar `value`. These helpers only build /
// edit the struct -- binding a material for drawing lives in mge_light.c
// (`Mge_SetMaterial`), which needs a live GL context.

#include "mge.h"

Material Mge_DefaultMaterial(void)
{
    Material m = { 0 };
    m.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;  // untinted albedo
    m.maps[MATERIAL_MAP_DIFFUSE].value = 1.0f;   // base-colour gain
    m.maps[MATERIAL_MAP_SPECULAR].color = WHITE; // white (untinted) highlight
    m.maps[MATERIAL_MAP_SPECULAR].value = 1.0f;  // specular strength as the light sets it
    m.maps[MATERIAL_MAP_NORMAL].color = WHITE;   // unused; kept sane for the inspector
    m.maps[MATERIAL_MAP_NORMAL].value = 1.0f;    // normal-map strength (0 = flat)
    m.maps[MATERIAL_MAP_HEIGHT].color = WHITE;   // unused
    m.maps[MATERIAL_MAP_HEIGHT].value = 0.08f;   // parallax height scale (only active with a height map)
    m.shininess = 32.0f;
    m.tiling = (Vector2){ 1.0f, 1.0f };          // no UV scaling
    m.offset = (Vector2){ 0.0f, 0.0f };
    m.triplanar = false;
    m.triplanarScale = 1.0f;                     // 1 world unit per tile
    return m;
}

void Mge_SetMaterialTexture(Material* material, int mapIndex, Texture2D texture)
{
    if (material == NULL || mapIndex < 0 || mapIndex >= MATERIAL_MAP_COUNT)
        return;
    material->maps[mapIndex].texture = texture;
}

// PBR: a white dielectric, mid roughness. Each map defaults to id 0 (unused) ->
// the shader falls back to the *Value / albedoColor scalars.
PBRMaterial Mge_DefaultPBRMaterial(void)
{
    PBRMaterial m = { 0 };
    m.albedoColor = (Vector3){ 1.0f, 1.0f, 1.0f };
    m.metallicValue = 0.0f;
    m.roughnessValue = 0.5f;
    m.aoValue = 1.0f;
    return m;
}
