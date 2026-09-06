# PLAN — Retained widget GUI (`Mge_Gui*`, Flutter-shaped, C API)

A second UI system for **games** (menus, HUD chrome, inventories, settings),
separate from the current Dear ImGui shim. Flutter's vocabulary and box model
(Container / BoxDecoration / EdgeInsets / Row / Column / Flex / Align;
constraints down, sizes up), but a **C, handle-based** style: a constructor
takes a style struct by value and returns an opaque `MgeUiWidget`; children are
attached by passing the parent handle first.

```c
MgeContainerStyle st = { .decoration = Mge_GuiBoxDecoration(.color = Mge_Colors.white),
                         .padding    = Mge_EdgeInsetsAll(12) };
MgeUiWidget card = Mge_GuiContainer(st);
Mge_GuiLabel(card, "Label...");
```

Model: a **retained tree** the toolkit owns. Each frame `Mge_GuiNewFrame` /
`Mge_GuiRender` runs a box-constraint **layout** pass (constraints down, sizes
up, parent positions child), a **paint** pass that emits `Draw_Rectangle` /
`Draw_Text` / rounded-rect / `MgeGL_*` geometry in screen space, then
**hit-testing** for the pointer / key events fed in. It renders as a flat top
layer after the scene + HDR resolve, before `Mge_EndDrawing` (the slot the
ImGui shim already uses).

Decisions to settle in Phase 0:

- **Namespace.** This wants `Mge_Gui*`. The existing ImGui shim (`mge_gui.cpp`,
  `Mge_GuiBeginFrame` ...) must move to a distinct prefix first -- e.g.
  `Mge_Dbg*` / `Mge_EdGui*` -- or this system takes a different one (`Mge_Ui*`).
- **Scene-module hook.** `MgeScene_Draw` runs inside the lit/HDR pass, so a game
  module can't call the GUI from there. Add an optional 5th export
  `MgeScene_DrawGui(MgeSceneCtx*)`, called by `runtime/player.c` and editor Play
  mode between `Mge_GuiNewFrame` / `Mge_GuiRender` -- same shape as the
  `MgeScene_Draw` + sceneHook addition.
- **State model.** Ship both: imperative (mutate handles + `Mge_GuiMarkNeedsBuild`)
  and reactive (`Mge_GuiStateful` + `Mge_GuiSetState` rebuilds a subtree).
- **Language.** Pure C -- no new C++ TU. If the editor is later ported onto this,
  `vendor/imgui` + `mge_gui.cpp` can be dropped and the engine becomes
  single-language.

Every phase ships: an `examples/ui/<phase>.c` demo, `test/test_ui_*.c` (the
layout solver is hermetic -- pure math, no GL), and a `render_smoke`
`scene_ui_*`.

## Phase 0 -- core: tree, constraint layout, paint, the minimal snippet   [LANDED]

Landed as `Mge_Ui*` / `MgeUi*` (`source/mge_ui.{h,c}`) -- the ImGui shim keeps
`Mge_Gui*`. The root is always laid out tight to the viewport (Flutter's
RenderView model); Container is single-child (Row/Column are Phase 1).
`examples/ui/hello_ui.c`, `test/test_ui_layout.c`, the `ui` render-smoke scene.

- [x] Value types: reuse `Color`; `Mge_Colors` (const struct: `white` `black`
      `transparent` `red` `green` `blue` `yellow` `cyan` `magenta` `gray`
      `lightGray` `darkGray`); `MgeEdgeInsets` +
      `Mge_EdgeInsets{All,Symmetric,LTRB}`; `MgeAlignment` +
      `MGE_ALIGN_{CENTER,TOP_LEFT,...}` + `Mge_Alignment(x,y)` (-1..1);
      `MgeUiSize`, reuse `Rectangle` / `Vector2`; `MgeUiConstraints` +
      `Mge_Constraints{Tight,Loose,Unbounded}`.
- [x] `MgeBorder` + `Mge_BorderAll`; `MgeBorderRadius` + `Mge_BorderRadiusAll`;
      `MgeBoxDecoration` { color, border, borderRadius } + `Mge_UiBoxDecoration(color)`
      convenience (else compound literals -- C, not Dart named args);
      `MgeTextStyle` { font, size, color } (defaults: default font / 16 / white).
- [x] `MgeContainerStyle` { padding, margin, alignment, width, height,
      constraints, decoration }.
