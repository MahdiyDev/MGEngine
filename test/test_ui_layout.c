// Retained widget GUI layout (source/mge_ui.c) with NO GL context: mge_ui.c +
// mge_text.c + mge_shapes.c + mge_gl.c are compiled against test/glstub.
// Covers the box-constraint solver -- container sizing, padding, alignment,
// constraints, nesting -- plus handle lifetime.
//
// Model note: the root is always laid out to fill the viewport (tight
// constraints), like Flutter's RenderView. To size / place something, wrap it
// in the root (Align / fixed size / constraints on a child).

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "mge.h"
#include "mge_ui.h"
#include "mge_gl.h"
#include "test.h"

static bool feq(float a, float b) { return fabsf(a - b) < 1e-3f; }
#define CHECK_F(a, b) CHECK(feq((a), (b)))

// mge_text.c / mge_shapes.c reference these; the layout tests never hit the
// file / texture paths, and always set an explicit viewport.
unsigned char* Mge_LoadFileData(const char* f, size_t* s) { (void)f; if (s) *s = 0; return NULL; }
void Mge_UnloadFileData(unsigned char* d) { (void)d; }
void Mge_UnloadTexture(Texture2D t) { (void)t; }
int Mge_GetScreenWidth(void) { return 800; }
int Mge_GetScreenHeight(void) { return 600; }

// mouse input the GUI reads in Mge_UiNewFrame; the scroll tests drive these
static Vector2 g_mouse = { -1.0f, -1.0f };
static bool    g_mouseDown = false;
static Vector2 g_wheel = { 0.0f, 0.0f };
Vector2 GetMousePosition(void) { return g_mouse; }
bool IsMouseButtonDown(int b) { (void)b; return g_mouseDown; }
Vector2 GetMouseWheelMoveV(void) { return g_wheel; }
// keyboard input the GUI reads in Mge_UiNewFrame / input_update (Phase 3b)
static int  g_charq[16];
static int  g_ncharq;
static bool g_keyEdge[400];
static bool g_keyDown[400];
static char g_clip[256];

int GetCharPressed(void)
{
    if (g_ncharq == 0) return 0;
    int c = g_charq[0];
    for (int i = 0; i < g_ncharq - 1; i++) g_charq[i] = g_charq[i + 1];
    g_ncharq--;
    return c;
}
int GetKeyPressed(void) { return 0; }
bool IsKeyPressed(int k) { return k >= 0 && k < 400 && g_keyEdge[k]; }
bool IsKeyPressedRepeat(int k) { (void)k; return false; }
bool IsKeyDown(int k) { return k >= 0 && k < 400 && g_keyDown[k]; }
const char* Mge_GetClipboardText(void) { return g_clip; }
void Mge_SetClipboardText(const char* s)
{
    strncpy(g_clip, s ? s : "", sizeof g_clip - 1);
    g_clip[sizeof g_clip - 1] = '\0';
}

static void input_reset(void)
{
    g_mouse = (Vector2){ -1.0f, -1.0f };
    g_mouseDown = false;
    g_wheel = (Vector2){ 0.0f, 0.0f };
    g_ncharq = 0;
    memset(g_keyEdge, 0, sizeof g_keyEdge);
    memset(g_keyDown, 0, sizeof g_keyDown);
    g_clip[0] = '\0';
}

static void push_text(const char* s)
{
    for (int i = 0; s[i] && g_ncharq < 16; i++)
        g_charq[g_ncharq++] = (unsigned char)s[i];
}

static void render(MgeUiWidget root, float w, float h)
{
    Mge_UiViewport(w, h);
    Mge_UiSetRoot(root);
    Mge_UiNewFrame(0.0f);
    Mge_UiRender();
}

// build a top-left-aligned viewport-filling root around `subject`, render, and
// return the root so the caller can destroy the whole tree.
static MgeUiWidget wrap(MgeUiWidget subject, float w, float h)
{
    MgeUiWidget root = Mge_UiContainer((MgeContainerStyle){ .alignment = MGE_ALIGN_TOP_LEFT });
    Mge_UiAddChild(root, subject);
    render(root, w, h);
    return root;
}

TEST(root_fills_the_viewport_even_with_a_fixed_size)
{
    MgeUiWidget c = Mge_UiContainer((MgeContainerStyle){ .width = 120, .height = 40 });
    render(c, 800, 600);
    Rectangle r = Mge_UiGetRect(c);
    CHECK_F(r.width, 800.0f); // tight root constraints win over the style size
    CHECK_F(r.height, 600.0f);
    CHECK_F(r.x, 0.0f);
    CHECK_F(r.y, 0.0f);
    Mge_UiDestroy(c);
}

TEST(non_root_container_honours_its_fixed_size)
{
    MgeUiWidget c = Mge_UiContainer((MgeContainerStyle){ .width = 120, .height = 40 });
    MgeUiWidget root = wrap(c, 800, 600);
    Rectangle r = Mge_UiGetRect(c);
    CHECK_F(r.width, 120.0f);
    CHECK_F(r.height, 40.0f);
    Mge_UiDestroy(root);
}

TEST(empty_container_shrinks_to_nothing)
{
    MgeUiWidget c = Mge_UiContainer((MgeContainerStyle){ 0 });
    MgeUiWidget root = wrap(c, 800, 600);
    Rectangle r = Mge_UiGetRect(c);
    CHECK_F(r.width, 0.0f);
    CHECK_F(r.height, 0.0f);
    Mge_UiDestroy(root);
}

TEST(padding_deflates_child_and_inflates_a_shrink_wrap_parent)
{
    MgeUiWidget outer = Mge_UiContainer((MgeContainerStyle){ .padding = Mge_EdgeInsetsAll(10) });
    MgeUiWidget inner = Mge_UiContainer((MgeContainerStyle){ .width = 50, .height = 30 });
    Mge_UiAddChild(outer, inner);
    MgeUiWidget root = wrap(outer, 800, 600);

    Rectangle ro = Mge_UiGetRect(outer);
    Rectangle ri = Mge_UiGetRect(inner);
    CHECK_F(ro.width, 50.0f + 20.0f);
    CHECK_F(ro.height, 30.0f + 20.0f);
    CHECK_F(ri.x, ro.x + 10.0f); // child offset by padding.left
    CHECK_F(ri.y, ro.y + 10.0f);
    Mge_UiDestroy(root);
}

TEST(alignment_positions_the_child_when_there_is_slack)
{
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){
        .width = 200, .height = 100, .alignment = MGE_ALIGN_CENTER });
    MgeUiWidget dot = Mge_UiContainer((MgeContainerStyle){ .width = 20, .height = 20 });
    Mge_UiAddChild(box, dot);
    MgeUiWidget root = wrap(box, 800, 600);

    Rectangle d = Mge_UiGetRect(dot);
    CHECK_F(d.x, Mge_UiGetRect(box).x + (200.0f - 20.0f) / 2.0f);
    CHECK_F(d.y, Mge_UiGetRect(box).y + (100.0f - 20.0f) / 2.0f);

    Mge_UiSetContainerStyle(box, (MgeContainerStyle){
        .width = 200, .height = 100, .alignment = MGE_ALIGN_BOTTOM_RIGHT });
    render(root, 800, 600);
    d = Mge_UiGetRect(dot);
    CHECK_F(d.x, Mge_UiGetRect(box).x + 200.0f - 20.0f);
    CHECK_F(d.y, Mge_UiGetRect(box).y + 100.0f - 20.0f);
    Mge_UiDestroy(root);
}

TEST(style_constraints_clamp_an_oversized_child)
{
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){
        .constraints = { .maxW = 80, .maxH = 80 } });
    MgeUiWidget big = Mge_UiContainer((MgeContainerStyle){ .width = 500, .height = 500 });
    Mge_UiAddChild(box, big);
    MgeUiWidget root = wrap(box, 800, 600);
    Rectangle r = Mge_UiGetRect(box);
    CHECK_F(r.width, 80.0f);
    CHECK_F(r.height, 80.0f);
    Mge_UiDestroy(root);
}

