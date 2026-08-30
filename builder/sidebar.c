#include "sidebar.h"

#include <mge_gui.h>
#include <stdio.h>

static void inspect_object(Object* o)
{
    Mge_GuiLabel(o->kind == OBJECT_3D ? "box (3D)" : "rect (2D)");
    Mge_GuiInputVec3("position", &o->position);
    Mge_GuiInputVec3("rotation", &o->rotation);
    Mge_GuiInputVec3("size", &o->size);
    Mge_GuiInputColor("color", &o->color);
    Mge_GuiSeparator();
    Mge_GuiLabel("material");
    Mge_GuiInputColor("diffuse", &o->material.maps[MATERIAL_MAP_DIFFUSE].color);
    Mge_GuiSliderFloat("specular", &o->material.maps[MATERIAL_MAP_SPECULAR].value, 0.0f, 1.0f);
    Mge_GuiInputFloat("shininess", &o->material.shininess);
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
}

void Sidebar_Draw(Scene* s, bool editMode, int fps, int draws)
{
    Mge_GuiBeginFrame();

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

    Mge_GuiEndFrame();
}