- [x] Lifecycle: `Mge_UiNewFrame(float dt)` (lazy-boots) / `Mge_UiRender(void)` /
      `Mge_UiShutdown`; `Mge_UiSetRoot` / `Mge_UiViewport` (0,0 = screen size);
      `Mge_UiWantsPointer` / `Mge_UiWantsKeyboard` (stubbed false until Phase 3).
- [x] Handle management: `MgeUiWidget` = uint32 (index<<8 | generation), 0 = null;
      `Mge_UiDestroy` / `Mge_UiAddChild` / `Mge_UiRemoveChild` /
      `Mge_UiClearChildren` / `Mge_UiChildCount` / `Mge_UiChildAt` /
      `Mge_UiParentOf` / `Mge_UiIsValid` / `Mge_UiMarkNeedsBuild|Layout|Paint`
      (one global dirty flag for now) / `Mge_UiGetRect`.
- [x] Widgets: `Mge_UiContainer(MgeContainerStyle) -> MgeUiWidget` (detached);
      `Mge_UiLabel(parent, text)`; `Mge_UiText(parent, text, MgeTextStyle)`;
      `Mge_UiSetText` / `Mge_UiSetContainerStyle`.
- [x] Paint primitives: `Draw_RectangleRounded[Lines]` (`mge_shapes.c`, raylib
      signatures; MSAA covers edge AA); `MgeGL_EnableScissor` /
      `MgeGL_DisableScissor` (`mge_gl.c`, top-left coords, flipped internally).
- [x] `MgeSceneDrawGuiFn` + `MgeScene_DrawGui(MgeSceneCtx*)` export;
      `SceneRuntime_DrawGui` (`editor/scene_runtime.{h,c}`); `runtime/player.c` +
      `editor/play.{h,c}` + `editor/main.c` call it between `Mge_UiNewFrame` and
      `Mge_UiRender`, after the scene.

## Phase 1 -- core layout widgets   [LANDED]

Detached constructors (like `Mge_UiContainer`) -- `Mge_UiAddChild` them in.
Row/Column/Stack are multi-child; the rest take one child. Two-pass flex
(inflexible measured, then Expanded/Spacer split the leftover by factor), a
`bool expand` on `MgeContainerStyle` for Center/Align. `test/test_ui_layout.c`
(+10 cases), `render_smoke` `ui` scene, `examples/ui/menu.c`.

- [x] Enums: `MgeAxis`, `MgeMainAxisAlignment` (start/end/center/spaceBetween/
      spaceAround/spaceEvenly), `MgeCrossAxisAlignment` (center/start/end/stretch),
      `MgeMainAxisSize` (max/min), `MgeFlexFit` (tight/loose), `MgeStackFit`
      (loose/expand). `MgeFlexStyle` { mainAxis, crossAxis, mainSize, spacing }.
- [x] `Mge_UiRow(MgeFlexStyle)` / `Mge_UiColumn(MgeFlexStyle)` /
      `Mge_UiFlex(MgeAxis, MgeFlexStyle)`.
- [x] `Mge_UiExpanded(int flex)` / `Mge_UiFlexible(int flex, MgeFlexFit)` /
      `Mge_UiSpacer(int flex)`.
- [x] `Mge_UiSizedBox(float w, float h)` (0 on an axis = pass through);
      `Mge_UiCenter()` / `Mge_UiAlign(MgeAlignment)` / `Mge_UiPadding(MgeEdgeInsets)` /
      `Mge_UiConstrainedBox(MgeUiConstraints)`.
- [x] `Mge_UiStack(MgeStackFit, MgeAlignment)` /
      `Mge_UiPositioned(l, t, r, b, w, h)` (`MGE_UI_NONE` = unset) /
      `Mge_UiPositionedFill()`.
- [x] `Mge_UiVisibility(bool)` + `Mge_UiSetVisible` (`!visible` -> zero size, not
      painted).

## Phase 1b -- layout tail   [MOSTLY LANDED]

Each is a self-contained sub-algorithm. `test/test_ui_layout.c` (+8 cases),
`render_smoke` `ui2` scene, `examples/ui/menu.c` stats table.

- [x] `Mge_UiWrap(MgeWrapStyle { axis, spacing, runSpacing, alignment,
      runAlignment })` -- children flow into runs; `MgeWrapAlignment` enum.
- [x] `Mge_UiTable(const MgeTableColumn* cols, int nCols, rowSpacing, colSpacing)`
      + `Mge_UiTableRow()` -- per-column `MGE_COL_FIXED` (px) / `_FLEX` (weight) /
      `_INTRINSIC` (widest dry-measured cell). Cells are any widget.
- [x] `Mge_UiIntrinsicWidth()` / `Mge_UiIntrinsicHeight()` -- dry-measure the
      child unbounded, then re-lay-out tight to that content size.