TEST(label_sizes_to_measured_text_and_settext_relayouts)
{
    Font f = Mge_GetDefaultFont();
    MgeUiWidget lbl = Mge_UiLabel(0, "AAAA");
    MgeUiWidget root = wrap(lbl, 800, 600);

    Rectangle r1 = Mge_UiGetRect(lbl);
    CHECK_F(r1.width, Mge_MeasureText(f, "AAAA", 16.0f).x);

    Mge_UiSetText(lbl, "AA");
    render(root, 800, 600);
    Rectangle r2 = Mge_UiGetRect(lbl);
    CHECK(r2.width < r1.width);
    CHECK_F(r2.width, Mge_MeasureText(f, "AA", 16.0f).x);
    Mge_UiDestroy(root);
}

TEST(destroy_invalidates_the_handle_and_its_subtree)
{
    MgeUiWidget p = Mge_UiContainer((MgeContainerStyle){ 0 });
    MgeUiWidget c = Mge_UiLabel(p, "x");
    CHECK(Mge_UiIsValid(p) && Mge_UiIsValid(c));
    CHECK(Mge_UiChildCount(p) == 1);

    Mge_UiDestroy(p);
    CHECK(!Mge_UiIsValid(p));
    CHECK(!Mge_UiIsValid(c)); // child went with it
    CHECK(Mge_UiChildCount(p) == 0);
}

TEST(add_remove_clear_children)
{
    MgeUiWidget p = Mge_UiContainer((MgeContainerStyle){ 0 });
    MgeUiWidget a = Mge_UiContainer((MgeContainerStyle){ 0 });
    MgeUiWidget b = Mge_UiContainer((MgeContainerStyle){ 0 });
    Mge_UiAddChild(p, a);
    Mge_UiAddChild(p, b);
    CHECK(Mge_UiChildCount(p) == 2);
    CHECK(Mge_UiChildAt(p, 0) == a && Mge_UiChildAt(p, 1) == b);
    CHECK(Mge_UiParentOf(a) == p);

    Mge_UiRemoveChild(p, a);
    CHECK(Mge_UiChildCount(p) == 1);
    CHECK(Mge_UiParentOf(a) == 0);
    CHECK(Mge_UiIsValid(a)); // removed, not destroyed

    Mge_UiClearChildren(p);
    CHECK(Mge_UiChildCount(p) == 0);
    CHECK(!Mge_UiIsValid(b)); // clear destroys
    Mge_UiDestroy(a);
    Mge_UiDestroy(p);
}

// ---- Phase 1: flex / stack / wrappers ----

static MgeUiWidget fixed(float w, float h) { return Mge_UiSizedBox(w, h); }

TEST(row_lays_children_along_x_cross_is_tallest)
{
    MgeUiWidget row = Mge_UiRow((MgeFlexStyle){ .mainSize = MGE_MAIN_SIZE_MIN });
    MgeUiWidget a = fixed(40, 20), b = fixed(30, 50);
    Mge_UiAddChild(row, a);
    Mge_UiAddChild(row, b);
    MgeUiWidget root = wrap(row, 800, 600);

    CHECK_F(Mge_UiGetRect(row).width, 70.0f);   // 40 + 30
    CHECK_F(Mge_UiGetRect(row).height, 50.0f);  // tallest child
    CHECK_F(Mge_UiGetRect(a).x, Mge_UiGetRect(row).x);
    CHECK_F(Mge_UiGetRect(b).x, Mge_UiGetRect(row).x + 40.0f);
    CHECK_F(Mge_UiGetRect(a).y, Mge_UiGetRect(row).y + (50.0f - 20.0f) / 2.0f); // cross centre
    Mge_UiDestroy(root);
}

TEST(row_spacing_inserts_a_gap)
{
    MgeUiWidget row = Mge_UiRow((MgeFlexStyle){ .mainSize = MGE_MAIN_SIZE_MIN, .spacing = 8 });
    MgeUiWidget a = fixed(10, 10), b = fixed(10, 10), c = fixed(10, 10);
    Mge_UiAddChild(row, a);
    Mge_UiAddChild(row, b);
    Mge_UiAddChild(row, c);
    MgeUiWidget root = wrap(row, 800, 600);
    CHECK_F(Mge_UiGetRect(row).width, 30.0f + 2.0f * 8.0f);
    CHECK_F(Mge_UiGetRect(b).x, Mge_UiGetRect(row).x + 10.0f + 8.0f);
    Mge_UiDestroy(root);
}

TEST(expanded_takes_the_remaining_main_space)
{
    MgeUiWidget row = Mge_UiRow((MgeFlexStyle){ 0 });
    MgeUiWidget fix = fixed(100, 20);
    MgeUiWidget exp = Mge_UiExpanded(1);
    MgeUiWidget inner = Mge_UiContainer((MgeContainerStyle){ 0 });
    Mge_UiAddChild(exp, inner);
    Mge_UiAddChild(row, fix);
    Mge_UiAddChild(row, exp);

    // put the row in a fixed 400-wide box
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 400, .height = 40 });
    Mge_UiAddChild(box, row);
    MgeUiWidget root = wrap(box, 800, 600);

    CHECK_F(Mge_UiGetRect(exp).width, 300.0f);        // 400 - 100
    CHECK_F(Mge_UiGetRect(exp).x, Mge_UiGetRect(row).x + 100.0f);
    Mge_UiDestroy(root);
}

TEST(two_expanded_split_by_flex_factor)
{
    MgeUiWidget row = Mge_UiRow((MgeFlexStyle){ 0 });
    MgeUiWidget e1 = Mge_UiExpanded(1), e2 = Mge_UiExpanded(2);
    Mge_UiAddChild(row, e1);
    Mge_UiAddChild(row, e2);
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 300, .height = 20 });
    Mge_UiAddChild(box, row);
    MgeUiWidget root = wrap(box, 800, 600);
    CHECK_F(Mge_UiGetRect(e1).width, 100.0f);
    CHECK_F(Mge_UiGetRect(e2).width, 200.0f);
    Mge_UiDestroy(root);
}

TEST(spacer_pushes_a_trailing_child_to_the_end)
{
    MgeUiWidget row = Mge_UiRow((MgeFlexStyle){ 0 });
    MgeUiWidget a = fixed(20, 10), b = fixed(20, 10);
    Mge_UiAddChild(row, a);
    Mge_UiAddChild(row, Mge_UiSpacer(1));
    Mge_UiAddChild(row, b);
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 200, .height = 10 });
    Mge_UiAddChild(box, row);
    MgeUiWidget root = wrap(box, 800, 600);
    CHECK_F(Mge_UiGetRect(a).x, Mge_UiGetRect(row).x);
    CHECK_F(Mge_UiGetRect(b).x, Mge_UiGetRect(row).x + 200.0f - 20.0f);
    Mge_UiDestroy(root);
}

// lay a row of two 30-wide children in a 200-wide box under `align`, return b's
// x relative to the row
static float row_b_x(MgeMainAxisAlignment align)
{
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 200, .height = 20 });
    MgeUiWidget row = Mge_UiRow((MgeFlexStyle){ .mainAxis = align });
    MgeUiWidget a = fixed(30, 10), b = fixed(30, 10);
    Mge_UiAddChild(row, a);
    Mge_UiAddChild(row, b);
    Mge_UiAddChild(box, row);
    MgeUiWidget root = wrap(box, 800, 600);
    float x = Mge_UiGetRect(b).x - Mge_UiGetRect(row).x;
    Mge_UiDestroy(root);
    return x;
}

TEST(main_axis_alignment_positions_children)
{
    CHECK_F(row_b_x(MGE_MAIN_START), 30.0f);          // a then b, flush left
    CHECK_F(row_b_x(MGE_MAIN_END), 200.0f - 30.0f);   // both flush right
    CHECK_F(row_b_x(MGE_MAIN_CENTER), 70.0f + 30.0f); // (200-60)/2 = 70 before a
    CHECK_F(row_b_x(MGE_MAIN_SPACE_BETWEEN), 200.0f - 30.0f);
}

TEST(cross_axis_stretch_fills_the_child)
{
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 100, .height = 60 });
    MgeUiWidget row = Mge_UiRow((MgeFlexStyle){ .crossAxis = MGE_CROSS_STRETCH, .mainSize = MGE_MAIN_SIZE_MIN });
    MgeUiWidget a = Mge_UiContainer((MgeContainerStyle){ .width = 20 }); // height unset
    Mge_UiAddChild(row, a);
    Mge_UiAddChild(box, row);
    MgeUiWidget root = wrap(box, 800, 600);
    CHECK_F(Mge_UiGetRect(a).height, 60.0f); // stretched to the row's cross size
    Mge_UiDestroy(root);
}

