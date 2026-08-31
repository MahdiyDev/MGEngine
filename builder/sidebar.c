#include "sidebar.h"

#include <mge_gui.h>
#include <stdlib.h>
#include <stdio.h>

// One material-map slot as a group: a texture thumbnail (opens a file picker,
// plus a clear "x"), then the slot's colour and its scalar value. `showColor` is
// false for the normal map, where a tint is meaningless. `valueMax` / `valueLabel`
// give the slot's `.value` its slot-specific meaning.
static void material_slot(Material* mat, int mapIndex, const char* name, const char* id,
    bool showColor, float valueMax, const char* valueLabel)
{
    MaterialMap* m = &mat->maps[mapIndex];
    char lbl[32];

    Mge_GuiSeparator();
    Mge_GuiLabel(name);

    if (Mge_GuiImageButton(id, m->texture.id, 56.0f)) {
        char* path = Mge_OpenImageDialog();
        if (path != NULL) {
            Mge_UnloadTexture(m->texture);
            m->texture = Mge_LoadTextureEx(path, mapIndex == MATERIAL_MAP_DIFFUSE);
            free(path);
        }
    }
    if (m->texture.id != 0) {
        Mge_GuiSameLine();
        snprintf(lbl, sizeof(lbl), "x##%s", id);
        if (Mge_GuiButton(lbl)) {
            Mge_UnloadTexture(m->texture);
            m->texture = (Texture2D){ 0 };
        }
    }

    if (showColor) {
        snprintf(lbl, sizeof(lbl), "color##%s", id);
        Mge_GuiInputColor(lbl, &m->color);
    }
    snprintf(lbl, sizeof(lbl), "%s##%s", valueLabel, id);
    Mge_GuiSliderFloat(lbl, &m->value, 0.0f, valueMax);
}

static void inspect_object(Object* o)
{
    static const char* prims[3] = { "cube (3D)", "sphere (3D)", "plane (3D)" };
    Mge_GuiLabel(o->kind == OBJECT_3D ? prims[o->primitive] : "rect (2D)");
    Mge_GuiInputVec3("position", &o->position);
    Mge_GuiInputVec3("rotation", &o->rotation);
    Mge_GuiInputVec3("size", &o->size);
    Mge_GuiSeparator();

    Mge_GuiLabel("material");
    Mge_GuiInputFloat("shininess", &o->material.shininess);
    material_slot(&o->material, MATERIAL_MAP_DIFFUSE, "diffuse map", "diffuse", true, 2.0f, "gain");
    material_slot(&o->material, MATERIAL_MAP_SPECULAR, "specular map", "specular", true, 1.0f, "strength");
    material_slot(&o->material, MATERIAL_MAP_NORMAL, "normal map", "normal", false, 4.0f, "strength");
}

static void inspect_light(Light* l)
{
    static const char* kinds[3] = { "directional", "point", "spot" };
    Mge_GuiLabel(kinds[l->type]);
    Mge_GuiCheckbox("enabled", &l->enabled);
    Mge_GuiInputColorRGB("color", &l->color);
    Mge_GuiSliderFloat("ambient", &l->ambient, 0.0f, 1.0f);
    Mge_GuiSliderFloat("diffuse", &l->diffuse, 0.0f, 2.0f);
    Mge_GuiSliderFloat("specular", &l->specular, 0.0f, 2.0f);
    if (l->type != LIGHT_DIRECTIONAL) {
        Mge_GuiSeparator();
        Mge_GuiInputVec3("position", &l->position);
        Mge_GuiInputFloat("linear", &l->linear);
        Mge_GuiInputFloat("quadratic", &l->quadratic);
    }
    if (l->type != LIGHT_POINT) {
        Mge_GuiSeparator();
        Mge_GuiInputVec3("direction", &l->direction);
    }
}

static void gizmo_switch(void)
{
    Mge_GuiLabel("GIZMO");
    GizmoMode mode = Mge_GetGizmoMode();
    if (Mge_GuiSelectable("Move (translate)", mode == GIZMO_TRANSLATE))
        Mge_SetGizmoMode(GIZMO_TRANSLATE);
    if (Mge_GuiSelectable("Rotate", mode == GIZMO_ROTATE))
        Mge_SetGizmoMode(GIZMO_ROTATE);
    if (Mge_GuiSelectable("Scale", mode == GIZMO_SCALE))
        Mge_SetGizmoMode(GIZMO_SCALE);

    Mge_GuiSpacing();
    GizmoSpace space = Mge_GetGizmoSpace();
    Mge_GuiLabel(space == GIZMO_LOCAL ? "SPACE: Local" : "SPACE: World");
    if (Mge_GuiSelectable("World", space == GIZMO_WORLD))
        Mge_SetGizmoSpace(GIZMO_WORLD);
    if (Mge_GuiSelectable("Local", space == GIZMO_LOCAL))
        Mge_SetGizmoSpace(GIZMO_LOCAL);
}

void Sidebar_Draw(Scene* s, bool editMode, int fps, int draws)
{
    if (Mge_GuiBeginSidebar("Scene", 300.0f, false)) {
        Mge_GuiLabel(editMode ? "MODE: EDIT" : "MODE: VIEW  (TAB to edit)");

        char row[48];
        snprintf(row, sizeof(row), "FPS: %d   draws: %d", fps, draws);
        Mge_GuiLabel(row);
        Mge_GuiCheckbox("shadows", &s->shadowsOn);
        Mge_GuiSeparator();

        gizmo_switch();
        Mge_GuiSeparator();

        Mge_GuiLabel("OBJECTS");
        for (int i = 0; i < s->objectCount; i++)
            if (Mge_GuiSelectable(s->objectNames[i], s->selKind == SEL_OBJECT && s->selIndex == i)) {
                s->selKind = SEL_OBJECT;
                s->selIndex = i;
            }

        Mge_GuiSeparator();
        Mge_GuiLabel("LIGHTS");
        for (int i = 0; i < s->lightCount; i++)
            if (Mge_GuiSelectable(s->lightNames[i], s->selKind == SEL_LIGHT && s->selIndex == i)) {
                s->selKind = SEL_LIGHT;
                s->selIndex = i;
            }

        Mge_GuiSeparator();
        Mge_GuiLabel("INSPECTOR");
        Mge_GuiSpacing();
        if (s->selKind == SEL_OBJECT)
            inspect_object(&s->objects[s->selIndex]);
        else if (s->selKind == SEL_LIGHT)
            inspect_light(&s->lights[s->selIndex]);
        else
            Mge_GuiLabel("(nothing selected)");
    }
    Mge_GuiEndSidebar();
}