- [x] `Mge_UiAspectRatio(float ratio)` /
      `Mge_UiFractionallySizedBox(float wf, float hf, MgeAlignment)` (0 = unset).
- [x] `Mge_UiUnconstrainedBox()` / `Mge_UiLimitedBox(maxW, maxH)` (caps only an
      axis that arrives unbounded).
- [x] `Mge_UiIndexedStack(int index)` + `Mge_UiSetStackIndex` (lays out all
      children, paints one); `Mge_UiOffstage(bool)`;
      `Mge_UiVisibilityMaintain(bool)` (hidden but keeps its size).
- [x] `Mge_UiLayoutBuilder(MgeUiLayoutCallback, void*)` -- landed in **Phase 2b**
      (with the re-entrancy audit that also unblocked the virtualized builders).
- [ ] `Mge_UiBaseline` + `MGE_CROSS_BASELINE` + `MgeTextBaseline` -- deferred:
      needs first-line-baseline propagation through every `layout_*` return.
- [ ] `OverflowBox` / `SizedOverflowBox` -- not planned (deliberately breaking
      layout bounds is an anti-pattern). `MgeVerticalDirection` -- not planned
      (add children in reverse order instead).

## Phase 2 -- scrolling & viewports   [CORE LANDED]

Landed: engine mouse-wheel (`GetMouseWheelMove` / `GetMouseWheelMoveV`,
`CoreData.Input.Mouse.currentWheelMove` + a GLFW scroll callback), the core
scroll views, rect clipping, a real `Mge_UiWantsPointer`. `test/test_ui_layout.c`
(+9 cases, 114 checks), `render_smoke` `ui_scroll` scene, `examples/ui/menu.c`
scrolling options list. Deferred work moved to Phase 2b below.

- [x] Engine: `float GetMouseWheelMove(void)` (dominant axis, raylib sign) /
      `Vector2 GetMouseWheelMoveV(void)`. Accumulated per frame, zeroed each poll.
- [x] `Mge_UiScrollView(MgeAxis axis, MgeScrollStyle)` -- one child, free to grow
      on `axis`; needs a bounded viewport on that axis (else falls back to
      passthrough). `MgeScrollStyle { bool noScrollbar; float scrollbarThickness;
      Color trackColor, thumbColor; }` (all-zero = thumb shown, 6 px, subtle grey).
- [x] `Mge_UiListView(MgeAxis axis, MgeScrollStyle)` -- a scroll view whose sole
      child is an internal Flex (`mainSize` MIN, cross STRETCH) on the same axis;
      `Mge_UiAddChild(listView, item)` routes items into it.
- [x] Input: mouse wheel while the cursor is over the view, click-drag on the
      content, and a draggable scrollbar thumb (track + rounded thumb sized
      `view/content` of the track). `Mge_UiWantsPointer()` true while hovered or
      dragging. Hit-test walks the tree respecting visibility / IndexedStack /
      scroll & clip bounds.
- [x] Clipping: `Mge_UiClipRect()` + a 16-deep scissor stack in the paint pass
      (`MgeGL_EnableScissor` intersect-and-push, disable on unwind). `NODE_SCROLL`
      and `NODE_CLIPRECT` clip their children; the thumb paints outside the clip.
- [x] Scroll query / control: `Mge_UiScrollOffset` / `Mge_UiScrollMax` /
      `Mge_UiScrollTo(px)` / `Mge_UiScrollToEdge(bool end)` /
      `Mge_UiScrollToChild(target)` -- all instant, clamped, on the scroll handle
      directly (no separate controller). `…ToChild` uses last frame's rects.

## Phase 2b -- deferred scrolling work   [PARTLY LANDED]

Landed: the re-entrancy-gated layout work. Audited every `layout_*` for the
index-only invariant (no cached `Node*` across a `layout_node` call) and wrote
it up as a banner comment; builders route through the existing index-safe
helpers. `test/test_ui_layout.c` (+7, 137 checks), `render_smoke` `ui_vlist`
scene, `examples/ui/list.c` (10k-row demo).

- [x] `Mge_UiLayoutBuilder(MgeUiLayoutCallback build, void* user)` -- the
      callback runs *during* layout with this box's incoming constraints and
      adds one child; rebuilt every pass.
