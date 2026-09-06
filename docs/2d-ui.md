# Text & GUI

## Text (`mge_text.c`)

`stb_truetype` bakes ASCII 32..126 into one coverage atlas at a chosen pixel
height, keeping a per-glyph rect + placement offsets + advance (the
LearnOpenGL In-Practice/Text-Rendering approach, minus FreeType). The atlas is
uploaded so the default batch shader draws it as ordinary alpha-blended geometry
— there is **no dedicated text shader**.

```c
typedef struct Font {
    Texture2D atlas;      // coverage atlas (internal format)
    float     size;       // pixel height it was baked at (its natural size)
    float     ascent;     // px from the top of a line down to the baseline, at `size`
    float     lineAdvance;// px between baselines, at `size`
    int       first, count; // codepoint range (32, 95)
    void     *glyphs;     // internal
} Font;

Font    Mge_LoadFont(const char* fileName, int pixelHeight);   // .ttf / .otf (pak-aware)
Font    Mge_LoadFontFromMemory(const unsigned char* ttf, int ttfSize, int pixelHeight);
Font    Mge_GetDefaultFont(void);   // a built-in 8x8 bitmap font -- needs no file; don't unload it
bool    Mge_IsFontValid(Font font);
void    Mge_UnloadFont(Font font);

void    Draw_Text(Font font, const char* text, Vector2 pos, float fontSize, Color tint);
Vector2 Mge_MeasureText(Font font, const char* text, float fontSize); // {max line width, total height}
void    Draw_Text3D(Font font, const char* text, Vector3 pos, float size, Color tint);
```

`Draw_Text` works in the same **screen space as `Draw_Rectangle`** — pixel
coordinates, top-left origin, +Y down. `pos` is the top-left of the first line;
`'\n'` starts a new line; `fontSize` scales linearly from `font.size` (pass
`font.size` for 1:1 — sharpest for the bitmap font at integer multiples). It
enables alpha blending for its own draw and restores the previous state, and
flushes its batch, so it composes with any 2D drawing around it.

`Draw_Text3D` is the **world-space** form: a camera-facing billboard centred on
`pos`, with `size` the glyph cell height in world units. Call it inside
`Mge_BeginMode3D` (the billboard basis comes from the active view matrix); the
text depth-tests against the scene and blooms like any other geometry. It draws
unlit — a lighting pass, if active, is bypassed for its own draw.

```c
Font font = Mge_GetDefaultFont();               // or Mge_LoadFont("res/ui.ttf", 32)
// ... in the frame, after Mge_EndMode3D (or with no 3D at all):
Vector2 sz = Mge_MeasureText(font, label, 20);
Draw_Rectangle(x - 4, y - 2, (int)sz.x + 8, (int)sz.y + 4, (Color){ 40, 40, 55, 255 });
Draw_Text(font, label, (Vector2){ x, y }, 20, WHITE);
```

Demo: `examples/text/draw_text.c` (`MGE_FONT=path/to/font.ttf` for the scalable
half; it also shows `Draw_Text3D` on an orbiting camera). Tests:
`test/test_text.c` (metrics / measure / batch emission, hermetic) and the
`text` / `text3d` / `skybox` scenes in `make render`.


## GUI (`mge_gui.h`)

An immediate-mode UI abstracted over Dear ImGui — the backend is baked into
`libmgengine`, so consumers include `<mge_gui.h>` and call plain C. No ImGui
types leak out.

```c
#include <mge_gui.h>

Mge_GuiBeginFrame();                         // after the 3D/2D scene
    if (Mge_GuiBeginSidebar("Scene", 300, false)) {   // full-height dock on the left
        if (Mge_GuiSelectable("Cube 0", sel == 0)) sel = 0;
        Mge_GuiSeparator();
        Mge_GuiInputVec3("position", &obj.transform.position);   // "draw input" for a Vector3
        Mge_GuiInputColor("diffuse", &mat.maps[MATERIAL_MAP_DIFFUSE].color); // 8-bit RGBA swatch
        Mge_GuiSliderFloat("shininess", &mat.shininess, 1, 128);
        if (Mge_GuiImageButton("albedo", tex.id, 56.0f)) { /* open a file picker */ }
    }
    Mge_GuiEndSidebar();
Mge_GuiEndFrame();                           // renders on top of the framebuffer
```

