#include "hierarchy.h"

#include <mge_gui.h>
#include <stdio.h>
#include <stdlib.h>
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

static void commit_rename(Scene* s, History* h)
{
    if (s_renameKind == SEL_OBJECT && s_renameIndex < s->objectCount)
        snprintf(s->objectNames[s_renameIndex], sizeof(s->objectNames[s_renameIndex]), "%s", s_renameBuf);
    else if (s_renameKind == SEL_LIGHT && s_renameIndex < s->lightCount)
        snprintf(s->lightNames[s_renameIndex], sizeof(s->lightNames[s_renameIndex]), "%s", s_renameBuf);
    s_renameKind = SEL_NONE;
    s_renameIndex = -1;
    History_Record(h);
    s->dirty = true;
}

// deferred hierarchy DnD result: move `from` object to `to`, or (parent) set its
// parent to `to`. -1 = nothing.
static int s_dndFrom = -1, s_dndTo = -1;
static bool s_dndParent = false;

// One object row: checkbox + name (drag source + drop target) + delete button.
static void object_row(Scene* s, History* h, int i, int* del)
{
    char id[32];
    snprintf(id, sizeof(id), "##o%d", i);
    int depth = Scene_ParentDepth(s, i);

    if (Mge_GuiCheckbox(id, &s->objects[i].active)) {
        History_Record(h);
        s->dirty = true;
    }
    Mge_GuiSameLine();

    if (s_renameKind == SEL_OBJECT && s_renameIndex == i) {
        char field[40];
        snprintf(field, sizeof(field), "##ren%s", id);
        Mge_GuiSetNextItemWidth(150.0f);
        Mge_GuiInputText(field, s_renameBuf, (int)sizeof(s_renameBuf));
        Mge_GuiSameLine();
        if (Mge_GuiButton("ok"))
            commit_rename(s, h);
        return;
    }

    char label[64];
    snprintf(label, sizeof(label), "%*s%s##o%d", depth * 2, "", s->objectNames[i], i);

    bool dbl = false;
    if (Mge_GuiSelectableEx(label, Scene_IsObjectSelected(s, i), &dbl)) {
        bool add = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        Scene_SelectObject(s, i, add);
    }
    if (dbl)
        begin_rename(SEL_OBJECT, i, s->objectNames[i]);

    // drag this row; drop another row onto it -> reorder (or reparent with Shift)
    char pay[16];
    snprintf(pay, sizeof(pay), "%d", i);
    Mge_GuiDragSource(pay, s->objectNames[i]);
    char got[16];
    if (Mge_GuiDropTarget(got, (int)sizeof(got))) {
        int from = atoi(got);
        if (from != i) {
            s_dndFrom = from;
            s_dndTo = i;
            s_dndParent = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        }
    }

    char b[40];
    snprintf(b, sizeof(b), "x%s", id);
    if (Mge_GuiRowButton(b))
        *del = i;
}

static void light_row(Scene* s, History* h, int i, int* del)
{
    char id[32];
    snprintf(id, sizeof(id), "##l%d", i);

    if (Mge_GuiCheckbox(id, &s->lights[i].enabled)) {
        History_Record(h);
        s->dirty = true;
    }
    Mge_GuiSameLine();

    if (s_renameKind == SEL_LIGHT && s_renameIndex == i) {
        char field[40];
        snprintf(field, sizeof(field), "##ren%s", id);
        Mge_GuiSetNextItemWidth(150.0f);
        Mge_GuiInputText(field, s_renameBuf, (int)sizeof(s_renameBuf));
        Mge_GuiSameLine();
        if (Mge_GuiButton("ok"))
            commit_rename(s, h);
        return;
    }

    bool dbl = false;
    bool selected = (s->selKind == SEL_LIGHT && s->selIndex == i);
    if (Mge_GuiSelectableEx(s->lightNames[i], selected, &dbl)) {
        s->selKind = SEL_LIGHT;
        s->selIndex = i;
        s->selExtraCount = 0;
    }
    if (dbl)
        begin_rename(SEL_LIGHT, i, s->lightNames[i]);

    if (s->lights[i].type != LIGHT_DIRECTIONAL) {
        char b[40];
        snprintf(b, sizeof(b), "x%s", id);
        if (Mge_GuiRowButton(b))
            *del = i;
    }
}

static void add_menu(Scene* s, History* h)
{
    if (!Mge_GuiBeginMenu("+ add"))
        return;
    if (Mge_GuiMenuItem("Cube")) { History_Record(h); Scene_AddShape(s, PRIM_CUBE); }
    if (Mge_GuiMenuItem("Sphere")) { History_Record(h); Scene_AddShape(s, PRIM_SPHERE); }
    if (Mge_GuiMenuItem("Plane")) { History_Record(h); Scene_AddShape(s, PRIM_PLANE); }
    if (Mge_GuiMenuItem("Light")) { History_Record(h); Scene_AddLight(s); }
    if (Mge_GuiMenuItem("Camera")) { History_Record(h); Scene_AddCamera(s); }
    Mge_GuiEndMenu();
}

bool Hierarchy_Draw(Rectangle rect, Scene* s, History* h)
{
    bool wantDelete = false;

    if (!Mge_GuiBeginPanel("Hierarchy", rect.x, rect.y, rect.width, rect.height)) {
        Mge_GuiEndPanel();
        return false;
    }

    Mge_GuiLabel("HIERARCHY");
    Mge_GuiSameLine();
    add_menu(s, h);
    Mge_GuiSeparator();

    int delObj = -1, delLight = -1;
    s_dndFrom = s_dndTo = -1;

    if (Mge_GuiSelectable("Environment", s->selKind == SEL_ENV)) {
        s->selKind = SEL_ENV;
        s->selIndex = 0;
        s->selExtraCount = 0;
    }
    Mge_GuiSeparator();

    Mge_GuiLabel("OBJECTS");
    for (int i = 0; i < s->objectCount; i++)
        object_row(s, h, i, &delObj);

    Mge_GuiSeparator();
    Mge_GuiLabel("LIGHTS");
    for (int i = 0; i < s->lightCount; i++)
        light_row(s, h, i, &delLight);

    // apply deferred DnD after the row loop so indices don't shift mid-frame
    if (s_dndFrom >= 0 && s_dndTo >= 0) {
        History_Record(h);
        if (s_dndParent)
            Scene_SetParent(s, s_dndFrom, s_dndTo);
        else
            Scene_MoveObject(s, s_dndFrom, s_dndTo);
    }

    if (delObj >= 0) {
        Scene_SelectObject(s, delObj, false);
        wantDelete = true; // let the caller confirm + record
    }
    if (delLight >= 0) {
        History_Record(h);
        Scene_DeleteLight(s, delLight);
    }

    Mge_GuiEndPanel();
    return wantDelete;
}