- [x] `Mge_UiListViewBuilder(MgeAxis axis, int itemCount, float itemExtent,
      MgeUiItemBuilder build, void* user, MgeScrollStyle)` /
      `Mge_UiGridViewBuilder(axis, crossAxisCount, itemCount, cellW, cellH,
      mainGap, crossGap, …)` -- virtualized: only lines intersecting the
      viewport (+1 overscan) are built. Fixed line extent => no off-screen
      measurement, so the re-entrancy surface stays small. `NODE_SCROLL`
      `kind` field; the built window is plain children (clip / thumb / wheel /
      drag reuse the Phase 2 code untouched).
- [x] `Mge_UiScrollToIndex(view, int index)` -- jump the line to the top;
      replaces `…ToChild` for builders (item handles are transient).
- [x] Non-virtual `Mge_UiGridView(MgeAxis axis, int crossAxisCount, float cellW,
      float cellH, float mainGap, float crossGap)` -- fixed-column grid,
      multi-child; compose inside a `Mge_UiScrollView` to scroll it.
- [ ] `MgeScrollController` (separate handle) + `Mge_ScrollController(void)` /
      `Mge_ScrollOffset(ctl)` / `Mge_ScrollExtent(ctl)`. **Deferred:** the scroll
      view is its own controller for now; a shared handle only earns its keep
      with `NestedScrollView` (below), which needs it to link an outer + inner
      scrollable.
- [ ] `MgeScrollPhysics` (clamping / bouncing) + overscroll glow + fling
      momentum. **Deferred to Phase 6** (animation): needs a per-frame velocity
      integrator + a relayout-while-idle tick, the same machinery as animated
      `Mge_UiScrollTo`. The current drag is a hard clamp, fine for menus / panels.
- [ ] Animated `Mge_UiScrollTo(px, durationSec, MgeCurve)` /
      `Mge_ScrollAnimateToEdge`. **Deferred to Phase 6** (the animation /
      transition system) -- `Mge_UiNewFrame` already carries `dt` for it.
- [ ] `Mge_UiClipRRect(MgeBorderRadius)` / `Mge_UiClipOval` / `Mge_UiClipPath`.
      **Deferred:** rectangular scissor can't do these -- needs a stencil pass or
      SDF/path clip, a renderer feature of its own.
- [ ] `Mge_UiNestedScrollView` / `Mge_UiScrollNotification` hook. **Deferred:**
      depends on `MgeScrollController` and the Phase 3 event model.
- [ ] `Mge_UiSafeArea(parent)` (applies `MgeMediaQuery` view insets).
      **Deferred to Phase 7** (MediaQuery).

## Phase 3 -- input & interactive widgets

- [ ] Events: `MgePointerEvent` (kind down/up/move/hover/scroll/cancel,
      position, delta, buttons, scrollDelta); `MgeKeyEvent` (key, mods,
      down/up/repeat); `MgeTextInputEvent` (utf-8). Feed via `Mge_GuiFeedInput`
      (pulls from the engine) or explicit `Mge_GuiPushPointer/Key/Text`.
- [ ] `Mge_GuiGestureDetector(parent)` + setters: `Mge_GuiOnTap` / `OnTapDown` /
      `OnTapUp` / `OnTapCancel` / `OnDoubleTap` / `OnLongPress` /
      `OnSecondaryTap` / `OnPanStart` / `OnPanUpdate` / `OnPanEnd` /
      `OnScaleStart/Update/End` / `OnHoverEnter` / `OnHoverExit` / `OnHoverMove`;
      each: `(MgeUiWidget, void (*cb)(const MgeGestureInfo*, void*), void* user)`.
- [ ] `Mge_GuiMouseRegion(parent, MgeMouseCursor)` /
      `Mge_GuiSetCursor(MgeUiWidget, MgeMouseCursor)` (arrow/hand/text/resize*/
      grab/forbidden).
- [ ] `MgeButtonStyle` + presets `Mge_ButtonStyle_{Filled,Tonal,Outlined,Text,
      Elevated}`; `Mge_GuiButton(parent, MgeButtonStyle) -> MgeUiWidget` +
      `Mge_GuiOnPressed(btn, cb, user)` + `Mge_GuiWasPressed(btn)` (poll) +
      `Mge_GuiSetEnabled(btn, bool)`; `Mge_GuiIconButton(parent, int iconId,
      MgeButtonStyle)`; `Mge_GuiFloatingActionButton`.
- [ ] `Mge_GuiInkWell(parent)` / `Mge_GuiInkResponse` (ripple, splash + hover +
      focus overlays).
- [ ] `Mge_GuiCheckbox(parent, bool* value)` / `Mge_GuiTristateCheckbox` /
      `Mge_GuiSwitch(parent, bool* value)` /
      `Mge_GuiRadio(parent, int* group, int value)`.
