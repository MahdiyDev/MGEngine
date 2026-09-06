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
} MgeContainerStyle;

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

void Mge_UiSetText(MgeUiWidget w, const char* text);
void Mge_UiSetContainerStyle(MgeUiWidget w, MgeContainerStyle style);

// the widget's computed screen rect after the last Mge_UiRender ({0} if unlaid)
Rectangle Mge_UiGetRect(MgeUiWidget w);

#ifdef __cplusplus
}
#endif