TEST(center_and_align_place_the_child)
{
    MgeUiWidget dot = fixed(20, 20);
    MgeUiWidget center = Mge_UiCenter();
    Mge_UiAddChild(center, dot);
    render(center, 400, 300);
    CHECK_F(Mge_UiGetRect(center).width, 400.0f); // expands
    CHECK_F(Mge_UiGetRect(dot).x, (400.0f - 20.0f) / 2.0f);
    CHECK_F(Mge_UiGetRect(dot).y, (300.0f - 20.0f) / 2.0f);
    Mge_UiDestroy(center);

    MgeUiWidget dot2 = fixed(20, 20);
    MgeUiWidget al = Mge_UiAlign(MGE_ALIGN_BOTTOM_RIGHT);
    Mge_UiAddChild(al, dot2);
    render(al, 400, 300);
    CHECK_F(Mge_UiGetRect(dot2).x, 400.0f - 20.0f);
    CHECK_F(Mge_UiGetRect(dot2).y, 300.0f - 20.0f);
    Mge_UiDestroy(al);
}

TEST(stack_sizes_to_children_and_positions_them)
{
    MgeUiWidget stack = Mge_UiStack(MGE_STACK_LOOSE, MGE_ALIGN_CENTER);
    MgeUiWidget big = fixed(100, 80);
    MgeUiWidget pinned = Mge_UiPositioned(10, MGE_UI_NONE, 10, MGE_UI_NONE, MGE_UI_NONE, 12);
    MgeUiWidget pinnedInner = Mge_UiContainer((MgeContainerStyle){ 0 });
    Mge_UiAddChild(pinned, pinnedInner);
    MgeUiWidget left = Mge_UiPositioned(5, 5, MGE_UI_NONE, MGE_UI_NONE, 20, 20);
    Mge_UiAddChild(left, Mge_UiContainer((MgeContainerStyle){ 0 }));
    Mge_UiAddChild(stack, big);
    Mge_UiAddChild(stack, pinned);
    Mge_UiAddChild(stack, left);
    MgeUiWidget root = wrap(stack, 800, 600);

    CHECK_F(Mge_UiGetRect(stack).width, 100.0f);       // sized to `big`
    CHECK_F(Mge_UiGetRect(pinned).width, 100.0f - 20.0f); // left 10 + right 10
    CHECK_F(Mge_UiGetRect(pinned).x, Mge_UiGetRect(stack).x + 10.0f);
    CHECK_F(Mge_UiGetRect(left).x, Mge_UiGetRect(stack).x + 5.0f);
    CHECK_F(Mge_UiGetRect(left).y, Mge_UiGetRect(stack).y + 5.0f);
    Mge_UiDestroy(root);
}

TEST(visibility_false_collapses_the_subtree)
{
    MgeUiWidget row = Mge_UiRow((MgeFlexStyle){ .mainSize = MGE_MAIN_SIZE_MIN });
    MgeUiWidget a = fixed(30, 10);
    MgeUiWidget vis = Mge_UiVisibility(false);
    Mge_UiAddChild(vis, fixed(50, 10));
    MgeUiWidget b = fixed(30, 10);
    Mge_UiAddChild(row, a);
    Mge_UiAddChild(row, vis);
    Mge_UiAddChild(row, b);
    MgeUiWidget root = wrap(row, 800, 600);

    CHECK_F(Mge_UiGetRect(vis).width, 0.0f);
    CHECK_F(Mge_UiGetRect(row).width, 60.0f); // a + b only
    CHECK_F(Mge_UiGetRect(b).x, Mge_UiGetRect(row).x + 30.0f);

    Mge_UiSetVisible(vis, true);
    render(root, 800, 600);
    CHECK_F(Mge_UiGetRect(vis).width, 50.0f);
    CHECK_F(Mge_UiGetRect(row).width, 110.0f);
    Mge_UiDestroy(root);
}

// ---- Phase 1b ----

TEST(wrap_flows_children_into_runs)
{
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 100 });
    MgeUiWidget w = Mge_UiWrap((MgeWrapStyle){ .spacing = 0, .runSpacing = 5 });
    MgeUiWidget a = fixed(60, 20), b = fixed(60, 24), c = fixed(60, 20);
    Mge_UiAddChild(w, a);
    Mge_UiAddChild(w, b);
    Mge_UiAddChild(w, c);
    Mge_UiAddChild(box, w);
    MgeUiWidget root = wrap(box, 800, 600);

    CHECK_F(Mge_UiGetRect(a).x, Mge_UiGetRect(w).x);          // run 1: a, b won't fit (60+60 > 100)
    CHECK_F(Mge_UiGetRect(b).x, Mge_UiGetRect(w).x);          // run 2: b
    CHECK_F(Mge_UiGetRect(b).y, Mge_UiGetRect(w).y + 20.0f + 5.0f); // run 1 height + runSpacing
    CHECK_F(Mge_UiGetRect(c).y, Mge_UiGetRect(b).y + 24.0f + 5.0f); // run 3: c
    CHECK_F(Mge_UiGetRect(w).height, 20.0f + 24.0f + 20.0f + 2.0f * 5.0f);
    Mge_UiDestroy(root);
}

TEST(table_resolves_columns_and_lays_rows)
{
    MgeTableColumn cols[3] = {
        { MGE_COL_FIXED, 40 },
        { MGE_COL_INTRINSIC, 0 },
        { MGE_COL_FLEX, 1 },
    };
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 200 });
    MgeUiWidget t = Mge_UiTable(cols, 3, 6.0f, 10.0f);

    MgeUiWidget r1 = Mge_UiTableRow();
    Mge_UiAddChild(r1, fixed(10, 12));
    MgeUiWidget widecell = Mge_UiText(0, "WWWW", (MgeTextStyle){ .size = 8 }); // intrinsic column driver
    Mge_UiAddChild(r1, widecell);
    Mge_UiAddChild(r1, fixed(10, 30)); // sets row height
    Mge_UiAddChild(t, r1);

    Mge_UiAddChild(box, t);
    MgeUiWidget root = wrap(box, 800, 600);

    float intrinsicW = Mge_MeasureText(Mge_GetDefaultFont(), "WWWW", 8.0f).x;
    // col0 = 40 fixed, col1 = intrinsicW, col2 = 200 - 20 (spacing) - 40 - intrinsicW
    CHECK_F(Mge_UiGetRect(widecell).x, Mge_UiGetRect(r1).x + 40.0f + 10.0f);
    CHECK_F(Mge_UiGetRect(r1).height, 30.0f); // tallest cell
    float col2x = 40.0f + 10.0f + intrinsicW + 10.0f;
    CHECK(Mge_UiChildAt(r1, 2) != 0);
    CHECK_F(Mge_UiGetRect(Mge_UiChildAt(r1, 2)).x, Mge_UiGetRect(r1).x + col2x);
    Mge_UiDestroy(root);
}

TEST(intrinsic_width_sizes_to_the_widest_child)
{
    Font f = Mge_GetDefaultFont();
    MgeUiWidget iw = Mge_UiIntrinsicWidth();
    MgeUiWidget col = Mge_UiColumn((MgeFlexStyle){ .crossAxis = MGE_CROSS_STRETCH, .mainSize = MGE_MAIN_SIZE_MIN });
    Mge_UiText(col, "short", (MgeTextStyle){ .size = 16 });
    Mge_UiText(col, "a longer line", (MgeTextStyle){ .size = 16 });
    Mge_UiAddChild(iw, col);
    MgeUiWidget root = wrap(iw, 800, 600);

    CHECK_F(Mge_UiGetRect(iw).width, Mge_MeasureText(f, "a longer line", 16.0f).x);
    Mge_UiDestroy(root);
}

TEST(aspect_ratio_fits_the_box)
{
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 200, .height = 200 });
    MgeUiWidget ar = Mge_UiAspectRatio(2.0f);
    Mge_UiAddChild(ar, Mge_UiContainer((MgeContainerStyle){ 0 }));
    Mge_UiAddChild(box, ar);
    MgeUiWidget root = wrap(box, 800, 600);
    CHECK_F(Mge_UiGetRect(ar).width, 200.0f);
    CHECK_F(Mge_UiGetRect(ar).height, 100.0f);
    Mge_UiDestroy(root);
}