- [ ] `Mge_GuiSlider(parent, float* value, float min, float max)` +
      `Mge_GuiSliderDivisions` / `Mge_GuiRangeSlider(parent, float* lo, float*
      hi, float min, float max)`.
- [ ] `Mge_GuiProgressBar(parent, float value01)` /
      `Mge_GuiProgressCircle(parent, float value01)` /
      `Mge_GuiProgressIndeterminate`.
- [ ] `Mge_GuiDropdown(parent, const char* const* items, int count, int* index)`;
      `Mge_GuiSegmentedControl(parent, items, count, int* index)`;
      `Mge_GuiChoiceChip` / `Mge_GuiFilterChip` / `Mge_GuiInputChip`.
- [ ] `MgeTextController` + `Mge_TextController(const char* initial)` /
      `Mge_TextControllerText` / `Mge_TextControllerSetText` /
      `Mge_TextControllerSelection` / `Mge_TextControllerClear`.
- [ ] `Mge_GuiTextField(parent, MgeTextController, MgeTextFieldStyle)`
      (placeholder, `obscure`, `maxLength`, `keyboardType`, prefix/suffix,
      `onChanged`, `onSubmitted`); `Mge_GuiTextArea` (multi-line);
      `Mge_GuiSelectableText` (read-only, copyable).
- [ ] Focus: `MgeFocusNode` + `Mge_FocusNode(void)` /
      `Mge_GuiFocus(parent, MgeFocusNode)` / `Mge_GuiRequestFocus` /
      `Mge_GuiFocusScope` / `Mge_GuiAutofocus` /
      `Mge_GuiFocusTraversalGroup(parent, MgeTraversalPolicy)`.
- [ ] `Mge_GuiShortcuts(parent, const MgeShortcut* binds, int n)` /
      `Mge_GuiActions` / `Mge_GuiCallbackShortcut(parent, MgeKeyChord, cb, user)`.
- [ ] `Mge_GuiTooltip(parent, const char* text, MgeTooltipStyle)`
      (hover delay, follow-cursor, rich variant).
- [ ] `Mge_GuiDraggable(parent, const char* payload, MgeUiWidget feedback)` /
      `Mge_GuiLongPressDraggable` /
      `Mge_GuiDragTarget(parent, bool (*onWillAccept)(const char*, void*),
      void (*onAccept)(const char*, void*), void* user)`.
- [ ] `Mge_GuiDismissible(parent, MgeAxis, void (*onDismissed)(void*), void*)` /
      `Mge_GuiReorderableList(parent, void (*onReorder)(int from, int to,
      void*), void*)`.

## Phase 4 -- visual styling depth (paint)

- [ ] `MgeGradient` + `Mge_LinearGradient(MgeAlignment begin, MgeAlignment end,
      const Color* colors, const float* stops, int n)` /
      `Mge_RadialGradient(center, radius, colors, stops, n)` / `Mge_SweepGradient`.
- [ ] `MgeBoxShadow` + `Mge_BoxShadow(Color, MgeUiOffset, float blur, float
      spread)` (list on `MgeBoxDecoration`); `Mge_GuiPhysicalModel(parent, float
      elevation, MgeBorderRadius, Color shadowColor)`.
- [ ] `MgeDecorationImage` (textureId, `MgeBoxFit`, alignment, repeat,
      `centerSlice` = nine-patch, color/tint, opacity, blendMode) on
      `MgeBoxDecoration`; `MgeBoxFit` enum
      (fill/contain/cover/fitWidth/fitHeight/none/scaleDown).
- [ ] Per-corner + elliptical `MgeBorderRadius`; per-side `MgeBorder`
      (width/color/style solid|dashed|dotted).
- [ ] `Mge_GuiFittedBox(parent, MgeBoxFit, MgeAlignment)`.
- [ ] `MgeImageStyle` + `Mge_GuiImage(parent, unsigned textureId, MgeImageStyle)`
      (fit, alignment, color/tint, blendMode, opacity);
      `Mge_GuiImageNinePatch(parent, unsigned textureId, MgeEdgeInsets slice)`.
- [ ] Icon fonts: `Mge_LoadIconFont(const char* ttf, int pixelHeight)` +
      codepoint constants; `Mge_GuiIcon(parent, int codepoint, float size, Color)`.
- [ ] `Mge_GuiOpacity(parent, float opacity)` /
      `Mge_GuiColorFiltered(parent, Color, MgeBlendMode)` /
      `Mge_GuiShaderMask(parent, MgeGradient)`.
