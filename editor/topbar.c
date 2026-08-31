#include "topbar.h"

#include <mge_gui.h>
#include <stdio.h>

// icons TBD -- text placeholders for now
static void gizmo_button(const char* label, GizmoMode mode)
{
    GizmoMode cur = Mge_GetGizmoMode();
    char lbl[24];
    snprintf(lbl, sizeof(lbl), cur == mode ? "[%s]" : " %s ", label);
    if (Mge_GuiButton(lbl))
        Mge_SetGizmoMode(mode);
    Mge_GuiSameLine();
}

static void render_menu(Scene* s)
{
    if (!Mge_GuiBeginMenu("Render"))
        return;

    Mge_GuiCheckbox("shadows", &s->shadowsOn);
    Mge_GuiCheckbox("HDR", &s->hdrOn);
    if (s->hdrOn) {
        static const char* const TONE_NAMES[3] = { "Reinhard", "Exposure", "ACES" };
        Mge_GuiSetNextItemWidth(120.0f);
        Mge_GuiCombo("tone map", &s->toneMap, TONE_NAMES, 3);
        Mge_GuiSliderFloat("exposure", &s->exposure, 0.1f, 4.0f);
        Mge_GuiCheckbox("bloom", &s->bloomOn);
        if (s->bloomOn) {
            Mge_GuiSliderFloat("threshold", &s->bloom.threshold, 0.0f, 3.0f);
            Mge_GuiSliderFloat("intensity", &s->bloom.intensity, 0.0f, 2.0f);
        }
    }
    Mge_GuiEndMenu();
}

void Topbar_Draw(Rectangle rect, Scene* s, bool* editMode)
{
    if (!Mge_GuiBeginPanel("##topbar", rect.x, rect.y, rect.width, rect.height)) {
        Mge_GuiEndPanel();
        return;
    }

    // scene name
    Mge_GuiSetNextItemWidth(150.0f);
    Mge_GuiInputText("##scene", s->name, (int)sizeof(s->name));
    Mge_GuiSameLine();

    // file actions -- wired up in Phase 2 / 3
    if (Mge_GuiButton("Open"))
        printf("[editor] Open: scene files land in Phase 2\n");
    Mge_GuiSameLine();
    if (Mge_GuiButton("Save"))
        printf("[editor] Save: scene files land in Phase 2\n");
    Mge_GuiSameLine();
    if (Mge_GuiButton("Build"))
        printf("[editor] Build: scene-as-code lands in Phase 3\n");
    Mge_GuiSameLine();
    Mge_GuiSeparator();
    Mge_GuiSameLine();

    // view / edit
    if (Mge_GuiButton(*editMode ? "EDIT" : "VIEW"))
        *editMode = !*editMode;
    Mge_GuiSameLine();

    // gizmo mode
    gizmo_button("Move", GIZMO_TRANSLATE);
    gizmo_button("Rot", GIZMO_ROTATE);
    gizmo_button("Scl", GIZMO_SCALE);

    // gizmo space
    int space = (Mge_GetGizmoSpace() == GIZMO_LOCAL) ? 1 : 0;
    static const char* const SPACE_NAMES[2] = { "World", "Local" };
    Mge_GuiSetNextItemWidth(80.0f);
    if (Mge_GuiCombo("##space", &space, SPACE_NAMES, 2))
        Mge_SetGizmoSpace(space == 1 ? GIZMO_LOCAL : GIZMO_WORLD);
    Mge_GuiSameLine();

    render_menu(s);

    Mge_GuiEndPanel();
}
