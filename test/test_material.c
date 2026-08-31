// Material / MaterialMap construction (no window or GL context needed).

#include <stdbool.h>

#include "mge.h"
#include "test.h" // mlib repo-wide harness

TEST(default_material)
{
    Material m = Mge_DefaultMaterial();

    // diffuse slot: white, untextured, value unused (1.0)
    CHECK(m.maps[MATERIAL_MAP_DIFFUSE].color.r == 255);
    CHECK(m.maps[MATERIAL_MAP_DIFFUSE].color.g == 255);
    CHECK(m.maps[MATERIAL_MAP_DIFFUSE].color.b == 255);
    CHECK(m.maps[MATERIAL_MAP_DIFFUSE].color.a == 255);
    CHECK(m.maps[MATERIAL_MAP_DIFFUSE].texture.id == 0);
    CHECK(m.maps[MATERIAL_MAP_DIFFUSE].value == 1.0f);

    // specular slot: strength multiplier defaults to 1, highlight untinted (white)
    CHECK(m.maps[MATERIAL_MAP_SPECULAR].value == 1.0f);
    CHECK(m.maps[MATERIAL_MAP_SPECULAR].color.r == 255 && m.maps[MATERIAL_MAP_SPECULAR].color.b == 255);
    CHECK(m.maps[MATERIAL_MAP_SPECULAR].texture.id == 0);

    // normal slot: strength defaults to 1 (0 would flatten any normal map)
    CHECK(m.maps[MATERIAL_MAP_NORMAL].value == 1.0f);
    CHECK(m.maps[MATERIAL_MAP_NORMAL].texture.id == 0);

    // height slot: a small default parallax scale, only active with a texture
    CHECK(m.maps[MATERIAL_MAP_HEIGHT].value > 0.0f && m.maps[MATERIAL_MAP_HEIGHT].value < 0.5f);
    CHECK(m.maps[MATERIAL_MAP_HEIGHT].texture.id == 0);

    CHECK(m.shininess == 32.0f);

    // UV transform: identity by default, triplanar off
    CHECK(m.tiling.x == 1.0f && m.tiling.y == 1.0f);
    CHECK(m.offset.x == 0.0f && m.offset.y == 0.0f);
    CHECK(!m.triplanar);
    CHECK(m.triplanarScale == 1.0f);
}

TEST(map_slots)
{
    // the slots are distinct and COUNT is the last
    CHECK(MATERIAL_MAP_DIFFUSE == 0);
    CHECK(MATERIAL_MAP_SPECULAR == 1);
    CHECK(MATERIAL_MAP_NORMAL == 2);
    CHECK(MATERIAL_MAP_HEIGHT == 3);
    CHECK(MATERIAL_MAP_COUNT == 4);
}

TEST(set_material_texture)
{
    Material m = Mge_DefaultMaterial();
    Texture2D wall = { .id = 7, .width = 64, .height = 64 };

    Mge_SetMaterialTexture(&m, MATERIAL_MAP_DIFFUSE, wall);
    CHECK(m.maps[MATERIAL_MAP_DIFFUSE].texture.id == 7);
    CHECK(m.maps[MATERIAL_MAP_DIFFUSE].texture.width == 64);
    // the colour tint is left untouched
    CHECK(m.maps[MATERIAL_MAP_DIFFUSE].color.r == 255);

    Texture2D spec = { .id = 9 };
    Mge_SetMaterialTexture(&m, MATERIAL_MAP_SPECULAR, spec);
    CHECK(m.maps[MATERIAL_MAP_SPECULAR].texture.id == 9);
    CHECK(m.maps[MATERIAL_MAP_DIFFUSE].texture.id == 7); // other slot unchanged
}

TEST(set_material_texture_guards)
{
    Material m = Mge_DefaultMaterial();
    Texture2D t = { .id = 5 };

    // out-of-range indices and a NULL material are silently ignored
    Mge_SetMaterialTexture(&m, -1, t);
    Mge_SetMaterialTexture(&m, MATERIAL_MAP_COUNT, t);
    Mge_SetMaterialTexture(NULL, MATERIAL_MAP_DIFFUSE, t);

    CHECK(m.maps[MATERIAL_MAP_DIFFUSE].texture.id == 0);
    CHECK(m.maps[MATERIAL_MAP_SPECULAR].texture.id == 0);
}

int main(void)
{
    RUN(default_material);
    RUN(map_slots);
    RUN(set_material_texture);
    RUN(set_material_texture_guards);
    return test_summary();
}
