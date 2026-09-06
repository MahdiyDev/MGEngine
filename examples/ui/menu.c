// Retained widget GUI -- Phase 1 + 2: a mock pause menu built from Column / Row /
// Center / Expanded / Spacer. Arrow keys move the selection; the tree is built
// once and only Mge_UiSetContainerStyle re-styles the highlighted row. The option
// list is longer than its box, so it sits in a Mge_UiListView -- mouse wheel or
// click-drag scrolls it, and arrow-key selection keeps the current row in view.

#include "mge.h"
#include "mge_ui.h"

#include <signal.h>
#include <stdlib.h>

static void signal_handler(int sig)
{
    Mge_CloseWindow();
    exit(sig);
}

#define N_ITEMS 9
static const char* LABELS[N_ITEMS] = {
    "Resume", "Restart Level", "Options", "Controls", "Audio",
    "Video", "Achievements", "Main Menu", "Quit"
};
static MgeUiWidget s_rows[N_ITEMS];
static MgeUiWidget s_list;

static MgeContainerStyle row_style(bool selected)
{
    return (MgeContainerStyle){
        .expand = true,
        .padding = Mge_EdgeInsetsSymmetric(18, 12),
        .decoration = {
            .color = selected ? (Color){ 70, 110, 200, 255 } : (Color){ 40, 44, 58, 255 },
            .borderRadius = Mge_BorderRadiusAll(8),
        },
    };
}

int main(void)
{
    Mge_InitWindow(720, 520, "MGEngine - ui menu");
    Mge_SetTargetFPS(60);
    signal(SIGINT, signal_handler);

    // full-screen dim backdrop, contents centred
    MgeUiWidget root = Mge_UiColumn((MgeFlexStyle){ .crossAxis = MGE_CROSS_STRETCH });
    Mge_UiAddChild(root, Mge_UiSpacer(1));

    // the card: a centred fixed-width Column
    MgeUiWidget centre = Mge_UiCenter();
    Mge_UiAddChild(root, centre);
    MgeUiWidget card = Mge_UiContainer((MgeContainerStyle){
        .width = 300,
        .padding = Mge_EdgeInsetsAll(20),
        .decoration = {
            .color = (Color){ 24, 26, 34, 245 },
            .border = Mge_BorderAll((Color){ 90, 100, 130, 255 }, 2),
            .borderRadius = Mge_BorderRadiusAll(14),
        },
    });
    Mge_UiAddChild(centre, card);

    MgeUiWidget col = Mge_UiColumn((MgeFlexStyle){ .crossAxis = MGE_CROSS_STRETCH, .spacing = 10 });
    Mge_UiAddChild(card, col);
    Mge_UiText(col, "PAUSED", (MgeTextStyle){ .size = 26, .color = Mge_Colors.white });
    Mge_UiAddChild(col, Mge_UiSizedBox(0, 8));

    // the options live in a fixed-height scroll box (wheel / drag to scroll)
    MgeUiWidget listBox = Mge_UiContainer((MgeContainerStyle){ .height = 172 });
    s_list = Mge_UiListView(MGE_AXIS_VERTICAL, (MgeScrollStyle){ 0 });
    Mge_UiAddChild(listBox, s_list);
    Mge_UiAddChild(col, listBox);
    for (int k = 0; k < N_ITEMS; k++) {
        MgeUiWidget slot = Mge_UiPadding(Mge_EdgeInsetsSymmetric(0, 4));
        s_rows[k] = Mge_UiContainer(row_style(k == 0));
        Mge_UiText(s_rows[k], LABELS[k], (MgeTextStyle){ .size = 20, .color = Mge_Colors.white });
        Mge_UiAddChild(slot, s_rows[k]);
        Mge_UiAddChild(s_list, slot);
    }

    // a small run stats block: a 2-column Table (label -> value)
    Mge_UiAddChild(col, Mge_UiSizedBox(0, 6));
    MgeTableColumn cols[2] = { { MGE_COL_INTRINSIC, 0 }, { MGE_COL_FLEX, 1 } };
    MgeUiWidget stats = Mge_UiTable(cols, 2, 4.0f, 12.0f);
    const char* sk[2] = { "Time", "Score" };
    const char* sv[2] = { "12:04", "8,120" };
    for (int k = 0; k < 2; k++) {
        MgeUiWidget r = Mge_UiTableRow();
        Mge_UiText(r, sk[k], (MgeTextStyle){ .size = 15, .color = (Color){ 150, 160, 190, 255 } });
        MgeUiWidget rightAlign = Mge_UiAlign(MGE_ALIGN_CENTER_RIGHT);
        Mge_UiText(rightAlign, sv[k], (MgeTextStyle){ .size = 15, .color = Mge_Colors.white });
        Mge_UiAddChild(r, rightAlign);
        Mge_UiAddChild(stats, r);
    }
    Mge_UiAddChild(col, stats);

    Mge_UiAddChild(root, Mge_UiSpacer(1));
    Mge_UiSetRoot(root);

    int sel = 0;
    while (!Mge_WindowShouldClose()) {
        int prev = sel;
        if (IsKeyPressed(KEY_DOWN)) sel = (sel + 1) % N_ITEMS;
        if (IsKeyPressed(KEY_UP)) sel = (sel + N_ITEMS - 1) % N_ITEMS;
        if (sel != prev) {
            Mge_UiSetContainerStyle(s_rows[prev], row_style(false));
            Mge_UiSetContainerStyle(s_rows[sel], row_style(true));
            Mge_UiScrollToChild(s_list, s_rows[sel]); // keep the selection visible
        }
        if (IsKeyPressed(KEY_ENTER) && sel == N_ITEMS - 1) // "Quit"
            break;

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