| kind | calls |
| --- | --- |
| frame | `Mge_GuiBeginFrame` / `Mge_GuiEndFrame`, `Mge_GuiShutdown` |
| boxes | `Mge_GuiBeginBox` (floating panel) / `Mge_GuiBeginSidebar` (full-height edge dock) / `Mge_GuiBeginPanel` (exact screen rect, no title bar — for a docked shell) + matching `End*` |
| widgets | `Mge_GuiLabel`, `Mge_GuiSeparator`, `Mge_GuiSpacing`, `Mge_GuiSameLine`, `Mge_GuiSetNextItemWidth`, `Mge_GuiIndent` / `Unindent`, `Mge_GuiButton`, `Mge_GuiSelectable`, `Mge_GuiSelectableEx` (reports double-click), `Mge_GuiTreeNode` / `Mge_GuiTreePop`, `Mge_GuiImage` / `Mge_GuiImageButton` (id `0` → a "+" placeholder), `Mge_GuiLogBox` (read-only auto-scrolling text), `Mge_GuiBeginMenu` / `Mge_GuiMenuItem` / `Mge_GuiEndMenu` (button → popup menu) |
| modals | `Mge_GuiOpenPopup` (trigger) + `Mge_GuiBeginPopup` / `Mge_GuiEndPopup` (every frame) + `Mge_GuiClosePopup` (dismiss from inside) |
| inputs | `Mge_GuiCheckbox`, `Mge_GuiCombo` (dropdown), `Mge_GuiInputText`, `Mge_GuiInputInt/Float`, `Mge_GuiSliderFloat`, `Mge_GuiInputVec2/Vec3`, `Mge_GuiInputColor` (8-bit RGBA), `Mge_GuiInputColorRGB` (0..1 linear, e.g. `Light.color`) |
| drag & drop | `Mge_GuiDragSource(payload, label)` after a draggable widget, `Mge_GuiDropTarget(out, n)` after a drop target (string payloads); `Mge_GuiBeginContextMenu` / `Mge_GuiEndContextMenu` (right-click menu on the last widget) |
| layout | `Mge_GuiSplitter(id, x, y, w, h, vertical)` — an invisible draggable strip; returns the pixel drag delta along the split axis (for a resizable docked shell) |

Every input returns `true` the frame its value changes; `Mge_GuiSelectable` /
`Mge_GuiButton` return `true` on click. Gate your own picking and camera on
`Mge_GuiWantsMouse()` / `Mge_GuiWantsKeyboard()` so widgets don't fight the
viewport. The backend boots lazily on the first `Mge_GuiBeginFrame` after
`Mge_InitWindow`; apps that never call it pay nothing.

`editor/` is the worked example — a docked shell (top bar + left hierarchy +
right inspector + bottom resources) built from `Mge_GuiBeginPanel`, all in one
`Mge_GuiBeginFrame` / `Mge_GuiEndFrame` pair. See
[../editor/USAGE.md](../editor/USAGE.md).

## Widget GUI (`mge_ui.h`)

A second UI system, aimed at **game** menus / HUD chrome (the `Mge_Gui*` shim
above is editor-facing). Flutter's box model — Container / BoxDecoration /
EdgeInsets / alignment, constraints down and sizes up — but **retained and
handle-based**: a constructor takes a style struct *by value* and returns an
opaque `MgeUiWidget`; children attach by passing the parent handle first. The
toolkit owns the tree; you mutate it and it re-lays-out.

```c
#include <mge_ui.h>

// build once
MgeUiWidget root = Mge_UiContainer((MgeContainerStyle){
    .decoration = Mge_UiBoxDecoration((Color){ 18, 20, 28, 255 }),
    .alignment  = MGE_ALIGN_CENTER });              // fills the viewport, centres its child
MgeUiWidget card = Mge_UiContainer((MgeContainerStyle){
    .padding    = Mge_EdgeInsetsAll(24),
    .decoration = { .color = Mge_Colors.white,
        .border       = Mge_BorderAll((Color){ 120, 140, 200, 255 }, 3),
        .borderRadius = Mge_BorderRadiusAll(12) } });
Mge_UiAddChild(root, card);
MgeUiWidget label = Mge_UiLabel(card, "Label...");
Mge_UiSetRoot(root);

// each frame, in 2D screen space AFTER the scene
Mge_UiNewFrame((float)Mge_GetDeltaTime());
// ... Mge_UiSetText(label, ...) etc. when state changes ...
Mge_UiRender();
```

C, not Dart: styles are plain structs — use compound literals with designated
initialisers (`Mge_UiBoxDecoration(color)` is a convenience for the solid-fill
case). Any all-zero field means "no opinion" (`width 0` = size to child,
`constraints` all-0 = unconstrained). Defaults: text uses `Mge_GetDefaultFont()`
at 16 px / opaque white.