TEST(fractionally_sized_box_takes_a_fraction_of_the_box)
{
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 400, .height = 300 });
    MgeUiWidget fb = Mge_UiFractionallySizedBox(0.5f, 0.5f, MGE_ALIGN_CENTER);
    MgeUiWidget inner = Mge_UiContainer((MgeContainerStyle){ 0 });
    Mge_UiAddChild(fb, inner);
    Mge_UiAddChild(box, fb);
    MgeUiWidget root = wrap(box, 800, 600);
    CHECK_F(Mge_UiGetRect(inner).width, 200.0f);
    CHECK_F(Mge_UiGetRect(inner).height, 150.0f);
    Mge_UiDestroy(root);
}

TEST(unconstrained_and_limited_box)
{
    // UnconstrainedBox: an oversized child keeps its size
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 50, .height = 50 });
    MgeUiWidget uc = Mge_UiUnconstrainedBox();
    Mge_UiAddChild(uc, fixed(300, 20));
    Mge_UiAddChild(box, uc);
    MgeUiWidget root = wrap(box, 800, 600);
    CHECK_F(Mge_UiGetRect(Mge_UiChildAt(uc, 0)).width, 300.0f);
    Mge_UiDestroy(root);

    // LimitedBox: caps an axis that arrives unbounded (a Row's main-axis pass),
    // leaves a bounded one alone
    MgeUiWidget rowL = Mge_UiRow((MgeFlexStyle){ .mainSize = MGE_MAIN_SIZE_MIN });
    MgeUiWidget lb = Mge_UiLimitedBox(120, 999);
    MgeUiWidget content = fixed(500, 30);
    Mge_UiAddChild(lb, content);
    Mge_UiAddChild(rowL, lb);
    root = wrap(rowL, 800, 600);
    CHECK_F(Mge_UiGetRect(content).width, 120.0f);  // width arrived unbounded -> capped
    CHECK_F(Mge_UiGetRect(content).height, 30.0f);  // height was bounded -> untouched
    Mge_UiDestroy(root);
}

TEST(indexed_stack_lays_out_all_but_paints_one)
{
    MgeUiWidget is = Mge_UiIndexedStack(1);
    MgeUiWidget a = fixed(40, 40), b = fixed(80, 20), c = fixed(30, 60);
    Mge_UiAddChild(is, a);
    Mge_UiAddChild(is, b);
    Mge_UiAddChild(is, c);
    MgeUiWidget root = wrap(is, 800, 600);
    // stack still sizes to the biggest child on each axis
    CHECK_F(Mge_UiGetRect(is).width, 80.0f);
    CHECK_F(Mge_UiGetRect(is).height, 60.0f);
    // all children laid out (rects set)
    CHECK_F(Mge_UiGetRect(a).width, 40.0f);
    CHECK_F(Mge_UiGetRect(c).height, 60.0f);
    Mge_UiSetStackIndex(is, 2); // no crash; layout unchanged
    render(root, 800, 600);
    CHECK_F(Mge_UiGetRect(is).width, 80.0f);
    Mge_UiDestroy(root);
}

TEST(visibility_maintain_keeps_the_size)
{
    MgeUiWidget row = Mge_UiRow((MgeFlexStyle){ .mainSize = MGE_MAIN_SIZE_MIN });
    MgeUiWidget a = fixed(30, 10);
    MgeUiWidget vm = Mge_UiVisibilityMaintain(false);
    Mge_UiAddChild(vm, fixed(50, 10));
    Mge_UiAddChild(row, a);
    Mge_UiAddChild(row, vm);
    MgeUiWidget root = wrap(row, 800, 600);
    CHECK_F(Mge_UiGetRect(vm).width, 50.0f);   // size kept
    CHECK_F(Mge_UiGetRect(row).width, 80.0f);
    Mge_UiDestroy(root);
}

// ---- Phase 2: scrolling & clipping ----

// a ScrollView of `n` fixed rows in a fixed-size box; returns the root, fills
// `*sv` with the scroll node and `*content` with the inner column.
static MgeUiWidget scroll_tree(int n, float rowH, float boxW, float boxH,
    MgeUiWidget* sv, MgeUiWidget* content)
{
    input_reset();
    MgeUiWidget s = Mge_UiScrollView(MGE_AXIS_VERTICAL, (MgeScrollStyle){ 0 });
    MgeUiWidget col = Mge_UiColumn((MgeFlexStyle){ .mainSize = MGE_MAIN_SIZE_MIN });
    for (int i = 0; i < n; i++)
        Mge_UiAddChild(col, fixed(80, rowH));
    Mge_UiAddChild(s, col);
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = boxW, .height = boxH });
    Mge_UiAddChild(box, s);
    MgeUiWidget root = wrap(box, 800, 600);
    if (sv) *sv = s;
    if (content) *content = col;
    return root;
}

TEST(scrollview_keeps_the_viewport_size_and_reports_scroll_max)
{
    MgeUiWidget sv, col;
    MgeUiWidget root = scroll_tree(10, 40.0f, 100.0f, 150.0f, &sv, &col); // 400 tall content
    CHECK_F(Mge_UiGetRect(sv).height, 150.0f);   // clamped to the viewport
    CHECK_F(Mge_UiGetRect(col).height, 400.0f);  // full content laid out
    CHECK_F(Mge_UiScrollMax(sv), 250.0f);        // 400 - 150
    CHECK_F(Mge_UiScrollOffset(sv), 0.0f);
    Mge_UiDestroy(root);
}

TEST(scroll_to_shifts_content_and_clamps)
{
    MgeUiWidget sv, col;
    MgeUiWidget root = scroll_tree(10, 40.0f, 100.0f, 150.0f, &sv, &col);

    Mge_UiScrollTo(sv, 100.0f);
    render(root, 800, 600);
    CHECK_F(Mge_UiScrollOffset(sv), 100.0f);
    CHECK_F(Mge_UiGetRect(col).y, Mge_UiGetRect(sv).y - 100.0f); // content pulled up

    Mge_UiScrollTo(sv, 1.0e9f);
    render(root, 800, 600);
    CHECK_F(Mge_UiScrollOffset(sv), 250.0f); // clamped to ScrollMax
    Mge_UiDestroy(root);
}

TEST(scroll_to_edge_hits_both_ends)
{
    MgeUiWidget sv, col;
    MgeUiWidget root = scroll_tree(10, 40.0f, 100.0f, 150.0f, &sv, &col);
    Mge_UiScrollToEdge(sv, true);
    render(root, 800, 600);
    CHECK_F(Mge_UiScrollOffset(sv), 250.0f);
    Mge_UiScrollToEdge(sv, false);
    render(root, 800, 600);
    CHECK_F(Mge_UiScrollOffset(sv), 0.0f);
    Mge_UiDestroy(root);
}

TEST(bounded_content_shorter_than_viewport_never_scrolls)
{
    MgeUiWidget sv, col;
    MgeUiWidget root = scroll_tree(1, 30.0f, 100.0f, 200.0f, &sv, &col); // 30 tall content
    CHECK_F(Mge_UiScrollMax(sv), 0.0f);
    Mge_UiScrollTo(sv, 50.0f);
    render(root, 800, 600);
    CHECK_F(Mge_UiScrollOffset(sv), 0.0f);
    Mge_UiDestroy(root);
}

TEST(listview_routes_items_into_its_inner_flex)
{
    input_reset();
    MgeUiWidget lv = Mge_UiListView(MGE_AXIS_VERTICAL, (MgeScrollStyle){ 0 });
    Mge_UiAddChild(lv, fixed(50, 25));
    Mge_UiAddChild(lv, fixed(50, 25));
    Mge_UiAddChild(lv, fixed(50, 25));
    CHECK(Mge_UiChildCount(lv) == 1); // just the internal flex
    MgeUiWidget inner = Mge_UiChildAt(lv, 0);
    CHECK(Mge_UiChildCount(inner) == 3);

    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 80, .height = 40 });
    Mge_UiAddChild(box, lv);
    MgeUiWidget root = wrap(box, 800, 600);
    CHECK_F(Mge_UiScrollMax(lv), 35.0f); // 75 content - 40 view
    Mge_UiDestroy(root);
}