- [ ] `Mge_GuiTransform(parent, Matrix)` / `Mge_GuiRotate(parent, float rad)` /
      `Mge_GuiScale(parent, float sx, float sy)` /
      `Mge_GuiTranslate(parent, Vector2)` /
      `Mge_GuiTransformOrigin(parent, MgeAlignment)`.
- [ ] `Mge_GuiDecoratedBox(parent, MgeBoxDecoration, bool foreground)`.
- [ ] Rich text: `MgeFontWeight`, `MgeTextDecoration` (underline/overline/
      lineThrough), `MgeTextAlign`, `MgeTextOverflow` (clip/fade/ellipsis/
      visible), `maxLines`, `softWrap`, `letterSpacing`, `wordSpacing`,
      `lineHeight`; `MgeTextSpan` + `Mge_TextSpan(text, MgeTextStyle, children,
      n)` + `Mge_GuiRichText(parent, MgeTextSpan root)`;
      `Mge_GuiDefaultTextStyle(parent, MgeTextStyle)` (inherited down the tree).
- [ ] `Mge_GuiBackdropFilter(parent, float blurSigma)` (frosted glass -- needs a
      read of the framebuffer behind the widget; may slip to Phase 8).

## Phase 5 -- overlays, routes, dialogs

- [ ] `Mge_GuiOverlayInsert(MgeUiWidget) -> MgeOverlayEntry` /
      `Mge_GuiOverlayRemove(MgeOverlayEntry)` / `Mge_GuiOverlayMarkDirty`
      (a z-ordered layer stack above the root).
- [ ] `Mge_GuiModalBarrier(Color, bool dismissible, void (*onDismiss)(void*),
      void*)`.
- [ ] `Mge_GuiShowDialog(MgeUiWidget content, bool barrierDismissible) ->
      MgeRoute` / `Mge_GuiAlertDialog(const char* title, MgeUiWidget body, const
      MgeDialogAction* actions, int n)` / `Mge_GuiDismissDialog(MgeRoute)`.
- [ ] `Mge_GuiShowModalBottomSheet(MgeUiWidget content)` /
      `Mge_GuiPersistentBottomSheet` /
      `Mge_GuiDraggableScrollableSheet(minRatio, maxRatio, initRatio)`.
- [ ] `Mge_GuiSnackBar(const char* text, MgeSnackAction action, float
      durationSec)` + an internal queue.
- [ ] `Mge_GuiMenuAnchor(parent)` / `Mge_GuiPopupMenu(MgeUiWidget anchor, const
      MgeMenuItem* items, int n)` / `Mge_GuiContextMenu(parent, items, n)`
      (right-click) / `Mge_GuiDropdownButton`.
- [ ] `Mge_GuiNavigator` -- `Mge_GuiPush(MgeRoute)` / `Mge_GuiPop(void)` /
      `Mge_GuiPushReplacement` / `Mge_GuiPopUntil(const char* name)` /
      `Mge_GuiCanPop`; named: `Mge_GuiRegisterRoute(const char* name, MgeUiWidget
      (*build)(void*), void*)` + `Mge_GuiPushNamed(const char*)`.
- [ ] `MgeRouteTransition` (fade/slide{Left,Right,Up,Down}/scale/rotation/none) +
      `Mge_GuiPageRoute(MgeUiWidget (*build)(void*), void*, MgeRouteTransition,
      float durationSec, MgeCurve)`.
- [ ] `Mge_GuiPopScope(parent, bool canPop, void (*onPopInvoked)(void*), void*)`
      (back-button / Esc intercept).
- [ ] `Mge_GuiHero(parent, const char* tag)` (shared-element transition between
      routes).
- [ ] Optional shell: `Mge_GuiScaffold` (appBar / body / bottomNav / drawer /
      FAB) / `Mge_GuiDrawer` / `Mge_GuiTabBar` + `Mge_GuiTabView` +
      `MgeTabController` / `Mge_GuiStepper` / `Mge_GuiExpansionTile`.

## Phase 6 -- animation & motion

- [ ] `MgeCurve` + `Mge_Curves` (linear, easeIn/Out/InOut, fastOutSlowIn,
      decelerate, bounceOut, elasticOut, ...) + `Mge_CubicBezierCurve(x1,y1,x2,y2)`
      + `Mge_CurveEval(MgeCurve, float t)`.
