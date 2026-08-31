#include "hierarchy.h"

#include <mge_gui.h>
#include <stdio.h>
#include <string.h>

// which row (if any) is being renamed in place
static int s_renameKind = SEL_NONE;
static int s_renameIndex = -1;
static char s_renameBuf[24];

static void begin_rename(int kind, int index, const char* current)
{
    s_renameKind = kind;
    s_renameIndex = index;
    snprintf(s_renameBuf, sizeof(s_renameBuf), "%s", current);
}

static void commit_rename(Scene* s)
{
    if (s_renameKind == SEL_OBJECT && s_renameIndex < s->objectCount)
        snprintf(s->objectNames[s_renameIndex], sizeof(s->objectNames[s_renameIndex]), "%s", s_renameBuf);
    else if (s_renameKind == SEL_LIGHT && s_renameIndex < s->lightCount)
        snprintf(s->lightNames[s_renameIndex], sizeof(s->lightNames[s_renameIndex]), "%s", s_renameBuf);
    s_renameKind = SEL_NONE;
    s_renameIndex = -1;
    s->dirty = true;
}

// One list row. Returns the index to delete (via *del), else leaves it.
static void entity_row(Scene* s, int kind, int i, const char* name, bool* activeFlag, bool canDelete, int* del)
{
    char id[32];
    snprintf(id, sizeof(id), "##%s%d", kind == SEL_OBJECT ? "o" : "l", i);

    if (Mge_GuiCheckbox(id, activeFlag)) // active (object) / enabled (light)
        s->dirty = true;
    Mge_GuiSameLine();

    if (s_renameKind == kind && s_renameIndex == i) {
        char field[40];
        snprintf(field, sizeof(field), "##ren%s", id);
        Mge_GuiSetNextItemWidth(150.0f);
        Mge_GuiInputText(field, s_renameBuf, (int)sizeof(s_renameBuf));
        Mge_GuiSameLine();
        if (Mge_GuiButton("ok"))
            commit_rename(s);
        return;
    }

    bool dbl = false;
    bool selected = (s->selKind == kind && s->selIndex == i);
    if (Mge_GuiSelectableEx(name, selected, &dbl)) {
        s->selKind = kind;
        s->selIndex = i;
    }
    if (dbl)
        begin_rename(kind, i, name);

    if (canDelete) {
        Mge_GuiSameLine();
        char b[40];
        snprintf(b, sizeof(b), "x%s", id);
        if (Mge_GuiButton(b))
            *del = i;
    }
}

static void add_menu(Scene* s)
{
    if (!Mge_GuiBeginMenu("+ add"))
        return;
    if (Mge_GuiMenuItem("Cube"))
        Scene_AddShape(s, PRIM_CUBE);
    if (Mge_GuiMenuItem("Sphere"))
        Scene_AddShape(s, PRIM_SPHERE);
    if (Mge_GuiMenuItem("Plane"))
        Scene_AddShape(s, PRIM_PLANE);
    if (Mge_GuiMenuItem("Light"))
        Scene_AddLight(s);
    if (Mge_GuiMenuItem("Camera"))
        Scene_AddCamera(s);
    Mge_GuiEndMenu();
}

void Hierarchy_Draw(Rectangle rect, Scene* s)
{
    if (!Mge_GuiBeginPanel("Hierarchy", rect.x, rect.y, rect.width, rect.height)) {
        Mge_GuiEndPanel();
        return;
    }

    Mge_GuiLabel("HIERARCHY");
    Mge_GuiSameLine();
    add_menu(s);
    Mge_GuiSeparator();

    int delObj = -1, delLight = -1;

    // fixed pseudo-entity: the scene's sun + skybox + game camera live here
    if (Mge_GuiSelectable("Environment", s->selKind == SEL_ENV)) {
        s->selKind = SEL_ENV;
        s->selIndex = 0;
    }
    Mge_GuiSeparator();

    Mge_GuiLabel("OBJECTS");
    for (int i = 0; i < s->objectCount; i++)
        entity_row(s, SEL_OBJECT, i, s->objectNames[i], &s->objects[i].active, true, &delObj);

    Mge_GuiSeparator();
    Mge_GuiLabel("LIGHTS");
    for (int i = 0; i < s->lightCount; i++)
        entity_row(s, SEL_LIGHT, i, s->lightNames[i], &s->lights[i].enabled,
            s->lights[i].type != LIGHT_DIRECTIONAL, &delLight);

    if (delObj >= 0)
        Scene_DeleteObject(s, delObj);
    if (delLight >= 0)
        Scene_DeleteLight(s, delLight);

    Mge_GuiEndPanel();
}
