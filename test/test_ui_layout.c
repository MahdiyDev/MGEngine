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
    return test_summary();
}
