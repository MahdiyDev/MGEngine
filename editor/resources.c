#include "resources.h"

#include <mge_gui.h>
#include <stdio.h>

void Resources_Draw(Rectangle rect, Project* proj, Scene* s, int fps, int draws)
{
    if (!Mge_GuiBeginPanel("Resources", rect.x, rect.y, rect.width, rect.height)) {
        Mge_GuiEndPanel();
        return;
    }

    Mge_GuiLabel("RESOURCES");
    Mge_GuiLabel("Per-scene res/ browser + file ops land in Phase 5.");
    Mge_GuiSeparator();

    char row[256];
    snprintf(row, sizeof(row),
        "project \"%s\" (%s)  --  %d scene%s  |  active \"%s\": %d/%d objects, %d/%d lights  |  FPS %d  draws %d",
        proj->name, proj->path[0] ? "saved" : "in-memory",
        proj->sceneCount, proj->sceneCount == 1 ? "" : "s",
        s->name, s->objectCount, SCENE_MAX_OBJECTS, s->lightCount, SCENE_MAX_LIGHTS, fps, draws);
    Mge_GuiLabel(row);

    Mge_GuiEndPanel();
}