- [ ] `MgeAnimationController` + `Mge_AnimationController(float durationSec)` /
      `Mge_AnimForward` / `Mge_AnimReverse` / `Mge_AnimRepeat(bool reverse)` /
      `Mge_AnimStop` / `Mge_AnimReset` / `Mge_AnimValue` / `Mge_AnimStatus`
      (dismissed/forward/reverse/completed) / `Mge_AnimOnStatusChange` /
      `Mge_AnimFling(velocity)`. Ticked from `Mge_GuiNewFrame(dt)`.
- [ ] `MgeTween` -- `Mge_TweenFloat(a,b)` / `Mge_TweenColor` / `Mge_TweenRect` /
      `Mge_TweenAlignment` / `Mge_TweenEdgeInsets` / `Mge_TweenBorderRadius` /
      `Mge_TweenDecoration`; `Mge_TweenEval(MgeTween, MgeCurve, float t)`;
      `Mge_TweenSequence(items, n)`.
- [ ] Explicit transitions (a controller drives them): `Mge_GuiFadeTransition` /
      `SlideTransition` / `ScaleTransition` / `RotationTransition` /
      `SizeTransition` / `AlignTransition` / `PositionedTransition` /
      `DecoratedBoxTransition`; `Mge_GuiAnimatedBuilder(parent,
      MgeAnimationController, MgeUiWidget (*build)(float v, void*), void*)`.
- [ ] Implicit animations (animate on style change):
      `Mge_GuiAnimatedContainer(parent, MgeContainerStyle, float durationSec,
      MgeCurve)` / `AnimatedOpacity` / `AnimatedAlign` / `AnimatedPadding` /
      `AnimatedPositioned` / `AnimatedSize` / `AnimatedDefaultTextStyle` /
      `AnimatedSwitcher` / `AnimatedCrossFade` /
      `Mge_GuiTweenAnimationBuilder(parent, MgeTween, float durationSec,
      MgeUiWidget (*build)(float v, void*), void*)`.
- [ ] `Mge_GuiAnimatedList` (`insertItem` / `removeItem` with an animation
      builder).
- [ ] `Mge_SpringSimulation(mass, stiffness, damping)` + `Mge_GuiSpringController`.
- [ ] `Mge_GuiTicker(void (*tick)(float dt, void*), void*)` (raw per-frame
      callback, no widget).

## Phase 7 -- theming, state, media query

- [ ] `MgeColorScheme` (primary/secondary/tertiary/surface/background/error +
      `onX`, plus outline/shadow); `Mge_ColorScheme_Light` / `_Dark` /
      `Mge_ColorSchemeFromSeed(Color seed, MgeBrightness)`.
- [ ] `MgeTextTheme` -- named roles (displayLarge...labelSmall) -> `MgeTextStyle`.
- [ ] `MgeUiThemeData` + `Mge_ThemeData(.colorScheme,.textTheme,.defaultRadius,
      .defaultPadding,.elevation,...)`; `Mge_GuiTheme(parent, MgeUiThemeData)`;
      `Mge_GuiThemeOf(void)` (nearest ancestor theme);
      `Mge_GuiThemeExtension(key, data)`.
- [ ] `MgeMediaQueryData` + `Mge_GuiMediaQuery(void)` (size, devicePixelRatio,
      textScaleFactor, padding / viewInsets / viewPadding, platformBrightness,
      disableAnimations, boldText); `Mge_GuiMediaQueryOverride(parent, ...)`.
- [ ] State: imperative (`Mge_GuiMarkNeedsBuild` + handle setters) AND reactive
      `Mge_GuiStateful(parent, MgeUiWidget (*build)(void* state), void* state)` +
      `Mge_GuiSetState(void* state)` (rebuilds that node);
      `MgeValueNotifier` + `Mge_ValueNotifier(void)` /
      `Mge_GuiValueListenableBuilder(parent, MgeValueNotifier, MgeUiWidget
      (*build)(void*), void*)`.
- [ ] Dependency injection down the tree:
      `Mge_GuiProvide(parent, const char* key, void* value)` /
      `Mge_GuiConsume(MgeUiWidget ctx, const char* key)` /
      `Mge_GuiInheritedBuilder`.
- [ ] `MgeUiKey` -- stable identity across rebuilds:
      `Mge_GuiKeyString(const char*)` / `Mge_GuiValueKey(int)` /
      `Mge_GuiGlobalKey(void)` + `Mge_GuiFindByKey(MgeUiKey)`.
- [ ] `Mge_GuiBuilder(parent, MgeUiWidget (*build)(MgeUiWidget ctx, void*),
      void*)` (deferred build with context).
- [ ] `MgeTextDirection` (LTR/RTL) + `MgeEdgeInsetsDirectional` /
      `MgeAlignmentDirectional`; `Mge_GuiSetLocale(const char*)` /
      `Mge_GuiTr(const char* key)` (string table).
