#include "inspector.h"

#include <mge_gui.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char* const WRAP_NAMES[4] = { "Repeat", "Clamp", "Mirror", "Mirror Once" };

// One material-map slot as a group: a texture thumbnail (opens a file picker,
// plus a clear "x"), then the slot's colour, scalar value and wrap mode.
// `showColor` is false for the normal / height maps, where a tint is meaningless.
// `valueMax` / `valueLabel` give the slot's `.value` its slot-specific meaning.
// `wrap` / `texPath` point at the editor's stored state for this slot.
// Returns true if anything changed this frame.
static bool material_slot(Material* mat, int mapIndex, unsigned char* wrap, char* texPath,
    const char* name, const char* id, bool showColor, float valueMax, const char* valueLabel)
{
    MaterialMap* m = &mat->maps[mapIndex];
    char lbl[32];
    bool changed = false;

    Mge_GuiSeparator();
    Mge_GuiLabel(name);

    if (Mge_GuiImageButton(id, m->texture.id, 56.0f)) {
        char* path = Mge_OpenImageDialog();
        if (path != NULL) {
            Mge_UnloadTexture(m->texture);
            m->texture = Mge_LoadTextureEx(path, mapIndex == MATERIAL_MAP_DIFFUSE);
            Mge_SetTextureWrap(m->texture, *wrap); // re-apply the chosen wrap
            snprintf(texPath, SCENE_TEXPATH_LEN, "%s", path);
            free(path);
            changed = true;
        }
    }
    if (m->texture.id != 0) {
        Mge_GuiSameLine();
        snprintf(lbl, sizeof(lbl), "x##%s", id);
        if (Mge_GuiButton(lbl)) {
            Mge_UnloadTexture(m->texture);
            m->texture = (Texture2D){ 0 };
            texPath[0] = '\0';
            changed = true;
        }
    }

    if (showColor) {
        snprintf(lbl, sizeof(lbl), "color##%s", id);
        changed |= Mge_GuiInputColor(lbl, &m->color);
    }
    snprintf(lbl, sizeof(lbl), "%s##%s", valueLabel, id);
    changed |= Mge_GuiSliderFloat(lbl, &m->value, 0.0f, valueMax);

    int w = *wrap;
    snprintf(lbl, sizeof(lbl), "wrap##%s", id);
    if (Mge_GuiCombo(lbl, &w, WRAP_NAMES, 4)) {
        *wrap = (unsigned char)w;
        Mge_SetTextureWrap(m->texture, w);
        changed = true;
    }
    return changed;
}

static void inspect_object(Scene* s)
{
    Object* o = &s->objects[s->selIndex];
    unsigned char* wrap = s->texWrap[s->selIndex];
    char (*tex)[SCENE_TEXPATH_LEN] = s->texPath[s->selIndex];
    bool ch = false;

    static const char* prims[3] = { "cube", "sphere", "plane" };
    ch |= Mge_GuiCheckbox("active", &o->active);
    if (o->kind == OBJECT_3D) {
        int prim = o->primitive;
        if (Mge_GuiCombo("primitive", &prim, prims, 3)) {
            o->primitive = (PrimitiveKind)prim;
            ch = true;
        }
    } else {
        Mge_GuiLabel("rect (2D)");
    }
    ch |= Mge_GuiInputVec3("position", &o->transform.position);
    ch |= Mge_GuiInputVec3("rotation", &o->transform.rotation);
    ch |= Mge_GuiInputVec3("size", &o->transform.scale);
    Mge_GuiSeparator();

    Mge_GuiLabel("material");
    ch |= Mge_GuiInputFloat("shininess", &o->material.shininess);
    ch |= Mge_GuiInputVec2("tiling", &o->material.tiling);   // uv * tiling + offset -- repeat without scaling
    ch |= Mge_GuiInputVec2("offset", &o->material.offset);
    ch |= Mge_GuiCheckbox("triplanar", &o->material.triplanar); // project diffuse from world XYZ
    if (o->material.triplanar)
        ch |= Mge_GuiInputFloat("triplanar scale", &o->material.triplanarScale);
    ch |= material_slot(&o->material, MATERIAL_MAP_DIFFUSE, &wrap[MATERIAL_MAP_DIFFUSE], tex[MATERIAL_MAP_DIFFUSE], "diffuse map", "diffuse", true, 2.0f, "gain");
    ch |= material_slot(&o->material, MATERIAL_MAP_SPECULAR, &wrap[MATERIAL_MAP_SPECULAR], tex[MATERIAL_MAP_SPECULAR], "specular map", "specular", true, 1.0f, "strength");
    ch |= material_slot(&o->material, MATERIAL_MAP_NORMAL, &wrap[MATERIAL_MAP_NORMAL], tex[MATERIAL_MAP_NORMAL], "normal map", "normal", false, 4.0f, "strength");
    ch |= material_slot(&o->material, MATERIAL_MAP_HEIGHT, &wrap[MATERIAL_MAP_HEIGHT], tex[MATERIAL_MAP_HEIGHT], "height map (parallax)", "height", false, 0.2f, "scale");

    if (ch)
        s->dirty = true;
}

static void inspect_light(Scene* s)
{
    Light* l = &s->lights[s->selIndex];
    static const char* kinds[3] = { "directional", "point", "spot" };
    bool ch = false;

    Mge_GuiLabel(kinds[l->type]);
    ch |= Mge_GuiCheckbox("enabled", &l->enabled);
    ch |= Mge_GuiInputColorRGB("color", &l->color);
    ch |= Mge_GuiSliderFloat("ambient", &l->ambient, 0.0f, 1.0f);
    ch |= Mge_GuiSliderFloat("diffuse", &l->diffuse, 0.0f, 2.0f);
    ch |= Mge_GuiSliderFloat("specular", &l->specular, 0.0f, 2.0f);
    if (l->type != LIGHT_DIRECTIONAL) {
        Mge_GuiSeparator();
        ch |= Mge_GuiInputVec3("position", &l->position);
        ch |= Mge_GuiInputFloat("linear", &l->linear);
        ch |= Mge_GuiInputFloat("quadratic", &l->quadratic);
    }
    if (l->type != LIGHT_POINT) {
        Mge_GuiSeparator();
        ch |= Mge_GuiInputVec3("direction", &l->direction);
    }

    if (ch)
        s->dirty = true;
}

void Inspector_Draw(Rectangle rect, Scene* s)
{
    if (!Mge_GuiBeginPanel("Inspector", rect.x, rect.y, rect.width, rect.height)) {
        Mge_GuiEndPanel();
        return;
    }

    Mge_GuiLabel("INSPECTOR");
    Mge_GuiSeparator();

    if (s->selKind == SEL_OBJECT)
        inspect_object(s);
    else if (s->selKind == SEL_LIGHT)
        inspect_light(s);
    else
        Mge_GuiLabel("(nothing selected)");

    Mge_GuiEndPanel();
}
