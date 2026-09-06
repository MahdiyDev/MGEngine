// Retained widget GUI for games -- Flutter's box model (Container / decoration /
// edge insets / constraints down, sizes up), C and handle-based: a constructor
// takes a style struct by value and returns an opaque MgeUiWidget; children are
// attached by passing the parent handle first.
//
//   MgeContainerStyle st = { .decoration = Mge_UiBoxDecoration(Mge_Colors.white),
//                            .padding    = Mge_EdgeInsetsAll(12),
//                            .alignment  = MGE_ALIGN_CENTER };
//   MgeUiWidget card = Mge_UiContainer(st);
//   Mge_UiLabel(card, "Label...");
//   Mge_UiSetRoot(card);
//
// Per frame, in 2D screen space after the scene (a scene module does this in
// MgeScene_DrawGui; the host wraps it):
//
//   Mge_UiNewFrame(dt);
//   ... build / mutate the tree ...
//   Mge_UiRender();
//
// This is a separate system from the Mge_Gui* Dear ImGui shim (mge_gui.h),
// which is editor-facing. Phase 0: Container + Label/Text only.

#pragma once

#include "mge.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- value types ---------------------------------------------------------

#define MGE_UI_INF 1.0e30f // "unbounded" in a constraint

// a named palette so `Mge_Colors.white` works like Dart's Colors.white
static const struct {
    Color white, black, transparent;
    Color red, green, blue, yellow, cyan, magenta;
    Color gray, lightGray, darkGray;
} Mge_Colors = {
    .white = { 255, 255, 255, 255 },
    .black = { 0, 0, 0, 255 },
    .transparent = { 0, 0, 0, 0 },
    .red = { 229, 57, 53, 255 },
    .green = { 67, 160, 71, 255 },
    .blue = { 30, 136, 229, 255 },
    .yellow = { 253, 216, 53, 255 },
    .cyan = { 0, 172, 193, 255 },
    .magenta = { 216, 27, 96, 255 },
    .gray = { 130, 130, 130, 255 },
    .lightGray = { 200, 200, 200, 255 },
    .darkGray = { 66, 66, 66, 255 },
};

typedef struct MgeEdgeInsets {
    float left, top, right, bottom;
} MgeEdgeInsets;

static inline MgeEdgeInsets Mge_EdgeInsetsAll(float v)
{
    return (MgeEdgeInsets){ v, v, v, v };
}
static inline MgeEdgeInsets Mge_EdgeInsetsSymmetric(float horizontal, float vertical)
{
    return (MgeEdgeInsets){ horizontal, vertical, horizontal, vertical };
}
static inline MgeEdgeInsets Mge_EdgeInsetsLTRB(float l, float t, float r, float b)
{
    return (MgeEdgeInsets){ l, t, r, b };
}

// -1..1 on each axis: {-1,-1} = top-left, {0,0} = centre, {1,1} = bottom-right
typedef struct MgeAlignment {
    float x, y;
} MgeAlignment;

static inline MgeAlignment Mge_Alignment(float x, float y) { return (MgeAlignment){ x, y }; }

#define MGE_ALIGN_TOP_LEFT      ((MgeAlignment){ -1.0f, -1.0f })
#define MGE_ALIGN_TOP_CENTER    ((MgeAlignment){ 0.0f, -1.0f })
#define MGE_ALIGN_TOP_RIGHT     ((MgeAlignment){ 1.0f, -1.0f })
#define MGE_ALIGN_CENTER_LEFT   ((MgeAlignment){ -1.0f, 0.0f })
#define MGE_ALIGN_CENTER        ((MgeAlignment){ 0.0f, 0.0f })
#define MGE_ALIGN_CENTER_RIGHT  ((MgeAlignment){ 1.0f, 0.0f })
#define MGE_ALIGN_BOTTOM_LEFT   ((MgeAlignment){ -1.0f, 1.0f })
#define MGE_ALIGN_BOTTOM_CENTER ((MgeAlignment){ 0.0f, 1.0f })
#define MGE_ALIGN_BOTTOM_RIGHT  ((MgeAlignment){ 1.0f, 1.0f })

