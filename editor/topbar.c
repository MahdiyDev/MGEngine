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

static TopbarResult project_menu(const Project* proj)
{
    TopbarResult r = { TOPBAR_NONE, 0 };
    if (!Mge_GuiBeginMenu("Project"))
        return r;
    if (Mge_GuiMenuItem("New Project..."))  r.action = TOPBAR_PROJECT_NEW;
    if (Mge_GuiMenuItem("Open Project...")) r.action = TOPBAR_PROJECT_OPEN;
    if (Mge_GuiMenuItem(proj->path[0] ? "Save Project" : "Save Project...")) r.action = TOPBAR_PROJECT_SAVE;
    if (proj->path[0]) {
        Mge_GuiSeparator();
        if (Mge_GuiMenuItem("Build Release...")) r.action = TOPBAR_BUILD_RELEASE;
    }
    Mge_GuiEndMenu();
    return r;
}

static TopbarResult scene_menu(const Project* proj, const Scene* s)
{
    TopbarResult r = { TOPBAR_NONE, 0 };

    char label[96];
    snprintf(label, sizeof(label), "Scene: %s%s", s->name, s->dirty ? " *" : "");
    if (!Mge_GuiBeginMenu(label))
        return r;

    for (int i = 0; i < proj->sceneCount; i++) {
        char row[80];
        snprintf(row, sizeof(row), "%s %s", (i == proj->activeScene) ? ">" : " ", proj->scenes[i]);
        if (Mge_GuiMenuItem(row)) {
            r.action = TOPBAR_SCENE_SWITCH;
            r.arg = i;
        }
    }
    Mge_GuiSeparator();

    bool haveProject = proj->path[0] != '\0';
    if (haveProject) {
        if (Mge_GuiMenuItem("New Scene..."))   r.action = TOPBAR_SCENE_NEW;
        if (Mge_GuiMenuItem("Add Scene..."))   r.action = TOPBAR_SCENE_ADD;
        if (Mge_GuiMenuItem("Save Scene"))     r.action = TOPBAR_SCENE_SAVE;
        if (Mge_GuiMenuItem("New Script..."))  r.action = TOPBAR_SCENE_NEWSCRIPT;
    } else {
        Mge_GuiLabel("(save the project to add scenes)");
    }
    Mge_GuiEndMenu();
    return r;
}

TopbarResult Topbar_Draw(Rectangle rect, Project* proj, Scene* s,
    bool* editMode, bool playing, bool* showConsole)
{
    if (!Mge_GuiBeginPanel("##topbar", rect.x, rect.y, rect.width, rect.height)) {
        Mge_GuiEndPanel();
        return (TopbarResult){ TOPBAR_NONE, 0 };
    }

    TopbarResult r = project_menu(proj);
    Mge_GuiSameLine();

    char proj_title[80];
    snprintf(proj_title, sizeof(proj_title), "%s%s", proj->name, proj->dirty ? " *" : "");
    Mge_GuiLabel(proj_title);
    Mge_GuiSameLine();

    TopbarResult sr = scene_menu(proj, s);
    if (sr.action != TOPBAR_NONE)
        r = sr;
    Mge_GuiSameLine();

    // build / run the scene code
    if (Mge_GuiButton(playing ? "[Stop]" : "Play"))
        r.action = playing ? TOPBAR_STOP : TOPBAR_PLAY;
    Mge_GuiSameLine();
    if (Mge_GuiButton("Build"))
        r.action = TOPBAR_BUILD;
    Mge_GuiSameLine();
    if (Mge_GuiButton(*showConsole ? "[Console]" : "Console"))
        *showConsole = !*showConsole;
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
    return r;
}
