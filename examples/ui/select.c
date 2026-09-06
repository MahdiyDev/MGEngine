// Retained widget GUI -- Phase 3c: dropdown + segmented control + tooltips. The
// dropdown's option list floats above the rest of the panel; a tooltip appears
// when the cursor rests on a control. A label echoes the current picks.

#include "mge.h"
#include "mge_ui.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static void signal_handler(int sig)
{
    Mge_CloseWindow();
    exit(sig);
}

static const char* const DIFFICULTY[4] = { "Easy", "Normal", "Hard", "Nightmare" };
static const char* const VIEW[3] = { "List", "Grid", "Map" };

int main(void)
{
    Mge_InitWindow(520, 420, "MGEngine - ui select");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    int diff = 1, view = 0;

    MgeUiWidget root = Mge_UiCenter();
    MgeUiWidget card = Mge_UiContainer((MgeContainerStyle){
        .width = 340, .padding = Mge_EdgeInsetsAll(24),
        .decoration = { .color = (Color){ 24, 26, 34, 255 },
            .border = Mge_BorderAll((Color){ 90, 100, 130, 255 }, 2),
            .borderRadius = Mge_BorderRadiusAll(16) } });
    Mge_UiAddChild(root, card);
    MgeUiWidget col = Mge_UiColumn((MgeFlexStyle){ .crossAxis = MGE_CROSS_STRETCH, .spacing = 14, .mainSize = MGE_MAIN_SIZE_MIN });
    Mge_UiAddChild(card, col);

    Mge_UiText(col, "New Game", (MgeTextStyle){ .size = 24, .color = Mge_Colors.white });

    Mge_UiText(col, "Difficulty", (MgeTextStyle){ .size = 14, .color = (Color){ 160, 170, 200, 255 } });
    MgeUiWidget diffTip = Mge_UiTooltip("scales enemy health and damage");
    MgeUiWidget dd = Mge_UiDropdown(DIFFICULTY, 4, &diff, (MgeUiDropdownStyle){ .expand = true, .accent = Mge_Colors.blue });
    Mge_UiAddChild(diffTip, dd);
    Mge_UiAddChild(col, diffTip);

    Mge_UiText(col, "Map view", (MgeTextStyle){ .size = 14, .color = (Color){ 160, 170, 200, 255 } });
    MgeUiWidget viewTip = Mge_UiTooltip("how the mini-map is drawn");
    MgeUiWidget sc = Mge_UiSegmentedControl(VIEW, 3, &view, Mge_Colors.blue);
    Mge_UiAddChild(viewTip, sc);
    Mge_UiAddChild(col, viewTip);

    MgeUiWidget echo = Mge_UiText(col, " ", (MgeTextStyle){ .size = 15, .color = (Color){ 150, 200, 160, 255 } });

    Mge_UiSetRoot(root);

    while (!Mge_WindowShouldClose()) {
        if (Mge_UiDropdownChanged(dd) || Mge_UiSegmentChanged(sc)) {
            char buf[64];
            snprintf(buf, sizeof buf, "%s  /  %s view", DIFFICULTY[diff], VIEW[view]);
            Mge_UiSetText(echo, buf);
        }

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 12, 13, 18, 255 });
        Mge_UiNewFrame((float)Mge_GetDeltaTime());
        Mge_UiRender();
        Mge_EndDrawing();
    }

    Mge_UiShutdown();
    Mge_CloseWindow();
    return 0;
}