typedef struct MgeUiSize {
    float w, h;
} MgeUiSize;

// box constraints: a child must size within [min, max] on each axis
typedef struct MgeUiConstraints {
    float minW, maxW, minH, maxH;
} MgeUiConstraints;

static inline MgeUiConstraints Mge_ConstraintsTight(float w, float h)
{
    return (MgeUiConstraints){ w, w, h, h };
}
static inline MgeUiConstraints Mge_ConstraintsLoose(float maxW, float maxH)
{
    return (MgeUiConstraints){ 0.0f, maxW, 0.0f, maxH };
}
static inline MgeUiConstraints Mge_ConstraintsUnbounded(void)
{
    return (MgeUiConstraints){ 0.0f, MGE_UI_INF, 0.0f, MGE_UI_INF };
}

typedef struct MgeBorder {
    Color color;
    float width;
} MgeBorder;

static inline MgeBorder Mge_BorderAll(Color color, float width)
{
    return (MgeBorder){ color, width };
}

typedef struct MgeBorderRadius {
    float tl, tr, br, bl;
} MgeBorderRadius;

static inline MgeBorderRadius Mge_BorderRadiusAll(float r)
{
    return (MgeBorderRadius){ r, r, r, r };
}

typedef struct MgeBoxDecoration {
    Color color;                 // fill; a == 0 => no fill
    MgeBorder border;            // width == 0 => no border
    MgeBorderRadius borderRadius; // 0 => square corners
} MgeBoxDecoration;

static inline MgeBoxDecoration Mge_UiBoxDecoration(Color fill)
{
    return (MgeBoxDecoration){ .color = fill };
}

typedef struct MgeTextStyle {
    Font  font;   // glyphs == NULL => Mge_GetDefaultFont()
    float size;   // <= 0 => 16
    Color color;  // a == 0 => opaque white
} MgeTextStyle;

// Container: the swiss-army box -- padding + margin + alignment + explicit
// size + constraints + decoration around a single optional child.
typedef struct MgeContainerStyle {
    MgeEdgeInsets    padding;
    MgeEdgeInsets    margin;
    MgeAlignment     alignment;   // how the child sits in the content box
    float            width;       // 0 => size to child
    float            height;      // 0 => size to child
    MgeUiConstraints constraints; // all 0 => unconstrained
    MgeBoxDecoration decoration;
    bool             expand;      // fill the max constraints instead of shrink-wrapping
} MgeContainerStyle;

// ---- flex / stack ---------------------------------------------------------

typedef enum { MGE_AXIS_HORIZONTAL = 0, MGE_AXIS_VERTICAL } MgeAxis;

// distribution of children along the main axis (Row: x, Column: y)
typedef enum {
    MGE_MAIN_START = 0,
    MGE_MAIN_END,
    MGE_MAIN_CENTER,
    MGE_MAIN_SPACE_BETWEEN,
    MGE_MAIN_SPACE_AROUND,
    MGE_MAIN_SPACE_EVENLY,
} MgeMainAxisAlignment;

// alignment of each child across the other axis
typedef enum {
    MGE_CROSS_CENTER = 0,
    MGE_CROSS_START,
    MGE_CROSS_END,
    MGE_CROSS_STRETCH, // child is forced to the cross size
} MgeCrossAxisAlignment;

typedef enum {
    MGE_MAIN_SIZE_MAX = 0, // fill the available main extent
    MGE_MAIN_SIZE_MIN,     // shrink-wrap the children
} MgeMainAxisSize;

typedef enum { MGE_FLEX_TIGHT = 0, MGE_FLEX_LOOSE } MgeFlexFit;
typedef enum { MGE_STACK_LOOSE = 0, MGE_STACK_EXPAND } MgeStackFit;

typedef struct MgeFlexStyle {
    MgeMainAxisAlignment  mainAxis;
    MgeCrossAxisAlignment crossAxis;
    MgeMainAxisSize       mainSize;
    float                 spacing; // gap inserted between children
} MgeFlexStyle;

