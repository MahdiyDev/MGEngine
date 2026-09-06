// Retained widget GUI -- Phase 0: a node pool, a box-constraint layout pass, and
// a paint pass that emits Draw_Rectangle* / Draw_Text in 2D screen space. See
// mge_ui.h for the API and the model.

#include "mge_ui.h"

#include "mge_gl.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

enum {
    NODE_FREE = 0,
    NODE_CONTAINER,
    NODE_TEXT,
    NODE_FLEX,       // Row / Column / Flex
    NODE_FLEXIBLE,   // Expanded / Flexible / Spacer wrapper
    NODE_STACK,      // also IndexedStack (index >= 0)
    NODE_POSITIONED, // wrapper, child of a Stack
    NODE_VISIBILITY,
    NODE_WRAP,
    NODE_TABLE,
    NODE_TABLE_ROW,
    NODE_INTRINSIC,  // v.axis: 0 = width, 1 = height
    NODE_ASPECT,
    NODE_FRACTIONAL,
    NODE_UNCONSTRAINED,
    NODE_LIMITED,
    NODE_SCROLL,
    NODE_CLIPRECT,
    NODE_LAYOUTBUILDER,
    NODE_GRIDVIEW,
};

#define MGE_UI_TABLE_MAX_COLS 16

typedef struct Node {
    uint8_t   type;
    int32_t   parent;      // index, or -1
    int32_t   firstChild;  // index, or -1
    int32_t   nextSibling; // index, or -1  (also the free-list link when NODE_FREE)
    Rectangle rect;         // computed by layout/place; a relative offset mid-layout
    union {
        MgeContainerStyle container;
        struct {
            char*        text;
            MgeTextStyle style;
        } text;
        struct {
            MgeAxis      axis;
            MgeFlexStyle style;
        } flex;
        struct {
            int        flex;
            MgeFlexFit fit;
        } flexible;
        struct {
            MgeStackFit  fit;
            MgeAlignment alignment;
            int          index; // -1 = paint all (a plain Stack)
        } stack;
        struct {
            float l, t, r, b, w, h; // MGE_UI_NONE == unset
        } positioned;
        struct {
            bool visible, maintainSize;
        } visibility;
        MgeWrapStyle wrap;
        struct {
            MgeTableColumn cols[MGE_UI_TABLE_MAX_COLS];
            int            nCols;
            float          rowSpacing, colSpacing;
        } table;
        uint8_t axis; // NODE_INTRINSIC: 0 = width, 1 = height
        float   ratio; // NODE_ASPECT
        struct {
            float wf, hf;
            MgeAlignment align;
        } fractional;
        struct {
            float maxW, maxH;
        } limited;
        struct {
            MgeAxis        axis;
            MgeScrollStyle style;
            float          offset;        // scroll position on `axis`, px
            float          contentExtent; // child size on `axis`
            float          viewExtent;    // viewport size on `axis`
            int32_t        innerFlex;     // Mge_UiListView: the flex children route into; else -1
            uint8_t        kind;          // 0 normal | 1 list-builder | 2 grid-builder
            int32_t        itemCount, crossCount;
            float          itemExtent, cellW, cellH, mainGap, crossGap;
            MgeUiItemBuilder build;
            void*          user;
        } scroll;
        struct {
            MgeUiLayoutCallback build;
            void*              user;
        } layoutBuilder;
        struct {
            MgeAxis axis;
            int32_t crossCount;
            float   cellW, cellH, mainGap, crossGap;
        } grid;
    } v;
} Node;

#define MGE_UI_WHEEL_STEP 42.0f
#define MGE_UI_CLIP_MAX   16

static struct {
    bool     booted;
    Node*    nodes;
    uint8_t* gen;
    int      count, cap;
    int32_t  freeHead; // -1 = none
    int32_t  root;     // -1 = none
    float    vpW, vpH; // 0 = use the screen size
    bool     dirty;

    // input (read by Mge_UiNewFrame, consumed by Mge_UiRender's input_update)
    Vector2  mouse, mousePrev;
    float    wheel;
    bool     mouseDown, mousePrevDown;
    int32_t  pointerNode;    // deepest node under the cursor, or -1
    int32_t  activeScroll;   // scroll view being drag-scrolled, or -1
    bool     draggingThumb;  // the active scroll's drag is on its scrollbar thumb
    float    dragAnchorMouse, dragAnchorOffset;
    bool     pointerOverScroll;

    // paint-time scissor stack
    Rectangle clipStack[MGE_UI_CLIP_MAX];
    int       clipTop;
} S;

// ---- helpers ----------------------------------------------------------

