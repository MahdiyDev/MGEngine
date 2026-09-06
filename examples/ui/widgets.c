// Retained widget GUI -- Phase 3: an interactive panel. A button bumps a
// counter, a checkbox shows / hides a note, a slider drives a label and a
// progress bar, a switch flips the accent, radios pick a size. Everything is
// pointer-driven (no keyboard); Mge_UiWantsPointer() gates nothing here because
// the demo has no world behind it.

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

int main(void)
{
    Mge_InitWindow(520, 460, "MGEngine - ui widgets");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    bool  showNote = true;
    bool  bigAccent = false;
    int   size = 1;
    float amount = 0.35f;
    int   clicks = 0;

    MgeUiWidget root = Mge_UiCenter();
    MgeUiWidget card = Mge_UiContainer((MgeContainerStyle){
        .width = 380, .padding = Mge_EdgeInsetsAll(22),
        .decoration = { .color = (Color){ 24, 26, 34, 255 },
            .border = Mge_BorderAll((Color){ 90, 100, 130, 255 }, 2),
            .borderRadius = Mge_BorderRadiusAll(16) } });
    Mge_UiAddChild(root, card);
    MgeUiWidget col = Mge_UiColumn((MgeFlexStyle){ .crossAxis = MGE_CROSS_STRETCH, .spacing = 14, .mainSize = MGE_MAIN_SIZE_MIN });
    Mge_UiAddChild(card, col);

    Mge_UiText(col, "Widgets", (MgeTextStyle){ .size = 24, .color = Mge_Colors.white });

    MgeUiWidget clickLabel = Mge_UiText(col, "clicked 0x", (MgeTextStyle){ .size = 16, .color = (Color){ 160, 170, 200, 255 } });
    MgeUiWidget btn = Mge_UiButton("Click me", Mge_UiButtonFilled(Mge_Colors.blue));
    Mge_UiAddChild(col, btn);

    MgeUiWidget noteRow = Mge_UiRow((MgeFlexStyle){ .spacing = 10, .crossAxis = MGE_CROSS_CENTER, .mainSize = MGE_MAIN_SIZE_MIN });
    MgeUiWidget noteCheck = Mge_UiCheckbox(&showNote, Mge_Colors.green);
    Mge_UiAddChild(noteRow, noteCheck);
    Mge_UiText(noteRow, "show the note", (MgeTextStyle){ .size = 15, .color = Mge_Colors.white });
    Mge_UiAddChild(col, noteRow);

    MgeUiWidget note = Mge_UiVisibility(true);
    Mge_UiText(note, "-- a note that the checkbox reveals --", (MgeTextStyle){ .size = 14, .color = (Color){ 150, 200, 160, 255 } });
    Mge_UiAddChild(col, note);

    MgeUiWidget accentRow = Mge_UiRow((MgeFlexStyle){ .spacing = 10, .crossAxis = MGE_CROSS_CENTER, .mainSize = MGE_MAIN_SIZE_MIN });
    Mge_UiAddChild(accentRow, Mge_UiSwitch(&bigAccent, Mge_Colors.magenta));
    Mge_UiText(accentRow, "punchy accent", (MgeTextStyle){ .size = 15, .color = Mge_Colors.white });
    Mge_UiAddChild(col, accentRow);

    MgeUiWidget sizeRow = Mge_UiRow((MgeFlexStyle){ .spacing = 8, .crossAxis = MGE_CROSS_CENTER, .mainSize = MGE_MAIN_SIZE_MIN });
    for (int k = 0; k < 3; k++) {
        Mge_UiAddChild(sizeRow, Mge_UiRadio(&size, k, Mge_Colors.blue));
        Mge_UiText(sizeRow, (const char*[]){ "S", "M", "L" }[k], (MgeTextStyle){ .size = 15, .color = Mge_Colors.white });
    }
    Mge_UiAddChild(col, sizeRow);

    MgeUiWidget amountLabel = Mge_UiText(col, "amount 35%", (MgeTextStyle){ .size = 15, .color = (Color){ 160, 170, 200, 255 } });
    Mge_UiAddChild(col, Mge_UiSlider(&amount, 0.0f, 1.0f, 0.0f));
    MgeUiWidget bar = Mge_UiProgressBar(amount);
    Mge_UiAddChild(col, bar);

    Mge_UiSetRoot(root);

    while (!Mge_WindowShouldClose()) {
        if (Mge_UiButtonClicked(btn)) {
            clicks++;
            char buf[32];
            snprintf(buf, sizeof buf, "clicked %dx", clicks);
            Mge_UiSetText(clickLabel, buf);
        }
        Mge_UiSetVisible(note, showNote);
        Mge_UiSetProgress(bar, amount);
        {
            char buf[32];
            snprintf(buf, sizeof buf, "amount %d%%", (int)(amount * 100.0f + 0.5f));
            Mge_UiSetText(amountLabel, buf);
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