// "unset" for an Mge_UiPositioned edge / size (a real value is finite)
#define MGE_UI_NONE MGE_UI_INF

// ---- wrap / table (Phase 1b) --------------------------------------------

typedef enum {
    MGE_WRAP_START = 0,
    MGE_WRAP_END,
    MGE_WRAP_CENTER,
    MGE_WRAP_SPACE_BETWEEN,
    MGE_WRAP_SPACE_AROUND,
    MGE_WRAP_SPACE_EVENLY,
} MgeWrapAlignment;

typedef struct MgeWrapStyle {
    MgeAxis          axis;         // main axis of each run
    float            spacing;      // gap between children in a run
    float            runSpacing;   // gap between runs
    MgeWrapAlignment alignment;    // children within a run (main axis)
    MgeWrapAlignment runAlignment; // runs across the cross axis
} MgeWrapStyle;

typedef enum {
    MGE_COL_FLEX = 0,  // value = weight; splits the leftover width
    MGE_COL_FIXED,     // value = pixels
    MGE_COL_INTRINSIC, // widest cell in the column
} MgeTableColumnMode;

typedef struct MgeTableColumn {
    MgeTableColumnMode mode;
    float              value;
} MgeTableColumn;

// ---- handles ------------------------------------------------------------

typedef uint32_t MgeUiWidget; // opaque; 0 == null

// ---- lifecycle --------------------------------------------------------

void Mge_UiNewFrame(float dt);       // begin a frame (lazily boots the backend)
void Mge_UiRender(void);             // layout + paint the root, in 2D screen space
void Mge_UiShutdown(void);           // optional; the OS reclaims everything at exit

void Mge_UiSetRoot(MgeUiWidget root);
void Mge_UiViewport(float w, float h); // 0,0 => the screen size

// Gate your game's own mouse / key handling on these while a widget has the
// pointer / focus. Phase 0: always false (no interactive widgets yet).
bool Mge_UiWantsPointer(void);
bool Mge_UiWantsKeyboard(void);

// ---- tree -------------------------------------------------------------

void        Mge_UiDestroy(MgeUiWidget w); // frees the subtree; the handle goes stale
void        Mge_UiAddChild(MgeUiWidget parent, MgeUiWidget child);
void        Mge_UiRemoveChild(MgeUiWidget parent, MgeUiWidget child);
void        Mge_UiClearChildren(MgeUiWidget parent);
int         Mge_UiChildCount(MgeUiWidget w);
MgeUiWidget Mge_UiChildAt(MgeUiWidget w, int index);
MgeUiWidget Mge_UiParentOf(MgeUiWidget w);
bool        Mge_UiIsValid(MgeUiWidget w);

void Mge_UiMarkNeedsBuild(MgeUiWidget w);
void Mge_UiMarkNeedsLayout(MgeUiWidget w);
void Mge_UiMarkNeedsPaint(MgeUiWidget w);

// ---- widgets --------------------------------------------------------

MgeUiWidget Mge_UiContainer(MgeContainerStyle style);              // detached
MgeUiWidget Mge_UiLabel(MgeUiWidget parent, const char* text);     // default text style
MgeUiWidget Mge_UiText(MgeUiWidget parent, const char* text, MgeTextStyle style);

// --- layout (Phase 1). All detached -- Mge_UiAddChild them into a parent.
// Row / Column / Flex / Stack are multi-child; everything else takes one child
// (extra children are ignored -- use a Row/Column).
MgeUiWidget Mge_UiRow(MgeFlexStyle style);
MgeUiWidget Mge_UiColumn(MgeFlexStyle style);
MgeUiWidget Mge_UiFlex(MgeAxis axis, MgeFlexStyle style);
MgeUiWidget Mge_UiExpanded(int flex);                    // fill a share of the free main space (fit = tight)
MgeUiWidget Mge_UiFlexible(int flex, MgeFlexFit fit);
MgeUiWidget Mge_UiSpacer(int flex);                      // an Expanded with no child

