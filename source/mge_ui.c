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
    NODE_STACK,
    NODE_POSITIONED, // wrapper, child of a Stack
    NODE_VISIBILITY,
};

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
        } stack;
        struct {
            float l, t, r, b, w, h; // MGE_UI_NONE == unset
        } positioned;
        bool visible;
    } v;
} Node;

static struct {
    bool     booted;
    Node*    nodes;
    uint8_t* gen;
    int      count, cap;
    int32_t  freeHead; // -1 = none
    int32_t  root;     // -1 = none
    float    vpW, vpH; // 0 = use the screen size
    bool     dirty;
} S;

// ---- helpers ----------------------------------------------------------

static float clampf(float v, float lo, float hi)
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
    S.dirty = true;
    return h_make(i);
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

MgeUiWidget Mge_UiVisibility(bool visible)
{
    int32_t i = alloc_node(NODE_VISIBILITY);
    S.nodes[i].v.visible = visible;
    S.dirty = true;
    return h_make(i);
}

void Mge_UiSetVisible(MgeUiWidget w, bool visible)
{
    int32_t i = h_index(w);
    if (i < 0 || S.nodes[i].type != NODE_VISIBILITY) return;
    S.nodes[i].v.visible = visible;
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

bool Mge_UiWantsPointer(void)  { return false; } // no interactive widgets yet
bool Mge_UiWantsKeyboard(void) { return false; }

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
        Node* p = &S.nodes[k];
        if (p->type != NODE_POSITIONED) {
            float fx = al.x * 0.5f + 0.5f, fy = al.y * 0.5f + 0.5f;
            p->rect.x = (sw - p->rect.width) * fx;
            p->rect.y = (sh - p->rect.height) * fy;
            continue;
        }
        float L = p->v.positioned.l, R = p->v.positioned.r, W = p->v.positioned.w;
        float T = p->v.positioned.t, B = p->v.positioned.b, H = p->v.positioned.h;
        float cw = (pos_set(L) && pos_set(R)) ? fmaxf(0.0f, sw - L - R) : (pos_set(W) ? W : -1.0f);
        float ch = (pos_set(T) && pos_set(B)) ? fmaxf(0.0f, sh - T - B) : (pos_set(H) ? H : -1.0f);
        MgeUiConstraints pc = {
            cw >= 0.0f ? cw : 0.0f, cw >= 0.0f ? cw : sw,
            ch >= 0.0f ? ch : 0.0f, ch >= 0.0f ? ch : sh,
        };
        MgeUiSize s = layout_node(k, pc);
        p->rect.x = pos_set(L) ? L : (pos_set(R) ? sw - R - s.w : (sw - s.w) * (al.x * 0.5f + 0.5f));
        p->rect.y = pos_set(T) ? T : (pos_set(B) ? sh - B - s.h : (sh - s.h) * (al.y * 0.5f + 0.5f));
    }

    S.nodes[i].rect.width = sw;
    S.nodes[i].rect.height = sh;
    return (MgeUiSize){ sw, sh };
}

static MgeUiSize layout_node(int32_t i, MgeUiConstraints c)
{
    switch (S.nodes[i].type) {
    case NODE_CONTAINER: return layout_container(i, c);
    case NODE_TEXT:      return layout_text(i, c);
    case NODE_FLEX:      return layout_flex(i, c);
    case NODE_STACK:     return layout_stack(i, c);
    case NODE_FLEXIBLE:  return layout_passthrough(i, c);
    case NODE_POSITIONED: return layout_passthrough(i, c);
    case NODE_VISIBILITY:
        if (!S.nodes[i].v.visible) {
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

// ---- paint ---------------------------------------------------------

static void paint_node(int32_t i)
{
    Node* n = &S.nodes[i];

    if (n->type == NODE_VISIBILITY && !n->v.visible)
        return; // hidden subtree

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

    for (int32_t c = n->firstChild; c >= 0; c = S.nodes[c].nextSibling)
        paint_node(c);
}

void Mge_UiRender(void)
{
    if (!S.booted || S.root < 0)
        return;

    float vpW = (S.vpW > 0.0f) ? S.vpW : (float)Mge_GetScreenWidth();
    float vpH = (S.vpH > 0.0f) ? S.vpH : (float)Mge_GetScreenHeight();

    layout_node(S.root, Mge_ConstraintsTight(vpW, vpH));
    place_node(S.root, 0.0f, 0.0f);

    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
    MgeGL_SetBlend(true);
    paint_node(S.root);
    MgeGL_Draw();
    MgeGL_SetBlend(false);

    S.dirty = false;
}
