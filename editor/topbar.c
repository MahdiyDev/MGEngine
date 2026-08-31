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

    bool msaa = Mge_IsMSAAEnabled();
    char msaaLbl[24];
    snprintf(msaaLbl, sizeof(msaaLbl), "MSAA (%dx)", Mge_GetRequestedMSAA());
    if (Mge_GuiCheckbox(msaa ? msaaLbl : "MSAA (off)", &msaa))
        Mge_SetMSAAEnabled(msaa);

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

static TopbarAction file_menu(Scene* s)
{
    TopbarAction act = TOPBAR_NONE;
    if (!Mge_GuiBeginMenu("File"))
        return act;
    if (Mge_GuiMenuItem("New")) act = TOPBAR_NEW;
    if (Mge_GuiMenuItem("Open...")) act = TOPBAR_OPEN;
    if (Mge_GuiMenuItem(s->path[0] ? "Save" : "Save...")) act = TOPBAR_SAVE;
    if (Mge_GuiMenuItem("Save As...")) act = TOPBAR_SAVE_AS;
    if (Mge_GuiMenuItem("Build")) act = TOPBAR_BUILD;
    Mge_GuiEndMenu();
    return act;
}

TopbarAction Topbar_Draw(Rectangle rect, Scene* s, bool* editMode)
{
    if (!Mge_GuiBeginPanel("##topbar", rect.x, rect.y, rect.width, rect.height)) {
        Mge_GuiEndPanel();
        return TOPBAR_NONE;
    }

    TopbarAction act = file_menu(s);
    Mge_GuiSameLine();

    // scene name + unsaved marker
    char title[80];
    snprintf(title, sizeof(title), "%s%s", s->name, s->dirty ? " *" : "");
    Mge_GuiLabel(title);
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
    return act;
}
