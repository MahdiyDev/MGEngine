// Retained widget GUI -- Phase 0: a centred rounded card holding a label.
//
// The tree is built once; Mge_UiRender lays it out and paints it each frame in
// 2D screen space. Resize the window -- the root always fills the viewport, so
// the card stays centred.

#include "mge.h"
#include "mge_ui.h"

#include <signal.h>
#include <stdlib.h>

static void signal_handler(int sig)
{
    Mge_CloseWindow();
    exit(sig);
}

int main(void)
{
    Mge_InitWindow(900, 560, "MGEngine - hello ui");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    // dim full-screen backdrop; centres its child
    MgeUiWidget root = Mge_UiContainer((MgeContainerStyle){
        .decoration = Mge_UiBoxDecoration((Color){ 18, 20, 28, 255 }),
        .alignment = MGE_ALIGN_CENTER,
    });

    // the card: white, rounded, a soft border, generous padding
    MgeUiWidget card = Mge_UiContainer((MgeContainerStyle){
        .padding = Mge_EdgeInsetsSymmetric(28.0f, 20.0f),
        .decoration = {
            .color = Mge_Colors.white,
            .border = Mge_BorderAll((Color){ 120, 140, 200, 255 }, 3.0f),
            .borderRadius = Mge_BorderRadiusAll(14.0f),
        },
    });
    Mge_UiAddChild(root, card);

    MgeUiWidget label = Mge_UiText(card, "Label...",
        (MgeTextStyle){ .size = 26, .color = (Color){ 30, 34, 48, 255 } });

    Mge_UiSetRoot(root);

    int frame = 0;
    while (!Mge_WindowShouldClose()) {
        // a tiny bit of life: retype the label every second
        if (++frame % 60 == 0)
            Mge_UiSetText(label, (frame / 60) % 2 ? "Label... (tick)" : "Label...");

        Mge_BeginDrawing();
        Mge_ClearBackground((Color){ 8, 9, 12, 255 });
        Mge_UiNewFrame((float)Mge_GetDeltaTime());
        Mge_UiRender();
        Mge_EndDrawing();
    }

    Mge_UiShutdown();
    Mge_CloseWindow();
    return 0;
}
