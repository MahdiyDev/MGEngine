#include "explorer.h"

#include <mge_gui.h>
#include <stdio.h>

void Explorer_Draw(Scene* s)
{
    if (!Mge_GuiBeginSidebar("Explorer", 220.0f, true)) {
        Mge_GuiEndSidebar();
        return;
    }

    Mge_GuiLabel("SHAPES");
    Mge_GuiLabel("click to spawn at the origin");
    Mge_GuiSpacing();

    if (Mge_GuiButton("  Cube  "))
        Scene_AddShape(s, PRIM_CUBE);
    if (Mge_GuiButton(" Sphere "))
        Scene_AddShape(s, PRIM_SPHERE);
    if (Mge_GuiButton(" Plane  "))
        Scene_AddShape(s, PRIM_PLANE);

    Mge_GuiSeparator();
    char count[32];
    snprintf(count, sizeof(count), "%d / %d objects", s->objectCount, SCENE_MAX_OBJECTS);
    Mge_GuiLabel(count);
    if (s->objectCount >= SCENE_MAX_OBJECTS)
        Mge_GuiLabel("(scene full)");

    Mge_GuiEndSidebar();
}