static float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int clampi(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static char* dup_str(const char* s)
{
    if (s == NULL) s = "";
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (p != NULL) memcpy(p, s, n);
    return p;
}

static void ensure_boot(void)
{
    if (S.booted) return;
    S.cap = 64;
    S.nodes = (Node*)calloc((size_t)S.cap, sizeof(Node));
    S.gen = (uint8_t*)malloc((size_t)S.cap);
    for (int i = 0; i < S.cap; i++)
        S.gen[i] = 1;
    S.count = 0;
    S.freeHead = -1;
    S.root = -1;
    S.vpW = S.vpH = 0.0f;
    S.dirty = true;
    S.booted = true;
    S.pointerNode = -1;
    S.activeScroll = -1;
}

static int32_t alloc_node(uint8_t type)
{
    ensure_boot();

    int32_t i;
    if (S.freeHead >= 0) {
        i = S.freeHead;
        S.freeHead = S.nodes[i].nextSibling;
    } else {
        if (S.count == S.cap) {
            int newCap = S.cap * 2;
            S.nodes = (Node*)realloc(S.nodes, (size_t)newCap * sizeof(Node));
            S.gen = (uint8_t*)realloc(S.gen, (size_t)newCap);
            for (int k = S.cap; k < newCap; k++) {
                memset(&S.nodes[k], 0, sizeof(Node));
                S.gen[k] = 1;
            }
            S.cap = newCap;
        }
        i = S.count++;
    }

    Node* n = &S.nodes[i];
    memset(n, 0, sizeof(*n));
    n->type = type;
    n->parent = n->firstChild = n->nextSibling = -1;
    return i;
}

// handle = ((index + 1) << 8) | generation ; 0 == null
static MgeUiWidget h_make(int32_t i)
{
    return ((uint32_t)(i + 1) << 8) | (uint32_t)S.gen[i];
}

static int32_t h_index(MgeUiWidget w)
{
    if (w == 0 || !S.booted) return -1;
    int32_t i = (int32_t)(w >> 8) - 1;
    if (i < 0 || i >= S.count) return -1;
    if ((uint32_t)S.gen[i] != (w & 0xFFu)) return -1;
    if (S.nodes[i].type == NODE_FREE) return -1;
    return i;
}

static void detach(int32_t i)
{
    Node* n = &S.nodes[i];
    if (n->parent < 0) return;
    Node* p = &S.nodes[n->parent];
    if (p->firstChild == i) {
        p->firstChild = n->nextSibling;
    } else {
        int32_t c = p->firstChild;
        while (c >= 0 && S.nodes[c].nextSibling != i)
            c = S.nodes[c].nextSibling;
        if (c >= 0) S.nodes[c].nextSibling = n->nextSibling;
    }
    n->parent = -1;
    n->nextSibling = -1;
}

static void free_rec(int32_t i)
{
    Node* n = &S.nodes[i];
    int32_t c = n->firstChild;
    while (c >= 0) {
        int32_t next = S.nodes[c].nextSibling;
        free_rec(c);
        c = next;
    }
    if (n->type == NODE_TEXT)
        free(n->v.text.text);

    S.gen[i]++;            // stale every outstanding handle
    if (S.gen[i] == 0) S.gen[i] = 1;
    n->type = NODE_FREE;
    n->firstChild = n->parent = -1;
    n->nextSibling = S.freeHead;
    S.freeHead = i;
}

// ---- tree -----------------------------------------------------------

void Mge_UiDestroy(MgeUiWidget w)
{
    int32_t i = h_index(w);
    if (i < 0) return;
    if (S.root == i) S.root = -1;
    detach(i);
    free_rec(i);
    S.dirty = true;
}

void Mge_UiAddChild(MgeUiWidget parent, MgeUiWidget child)
{
    int32_t p = h_index(parent), c = h_index(child);
    if (p < 0 || c < 0 || p == c) return;
    // Mge_UiListView: items route into the internal flex, not the scroll node
    if (S.nodes[p].type == NODE_SCROLL && S.nodes[p].v.scroll.innerFlex >= 0)
        p = S.nodes[p].v.scroll.innerFlex;
    detach(c);
    // append to the end of parent's child list
    Node* pn = &S.nodes[p];
    if (pn->firstChild < 0) {
        pn->firstChild = c;
    } else {
        int32_t last = pn->firstChild;
        while (S.nodes[last].nextSibling >= 0)
            last = S.nodes[last].nextSibling;
        S.nodes[last].nextSibling = c;
    }
    S.nodes[c].parent = p;
    S.nodes[c].nextSibling = -1;
    S.dirty = true;
}

void Mge_UiRemoveChild(MgeUiWidget parent, MgeUiWidget child)
{
    int32_t p = h_index(parent), c = h_index(child);
    if (p < 0 || c < 0 || S.nodes[c].parent != p) return;
    detach(c);
    S.dirty = true;
}

void Mge_UiClearChildren(MgeUiWidget parent)
{
    int32_t p = h_index(parent);
    if (p < 0) return;
    int32_t c = S.nodes[p].firstChild;
    S.nodes[p].firstChild = -1;
    while (c >= 0) {
        int32_t next = S.nodes[c].nextSibling;
        S.nodes[c].parent = -1;
        free_rec(c);
        c = next;
    }
    S.dirty = true;
}

int Mge_UiChildCount(MgeUiWidget w)
{
    int32_t i = h_index(w);
    if (i < 0) return 0;
    int n = 0;
    for (int32_t c = S.nodes[i].firstChild; c >= 0; c = S.nodes[c].nextSibling)
        n++;
    return n;
}

MgeUiWidget Mge_UiChildAt(MgeUiWidget w, int index)
{
    int32_t i = h_index(w);
    if (i < 0 || index < 0) return 0;
    int32_t c = S.nodes[i].firstChild;
    while (c >= 0 && index-- > 0)
        c = S.nodes[c].nextSibling;
    return c >= 0 ? h_make(c) : 0;
}

MgeUiWidget Mge_UiParentOf(MgeUiWidget w)
{
    int32_t i = h_index(w);
    if (i < 0 || S.nodes[i].parent < 0) return 0;
    return h_make(S.nodes[i].parent);
}

bool Mge_UiIsValid(MgeUiWidget w) { return h_index(w) >= 0; }

void Mge_UiMarkNeedsBuild(MgeUiWidget w)  { (void)w; S.dirty = true; }
void Mge_UiMarkNeedsLayout(MgeUiWidget w) { (void)w; S.dirty = true; }
void Mge_UiMarkNeedsPaint(MgeUiWidget w)  { (void)w; S.dirty = true; }

// ---- widgets --------------------------------------------------------

MgeUiWidget Mge_UiContainer(MgeContainerStyle style)
{
    int32_t i = alloc_node(NODE_CONTAINER);
    S.nodes[i].v.container = style;
    S.dirty = true;
    return h_make(i);
}

MgeUiWidget Mge_UiText(MgeUiWidget parent, const char* text, MgeTextStyle style)
{
    int32_t i = alloc_node(NODE_TEXT);
    S.nodes[i].v.text.text = dup_str(text);
    S.nodes[i].v.text.style = style;
    MgeUiWidget h = h_make(i);
    if (parent != 0)
        Mge_UiAddChild(parent, h);
    S.dirty = true;
    return h;
}

MgeUiWidget Mge_UiLabel(MgeUiWidget parent, const char* text)
{
    return Mge_UiText(parent, text, (MgeTextStyle){ 0 });
}

void Mge_UiSetText(MgeUiWidget w, const char* text)
{
    int32_t i = h_index(w);
    if (i < 0 || S.nodes[i].type != NODE_TEXT) return;
    free(S.nodes[i].v.text.text);
    S.nodes[i].v.text.text = dup_str(text);
    S.dirty = true;
}

void Mge_UiSetContainerStyle(MgeUiWidget w, MgeContainerStyle style)
{
    int32_t i = h_index(w);
    if (i < 0 || S.nodes[i].type != NODE_CONTAINER) return;
    S.nodes[i].v.container = style;
    S.dirty = true;
}

// ---- layout widgets (Phase 1) --------------------------------------

MgeUiWidget Mge_UiFlex(MgeAxis axis, MgeFlexStyle style)
{
    int32_t i = alloc_node(NODE_FLEX);
    S.nodes[i].v.flex.axis = axis;
    S.nodes[i].v.flex.style = style;
    S.dirty = true;
    return h_make(i);
}

MgeUiWidget Mge_UiRow(MgeFlexStyle style)    { return Mge_UiFlex(MGE_AXIS_HORIZONTAL, style); }
MgeUiWidget Mge_UiColumn(MgeFlexStyle style) { return Mge_UiFlex(MGE_AXIS_VERTICAL, style); }

MgeUiWidget Mge_UiFlexible(int flex, MgeFlexFit fit)
{
    int32_t i = alloc_node(NODE_FLEXIBLE);
    S.nodes[i].v.flexible.flex = flex < 1 ? 1 : flex;
    S.nodes[i].v.flexible.fit = fit;
    S.dirty = true;
    return h_make(i);
}

MgeUiWidget Mge_UiExpanded(int flex) { return Mge_UiFlexible(flex, MGE_FLEX_TIGHT); }
MgeUiWidget Mge_UiSpacer(int flex)   { return Mge_UiFlexible(flex, MGE_FLEX_TIGHT); }

MgeUiWidget Mge_UiSizedBox(float w, float h)
{
    return Mge_UiContainer((MgeContainerStyle){ .width = w, .height = h });
}
MgeUiWidget Mge_UiCenter(void)
{
    return Mge_UiContainer((MgeContainerStyle){ .expand = true });
}
MgeUiWidget Mge_UiAlign(MgeAlignment alignment)
{
    return Mge_UiContainer((MgeContainerStyle){ .expand = true, .alignment = alignment });
}
MgeUiWidget Mge_UiPadding(MgeEdgeInsets insets)
{
    return Mge_UiContainer((MgeContainerStyle){ .padding = insets });
}
MgeUiWidget Mge_UiConstrainedBox(MgeUiConstraints constraints)
{
    return Mge_UiContainer((MgeContainerStyle){ .constraints = constraints });
}

MgeUiWidget Mge_UiStack(MgeStackFit fit, MgeAlignment alignment)
{
    int32_t i = alloc_node(NODE_STACK);
    S.nodes[i].v.stack.fit = fit;
    S.nodes[i].v.stack.alignment = alignment;
    S.nodes[i].v.stack.index = -1; // paint all
    S.dirty = true;
    return h_make(i);
}

MgeUiWidget Mge_UiIndexedStack(int index)
{
    int32_t i = alloc_node(NODE_STACK);
    S.nodes[i].v.stack.fit = MGE_STACK_LOOSE;
    S.nodes[i].v.stack.alignment = (MgeAlignment){ 0, 0 };
    S.nodes[i].v.stack.index = index < 0 ? 0 : index;
    S.dirty = true;
    return h_make(i);
}

void Mge_UiSetStackIndex(MgeUiWidget w, int index)
{
    int32_t i = h_index(w);
    if (i < 0 || S.nodes[i].type != NODE_STACK) return;
    S.nodes[i].v.stack.index = index;
    S.dirty = true;
}

MgeUiWidget Mge_UiPositioned(float left, float top, float right, float bottom,
    float width, float height)
{
    int32_t i = alloc_node(NODE_POSITIONED);
    S.nodes[i].v.positioned.l = left;
    S.nodes[i].v.positioned.t = top;
    S.nodes[i].v.positioned.r = right;
    S.nodes[i].v.positioned.b = bottom;
    S.nodes[i].v.positioned.w = width;
    S.nodes[i].v.positioned.h = height;
    S.dirty = true;
    return h_make(i);
}
MgeUiWidget Mge_UiPositionedFill(void)
{
    return Mge_UiPositioned(0.0f, 0.0f, 0.0f, 0.0f, MGE_UI_NONE, MGE_UI_NONE);
}

static MgeUiWidget make_visibility(bool visible, bool maintainSize)
{
    int32_t i = alloc_node(NODE_VISIBILITY);
    S.nodes[i].v.visibility.visible = visible;
    S.nodes[i].v.visibility.maintainSize = maintainSize;
    S.dirty = true;
    return h_make(i);
}

MgeUiWidget Mge_UiVisibility(bool visible)         { return make_visibility(visible, false); }
MgeUiWidget Mge_UiVisibilityMaintain(bool visible) { return make_visibility(visible, true); }
MgeUiWidget Mge_UiOffstage(bool offstage)          { return make_visibility(!offstage, false); }

void Mge_UiSetVisible(MgeUiWidget w, bool visible)
{
    int32_t i = h_index(w);
    if (i < 0 || S.nodes[i].type != NODE_VISIBILITY) return;
    S.nodes[i].v.visibility.visible = visible;
    S.dirty = true;
}

// ---- layout tail (Phase 1b) --------------------------------------

MgeUiWidget Mge_UiWrap(MgeWrapStyle style)
{
    int32_t i = alloc_node(NODE_WRAP);
    S.nodes[i].v.wrap = style;
    S.dirty = true;
    return h_make(i);
}

MgeUiWidget Mge_UiTable(const MgeTableColumn* cols, int nCols, float rowSpacing, float colSpacing)
{
    int32_t i = alloc_node(NODE_TABLE);
    if (nCols < 1) nCols = 1;
    if (nCols > MGE_UI_TABLE_MAX_COLS) nCols = MGE_UI_TABLE_MAX_COLS;
    S.nodes[i].v.table.nCols = nCols;
    S.nodes[i].v.table.rowSpacing = rowSpacing;
    S.nodes[i].v.table.colSpacing = colSpacing;
    for (int k = 0; k < nCols; k++)
        S.nodes[i].v.table.cols[k] = cols ? cols[k] : (MgeTableColumn){ MGE_COL_FLEX, 1.0f };
    S.dirty = true;
    return h_make(i);
}

MgeUiWidget Mge_UiTableRow(void)
{
    int32_t i = alloc_node(NODE_TABLE_ROW);
    S.dirty = true;
    return h_make(i);
}

static MgeUiWidget make_intrinsic(uint8_t axis)
{
    int32_t i = alloc_node(NODE_INTRINSIC);
    S.nodes[i].v.axis = axis;
    S.dirty = true;
    return h_make(i);
}
MgeUiWidget Mge_UiIntrinsicWidth(void)  { return make_intrinsic(0); }
MgeUiWidget Mge_UiIntrinsicHeight(void) { return make_intrinsic(1); }

MgeUiWidget Mge_UiAspectRatio(float ratio)
{
    int32_t i = alloc_node(NODE_ASPECT);
    S.nodes[i].v.ratio = (ratio > 0.0f) ? ratio : 1.0f;
    S.dirty = true;
    return h_make(i);
}

MgeUiWidget Mge_UiFractionallySizedBox(float wFactor, float hFactor, MgeAlignment align)
{
    int32_t i = alloc_node(NODE_FRACTIONAL);
    S.nodes[i].v.fractional.wf = wFactor;
    S.nodes[i].v.fractional.hf = hFactor;
    S.nodes[i].v.fractional.align = align;
    S.dirty = true;
    return h_make(i);
}

MgeUiWidget Mge_UiUnconstrainedBox(void)
{
    int32_t i = alloc_node(NODE_UNCONSTRAINED);
    S.dirty = true;
    return h_make(i);
}

MgeUiWidget Mge_UiLimitedBox(float maxW, float maxH)
{
    int32_t i = alloc_node(NODE_LIMITED);
    S.nodes[i].v.limited.maxW = maxW;
    S.nodes[i].v.limited.maxH = maxH;
    S.dirty = true;
    return h_make(i);
}

// ---- scrolling & clipping (Phase 2) --------------------------------

MgeUiWidget Mge_UiScrollView(MgeAxis axis, MgeScrollStyle style)
{
    int32_t i = alloc_node(NODE_SCROLL);
    S.nodes[i].v.scroll.axis = axis;
    S.nodes[i].v.scroll.style = style;
    S.nodes[i].v.scroll.innerFlex = -1;
    S.dirty = true;
    return h_make(i);
}

MgeUiWidget Mge_UiListView(MgeAxis axis, MgeScrollStyle style)
{
    MgeUiWidget sv = Mge_UiScrollView(axis, style);
    int32_t si = h_index(sv);
    // an internal flex the ScrollView scrolls; Mge_UiAddChild(listView, ...) redirects here
    MgeUiWidget flex = Mge_UiFlex(axis, (MgeFlexStyle){
        .mainSize = MGE_MAIN_SIZE_MIN, .crossAxis = MGE_CROSS_STRETCH });
    S.nodes[si].firstChild = h_index(flex);
    S.nodes[h_index(flex)].parent = si;
    S.nodes[si].v.scroll.innerFlex = h_index(flex);
    return sv;
}

MgeUiWidget Mge_UiClipRect(void)
{
    int32_t i = alloc_node(NODE_CLIPRECT);
    S.dirty = true;
    return h_make(i);
}

// ---- virtualization & grid (Phase 2b) -----------------------------

MgeUiWidget Mge_UiLayoutBuilder(MgeUiLayoutCallback build, void* user)
{
    int32_t i = alloc_node(NODE_LAYOUTBUILDER);
    S.nodes[i].v.layoutBuilder.build = build;
    S.nodes[i].v.layoutBuilder.user = user;
    S.dirty = true;
    return h_make(i);
}

MgeUiWidget Mge_UiListViewBuilder(MgeAxis axis, int itemCount, float itemExtent,
    MgeUiItemBuilder build, void* user, MgeScrollStyle style)
{
    int32_t i = alloc_node(NODE_SCROLL);
    S.nodes[i].v.scroll.axis = axis;
    S.nodes[i].v.scroll.style = style;
    S.nodes[i].v.scroll.innerFlex = -1; // the builder owns its children directly
    S.nodes[i].v.scroll.kind = 1;
    S.nodes[i].v.scroll.itemCount = itemCount;
    S.nodes[i].v.scroll.itemExtent = itemExtent;
    S.nodes[i].v.scroll.build = build;
    S.nodes[i].v.scroll.user = user;
    S.dirty = true;
    return h_make(i);
}

MgeUiWidget Mge_UiGridViewBuilder(MgeAxis axis, int crossAxisCount, int itemCount,
    float cellW, float cellH, float mainGap, float crossGap,
    MgeUiItemBuilder build, void* user, MgeScrollStyle style)
{
    int32_t i = alloc_node(NODE_SCROLL);
    S.nodes[i].v.scroll.axis = axis;
    S.nodes[i].v.scroll.style = style;
    S.nodes[i].v.scroll.innerFlex = -1;
    S.nodes[i].v.scroll.kind = 2;
    S.nodes[i].v.scroll.itemCount = itemCount;
    S.nodes[i].v.scroll.crossCount = (crossAxisCount > 0) ? crossAxisCount : 1;
    S.nodes[i].v.scroll.cellW = cellW;
    S.nodes[i].v.scroll.cellH = cellH;
    S.nodes[i].v.scroll.mainGap = mainGap;
    S.nodes[i].v.scroll.crossGap = crossGap;
    S.nodes[i].v.scroll.build = build;
    S.nodes[i].v.scroll.user = user;
    S.dirty = true;
    return h_make(i);
}

MgeUiWidget Mge_UiGridView(MgeAxis axis, int crossAxisCount,
    float cellW, float cellH, float mainGap, float crossGap)
{
    int32_t i = alloc_node(NODE_GRIDVIEW);
    S.nodes[i].v.grid.axis = axis;
    S.nodes[i].v.grid.crossCount = (crossAxisCount > 0) ? crossAxisCount : 1;
    S.nodes[i].v.grid.cellW = cellW;
    S.nodes[i].v.grid.cellH = cellH;
    S.nodes[i].v.grid.mainGap = mainGap;
    S.nodes[i].v.grid.crossGap = crossGap;
    S.dirty = true;
    return h_make(i);
}

static int32_t scroll_index(MgeUiWidget w)
{
    int32_t i = h_index(w);
    return (i >= 0 && S.nodes[i].type == NODE_SCROLL) ? i : -1;
}

static float scroll_max_i(int32_t i)
{
    return fmaxf(0.0f, S.nodes[i].v.scroll.contentExtent - S.nodes[i].v.scroll.viewExtent);
}

float Mge_UiScrollOffset(MgeUiWidget w)
{
    int32_t i = scroll_index(w);
    return i >= 0 ? S.nodes[i].v.scroll.offset : 0.0f;
}

float Mge_UiScrollMax(MgeUiWidget w)
{
    int32_t i = scroll_index(w);
    return i >= 0 ? scroll_max_i(i) : 0.0f;
}

void Mge_UiScrollTo(MgeUiWidget w, float px)
{
    int32_t i = scroll_index(w);
    if (i < 0) return;
    S.nodes[i].v.scroll.offset = clampf(px, 0.0f, scroll_max_i(i));
    S.dirty = true;
}

void Mge_UiScrollToEdge(MgeUiWidget w, bool end)
{
    int32_t i = scroll_index(w);
    if (i < 0) return;
    S.nodes[i].v.scroll.offset = end ? scroll_max_i(i) : 0.0f;
    S.dirty = true;
}

void Mge_UiScrollToChild(MgeUiWidget w, MgeUiWidget target)
{
    int32_t i = scroll_index(w), t = h_index(target);
    if (i < 0 || t < 0) return;
    const bool horiz = (S.nodes[i].v.scroll.axis == MGE_AXIS_HORIZONTAL);
    // target rect is absolute after a render; scroll view rect too
    float tPos = (horiz ? S.nodes[t].rect.x : S.nodes[t].rect.y);
    float tExt = (horiz ? S.nodes[t].rect.width : S.nodes[t].rect.height);
    float vPos = (horiz ? S.nodes[i].rect.x : S.nodes[i].rect.y);
    float view = S.nodes[i].v.scroll.viewExtent;
    float rel = (tPos - vPos) + S.nodes[i].v.scroll.offset; // position within content
    float off = S.nodes[i].v.scroll.offset;
    if (rel < off) off = rel;
    else if (rel + tExt > off + view) off = rel + tExt - view;
    S.nodes[i].v.scroll.offset = clampf(off, 0.0f, scroll_max_i(i));
    S.dirty = true;
}

void Mge_UiScrollToIndex(MgeUiWidget w, int index)
{
    int32_t i = scroll_index(w);
    if (i < 0) return;
    const bool horiz = (S.nodes[i].v.scroll.axis == MGE_AXIS_HORIZONTAL);
    int line = (S.nodes[i].v.scroll.kind == 2 && S.nodes[i].v.scroll.crossCount > 0)
        ? index / S.nodes[i].v.scroll.crossCount
        : index;
    float lineExtent = (S.nodes[i].v.scroll.kind == 2)
        ? (horiz ? S.nodes[i].v.scroll.cellW : S.nodes[i].v.scroll.cellH) + S.nodes[i].v.scroll.mainGap
        : S.nodes[i].v.scroll.itemExtent;
    S.nodes[i].v.scroll.offset = clampf((float)line * lineExtent, 0.0f, scroll_max_i(i));
    S.dirty = true;
}

Rectangle Mge_UiGetRect(MgeUiWidget w)
{
    int32_t i = h_index(w);
    return i >= 0 ? S.nodes[i].rect : (Rectangle){ 0 };
}

// ---- lifecycle -----------------------------------------------------

void Mge_UiNewFrame(float dt)
{
    (void)dt; // animation lands in a later phase
    ensure_boot();

    S.mousePrev = S.mouse;
    S.mousePrevDown = S.mouseDown;
    S.mouse = GetMousePosition();
    S.wheel = GetMouseWheelMoveV().y;
    S.mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
}

void Mge_UiSetRoot(MgeUiWidget root)
{
    ensure_boot();
    S.root = h_index(root); // -1 when root == 0 / stale
    S.dirty = true;
}

void Mge_UiViewport(float w, float h)
{
    S.vpW = w;
    S.vpH = h;
    S.dirty = true;
}

bool Mge_UiWantsPointer(void)  { return S.pointerOverScroll || S.activeScroll >= 0; }
bool Mge_UiWantsKeyboard(void) { return false; } // Phase 3

void Mge_UiShutdown(void)
{
    if (!S.booted) return;
    for (int i = 0; i < S.count; i++)
        if (S.nodes[i].type == NODE_TEXT)
            free(S.nodes[i].v.text.text);
    free(S.nodes);
    free(S.gen);
    memset(&S, 0, sizeof(S));
}

// ---- layout (box constraints: constraints down, sizes up) -----------
//
// Re-entrancy invariant: every layout_* reaches nodes by index only -- never
// hold a `Node*` (or `&S.nodes[x]`) across a layout_node() call. layout_node may
// grow the pool (alloc_node -> realloc), and a builder (NODE_SCROLL kind != 0 /
// NODE_LAYOUTBUILDER) frees + rebuilds its own subtree mid-pass. Indices stay
// valid across both; a builder only ever touches nodes at or below itself.

static MgeUiSize layout_node(int32_t i, MgeUiConstraints c);

static MgeUiSize layout_container(int32_t i, MgeUiConstraints c)
{
    const MgeContainerStyle st = S.nodes[i].v.container;
    const float padH = st.padding.left + st.padding.right;
    const float padV = st.padding.top + st.padding.bottom;

    // fold the style's own constraints + explicit size into the incoming ones
    if (st.constraints.minW > 0.0f) c.minW = fmaxf(c.minW, st.constraints.minW);
    if (st.constraints.maxW > 0.0f) c.maxW = fminf(c.maxW, st.constraints.maxW);
    if (st.constraints.minH > 0.0f) c.minH = fmaxf(c.minH, st.constraints.minH);
    if (st.constraints.maxH > 0.0f) c.maxH = fminf(c.maxH, st.constraints.maxH);
    if (st.width > 0.0f)  { float w = clampf(st.width, c.minW, c.maxW);  c.minW = c.maxW = w; }
    if (st.height > 0.0f) { float h = clampf(st.height, c.minH, c.maxH); c.minH = c.maxH = h; }
    // Center / Align: fill the available box instead of shrink-wrapping
    if (st.expand) {
        if (st.width <= 0.0f && c.maxW < MGE_UI_INF) c.minW = c.maxW;
        if (st.height <= 0.0f && c.maxH < MGE_UI_INF) c.minH = c.maxH;
    }

    // the child gets a loosened, padding-deflated box
    MgeUiConstraints inner = {
        0.0f, fmaxf(0.0f, c.maxW - padH),
        0.0f, fmaxf(0.0f, c.maxH - padV),
    };

    float contentW = 0.0f, contentH = 0.0f;
    int32_t child = S.nodes[i].firstChild;
    if (child >= 0) {
        MgeUiSize cs = layout_node(child, inner);
        contentW = cs.w;
        contentH = cs.h;
    }

    float w = clampf(contentW + padH, c.minW, c.maxW);
    float h = clampf(contentH + padV, c.minH, c.maxH);
    S.nodes[i].rect.width = w;
    S.nodes[i].rect.height = h;

    // position the child in the content box by alignment (only matters when
    // the container ended up bigger than shrink-wrap)
    if (child >= 0) {
        float cw = fmaxf(0.0f, w - padH);
        float ch = fmaxf(0.0f, h - padV);
        float fx = st.alignment.x * 0.5f + 0.5f;
        float fy = st.alignment.y * 0.5f + 0.5f;
        S.nodes[child].rect.x = st.padding.left + (cw - S.nodes[child].rect.width) * fx;
        S.nodes[child].rect.y = st.padding.top + (ch - S.nodes[child].rect.height) * fy;
    }

    return (MgeUiSize){ w, h };
}

static MgeUiSize layout_text(int32_t i, MgeUiConstraints c)
{
    const MgeTextStyle st = S.nodes[i].v.text.style;
    Font f = (st.font.glyphs != NULL) ? st.font : Mge_GetDefaultFont();
    float size = (st.size > 0.0f) ? st.size : 16.0f;
    Vector2 m = Mge_MeasureText(f, S.nodes[i].v.text.text, size);
    float w = clampf(m.x, c.minW, c.maxW);
    float h = clampf(m.y, c.minH, c.maxH);
    S.nodes[i].rect.width = w;
    S.nodes[i].rect.height = h;
    return (MgeUiSize){ w, h };
}

// a wrapper that just sizes to (and positions at the origin of) its one child
static MgeUiSize layout_passthrough(int32_t i, MgeUiConstraints c)
{
    int32_t child = S.nodes[i].firstChild;
    MgeUiSize s = { clampf(0.0f, c.minW, c.maxW), clampf(0.0f, c.minH, c.maxH) };
    if (child >= 0) {
        s = layout_node(child, c);
        S.nodes[child].rect.x = 0.0f;
        S.nodes[child].rect.y = 0.0f;
    }
    S.nodes[i].rect.width = s.w;
    S.nodes[i].rect.height = s.h;
    return s;
}

// Row / Column / Flex: two passes -- size the inflexible children, then hand the
// leftover main space to the Expanded / Flexible ones by flex factor.
static MgeUiSize layout_flex(int32_t i, MgeUiConstraints c)
{
    const MgeAxis ax = S.nodes[i].v.flex.axis;
    const MgeFlexStyle st = S.nodes[i].v.flex.style;
    const int horiz = (ax == MGE_AXIS_HORIZONTAL);

    const float mainMin = horiz ? c.minW : c.minH;
    const float mainMax = horiz ? c.maxW : c.maxH;
    const float crossMin = horiz ? c.minH : c.minW;
    const float crossMax = horiz ? c.maxH : c.maxW;

    int n = 0, totalFlex = 0;
    for (int32_t k = S.nodes[i].firstChild; k >= 0; k = S.nodes[k].nextSibling) {
        n++;
        if (S.nodes[k].type == NODE_FLEXIBLE)
            totalFlex += S.nodes[k].v.flexible.flex;
    }
    const float gapTotal = (n > 1) ? st.spacing * (float)(n - 1) : 0.0f;

    const bool stretch = (st.crossAxis == MGE_CROSS_STRETCH) && (crossMax < MGE_UI_INF);
    const float crossLo = stretch ? crossMax : 0.0f;
    MgeUiConstraints childCross = { crossLo, crossMax, crossLo, crossMax }; // one axis used

    float used = gapTotal, crossSize = 0.0f;

    // pass 1 -- inflexible children
    for (int32_t k = S.nodes[i].firstChild; k >= 0; k = S.nodes[k].nextSibling) {
        if (S.nodes[k].type == NODE_FLEXIBLE) continue;
        MgeUiConstraints cc;
        if (horiz)
            cc = (MgeUiConstraints){ 0.0f, MGE_UI_INF, childCross.minH, childCross.maxH };
        else
            cc = (MgeUiConstraints){ childCross.minW, childCross.maxW, 0.0f, MGE_UI_INF };
        MgeUiSize s = layout_node(k, cc);
        used += horiz ? s.w : s.h;
        crossSize = fmaxf(crossSize, horiz ? s.h : s.w);
    }

    // pass 2 -- flexible children share the remaining main space
    float freeSpace = (mainMax < MGE_UI_INF) ? fmaxf(0.0f, mainMax - used) : 0.0f;
    float per = (totalFlex > 0) ? freeSpace / (float)totalFlex : 0.0f;
    for (int32_t k = S.nodes[i].firstChild; k >= 0; k = S.nodes[k].nextSibling) {
        if (S.nodes[k].type != NODE_FLEXIBLE) continue;
        float ext = per * (float)S.nodes[k].v.flexible.flex;
        float lo = (S.nodes[k].v.flexible.fit == MGE_FLEX_TIGHT) ? ext : 0.0f;
        MgeUiConstraints cc;
        if (horiz)
            cc = (MgeUiConstraints){ lo, ext, childCross.minH, childCross.maxH };
        else
            cc = (MgeUiConstraints){ childCross.minW, childCross.maxW, lo, ext };
        MgeUiSize s = layout_node(k, cc);
        used += horiz ? s.w : s.h;
        crossSize = fmaxf(crossSize, horiz ? s.h : s.w);
    }

    float mainSize = (st.mainSize == MGE_MAIN_SIZE_MAX && mainMax < MGE_UI_INF)
        ? mainMax
        : clampf(used, mainMin, mainMax);
    crossSize = clampf(crossSize, crossMin, crossMax);

    // pass 3 -- position
    float leftover = fmaxf(0.0f, mainSize - used);
    float first = 0.0f, gap = st.spacing;
    switch (st.mainAxis) {
    case MGE_MAIN_END:           first = leftover; break;
    case MGE_MAIN_CENTER:        first = leftover * 0.5f; break;
    case MGE_MAIN_SPACE_BETWEEN: if (n > 1) gap += leftover / (float)(n - 1); break;
    case MGE_MAIN_SPACE_AROUND:  if (n > 0) { float e = leftover / (float)n; first = e * 0.5f; gap += e; } break;
    case MGE_MAIN_SPACE_EVENLY:  { float e = leftover / (float)(n + 1); first = e; gap += e; } break;
    default: break; // MGE_MAIN_START
    }

    float mainPos = first;
    for (int32_t k = S.nodes[i].firstChild; k >= 0; k = S.nodes[k].nextSibling) {
        float cm = horiz ? S.nodes[k].rect.width : S.nodes[k].rect.height;
        float cc = horiz ? S.nodes[k].rect.height : S.nodes[k].rect.width;
        float crossPos;
        switch (st.crossAxis) {
        case MGE_CROSS_END:    crossPos = crossSize - cc; break;
        case MGE_CROSS_CENTER: crossPos = (crossSize - cc) * 0.5f; break;
        default:              crossPos = 0.0f; break; // START / STRETCH
        }
        S.nodes[k].rect.x = horiz ? mainPos : crossPos;
        S.nodes[k].rect.y = horiz ? crossPos : mainPos;
        mainPos += cm + gap;
    }

    MgeUiSize out = { horiz ? mainSize : crossSize, horiz ? crossSize : mainSize };
    S.nodes[i].rect.width = out.w;
    S.nodes[i].rect.height = out.h;
    return out;
}

static bool pos_set(float v) { return v < MGE_UI_INF; }

// Stack: non-positioned children pile up aligned to `alignment` and set the
// stack's size; Positioned children are placed against that size.
static MgeUiSize layout_stack(int32_t i, MgeUiConstraints c)
{
    const MgeStackFit fit = S.nodes[i].v.stack.fit;
    const MgeAlignment al = S.nodes[i].v.stack.alignment;

    MgeUiConstraints nonPos = (fit == MGE_STACK_EXPAND)
        ? (MgeUiConstraints){ c.maxW, c.maxW, c.maxH, c.maxH }
        : (MgeUiConstraints){ 0.0f, c.maxW, 0.0f, c.maxH };

    float sw = 0.0f, sh = 0.0f;
    for (int32_t k = S.nodes[i].firstChild; k >= 0; k = S.nodes[k].nextSibling) {
        if (S.nodes[k].type == NODE_POSITIONED) continue;
        MgeUiSize s = layout_node(k, nonPos);
        sw = fmaxf(sw, s.w);
        sh = fmaxf(sh, s.h);
    }
    sw = clampf(sw, c.minW, c.maxW);
    sh = clampf(sh, c.minH, c.maxH);

    for (int32_t k = S.nodes[i].firstChild; k >= 0; k = S.nodes[k].nextSibling) {
        if (S.nodes[k].type != NODE_POSITIONED) {
            float fx = al.x * 0.5f + 0.5f, fy = al.y * 0.5f + 0.5f;
            S.nodes[k].rect.x = (sw - S.nodes[k].rect.width) * fx;
            S.nodes[k].rect.y = (sh - S.nodes[k].rect.height) * fy;
            continue;
        }
        const float L = S.nodes[k].v.positioned.l, R = S.nodes[k].v.positioned.r, W = S.nodes[k].v.positioned.w;
        const float T = S.nodes[k].v.positioned.t, B = S.nodes[k].v.positioned.b, H = S.nodes[k].v.positioned.h;
        float cw = (pos_set(L) && pos_set(R)) ? fmaxf(0.0f, sw - L - R) : (pos_set(W) ? W : -1.0f);
        float ch = (pos_set(T) && pos_set(B)) ? fmaxf(0.0f, sh - T - B) : (pos_set(H) ? H : -1.0f);
        MgeUiConstraints pc = {
            cw >= 0.0f ? cw : 0.0f, cw >= 0.0f ? cw : sw,
            ch >= 0.0f ? ch : 0.0f, ch >= 0.0f ? ch : sh,
        };
        MgeUiSize s = layout_node(k, pc); // may realloc the pool -- re-index below
        S.nodes[k].rect.x = pos_set(L) ? L : (pos_set(R) ? sw - R - s.w : (sw - s.w) * (al.x * 0.5f + 0.5f));
        S.nodes[k].rect.y = pos_set(T) ? T : (pos_set(B) ? sh - B - s.h : (sh - s.h) * (al.y * 0.5f + 0.5f));
    }

    S.nodes[i].rect.width = sw;
    S.nodes[i].rect.height = sh;
    return (MgeUiSize){ sw, sh };
}

// ---- Phase 1b layout ----------------------------------------------

static float wrap_lead(MgeWrapAlignment a, float leftover, int n, float* gapAdd)
{
    *gapAdd = 0.0f;
    if (n <= 0) return 0.0f;
    switch (a) {
    case MGE_WRAP_END:           return leftover;
    case MGE_WRAP_CENTER:        return leftover * 0.5f;
    case MGE_WRAP_SPACE_BETWEEN: if (n > 1) *gapAdd = leftover / (float)(n - 1); return 0.0f;
    case MGE_WRAP_SPACE_AROUND:  { float e = leftover / (float)n; *gapAdd = e; return e * 0.5f; }
    case MGE_WRAP_SPACE_EVENLY:  { float e = leftover / (float)(n + 1); *gapAdd = e; return e; }
    default:                     return 0.0f; // START
    }
}

// Wrap: children flow along `axis`; a child that won't fit starts a new run.
static MgeUiSize layout_wrap(int32_t i, MgeUiConstraints c)
{
    const MgeWrapStyle st = S.nodes[i].v.wrap;
    const int horiz = (st.axis == MGE_AXIS_HORIZONTAL);
    const float mainMax = (horiz ? c.maxW : c.maxH);
    const float crossMax = (horiz ? c.maxH : c.maxW);

    // measure every child (loose), then walk them into runs
    int32_t kids[256];
    int nKids = 0;
    for (int32_t k = S.nodes[i].firstChild; k >= 0 && nKids < 256; k = S.nodes[k].nextSibling) {
        layout_node(k, (MgeUiConstraints){ 0.0f, mainMax, 0.0f, crossMax });
        kids[nKids++] = k;
    }

    float totalMain = 0.0f, totalCross = 0.0f;
    int idx = 0;
    while (idx < nKids) {
        // fill a run
        int start = idx;
        float runMain = 0.0f, runCross = 0.0f;
        while (idx < nKids) {
            float m = horiz ? S.nodes[kids[idx]].rect.width : S.nodes[kids[idx]].rect.height;
            float add = (idx > start ? st.spacing : 0.0f) + m;
            if (idx > start && runMain + add > mainMax) break;
            runMain += add;
            runCross = fmaxf(runCross, horiz ? S.nodes[kids[idx]].rect.height : S.nodes[kids[idx]].rect.width);
            idx++;
        }
        int runN = idx - start;
        float gapAdd, lead = wrap_lead(st.alignment, fmaxf(0.0f, mainMax - runMain), runN, &gapAdd);
        // only distribute along a bounded main axis
        if (mainMax >= MGE_UI_INF) { lead = 0.0f; gapAdd = 0.0f; }
        float pos = lead;
        for (int j = start; j < idx; j++) {
            float m = horiz ? S.nodes[kids[j]].rect.width : S.nodes[kids[j]].rect.height;
            float csz = horiz ? S.nodes[kids[j]].rect.height : S.nodes[kids[j]].rect.width;
            float crossPos = totalCross + (runCross - csz) * 0.5f;
            S.nodes[kids[j]].rect.x = horiz ? pos : crossPos;
            S.nodes[kids[j]].rect.y = horiz ? crossPos : pos;
            pos += m + st.spacing + gapAdd;
        }
        totalMain = fmaxf(totalMain, runMain);
        totalCross += runCross + st.runSpacing;
    }
    if (idx > 0) totalCross -= st.runSpacing; // trailing gap

    float w = clampf(horiz ? totalMain : totalCross, c.minW, c.maxW);
    float h = clampf(horiz ? totalCross : totalMain, c.minH, c.maxH);
    S.nodes[i].rect.width = w;
    S.nodes[i].rect.height = h;
    return (MgeUiSize){ w, h };
}

// Table: resolve column widths, then lay each row's cells at those widths.
static MgeUiSize layout_table(int32_t i, MgeUiConstraints c)
{
    const int n = S.nodes[i].v.table.nCols;
    const float rowSp = S.nodes[i].v.table.rowSpacing, colSp = S.nodes[i].v.table.colSpacing;
    MgeTableColumn cols[MGE_UI_TABLE_MAX_COLS];
    memcpy(cols, S.nodes[i].v.table.cols, sizeof(cols));

    float avail = fmaxf(0.0f, c.maxW - colSp * (float)(n - 1));
    float colW[MGE_UI_TABLE_MAX_COLS] = { 0 };
    float fixedSum = 0.0f, flexSum = 0.0f;
    for (int k = 0; k < n; k++) {
        if (cols[k].mode == MGE_COL_FIXED) { colW[k] = cols[k].value; fixedSum += cols[k].value; }
        else if (cols[k].mode == MGE_COL_FLEX) flexSum += fmaxf(0.0f, cols[k].value);
        else { // INTRINSIC -- widest dry cell in this column
            float mx = 0.0f;
            for (int32_t r = S.nodes[i].firstChild; r >= 0; r = S.nodes[r].nextSibling) {
                if (S.nodes[r].type != NODE_TABLE_ROW) continue;
                int32_t cell = S.nodes[r].firstChild;
                for (int cc = 0; cc < k && cell >= 0; cc++) cell = S.nodes[cell].nextSibling;
                if (cell >= 0) mx = fmaxf(mx, layout_node(cell, Mge_ConstraintsUnbounded()).w);
            }
            colW[k] = mx;
            fixedSum += mx;
        }
    }
    float flexSpace = fmaxf(0.0f, avail - fixedSum);
    for (int k = 0; k < n; k++)
        if (cols[k].mode == MGE_COL_FLEX)
            colW[k] = (flexSum > 0.0f) ? flexSpace * fmaxf(0.0f, cols[k].value) / flexSum : 0.0f;

    float tableW = colSp * (float)(n - 1);
    for (int k = 0; k < n; k++) tableW += colW[k];

    float y = 0.0f;
    for (int32_t r = S.nodes[i].firstChild; r >= 0; r = S.nodes[r].nextSibling) {
        if (S.nodes[r].type != NODE_TABLE_ROW) continue;
        float rowH = 0.0f, x = 0.0f;
        int k = 0;
        for (int32_t cell = S.nodes[r].firstChild; cell >= 0 && k < n; cell = S.nodes[cell].nextSibling, k++) {
            MgeUiSize s = layout_node(cell, (MgeUiConstraints){ colW[k], colW[k], 0.0f, MGE_UI_INF });
            rowH = fmaxf(rowH, s.h);
        }
        k = 0;
        x = 0.0f;
        for (int32_t cell = S.nodes[r].firstChild; cell >= 0 && k < n; cell = S.nodes[cell].nextSibling, k++) {
            S.nodes[cell].rect.x = x;
            S.nodes[cell].rect.y = (rowH - S.nodes[cell].rect.height) * 0.5f;
            x += colW[k] + colSp;
        }
        S.nodes[r].rect.x = 0.0f;
        S.nodes[r].rect.y = y;
        S.nodes[r].rect.width = tableW;
        S.nodes[r].rect.height = rowH;
        y += rowH + rowSp;
    }
    if (y > 0.0f) y -= rowSp;

    float w = clampf(tableW, c.minW, c.maxW);
    float h = clampf(y, c.minH, c.maxH);
    S.nodes[i].rect.width = w;
    S.nodes[i].rect.height = h;
    return (MgeUiSize){ w, h };
}

// Intrinsic{Width,Height}: dry-measure the child unbounded, then lay it out for
// real tightened to that content size on the chosen axis.
static MgeUiSize layout_intrinsic(int32_t i, MgeUiConstraints c)
{
    int32_t child = S.nodes[i].firstChild;
    if (child < 0) {
        S.nodes[i].rect.width = clampf(0.0f, c.minW, c.maxW);
        S.nodes[i].rect.height = clampf(0.0f, c.minH, c.maxH);
        return (MgeUiSize){ S.nodes[i].rect.width, S.nodes[i].rect.height };
    }
    MgeUiSize dry = layout_node(child, Mge_ConstraintsUnbounded());
    if (S.nodes[i].v.axis == 0) {
        float w = clampf(dry.w, c.minW, c.maxW);
        c.minW = c.maxW = w;
    } else {
        float h = clampf(dry.h, c.minH, c.maxH);
        c.minH = c.maxH = h;
    }
    MgeUiSize s = layout_node(child, c);
    S.nodes[child].rect.x = 0.0f;
    S.nodes[child].rect.y = 0.0f;
    S.nodes[i].rect.width = s.w;
    S.nodes[i].rect.height = s.h;
    return s;
}

static MgeUiSize layout_aspect(int32_t i, MgeUiConstraints c)
{
    float ratio = S.nodes[i].v.ratio;
    // start from the widest allowed, derive height, then fit
    float w = (c.maxW < MGE_UI_INF) ? c.maxW : (c.maxH < MGE_UI_INF ? c.maxH * ratio : 0.0f);
    float h = w / ratio;
    if (c.maxH < MGE_UI_INF && h > c.maxH) { h = c.maxH; w = h * ratio; }
    w = clampf(w, c.minW, c.maxW);
    h = clampf(h, c.minH, c.maxH);
    int32_t child = S.nodes[i].firstChild;
    if (child >= 0) {
        layout_node(child, Mge_ConstraintsTight(w, h));
        S.nodes[child].rect.x = 0.0f;
        S.nodes[child].rect.y = 0.0f;
    }
    S.nodes[i].rect.width = w;
    S.nodes[i].rect.height = h;
    return (MgeUiSize){ w, h };
}

static MgeUiSize layout_fractional(int32_t i, MgeUiConstraints c)
{
    const float wf = S.nodes[i].v.fractional.wf, hf = S.nodes[i].v.fractional.hf;
    const MgeAlignment al = S.nodes[i].v.fractional.align;
    MgeUiConstraints cc = { 0.0f, c.maxW, 0.0f, c.maxH };
    if (wf > 0.0f && c.maxW < MGE_UI_INF) { float w = wf * c.maxW; cc.minW = cc.maxW = w; }
    if (hf > 0.0f && c.maxH < MGE_UI_INF) { float h = hf * c.maxH; cc.minH = cc.maxH = h; }

    float ownW = 0.0f, ownH = 0.0f;
    int32_t child = S.nodes[i].firstChild;
    if (child >= 0) {
        MgeUiSize s = layout_node(child, cc);
        ownW = s.w;
        ownH = s.h;
    }
    float w = clampf(ownW, c.minW, c.maxW);
    float h = clampf(ownH, c.minH, c.maxH);
    if (child >= 0) {
        S.nodes[child].rect.x = (w - S.nodes[child].rect.width) * (al.x * 0.5f + 0.5f);
        S.nodes[child].rect.y = (h - S.nodes[child].rect.height) * (al.y * 0.5f + 0.5f);
    }
    S.nodes[i].rect.width = w;
    S.nodes[i].rect.height = h;
    return (MgeUiSize){ w, h };
}

static MgeUiSize layout_unconstrained(int32_t i, MgeUiConstraints c)
{
    int32_t child = S.nodes[i].firstChild;
    MgeUiSize s = { 0, 0 };
    if (child >= 0) {
        s = layout_node(child, Mge_ConstraintsUnbounded());
        S.nodes[child].rect.x = 0.0f;
        S.nodes[child].rect.y = 0.0f;
    }
    float w = clampf(s.w, c.minW, c.maxW);
    float h = clampf(s.h, c.minH, c.maxH);
    S.nodes[i].rect.width = w;
    S.nodes[i].rect.height = h;
    return (MgeUiSize){ w, h };
}

// Mge_UiListViewBuilder / Mge_UiGridViewBuilder: rebuild only the lines that
// intersect the viewport (+1 line overscan) each pass; item `idx` sits at a
// fixed line offset so no off-screen measurement is needed.
static MgeUiSize layout_scroll_builder(int32_t i, MgeUiConstraints c,
    bool horiz, float viewMain, float crossMax)
{
    const int kind = S.nodes[i].v.scroll.kind;
    const int itemCount = S.nodes[i].v.scroll.itemCount;
    const int cols = (kind == 2) ? S.nodes[i].v.scroll.crossCount : 1;
    const float cellMain = (kind == 2)
        ? (horiz ? S.nodes[i].v.scroll.cellW : S.nodes[i].v.scroll.cellH)
        : S.nodes[i].v.scroll.itemExtent;
    const float cellCross = (kind == 2)
        ? (horiz ? S.nodes[i].v.scroll.cellH : S.nodes[i].v.scroll.cellW)
        : crossMax;
    const float mainGap = (kind == 2) ? S.nodes[i].v.scroll.mainGap : 0.0f;
    const float crossGap = (kind == 2) ? S.nodes[i].v.scroll.crossGap : 0.0f;
    const float lineExtent = fmaxf(1.0f, cellMain + mainGap);
    const int lineCount = (cols > 0 && itemCount > 0) ? (itemCount + cols - 1) / cols : 0;

    float contentExtent = (lineCount > 0) ? (float)lineCount * lineExtent - mainGap : 0.0f;
    contentExtent = fmaxf(0.0f, contentExtent);
    S.nodes[i].v.scroll.contentExtent = contentExtent;
    S.nodes[i].v.scroll.viewExtent = viewMain;
    const float offset = clampf(S.nodes[i].v.scroll.offset, 0.0f, fmaxf(0.0f, contentExtent - viewMain));
    S.nodes[i].v.scroll.offset = offset;

    const int first = clampi((int)floorf(offset / lineExtent) - 1, 0, lineCount);
    const int last = clampi((int)ceilf((offset + viewMain) / lineExtent) + 1, 0, lineCount);

    Mge_UiClearChildren(h_make(i)); // drop last frame's window

    MgeUiItemBuilder build = S.nodes[i].v.scroll.build;
    void* user = S.nodes[i].v.scroll.user;
    const MgeUiConstraints cellC = (kind == 2)
        ? Mge_ConstraintsTight(S.nodes[i].v.scroll.cellW, S.nodes[i].v.scroll.cellH)
        : (horiz ? Mge_ConstraintsTight(cellMain, crossMax)
                 : Mge_ConstraintsTight(crossMax, cellMain));

    for (int ln = first; ln < last && build; ln++) {
        for (int col = 0; col < cols; col++) {
            const int idx = ln * cols + col;
            if (idx >= itemCount) break;
            MgeUiWidget w = build(idx, user);
            if (h_index(w) < 0) continue;
            Mge_UiAddChild(h_make(i), w); // may realloc the pool
            int32_t ci = h_index(w);
            if (ci < 0) continue;
            layout_node(ci, cellC); // may realloc -- re-index via h_index next time
            ci = h_index(w);
            if (ci < 0) continue;
            const float mainPos = (float)ln * lineExtent - offset;
            const float crossPos = (float)col * (cellCross + crossGap);
            S.nodes[ci].rect.x = horiz ? mainPos : crossPos;
            S.nodes[ci].rect.y = horiz ? crossPos : mainPos;
        }
    }

    float crossSize = (kind == 2)
        ? (float)cols * cellCross + (float)(cols - 1) * crossGap
        : crossMax;
    crossSize = clampf(crossSize, horiz ? c.minH : c.minW, crossMax);
    const float w = horiz ? viewMain : crossSize;
    const float h = horiz ? crossSize : viewMain;
    S.nodes[i].rect.width = w;
    S.nodes[i].rect.height = h;
    return (MgeUiSize){ w, h };
}

// ScrollView: lay the child out with the scroll axis unbounded, keep the
// viewport size, offset the child by -scroll on that axis.
static MgeUiSize layout_scroll(int32_t i, MgeUiConstraints c)
{
    const bool horiz = (S.nodes[i].v.scroll.axis == MGE_AXIS_HORIZONTAL);
    const float viewMain = horiz ? c.maxW : c.maxH;
    if (viewMain >= MGE_UI_INF) { // no bounded viewport -> can't scroll
        S.nodes[i].v.scroll.contentExtent = 0.0f;
        S.nodes[i].v.scroll.viewExtent = 0.0f;
        return layout_passthrough(i, c);
    }
    const float crossMax = horiz ? c.maxH : c.maxW;

    if (S.nodes[i].v.scroll.kind != 0)
        return layout_scroll_builder(i, c, horiz, viewMain, crossMax);

    MgeUiConstraints cc = horiz
        ? (MgeUiConstraints){ 0.0f, MGE_UI_INF, 0.0f, crossMax }
        : (MgeUiConstraints){ 0.0f, crossMax, 0.0f, MGE_UI_INF };

    float content = 0.0f, childCross = 0.0f;
    int32_t child = S.nodes[i].firstChild;
    if (child >= 0) {
        MgeUiSize s = layout_node(child, cc);
        content = horiz ? s.w : s.h;
        childCross = horiz ? s.h : s.w;
    }
    S.nodes[i].v.scroll.contentExtent = content;
    S.nodes[i].v.scroll.viewExtent = viewMain;
    S.nodes[i].v.scroll.offset = clampf(S.nodes[i].v.scroll.offset, 0.0f, fmaxf(0.0f, content - viewMain));

    if (child >= 0) {
        S.nodes[child].rect.x = horiz ? -S.nodes[i].v.scroll.offset : 0.0f;
        S.nodes[child].rect.y = horiz ? 0.0f : -S.nodes[i].v.scroll.offset;
    }

    float w = horiz ? viewMain : clampf(childCross, c.minW, c.maxW);
    float h = horiz ? clampf(childCross, c.minH, c.maxH) : viewMain;
    S.nodes[i].rect.width = w;
    S.nodes[i].rect.height = h;
    return (MgeUiSize){ w, h };
}

// Non-virtual grid: `crossCount` equal cells per line, lines stack on `axis`.
static MgeUiSize layout_gridview(int32_t i, MgeUiConstraints c)
{
    const bool horiz = (S.nodes[i].v.grid.axis == MGE_AXIS_HORIZONTAL);
    const int cols = S.nodes[i].v.grid.crossCount;
    const float cw = S.nodes[i].v.grid.cellW, ch = S.nodes[i].v.grid.cellH;
    const float mainGap = S.nodes[i].v.grid.mainGap, crossGap = S.nodes[i].v.grid.crossGap;
    const MgeUiConstraints cellC = Mge_ConstraintsTight(cw, ch);
    const float cellMain = horiz ? cw : ch;
    const float cellCross = horiz ? ch : cw;

    int n = 0;
    for (int32_t k = S.nodes[i].firstChild; k >= 0; k = S.nodes[k].nextSibling) {
        layout_node(k, cellC); // may realloc -- k stays valid (index)
        const int ln = n / cols, col = n % cols;
        const float mainPos = (float)ln * (cellMain + mainGap);
        const float crossPos = (float)col * (cellCross + crossGap);
        S.nodes[k].rect.x = horiz ? mainPos : crossPos;
        S.nodes[k].rect.y = horiz ? crossPos : mainPos;
        n++;
    }
    const int rows = (n + cols - 1) / cols;
    const int perRow = (n < cols) ? n : cols;
    const float crossExtent = (perRow > 0)
        ? (float)perRow * cellCross + (float)(perRow - 1) * crossGap
        : 0.0f;
    const float mainExtent = (rows > 0)
        ? (float)rows * cellMain + (float)(rows - 1) * mainGap
        : 0.0f;
    const float w = clampf(horiz ? mainExtent : crossExtent, c.minW, c.maxW);
    const float h = clampf(horiz ? crossExtent : mainExtent, c.minH, c.maxH);
    S.nodes[i].rect.width = w;
    S.nodes[i].rect.height = h;
    return (MgeUiSize){ w, h };
}

// LayoutBuilder: rebuild the subtree with the incoming constraints, then size to
// the (expected single) child like a passthrough.
static MgeUiSize layout_layoutbuilder(int32_t i, MgeUiConstraints c)
{
    MgeUiLayoutCallback build = S.nodes[i].v.layoutBuilder.build;
    void* user = S.nodes[i].v.layoutBuilder.user;
    Mge_UiClearChildren(h_make(i));
    if (build)
        build(h_make(i), c, user); // adds one child; may realloc the pool
    return layout_passthrough(i, c);
}

static MgeUiSize layout_node(int32_t i, MgeUiConstraints c)
{
    switch (S.nodes[i].type) {
    case NODE_CONTAINER: return layout_container(i, c);
    case NODE_TEXT:      return layout_text(i, c);
    case NODE_FLEX:      return layout_flex(i, c);
    case NODE_STACK:     return layout_stack(i, c);
    case NODE_WRAP:      return layout_wrap(i, c);
    case NODE_TABLE:     return layout_table(i, c);
    case NODE_INTRINSIC: return layout_intrinsic(i, c);
    case NODE_ASPECT:    return layout_aspect(i, c);
    case NODE_FRACTIONAL: return layout_fractional(i, c);
    case NODE_UNCONSTRAINED: return layout_unconstrained(i, c);
    case NODE_SCROLL:    return layout_scroll(i, c);
    case NODE_GRIDVIEW:  return layout_gridview(i, c);
    case NODE_LAYOUTBUILDER: return layout_layoutbuilder(i, c);
    case NODE_LIMITED:
        if (c.maxW >= MGE_UI_INF) c.maxW = S.nodes[i].v.limited.maxW;
        if (c.maxH >= MGE_UI_INF) c.maxH = S.nodes[i].v.limited.maxH;
        return layout_passthrough(i, c);
    case NODE_FLEXIBLE:  return layout_passthrough(i, c);
    case NODE_POSITIONED: return layout_passthrough(i, c);
    case NODE_TABLE_ROW: return layout_passthrough(i, c);
    case NODE_CLIPRECT:  return layout_passthrough(i, c);
    case NODE_VISIBILITY:
        if (!S.nodes[i].v.visibility.visible && !S.nodes[i].v.visibility.maintainSize) {
            S.nodes[i].rect = (Rectangle){ 0, 0, 0, 0 };
            return (MgeUiSize){ 0, 0 };
        }
        return layout_passthrough(i, c);
    default: return (MgeUiSize){ 0, 0 };
    }
}

// resolve each node's rect from relative offsets to absolute screen coords
static void place_node(int32_t i, float absX, float absY)
{
    S.nodes[i].rect.x = absX;
    S.nodes[i].rect.y = absY;
    for (int32_t c = S.nodes[i].firstChild; c >= 0; c = S.nodes[c].nextSibling)
        place_node(c, absX + S.nodes[c].rect.x, absY + S.nodes[c].rect.y);
}

// ---- input --------------------------------------------------------

static bool rect_contains(Rectangle r, Vector2 p)
{
    return p.x >= r.x && p.x < r.x + r.width && p.y >= r.y && p.y < r.y + r.height;
}

// deepest node whose absolute rect contains p; later siblings win (painted on
// top). Mirrors paint_node's skip rules and clips at scroll / clip boundaries.
static int32_t hit_test(int32_t i, Vector2 p)
{
    Node* n = &S.nodes[i];

    if (n->type == NODE_VISIBILITY && !n->v.visibility.visible)
        return -1;
    if ((n->type == NODE_SCROLL || n->type == NODE_CLIPRECT) && !rect_contains(n->rect, p))
        return -1;

    if (n->type == NODE_STACK && n->v.stack.index >= 0) {
        int32_t c = n->firstChild;
        for (int k = 0; k < n->v.stack.index && c >= 0; k++)
            c = S.nodes[c].nextSibling;
        int32_t hit = (c >= 0) ? hit_test(c, p) : -1;
        if (hit >= 0) return hit;
        return rect_contains(n->rect, p) ? i : -1;
    }

    int32_t best = -1;
    for (int32_t c = n->firstChild; c >= 0; c = S.nodes[c].nextSibling) {
        int32_t hit = hit_test(c, p);
        if (hit >= 0) best = hit;
    }
    if (best >= 0) return best;
    return rect_contains(n->rect, p) ? i : -1;
}

static int32_t nearest_scroll(int32_t i)
{
    while (i >= 0 && S.nodes[i].type != NODE_SCROLL)
        i = S.nodes[i].parent;
    return i;
}

static float scroll_axis_mouse(int32_t sc)
{
    return (S.nodes[sc].v.scroll.axis == MGE_AXIS_HORIZONTAL) ? S.mouse.x : S.mouse.y;
}

// scrollbar thumb rect in screen coords; false when the view doesn't overflow
// or the style hides the bar.
static bool scrollbar_thumb_rect(int32_t sc, Rectangle* out)
{
    Node* n = &S.nodes[sc];
    float maxOff = scroll_max_i(sc);
    if (n->v.scroll.style.noScrollbar || maxOff <= 0.0f)
        return false;

    const bool horiz = (n->v.scroll.axis == MGE_AXIS_HORIZONTAL);
    float thick = n->v.scroll.style.scrollbarThickness > 0.0f
        ? n->v.scroll.style.scrollbarThickness
        : 6.0f;
    Rectangle r = n->rect;
    float view = n->v.scroll.viewExtent;
    float content = fmaxf(n->v.scroll.contentExtent, 1.0f);
    float trackLen = horiz ? r.width : r.height;
    float thumbLen = clampf(trackLen * (view / content), 24.0f, trackLen);
    float thumbPos = (n->v.scroll.offset / maxOff) * (trackLen - thumbLen);

    if (horiz)
        *out = (Rectangle){ r.x + thumbPos, r.y + r.height - thick, thumbLen, thick };
    else
        *out = (Rectangle){ r.x + r.width - thick, r.y + thumbPos, thick, thumbLen };
    return true;
}

static void scroll_by(int32_t sc, float delta)
{
    if (sc < 0) return;
    S.nodes[sc].v.scroll.offset = clampf(S.nodes[sc].v.scroll.offset + delta, 0.0f, scroll_max_i(sc));
    S.dirty = true;
}

// hit-test + wheel / drag routing; called after place_node (needs absolute rects)
static void input_update(void)
{
    S.pointerNode = (S.root >= 0) ? hit_test(S.root, S.mouse) : -1;
    int32_t target = nearest_scroll(S.pointerNode);
    S.pointerOverScroll = (target >= 0);

    const bool pressed = S.mouseDown && !S.mousePrevDown;
    const bool released = !S.mouseDown && S.mousePrevDown;

    if (target >= 0 && S.wheel != 0.0f)
        scroll_by(target, -S.wheel * MGE_UI_WHEEL_STEP);

    if (pressed && target >= 0) {
        Rectangle thumb;
        S.draggingThumb = scrollbar_thumb_rect(target, &thumb) && rect_contains(thumb, S.mouse);
        S.activeScroll = target;
        S.dragAnchorMouse = scroll_axis_mouse(target);
        S.dragAnchorOffset = S.nodes[target].v.scroll.offset;
    }

    if (S.activeScroll >= 0 && S.mouseDown) {
        int32_t sc = S.activeScroll;
        Node* n = &S.nodes[sc];
        float delta = scroll_axis_mouse(sc) - S.dragAnchorMouse;
        float maxOff = scroll_max_i(sc);
        if (S.draggingThumb) {
            const bool horiz = (n->v.scroll.axis == MGE_AXIS_HORIZONTAL);
            float trackLen = horiz ? n->rect.width : n->rect.height;
            float content = fmaxf(n->v.scroll.contentExtent, 1.0f);
            float thumbLen = clampf(trackLen * (n->v.scroll.viewExtent / content), 24.0f, trackLen);
            float travel = fmaxf(1.0f, trackLen - thumbLen);
            n->v.scroll.offset = clampf(S.dragAnchorOffset + delta / travel * maxOff, 0.0f, maxOff);
        } else {
            // drag the content: pulling the mouse down scrolls back toward the top
            n->v.scroll.offset = clampf(S.dragAnchorOffset - delta, 0.0f, maxOff);
        }
        S.dirty = true;
        S.pointerOverScroll = true;
    }

    if (released) {
        S.activeScroll = -1;
        S.draggingThumb = false;
    }
}

// ---- paint-time scissor stack ------------------------------------

static Rectangle rect_intersect(Rectangle a, Rectangle b)
{
    float x0 = fmaxf(a.x, b.x), y0 = fmaxf(a.y, b.y);
    float x1 = fminf(a.x + a.width, b.x + b.width);
    float y1 = fminf(a.y + a.height, b.y + b.height);
    return (Rectangle){ x0, y0, fmaxf(0.0f, x1 - x0), fmaxf(0.0f, y1 - y0) };
}

static void clip_push(Rectangle r)
{
    if (S.clipTop > 0)
        r = rect_intersect(r, S.clipStack[S.clipTop - 1]);
    if (S.clipTop < MGE_UI_CLIP_MAX)
        S.clipStack[S.clipTop++] = r;
    MgeGL_EnableScissor((int)r.x, (int)r.y, (int)r.width, (int)r.height);
}

static void clip_pop(void)
{
    if (S.clipTop > 0) S.clipTop--;
    if (S.clipTop > 0) {
        Rectangle r = S.clipStack[S.clipTop - 1];
        MgeGL_EnableScissor((int)r.x, (int)r.y, (int)r.width, (int)r.height);
    } else {
        MgeGL_DisableScissor();
    }
}

static void paint_scrollbar(int32_t i)
{
    Node* n = &S.nodes[i];
    Rectangle thumb;
    if (!scrollbar_thumb_rect(i, &thumb))
        return;

    const MgeScrollStyle st = n->v.scroll.style;
    const bool horiz = (n->v.scroll.axis == MGE_AXIS_HORIZONTAL);
    float thick = st.scrollbarThickness > 0.0f ? st.scrollbarThickness : 6.0f;
    Rectangle track = horiz
        ? (Rectangle){ n->rect.x, n->rect.y + n->rect.height - thick, n->rect.width, thick }
        : (Rectangle){ n->rect.x + n->rect.width - thick, n->rect.y, thick, n->rect.height };

    Color thumbC = (st.thumbColor.a != 0) ? st.thumbColor : (Color){ 255, 255, 255, 90 };
    if (st.trackColor.a != 0)
        Draw_RectangleRec(track, st.trackColor);
    Draw_RectangleRounded(thumb, 1.0f, 6, thumbC);
}

// ---- paint ---------------------------------------------------------

static void paint_node(int32_t i)
{
    Node* n = &S.nodes[i];

    if (n->type == NODE_VISIBILITY && !n->v.visibility.visible)
        return; // hidden subtree

    if (n->type == NODE_STACK && n->v.stack.index >= 0) {
        // IndexedStack: paint just the one child
        int32_t c = n->firstChild;
        for (int k = 0; k < n->v.stack.index && c >= 0; k++)
            c = S.nodes[c].nextSibling;
        if (c >= 0) paint_node(c);
        return;
    }

    if (n->type == NODE_CONTAINER) {
        const MgeBoxDecoration d = n->v.container.decoration;
        Rectangle r = n->rect;
        float shorter = fminf(r.width, r.height);
        float rad = fminf(fminf(d.borderRadius.tl, d.borderRadius.tr),
                       fminf(d.borderRadius.br, d.borderRadius.bl));
        float roundness = (shorter > 0.0f && rad > 0.0f) ? clampf(2.0f * rad / shorter, 0.0f, 1.0f) : 0.0f;

        if (d.color.a != 0) {
            if (roundness > 0.0f)
                Draw_RectangleRounded(r, roundness, 8, d.color);
            else
                Draw_RectangleRec(r, d.color);
        }
        if (d.border.width > 0.0f && d.border.color.a != 0) {
            if (roundness > 0.0f)
                Draw_RectangleRoundedLines(r, roundness, 8, d.border.width, d.border.color);
            else
                Draw_RectangleLines((int)r.x, (int)r.y, (int)r.width, (int)r.height, d.border.color);
        }
    } else if (n->type == NODE_TEXT) {
        const MgeTextStyle st = n->v.text.style;
        Font f = (st.font.glyphs != NULL) ? st.font : Mge_GetDefaultFont();
        float size = (st.size > 0.0f) ? st.size : 16.0f;
        Color col = (st.color.a != 0) ? st.color : (Color){ 255, 255, 255, 255 };
        Draw_Text(f, n->v.text.text, (Vector2){ n->rect.x, n->rect.y }, size, col);
    }

    const bool clipping = (n->type == NODE_SCROLL || n->type == NODE_CLIPRECT);
    if (clipping)
        clip_push(n->rect);

    for (int32_t c = n->firstChild; c >= 0; c = S.nodes[c].nextSibling)
        paint_node(c);

    if (clipping)
        clip_pop();
    if (n->type == NODE_SCROLL)
        paint_scrollbar(i); // after clip_pop so the thumb isn't clipped
}

void Mge_UiRender(void)
{
    if (!S.booted || S.root < 0)
        return;

    float vpW = (S.vpW > 0.0f) ? S.vpW : (float)Mge_GetScreenWidth();
    float vpH = (S.vpH > 0.0f) ? S.vpH : (float)Mge_GetScreenHeight();

    layout_node(S.root, Mge_ConstraintsTight(vpW, vpH));
    place_node(S.root, 0.0f, 0.0f);
    input_update();

    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
    MgeGL_SetBlend(true);
    S.clipTop = 0;
    paint_node(S.root);
    MgeGL_Draw();
    MgeGL_DisableScissor(); // safety net if a clip push was left unbalanced
    MgeGL_SetBlend(false);

    S.dirty = false;
}
