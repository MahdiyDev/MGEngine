#include "resources.h"

#include <mge_gui.h>
#include <stdio.h>

void Resources_Draw(Rectangle rect, Scene* s, int fps, int draws)
{
    if (!Mge_GuiBeginPanel("Resources", rect.x, rect.y, rect.width, rect.height)) {
        Mge_GuiEndPanel();
        return;
    }

    Mge_GuiLabel("RESOURCES");
    Mge_GuiLabel("Per-scene res/ browser + file ops land in Phase 4.");
    Mge_GuiSeparator();

    char row[200];
    snprintf(row, sizeof(row), "scene \"%s\"  --  %d / %d objects, %d / %d lights   |   FPS %d   draws %d",
        s->name, s->objectCount, SCENE_MAX_OBJECTS, s->lightCount, SCENE_MAX_LIGHTS, fps, draws);
    Mge_GuiLabel(row);

    Mge_GuiEndPanel();
}