MgeUiWidget Mge_UiSizedBox(float w, float h);            // 0 on an axis => size to child
MgeUiWidget Mge_UiCenter(void);
MgeUiWidget Mge_UiAlign(MgeAlignment alignment);
MgeUiWidget Mge_UiPadding(MgeEdgeInsets insets);
MgeUiWidget Mge_UiConstrainedBox(MgeUiConstraints constraints);

MgeUiWidget Mge_UiStack(MgeStackFit fit, MgeAlignment alignment);
MgeUiWidget Mge_UiPositioned(float left, float top, float right, float bottom,
    float width, float height);                          // MGE_UI_NONE = unset; child of a Stack
MgeUiWidget Mge_UiPositionedFill(void);

MgeUiWidget Mge_UiVisibility(bool visible);              // !visible => zero size, not painted
MgeUiWidget Mge_UiVisibilityMaintain(bool visible);     // !visible => keeps its size, still not painted
MgeUiWidget Mge_UiOffstage(bool offstage);              // == Mge_UiVisibility(!offstage)

// --- layout tail (Phase 1b) ---
MgeUiWidget Mge_UiWrap(MgeWrapStyle style);              // multi-child: children flow into runs
MgeUiWidget Mge_UiTable(const MgeTableColumn* cols, int nCols,
    float rowSpacing, float colSpacing);                 // multi-child: add Mge_UiTableRow rows
MgeUiWidget Mge_UiTableRow(void);                        // multi-child: cells (any widget)

MgeUiWidget Mge_UiIntrinsicWidth(void);                  // size the child to its max-content width
MgeUiWidget Mge_UiIntrinsicHeight(void);

MgeUiWidget Mge_UiAspectRatio(float ratio);             // largest w/h == ratio box that fits
MgeUiWidget Mge_UiFractionallySizedBox(float wFactor, float hFactor, MgeAlignment align); // 0 => unset
MgeUiWidget Mge_UiUnconstrainedBox(void);               // let the child pick any size
MgeUiWidget Mge_UiLimitedBox(float maxW, float maxH);   // caps only an axis that arrives unbounded

MgeUiWidget Mge_UiIndexedStack(int index);              // a Stack that paints only child `index`
void        Mge_UiSetStackIndex(MgeUiWidget w, int index);

// --- scrolling & clipping (Phase 2) ---

typedef struct MgeScrollStyle {
    bool  noScrollbar;         // hide the thumb (default: shown)
    float scrollbarThickness;  // 0 => 6 px
    Color trackColor;          // a == 0 => transparent
    Color thumbColor;          // a == 0 => subtle grey
} MgeScrollStyle;

MgeUiWidget Mge_UiScrollView(MgeAxis axis, MgeScrollStyle style); // single child, scrolls on `axis`
MgeUiWidget Mge_UiListView(MgeAxis axis, MgeScrollStyle style);   // scroll + an internal Row/Column
MgeUiWidget Mge_UiClipRect(void);                                 // single child, clipped to this rect

float Mge_UiScrollOffset(MgeUiWidget scrollView);
float Mge_UiScrollMax(MgeUiWidget scrollView);                    // contentExtent - viewExtent, >= 0
void  Mge_UiScrollTo(MgeUiWidget scrollView, float px);           // instant; clamped
void  Mge_UiScrollToEdge(MgeUiWidget scrollView, bool end);       // 0 or max
void  Mge_UiScrollToChild(MgeUiWidget scrollView, MgeUiWidget target); // bring it into view

// --- virtualization & grid (Phase 2b) ---

// runs during layout with this box's incoming constraints; add exactly one child
// to `slot`. Rebuilt every layout pass -- do NOT call Mge_UiRender from inside.
typedef void (*MgeUiLayoutCallback)(MgeUiWidget slot, MgeUiConstraints c, void* user);
MgeUiWidget Mge_UiLayoutBuilder(MgeUiLayoutCallback build, void* user);

// returns a detached widget for item `index`; called only for on-screen items,
// every layout pass. The returned handle is transient -- don't cache it.
typedef MgeUiWidget (*MgeUiItemBuilder)(int index, void* user);