TEST(scroll_to_child_brings_a_far_row_into_view)
{
    input_reset();
    MgeUiWidget lv = Mge_UiListView(MGE_AXIS_VERTICAL, (MgeScrollStyle){ 0 });
    MgeUiWidget rows[40];
    for (int i = 0; i < 40; i++) {
        rows[i] = fixed(80, 20);
        Mge_UiAddChild(lv, rows[i]);
    }
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 100, .height = 100 });
    Mge_UiAddChild(box, lv);
    MgeUiWidget root = wrap(box, 800, 600);
    CHECK_F(Mge_UiScrollMax(lv), 700.0f); // 800 content - 100 view

    Mge_UiScrollToChild(lv, rows[30]);
    render(root, 800, 600);
    float rel = Mge_UiGetRect(rows[30]).y - Mge_UiGetRect(lv).y;
    CHECK(rel >= -0.1f && rel + 20.0f <= 100.0f + 0.1f); // fully visible
    Mge_UiDestroy(root);
}

TEST(clip_rect_lays_out_passthrough)
{
    input_reset();
    MgeUiWidget cr = Mge_UiClipRect();
    Mge_UiAddChild(cr, fixed(120, 45));
    MgeUiWidget root = wrap(cr, 800, 600);
    CHECK_F(Mge_UiGetRect(cr).width, 120.0f);
    CHECK_F(Mge_UiGetRect(cr).height, 45.0f);
    Mge_UiDestroy(root);
}

TEST(wheel_over_a_scroll_view_scrolls_it_and_captures_the_pointer)
{
    MgeUiWidget sv, col;
    MgeUiWidget root = scroll_tree(10, 40.0f, 100.0f, 150.0f, &sv, &col);

    g_mouse = (Vector2){ 10.0f, 10.0f }; // inside the box (top-left aligned root)
    g_wheel = (Vector2){ 0.0f, -1.0f };  // one notch toward the content end
    render(root, 800, 600);
    CHECK(Mge_UiWantsPointer());
    float off = Mge_UiScrollOffset(sv);
    CHECK(off > 0.0f && off <= 60.0f);

    g_wheel = (Vector2){ 0.0f, 0.0f };
    g_mouse = (Vector2){ 400.0f, 400.0f }; // outside
    render(root, 800, 600);
    CHECK(!Mge_UiWantsPointer());
    Mge_UiDestroy(root);
}

TEST(click_drag_scrolls_the_content)
{
    MgeUiWidget sv, col;
    MgeUiWidget root = scroll_tree(10, 40.0f, 100.0f, 150.0f, &sv, &col);

    g_mouse = (Vector2){ 10.0f, 20.0f };
    g_mouseDown = true; // press
    render(root, 800, 600);

    g_mouse = (Vector2){ 10.0f, 5.0f }; // dragged up 15 px
    render(root, 800, 600);
    CHECK_F(Mge_UiScrollOffset(sv), 15.0f);

    g_mouseDown = false; // release
    render(root, 800, 600);
    CHECK_F(Mge_UiScrollOffset(sv), 15.0f); // no fling; stays put
    Mge_UiDestroy(root);
}

// ---- Phase 2b: virtualization & grid ----

struct build_probe {
    int calls;      // total item-builder invocations
    int lo, hi;     // min / max index seen this run
    float rowH;
};

static void probe_reset(struct build_probe* p, float rowH)
{
    p->calls = 0;
    p->lo = 1 << 30;
    p->hi = -1;
    p->rowH = rowH;
}

static MgeUiWidget probe_item(int index, void* user)
{
    struct build_probe* p = user;
    p->calls++;
    if (index < p->lo) p->lo = index;
    if (index > p->hi) p->hi = index;
    return Mge_UiSizedBox(60, p->rowH);
}

// LayoutBuilder callback: record the box width it is handed, add one child that
// fills it.
static float g_lb_seen_w;
static int   g_lb_calls;
static void build_fill(MgeUiWidget slot, MgeUiConstraints c, void* user)
{
    (void)user;
    g_lb_seen_w = c.maxW;
    g_lb_calls++;
    Mge_UiAddChild(slot, Mge_UiSizedBox(c.maxW, 20));
}

TEST(layoutbuilder_builds_with_the_incoming_constraints)
{
    g_lb_seen_w = 0.0f;
    g_lb_calls = 0;
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 240, .height = 80 });
    MgeUiWidget b = Mge_UiLayoutBuilder(build_fill, NULL);
    Mge_UiAddChild(box, b);
    MgeUiWidget root = wrap(box, 800, 600);

    CHECK_F(g_lb_seen_w, 240.0f);
    CHECK_F(Mge_UiGetRect(b).width, 240.0f);
    CHECK_F(Mge_UiGetRect(Mge_UiChildAt(b, 0)).width, 240.0f);

    render(root, 800, 600); // rebuilds every pass
    CHECK(g_lb_calls == 2);
    Mge_UiDestroy(root);
}

TEST(listviewbuilder_builds_only_the_visible_window)
{
    struct build_probe p;
    probe_reset(&p, 20.0f);
    MgeUiWidget lv = Mge_UiListViewBuilder(MGE_AXIS_VERTICAL, 1000, 20.0f, probe_item, &p, (MgeScrollStyle){ 0 });
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 120, .height = 100 });
    Mge_UiAddChild(box, lv);
    MgeUiWidget root = wrap(box, 800, 600);

    CHECK_F(Mge_UiScrollMax(lv), 1000.0f * 20.0f - 100.0f);
    CHECK(p.calls > 0 && p.calls < 20);   // ~5 visible + overscan, not 1000
    CHECK(p.lo == 0);
    Mge_UiDestroy(root);
}

TEST(listviewbuilder_scroll_moves_the_window)
{
    struct build_probe p;
    probe_reset(&p, 20.0f);
    MgeUiWidget lv = Mge_UiListViewBuilder(MGE_AXIS_VERTICAL, 1000, 20.0f, probe_item, &p, (MgeScrollStyle){ 0 });
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 120, .height = 100 });
    Mge_UiAddChild(box, lv);
    MgeUiWidget root = wrap(box, 800, 600);

    Mge_UiScrollTo(lv, 5000.0f); // row 250
    probe_reset(&p, 20.0f);
    render(root, 800, 600);
    CHECK(p.lo <= 250 && p.hi >= 250); // window brackets row 250
    CHECK(p.lo >= 245 && p.hi <= 260); // and only that neighbourhood
    Mge_UiDestroy(root);
}

TEST(scroll_to_index_lands_the_line_at_the_top)
{
    struct build_probe p;
    probe_reset(&p, 25.0f);
    MgeUiWidget lv = Mge_UiListViewBuilder(MGE_AXIS_VERTICAL, 500, 25.0f, probe_item, &p, (MgeScrollStyle){ 0 });
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 120, .height = 100 });
    Mge_UiAddChild(box, lv);
    MgeUiWidget root = wrap(box, 800, 600);

    Mge_UiScrollToIndex(lv, 40);
    render(root, 800, 600);
    CHECK_F(Mge_UiScrollOffset(lv), 40.0f * 25.0f);

    Mge_UiScrollToIndex(lv, 1 << 20); // way past the end
    render(root, 800, 600);
    CHECK_F(Mge_UiScrollOffset(lv), Mge_UiScrollMax(lv));
    Mge_UiDestroy(root);
}

TEST(gridviewbuilder_lays_a_virtualized_grid)
{
    struct build_probe p;
    probe_reset(&p, 40.0f);
    MgeUiWidget gv = Mge_UiGridViewBuilder(MGE_AXIS_VERTICAL, 3, 30, 40.0f, 40.0f, 4.0f, 4.0f,
        probe_item, &p, (MgeScrollStyle){ 0 });
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 140, .height = 100 });
    Mge_UiAddChild(box, gv);
    MgeUiWidget root = wrap(box, 800, 600);

    CHECK_F(Mge_UiScrollMax(gv), (10.0f * 44.0f - 4.0f) - 100.0f); // 10 rows, last has no trailing gap
    CHECK(p.calls > 0 && p.calls < 30);
    // cell index 4 => row 1, col 1
    MgeUiWidget c4 = Mge_UiChildAt(gv, 4);
    CHECK(c4 != 0);
    CHECK_F(Mge_UiGetRect(c4).x - Mge_UiGetRect(gv).x, 44.0f);
    CHECK_F(Mge_UiGetRect(c4).y - Mge_UiGetRect(gv).y, 44.0f);
    Mge_UiDestroy(root);
}