- [ ] `Mge_GuiSemantics(parent, const char* label, const char* hint, MgeRole)`
      (metadata for automation / screen readers / test harness).

## Phase 8 -- canvas, custom painting, extensibility

- [ ] `MgeCanvas` handle + ops: `Mge_CanvasSave` / `Restore` / `SaveLayer` /
      `ClipRect` / `ClipRRect` / `ClipPath` / `Translate` / `Rotate` / `Scale` /
      `Transform`; `DrawRect` / `DrawRRect` / `DrawCircle` / `DrawOval` /
      `DrawArc` / `DrawLine` / `DrawPoints` / `DrawPath` / `DrawShadow` /
      `DrawImage` / `DrawImageRect` / `DrawImageNine` / `DrawParagraph` /
      `DrawColor` / `DrawVertices`.
- [ ] `MgePaint` (color, style fill|stroke, strokeWidth, strokeCap, strokeJoin,
      blendMode, shader = gradient, maskFilter = blur sigma, antiAlias).
- [ ] `MgePath` -- `Mge_PathMoveTo` / `LineTo` / `CubicTo` / `QuadTo` / `ArcTo` /
      `AddRect` / `AddRRect` / `AddOval` / `AddPolygon` / `Close`;
      `Mge_PathCombine(op union|diff|intersect|xor)` / `Mge_PathContains(pt)` /
      `Mge_PathBounds` / `Mge_PathMetrics` (length, sample position + tangent).
- [ ] `Mge_GuiCustomPaint(parent, void (*paint)(MgeCanvas, MgeUiSize, void*),
      void* user, bool foreground)`.
- [ ] `Mge_GuiCustomSingleChildLayout(parent, MgeLayoutDelegate)` /
      `Mge_GuiCustomMultiChildLayout(parent, MgeMultiLayoutDelegate)`.
- [ ] `Mge_GuiRepaintBoundary(parent)` (cache the subtree to a texture) +
      `Mge_GuiCaptureToTexture(MgeUiWidget) -> Texture2D` (thumbnails, Hero
      snapshots).
- [ ] `MgeTextPainter` -- `Mge_TextPainterLayout(text, MgeTextStyle, float
      maxWidth)` / `...Size` / `...LineCount` / `...OffsetForPosition` (measure +
      lay out text outside a widget; wraps `Mge_MeasureText`).
- [ ] Custom widget authoring: `Mge_GuiLeaf(MgeMeasureFn, MgePaintFn, MgeHitFn,
      void* user)` / `Mge_GuiRegisterWidget(MgeWidgetVTable)`.
- [ ] In-world UI: `Mge_GuiWidgetToTexture(MgeUiWidget) -> Texture2D` for use
      with `Draw_Quad3D` (diegetic panels / screens on objects; also lets the
      GUI live inside the lit pass when a game wants that).

## Phase 9 -- tooling, debug, controller nav, migration

- [ ] `Mge_GuiDebugDumpTree(void)` / `Mge_GuiDebugDumpLayoutTree` /
      `Mge_GuiDebugDumpFocusTree`.
- [ ] `Mge_GuiDebugPaintBounds(bool)` / `...PaintBaselines(bool)` /
      `...PaintPointers(bool)` / `...RepaintRainbow(bool)`.
- [ ] `Mge_GuiPerfOverlay(bool)` (build / layout / paint ms, widget count,
      rebuilds per frame, overdraw); `MgeGuiFrameStats` struct.
- [ ] `Mge_GuiInspector(bool)` -- click a pixel, highlight the widget, show its
      props (built with the toolkit itself).
- [ ] Controller / keyboard navigation: `MgeGuiNavFlags` (enable dpad/keyboard),
      directional focus movement between focusables, `Mge_GuiNavHighlight` ring,
      an on-screen keyboard for `Mge_GuiTextField` on gamepad.
- [ ] `test/test_ui_layout.c` (constraint solver: Row/Column/Flex/Expanded/
      Stack/Wrap/intrinsics -- pure math, hermetic) + `test/test_ui_hit.c`
      (hit-testing + event routing, stubbed input).
- [ ] Migration: settle the namespace (rename the ImGui shim to `Mge_Dbg*` /
      `Mge_EdGui*`, or give this system its own prefix). Optionally port the
      editor panels (`hierarchy` / `inspector` / `topbar` / `resources`) onto
      this toolkit, then drop `vendor/imgui` + `mge_gui.cpp` -> the engine is
      single-language C.