// Virtualized list: only items whose line intersects the viewport (+1 line
// overscan) are built. Item `index` occupies a fixed `itemExtent` px on `axis`;
// the cross axis fills the viewport. Needs a bounded viewport (wrap in a box).
MgeUiWidget Mge_UiListViewBuilder(MgeAxis axis, int itemCount, float itemExtent,
    MgeUiItemBuilder build, void* user, MgeScrollStyle style);

// Virtualized grid: `crossAxisCount` cells per line, each `cellW` x `cellH`,
// `mainGap` / `crossGap` between them. Scrolls on `axis`.
MgeUiWidget Mge_UiGridViewBuilder(MgeAxis axis, int crossAxisCount, int itemCount,
    float cellW, float cellH, float mainGap, float crossGap,
    MgeUiItemBuilder build, void* user, MgeScrollStyle style);

// jump so item `index`'s line sits at the top / left of the viewport (clamped);
// use this instead of Mge_UiScrollToChild on a builder view
void Mge_UiScrollToIndex(MgeUiWidget builderView, int index);

// Non-virtual fixed-column grid (equal cells, like a Wrap). Multi-child; compose
// inside a Mge_UiScrollView to scroll it.
MgeUiWidget Mge_UiGridView(MgeAxis axis, int crossAxisCount,
    float cellW, float cellH, float mainGap, float crossGap);

// ---- interaction (Phase 3) -------------------------------------------
//
// Callbacks fire inside Mge_UiRender (from the input pass, after layout). The
// poll helpers (Mge_UiTapped / Clicked / ToggleChanged / SliderChanged) report
// the last Mge_UiRender -- read them after Mge_UiRender() and before the next
// Mge_UiNewFrame. Gate your game's own mouse handling on Mge_UiWantsPointer().

typedef struct MgeUiGestureInfo {
    Vector2 position;   // cursor, screen space
    Vector2 localPos;   // cursor relative to the widget's top-left
    Vector2 delta;      // movement since the last frame (pan)
    Vector2 totalDelta; // movement since the press began (pan)
} MgeUiGestureInfo;

typedef void (*MgeUiGestureFn)(const MgeUiGestureInfo* g, void* user);

// invisible; wraps one child and reports pointer gestures over it
MgeUiWidget Mge_UiGestureDetector(void);
void Mge_UiOnTap(MgeUiWidget w, MgeUiGestureFn cb, void* user);
void Mge_UiOnTapDown(MgeUiWidget w, MgeUiGestureFn cb, void* user);
void Mge_UiOnTapUp(MgeUiWidget w, MgeUiGestureFn cb, void* user);
void Mge_UiOnPanStart(MgeUiWidget w, MgeUiGestureFn cb, void* user);
void Mge_UiOnPanUpdate(MgeUiWidget w, MgeUiGestureFn cb, void* user);
void Mge_UiOnPanEnd(MgeUiWidget w, MgeUiGestureFn cb, void* user);
void Mge_UiOnHoverEnter(MgeUiWidget w, MgeUiGestureFn cb, void* user);
void Mge_UiOnHoverExit(MgeUiWidget w, MgeUiGestureFn cb, void* user);

bool Mge_UiTapped(MgeUiWidget w);  // poll: true the frame a tap completed
bool Mge_UiHovered(MgeUiWidget w);
bool Mge_UiPressed(MgeUiWidget w);

typedef enum {
    MGE_BTN_FILLED = 0,
    MGE_BTN_TONAL,
    MGE_BTN_OUTLINED,
    MGE_BTN_TEXT,
} MgeUiButtonVariant;

typedef struct MgeUiButtonStyle {
    MgeUiButtonVariant variant;
    Color              accent;  // a == 0 => default blue
    float              radius;  // 0 => 8
    MgeEdgeInsets      padding; // all 0 => symmetric(16, 10)
    float              textSize; // 0 => 16
    bool               expand;  // fill the available width
} MgeUiButtonStyle;