TEST(gridview_non_virtual_sizes_to_its_rows)
{
    MgeUiWidget gv = Mge_UiGridView(MGE_AXIS_VERTICAL, 3, 40.0f, 40.0f, 4.0f, 4.0f);
    for (int k = 0; k < 7; k++)
        Mge_UiAddChild(gv, fixed(10, 10)); // laid out tight to the cell anyway
    MgeUiWidget root = wrap(gv, 800, 600);

    CHECK_F(Mge_UiGetRect(gv).width, 3.0f * 40.0f + 2.0f * 4.0f);   // 3 columns
    CHECK_F(Mge_UiGetRect(gv).height, 3.0f * 40.0f + 2.0f * 4.0f);  // 7 items => 3 rows
    CHECK_F(Mge_UiGetRect(Mge_UiChildAt(gv, 4)).x - Mge_UiGetRect(gv).x, 44.0f);
    CHECK_F(Mge_UiGetRect(Mge_UiChildAt(gv, 4)).y - Mge_UiGetRect(gv).y, 44.0f);
    Mge_UiDestroy(root);
}

TEST(builder_survives_pool_growth_mid_layout)
{
    struct build_probe p;
    probe_reset(&p, 4.0f);
    MgeUiWidget lv = Mge_UiListViewBuilder(MGE_AXIS_VERTICAL, 100000, 4.0f, probe_item, &p, (MgeScrollStyle){ 0 });
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 120, .height = 400 });
    Mge_UiAddChild(box, lv);
    MgeUiWidget root = wrap(box, 800, 600); // ~100 items built per frame, each an alloc

    CHECK_F(Mge_UiScrollMax(lv), 100000.0f * 4.0f - 400.0f);
    Mge_UiScrollToIndex(lv, 50000);
    render(root, 800, 600);
    CHECK_F(Mge_UiScrollOffset(lv), 50000.0f * 4.0f);
    CHECK(Mge_UiChildCount(lv) > 90 && Mge_UiChildCount(lv) < 110);
    Mge_UiDestroy(root);
}

// ---- Phase 3: interaction ----

static int     g_cb_count;
static Vector2 g_cb_total;
static void count_cb(const MgeUiGestureInfo* g, void* user) { (void)g; (void)user; g_cb_count++; }
static void pan_cb(const MgeUiGestureInfo* g, void* user) { (void)user; g_cb_count++; g_cb_total = g->totalDelta; }

static void click_at(MgeUiWidget root, float x, float y)
{
    g_mouse = (Vector2){ x, y };
    g_mouseDown = true;
    render(root, 800, 600); // press
    g_mouseDown = false;
    render(root, 800, 600); // release
}

static Vector2 center_of(MgeUiWidget w)
{
    Rectangle r = Mge_UiGetRect(w);
    return (Vector2){ r.x + r.width * 0.5f, r.y + r.height * 0.5f };
}

TEST(button_click_fires_callback_and_poll)
{
    input_reset();
    g_cb_count = 0;
    MgeUiWidget btn = Mge_UiButton("Play", Mge_UiButtonFilled(Mge_Colors.blue));
    Mge_UiOnPressed(btn, count_cb, NULL);
    MgeUiWidget root = wrap(btn, 800, 600);

    Vector2 c = center_of(btn);
    click_at(root, c.x, c.y);
    CHECK(Mge_UiButtonClicked(btn));
    CHECK(g_cb_count == 1);

    render(root, 800, 600); // idle frame -> latch clears
    CHECK(!Mge_UiButtonClicked(btn));
    Mge_UiDestroy(root);
}

TEST(press_here_release_elsewhere_is_not_a_click)
{
    input_reset();
    g_cb_count = 0;
    MgeUiWidget btn = Mge_UiButton("X", Mge_UiButtonFilled(Mge_Colors.blue));
    Mge_UiOnPressed(btn, count_cb, NULL);
    MgeUiWidget root = wrap(btn, 800, 600);
    Vector2 c = center_of(btn);

    g_mouse = c;
    g_mouseDown = true;
    render(root, 800, 600);
    g_mouse = (Vector2){ 500, 500 }; // dragged off
    render(root, 800, 600);
    g_mouseDown = false;
    render(root, 800, 600);
    CHECK(!Mge_UiButtonClicked(btn));
    CHECK(g_cb_count == 0);
    Mge_UiDestroy(root);
}

TEST(disabled_button_ignores_clicks)
{
    input_reset();
    g_cb_count = 0;
    MgeUiWidget btn = Mge_UiButton("No", Mge_UiButtonFilled(Mge_Colors.blue));
    Mge_UiOnPressed(btn, count_cb, NULL);
    Mge_UiSetEnabled(btn, false);
    MgeUiWidget root = wrap(btn, 800, 600);
    Vector2 c = center_of(btn);
    click_at(root, c.x, c.y);
    CHECK(!Mge_UiButtonClicked(btn));
    CHECK(g_cb_count == 0);
    CHECK(!Mge_UiWantsPointer()); // a disabled widget doesn't grab the pointer
    Mge_UiDestroy(root);
}

TEST(checkbox_toggles_the_bound_bool)
{
    input_reset();
    bool v = false;
    MgeUiWidget cb = Mge_UiCheckbox(&v, Mge_Colors.green);
    MgeUiWidget root = wrap(cb, 800, 600);
    Vector2 c = center_of(cb);

    click_at(root, c.x, c.y);
    CHECK(v == true);
    CHECK(Mge_UiToggleChanged(cb));
    render(root, 800, 600);
    CHECK(!Mge_UiToggleChanged(cb)); // only on the click frame
    click_at(root, c.x, c.y);
    CHECK(v == false);
    Mge_UiDestroy(root);
}

TEST(radio_group_selects_its_value)
{
    input_reset();
    int group = 0;
    MgeUiWidget row = Mge_UiRow((MgeFlexStyle){ .mainSize = MGE_MAIN_SIZE_MIN, .spacing = 8 });
    MgeUiWidget r1 = Mge_UiRadio(&group, 1, Mge_Colors.blue);
    MgeUiWidget r2 = Mge_UiRadio(&group, 2, Mge_Colors.blue);
    Mge_UiAddChild(row, Mge_UiRadio(&group, 0, Mge_Colors.blue));
    Mge_UiAddChild(row, r1);
    Mge_UiAddChild(row, r2);
    MgeUiWidget root = wrap(row, 800, 600);

    Vector2 c = center_of(r2);
    click_at(root, c.x, c.y);
    CHECK(group == 2);
    c = center_of(r1);
    click_at(root, c.x, c.y);
    CHECK(group == 1);
    Mge_UiDestroy(root);
}

TEST(switch_toggles)
{
    input_reset();
    bool on = true;
    MgeUiWidget sw = Mge_UiSwitch(&on, Mge_Colors.blue);
    MgeUiWidget root = wrap(sw, 800, 600);
    Vector2 c = center_of(sw);
    click_at(root, c.x, c.y);
    CHECK(on == false);
    Mge_UiDestroy(root);
}

TEST(slider_maps_the_drag_to_value)
{
    input_reset();
    float v = 0.0f;
    MgeUiWidget sld = Mge_UiSlider(&v, 0.0f, 100.0f, 0.0f);
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 200, .height = 24 });
    Mge_UiAddChild(box, sld);
    MgeUiWidget root = wrap(box, 800, 600);
    Rectangle r = Mge_UiGetRect(sld);
    CHECK_F(r.width, 200.0f);

    g_mouse = (Vector2){ r.x + r.width * 0.25f, r.y + 12 };
    g_mouseDown = true;
    render(root, 800, 600);
    CHECK_F(v, 25.0f);
    g_mouse = (Vector2){ r.x + r.width * 0.75f, r.y + 12 };
    render(root, 800, 600);
    CHECK_F(v, 75.0f);
    CHECK(Mge_UiSliderChanged(sld));
    g_mouseDown = false;
    render(root, 800, 600);
    Mge_UiDestroy(root);
}

TEST(slider_step_quantises)
{
    input_reset();
    float v = 0.0f;
    MgeUiWidget sld = Mge_UiSlider(&v, 0.0f, 100.0f, 25.0f);
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 200, .height = 24 });
    Mge_UiAddChild(box, sld);
    MgeUiWidget root = wrap(box, 800, 600);
    Rectangle r = Mge_UiGetRect(sld);

    g_mouse = (Vector2){ r.x + r.width * 0.6f, r.y + 12 }; // 60 -> nearest 25 -> 50
    g_mouseDown = true;
    render(root, 800, 600);
    CHECK_F(v, 50.0f);
    Mge_UiDestroy(root);
}

