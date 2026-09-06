// Retained widget GUI -- Phase 3b: a tiny form. Two text fields (Tab between
// them, click to place the caret, arrows / selection / clipboard all work), a
// "Greet" button that echoes the name into a label. The window's exit key is
// cleared so Esc just unfocuses the field.

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
    Mge_InitWindow(520, 380, "MGEngine - ui form");
    Mge_SetTargetFPS(60);
    Mge_SetExitKey(0); // Esc unfocuses a field instead of closing the window
    signal(SIGINT, signal_handler);

    static MgeUiTextBuffer name, note;
    Mge_UiTextBufferSet(&name, "");
    Mge_UiTextBufferSet(&note, "");

    MgeUiWidget root = Mge_UiCenter();
    MgeUiWidget card = Mge_UiContainer((MgeContainerStyle){
        .width = 400, .padding = Mge_EdgeInsetsAll(24),
        .decoration = { .color = (Color){ 24, 26, 34, 255 },
            .border = Mge_BorderAll((Color){ 90, 100, 130, 255 }, 2),
            .borderRadius = Mge_BorderRadiusAll(16) } });
    Mge_UiAddChild(root, card);
    MgeUiWidget col = Mge_UiColumn((MgeFlexStyle){ .crossAxis = MGE_CROSS_STRETCH, .spacing = 12, .mainSize = MGE_MAIN_SIZE_MIN });
    Mge_UiAddChild(card, col);

    Mge_UiText(col, "Sign the guestbook", (MgeTextStyle){ .size = 22, .color = Mge_Colors.white });

    Mge_UiText(col, "Name", (MgeTextStyle){ .size = 14, .color = (Color){ 160, 170, 200, 255 } });
    MgeUiWidget nameField = Mge_UiTextField(&name, "your name", (MgeUiTextFieldStyle){ .expand = true, .maxLength = 40 });
    Mge_UiAddChild(col, nameField);

    Mge_UiText(col, "Note", (MgeTextStyle){ .size = 14, .color = (Color){ 160, 170, 200, 255 } });
    MgeUiWidget noteField = Mge_UiTextField(&note, "leave a message", (MgeUiTextFieldStyle){ .expand = true });
    Mge_UiAddChild(col, noteField);

    MgeUiWidget greet = Mge_UiButton("Greet", Mge_UiButtonFilled(Mge_Colors.blue));
    Mge_UiAddChild(col, greet);
    MgeUiWidget out = Mge_UiText(col, " ", (MgeTextStyle){ .size = 16, .color = (Color){ 150, 200, 160, 255 } });

    Mge_UiSetRoot(root);
    Mge_UiFocus(nameField);

    while (!Mge_WindowShouldClose()) {
        if (Mge_UiButtonClicked(greet) || Mge_UiTextSubmitted(noteField)) {
            char buf[2 * MGE_UI_TEXT_CAP + 32];
            snprintf(buf, sizeof buf, "Hello, %s!  (%s)",
                name.len ? name.text : "stranger",
                note.len ? note.text : "no note");
            Mge_UiSetText(out, buf);
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