| kind | calls |
| --- | --- |
| lifecycle | `Mge_UiNewFrame(dt)` (lazy-boots; samples the mouse + typed characters) / `Mge_UiRender` / `Mge_UiShutdown`; `Mge_UiSetRoot`, `Mge_UiViewport(w,h)` (0,0 → screen size); `Mge_UiWantsPointer` (true over a scroll view or an interactive widget, or while a drag is active) / `Mge_UiWantsKeyboard` (true while a text field is focused) — gate your game's input on these |
| tree | `Mge_UiContainer(style)`, `Mge_UiLabel(parent, text)`, `Mge_UiText(parent, text, style)`; `Mge_UiAddChild` / `Mge_UiRemoveChild` / `Mge_UiClearChildren` / `Mge_UiChildCount` / `Mge_UiChildAt` / `Mge_UiParentOf` / `Mge_UiDestroy` (frees the subtree; the handle goes stale) / `Mge_UiIsValid` |
| layout | `Mge_UiRow` / `Mge_UiColumn` / `Mge_UiFlex(axis, MgeFlexStyle)` (main/cross alignment, `mainSize` MAX\|MIN, `spacing`); `Mge_UiExpanded(flex)` / `Mge_UiFlexible(flex, fit)` / `Mge_UiSpacer(flex)` inside a Row/Column; `Mge_UiCenter` / `Mge_UiAlign(a)` / `Mge_UiPadding(insets)` / `Mge_UiSizedBox(w,h)` / `Mge_UiConstrainedBox(c)`; `Mge_UiStack(fit, align)` + `Mge_UiPositioned(l,t,r,b,w,h)` (`MGE_UI_NONE` = unset) / `Mge_UiPositionedFill`; `Mge_UiVisibility(bool)` / `Mge_UiVisibilityMaintain` / `Mge_UiOffstage` + `Mge_UiSetVisible` |
| layout tail | `Mge_UiWrap(MgeWrapStyle)` (children flow into runs); `Mge_UiTable(cols, n, rowSp, colSp)` + `Mge_UiTableRow` (per-column `MGE_COL_FIXED` px / `_FLEX` weight / `_INTRINSIC` widest-cell); `Mge_UiIntrinsicWidth` / `Mge_UiIntrinsicHeight`; `Mge_UiAspectRatio(r)` / `Mge_UiFractionallySizedBox(wf, hf, align)` / `Mge_UiUnconstrainedBox` / `Mge_UiLimitedBox(maxW, maxH)`; `Mge_UiIndexedStack(index)` + `Mge_UiSetStackIndex` |
| scrolling | `Mge_UiScrollView(axis, MgeScrollStyle)` (one child, free on `axis`) / `Mge_UiListView(axis, style)` (items route into an internal Flex); needs a **bounded** viewport on the scroll axis — wrap it in a sized box. Mouse wheel when hovered, click-drag on the content, draggable thumb (hide via `MgeScrollStyle.noScrollbar`). `Mge_UiScrollOffset` / `Mge_UiScrollMax` / `Mge_UiScrollTo(px)` / `Mge_UiScrollToEdge(end)` / `Mge_UiScrollToChild(target)` (uses last frame's rects). `Mge_UiClipRect` clips its child to its own rect. |
| virtualization & grid | `Mge_UiListViewBuilder(axis, count, itemExtent, MgeUiItemBuilder, user, style)` / `Mge_UiGridViewBuilder(axis, crossCount, count, cellW, cellH, mainGap, crossGap, …)` — only the lines that intersect the viewport (+1 overscan) are built each pass; fixed line extent, no off-screen measurement. `Mge_UiScrollToIndex(view, i)` (item handles are transient — don't use `…ToChild`). `Mge_UiGridView(axis, crossCount, cellW, cellH, mainGap, crossGap)` — non-virtual fixed-column grid, multi-child; scroll it by composing inside a `Mge_UiScrollView`. `Mge_UiLayoutBuilder(MgeUiLayoutCallback, user)` — the callback runs **during layout** with the box's constraints and adds one child; rebuilt every pass, never call `Mge_UiRender` from it. |
| interaction | `Mge_UiGestureDetector()` (invisible, wraps one child) + `Mge_UiOn{Tap,TapDown,TapUp,TapCancel,DoubleTap,LongPress,PanStart,PanUpdate,PanEnd,HoverEnter,HoverExit}(w, cb, user)` (`cb(const MgeUiGestureInfo*, user)`) + poll `Mge_UiTapped` / `Hovered` / `Pressed`. `Mge_UiButton(label, MgeUiButtonStyle)` — `Mge_UiButton{Filled,Tonal,Outlined,Text}(accent)` presets; `Mge_UiOnPressed(btn, cb, user)` / `Mge_UiButtonClicked(btn)` poll / `Mge_UiSetEnabled` / `Mge_UiSetButtonLabel`. `Mge_UiCheckbox(bool*, accent)` / `Mge_UiSwitch(bool*, accent)` / `Mge_UiRadio(int* group, value, accent)` — click flips the bound var; `Mge_UiToggleChanged` poll. `Mge_UiSlider(float*, min, max, step)` (step 0 = continuous) + `Mge_UiSliderChanged`. `Mge_UiProgressBar(t01)` + `Mge_UiSetProgress` / `Mge_UiGetProgress`. `Mge_UiSetCursor(w, MgeMouseCursor)` (else buttons/dropdowns show a hand, fields an I-beam, automatically). Callbacks fire inside `Mge_UiRender`; read poll flags after `Mge_UiRender()` and before the next `Mge_UiNewFrame`. A press that lands on an interactive widget inside a scroll view does not drag-scroll (the wheel still does). |
| overlays | `Mge_UiDropdown(items, count, int* index, MgeUiDropdownStyle)` — a closed control that opens a floating list on click; sets `*index`, `Mge_UiDropdownChanged` poll, `Mge_UiDropdownOpen`. Click-away / Esc close it; while open it's modal (takes the pointer). `Mge_UiSegmentedControl(items, count, int* index, accent)` + `Mge_UiSegmentChanged` — an inline row of segments. `Mge_UiTooltip(text)` wraps one child and shows a hint box after the cursor rests ~0.5 s; `Mge_UiTooltipShowing()`. The dropdown list and the tooltip paint above the whole tree. |
| text & focus | `Mge_UiTextField(MgeUiTextBuffer* buf, placeholder, MgeUiTextFieldStyle)` — single-line editor over a caller-owned `MgeUiTextBuffer` (`Mge_UiTextBufferSet(buf, str)` to seed it); caret, selection, arrows / home / end, backspace / delete, Enter (poll `Mge_UiTextSubmitted`), Ctrl+A/C/X/V, Tab moves between fields, Esc unfocuses. Poll `Mge_UiTextChanged`. `Mge_UiFocus(w)` / `Mge_UiUnfocus()` / `Mge_UiIsFocused(w)`. `Mge_UiWantsKeyboard()` is true while any field is focused. |
| mutate | `Mge_UiSetText`, `Mge_UiSetContainerStyle`, `Mge_UiSetVisible`, `Mge_UiSetStackIndex`, `Mge_UiSetEnabled`, `Mge_UiMarkNeedsBuild/Layout/Paint`; `Mge_UiGetRect(w)` reads the laid-out screen rect |
| values | `Mge_Colors.<name>`, `Mge_EdgeInsets{All,Symmetric,LTRB}`, `Mge_Alignment(x,y)` + `MGE_ALIGN_*`, `Mge_ConstraintsTight/Loose`, `Mge_BorderAll`, `Mge_BorderRadiusAll`, `MgeFlexStyle`, `MgeWrapStyle`, `MgeTableColumn`, `MgeScrollStyle`, `MgeUiButtonStyle`, `MgeUiGestureInfo`, `MgeUiTextBuffer`, `MgeUiTextFieldStyle`, `MgeUiDropdownStyle`, `MgeMouseCursor` |

The **root is always laid out to fill the viewport** (like Flutter's
`RenderView`) — to size or place something, wrap it. **Row / Column / Stack /
Wrap / Table are multi-child**; every other node takes one child (extra children
are ignored). Flex is the standard model: inflexible children measure first,
then `Mge_UiExpanded` / `Spacer` split the leftover main-axis space by `flex`
factor. `Mge_UiCenter` / `Align` fill their box and place the child;
`Mge_UiPadding` / `SizedBox` / `ConstrainedBox` are thin `Container` presets.
Multi-line text areas, drag-and-drop, baseline alignment, fling/bounce scroll
physics, and rounded/shaped clips are still on the roadmap in
[../todo/todo_gui.md](../todo/todo_gui.md).

From a scene module, build the HUD in the optional `MgeScene_DrawGui(MgeSceneCtx*)`
export (see [scene.md](scene.md#hot-reloadable-scene-modules-mge_dylibc)) — the host calls `Mge_UiNewFrame` / `Mge_UiRender` around it.