TEST(gesture_detector_tap_and_pan)
{
    input_reset();
    g_cb_count = 0;
    g_cb_total = (Vector2){ 0, 0 };
    MgeUiWidget gd = Mge_UiGestureDetector();
    Mge_UiAddChild(gd, fixed(120, 80));
    Mge_UiOnTap(gd, count_cb, NULL);
    Mge_UiOnPanUpdate(gd, pan_cb, NULL);
    MgeUiWidget root = wrap(gd, 800, 600);

    click_at(root, 60, 40); // inside the 120x80
    CHECK(g_cb_count == 1);
    CHECK(Mge_UiTapped(gd));

    g_cb_count = 0;
    g_mouse = (Vector2){ 20, 20 };
    g_mouseDown = true;
    render(root, 800, 600);
    g_mouse = (Vector2){ 70, 40 }; // moved (50,20) -> past the 4px threshold
    render(root, 800, 600);
    g_mouse = (Vector2){ 90, 55 };
    render(root, 800, 600);
    g_mouseDown = false;
    render(root, 800, 600);
    CHECK(g_cb_count >= 2);          // onPanUpdate fired on the move frames
    CHECK(g_cb_total.x > 60.0f);     // 90 - 20
    CHECK(!Mge_UiTapped(gd));        // a pan is not a tap
    Mge_UiDestroy(root);
}

TEST(hover_enter_exit_edges)
{
    input_reset();
    g_cb_count = 0;
    MgeUiWidget btn = Mge_UiButton("H", Mge_UiButtonFilled(Mge_Colors.blue));
    Mge_UiOnHoverEnter(btn, count_cb, NULL);
    Mge_UiOnHoverExit(btn, count_cb, NULL);
    MgeUiWidget root = wrap(btn, 800, 600);
    Vector2 c = center_of(btn);

    g_mouse = c;
    render(root, 800, 600);
    CHECK(g_cb_count == 1); // enter
    CHECK(Mge_UiHovered(btn));

    g_mouse = (Vector2){ 600, 400 };
    render(root, 800, 600);
    CHECK(g_cb_count == 2); // exit
    CHECK(!Mge_UiHovered(btn));
    Mge_UiDestroy(root);
}

TEST(button_in_a_scroll_view_does_not_drag_scroll)
{
    input_reset();
    MgeUiWidget lv = Mge_UiListView(MGE_AXIS_VERTICAL, (MgeScrollStyle){ 0 });
    for (int k = 0; k < 20; k++)
        Mge_UiAddChild(lv, Mge_UiButton("row", Mge_UiButtonFilled(Mge_Colors.blue)));
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 200, .height = 120 });
    Mge_UiAddChild(box, lv);
    MgeUiWidget root = wrap(box, 800, 600);

    MgeUiWidget b0 = Mge_UiChildAt(Mge_UiChildAt(lv, 0), 0); // inner flex -> first button
    Rectangle rb = Mge_UiGetRect(b0);
    g_mouse = (Vector2){ rb.x + 10, rb.y + 8 };
    g_mouseDown = true;
    render(root, 800, 600);
    g_mouse = (Vector2){ rb.x + 10, rb.y + 48 }; // drag down 40 px over the button
    render(root, 800, 600);
    CHECK_F(Mge_UiScrollOffset(lv), 0.0f); // the button owned the press

    g_mouseDown = false;
    render(root, 800, 600);
    g_mouse = (Vector2){ 100, 60 };
    g_wheel = (Vector2){ 0.0f, -1.0f };
    render(root, 800, 600);
    CHECK(Mge_UiScrollOffset(lv) > 0.0f); // wheel still scrolls
    Mge_UiDestroy(root);
}

TEST(progressbar_reports_its_value)
{
    input_reset();
    MgeUiWidget pb = Mge_UiProgressBar(0.25f);
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 200, .height = 20 });
    Mge_UiAddChild(box, pb);
    MgeUiWidget root = wrap(box, 800, 600);
    CHECK_F(Mge_UiGetProgress(pb), 0.25f);
    Mge_UiSetProgress(pb, 0.6f);
    render(root, 800, 600);
    CHECK_F(Mge_UiGetProgress(pb), 0.6f);
    Mge_UiDestroy(root);
}

// ---- Phase 3b: text fields & focus ----

static MgeUiWidget tf_tree(MgeUiTextBuffer* buf, const char* initial, MgeUiWidget* field)
{
    input_reset();
    Mge_UiTextBufferSet(buf, initial);
    MgeUiWidget f = Mge_UiTextField(buf, "type here", (MgeUiTextFieldStyle){ .expand = true });
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 220, .height = 36 });
    Mge_UiAddChild(box, f);
    MgeUiWidget root = wrap(box, 800, 600);
    if (field) *field = f;
    return root;
}

static void key_mod(MgeUiWidget root, int k, bool shift, bool ctrl)
{
    g_keyDown[KEY_LEFT_SHIFT] = shift;
    g_keyDown[KEY_LEFT_CONTROL] = ctrl;
    g_keyEdge[k] = true;
    render(root, 800, 600);
    g_keyEdge[k] = false;
    g_keyDown[KEY_LEFT_SHIFT] = false;
    g_keyDown[KEY_LEFT_CONTROL] = false;
}
static void key(MgeUiWidget root, int k) { key_mod(root, k, false, false); }

TEST(textfield_types_and_reports_changed)
{
    MgeUiTextBuffer buf;
    MgeUiWidget f, root = tf_tree(&buf, "", &f);
    Mge_UiFocus(f);
    push_text("hi");
    render(root, 800, 600);
    CHECK(strcmp(buf.text, "hi") == 0 && buf.len == 2);
    CHECK(Mge_UiTextChanged(f));
    CHECK(Mge_UiWantsKeyboard());
    render(root, 800, 600);
    CHECK(!Mge_UiTextChanged(f));
    Mge_UiDestroy(root);
}

TEST(backspace_and_delete_at_the_caret)
{
    MgeUiTextBuffer buf;
    MgeUiWidget f, root = tf_tree(&buf, "abc", &f);
    Mge_UiFocus(f);
    render(root, 800, 600);
    key(root, KEY_BACKSPACE);
    CHECK(strcmp(buf.text, "ab") == 0);
    key(root, KEY_HOME);
    key(root, KEY_DELETE);
    CHECK(strcmp(buf.text, "b") == 0);
    Mge_UiDestroy(root);
}

TEST(caret_move_then_insert_in_the_middle)
{
    MgeUiTextBuffer buf;
    MgeUiWidget f, root = tf_tree(&buf, "abc", &f);
    Mge_UiFocus(f);
    render(root, 800, 600);
    key(root, KEY_LEFT);
    key(root, KEY_LEFT); // caret at index 1
    push_text("X");
    render(root, 800, 600);
    CHECK(strcmp(buf.text, "aXbc") == 0);
    Mge_UiDestroy(root);
}

TEST(shift_arrow_selects_then_typing_replaces)
{
    MgeUiTextBuffer buf;
    MgeUiWidget f, root = tf_tree(&buf, "abc", &f);
    Mge_UiFocus(f);
    render(root, 800, 600);
    key(root, KEY_END);
    key_mod(root, KEY_LEFT, true, false);
    key_mod(root, KEY_LEFT, true, false); // "bc" selected
    push_text("Z");
    render(root, 800, 600);
    CHECK(strcmp(buf.text, "aZ") == 0);
    Mge_UiDestroy(root);
}

TEST(ctrl_a_then_type_replaces_all)
{
    MgeUiTextBuffer buf;
    MgeUiWidget f, root = tf_tree(&buf, "hello world", &f);
    Mge_UiFocus(f);
    render(root, 800, 600);
    key_mod(root, KEY_A, false, true);
    push_text("!");
    render(root, 800, 600);
    CHECK(strcmp(buf.text, "!") == 0);
    Mge_UiDestroy(root);
}

TEST(clipboard_copy_then_paste)
{
    MgeUiTextBuffer buf;
    MgeUiWidget f, root = tf_tree(&buf, "abc", &f);
    Mge_UiFocus(f);
    render(root, 800, 600);
    key_mod(root, KEY_A, false, true);
    key_mod(root, KEY_C, false, true);
    CHECK(strcmp(g_clip, "abc") == 0);
    key(root, KEY_END);
    key_mod(root, KEY_V, false, true);
    CHECK(strcmp(buf.text, "abcabc") == 0);
    Mge_UiDestroy(root);
}

