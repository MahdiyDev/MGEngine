// Retained widget GUI -- Phase 2b: a 10,000-row virtualized list. Only the rows
// on screen exist as widgets; Mge_UiListViewBuilder rebuilds that window every
// frame. Mouse wheel / drag scroll it; HOME/END jump to the ends, G to a random
// row. A header reads back Mge_UiScrollOffset / Mge_UiScrollMax.

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

#define N_ROWS 10000

static MgeUiWidget row_builder(int index, void* user)
{
    (void)user;
    char label[32];
    snprintf(label, sizeof label, "Item #%04d", index);

    MgeUiWidget pad = Mge_UiPadding(Mge_EdgeInsetsSymmetric(14, 5));
    bool band = (index / 5) % 2 == 0;
    MgeUiWidget row = Mge_UiContainer((MgeContainerStyle){
        .expand = true,
        .padding = Mge_EdgeInsetsSymmetric(12, 8),
        .decoration = {
            .color = band ? (Color){ 32, 36, 48, 255 } : (Color){ 24, 27, 37, 255 },
            .borderRadius = Mge_BorderRadiusAll(6),
        },
    });
    Mge_UiText(row, label, (MgeTextStyle){ .size = 18, .color = Mge_Colors.white });
    Mge_UiAddChild(pad, row);
    return pad;
}

int main(void)
{
    Mge_InitWindow(560, 640, "MGEngine - ui virtualized list");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    MgeUiWidget root = Mge_UiColumn((MgeFlexStyle){ .crossAxis = MGE_CROSS_STRETCH });

    MgeUiWidget headerBox = Mge_UiContainer((MgeContainerStyle){
        .height = 40, .padding = Mge_EdgeInsetsSymmetric(14, 10),
        .decoration = { .color = (Color){ 18, 20, 28, 255 } } });
    MgeUiWidget header = Mge_UiText(headerBox, "", (MgeTextStyle){ .size = 16, .color = (Color){ 170, 180, 210, 255 } });
    Mge_UiAddChild(root, headerBox);

    MgeUiWidget listArea = Mge_UiExpanded(1);
    MgeUiWidget list = Mge_UiListViewBuilder(MGE_AXIS_VERTICAL, N_ROWS, 40.0f,
        row_builder, NULL, (MgeScrollStyle){ 0 });
    Mge_UiAddChild(listArea, list);
    Mge_UiAddChild(root, listArea);
    Mge_UiSetRoot(root);

    srand(1234);
    while (!Mge_WindowShouldClose()) {
        if (IsKeyPressed(KEY_HOME)) Mge_UiScrollToEdge(list, false);
        if (IsKeyPressed(KEY_END)) Mge_UiScrollToEdge(list, true);
        if (IsKeyPressed(KEY_G)) Mge_UiScrollToIndex(list, rand() % N_ROWS);

        char info[96];
        snprintf(info, sizeof info, "%d rows   scroll %.0f / %.0f   (wheel / drag, HOME / END, G = random)",
            N_ROWS, Mge_UiScrollOffset(list), Mge_UiScrollMax(list));
        Mge_UiSetText(header, info);

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