static inline MgeUiButtonStyle Mge_UiButtonFilled(Color a) { return (MgeUiButtonStyle){ .variant = MGE_BTN_FILLED, .accent = a }; }
static inline MgeUiButtonStyle Mge_UiButtonTonal(Color a) { return (MgeUiButtonStyle){ .variant = MGE_BTN_TONAL, .accent = a }; }
static inline MgeUiButtonStyle Mge_UiButtonOutlined(Color a) { return (MgeUiButtonStyle){ .variant = MGE_BTN_OUTLINED, .accent = a }; }
static inline MgeUiButtonStyle Mge_UiButtonText(Color a) { return (MgeUiButtonStyle){ .variant = MGE_BTN_TEXT, .accent = a }; }

MgeUiWidget Mge_UiButton(const char* label, MgeUiButtonStyle style);
void Mge_UiOnPressed(MgeUiWidget btn, MgeUiGestureFn cb, void* user);
bool Mge_UiButtonClicked(MgeUiWidget btn);   // poll
void Mge_UiSetEnabled(MgeUiWidget w, bool enabled);
void Mge_UiSetButtonLabel(MgeUiWidget btn, const char* label);

// toggles -- click flips the caller's variable; also poll Mge_UiToggleChanged
MgeUiWidget Mge_UiCheckbox(bool* value, Color accent);   // accent a == 0 => default blue
MgeUiWidget Mge_UiSwitch(bool* value, Color accent);
MgeUiWidget Mge_UiRadio(int* group, int value, Color accent); // selected when *group == value
bool Mge_UiToggleChanged(MgeUiWidget w);   // poll

MgeUiWidget Mge_UiSlider(float* value, float min, float max, float step); // step 0 => continuous
bool  Mge_UiSliderChanged(MgeUiWidget w);   // poll

MgeUiWidget Mge_UiProgressBar(float t01);
void  Mge_UiSetProgress(MgeUiWidget w, float t01);
float Mge_UiGetProgress(MgeUiWidget w);

// ---- text & focus (Phase 3b) ----------------------------------------
//
// A field edits a caller-owned MgeUiTextBuffer. Mge_UiWantsKeyboard() is true
// while any field is focused -- gate your game's key handling on it. Poll
// Mge_UiTextChanged / Submitted between Mge_UiRender() and the next NewFrame.

#define MGE_UI_TEXT_CAP 256
typedef struct MgeUiTextBuffer {
    char text[MGE_UI_TEXT_CAP];
    int  len;
} MgeUiTextBuffer;
void Mge_UiTextBufferSet(MgeUiTextBuffer* b, const char* s); // fill + clamp + NUL-terminate

typedef struct MgeUiTextFieldStyle {
    Color accent;    // caret + focus border; a == 0 => default blue
    Color bg;        // a == 0 => a dark default
    Color textColor; // a == 0 => white
    float textSize;  // 0 => 16
    float radius;    // 0 => 6
    int   maxLength; // 0 => MGE_UI_TEXT_CAP - 1
    bool  obscure;   // render as dots
    bool  expand;    // fill the available width (else 200)
} MgeUiTextFieldStyle;

MgeUiWidget Mge_UiTextField(MgeUiTextBuffer* buf, const char* placeholder, MgeUiTextFieldStyle style);
bool Mge_UiTextChanged(MgeUiWidget w);   // poll: text edited this frame
bool Mge_UiTextSubmitted(MgeUiWidget w); // poll: Enter pressed while focused
void Mge_UiFocus(MgeUiWidget w);
void Mge_UiUnfocus(void);
bool Mge_UiIsFocused(MgeUiWidget w);

void Mge_UiSetText(MgeUiWidget w, const char* text);
void Mge_UiSetContainerStyle(MgeUiWidget w, MgeContainerStyle style);
void Mge_UiSetVisible(MgeUiWidget w, bool visible);      // on an Mge_UiVisibility node

// the widget's computed screen rect after the last Mge_UiRender ({0} if unlaid)
Rectangle Mge_UiGetRect(MgeUiWidget w);

#ifdef __cplusplus
}
#endif