TEST(enter_latches_submitted_without_a_newline)
{
    MgeUiTextBuffer buf;
    MgeUiWidget f, root = tf_tree(&buf, "go", &f);
    Mge_UiFocus(f);
    render(root, 800, 600);
    key(root, KEY_ENTER);
    CHECK(Mge_UiTextSubmitted(f));
    CHECK(strcmp(buf.text, "go") == 0);
    render(root, 800, 600);
    CHECK(!Mge_UiTextSubmitted(f));
    Mge_UiDestroy(root);
}

TEST(esc_unfocuses_and_wants_keyboard_tracks_focus)
{
    MgeUiTextBuffer buf;
    MgeUiWidget f, root = tf_tree(&buf, "x", &f);
    Mge_UiFocus(f);
    render(root, 800, 600);
    CHECK(Mge_UiWantsKeyboard() && Mge_UiIsFocused(f));
    key(root, KEY_ESCAPE);
    CHECK(!Mge_UiWantsKeyboard() && !Mge_UiIsFocused(f));
    Mge_UiDestroy(root);
}

TEST(tab_cycles_focus_between_two_fields)
{
    input_reset();
    MgeUiTextBuffer b1, b2;
    Mge_UiTextBufferSet(&b1, "one");
    Mge_UiTextBufferSet(&b2, "two");
    MgeUiWidget col = Mge_UiColumn((MgeFlexStyle){ .crossAxis = MGE_CROSS_STRETCH, .spacing = 6, .mainSize = MGE_MAIN_SIZE_MIN });
    MgeUiWidget f1 = Mge_UiTextField(&b1, "", (MgeUiTextFieldStyle){ .expand = true });
    MgeUiWidget f2 = Mge_UiTextField(&b2, "", (MgeUiTextFieldStyle){ .expand = true });
    Mge_UiAddChild(col, f1);
    Mge_UiAddChild(col, f2);
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 200, .height = 100 });
    Mge_UiAddChild(box, col);
    MgeUiWidget root = wrap(box, 800, 600);

    Mge_UiFocus(f1);
    render(root, 800, 600);
    CHECK(Mge_UiIsFocused(f1));
    key(root, KEY_TAB);
    CHECK(Mge_UiIsFocused(f2));
    key(root, KEY_TAB);
    CHECK(Mge_UiIsFocused(f1)); // wraps
    key_mod(root, KEY_TAB, true, false);
    CHECK(Mge_UiIsFocused(f2)); // shift+tab
    Mge_UiDestroy(root);
}

TEST(click_positions_the_caret)
{
    MgeUiTextBuffer buf;
    MgeUiWidget f, root = tf_tree(&buf, "hello", &f);
    render(root, 800, 600);
    Rectangle r = Mge_UiGetRect(f);
    click_at(root, r.x + r.width - 4.0f, r.y + r.height * 0.5f); // right edge -> caret at end
    push_text("!");
    render(root, 800, 600);
    CHECK(strcmp(buf.text, "hello!") == 0);
    Mge_UiDestroy(root);

    MgeUiWidget root2 = tf_tree(&buf, "hello", &f);
    render(root2, 800, 600);
    r = Mge_UiGetRect(f);
    click_at(root2, r.x + 1.0f, r.y + r.height * 0.5f); // far left -> caret at 0
    push_text("^");
    render(root2, 800, 600);
    CHECK(strcmp(buf.text, "^hello") == 0);
    Mge_UiDestroy(root2);
}

TEST(max_length_caps_input)
{
    input_reset();
    MgeUiTextBuffer buf;
    Mge_UiTextBufferSet(&buf, "");
    MgeUiWidget f = Mge_UiTextField(&buf, "", (MgeUiTextFieldStyle){ .expand = true, .maxLength = 3 });
    MgeUiWidget box = Mge_UiContainer((MgeContainerStyle){ .width = 200, .height = 36 });
    Mge_UiAddChild(box, f);
    MgeUiWidget root = wrap(box, 800, 600);
    Mge_UiFocus(f);
    push_text("abcdef");
    render(root, 800, 600);
    CHECK(strcmp(buf.text, "abc") == 0);
    Mge_UiDestroy(root);
}

int main(void)
{
    MgeGL_Init(800, 600); // the batcher the paint pass feeds

    RUN(root_fills_the_viewport_even_with_a_fixed_size);
    RUN(non_root_container_honours_its_fixed_size);
    RUN(empty_container_shrinks_to_nothing);
    RUN(padding_deflates_child_and_inflates_a_shrink_wrap_parent);
    RUN(alignment_positions_the_child_when_there_is_slack);
    RUN(style_constraints_clamp_an_oversized_child);
    RUN(label_sizes_to_measured_text_and_settext_relayouts);
    RUN(destroy_invalidates_the_handle_and_its_subtree);
    RUN(add_remove_clear_children);

    RUN(row_lays_children_along_x_cross_is_tallest);
    RUN(row_spacing_inserts_a_gap);
    RUN(expanded_takes_the_remaining_main_space);
    RUN(two_expanded_split_by_flex_factor);
    RUN(spacer_pushes_a_trailing_child_to_the_end);
    RUN(main_axis_alignment_positions_children);
    RUN(cross_axis_stretch_fills_the_child);
    RUN(center_and_align_place_the_child);
    RUN(stack_sizes_to_children_and_positions_them);
    RUN(visibility_false_collapses_the_subtree);

    RUN(wrap_flows_children_into_runs);
    RUN(table_resolves_columns_and_lays_rows);
    RUN(intrinsic_width_sizes_to_the_widest_child);
    RUN(aspect_ratio_fits_the_box);
    RUN(fractionally_sized_box_takes_a_fraction_of_the_box);
    RUN(unconstrained_and_limited_box);
    RUN(indexed_stack_lays_out_all_but_paints_one);
    RUN(visibility_maintain_keeps_the_size);

    RUN(scrollview_keeps_the_viewport_size_and_reports_scroll_max);
    RUN(scroll_to_shifts_content_and_clamps);
    RUN(scroll_to_edge_hits_both_ends);
    RUN(bounded_content_shorter_than_viewport_never_scrolls);
    RUN(listview_routes_items_into_its_inner_flex);
    RUN(scroll_to_child_brings_a_far_row_into_view);
    RUN(clip_rect_lays_out_passthrough);
    RUN(wheel_over_a_scroll_view_scrolls_it_and_captures_the_pointer);
    RUN(click_drag_scrolls_the_content);

    RUN(layoutbuilder_builds_with_the_incoming_constraints);
    RUN(listviewbuilder_builds_only_the_visible_window);
    RUN(listviewbuilder_scroll_moves_the_window);
    RUN(scroll_to_index_lands_the_line_at_the_top);
    RUN(gridviewbuilder_lays_a_virtualized_grid);
    RUN(gridview_non_virtual_sizes_to_its_rows);
    RUN(builder_survives_pool_growth_mid_layout);

    RUN(button_click_fires_callback_and_poll);
    RUN(press_here_release_elsewhere_is_not_a_click);
    RUN(disabled_button_ignores_clicks);
    RUN(checkbox_toggles_the_bound_bool);
    RUN(radio_group_selects_its_value);
    RUN(switch_toggles);
    RUN(slider_maps_the_drag_to_value);
    RUN(slider_step_quantises);
    RUN(gesture_detector_tap_and_pan);
    RUN(hover_enter_exit_edges);
    RUN(button_in_a_scroll_view_does_not_drag_scroll);
    RUN(progressbar_reports_its_value);

    RUN(textfield_types_and_reports_changed);
    RUN(backspace_and_delete_at_the_caret);
    RUN(caret_move_then_insert_in_the_middle);
    RUN(shift_arrow_selects_then_typing_replaces);
    RUN(ctrl_a_then_type_replaces_all);
    RUN(clipboard_copy_then_paste);
    RUN(enter_latches_submitted_without_a_newline);
    RUN(esc_unfocuses_and_wants_keyboard_tracks_focus);
    RUN(tab_cycles_focus_between_two_fields);
    RUN(click_positions_the_caret);
    RUN(max_length_caps_input);
    return test_summary();
}
